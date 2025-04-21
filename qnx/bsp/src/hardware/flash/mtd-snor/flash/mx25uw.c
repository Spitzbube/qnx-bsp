/*
 * Copyright (c) 2023-2024, BlackBerry Limited.
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

#define MX25UW_ID_LEN             6
#define MX25UW_MANID              0xC2            /* Manufacturer ID, Macronix */
#define MX25UW_MEM_TYPE           0x81            /* Memory type */
#define MX25UW_MEM_DENSITY_32MB   0x39            /* Memory density */
#define MX25UW_MEM_DENSITY_64MB   0x3A            /* Memory density */
#define MX25UW_BLOCKER_SIZE       0x10000         /* Default block erase size: 64KB */
#define MX25UW_BLOCKER_PW2SZ      16              /* Block erase size, power of 2: 2^16 = 64KB */
#define MX25UW_PAGE_SIZE          256             /* Program page size: 256B*/

#define MX25UW_OP_RDCR            0x15            /* Read configuration Register */
#define MX25UW_OP_RDSR            0x05            /* Read status Register */
#define MX25UW_OP_WRSCR           0x01            /* Write status/configuration Register */
#define MX25UW_OP_RDCR2           0x71            /* Read configuration Register2 */
#define MX25UW_OP_WRCR2           0x72            /* Write configuration Register2 */

/* registers address */
#define MX25UW_CR2_REG0           0x00000000UL   /* CRCEN */
    #define CR2_REG0_DOPI           0x2
    #define CR2_REG0_SOPI           0x1
    #define CR2_REG0_SPI            0x0
    #define CR2_REG0_OPI_MASK       0x3
#define MX25UW_CR2_REG1           0x00000200UL   /* DOS, DQSPRC */
    #define CR2_REG1_DOS            0x2
    #define CR2_REG1_DQSPRC_CYCLE(x)      ((x) & 0x1)
    #define CR2_REG1_DQSPRC_CYCLE_MASK    0x1
#define MX25UW_CR2_REG2           0x00000300UL   /* DC */
#define MX25UW_CR2_REG4           0x00000500UL   /* PPTSEL, CRCBEN, CRC CYC, CRCBIN */

#define MX25UW_ODS_MASK           0x7U           /* Output driver strength mask */

static int _mx25uw_set_protocol(struct _snor_chip_t *chip, uint32_t proto);

/**
 *  @brief             Ident callout for Macronix MX25UW serial NOR flash.
 *  @param dbase       F3S data base handle.
 *  @param access      F3S access handle.
 *  @param flags       Ident flags.
 *  @param cs          Chip select
 *
 *  @return            EOK --success otherwise fail.
 */
