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

/* Command addresses listed below is matched with the description
 * of hyper flash(S26K) and they are all byte aligned address, low level
 * drivers should convert it to word aligned address according to HW configurations.
 */
#define HYPER_UNLOCK1_ADDR          (0x555u)
#define HYPER_UNLOCK1_DATA          (0xAAu)
#define HYPER_UNLOCK2_ADDR          (0x2AAu)
#define HYPER_UNLOCK2_DATA          (0x55u)
#define HYPER_ENTRY_ADDR            (0x555u)
#define HYPER_ID_ENTRY_DATA         (0x90u)
#define HYPER_RESET_ADDR            (0x00u)
#define HYPER_RESET_DATA            (0xF0u)
#define HYPER_READ_STATUS           (0x70u)
#define HYPER_ERASE_ADDR1           (0x555u)
#define HYPER_ERASE_DATA1           (0x80u)
#define HYPER_ERASE_ADDR2           (0x555u)
#define HYPER_ERASE_DATA2           (0xAAu)
#define HYPER_ERASE_ADDR3           (0x2AAu)
#define HYPER_ERASE_DATA3           (0x55u)
#define HYPER_ERASE_DATA4           (0x30u)
#define HYPER_ERASE_SUSPEND_DATA    (0xB0u)
#define HYPER_ERASE_RESUME_DATA     (0x30u)
#define HYPER_WORD_PROGRAM_DATA     (0xA0u)
#define HYPER_VCR_ADDR              (0x555u)
#define HYPER_RDNVCR_DATA           (0xC6u)
#define HYPER_RDVCR_DATA            (0xC7u)
#define HYPER_WRVCR_DATA            (0x38u)
#define HYPER_OPCODE_READ           (0xA0u)
#define HYPER_OPCODE_WRITE          (0x00u)

#define HYPER_ID_LEN                80
#define HYPER_VCR_ODS_MASK          (0x7u)
#define HYPER_VCR_ODS_SHIFT         (12u)

static int _hpf_set_protocol(struct _snor_chip_t *chip, uint32_t proto);
static int _hpf_unlock(snor_chip_t* const chip);

static int _hpf_write_cmd(snor_chip_t* const chip, const uint32_t addr, const uint16_t data)
{
    snor_ctrl_t* const ctrl = chip->ctrl;
    snor_cmd_t  cmd;
    /* Hyper always send 6 CA(command-address) bytes followed by two data bytes.
     * to keep the command data consistent, we always use 2-byte data, it is up to
     * low layer driver how to construct these two byte according to the hardware
     * configuration.
     * Hyper flash is using different addressing scheme(24bit row and 16bit column),
     * so, address length is not important to lower level driver, however, we still want to keep
     * them consistent with 4byte address.
     */
    snor_op_t   wrop = { .opcode = HYPER_OPCODE_WRITE, .dcycle = 0, .adrlen = 4 };
    const uint8_t buf[2] = {(uint8_t)((data >> 8) & 0xFFu), (uint8_t)(data & 0xFFu) };

    SNOR_SET_CMD(cmd, &wrop, &chip->cfg, addr);

    if (ctrl->funcs.write_reg != NULL) {
        return ctrl->funcs.write_reg(ctrl, &cmd, (uint8_t *)buf, sizeof(buf));
    }

    return (ENOTSUP);
}

static int _hpf_read_vcr(snor_chip_t* const chip, uint16_t* const vcr)
{

    if (_hpf_unlock(chip) != EOK) return (EIO);

    if (_hpf_write_cmd(chip, HYPER_VCR_ADDR, HYPER_RDVCR_DATA) != EOK) return (EIO);

    if (snor_read_register(chip, HYPER_OPCODE_READ, 0, 4, (uint8_t *)vcr, (int)sizeof(uint16_t)) != EOK) return (EIO);

    return (EOK);
}

static int _hpf_load_vcr(snor_chip_t* const chip, const uint16_t vcr)
{
    if (_hpf_unlock(chip) != EOK) return (EIO);

    if (_hpf_write_cmd(chip, HYPER_VCR_ADDR, HYPER_WRVCR_DATA) != EOK) return (EIO);

    return _hpf_write_cmd(chip, 0, vcr);
}

