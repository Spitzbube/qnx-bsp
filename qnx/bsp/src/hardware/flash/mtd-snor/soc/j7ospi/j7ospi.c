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

#include "j7ospi.h"

#ifdef  J7OSPI_UDMA_SUPPORT // This flag tells us PSDKQA is installed
#include "ti/drv/sciclient/sciclient.h"
#include "ti/csl/csl_types.h"
#include "sciclient_fmwMsgParams.h"
#endif

static int j7ospi_map_dacmem(j7ospi_dev_t *const ospi);
static int j7ospi_wait_idle(j7ospi_dev_t *const ospi);
static int j7ospi_cfg_bus(snor_ctrl_t *const snor, snor_cfg_t *const cfg);
static int j7ospi_read(snor_ctrl_t *const snor, const snor_cmd_t *const cmd, uint8_t *const buf, const uint32_t len);

static void j7ospi_read_fifo_data(const j7ospi_dev_t *const ospi, uint8_t *const buf, const uint32_t count)
{
    uint32_t    remain = count;
    uint32_t    *dptr = (uint32_t *)buf;
    uint32_t    data;

    while (remain > 0) {
        data = in32(ospi->mem_vbase);
        if (remain >= sizeof(uint32_t)) {
            *dptr++ = data;
            remain -= sizeof(uint32_t);
        } else {
            memcpy(dptr, &data, remain);
            break;
        }
    }
}

static void j7ospi_write_fifo_data(const j7ospi_dev_t *const ospi, const uint8_t *const buf, const uint32_t count)
{
    uint32_t       remain = count;
    const uint32_t *dptr = (uint32_t *)buf;
    uint32_t       data;

    while (remain > 0) {
        if (remain >= 4) {
            data = *dptr++;
            remain -= 4;
        } else {
            memcpy(&data, dptr, remain);
            remain = 0;
        }

        out32(ospi->mem_vbase, data);
    }
}

static int j7ospi_wait_for_bit(const uintptr_t reg, const uint32_t mask, const int clear)
{
    uint32_t    val;
    uint32_t    timeout = J7OSPI_TIMEOUT_NS / 100;

    while (timeout--) {
        val = in32(reg);
        if (clear) {
            val = ~val;
        }
        val &= mask;

        if (val == mask) return (EOK);

        nanospin_ns(100);
    }

    snor_slogf(_SLOG_ERROR, 0, 0,
        "%s: wait for register %x[%x:%x] %s timed out",
        __func__, reg & (uintptr_t)0xFF, in32(reg), mask, clear ? "clear" : "set");

    return (ETIMEDOUT);
}

static int j7ospi_is_idle(j7ospi_dev_t *const ospi)
{
    const  uint32_t reg = in32(ospi->ctrl_vbase + J7OSPI_CONFIG);

    return (reg & (1 << J7OSPI_CONFIG_IDLE_LSB));
}

static int j7ospi_wait_idle(j7ospi_dev_t *const ospi)
{
    unsigned long count = 0;
    unsigned long timeout = 0;

    while (1) {
        /*
         * Read few times in succession to ensure the controller
         * is indeed idle, that is, the bit does not transition
         * low again.
         */
        if (j7ospi_is_idle(ospi)) {
            if (++count >= 3) return (EOK);
        } else {
            count = 0;
        }

        // 500ms
        if (++timeout > J7OSPI_TIMEOUT_NS / 100) {
            /* Timeout, in busy mode. */
            snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "%s: OSPI is still busy", __func__);
            return (ETIMEDOUT);
        }

        nanospin_ns(100);
    }
}

static void j7ospi_controller_enable(j7ospi_dev_t *const ospi, const int enable)
{
    uint32_t    reg;

    reg = in32(ospi->ctrl_vbase + J7OSPI_CONFIG);

    if (enable) {
        reg |= J7OSPI_CONFIG_ENABLE_MASK;
    } else {
        reg &= ~J7OSPI_CONFIG_ENABLE_MASK;
    }

    out32(ospi->ctrl_vbase + J7OSPI_CONFIG, reg);
}

static void j7ospi_chipselect(j7ospi_dev_t *const ospi, uint8_t chip_select)
{
    const uintptr_t   base = ospi->ctrl_vbase;
    uint32_t    reg;

    reg = in32(base + J7OSPI_CONFIG);
    if (ospi->is_decoded_cs) {
        reg |= J7OSPI_CONFIG_DECODE_MASK;
    } else {
        reg &= ~J7OSPI_CONFIG_DECODE_MASK;

        /* Convert CS if without decoder.
         * CS0 to 4b'1110
         * CS1 to 4b'1101
         * CS2 to 4b'1011
         * CS3 to 4b'0111
         */
        chip_select = 0xF & ~(1 << chip_select);
    }

    reg &= ~(J7OSPI_CONFIG_CHIPSELECT_MASK << J7OSPI_CONFIG_CHIPSELECT_LSB);
    reg |= (chip_select & J7OSPI_CONFIG_CHIPSELECT_MASK) << J7OSPI_CONFIG_CHIPSELECT_LSB;
    out32(base + J7OSPI_CONFIG, reg);

    ospi->current_cs = chip_select;
}

static void j7ospi_configure_sizes(const j7ospi_dev_t *const ospi, const uint32_t page_size, const uint32_t sect_size, const uint16_t addr_width)
{
    const uintptr_t base = ospi->ctrl_vbase;
    uint32_t    reg;
    uint32_t    erase_power2;

    for (erase_power2 = 0; sect_size != (uint32_t)(1 << erase_power2); erase_power2++) {
        ;
    }

    /* configure page size and block size. */
    reg = in32(base + J7OSPI_SIZE_CFG);
    reg &= ~(J7OSPI_SIZE_CFG_PAGE_MASK << J7OSPI_SIZE_CFG_PAGE_LSB);
    reg &= ~(J7OSPI_SIZE_CFG_BLOCK_MASK << J7OSPI_SIZE_CFG_BLOCK_LSB);
    reg &= ~J7OSPI_SIZE_CFG_ADDRESS_MASK;
    reg |= (page_size << J7OSPI_SIZE_CFG_PAGE_LSB);
    reg |= (erase_power2 << J7OSPI_SIZE_CFG_BLOCK_LSB);
    reg |= (addr_width - 1);
    out32(base + J7OSPI_SIZE_CFG, reg);
}

static int j7ospi_exec_flash_cmd(j7ospi_dev_t *const ospi, unsigned int reg)
{
    const uintptr_t base = ospi->ctrl_vbase;
    const uint8_t   opcode = (uint8_t)((reg >> J7OSPI_CMDCTRL_OPCODE_LSB) & J7OSPI_RD_INSTR_OPCODE_MASK);
    uint32_t        ext_reg;
    int   ret;

    /* Set OSPI_OPCODE_EXT_LOWER_REG */
    if (ospi->buscfg.cflgs & SNOR_CFGFLGS_DBOP) {
        ext_reg = in32(ospi->ctrl_vbase + J7OSPI_OPCODE_EXT_LOWER_REG);
        ext_reg &= ~(J7OSPI_RD_INSTR_OPCODE_MASK << J7OSPI_OPCODE_EXT_LOWER_STIG_LSB);
        ext_reg |= (opcode << J7OSPI_OPCODE_EXT_LOWER_STIG_LSB);
        out32(ospi->ctrl_vbase + J7OSPI_OPCODE_EXT_LOWER_REG, ext_reg);
    }

    /* Write the CMDCTRL without start execution. */
    out32(base + J7OSPI_CMDCTRL, reg);
    /* Start execute */
    reg |= J7OSPI_CMDCTRL_EXECUTE_MASK;
    out32(base + J7OSPI_CMDCTRL, reg);

    /* Polling for completion. */
    ret = j7ospi_wait_for_bit(base + J7OSPI_CMDCTRL, J7OSPI_CMDCTRL_INPROGRESS_MASK, 1);
    if (ret != EOK) {
        snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "%s: Flash command execution timed out.", __func__);
        return (ret);
    }

    /* Polling OSPI idle status. */
    ret = j7ospi_wait_idle(ospi);
    if (ret != EOK) {
        snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "%s: Polling OSPI idle timed out.", __func__);
        return (ret);
    }

    return (EOK);
}

static void j7ospi_read_setup(j7ospi_dev_t *const ospi, const snor_cmd_t *const cmd)
{
    uint32_t       reg;
    const uint8_t  opcode = cmd->op->opcode;
    const uint8_t  alen   = cmd->op->adrlen;
    const uint8_t  dcycle = cmd->op->dcycle;
    const uint8_t  dtr    = (uint8_t)((ospi->proto & SNOR_BUSPROTO_DTR_MODE) ? 1 : 0);

    /* OSPI_DEV_INSTR_RD_CONFIG_REG */
    reg = in32(ospi->ctrl_vbase + J7OSPI_RD_INSTR);
    reg &= ~((J7OSPI_RD_INSTR_OPCODE_MASK << J7OSPI_RD_INSTR_OPCODE_LSB) |
        (J7OSPI_RD_INSTR_TYPE_INSTR_MASK << J7OSPI_RD_INSTR_TYPE_INSTR_LSB) |
        (1 << J7OSPI_RD_INSTR_DDR_EN_LSB) |
        (J7OSPI_RD_INSTR_TYPE_ADDR_MASK << J7OSPI_RD_INSTR_TYPE_ADDR_LSB) |
        (J7OSPI_RD_INSTR_TYPE_DATA_MASK << J7OSPI_RD_INSTR_TYPE_DATA_LSB) |
        (J7OSPI_RD_INSTR_DUMMY_MASK << J7OSPI_RD_INSTR_DUMMY_LSB));
    reg |= ((opcode << J7OSPI_RD_INSTR_OPCODE_LSB) |
        (ospi->inst_width << J7OSPI_RD_INSTR_TYPE_INSTR_LSB) |
        (dtr << J7OSPI_RD_INSTR_DDR_EN_LSB) |
        (ospi->addr_width << J7OSPI_RD_INSTR_TYPE_ADDR_LSB) |
        (ospi->data_width << J7OSPI_RD_INSTR_TYPE_DATA_LSB) |
        (dcycle << J7OSPI_RD_INSTR_DUMMY_LSB));
    out32(ospi->ctrl_vbase + J7OSPI_RD_INSTR, reg);

    /* Set address width */
    reg = in32(ospi->ctrl_vbase + J7OSPI_SIZE_CFG);
    reg &= ~J7OSPI_SIZE_CFG_ADDRESS_MASK;
    reg |= (alen - 1) & J7OSPI_SIZE_CFG_ADDRESS_MASK;
    out32(ospi->ctrl_vbase + J7OSPI_SIZE_CFG, reg);

    /* Set OSPI_OPCODE_EXT_LOWER_REG */
    if (ospi->buscfg.cflgs & SNOR_CFGFLGS_DBOP) {
        reg = in32(ospi->ctrl_vbase + J7OSPI_OPCODE_EXT_LOWER_REG);
        reg &= ~(J7OSPI_RD_INSTR_OPCODE_MASK << J7OSPI_OPCODE_EXT_LOWER_RD_LSB);
        reg |= (opcode << J7OSPI_OPCODE_EXT_LOWER_RD_LSB);
        out32(ospi->ctrl_vbase + J7OSPI_OPCODE_EXT_LOWER_REG, reg);
    }

}

