/*
 * Copyright (c) 2022, 2024, BlackBerry Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as
 * contributors under the License or as licensors under other terms.
 * Please review this entire file for other proprietary rights or license
 * notices, as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

#include "f3s_snor.h"

#define SPINOR_ID_SIZE      4

#define S28Hx_MANID         0x34            // JEDEC ID, Cypress
#define S28Hx_TYPE_33v      0x5A            // 3.3v type (HL-T)
#define S28Hx_TYPE_18v      0x5B            // 1.8v type (HS-T)

#define S28Hx_SOFT_RESET_DELAY_US 100

enum cfr_reg_index {
    CFR1 = 0,
    CFR2,
    CFR3,
    CFR4,
    CFR5,
    CFR_MAX
};

/* Configuration register (Volatile) */
#define S28Hx_CFR1V          0x00800002
#define S28Hx_CFR2V          0x00800003
#define S28Hx_CFR3V          0x00800004
#define S28Hx_CFR4V          0x00800005
#define S28Hx_CFR5V          0x00800006

/* Configuration register (Nonvolatile) */
#define S28Hx_CFR1N          0x00000002
#define S28Hx_CFR2N          0x00000003
#define S28Hx_CFR3N          0x00000004
#define S28Hx_CFR4N          0x00000005
#define S28Hx_CFR5N          0x00000006

/* CFR1x bit definitions */
#define S28Hx_CFR1X_TB4KBS       (1 << 2)  // 0 = 4KB Sector Block is in the bottom of the memory address space
                                           // 1 = 4KB Sector Block is in the top of the memory address space
#define S28Hx_CFR1X_SP4KBS       (1 << 6)  // 0 = 4KB Sectors are grouped together
                                           // 1 = 4KB Sectors are split between High and Low Addresses

/* CFR2x bit definitions */
#define S28Hx_CFR2X_MEMLAT_200M  (0xB)     // Single mode: dummy cycle=11; Octal mode: dummy cycle=24

/* CFR3x bit definitions */
#define S28Hx_CFR3X_UNHYSA       (1 << 3)  // 1: Uniform Sector Architecture (all 256KB sectors); 0: Hybrid Sector Architecture
#define S28Hx_CFR3X_PGMBUF_512   (1 << 4)  // 1: 512 Byte Write Buffer Size; 0: 256 Byte Write Buffer Size
#define S28Hx_CFR3X_BLKCHK       (1 << 5)  // 1: Blank Check evaluation is enabled before executing an erase operation; 0: disabled

/* CFR4x bit definitions */
#define S28Hx_CFR4X_ECC12S_2     (1 << 3)  // 1-bit ECC Error Detection/Correction and 2-bit ECC error detection
#define S28Hx_CFR4X_DRVSTRG_MSK  (0x7)     // Output driver strength mask
#define S28Hx_CFR4X_DRVSTRG_SHFT (5u)      // Output driver strength shift

/* CFR5x bit definitions */
#define S28Hx_CFR5X_DDR          (1 << 1)  // 1: DDR enabled; 0 = SDR enabled
#define S28Hx_CFR5X_OCTAL        (1 << 0)  // 1: Data Width set to 8 wide (8x)-Octal Protocol; 0: Legacy Single SPI Protocol

/**
 *  @brief             Write data to s28hx register.
 *  @param chip        Flash chip handle.
 *  @param opcode      OP code.
 *  @param addr        Register address.
 *  @param adrlen      Register address length.
 *  @param buf         Data buffer.
 *  @param len         Data length
 *
 *  @return            EOK --success otherwise fail.
 */
static int s28hx_write_register(snor_chip_t* const chip, const uint8_t opcode,
                const uint32_t addr, const uint8_t adrlen, uint8_t* const buf, const int len)
{
    int status;

    status = snor_write_register(chip, opcode, addr, adrlen, buf, len);
    if (status != EOK) {
        return (status);
    }

    nanospin_ns(100);

    return (EOK);
}