/**
 *  @brief             Ident callout for SPI Hyper flash.
 *  @param dbase       F3S data base handle.
 *  @param access      F3S access handle.
 *  @param flags       Ident flags.
 *  @param cs          Chip select
 *
 *  @return            EOK --success otherwise fail.
 */
int32_t f3s_hpf_ident(f3s_dbase_t *dbase, f3s_access_t *access, uint32_t flags, uint32_t cs)
{
    snor_ctrl_t *ctrl;
    snor_chip_t *chip;
    uint16_t     ids[HYPER_ID_LEN] = {0};
    uint32_t    unit_size;
    uint32_t    geo_index;
    uint32_t    geo_pos;
    uint32_t    chip_size = 0;

    if (access == NULL) return (ENODEV);

    ctrl = (snor_ctrl_t *)access->socket.memory;

    chip = &ctrl->chip[cs];

    if (chip->cfg.bus_proto != SNOR_BUSPROTO_HYPER) return (ENOTSUP);

    if (_hpf_unlock(chip) != EOK) return (EIO);

    if (_hpf_write_cmd(chip, HYPER_ENTRY_ADDR, HYPER_ID_ENTRY_DATA) != EOK) return (EIO);

    /* Read register needs 16 dummy cycles */
    chip->rdr_dc = 16u;

    if (snor_read_register(chip, HYPER_OPCODE_READ, 0, 4, (uint8_t *)ids, (int)sizeof(ids)) != EOK) {
        snor_slogf(_SLOG_ERROR, 0, 0, "%s: Read JEDEC-ID failed\n", __func__);
        return (EIO);
    }

    if ((ids[0x10] != (uint16_t)'Q') || (ids[0x11] != (uint16_t)'R') || (ids[0x12] != (uint16_t)'Y') ||
        (ids[0x40] != (uint16_t)'P') || (ids[0x41] != (uint16_t)'R') || (ids[0x42] != (uint16_t)'I')) {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, 0,
            "%s: unsupported ID[%04x:%04x]", __func__, ids[0], ids[1]);
        return (ENOTSUP);
    }

    /* Reset/ASO Exit Command */
    if (_hpf_write_cmd(chip, HYPER_RESET_ADDR, HYPER_RESET_DATA) != EOK) return (EIO);

    /* Fill dbase entry */
    dbase->struct_size = sizeof(*dbase);
    dbase->jedec_hi    = ids[0];
    dbase->jedec_lo    = (uint16_t)((ids[1] << 8) | ids[2]);
    dbase->name        = "HyperBus MirrotBit SIO";

    /* write buffer size */
    chip->pagesz = (uint16_t)(1 << (ids[0x2b] | ids[0x2a]));

    /* Read number of geometries */
    dbase->geo_num = ids[0x2c];

    /* Read geometry information
     * Need to read VCR to determine whether parameter page is enabled and
     * this will be required once we support non-power of 2 sector size.
     */
    geo_pos = 0x2d;
    for (geo_index = 0; geo_index < dbase->geo_num; geo_index++) {
        /* Read number of units */
        dbase->geo_vect[geo_index].unit_num   = ids[geo_pos + 1];
        dbase->geo_vect[geo_index].unit_num <<= 8;
        dbase->geo_vect[geo_index].unit_num  += ids[geo_pos + 0];
        dbase->geo_vect[geo_index].unit_num  += 1;

        /* Read size of unit */
        unit_size = ids[geo_pos + 3];
        unit_size <<= 8;
        unit_size += ids[geo_pos + 2];

        /* Interpret according to the CFI specs */
        unit_size = (unit_size == 0) ? 128U : (unit_size * 256U);

        chip_size += unit_size * dbase->geo_vect[geo_index].unit_num;

        /* Convert size to power of 2 */
        dbase->geo_vect[geo_index].unit_pow2 = 0;
        while (unit_size > 1) {
            unit_size >>= 1;
            dbase->geo_vect[geo_index].unit_pow2++;
        }

        if (ctrl->verbosity > _SLOG_DEBUG1) {
            snor_slogf(_SLOG_INFO, ctrl->verbosity, _SLOG_INFO, "(devf  t%d::%s:%d) dbase->geo_vect[%d].unit_pow2 = %u\n",
                pthread_self(), __func__, __LINE__, geo_index, dbase->geo_vect[geo_index].unit_pow2);
            snor_slogf(_SLOG_INFO, ctrl->verbosity, _SLOG_INFO, "(devf  t%d::%s:%d) dbase->geo_vect[%d].unit_num  = %u\n",
                pthread_self(), __func__, __LINE__, geo_index, dbase->geo_vect[geo_index].unit_num);
        }
        geo_pos += 4;
    }

    chip->chipsz = chip_size;

    chip->hcaps = SNOR_HCAPS_RD_OCTAL | SNOR_HCAPS_HYPER;

    /* erase suspend/resume
     * Detect read / write suspend
     */
    if (ids[0x46] == 1U) {
        dbase->flags = F3S_ERASE_FOR_READ;
    } else if (ids[0x46] == 2U) {
        dbase->flags = F3S_ERASE_FOR_READ | F3S_ERASE_FOR_WRITE;
    } else {
        dbase->flags = 0;
    }
    chip->dcaps |= SNOR_DCAPS_PSR | SNOR_DCAPS_ESR;

    chip->set_protocol = _hpf_set_protocol;
    chip->rdcfg.bus_proto = SNOR_BUSPROTO_HYPER;
    chip->wrcfg.bus_proto = SNOR_BUSPROTO_HYPER;

    return (EOK);
}