static void j7ospi_write_setup(j7ospi_dev_t *const ospi, const snor_cmd_t *const cmd)
{
    const uintptr_t  base   = ospi->ctrl_vbase;
    const uint8_t    opcode = cmd->op->opcode;
    const uint32_t   adrlen = cmd->op->adrlen;
    const uint8_t    cmd_flgs = cmd->op->flags;
    uint32_t    reg;
    const snor_ctrl_t *const ctrl = (snor_ctrl_t *)ospi;

    /* Set opcode. */
    reg = in32(base + J7OSPI_WR_INSTR);
    reg &= ~((J7OSPI_WR_INSTR_OPCODE_MASK << J7OSPI_WR_INSTR_OPCODE_LSB) |
        (J7OSPI_WR_INSTR_TYPE_ADDR_MASK << J7OSPI_WR_INSTR_TYPE_ADDR_LSB) |
        (J7OSPI_WR_INSTR_TYPE_DATA_MASK << J7OSPI_WR_INSTR_TYPE_DATA_LSB));
    reg |= ((opcode << J7OSPI_WR_INSTR_OPCODE_LSB) |
        (ospi->addr_width << J7OSPI_WR_INSTR_TYPE_ADDR_LSB) |
        (ospi->data_width << J7OSPI_WR_INSTR_TYPE_DATA_LSB));
    out32(base + J7OSPI_WR_INSTR, reg);

    /* OSPI_DEV_INSTR_RD_CONFIG_REG[INSTR_TYPE_FLD] */
    /* NOTE: if (INSTR_TYPE_FLD != 0), we don't care about
       ADDR_XFER_TYPE_STD_MODE_FLD & DATA_XFER_TYPE_EXT_MODE_FLD
       (See J7200 TRM Table 12-6327) */
    reg = in32(base + J7OSPI_RD_INSTR);
    reg &= ~(J7OSPI_RD_INSTR_TYPE_INSTR_MASK << J7OSPI_RD_INSTR_TYPE_INSTR_LSB);
    reg |= ospi->inst_width << J7OSPI_RD_INSTR_TYPE_INSTR_LSB;
    out32(base + J7OSPI_RD_INSTR, reg);

    /* Set address width */
    reg = in32(base + J7OSPI_SIZE_CFG);
    reg &= ~J7OSPI_SIZE_CFG_ADDRESS_MASK;
    reg |= (adrlen - 1) & J7OSPI_SIZE_CFG_ADDRESS_MASK;
    out32(base + J7OSPI_SIZE_CFG, reg);

    /* Set OSPI_OPCODE_EXT_LOWER_REG */
    if (ospi->buscfg.cflgs & SNOR_CFGFLGS_DBOP) {
        reg = in32(ospi->ctrl_vbase + J7OSPI_OPCODE_EXT_LOWER_REG);
        reg &= ~(J7OSPI_WR_INSTR_OPCODE_MASK << J7OSPI_OPCODE_EXT_LOWER_WR_LSB);
        reg |= (opcode << J7OSPI_OPCODE_EXT_LOWER_WR_LSB);
        out32(ospi->ctrl_vbase + J7OSPI_OPCODE_EXT_LOWER_REG, reg);
    }

    /* Set auto WEL */
    reg = in32(base + J7OSPI_WR_INSTR);
    /* Command needs auto WEL */
    if (cmd_flgs & SNOR_OPFLGS_AWREN) {
        /* Enable auto WEL */
        if (reg & J7OSPI_WR_INSTR_WEL_DIS) {
            reg &= ~J7OSPI_WR_INSTR_WEL_DIS;
            out32(base + J7OSPI_WR_INSTR, reg);
        }
    } else {
        /* Disable auto WEL */
        if (!(reg & J7OSPI_WR_INSTR_WEL_DIS)) {
            reg |= J7OSPI_WR_INSTR_WEL_DIS;
            out32(base + J7OSPI_WR_INSTR, reg);
        }
    }

    /* Set auto status polling */
    reg = in32(base + J7OSPI_WRITE_COMPLETION_CTRL);
    /* Command needs auto status polling */
    if (cmd_flgs & SNOR_OPFLGS_ASP) {
        /* Enable auto-polling */
        if (reg & J7OSPI_WCC_DISABLE_POLLING) {
            reg &= ~J7OSPI_WCC_DISABLE_POLLING;
        }

        reg &= ~(J7OSPI_WCC_OPCODE_MASK | J7OSPI_WCC_POLLING_BIT_MASK | J7OSPI_WCC_POLLING_POLARITY);
        reg |= (ctrl->chip[cmd->cfg->cs].sp_op << 0) | (ctrl->chip[cmd->cfg->cs].sp_bit << 8) | (ctrl->chip[cmd->cfg->cs].sp_pol << 13);
        out32(base + J7OSPI_WRITE_COMPLETION_CTRL, reg);

        /* Set number of dummy cycles for auto-polling */
        reg = in32(base + J7OSPI_POLLING_FLASH_STATUS);
        if ((ospi->proto & SNOR_BUSPROTO_BUS_MASK) == SNOR_BUSPROTO_8_8_8) { /* Octal mode */
            if ((reg & J7OSPI_DEV_STATUS_NB_DUMMY_MASK) != J7OSPI_DEV_STATUS_NB_DUMMY_OCTAL) {
                reg &= ~(J7OSPI_DEV_STATUS_NB_DUMMY_MASK);
                reg |= J7OSPI_DEV_STATUS_NB_DUMMY_OCTAL;
                out32(base + J7OSPI_POLLING_FLASH_STATUS, reg);
            }
        } else {
            if ((reg & J7OSPI_DEV_STATUS_NB_DUMMY_MASK) != 0) {
                /* Clear dummy cycle field */
                reg &= ~(J7OSPI_DEV_STATUS_NB_DUMMY_MASK);
                out32(base + J7OSPI_POLLING_FLASH_STATUS, reg);
            }
        }

        if (ospi->buscfg.cflgs & SNOR_CFGFLGS_DBOP) {
            reg = in32(base + J7OSPI_OPCODE_EXT_LOWER_REG);
            reg &= ~(J7OSPI_WR_INSTR_OPCODE_MASK << J7OSPI_OPCODE_EXT_LOWER_POLL_LSB);
            reg |= (ctrl->chip[cmd->cfg->cs].sp_op << J7OSPI_OPCODE_EXT_LOWER_POLL_LSB);
            out32(base + J7OSPI_OPCODE_EXT_LOWER_REG, reg);
        }

    } else {
        /* Disable auto-polling */
        if (!(reg & J7OSPI_WCC_DISABLE_POLLING)) {
            reg |= J7OSPI_WCC_DISABLE_POLLING;
            out32(base + J7OSPI_WRITE_COMPLETION_CTRL, reg);
        }
    }
}

static uint32_t calculate_ticks_for_ns(const uint32_t ref_clk_hz,
                        const uint32_t ns_val)
{
    uint32_t    ticks;

    ticks = ref_clk_hz / 1000;  /* kHz */
    ticks = (ticks * ns_val + 1000000 - 1) / 1000000;

    return (ticks);
}

static void j7ospi_readdata_capture(j7ospi_dev_t *const ospi,
                                const int bypass, const uint32_t read_delay, const uint8_t dqs_en)
{
    uint32_t    reg = in32(ospi->ctrl_vbase + J7OSPI_READCAPTURE);

    if (bypass) {
        reg |= (1 << J7OSPI_READCAPTURE_BYPASS_LSB);
    } else {
        reg &= ~(1 << J7OSPI_READCAPTURE_BYPASS_LSB);
    }

    reg |= J7OSPI_READCAPTURE_SAMPLE_EDGE;

    if (dqs_en) {
        reg |= J7OSPI_READCAPTURE_DQS;
    } else {
        reg &= ~(J7OSPI_READCAPTURE_DQS);
    }

    reg &= ~(J7OSPI_READCAPTURE_DELAY_MASK << J7OSPI_READCAPTURE_DELAY_LSB);
    reg |= (read_delay & J7OSPI_READCAPTURE_DELAY_MASK) << J7OSPI_READCAPTURE_DELAY_LSB;

    out32(ospi->ctrl_vbase + J7OSPI_READCAPTURE, reg);
}