/*  s28hx_set_device_config
 *
 *  Set device specific configurations and set I/O mode to Octal DDR for bus proto 888/888DTR.
 *
 *  Assumption: The device is currently in default configuration in standard SPI mode.
 */
static int32_t s28hx_set_device_config(struct _snor_chip_t* const chip, const uint32_t proto)
{
    const snor_ctrl_t* const ctrl = chip->ctrl;
    int32_t        err;
    const uint8_t  alen = (uint8_t)((chip->flags & SNOR_CFLG_4B_ADDR) ? 4 : 3);
    uint8_t        cfr[CFR_MAX];
    const uint32_t adr[CFR_MAX] = {S28Hx_CFR1V, S28Hx_CFR2V, S28Hx_CFR3V, S28Hx_CFR4V, S28Hx_CFR5V};

    for (int idx = 0; idx < CFR_MAX; idx++) {
        err = snor_read_register(chip, SNOR_CMD_RDAR, adr[idx], alen, &cfr[idx], sizeof(cfr[0]));
        if (err != EOK) {
            snor_slogf(_SLOG_ERROR, 0, 0, "%s: Read of S28Hx_CFR%dV failed", __func__, idx + 1);
            return (err);
        }
    }

    if (cfr[CFR3] & S28Hx_CFR3X_PGMBUF_512) {
        chip->pagesz = 512;
    } else {
        chip->pagesz = 256;
    }

    /* Check sector architecture
     * When CFR3V[3]: UNHYSA - Uniform or Hybrid Sector Architecture Selection
     * 0 = Hybrid Sector Architecture (combination of 4KB sectors and 256KB sectors)
     * 1 = Uniform Sector Architecture (all 256KB sectors)
     */
    if (!(cfr[CFR3] & S28Hx_CFR3X_UNHYSA)) {
        snor_slogf(_SLOG_INFO, ctrl->verbosity, _SLOG_INFO, "%s: Hybrid Sector Architecture detected: CFR3V: 0x%x", __func__, cfr[CFR3]);

        /* S28Hx_CFR1V */
        if (cfr[CFR1] & S28Hx_CFR1X_TB4KBS) {
            snor_slogf(_SLOG_INFO, ctrl->verbosity, _SLOG_INFO, "%s: 4KB Sector Block is in the top of the memory address space", __func__);
        } else {
            snor_slogf(_SLOG_INFO, ctrl->verbosity, _SLOG_INFO, "%s: 4KB Sector Block is in the bottom of the memory address space", __func__);
        }

        if (cfr[CFR1] & S28Hx_CFR1X_SP4KBS) {
            snor_slogf(_SLOG_INFO, ctrl->verbosity, _SLOG_INFO, "%s: 4KB Sectors are split between High and Low Addresses", __func__);
        } else {
            snor_slogf(_SLOG_INFO, ctrl->verbosity, _SLOG_INFO, "%s: 4KB Sectors are grouped together", __func__);
        }
    }

    /* Set Latency Code for 200 MHz Reference Clock (WRARG_C_1 CFR2)
     * 11 dummy cycles for 1-1-1 mode, 24 for 8-8-8 mode */
    cfr[CFR2] = (uint8_t)((cfr[CFR2] & 0xF0) | S28Hx_CFR2X_MEMLAT_200M);
    if (s28hx_write_register(chip, SNOR_CMD_WRAR, S28Hx_CFR2V, alen, &cfr[CFR2], 1) != EOK) {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, 0, "%s: Write S28Hx_CFR2V failed", __func__);
        return (ENOTSUP);
    }

    /* Enable Blank Check evaluation before executing an erase operation */
    if (!(cfr[CFR3] & S28Hx_CFR3X_BLKCHK)) {
        cfr[CFR3] |= S28Hx_CFR3X_BLKCHK;
    }

    /* Set Maximum Latency Code for volatile register read (CFR3)
     * 2 dummy cycles for 1-1-1 mode, 6 for 8-8-8 mode */
    cfr[CFR3] |= 0xC0;
    if (s28hx_write_register(chip, SNOR_CMD_WRAR, S28Hx_CFR3V, alen, &cfr[CFR3], 1) != EOK) {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, 0, "%s: Write S28Hx_CFR3V failed", __func__);
        return (ENOTSUP);
    }

    /* Command line opts set driver strength */
    if (chip->drv_type != SNOR_NO_DRVSTRG_OPT) {
        if (chip->drv_type > S28Hx_CFR4X_DRVSTRG_MSK) {
            snor_slogf(_SLOG_WARNING, ctrl->verbosity, 3, "%s: Invalid driver strength: 0x%x", __func__, chip->drv_type);
        } else {
            /* Only update driver strength if it is different than current value */
            if ((uint8_t)chip->drv_type != (uint8_t)((cfr[CFR4] >> S28Hx_CFR4X_DRVSTRG_SHFT) & S28Hx_CFR4X_DRVSTRG_MSK)) {
                cfr[CFR4] &= ~(S28Hx_CFR4X_DRVSTRG_MSK << S28Hx_CFR4X_DRVSTRG_SHFT);
                cfr[CFR4] |= (uint8_t)(chip->drv_type << S28Hx_CFR4X_DRVSTRG_SHFT);
            }
        }
    }

    /*
     * Disable 2-bit ECC Error Detection (WRARG_C_1 CFR4)
     * According to Cypress S28Hx Document Number: 002-18216, Section 4.1 Error Detection and Correction,
     * when 2-bit error detection is enabled, byte-programming/bit-walking/multiple page program operation
     * (without an erase) will result in Program Error (PRGERR)
     */
    cfr[CFR4] &= ~S28Hx_CFR4X_ECC12S_2;
    if (s28hx_write_register(chip, SNOR_CMD_WRAR, S28Hx_CFR4V, alen, &cfr[CFR4], 1) != EOK) {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, 0, "%s: Write S28Hx_CFR4V failed", __func__);
        return (ENOTSUP);
    }

    /* Enable/Disable Octal DTR (WRARG_C_1 CFR5)
     * This has to be the last step for chip configuration
     */
    uint8_t octddr = 0;
    if ((proto & SNOR_BUSPROTO_BUS_MASK) == SNOR_BUSPROTO_8_8_8) {
        octddr = S28Hx_CFR5X_OCTAL;
        if (proto & SNOR_BUSPROTO_DTR_MODE) {
            octddr |= S28Hx_CFR5X_DDR;
        }
    }

    if ((cfr[CFR5] & 0x03u) != octddr) {
        cfr[CFR5] &= 0xFC;
        cfr[CFR5] |= octddr;
        if (s28hx_write_register(chip, SNOR_CMD_WRAR, S28Hx_CFR5V, alen, &cfr[CFR5], 1) != EOK) {
            snor_slogf(_SLOG_ERROR, ctrl->verbosity, 0, "%s: Write S28Hx_CFR5V failed", __func__);
            return (ENOTSUP);
        }
    }

    /* Overwrite chip opcode */
    if ((proto & SNOR_BUSPROTO_BUS_MASK) == SNOR_BUSPROTO_8_8_8) {
        chip->op_rd.opcode = SNOR_CMD_READ_144_4B;
        chip->op_rd.dcycle = 24;
        chip->op_wr.opcode = SNOR_CMD_PP_4B;
        chip->flags       |= SNOR_CFLG_4B_ADDR;   // 4 bytes address for 8-8-8 mode

        /*
         * The dummy cycle should be 6, but since register read needs
         * 4 bytes address in octal mode, we add 2 dummy cycles for
         * OCT-DDR, 4 for OCT-SDR mode.
         * In case of volatile register read, since the device returns
         * the register content repeatly, extra dummy cycle shouldn't
         * affect the read back value
         */
        if (proto & SNOR_BUSPROTO_DTR_MODE) {
            chip->op_rd.opcode = SNOR_CMD_READ_144_DTR_4B;
            chip->rdr_dc = 8;
        } else {
            chip->rdr_dc = 10;
        }
    } else {
        chip->op_rd.opcode = SNOR_CMD_READ_FAST;
        chip->op_rd.dcycle = 11;
        chip->op_wr.opcode = SNOR_CMD_PP_4B;
        chip->rdr_dc       = 2;
    }

    return EOK;
}