/**
 *  @brief             Reset callout for SPI Hyper flash.
 *  @param dbase       Pointer to F3S data base.
 *  @param access      Pointer to flash access structure.
 *  @param flags       Reset  flags.
 *  @param offset      Reset offset.
 *
 *  @return            None
 */
void f3s_hpf_reset(f3s_dbase_t *dbase,
                    f3s_access_t *access,
                    uint32_t flags,
                    uint32_t offset)
{
    snor_chip_t *chip;

    chip = (snor_chip_t *)access->service->page(&access->socket, flags, offset, NULL);
    if (chip == NULL) return;

    if (_hpf_write_cmd(chip, HYPER_RESET_ADDR, HYPER_RESET_DATA) != EOK) {
        snor_slogf(_SLOG_ERROR, 0, 0, "%s: reset failed", __func__);
    }
}

/**
 *  @brief             Read SPI Hyper flash status
 *  @param chip        Pointer to chip structure.
 *  @param sts         Pointer to chip status.
 *
 *  @return            EOK --success otherwise fail.
 *
 */
static int f3s_hpf_status(snor_chip_t* const chip, uint16_t *sts)
{
    if (_hpf_write_cmd(chip, HYPER_ENTRY_ADDR, HYPER_READ_STATUS) != EOK) return (EIO);

    if (snor_read_register(chip, HYPER_OPCODE_READ, 0, 4, (uint8_t *)sts, (int)sizeof(uint16_t)) != EOK) {
        snor_slogf(_SLOG_ERROR, 0, 0, "%s: read status register failed", __func__);
        return (EIO);
    }

    return (EOK);
}


/**
 *  @brief             Sync callout for SPI Hyper flash.
 *  @param dbase       Pointer to F3S data base.
 *  @param access      Pointer to flash access structure.
 *  @param flags       Sync flags.
 *  @param offset      Sync offset.
 *
 *  @return            EOK --erase complete
 *                     EAGAIN --erase inprogress
 *                     Otherwise --failure
 */