static void j7ospi_delay(j7ospi_dev_t *const ospi)
{
    uint32_t    tshsl, tchsh, tslch, tsd2d;
    uint32_t    reg;
    uint32_t    tsclk;

    tsclk = (ospi->refclk + ospi->busclk - 1) / ospi->busclk;

    tshsl = calculate_ticks_for_ns(ospi->refclk, ospi->tshsl_ns);
    /* this particular value must be at least one sclk */
    if (tshsl < tsclk) {
        tshsl = tsclk;
    }

    tchsh = calculate_ticks_for_ns(ospi->refclk, ospi->tchsh_ns);
    tslch = calculate_ticks_for_ns(ospi->refclk, ospi->tslch_ns);
    tsd2d = calculate_ticks_for_ns(ospi->refclk, ospi->tsd2d_ns);

    reg  = (tshsl & J7OSPI_DELAY_TSHSL_MASK) << J7OSPI_DELAY_TSHSL_LSB;
    reg |= (tchsh & J7OSPI_DELAY_TCHSH_MASK) << J7OSPI_DELAY_TCHSH_LSB;
    reg |= (tslch & J7OSPI_DELAY_TSLCH_MASK) << J7OSPI_DELAY_TSLCH_LSB;
    reg |= (tsd2d & J7OSPI_DELAY_TSD2D_MASK) << J7OSPI_DELAY_TSD2D_LSB;

    out32(ospi->ctrl_vbase + J7OSPI_DELAY, reg);
}

static uint64_t j7ospi_get_refclk(const j7ospi_dev_t *const ospi)
{
#ifdef  J7OSPI_UDMA_SUPPORT // This flag tells us PSDKQA is installed
    struct tisci_msg_get_freq_req reqFreq ;
    struct tisci_msg_get_freq_resp const respFreq = {0};

    /* Fill in payload */
    snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: J7OSPI_FREQ_DEV = %d, J7OSPI_FREQ_CLK = %d", __func__, J7OSPI_FREQ_DEV, J7OSPI_FREQ_CLK);
    reqFreq.device = J7OSPI_FREQ_DEV;

    if (J7OSPI_FREQ_CLK >= 255U) {
        reqFreq.clk32 = J7OSPI_FREQ_CLK;
        reqFreq.clk   = (uint8_t) 255U;
    } else {
        reqFreq.clk   = (uint8_t) J7OSPI_FREQ_CLK;
    }

    /* Create Request message */
    const Sciclient_ReqPrm_t      reqPrm = {
        .messageType = TISCI_MSG_GET_FREQ,
        .flags = TISCI_MSG_FLAG_AOP,
        .pReqPayload = (uint8_t *) &reqFreq,
        .reqPayloadSize = sizeof(reqFreq),
        .timeout = SCICLIENT_SERVICE_WAIT_FOREVER,
        .forwardStatus = 0
    };

    /* Create response buffer */
    Sciclient_RespPrm_t           respPrm = {
        .flags = 0,
        .pRespPayload = (uint8_t *) &respFreq,
        .respPayloadSize = sizeof (respFreq)
    };

    /* Send request */
    if (Sciclient_service(&reqPrm, &respPrm) == CSL_PASS) {
        if (respPrm.flags == TISCI_MSG_FLAG_ACK) {
            return (respFreq.freq_hz);
        }
    }

    snor_slogf(_SLOG_WARNING, ospi->ctrl.verbosity, 0,
                    "%s, failed to read OSPI reference clock, use default clock rate(166.7MHz)", __func__);
#endif

    return ((uint64_t)J7OSPI_RCLK);
}

static void j7ospi_config_baudrate_div(j7ospi_dev_t *const ospi, uint32_t clk)
{
    uint32_t    reg, divisor;

    if (clk == ospi->busclk) return;
    if (clk == 0) {
        clk = ospi->busclk;
    }

    if (ospi->refclk == 0) {
        ospi->refclk = (uint32_t)j7ospi_get_refclk(ospi);
    }

    /* calculate the baudrate divisor */
    divisor = ((ospi->refclk + clk - 1) / clk) >> 1;
    if (divisor > 0) {
        if (--divisor > J7OSPI_CONFIG_BAUD_MASK) {
            divisor = J7OSPI_CONFIG_BAUD_MASK;
        }
    }

    snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 2,
                    "%s, set data rate to %dHz", __func__, ospi->refclk / (divisor + 1) / 2);

    reg = in32(ospi->ctrl_vbase + J7OSPI_CONFIG);
    reg &= ~(J7OSPI_CONFIG_BAUD_MASK << J7OSPI_CONFIG_BAUD_LSB);
    reg |= (divisor & J7OSPI_CONFIG_BAUD_MASK) << J7OSPI_CONFIG_BAUD_LSB;
    out32(ospi->ctrl_vbase + J7OSPI_CONFIG, reg);

    ospi->busclk = clk;
}

static void j7ospi_dac_enable(const j7ospi_dev_t *const ospi, const uint8_t enable)
{
    const uintptr_t base = ospi->ctrl_vbase;
    uint32_t    reg;

    reg = in32(base + J7OSPI_CONFIG);
    if (enable) {
        reg |= J7OSPI_CONFIG_ENB_DIR_ACC_CTRL;
    } else {
        reg &= ~J7OSPI_CONFIG_ENB_DIR_ACC_CTRL;
    }
    out32(base + J7OSPI_CONFIG, reg);
}

static void j7ospi_controller_init(j7ospi_dev_t *const ospi)
{
    const uintptr_t base = ospi->ctrl_vbase;
    uint32_t        reg;

    /* Disable controller */
    j7ospi_controller_enable(ospi, 0);

    /* Reset all configure bits */
    out32(ospi->ctrl_vbase + J7OSPI_CONFIG, 0x01u << J7OSPI_CONFIG_IDLE_LSB);

    /* Enable auto WEL */
    reg = in32(base + J7OSPI_WR_INSTR);
    if (reg & J7OSPI_WR_INSTR_WEL_DIS) {
        snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: Enable auto WEL", __func__);
        reg &= ~J7OSPI_WR_INSTR_WEL_DIS;
        out32(base + J7OSPI_WR_INSTR, reg);
    }
    snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: J7OSPI_WR_INSTR: 0x%x", __func__, in32(base + J7OSPI_WR_INSTR));

    /* Enable auto status polling */
    reg = in32(base + J7OSPI_WRITE_COMPLETION_CTRL);
    if (reg & J7OSPI_WCC_DISABLE_POLLING) {
        snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: Enable auto status polling", __func__);
        reg &= ~J7OSPI_WCC_DISABLE_POLLING;
    }

    /* Set auto status polling count */
    reg &= ~(J7OSPI_WCC_POLL_COUNT_MASK);
    reg |= J7OSPI_WCC_POLL_COUNT_MIN;
    out32(base + J7OSPI_WRITE_COMPLETION_CTRL, reg);
    snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: J7OSPI_WRITE_COMPLETION_CTRL: 0x%x", __func__, in32(base + J7OSPI_WRITE_COMPLETION_CTRL));

    /* Configure the remap address register, no remap */
    out32(base + J7OSPI_REMAP, 0);

    /* Disable all interrupts. */
    out32(base + J7OSPI_IRQMASK, 0);

    /* Configure the SRAM split to 1:1 . */
    out32(base + J7OSPI_SRAMPARTITION, ospi->fifo_depth / 2);

    /* WEL opcode . */
    out32(base + J7OSPI_OPCODE_EXT_UPPER_REG, 0x06060000);

    /* Load indirect trigger address. */
    if (ospi->dma_enable) {
        out32(base + J7OSPI_INDIRECTTRIGGER, 0x10000000);
    } else {
        out32(base + J7OSPI_INDIRECTTRIGGER, 0);
    }

    /* Program read watermark -- 1/2 of the FIFO. */
    out32(base + J7OSPI_INDIRECTRDWATERMARK, ospi->fifo_depth / 2 / 2);
    /* Program write watermark -- 1/8 of the FIFO. */
    out32(base + J7OSPI_INDIRECTWRWATERMARK, ospi->fifo_depth / 2 / 4);

    /* Enable Direct Access Controller if DMA enabled */
    j7ospi_dac_enable(ospi, ospi->dma_enable);

    /* Configure bus clock */
    j7ospi_config_baudrate_div(ospi, 0);
    j7ospi_delay(ospi);
    j7ospi_readdata_capture(ospi, !ospi->rclk_en, ospi->read_delay, ospi->dqs_en);

    j7ospi_controller_enable(ospi, 1);
}

static int j7ospi_command_read(j7ospi_dev_t *const ospi,
        const snor_cmd_t *const cmd, uint8_t *rxbuf, const uint32_t n_rx)
{
    const uintptr_t  base   = ospi->ctrl_vbase;
    const uint8_t    opcode = cmd->op->opcode;
    const uint8_t    dcycle = cmd->op->dcycle;
    uint32_t    reg;
    uint32_t    read_len;
    int         status;

    if ((n_rx == 0) || (n_rx > J7OSPI_STIG_DATA_LEN_MAX) || (rxbuf == NULL)) {
        snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "Invalid length, len %d rxbuf 0x%p", n_rx, rxbuf);
        return (EINVAL);
    }

    reg = (uint32_t)(opcode << J7OSPI_CMDCTRL_OPCODE_LSB);
    reg |= (0x1 << J7OSPI_CMDCTRL_RD_EN_LSB);
    /* 0 means 1 byte. */
    reg |= (((n_rx - 1) & J7OSPI_CMDCTRL_RD_BYTES_MASK) << J7OSPI_CMDCTRL_RD_BYTES_LSB);
    /* dummy cycle */
    reg |= (dcycle & J7OSPI_CMDCTRL_DUMMY_MASK) << J7OSPI_CMDCTRL_DUMMY_LSB;

    if (cmd->op->adrlen > 0) {
        reg |= (1 << J7OSPI_CMDCTRL_ADDR_EN_LSB);
        reg |= ((cmd->op->adrlen - 1) & J7OSPI_CMDCTRL_ADD_BYTES_MASK) << J7OSPI_CMDCTRL_ADD_BYTES_LSB;
        out32(ospi->ctrl_vbase + J7OSPI_CMDADDRESS, cmd->addr);
    }

    status = j7ospi_exec_flash_cmd(ospi, reg);
    if (status != EOK) return (status);

    reg = in32(base + J7OSPI_CMDREADDATALOWER);

    /* Put the read value into rx_buf */
    read_len = (n_rx > 4) ? 4 : n_rx;
    memcpy(rxbuf, &reg, read_len);

    if (n_rx > 4) {
        rxbuf += 4;
        reg = in32(base + J7OSPI_CMDREADDATAUPPER);

        read_len = n_rx - read_len;
        read_len = (read_len > 4) ? 4 : read_len;
        memcpy(rxbuf, &reg, read_len);
    }

    return (EOK);
}