static void s28hx_set_dopflgs(struct _snor_chip_t* const chip, const uint32_t proto)
{
    if ((proto & SNOR_BUSPROTO_BUS_MASK) == SNOR_BUSPROTO_8_8_8) {
        chip->rdcfg.cflgs |= SNOR_CFGFLGS_DBOP;
        chip->wrcfg.cflgs |= SNOR_CFGFLGS_DBOP;
        chip->cfg.cflgs   |= SNOR_CFGFLGS_DBOP;
    } else {
        chip->rdcfg.cflgs &= ~SNOR_CFGFLGS_DBOP;
        chip->wrcfg.cflgs &= ~SNOR_CFGFLGS_DBOP;
        chip->cfg.cflgs   &= ~SNOR_CFGFLGS_DBOP;
    }
}

/**
 *  @brief             Set protocol callout for Cypress S28Hx serial NOR flash.
 *  @param chip        Chip handle.
 *  @param proto       Protocol.
 *
 *  @return            EOK --success otherwise fail.
 */
static int _s28hx_set_protocol(struct _snor_chip_t* const chip, const uint32_t proto)
{
    const snor_ctrl_t* const ctrl = chip->ctrl;

    if (ctrl->verbosity > 3) {
        snor_slogf(_SLOG_INFO, ctrl->verbosity, 1, "%s: set protocol %x%s%s", __func__,
            proto & SNOR_BUSPROTO_BUS_MASK,
            (proto & SNOR_BUSPROTO_DTR_MODE) ? "-DTR" : "",
            (proto & SNOR_BUSPROTO_DQS) ? "-DQS" : "");
    }

    const int32_t err = s28hx_set_device_config(chip, proto);
    if (err != EOK) {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, 0, "%s: s28hx_set_device_config failed", __func__);
        return err;
    }

    s28hx_set_dopflgs(chip, proto);

    return err;
}