int32_t f3s_mx25uw_ident(f3s_dbase_t *dbase, f3s_access_t *access, uint32_t flags, uint32_t cs)
{
    snor_ctrl_t *ctrl;
    snor_chip_t *chip;
    int         ret;
    uint8_t     ids[MX25UW_ID_LEN] = {0};

    if (access == NULL) return (ENODEV);

    ctrl = (snor_ctrl_t *)access->socket.memory;
    chip =  &ctrl->chip[cs];
    const uint8_t   cflgs = chip->cfg.cflgs; // cache the original configs
    const uint32_t  proto = (chip->cfg.bus_proto & SNOR_BUSPROTO_MASK);
    const uint32_t  bus_proto = (chip->cfg.bus_proto & SNOR_BUSPROTO_BUS_MASK);

    if ((proto != SNOR_BUSPROTO_1_1_1) &&
        (proto != SNOR_BUSPROTO_8_8_8) &&
        (proto != SNOR_BUSPROTO_8_8_8_DTR)) return (ENOTSUP);


    if (bus_proto == SNOR_BUSPROTO_8_8_8) {
        chip->cfg.cflgs |= SNOR_CFGFLGS_DBIOP;
        chip->rdr_dc = 8;
    } else {
        chip->cfg.cflgs &= ~SNOR_CFGFLGS_DBIOP;
    }

    if ((chip->vid != 0) && (chip->did != 0)) {
        ids[0] = chip->vid;
        ids[1] = chip->did;
    } else {
        if (snor_read_id(chip, ids, MX25UW_ID_LEN) != EOK) return (EIO);
        /* data bytes are always output in STR */
        if (proto & SNOR_BUSPROTO_DTR_MODE) {
            ids[1] = ids[2];
            ids[2] = ids[4];
        }
    }

    if ((ids[0] == MX25UW_MANID) &&
        (ids[1] == MX25UW_MEM_TYPE) &&
        ((ids[2] == MX25UW_MEM_DENSITY_32MB) || (ids[2] == MX25UW_MEM_DENSITY_64MB))) {

        chip->hcaps = SNOR_HCAPS_RD_1_1_1 | SNOR_HCAPS_PP_1_1_1 | SNOR_HCAPS_RD_8_8_8 |
                      SNOR_HCAPS_PP_8_8_8 | SNOR_HCAPS_DQS | SNOR_HCAPS_DTR;

        ret = f3s_sfdp_ident(dbase, access, flags, cs);

        chip->flags |= SNOR_CFLG_DLTBS | SNOR_CFLG_DLBBS | SNOR_CFLG_PSLOCK | SNOR_CFLG_DNLOCK;

    } else {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, 0,
            "%s: unsupported ID[%02x:%02x:%02x] proto[0x%#x]", __func__, ids[0], ids[1], ids[2], proto);
        return (ENOTSUP);
    }

    if (dbase != NULL) {
        dbase->jedec_hi    = ids[0];
        dbase->jedec_lo    = (uint16_t)(ids[1] << 8);
        dbase->jedec_lo    |= (uint16_t)ids[2];
        dbase->name        = "Macronix MX25UW";

        /* mx25uw51245g is a SPI NOR that supportes the SFDPRD command but some samples return an empty SFDP page.
        *  This forces us to hardcode the following parameters.
        */
        if (ret != EOK) {
            const uint32_t chip_size = (ids[2] == MX25UW_MEM_DENSITY_32MB) ? 0x2000000U : 0x4000000U;
            const uint32_t unit_size = (access->socket.unit_size) ? access->socket.unit_size : MX25UW_BLOCKER_SIZE;
            access->socket.array_size = chip_size;
            access->socket.window_size = chip_size;
            access->socket.unit_size = unit_size;
            dbase->buffer_size = MX25UW_PAGE_SIZE;
            dbase->geo_num = 1;
            dbase->geo_vect[0].unit_pow2 = (uint16_t)MX25UW_BLOCKER_PW2SZ;
            dbase->geo_vect[0].unit_num  = (uint16_t)(chip_size / unit_size);
            dbase->flags = 0;
            chip->chipsz = chip_size;
            chip->pagesz = MX25UW_PAGE_SIZE;
            chip->dcaps |= SNOR_DCAPS_PSR | SNOR_DCAPS_ESR;
            for (int i = 0; i < SNOR_MAX_ERSCFG; i++) {
                chip->blkers[i].blksz_pow2 = MX25UW_BLOCKER_PW2SZ;
            }
        }
    }

    /* restore orginal configs */
    chip->cfg.cflgs = cflgs;
    chip->rdr_dc = 0;
    chip->set_protocol = _mx25uw_set_protocol;
    return (EOK);
}