static int j7ospi_command_write(j7ospi_dev_t *const ospi, const snor_cmd_t *const cmd,
                                   const uint8_t *const txbuf, const unsigned n_tx)
{
    const uintptr_t   base = ospi->ctrl_vbase;
    uint32_t    reg;
    uint64_t    data;

    if ((n_tx > J7OSPI_STIG_DATA_LEN_MAX) || (n_tx && !txbuf)) {
        snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "Invalid length, len %d rxbuf 0x%p", n_tx, txbuf);
        return (EINVAL);
    }

    reg = (uint32_t)(cmd->op->opcode << J7OSPI_CMDCTRL_OPCODE_LSB);
    if (cmd->op->adrlen > 0) {
        reg |= (1 << J7OSPI_CMDCTRL_ADDR_EN_LSB);
        reg |= ((cmd->op->adrlen - 1) & J7OSPI_CMDCTRL_ADD_BYTES_MASK) << J7OSPI_CMDCTRL_ADD_BYTES_LSB;
        out32(ospi->ctrl_vbase + J7OSPI_CMDADDRESS, cmd->addr);
    }
    if (n_tx) {
        reg |= (0x1 << J7OSPI_CMDCTRL_WR_EN_LSB);
        reg |= ((n_tx - 1) & J7OSPI_CMDCTRL_WR_BYTES_MASK) << J7OSPI_CMDCTRL_WR_BYTES_LSB;
        data = 0;
        memcpy(&data, txbuf, n_tx);
        out32(base + J7OSPI_CMDWRITEDATALOWER, (uint32_t)data);
        if (n_tx > 4) {
            out32(base + J7OSPI_CMDWRITEDATAUPPER, (uint32_t)(data >> 32));
        }
    }

    return j7ospi_exec_flash_cmd(ospi, reg);
}

#ifdef  J7OSPI_UDMA_SUPPORT
static void j7ospi_phy_enable(j7ospi_dev_t *const ospi, const int enable)
{
    const uintptr_t   base = ospi->ctrl_vbase;
    uint32_t    reg;

    reg = in32(base + J7OSPI_CONFIG);

    if (enable) {
        reg |= (J7OSPI_CONFIG_PHY | J7OSPI_CONFIG_PHY_PIPELINE);
    } else {
        reg &= ~(J7OSPI_CONFIG_PHY | J7OSPI_CONFIG_PHY_PIPELINE);
    }

    out32(base + J7OSPI_CONFIG, reg);

    if (ospi->dqs_en && enable) {
        j7ospi_readdata_capture(ospi, !ospi->rclk_en, ospi->read_delay, 1);
    } else {
        j7ospi_readdata_capture(ospi, !ospi->rclk_en, ospi->read_delay, 0);
    }

    if (j7ospi_wait_idle(ospi) != EOK) {
        snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "%s: wait idle failed", __func__);
    }
}
#endif

#ifdef  J7OSPI_UDMA_SUPPORT
static int j7ospi_direct_read_execute_dma(j7ospi_dev_t *const ospi, uint8_t *const buf, const uint32_t from, const uint32_t len)
{
    int     ret;

    if (ospi->use_phy) {
        j7ospi_phy_enable(ospi, 1);
    }

    ret = j7ospi_wait_idle(ospi);
    if (ret == EOK) {
        // PHY pipeline needs 16B alignment
        const uint32_t align   = from & 0x0f;
        const uint32_t readlen = (len + align + 15) & 0xfffffff0;
        const paddr_t  src = ospi->mem_pbase + (from & ~0x0fu);
        ret = j7ospi_udma_xfer(ospi, src, (paddr_t)ospi->p_buf, readlen);
        if (ret == EOK) {
            memcpy(buf, (uint8_t *)ospi->v_buf + align, (size_t)len);
        }

        ret = j7ospi_wait_idle(ospi);
    }

    if (ospi->use_phy) {
        j7ospi_phy_enable(ospi, 0);
    }

    if (ret == EOK) {
        return (len);
    } else {
        return (-1);
    }
}
#endif

static void j7ospi_set_tx_dll(const j7ospi_dev_t *const ospi, const uint8_t dll)
{
    uint32_t        reg;
    uintptr_t const base = ospi->ctrl_vbase;

    reg = in32(base + J7OSPI_PHY_CONFIG);
    reg &= ~(J7OSPI_PHY_CONFIG_TX_DLL_MASK);
    reg |= ((dll << J7OSPI_PHY_CONFIG_TX_DLL_SHIFT) &
        J7OSPI_PHY_CONFIG_TX_DLL_MASK);
    reg |= J7OSPI_PHY_CONFIG_DLL_RESYNC;

    out32(base + J7OSPI_PHY_CONFIG, reg);
}

static void j7ospi_set_rx_dll(const j7ospi_dev_t *const ospi, const uint8_t dll)
{
    uint32_t        reg;
    uintptr_t const base = ospi->ctrl_vbase;

    reg = in32(base + J7OSPI_PHY_CONFIG);

    reg &= ~(J7OSPI_PHY_CONFIG_RX_DLL_MASK);
    reg |= (dll & J7OSPI_PHY_CONFIG_RX_DLL_MASK);
    reg |= J7OSPI_PHY_CONFIG_DLL_RESYNC;

    out32(base + J7OSPI_PHY_CONFIG, reg);
}

static int j7ospi_get_temp(const int *const temp)
{
    snor_slogf(_SLOG_ERROR, 0, 0, "%s: Can't get temp!", __func__);
    return (EOPNOTSUPP);
}

static void j7ospi_phy_apply_setting(j7ospi_dev_t *ospi, const j7_phy_setting*const  phy)
{
    j7ospi_set_rx_dll(ospi, phy->rx);
    j7ospi_set_tx_dll(ospi, phy->tx);
    ospi->read_delay = phy->read_delay;
}

static int j7ospi_phy_check_pattern(j7ospi_dev_t *const ospi)
{
    uint8_t     read_data[J7OSPI_TUNING_PATTERN_SIZE];
    int         ret = EOK;
    snor_ctrl_t *const snor = (snor_ctrl_t *)ospi;
    snor_chip_t *chip = &snor->chip[ospi->current_cs];
    snor_cmd_t  cmd;

    static const uint8_t phy_tuning_pattern[J7OSPI_TUNING_PATTERN_SIZE] = {
    0xFE, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFE, 0xFE, 0x01, 0x01,
    0x01, 0x01, 0x00, 0x00, 0xFE, 0xFE, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
    0x00, 0xFE, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFE, 0xFE, 0xFF, 0x01,
    0x01, 0x01, 0x01, 0x01, 0xFE, 0x00, 0xFE, 0xFE, 0x01, 0x01, 0x01, 0x01, 0xFE,
    0x00, 0xFE, 0xFE, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0xFE, 0xFE,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0xFE, 0xFE, 0xFF, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x00, 0xFE, 0xFE, 0xFE, 0x01, 0x01, 0x01, 0x01, 0x00, 0xFE, 0xFE, 0xFE,
    0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFE, 0xFE, 0xFE, 0xFF, 0xFF, 0xFF,
    0xFF, 0x00, 0xFE, 0xFE, 0xFE, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFE, 0xFE,
    0xFE, 0xFE, 0x01, 0x01, 0x01, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01,
    };

    SNOR_SET_CMD(cmd, &chip->op_rd, &chip->rdcfg, ospi->pattern_offset & ~1);

    if (j7ospi_read(snor, &cmd, read_data, J7OSPI_TUNING_PATTERN_SIZE) != J7OSPI_TUNING_PATTERN_SIZE) {
        snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "%s: unaligned read failed", __func__);
        return (EIO);
    }

    if (memcmp(read_data, phy_tuning_pattern, J7OSPI_TUNING_PATTERN_SIZE)) {
        ret = EIO;
    }

    return (ret);
}

static int j7ospi_find_rx_low(j7ospi_dev_t *const ospi, j7_phy_setting* phy)
{
    int ret;

    do {
        phy->rx = 0;
        do {
            j7ospi_phy_apply_setting(ospi, phy);
            ret = j7ospi_phy_check_pattern(ospi);
            if (!ret) {
                snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: Found RX low", __func__);
                return (EOK);
            }

            phy->rx++;
        } while (phy->rx <= J7OSPI_PHY_LOW_RX_BOUND);

        phy->read_delay++;
    } while (phy->read_delay <= J7OSPI_PHY_MAX_RD);

    snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "%s: Unable to find RX low", __func__);

    return (EIO);
}

static int j7ospi_find_rx_high(j7ospi_dev_t *const ospi, j7_phy_setting* phy)
{
    int ret;

    do {
        phy->rx = J7OSPI_PHY_MAX_RX;
        do {
            j7ospi_phy_apply_setting(ospi, phy);
            ret = j7ospi_phy_check_pattern(ospi);
            if (!ret) {
                snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: Found RX high", __func__);
                return (EOK);
            }

            phy->rx--;
        } while (phy->rx >= J7OSPI_PHY_HIGH_RX_BOUND);

        phy->read_delay++;
    } while (phy->read_delay <= J7OSPI_PHY_MAX_RD);

    snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "%s: Unable to find RX high", __func__);

    return (EIO);
}