/**
 *  @brief             Configure device to 4 byte commands mode.
 *  @param chip        Pointer to chip structure.
 *
 *  @return            EOK --success otherwise fail.
 */
static int _s28hx_enter_4b_address(struct _snor_chip_t* const chip)
{
    const snor_ctrl_t* const ctrl = chip->ctrl;
    int32_t err;
    uint8_t cfr2;

    err = snor_read_register(chip, SNOR_CMD_RDAR, S28Hx_CFR2V, 3, &cfr2, 1);
    if (err == EOK) {
        cfr2 |= (1 << 7);

        err = s28hx_write_register(chip, SNOR_CMD_WRAR, S28Hx_CFR2V, 3, &cfr2, 1);
        if (err != EOK) {
            snor_slogf(_SLOG_ERROR, ctrl->verbosity, 0, "%s: Write S28Hx_CFR2V failed", __func__);
        }
    } else {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, 0, "%s: Read S28Hx_CFR2V failed", __func__);
    }

    return (err);
}

/**
 *  @brief             Ident callout for Cypress S28Hx serial NOR flash.
 *  @param dbase       F3S data base handle.
 *  @param access      F3S access handle.
 *  @param flags       Ident flags.
 *  @param cs          Chip select
 *
 *  @return            EOK --success otherwise fail.
 */