int32_t f3s_hpf_sync(f3s_dbase_t *dbase, f3s_access_t *access, uint32_t flags, uint32_t offset)
{
    snor_chip_t *chip;
    uint16_t    sts;
    int         ret;

    chip = (snor_chip_t *)access->service->page(&access->socket, 0, offset, NULL);
    if (chip == NULL) return (ERANGE);

    ret = f3s_hpf_status(chip, &sts);
    if (ret != EOK) return (ret);

    if (sts & (SNOR_FLGSTS_EFAIL | SNOR_FLGSTS_PFAIL | SNOR_FLGSTS_EPROCT)) return (EIO);

    if (!(sts & SNOR_FLGSTS_RDY)) return (EAGAIN);

    return (EOK);
}

/**
 *  @brief             Erase callout for SPI Hyper flash.
 *  @param dbase       Pointer to F3S data base.
 *  @param access      Pointer to flash access structure.
 *  @param flags       Erase flags.
 *  @param offset      Erase offset.
 *
 *  @return            EOK --success otherwise fail.
 */
int f3s_hpf_erase(f3s_dbase_t *dbase, f3s_access_t *access, uint32_t flags, uint32_t offset)
{
    snor_chip_t *chip;

    chip = (snor_chip_t *)access->service->page(&access->socket, 0, offset, NULL);
    if (chip == NULL) return (ERANGE);

    if (_hpf_unlock(chip) != EOK) return (EIO);

    if (_hpf_write_cmd(chip, HYPER_ERASE_ADDR1, HYPER_ERASE_DATA1) != EOK) return (EIO);

    if (_hpf_write_cmd(chip, HYPER_ERASE_ADDR2, HYPER_ERASE_DATA2) != EOK) return (EIO);

    if (_hpf_write_cmd(chip, HYPER_ERASE_ADDR3, HYPER_ERASE_DATA3) != EOK) return (EIO);

    /* Only pass the aligned address to lowlevel write_reg function */
    return _hpf_write_cmd(chip, chip->offset >> 1, HYPER_ERASE_DATA4);
}

/**
 *  @brief             Erase suspend callout for SPI Hyper flash.
 *  @param dbase       Pointer to F3S data base.
 *  @param access      Pointer to flash access structure.
 *  @param flags       Erase suspend flags.
 *  @param offset      Erase suspend offset.
 *
 *  @return            EOK --suspended successfully
 *                     ECANCELED --Erase complete, suspended unnecessary
 *                     Otherwise --failure
 */
int32_t f3s_hpf_suspend(f3s_dbase_t *dbase, f3s_access_t *access, uint32_t flags, uint32_t offset)
{
    snor_chip_t *chip;
    uint16_t    sts;

    chip = (snor_chip_t *)access->service->page(&access->socket, 0, offset, NULL);
    if (chip == NULL) return (ERANGE);

    if (_hpf_write_cmd(chip, chip->offset >> 1, HYPER_ERASE_SUSPEND_DATA) != EOK) return (EIO);

    for (int loop = 64; loop > 0; loop--) {
        const int ret = f3s_hpf_status(chip, &sts);
        if (ret != EOK) return (ret);

        if ((sts & SNOR_FLGSTS_RDY)) {
            if ((sts & SNOR_FLGSTS_ESUS)) return (EOK);
            if ((sts & (SNOR_FLGSTS_EFAIL | SNOR_FLGSTS_EPROCT))) return (EIO);
            return (ECANCELED);
        }

        nanospin_ns(1000);
    }

    return (EIO);
}

/**
 *  @brief             Erase resume callout for SPI Hyper flash.
 *  @param dbase       Pointer to F3S data base.
 *  @param access      Pointer to flash access structure.
 *  @param flags       Resume flags.
 *  @param offset      Resume offset.
 *
 *  @return            EOK --success otherwise fail.
 */
int32_t f3s_hpf_resume(f3s_dbase_t *dbase, f3s_access_t *access, uint32_t flags, uint32_t offset)
{
    snor_chip_t *chip;
    int32_t     ret;

    chip = (snor_chip_t *)access->service->page(&access->socket, 0, offset, NULL);
    if (chip == NULL) return (ERANGE);

    // log time for future suspend usage
    ret = _hpf_write_cmd(chip, chip->offset >> 1, HYPER_ERASE_RESUME_DATA);

    if (ret == EOK) {
        /* The latency between erase resume and next suspend (tERS) is 0.3us minimum, 400us typical */
        usleep(400);
    }

    return (ret);
}