static int j7ospi_find_tx_low(j7ospi_dev_t *const ospi, j7_phy_setting* phy)
{
    int ret;

    do {
        phy->tx = 0;
        do {
            j7ospi_phy_apply_setting(ospi, phy);
            ret = j7ospi_phy_check_pattern(ospi);
            if (!ret) {
                snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: Found TX low", __func__);
                return (EOK);
            }

            phy->tx++;
        } while (phy->tx <= J7OSPI_PHY_LOW_TX_BOUND);

        phy->read_delay++;
    } while (phy->read_delay <= J7OSPI_PHY_MAX_RD);

    snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "%s: Unable to find TX low", __func__);

    return (EIO);
}

static int j7ospi_find_tx_high(j7ospi_dev_t *const ospi, j7_phy_setting* phy)
{
    int ret;

    do {
        phy->tx = J7OSPI_PHY_MAX_TX;
        do {
            j7ospi_phy_apply_setting(ospi, phy);
            ret = j7ospi_phy_check_pattern(ospi);
            if (!ret) {
                snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: Found TX high", __func__);
                return (EOK);
            }

            phy->tx--;
        } while (phy->tx >= J7OSPI_PHY_HIGH_TX_BOUND);

        phy->read_delay++;
    } while (phy->read_delay <= J7OSPI_PHY_MAX_RD);

    snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "%s: Unable to find TX high", __func__);

    return (EIO);
}

static int j7ospi_phy_find_gaplow(j7ospi_dev_t *const ospi, const j7_phy_setting *const bottomleft,
                const j7_phy_setting *const topright, j7_phy_setting *gaplow)
{
    j7_phy_setting left, right, mid;
    int ret;

    left = *bottomleft;
    right = *topright;

    mid.tx = (uint8_t)(left.tx + (uint8_t)((right.tx - left.tx) / 2));
    mid.rx = (uint8_t)(left.rx + (uint8_t)((right.rx - left.rx) / 2));
    mid.read_delay = left.read_delay;

    do {
        j7ospi_phy_apply_setting(ospi, &mid);
        ret = j7ospi_phy_check_pattern(ospi);
        if (ret) {
            /*
             * Since we couldn't find the pattern, we need to go to
             * the lower half.
             */
            right.tx = mid.tx;
            right.rx = mid.rx;

            mid.tx = (uint8_t)(left.tx + (uint8_t)((mid.tx - left.tx) / 2));
            mid.rx = (uint8_t)(left.rx + (uint8_t)((mid.rx - left.rx) / 2));
        } else {
            /*
             * Since we found the pattern, we need to go the the
             * upper half.
             */
            left.tx = mid.tx;
            left.rx = mid.rx;

            mid.tx = (uint8_t)(mid.tx + (uint8_t)((right.tx - mid.tx) / 2));
            mid.rx = (uint8_t)(mid.rx + (uint8_t)((right.rx - mid.rx) / 2));
        }

    /* Break the loop if the window has closed. */
    } while ((right.tx - left.tx >= 2) && (right.rx - left.rx >= 2));

    *gaplow = mid;
    return (EOK);
}

static int j7ospi_phy_find_gaphigh(j7ospi_dev_t *const ospi, const j7_phy_setting *const bottomleft,
                const j7_phy_setting *const topright, j7_phy_setting *gaphigh)
    {
    j7_phy_setting left, right, mid;
    int ret;

    left = *bottomleft;
    right = *topright;

    mid.tx = (uint8_t)(left.tx + (uint8_t)((right.tx - left.tx) / 2));
    mid.rx = (uint8_t)(left.rx + (uint8_t)((right.rx - left.rx) / 2));
    mid.read_delay = right.read_delay;

    do {
        j7ospi_phy_apply_setting(ospi, &mid);
        ret = j7ospi_phy_check_pattern(ospi);
        if (ret) {
            /*
             * Since we couldn't find the pattern, we need to go the
             * the upper half.
             */
            left.tx = mid.tx;
            left.rx = mid.rx;

            mid.tx = (uint8_t)(mid.tx + (uint8_t)((right.tx - mid.tx) / 2));
            mid.rx = (uint8_t)(mid.rx + (uint8_t)((right.rx - mid.rx) / 2));
        } else {
            /*
             * Since we found the pattern, we need to go to the
             * lower half.
             */
            right.tx = mid.tx;
            right.rx = mid.rx;

            mid.tx = (uint8_t)(left.tx + (uint8_t)((mid.tx - left.tx) / 2));
            mid.rx = (uint8_t)(left.rx + (uint8_t)((mid.rx - left.rx) / 2));
        }

    /* Break the loop if the window has closed. */
    } while ((right.tx - left.tx >= 2) && (right.rx - left.rx >= 2));

    *gaphigh = mid;
    return (EOK);
}