int32_t f3s_s28hx_ident(f3s_dbase_t *dbase, f3s_access_t *access, const uint32_t flags, const uint32_t cs)
{
    snor_ctrl_t   *ctrl;
    snor_chip_t   *chip;
    uint8_t       ids[SPINOR_ID_SIZE];

    if (access == NULL) return ENODEV;

    ctrl = (snor_ctrl_t *)access->socket.memory;
    chip = &ctrl->chip[cs];

    if ((chip->cfg.bus_proto != SNOR_BUSPROTO_1_1_1) &&
        (chip->cfg.bus_proto != SNOR_BUSPROTO_8_8_8) &&
        (chip->cfg.bus_proto != SNOR_BUSPROTO_8_8_8_DTR)) return (ENOTSUP);

    if ((chip->vid != 0) && (chip->did != 0)) {
        ids[0] = chip->vid;
        ids[1] = chip->did;
    } else {
        if (snor_read_id(chip, ids, SPINOR_ID_SIZE) != EOK) return EIO;
    }

    if ((ids[0] == S28Hx_MANID) && ((ids[1] == S28Hx_TYPE_33v) || (ids[1] == S28Hx_TYPE_18v))) {
        chip->hcaps = SNOR_HCAPS_RD_1_1_1 | SNOR_HCAPS_RD_1_1_1_FAST | SNOR_HCAPS_RD_8_8_8 | SNOR_HCAPS_RD_OCTAL |
                      SNOR_HCAPS_PP_1_1_1 | SNOR_HCAPS_PP_8_8_8 |
                      SNOR_HCAPS_DTR | SNOR_HCAPS_DQS;

        if (f3s_sfdp_ident(dbase, access, flags, cs) != EOK) return ENOTSUP;

        chip->op_pr = 0x30;
        chip->op_ps = 0xB0;
        chip->op_er = 0x30;
        chip->op_es = 0xB0;

        /* Add chip specific erase suspend calllout, generic resume callout should work. */
        chip->asr2  = 0x07;
        chip->sr2_esbit = 1;
    } else {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, 0,
            "%s: unsupported ID[%02x:%02x]", __func__, ids[0], ids[1]);
        return ENOTSUP;
    }

    if (dbase != NULL) {
        dbase->jedec_hi    = ids[0];
        dbase->jedec_lo    = (uint16_t)(ids[1] << 8);
        dbase->jedec_lo    = (uint16_t)ids[2];
        dbase->name        = "S28Hx";
    }

    chip->set_protocol = _s28hx_set_protocol;
    chip->enter_4b_address = _s28hx_enter_4b_address;

    return EOK;
}


/**
 *  @brief             Reset callout for Cypress S28Hx serial NOR flash.
 *  @param dbase       Pointer to F3S data base.
 *  @param access      Pointer to flash access structure.
 *  @param flags       Reset  flags.
 *  @param offset      Reset offset.
 *
 *  @return            None
 */
void f3s_s28hx_reset(f3s_dbase_t *dbase, f3s_access_t *access, uint32_t flags, uint32_t offset)
{
    snor_chip_t *chip;
    snor_ctrl_t *ctrl;

    chip = (snor_chip_t *)access->service->page(&access->socket, flags, offset, NULL);
    if (chip == NULL) return;

    ctrl = chip->ctrl;

    s28hx_set_dopflgs(chip, chip->cfg.bus_proto);

    /* Reconfig the bus */
    if (ctrl->funcs.cfg_bus != NULL) {
        ctrl->funcs.cfg_bus(ctrl, &chip->cfg);
    }

    if (snor_reset(chip) == EOK) {
        /* S28hx does not provide a status bit to verify reset completion.
         * Need a delay to ensure the volatile registers have been loaded from
         * non-volatile regs.
         */
        usleep(S28Hx_SOFT_RESET_DELAY_US);

        /* Reconfig the bus */
        if (ctrl->funcs.cfg_bus != NULL) {
            chip->cfg.bus_proto = SNOR_BUSPROTO_1_1_1;
            ctrl->funcs.cfg_bus(ctrl, &chip->cfg);
        }

        /*
         * Set to manufacture default mode,
         * but this is not necessarily true, it all depends on the value
         * of non-volatile registers, so if the bus mode and dummy cycles
         * are not what we expected, customized reset function is needed.
         */
        /* Update read register dummy cycles */
        chip->rdr_dc = RDR_DC_DFLT;

        /* Single, 3 byte address mode */
        chip->flags &= ~(SNOR_CFLG_4B_ADDR | SNOR_CFLG_DUAL | SNOR_CFLG_QUAD | SNOR_CFLG_OCTAL);

        s28hx_set_dopflgs(chip, chip->cfg.bus_proto);
    } else {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, 0, "%s: snor_reset failed", __func__);
    }
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif

