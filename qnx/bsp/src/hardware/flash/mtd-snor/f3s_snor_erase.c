/*
 * Copyright (c) 2024, BlackBerry Limited.
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

/**
 *  @brief             Erase callout for SPI serial NOR flash.
 *  @param dbase       Pointer to F3S data base.
 *  @param access      Pointer to flash access structure.
 *  @param flags       Erase flags.
 *  @param offset      Erase offset.
 *
 *  @return            EOK --success otherwise fail.
 */
int f3s_snor_erase(f3s_dbase_t *dbase, f3s_access_t *access, uint32_t flags, uint32_t offset)
{
    snor_ctrl_t *const ctrl = (snor_ctrl_t *)access->socket.memory;
    snor_chip_t *chip;
    snor_cmd_t  cmd;
    snor_op_t   erase;

    chip = (snor_chip_t *)access->service->page(&access->socket, 0, offset, NULL);
    if (chip == NULL) return (ERANGE);

    uint8_t     gidx;  // index in geo_vect array
    uint8_t     eidx;  // index in chip->blkers array
    uint32_t    size = 0;
    uint16_t    unit_pow2 = 0;

    for (gidx = 0; gidx < dbase->geo_num; gidx++) {
        unit_pow2 = dbase->geo_vect[gidx].unit_pow2;
        size += (1u << unit_pow2) * dbase->geo_vect[gidx].unit_num;

        if (ctrl->flags & SNOR_FLG_STRIPE) {
            if (size * chip->cfg.ncs > offset) break;
        } else {
            if (size > offset) break;
        }
    }

    if (ctrl->flags & SNOR_FLG_STRIPE) {
        if (chip->cfg.ncs == 8) {
            unit_pow2 -= 3;
        } else {
            unit_pow2 -= (uint16_t)(chip->cfg.ncs >> 1);
        }
    }

    eidx = chip->eid[gidx];
    erase.dcycle = 0;
    erase.opcode = chip->blkers[eidx].opcode;
    erase.adrlen = 4;

    /* Not in 4B address mode? */
    if (!(chip->flags & SNOR_CFLG_4B_ADDR)) {
        if (chip->chipsz > 16 * 1024 * 1024) {
            /* 3B address can only go up to 16MB,
             * so need 4B address command support
             */
            if (chip->blkers[eidx].opcode_4b == 0) return (ERANGE);
            erase.opcode = chip->blkers[eidx].opcode_4b;
        } else {
            erase.adrlen = 3;
        }
    }

    if (snor_write_cmd(chip, SNOR_CMD_WREN) != EOK) return (EIO);

    if (ctrl->verbosity >= _SLOG_DEBUG2) {
        snor_slogf(_SLOG_DEBUG2, ctrl->verbosity, _SLOG_DEBUG2, "(devf  t%d::%s:%d) gidx %d eidx %d offset 0x%08X op 0x%02X",
                  pthread_self(), __func__, __LINE__, gidx, eidx, chip->offset, erase.opcode);
    }

    SNOR_SET_CMD(cmd, &erase, &chip->cfg, chip->offset);

    if (ctrl->funcs.write(ctrl, &cmd, NULL, 0) != 0) return (EIO);

    return (EOK);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