static int j7ospi_calibrate_phy(j7ospi_dev_t *const ospi, const snor_cmd_t *const cmd)
{
    j7_phy_setting  rxlow, rxhigh, txlow, txhigh, temp;
    j7_phy_setting  bottomleft, topright, searchpoint, gaplow, gaphigh;
    uintptr_t const base = ospi->ctrl_vbase;
    int             ret, tmp, val;
    uint32_t        reg;

    /* Reset PHY */
    reg = in32(base + J7OSPI_CONFIG);
    reg &= ~(J7OSPI_CONFIG_PHY | J7OSPI_CONFIG_PHY_PIPELINE);
    out32(base + J7OSPI_CONFIG, reg);

    /* Look for RX boundaries at lower TX range. */
    rxlow.tx = J7OSPI_PHY_TX_START;

    do {
        snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: Searching for rxlow on TX = %d", __func__, rxlow.tx);

        rxlow.read_delay = J7OSPI_PHY_INIT_RD;
        ret = j7ospi_find_rx_low(ospi, &rxlow);
        rxlow.tx++;
    } while (ret && (rxlow.tx <= J7OSPI_PHY_TX_LOOKUP_LOW_BOUND));

    if (ret != EOK) return (ret);

    snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: rxlow: RX: %d TX: %d RD: %d",
            __func__, rxlow.rx, rxlow.tx, rxlow.read_delay);

    rxhigh.tx = rxlow.tx;
    rxhigh.read_delay = rxlow.read_delay;
    j7ospi_find_rx_high(ospi, &rxhigh);

    if (ret != EOK) return (ret);

    snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: rxhigh: RX: %d TX: %d RD: %d",
            __func__, rxhigh.rx, rxhigh.tx, rxhigh.read_delay);

    /*
     * Check a different point if rxlow and rxhigh are on the same read
     * delay. This avoids mistaking the failing region for an RX boundary.
     */
    if (rxlow.read_delay == rxhigh.read_delay) {
        snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: rxlow and rxhigh at the same read delay.", __func__);

        /* Look for RX boundaries at upper TX range. */
        temp.tx = J7OSPI_PHY_TX_END;

        do {
            snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: Searching for rxlow on TX = %d", __func__, temp.tx);
            temp.read_delay = J7OSPI_PHY_INIT_RD;
            ret = j7ospi_find_rx_low(ospi, &temp);
            temp.tx--;
        } while (ret && (temp.tx >= J7OSPI_PHY_TX_LOOKUP_HIGH_BOUND));

        if (ret != EOK) return (ret);

        snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: rxlow: RX: %d TX: %d RD: %d",
                __func__, temp.rx, temp.tx, temp.read_delay);

        if (temp.rx < rxlow.rx) {
            rxlow = temp;
            snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: Updating rxlow to the one at TX = 48", __func__);
        }

        /* Find RX max. */
        ret = j7ospi_find_rx_high(ospi, &temp);
        if (ret != EOK) return (ret);

        snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: rxlow: RX: %d TX: %d RD: %d",
                __func__, temp.rx, temp.tx, temp.read_delay);

        if (temp.rx < rxhigh.rx) {
            rxhigh = temp;
            snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: Updating rxhigh to the one at TX = 48.", __func__);
        }
    }

    /* Look for TX boundaries at 1/4 of RX window. */
    txlow.rx = (uint8_t)(rxlow.rx + (uint8_t)((rxhigh.rx - rxlow.rx) / 4));
    txhigh.rx = txlow.rx;

    txlow.read_delay = J7OSPI_PHY_INIT_RD;

    snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: Searching for txlow on RX = %d", __func__, txlow.rx);

    ret = j7ospi_find_tx_low(ospi, &txlow);
    if (ret != EOK) return (ret);

    snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: txlow: RX: %d TX: %d RD: %d",
            __func__, txlow.rx, txlow.tx, txlow.read_delay);

    txhigh.read_delay = txlow.read_delay;
    ret = j7ospi_find_tx_high(ospi, &txhigh);
    if (ret != EOK) return (ret);

    snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: txhigh: RX: %d TX: %d RD: %d",
            __func__, txhigh.rx, txhigh.tx, txhigh.read_delay);

    /*
     * Check a different point if txlow and txhigh are on the same read
     * delay. This avoids mistaking the failing region for an TX boundary.
     */
    if (txlow.read_delay == txhigh.read_delay) {
        /* Look for TX boundaries at 3/4 of RX window. */
        temp.rx = (uint8_t)(rxlow.rx + (uint8_t)(3 * (rxhigh.rx - rxlow.rx) / 4));
        temp.read_delay = J7OSPI_PHY_INIT_RD;
        snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: txlow and txhigh at the same read delay. Searching at RX = %d", __func__, temp.rx);

        ret = j7ospi_find_tx_low(ospi, &temp);
        if (ret != EOK) return (ret);

        snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: txlow: RX: %d TX: %d RD: %d",
                __func__, temp.rx, temp.tx, temp.read_delay);

        if (temp.tx < txlow.tx) {
            txlow = temp;
            snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: Updating txlow with the one at RX = %d", __func__, txlow.rx);
        }

        ret = j7ospi_find_tx_high(ospi, &temp);
        if (ret != EOK) return (ret);

        snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: txhigh: RX: %d TX: %d RD: %d",
                __func__, temp.rx, temp.tx, temp.read_delay);

        if (temp.tx < txhigh.tx) {
            txhigh = temp;
            snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: Updating txhigh with the one at RX = %d", __func__, txhigh.rx);
        }
    }

    /*
     * Set bottom left and top right corners. These are theoretical
     * corners. They may not actually be "good" points. But the longest
     * diagonal will be between these corners.
     */
    bottomleft.tx = txlow.tx;
    bottomleft.rx = rxlow.rx;
    if (txlow.read_delay <= rxlow.read_delay) {
        bottomleft.read_delay = txlow.read_delay;
    } else {
        bottomleft.read_delay = rxlow.read_delay;
    }

    temp = bottomleft;
    temp.tx += 4;
    temp.rx += 4;
    j7ospi_phy_apply_setting(ospi, &temp);
    ret = j7ospi_phy_check_pattern(ospi);
    if (ret != EOK) {
        temp.read_delay--;
        j7ospi_phy_apply_setting(ospi, &temp);
        ret = j7ospi_phy_check_pattern(ospi);
    }

    if (ret == EOK) {
        bottomleft.read_delay = temp.read_delay;
    }

    topright.tx = txhigh.tx;
    topright.rx = rxhigh.rx;
    if (txhigh.read_delay >= rxhigh.read_delay) {
        topright.read_delay = txhigh.read_delay;
    } else {
        topright.read_delay = rxhigh.read_delay;
    }

    temp = topright;
    temp.tx -= 4;
    temp.rx -= 4;
    j7ospi_phy_apply_setting(ospi, &temp);
    ret = j7ospi_phy_check_pattern(ospi);
    if (ret != EOK) {
        temp.read_delay++;
        j7ospi_phy_apply_setting(ospi, &temp);
        ret = j7ospi_phy_check_pattern(ospi);
    }

    if (ret == EOK) {
        topright.read_delay = temp.read_delay;
    }

    snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: topright: RX: %d TX: %d RD: %d", __func__, topright.rx,
            topright.tx, topright.read_delay);
    snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: bottomleft: RX: %d TX: %d RD: %d", __func__, bottomleft.rx,
            bottomleft.tx, bottomleft.read_delay);

    ret = j7ospi_phy_find_gaplow(ospi, &bottomleft, &topright, &gaplow);
    if (ret != EOK) return (ret);

    snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: gaplow: RX: %d TX: %d RD: %d", __func__, gaplow.rx, gaplow.tx,
            gaplow.read_delay);

    if (bottomleft.read_delay == topright.read_delay) {
        snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: bottomleft.read_delay == topright.read_delay", __func__);
        /*
         * If there is only one passing region, it means that the "true"
         * topright is too small to find, so the start of the failing
         * region is a good approximation. Put the tuning point in the
         * middle and adjust for temperature.
         */
        topright = gaplow;
        searchpoint.read_delay = bottomleft.read_delay;
        searchpoint.tx = (uint8_t)(bottomleft.tx + (uint8_t)((topright.tx - bottomleft.tx) / 2));
        searchpoint.rx = (uint8_t)(bottomleft.rx + (uint8_t)((topright.rx - bottomleft.rx) / 2));

        val = j7ospi_get_temp(&tmp);
        if (val != EOK) {
            /*
             * Assume room temperature if we couldn't get it from
             * the thermal sensor.
             */
            snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: Unable to get temperature. Assuming room temperature", __func__);
            tmp = J7OSPI_PHY_DEFAULT_TEMP;
        }

        if ((tmp < J7OSPI_PHY_MIN_TEMP) || (tmp > J7OSPI_PHY_MAX_TEMP)) {
            snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "%s: Temperature outside operating range: %dC", __func__, tmp);
            return (ret);
        }
        /* Avoid a divide-by-zero. */
        if (tmp == J7OSPI_PHY_MID_TEMP) {
            tmp++;
        }

        snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: Temperature: %dC", __func__, tmp);

        val = 330 / (tmp - J7OSPI_PHY_MID_TEMP);
        searchpoint.tx += (uint8_t)((topright.tx - bottomleft.tx) / val);
        searchpoint.rx += (uint8_t)((topright.rx - bottomleft.rx) / val);
    } else {
        /*
         * If there are two passing regions, find the start and end of
         * the second one.
         */
        ret = j7ospi_phy_find_gaphigh(ospi, &bottomleft, &topright, &gaphigh);
        if (ret != EOK) return (ret);

        snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: gaphigh: RX: %d TX: %d RD: %d", __func__, gaphigh.rx,
                            gaphigh.tx, gaphigh.read_delay);

        /*
         * Place the final tuning point in the corner furthest from the
         * failing region but leave some margin for temperature changes.
         */
        if ((abs(gaplow.tx - bottomleft.tx) +
             abs(gaplow.rx - bottomleft.rx)) <
            (abs(gaphigh.tx - topright.tx) +
             abs(gaphigh.rx - topright.rx))) {
            searchpoint = topright;
            searchpoint.tx -= 16;
            searchpoint.rx -= (uint8_t)((16 * (topright.rx - bottomleft.rx)) / (topright.tx - bottomleft.tx));
        } else {
            searchpoint = bottomleft;
            searchpoint.tx += 16;
            searchpoint.rx += (uint8_t)((16 * (topright.rx - bottomleft.rx)) / (topright.tx - bottomleft.tx));
        }
    }

    /* Set the final PHY settings we found. */
    j7ospi_phy_apply_setting(ospi, &searchpoint);

    snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: Final tuning point: RX: %d TX: %d RD: %d",
            __func__, searchpoint.rx, searchpoint.tx, searchpoint.read_delay);

    ret = j7ospi_phy_check_pattern(ospi);
    if (ret != EOK) {
        snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "%s: Failed to find pattern at final calibration point", __func__);
        ret = EINVAL;
        return (ret);
    }

    ospi->read_delay = searchpoint.read_delay;

    snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 0, "%s: PHY Calibration succeeded", __func__);

    return (ret);
}

static int j7ospi_write_reg(snor_ctrl_t *const snor, const snor_cmd_t *const cmd, uint8_t *const regs, const uint32_t len)
{
    j7ospi_dev_t *const ospi = (j7ospi_dev_t *)snor;

    if (cmd->cfg != NULL) {
        j7ospi_cfg_bus(snor, cmd->cfg);
    }

    j7ospi_write_setup(ospi, cmd);

    return j7ospi_command_write(ospi, cmd, regs, len);
}

static int j7ospi_wait_write_sram(const j7ospi_dev_t *const ospi, uint32_t *level)
{
    const uintptr_t base = ospi->ctrl_vbase;
    uint32_t  lvl;
    uint32_t  retry = 10000;

    do {
        lvl = in32(base + J7OSPI_SRAMLEVEL);
        lvl = (lvl >> J7OSPI_SRAMLEVEL_WR_LSB) & J7OSPI_SRAMLEVEL_WR_MASK;
        if (lvl < ospi->fifo_depth / 2) {
            *level = lvl;
            return (EOK);
        }
        nanospin_ns(100);
    } while (--retry > 0);

    snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "%s: timed out", __func__);

    return (ETIMEDOUT);
}

static int j7ospi_ind_write(j7ospi_dev_t *const ospi, const snor_cmd_t *const cmd, const uint8_t *buffer, const uint32_t len)
{
    const uintptr_t  base = ospi->ctrl_vbase;
    uint32_t    remain = len;
    uint32_t    level, nbytes;
    int         ret = -1;
    const uint32_t offset = cmd->addr;

    // Set start address
    out32(base + J7OSPI_INDIRECTWRSTARTADDR, offset);
    // Set write length
    out32(base + J7OSPI_INDIRECTWRBYTES, len);
    // reset write water mark
    out32(base + J7OSPI_INDIRECTWRWATERMARK, 0);
    // set write water mark
    out32(base + J7OSPI_INDIRECTWRWATERMARK, ospi->fifo_depth / 2 / 4);

    // start indirect write xfer
    out32(base + J7OSPI_INDIRECTWR,
            in32(base + J7OSPI_INDIRECTWR) | J7OSPI_INDIRECTWR_START_MASK);

    while (remain > 0) {
        ret = j7ospi_wait_write_sram(ospi, &level);
        if (ret != EOK) {
            break;
        }
        nbytes = (ospi->fifo_depth / 2 - level) * 4;
        if (nbytes > remain) {
            nbytes = remain;
        }

        j7ospi_write_fifo_data(ospi, buffer, nbytes);
        buffer += nbytes;
        remain -= nbytes;
    }

    if (ret == EOK) {
        ret = j7ospi_wait_for_bit(base + J7OSPI_INDIRECTWR, J7OSPI_INDIRECTWR_DONE_MASK, 0);
        if (ret == EOK) {
            out32(base + J7OSPI_INDIRECTWR, J7OSPI_INDIRECTWR_DONE_MASK);

            j7ospi_wait_idle(ospi);

            return (len);
        }
    }

    out32(base + J7OSPI_INDIRECTWR, J7OSPI_INDIRECTWR_CANCEL_MASK);

    return (-1);
}

static int j7ospi_write(snor_ctrl_t *const snor, const snor_cmd_t *const cmd, uint8_t *const buf, const uint32_t len)
{
    j7ospi_dev_t *const ospi = (j7ospi_dev_t *)snor;
    int ret;

    if (len == 0) {
        ret = j7ospi_command_write(ospi, cmd, buf, len);
        if (ret != EOK) {
            errno = ret;
            return (-1);
        } else {
            return (len);
        }
    }

    ret = j7ospi_map_dacmem(ospi);
    if (ret != EOK) return (-1);

    j7ospi_write_setup(ospi, cmd);

#ifdef  J7OSPI_UDMA_SUPPORT
    if (ospi->dma_enable) {
        memcpy(ospi->v_buf, buf, len);
        const paddr_t dst = ospi->mem_pbase + cmd->addr;
        ret = j7ospi_udma_xfer(ospi, (paddr_t)ospi->p_buf, dst, len);
        if (ret != EOK) {
            snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "%s: DMA xfer failed, offset, %x, len %x", __func__, cmd->addr, len);
            return (-1);
        }
    } else {
#else
    {
#endif
        return j7ospi_ind_write(ospi, cmd, buf, len);
    }

    ret = j7ospi_wait_idle(ospi);
    if (ret != EOK) {
        errno = ret;
        return (-1);
    }

    return (len);
}