static int _mx25uw_cfg_chip(struct _snor_chip_t* const chip, const uint32_t proto, uint8_t *const preset_dc)
{
    const snor_ctrl_t* const ctrl = chip->ctrl;
    const uint8_t dc_table[] = {20, 18, 16, 14, 12, 10, 8, 6};
    uint8_t cr2_r0, cr2_r1, cr2_r2, sr, cr;
    int ret;

    ret = snor_read_register(chip, MX25UW_OP_RDCR2, MX25UW_CR2_REG0, 4, &cr2_r0, 1);
    if (ret != EOK) {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, 0, "%s: Error: read cr2_r0 register", __func__);
        return (EIO);
    }

    ret = snor_read_register(chip, MX25UW_OP_RDCR2, MX25UW_CR2_REG1, 4, &cr2_r1, 1);
    if (ret != EOK) {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, 0, "%s: Error: read cr2_r1 register", __func__);
        return (EIO);
    }

    ret = snor_read_register(chip, MX25UW_OP_RDCR2, MX25UW_CR2_REG2, 4, &cr2_r2, 1);
    if (ret != EOK) {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, 0, "%s: Error: read cr2_r2 register", __func__);
        return (EIO);
    }

    ret = snor_read_register(chip, MX25UW_OP_RDSR, 0, 0, &sr, 1);
    if (ret != EOK) {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, 0, "%s: Error: read sr register", __func__);
        return (EIO);
    }

    ret = snor_read_register(chip, MX25UW_OP_RDCR, 0, 0, &cr, 1);
    if (ret != EOK) {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, 0, "%s: Error: read cr register", __func__);
        return (EIO);
    }

    /* Command line opts set driver strength */
    if (chip->drv_type != SNOR_NO_DRVSTRG_OPT) {
        if (chip->drv_type > MX25UW_ODS_MASK) {
            snor_slogf(_SLOG_WARNING, ctrl->verbosity, 3, "%s: Invalid driver strength: 0x%x", __func__, chip->drv_type);
        } else {
            if ((cr & MX25UW_ODS_MASK) != (uint8_t)chip->drv_type) {
                cr &= ~MX25UW_ODS_MASK;
                cr |= (uint8_t)(chip->drv_type & MX25UW_ODS_MASK);
                const uint8_t sr_cr[2] = {sr, cr};
                ret = snor_write_register(chip, MX25UW_OP_WRSCR, 0, 0, (uint8_t *)sr_cr, 2);
                if (ret != EOK) {
                    snor_slogf(_SLOG_ERROR, ctrl->verbosity, 0, "%s: error to write cr2_r1 register", __func__);
                    return (EIO);
                }

            }
        }
    }

    if (preset_dc != NULL) {
        *preset_dc = dc_table[(cr2_r2 & 0x7U)];
    }

    cr2_r0 &= ~CR2_REG0_OPI_MASK;
    cr2_r1 &= ~CR2_REG1_DOS;

    /* OPI mode */
    if ((proto & SNOR_BUSPROTO_BUS_MASK) == SNOR_BUSPROTO_8_8_8) {
        cr2_r0 |= (proto & SNOR_BUSPROTO_DTR_MODE) ? CR2_REG0_DOPI : CR2_REG0_SOPI;
    } else { /* SPI mode */
        cr2_r0 |= CR2_REG0_SPI;
    }

    /* DQS on STR mode */
    if (proto & SNOR_BUSPROTO_DQS) {
        cr2_r1 |= (proto & SNOR_BUSPROTO_DTR_MODE) ? CR2_REG1_DQSPRC_CYCLE(0) : CR2_REG1_DOS;
    }

    ret = snor_write_register(chip, MX25UW_OP_WRCR2, MX25UW_CR2_REG1, 4, &cr2_r1, 1);
    if (ret != EOK) {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, 0, "%s: error to write cr2_r1 register", __func__);
        return (EIO);
    }

    ret = snor_write_register(chip, MX25UW_OP_WRCR2, MX25UW_CR2_REG0, 4, &cr2_r0, 1);
    if (ret != EOK) {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, 0, "%s: error to write cr2_r0 register", __func__);
        return (EIO);
    }
    return (EOK);
}

/**
 *  @brief             Set protocol callout for Macronix MX25UW serial NOR flash.
 *                     Use Volatile Configuration Register to configure SPI, OPI
 *                     DTR and DQS.
 *  @param chip        Chip handle.
 *  @param proto       Protocol.
 *
 *  @return            EOK --success otherwise fail.
 */