/**
 *  @brief             SPI Hyper flash unlock seqnence
 *  @param chip        Pointer to chip structure.
 *
 *  @return            EOK ---success otherwise fail
 */
static int _hpf_unlock(snor_chip_t* const chip)
{
    if (_hpf_write_cmd(chip, HYPER_UNLOCK1_ADDR, HYPER_UNLOCK1_DATA) != EOK) return (EIO);

    return _hpf_write_cmd(chip, HYPER_UNLOCK2_ADDR, HYPER_UNLOCK2_DATA);
}

/**
 *  @brief             Page program callout for SPI Hyper NOR flash.
 *  @param dbase       Pointer to F3S data base.
 *  @param access      Pointer to flash access structure.
 *  @param flags       Lock flags.
 *  @param offset      Lock offset.
 *  @param size        Buffer size.
 *  @param buffer      Buffer pointer.
 *
 *  @return            Written byte count --success otherwise negative value.
 */
int32_t f3s_hpf_program(f3s_dbase_t *dbase,
                        f3s_access_t *access,
                        uint32_t flags,
                        uint32_t offset,
                        int32_t size,
                        uint8_t *buffer)
{
    snor_chip_t *chip;

    chip = (snor_chip_t *)access->service->page(&access->socket, 0, offset, &size);
    if (chip == NULL) {
        errno = ERANGE;
        return (-1);
    }

    if (_hpf_unlock(chip) != EOK) {
        errno = EIO;
        return (-1);
    }

    if (_hpf_write_cmd(chip, HYPER_ENTRY_ADDR, HYPER_WORD_PROGRAM_DATA) != EOK) {
        errno = EIO;
        return (-1);
    }

    const int32_t wsize = f3s_snor_program(dbase, access, flags, offset, size, buffer);

    if (wsize <= 0) {
        f3s_hpf_reset(dbase, access, flags, offset);
        return (-1);
    }

    return (wsize);
}

/**
 *  @brief             Set protocol callout for Spansion serial NOR hyper flash.
 *  @param chip        Chip handle.
 *  @param proto       Protocol.
 *
 *  @return            EOK --success otherwise fail.
 */
static int _hpf_set_protocol(struct _snor_chip_t* const chip, const uint32_t proto)
{
    const snor_ctrl_t* const ctrl = chip->ctrl;
    uint16_t vcr;

    if (_hpf_read_vcr(chip, &vcr) != EOK) return (EIO);

    /* Command line opts set driver strength */
    if (chip->drv_type != SNOR_NO_DRVSTRG_OPT) {
        if (chip->drv_type > HYPER_VCR_ODS_MASK) {
            snor_slogf(_SLOG_WARNING, ctrl->verbosity, 3, "%s: Invalid driver strength: 0x%x", __func__, chip->drv_type);
        } else {
            if (((vcr >> HYPER_VCR_ODS_SHIFT) & HYPER_VCR_ODS_MASK) != chip->drv_type) {
                vcr &= ~(HYPER_VCR_ODS_MASK << HYPER_VCR_ODS_SHIFT);
                vcr |= (uint16_t)((chip->drv_type & HYPER_VCR_ODS_MASK) << HYPER_VCR_ODS_SHIFT);
                snor_slogf(_SLOG_WARNING, ctrl->verbosity, 3, "%s: setting vcr:0x%x", __func__, vcr);
                if (_hpf_load_vcr(chip, vcr) != EOK) return (EIO);
            }
        }
    }

    chip->op_rd.opcode    = HYPER_OPCODE_READ;
    chip->op_rd.dcycle    = 15U;
    chip->op_rd.adrlen    = 4;
    chip->rdr_dc          = 16U;
    chip->op_wr.opcode    = HYPER_OPCODE_WRITE;
    chip->op_wr.adrlen    = 4;

    return (EOK);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