static int j7ospi_read_reg(snor_ctrl_t *const snor, const snor_cmd_t *const cmd, uint8_t *const regs, const uint32_t len)
{
    j7ospi_dev_t  *const ospi = (j7ospi_dev_t *)snor;

    if (cmd->cfg != NULL) {
        j7ospi_cfg_bus(snor, cmd->cfg);
    }

    j7ospi_read_setup(ospi, cmd);

    return j7ospi_command_read(ospi, cmd, regs, len);
}

static int j7ospi_wait_read_sram(const j7ospi_dev_t *const ospi, uint32_t *level)
{
    const uintptr_t base = ospi->ctrl_vbase;
    uint32_t    lvl;
    uint32_t    retry = 10000;

    do {
        lvl = in32(base + J7OSPI_SRAMLEVEL) & J7OSPI_SRAMLEVEL_RD_MASK;
        if (lvl) {
            *level = lvl;
            return (EOK);
        }
        nanospin_ns(100);
    } while (--retry > 0);

    snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "%s: timed out", __func__);

    return (ETIMEDOUT);
}

static int j7ospi_ind_read(j7ospi_dev_t *const ospi, const snor_cmd_t *const cmd, uint8_t *buffer, const uint32_t len)
{
    const uintptr_t base = ospi->ctrl_vbase;
    uint32_t    remain = len;
    uint32_t    level, nbytes;
    int         ret = -1;
    const uint32_t  offset = cmd->addr;

    // Set start address
    out32(base + J7OSPI_INDIRECTRDSTARTADDR, offset);
    // Set read length, always asks for multiple of FIFO width
    out32(base + J7OSPI_INDIRECTRDBYTES, (len + 3) & ~3);
    // Set read water mark
    out32(base + J7OSPI_INDIRECTRDWATERMARK, ospi->fifo_depth / 2 / 2);

    // start indirect read xfer
    out32(base + J7OSPI_INDIRECTRD,
            in32(base + J7OSPI_INDIRECTRD) | J7OSPI_INDIRECTRD_START_MASK);

    while (remain) {
        ret = j7ospi_wait_read_sram(ospi, &level);
        if (ret != EOK) break;
        nbytes = level * sizeof(uint32_t);
        if (nbytes > remain) {
            nbytes = remain;
        }

        j7ospi_read_fifo_data(ospi, buffer, nbytes);
        buffer += nbytes;
        remain -= nbytes;
    }

    if (ret == EOK) {
        ret = j7ospi_wait_for_bit(base + J7OSPI_INDIRECTRD, J7OSPI_INDIRECTRD_DONE_MASK, 0);
        if (ret == EOK) {
            out32(base + J7OSPI_INDIRECTRD, J7OSPI_INDIRECTRD_DONE_MASK);
            return (len);
        }
    }

    out32(base + J7OSPI_INDIRECTRD, J7OSPI_INDIRECTRD_CANCEL_MASK);

    return (-1);
}

static int j7ospi_read(snor_ctrl_t *const snor, const snor_cmd_t *const cmd, uint8_t *const buf, const uint32_t len)
{
    j7ospi_dev_t *const ospi = (j7ospi_dev_t *)snor;
    int ret;

    ret = j7ospi_map_dacmem(ospi);
    if (ret != EOK) return (-1);

    if (cmd->cfg != NULL) {
        j7ospi_cfg_bus(snor, cmd->cfg);
    }

    j7ospi_read_setup(ospi, cmd);

#ifdef  J7OSPI_UDMA_SUPPORT
    if (ospi->dma_enable) {
        return j7ospi_direct_read_execute_dma(ospi, buf, cmd->addr, len);
    } else {
#else
    {
#endif
        return j7ospi_ind_read(ospi, cmd, buf, len);
    }
}

static int j7ospi_dinit(void *const hdl)
{
    j7ospi_dev_t  *const ospi = hdl;

    munmap_device_io(ospi->ctrl_vbase, J7_OSPIC_SIZE);
    munmap_device_io(ospi->mem_vbase, ospi->mem_size);

#ifdef  J7OSPI_UDMA_SUPPORT
    if (ospi->dma_enable) {
        j7ospi_dinit_udma(ospi);
    }
#endif

    free(hdl);

    return (EOK);
}

static void j7ospi_atexit_dinit(void)
{
#ifdef  J7OSPI_UDMA_SUPPORT
    j7ospi_dinit_udma(NULL);
#endif
}

static void j7ospi_dtr_enable(const j7ospi_dev_t *const ospi, const int enable)
{
    const uintptr_t base = ospi->ctrl_vbase;
    uint32_t    reg;

    reg = in32(base + J7OSPI_CONFIG);
    if (enable) {
        reg |= (J7OSPI_CONFIG_DTR_PROTO);
    } else {
        reg &= ~(J7OSPI_CONFIG_DTR_PROTO);
    }
    out32(base + J7OSPI_CONFIG, reg);
}

static int j7ospi_set_bus_protocol(j7ospi_dev_t *const ospi, const uint32_t proto)
{
    if (ospi->proto == proto) return (EOK);

    switch (proto & SNOR_BUSPROTO_MASK) {
        case SNOR_BUSPROTO_1_1_1:
            ospi->inst_width = J7OSPI_BUS_TYPE_SINGLE;
            ospi->addr_width = J7OSPI_BUS_TYPE_SINGLE;
            ospi->data_width = J7OSPI_BUS_TYPE_SINGLE;
            j7ospi_dtr_enable(ospi, 0);
            break;
        case SNOR_BUSPROTO_8_8_8:
            ospi->inst_width = J7OSPI_BUS_TYPE_OCTAL;
            ospi->data_width = J7OSPI_BUS_TYPE_OCTAL;
            ospi->addr_width = J7OSPI_BUS_TYPE_OCTAL;
            j7ospi_dtr_enable(ospi, 0);
            break;
        case SNOR_BUSPROTO_8_8_8_DTR:
            ospi->inst_width = J7OSPI_BUS_TYPE_OCTAL;
            ospi->data_width = J7OSPI_BUS_TYPE_OCTAL;
            ospi->addr_width = J7OSPI_BUS_TYPE_OCTAL;
            j7ospi_dtr_enable(ospi, 1);
            break;
        default:
            return (ENOTSUP);
    }

    ospi->proto = proto;

    return (EOK);
}

static int j7ospi_select_chip(snor_ctrl_t *ctrl, uint8_t cs)
{
    j7ospi_dev_t    *ospi = (j7ospi_dev_t *)ctrl;

    // use default configuration before the chip is identified
    if (ctrl->chip[cs].pagesz == 0) {
        ctrl->chip[cs].pagesz = 256;
        ctrl->chip[cs].addrsz = 4;
        ctrl->chip[cs].sectsz = 1 << 17;
    }

    // chip configuration changed?
    if (cs == ospi->current_cs) {
        if ((ospi->page_size != ctrl->chip[cs].pagesz) ||
            (ospi->sector_size != ctrl->chip[cs].sectsz) ||
            (ospi->chip_size != ctrl->chip[cs].chipsz)) {
            ospi->current_cs++;     // anything other than current_cs
        }
    }

    if (cs != ospi->current_cs) {
        j7ospi_controller_enable(ospi, 0);

        j7ospi_configure_sizes(ospi,
                ctrl->chip[cs].pagesz, ctrl->chip[cs].sectsz, ctrl->chip[cs].addrsz);
        /* configure the chip select */
        j7ospi_chipselect(ospi, cs);

        j7ospi_controller_enable(ospi, 1);

        ospi->chip_size   = ctrl->chip[cs].chipsz;
        ospi->page_size   = ctrl->chip[cs].pagesz;
        ospi->sector_size = ctrl->chip[cs].sectsz;

        ospi->current_cs  = cs;
    }

    return (EOK);
}

static int j7ospi_post_ident(snor_ctrl_t *snor, int cs)
{
    snor_chip_t  *chip = &snor->chip[cs];

    if ((chip->cfg.bus_proto & SNOR_BUSPROTO_BUS_MASK) == SNOR_BUSPROTO_8_8_8) {
        chip->align = 2;
    } else {
        chip->align = 1;
    }

    /* PHY calibration */
    j7ospi_dev_t *const ospi = (j7ospi_dev_t *)snor;
    snor_cmd_t   cmd;
    if (ospi->phy_enable) {
        SNOR_SET_CMD(cmd, &chip->op_rd, &chip->rdcfg, ospi->pattern_offset & ~1);
        ospi->use_phy = 1;
        if (j7ospi_calibrate_phy(ospi, &cmd) != EOK) {
            ospi->use_phy = 0;
            snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "%s: PHY Calibration failed", __func__);
        }
    }

    return (EOK);
}

static int j7ospi_cfg_bus(snor_ctrl_t *const snor, snor_cfg_t *const cfg)
{
    j7ospi_dev_t    *ospi = (j7ospi_dev_t *)snor;

    /* no need to re-config if there is no bus changes */
    if (memcmp(cfg, &ospi->buscfg, sizeof(*cfg)) == 0) return (EOK);

    memcpy(&ospi->buscfg, cfg, sizeof(*cfg));

    j7ospi_select_chip(snor, cfg->cs);

    if ((cfg->clk > 0) && (cfg->clk != ospi->busclk)) {
        j7ospi_controller_enable(ospi, 0);

        j7ospi_config_baudrate_div(ospi, cfg->clk);
        j7ospi_delay(ospi);

        j7ospi_controller_enable(ospi, 1);
    }

    j7ospi_set_bus_protocol(ospi, cfg->bus_proto);

    /* Set Dual-byte Opcode Mode */
    uint32_t reg;
    if (ospi->buscfg.cflgs & SNOR_CFGFLGS_DBOP) {
        reg = in32(ospi->ctrl_vbase + J7OSPI_CONFIG);
        reg |= (1 << J7OSPI_CONFIG_DUAL_BYTE_OPCODE_LSB);
        out32(ospi->ctrl_vbase + J7OSPI_CONFIG, reg);
    } else {
        reg = in32(ospi->ctrl_vbase + J7OSPI_CONFIG);
        reg &= ~(1 << J7OSPI_CONFIG_DUAL_BYTE_OPCODE_LSB);
        out32(ospi->ctrl_vbase + J7OSPI_CONFIG, reg);
    }

    const uint8_t dqs_en = (uint8_t)((cfg->bus_proto & SNOR_BUSPROTO_DQS) ? 1 : 0);
    if (dqs_en != ospi->dqs_en) {
        ospi->dqs_en = dqs_en;
        j7ospi_readdata_capture(ospi, !ospi->rclk_en, ospi->read_delay, ospi->dqs_en);
    }

    return (EOK);
}