static int _mx25uw_set_protocol(struct _snor_chip_t* const chip, const uint32_t proto)
{
    const snor_ctrl_t* const ctrl = chip->ctrl;
    uint8_t preset_dc = 0;
    int ret;

    if (ctrl->verbosity > 3) {
        snor_slogf(_SLOG_INFO, ctrl->verbosity, 1, "%s: set protocol %x%s%s", __func__,
            (proto & SNOR_BUSPROTO_BUS_MASK),
            (proto & SNOR_BUSPROTO_DTR_MODE) ? "-DTR" : "",
            (proto & SNOR_BUSPROTO_DQS) ? "-DQS" : "");
    }

    ret = _mx25uw_cfg_chip(chip, proto, &preset_dc);
    if (ret != EOK) return (ret);

    /* update parameters */
    if ((proto & SNOR_BUSPROTO_BUS_MASK) == SNOR_BUSPROTO_8_8_8) {
        /* overwrite opcode*/
        chip->op_rd.opcode = SNOR_CMD_READ_144_4B;
        chip->op_rd.dcycle = (preset_dc) ? preset_dc : 20U;
        chip->op_rd.adrlen = 4;
        chip->op_wr.opcode = SNOR_CMD_PP_4B;
        chip->op_wr.adrlen = 4;
        /* 4 address bytes are mandatory in OPI mode,
         * the extra 4 dummy cycle is account for the extra delay when
         * flash layer passes zero address length to driver code.
         */
        chip->rdr_dc       = 4 + 4;
        chip->flags       |= SNOR_CFLG_4B_ADDR;
        chip->rdcfg.cflgs |= SNOR_CFGFLGS_DBIOP;
        chip->wrcfg.cflgs |= SNOR_CFGFLGS_DBIOP;
        chip->cfg.cflgs   |= SNOR_CFGFLGS_DBIOP;

        if (proto & SNOR_BUSPROTO_DTR_MODE) {
            chip->op_rd.opcode = SNOR_CMD_READ_144_DTR_4B;
        }

        for (int i = 0; i < SNOR_MAX_ERSCFG; i++) {
            chip->blkers[i].opcode = SNOR_CMD_SE_4B;
            chip->blkers[i].opcode_4b = SNOR_CMD_SE_4B;
        }

    } else { // 1-1-1
        chip->op_rd.opcode = SNOR_CMD_READ_FAST_4B;
        chip->op_rd.dcycle = 8;
        chip->op_rd.adrlen = 4;
        chip->op_wr.opcode = SNOR_CMD_PP_4B;
        chip->op_wr.adrlen = 4;
        chip->rdr_dc       = 0;
        chip->flags       |= SNOR_CFLG_4B_ADDR;
        chip->rdcfg.cflgs &= ~SNOR_CFGFLGS_DBIOP;
        chip->wrcfg.cflgs &= ~SNOR_CFGFLGS_DBIOP;
        chip->cfg.cflgs   &= ~SNOR_CFGFLGS_DBIOP;
        for (int i = 0; i < SNOR_MAX_ERSCFG; i++) {
            chip->blkers[i].opcode = SNOR_CMD_SE_4B;
            chip->blkers[i].opcode_4b = SNOR_CMD_SE_4B;
        }
    }

    return (EOK);
}

/**
 *  @brief             Reset callout for Macronix MX25UW serial NOR flash.
 *  @param dbase       Pointer to F3S data base.
 *  @param access      Pointer to flash access structure.
 *  @param flags       Reset  flags.
 *  @param offset      Reset offset.
 *
 *  @return            None
 */
void f3s_mx25uw_reset(f3s_dbase_t *dbase, f3s_access_t *access, uint32_t flags, uint32_t offset)
{
    snor_chip_t *chip;
    snor_ctrl_t *ctrl;

    chip = (snor_chip_t *)access->service->page(&access->socket, flags, offset, NULL);
    if (chip == NULL) return;

    ctrl = chip->ctrl;

     if ((chip->cfg.bus_proto & SNOR_BUSPROTO_BUS_MASK) == SNOR_BUSPROTO_8_8_8) {
        chip->cfg.cflgs   |= SNOR_CFGFLGS_DBIOP;
     } else {
        chip->cfg.cflgs   &= ~SNOR_CFGFLGS_DBIOP;
     }

    /* Reconfig the bus */
    if (ctrl->funcs.cfg_bus != NULL) {
        ctrl->funcs.cfg_bus(ctrl, &chip->cfg);
    }

    if (snor_reset(chip) == EOK) {
        /* Reconfig the bus */
        if (ctrl->funcs.cfg_bus != NULL) {
            chip->cfg.bus_proto = SNOR_BUSPROTO_1_1_1;
            ctrl->funcs.cfg_bus(ctrl, &chip->cfg);
        }

        chip->rdr_dc = 0;

        /* Single, 3 byte address mode */
        chip->flags       &= ~(SNOR_CFLG_4B_ADDR | SNOR_CFLG_DUAL | SNOR_CFLG_QUAD | SNOR_CFLG_OCTAL |SNOR_CFLG_HYPER);
        chip->cfg.cflgs   &= ~SNOR_CFGFLGS_DBIOP;
        chip->rdcfg.cflgs &= ~SNOR_CFGFLGS_DBIOP;
        chip->wrcfg.cflgs &= ~SNOR_CFGFLGS_DBIOP;

        if (chip->flags & SNOR_CFLG_PRESENT) {
            chip->post_ident(chip);
        }

    } else {
        /* In case of failure, we reset all DBIOP configs */
        chip->cfg.cflgs   &= ~SNOR_CFGFLGS_DBIOP;
        chip->rdcfg.cflgs &= ~SNOR_CFGFLGS_DBIOP;
        chip->wrcfg.cflgs &= ~SNOR_CFGFLGS_DBIOP;
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, 0, "%s: snor_reset failed", __func__);
    }
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