static int j7ospi_map_dacmem(j7ospi_dev_t *const ospi)
{
    uint32_t  mapsize;

    if (ospi->mem_size == 0) {     // default direct memory size
        mapsize = J7OSPI_DFLT_MEMSIZE;
    } else if (ospi->mem_size < ospi->chip_size) {  // remap data memory to fit in the entire flash
        munmap_device_io(ospi->mem_vbase, ospi->mem_size);
        ospi->mem_size = 0;
        mapsize = ospi->chip_size;
    } else {
        return (EOK);
    }

    ospi->mem_vbase = mmap_device_io(mapsize, ospi->mem_pbase);
    if (ospi->mem_vbase == (uintptr_t)MAP_FAILED) {
        snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "%s: map OSPI data memory failed", __func__);
        return (errno);
    }

    ospi->mem_size = mapsize;

    return (EOK);
}

static int j7ospi_options(j7ospi_dev_t *const ospi)
{
    char    *value, *freeptr, *options;
    int     opt;
    int     ret = EOK;

    static char *supported_opts[] = {
        "rclk",     // reference clock
#define OPT_RCLK        0
        "clk",      // SPI bus clock
#define OPT_CLK         1
        "base",     // OSPI controller base address
#define OPT_BASE        2
        "data",     // OSPI data port address
#define OPT_DATA        3
        "phy",      // PHY mode
#define OPT_PHY         4
        "poffset",  // PHY tuning pattern offset
#define OPT_POFF        5
        "rdelay",   // Read Delay (cycles)
#define OPT_RDLY        6
    #ifdef  J7OSPI_UDMA_SUPPORT
        "dma",      // DMA mode
#define OPT_DMA         7
        "ch",       // Channel number
#define OPT_CH          8
        "mem",      // typed memory name
#define OPT_MEM         9
    #endif
        NULL
    };

    // default values
    ospi->ctrl_pbase = J7_OSPIC0_BASE;
    ospi->mem_pbase  = J7_OSPIC0_DATA_BASE;
    ospi->refclk     = 0;
    ospi->busclk     = J7OSPI_DFLT_BUSCLK;
    ospi->fifo_depth = J7OSPI_FIFO_DEPTH;
    ospi->fifo_width = J7OSPI_FIFO_WIDTH;
    ospi->tshsl_ns   = 60;
    ospi->tchsh_ns   = 60;
    ospi->tslch_ns   = 60;
    ospi->tsd2d_ns   = 60;
    ospi->rclk_en    = 0;
    ospi->read_delay = 0;
    ospi->dqs_en     = 1;
    ospi->dma_enable = 0;
    ospi->phy_enable = 0;
    ospi->use_phy = 0;
    ospi->pattern_offset = J7OSPI_TUNING_PATTERN_OFFSET;
    ospi->current_cs = 0xFFu;
    ospi->tpmfd      = NOFD;
    ospi->ch         = -1;

    freeptr = ospi->ctrl.soc_opts;
    if (freeptr == NULL) return (ret);

    options = freeptr;
    while ((options != NULL) && (*options != '\0')) {
        opt = snor_soc_getsubopt(&options, supported_opts, &value);
        switch (opt) {
            case OPT_RCLK:
                ret = snor_options_arg_value(__func__, supported_opts[opt], value);
                if (ret == EOK) {
                    ospi->refclk = (uint32_t)strtoul(value, NULL, 0);
                }
                break;
            case OPT_CLK:
                ret = snor_options_arg_value(__func__, supported_opts[opt], value);
                if (ret == EOK) {
                    ospi->busclk = (uint32_t)strtoul(value, NULL, 0);
                }
                break;
            case OPT_BASE:
                ret = snor_options_arg_value(__func__, supported_opts[opt], value);
                if (ret == EOK) {
                    ospi->ctrl_pbase = (paddr_t)strtoul(value, NULL, 0);
                }
                break;
            case OPT_DATA:
                ret = snor_options_arg_value(__func__, supported_opts[opt], value);
                if (ret == EOK) {
                    ospi->mem_pbase = (paddr_t)strtoul(value, NULL, 0);
                }
                break;
            case OPT_PHY:
                ret = snor_options_arg_value(__func__, supported_opts[opt], value);
                if (ret == EOK) {
                    ospi->phy_enable = (uint8_t)strtoul(value, NULL, 0);
                }
                break;
            case OPT_POFF:
                ret = snor_options_arg_value(__func__, supported_opts[opt], value);
                if (ret == EOK) {
                    ospi->pattern_offset = (uint32_t)strtoul(value, NULL, 0);
                }
                break;
            case OPT_RDLY:
                ret = snor_options_arg_value(__func__, supported_opts[opt], value);
                if (ret == EOK) {
                    ospi->read_delay = (uint32_t)strtoul(value, NULL, 0);
                }
                break;
#ifdef  J7OSPI_UDMA_SUPPORT
            case OPT_DMA:
                ret = snor_options_arg_value(__func__, supported_opts[opt], value);
                if (ret == EOK) {
                    ospi->dma_enable = (uint8_t)strtoul(value, NULL, 0);
                }
                break;
            case OPT_CH:
                ret = snor_options_arg_value(__func__, supported_opts[opt], value);
                if (ret == EOK) {
                    ospi->ch = (int)strtoul(value, NULL, 0);
                }
                break;
            case OPT_MEM:
                ret = snor_options_arg_value(__func__, supported_opts[opt], value);
                ospi->tpmfd = posix_typed_mem_open(value, O_RDWR, POSIX_TYPED_MEM_ALLOCATE_CONTIG);
                if (ospi->tpmfd == -1) {
                    snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "%s: Typed memory [%s] open failed", __func__, value);
                    ret = EINVAL;
                }
                break;
#endif
            default:
                snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "%s:unknown option: %s", __func__, value);
                break;
        }

        if (ret != EOK) break;
    }

    free(freeptr);

    return (ret);
}

static int j7ospi_init(j7ospi_dev_t *const ospi)
{
    static const snor_func_t j721e_ospi_func = {
        .dinit = j7ospi_dinit,
        .cfg_bus = j7ospi_cfg_bus,
        .post_ident = j7ospi_post_ident,
        .dstripe = NULL,
        .read_reg = j7ospi_read_reg,
        .write_reg = j7ospi_write_reg,
        .read = j7ospi_read,
        .write = j7ospi_write
    };

    /* check command line options */
    if (j7ospi_options(ospi) != EOK) {
        free(ospi);
        return (ENODEV);
    }

    ospi->ctrl_vbase = mmap_device_io(J7_OSPIC_SIZE, ospi->ctrl_pbase);
    if (ospi->ctrl_vbase == (uintptr_t)MAP_FAILED) {
        snor_slogf(_SLOG_ERROR, ospi->ctrl.verbosity, 0, "map OSPI controller failed");
        free(ospi);
        return (errno);
    }

    /* Temporary until JI2893371 can be fixed
     * The flash resmgr is not calling board specific close */
    atexit(j7ospi_atexit_dinit);

    if (j7ospi_map_dacmem(ospi) != EOK) {
        munmap_device_io(ospi->ctrl_vbase, J7_OSPIC_SIZE);
        free(ospi);
        return (errno);
    }

    j7ospi_wait_idle(ospi);
    j7ospi_controller_init(ospi);

    memcpy(&ospi->ctrl.funcs, &j721e_ospi_func, sizeof(snor_func_t));
    ospi->ctrl.flags  = 0;  // TBD
    ospi->ctrl.hcaps  = SNOR_HCAPS_RD_1_1_1 |
                        SNOR_HCAPS_RD_1_1_1_FAST |
                        SNOR_HCAPS_RD_8_8_8 |
                        SNOR_HCAPS_DTR |
                        SNOR_HCAPS_PP_8_8_8 |
                        SNOR_HCAPS_PP_1_1_1;
    ospi->ctrl.hcaps |= (ospi->dqs_en == 1) ? SNOR_HCAPS_DQS : 0;
    ospi->ctrl.ccaps |= SNOR_CCAPS_PPAWREN |   // controller automatically issues WREN for page program
                        SNOR_CCAPS_PPASP;      // controller automatically polls status for page program

    if (ospi->dma_enable == 0) return (EOK);

#ifdef  J7OSPI_UDMA_SUPPORT
    if (j7ospi_init_udma(ospi) == EOK) {
        return (EOK);
    }

    munmap_device_io(ospi->ctrl_vbase, J7_OSPIC_SIZE);
    munmap_device_io(ospi->mem_vbase, ospi->mem_size);

    free(ospi);
#endif

    return (ENODEV);
}

int32_t f3s_j7ospi_open(f3s_socket_t *socket, const uint32_t flags)
{
    j7ospi_dev_t *dev;

    if (socket->memory) return (EOK);

    /* Allocate driver handle */
    dev = snor_alloc_handle(socket, sizeof(j7ospi_dev_t));
    if (dev == NULL) return (ENOMEM);

    socket->name = (unsigned char*)"TI OSPI";

    return j7ospi_init(dev);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
