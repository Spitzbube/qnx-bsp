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

#ifndef J7OSPI_H_
#define J7OSPI_H_

#include <sys/mman.h>
#include <sys/neutrino.h>
#include <sys/cache.h>
#include <hw/inout.h>

#include "f3s_snor.h"

#define J7_OSPIC0_BASE      0x47040000
#define J7_OSPIC1_BASE      0x47050000

#define J7_OSPIC0_DATA_BASE 0x400000000ULL
#define J7_OSPIC1_DATA_BASE 0x480000000ULL

#define J7_OSPIC_SIZE       0x100
#define J7OSPI_DMABUF_SIZE  (64 * 1024)

#define J7OSPI_RCLK         166666666
#define J7OSPI_DFLT_BUSCLK  5000000
#define J7OSPI_FIFO_DEPTH   256
#define J7OSPI_FIFO_WIDTH   4

/* Instruction type */
#define J7OSPI_BUS_TYPE_SINGLE          0
#define J7OSPI_BUS_TYPE_OCTAL           3

#define J7OSPI_DUMMY_CLKS_MAX           31

#define J7OSPI_STIG_DATA_LEN_MAX        8

/* Register map */
#define J7OSPI_CONFIG                   0x00
#define J7OSPI_CONFIG_ENABLE_MASK       (0x1U << 0)
#define J7OSPI_CONFIG_PHY               (0x1U << 3)
#define J7OSPI_CONFIG_ENB_DIR_ACC_CTRL  (0x1U << 7)
#define J7OSPI_CONFIG_DECODE_MASK       (0x1U << 9)
#define J7OSPI_CONFIG_CHIPSELECT_LSB    10
#define J7OSPI_CONFIG_DMA_MASK          (0x1U << 15)
#define J7OSPI_CONFIG_BAUD_LSB          19
#define J7OSPI_CONFIG_DTR_PROTO         (0x1U << 24)
#define J7OSPI_CONFIG_PHY_PIPELINE      (0x1U << 25)
#define J7OSPI_CONFIG_DUAL_BYTE_OPCODE_LSB  30
#define J7OSPI_CONFIG_IDLE_LSB          31
#define J7OSPI_CONFIG_CHIPSELECT_MASK   0xF
#define J7OSPI_CONFIG_BAUD_MASK         0xF

#define J7OSPI_RD_INSTR                 0x04
#define J7OSPI_RD_INSTR_OPCODE_LSB      0
#define J7OSPI_RD_INSTR_OPCODE_MASK     0xFF
#define J7OSPI_RD_INSTR_TYPE_INSTR_LSB  8
#define J7OSPI_RD_INSTR_DDR_EN_LSB      10
#define J7OSPI_RD_INSTR_TYPE_ADDR_LSB   12
#define J7OSPI_RD_INSTR_TYPE_DATA_LSB   16
#define J7OSPI_RD_INSTR_MODE_EN_LSB     20
#define J7OSPI_RD_INSTR_DUMMY_LSB       24
#define J7OSPI_RD_INSTR_TYPE_INSTR_MASK 0x3
#define J7OSPI_RD_INSTR_TYPE_ADDR_MASK  0x3
#define J7OSPI_RD_INSTR_TYPE_DATA_MASK  0x3
#define J7OSPI_RD_INSTR_DUMMY_MASK      0x1F

#define J7OSPI_WR_INSTR                 0x08
#define J7OSPI_WR_INSTR_OPCODE_LSB      0
#define J7OSPI_WR_INSTR_TYPE_ADDR_LSB   12
#define J7OSPI_WR_INSTR_TYPE_DATA_LSB   16
#define J7OSPI_WR_INSTR_OPCODE_MASK     0xFF
#define J7OSPI_WR_INSTR_TYPE_ADDR_MASK  0x3
#define J7OSPI_WR_INSTR_TYPE_DATA_MASK  0x3
#define J7OSPI_WR_INSTR_WEL_DIS         (0x1U << 8)

#define J7OSPI_DELAY                    0x0C
#define J7OSPI_DELAY_TSLCH_LSB          0
#define J7OSPI_DELAY_TCHSH_LSB          8
#define J7OSPI_DELAY_TSD2D_LSB          16
#define J7OSPI_DELAY_TSHSL_LSB          24
#define J7OSPI_DELAY_TSLCH_MASK         0xFF
#define J7OSPI_DELAY_TCHSH_MASK         0xFF
#define J7OSPI_DELAY_TSD2D_MASK         0xFF
#define J7OSPI_DELAY_TSHSL_MASK         0xFF

#define J7OSPI_READCAPTURE              0x10
#define J7OSPI_READCAPTURE_BYPASS_LSB   0
#define J7OSPI_READCAPTURE_DELAY_LSB    1
#define J7OSPI_READCAPTURE_SAMPLE_EDGE  (0x1U << 5)
#define J7OSPI_READCAPTURE_DQS          (0x1U << 8)
#define J7OSPI_READCAPTURE_DELAY_MASK   0xF

#define J7OSPI_SIZE_CFG                 0x14
#define J7OSPI_SIZE_CFG_ADDRESS_LSB     0
#define J7OSPI_SIZE_CFG_PAGE_LSB        4
#define J7OSPI_SIZE_CFG_BLOCK_LSB       16
#define J7OSPI_SIZE_CFG_ADDRESS_MASK    0xF
#define J7OSPI_SIZE_CFG_PAGE_MASK       0xFFF
#define J7OSPI_SIZE_CFG_BLOCK_MASK      0x3F

#define J7OSPI_SRAMPARTITION            0x18
#define J7OSPI_INDIRECTTRIGGER          0x1C

#define J7OSPI_DMA_CONFIG               0x20
#define J7OSPI_DMA_CONFIG_SINGLE_LSB    0
#define J7OSPI_DMA_CONFIG_BURST_LSB     8
#define J7OSPI_DMA_CONFIG_SINGLE_MASK   0xFF
#define J7OSPI_DMA_CONFIG_BURST_MASK    0xFF

#define J7OSPI_REMAP                    0x24
#define J7OSPI_MODE_BIT                 0x28

#define J7OSPI_SRAMLEVEL                0x2C
#define J7OSPI_SRAMLEVEL_RD_LSB         0
#define J7OSPI_SRAMLEVEL_WR_LSB         16
#define J7OSPI_SRAMLEVEL_RD_MASK        0xFFFF
#define J7OSPI_SRAMLEVEL_WR_MASK        0xFFFF

#define J7OSPI_WRITE_COMPLETION_CTRL    0x38
#define J7OSPI_WCC_OPCODE_MASK          (0xFF)
#define J7OSPI_WCC_OPCODE_LSB           (0)
#define J7OSPI_WCC_POLLING_BIT_MASK     (0x7)
#define J7OSPI_WCC_POLLING_BIT_LSB      (8)
#define J7OSPI_WCC_POLLING_POLARITY     (1 << 13)
#define J7OSPI_WCC_DISABLE_POLLING      (1 << 14)
#define J7OSPI_WCC_POLL_COUNT_MASK      (0xFFU << 16)
#define J7OSPI_WCC_POLL_COUNT_MIN       (3 << 16)

#define J7OSPI_IRQSTATUS                0x40
#define J7OSPI_IRQMASK                  0x44

#define J7OSPI_INDIRECTRD               0x60
#define J7OSPI_INDIRECTRD_START_MASK    (0x1U << 0)
#define J7OSPI_INDIRECTRD_CANCEL_MASK   (0x1U << 1)
#define J7OSPI_INDIRECTRD_DONE_MASK     (0x1U << 5)

#define J7OSPI_INDIRECTRDWATERMARK      0x64
#define J7OSPI_INDIRECTRDSTARTADDR      0x68
#define J7OSPI_INDIRECTRDBYTES          0x6C

#define J7OSPI_INDIRECTWR               0x70
#define J7OSPI_INDIRECTWR_START_MASK    (0x1U << 0)
#define J7OSPI_INDIRECTWR_CANCEL_MASK   (0x1U << 1)
#define J7OSPI_INDIRECTWR_DONE_MASK     (0x1U << 5)

#define J7OSPI_INDIRECTWRWATERMARK      0x74
#define J7OSPI_INDIRECTWRSTARTADDR      0x78
#define J7OSPI_INDIRECTWRBYTES          0x7C

#define J7OSPI_CMDCTRL                  0x90
#define J7OSPI_CMDCTRL_EXECUTE_MASK     (0x1U << 0)
#define J7OSPI_CMDCTRL_INPROGRESS_MASK  (0x1U << 1)
#define J7OSPI_CMDCTRL_DUMMY_LSB        7
#define J7OSPI_CMDCTRL_DUMMY_MASK       0x1F
#define J7OSPI_CMDCTRL_WR_BYTES_LSB     12
#define J7OSPI_CMDCTRL_WR_EN_LSB        15
#define J7OSPI_CMDCTRL_ADD_BYTES_LSB    16
#define J7OSPI_CMDCTRL_ADDR_EN_LSB      19
#define J7OSPI_CMDCTRL_RD_BYTES_LSB     20
#define J7OSPI_CMDCTRL_RD_EN_LSB        23
#define J7OSPI_CMDCTRL_OPCODE_LSB       24
#define J7OSPI_CMDCTRL_WR_BYTES_MASK    0x7
#define J7OSPI_CMDCTRL_ADD_BYTES_MASK   0x3
#define J7OSPI_CMDCTRL_RD_BYTES_MASK    0x7

#define J7OSPI_CMDADDRESS               0x94
#define J7OSPI_CMDREADDATALOWER         0xA0
#define J7OSPI_CMDREADDATAUPPER         0xA4
#define J7OSPI_CMDWRITEDATALOWER        0xA8
#define J7OSPI_CMDWRITEDATAUPPER        0xAC

#define J7OSPI_POLLING_FLASH_STATUS      0xB0
#define J7OSPI_DEV_STATUS_NB_DUMMY_MASK  (0xFU << 16)
#define J7OSPI_DEV_STATUS_NB_DUMMY_OCTAL (8 << 16)

#define J7OSPI_PHY_CONFIG               0xB4
#define J7OSPI_PHY_CONFIG_DLL_RESET     (0x1U << 30)
#define J7OSPI_PHY_CONFIG_DLL_RESYNC    (0x1U << 31)
#define J7OSPI_PHY_CONFIG_RX_DLL_MASK   (0x7FU)
#define J7OSPI_PHY_CONFIG_TX_DLL_SHIFT  16
#define J7OSPI_PHY_CONFIG_TX_DLL_MASK   (0x7FU << J7OSPI_PHY_CONFIG_TX_DLL_SHIFT)

#define J7OSPI_PHY_MASTER_CONTROL       0xB8
#define J7OSPI_PHY_MASTER_BYPASS_MODE   (0x1U << 23)
#define J7OSPI_PHY_MASTER_CONTROL_INIT_DELAY_MASK   (0x7FU)

#define J7OSPI_PHY_MASTER_DFLT_INIT_DELAY   4

#define J7OSPI_DLL_OBS_LOWER            0xBC
#define J7OSPI_DLL_OBS_LOWER_LOCK       (0x1U << 15)

#define J7OSPI_OPCODE_EXT_LOWER_REG     0xE0
#define J7OSPI_OPCODE_EXT_LOWER_STIG_LSB 0
#define J7OSPI_OPCODE_EXT_LOWER_POLL_LSB 8
#define J7OSPI_OPCODE_EXT_LOWER_WR_LSB   16
#define J7OSPI_OPCODE_EXT_LOWER_RD_LSB   24

#define J7OSPI_OPCODE_EXT_UPPER_REG     0xE4
#define J7OSPI_OPCODE_EXT_UPPER_EXT_WEL_LSB 16
#define J7OSPI_OPCODE_EXT_UPPER_WEL_LSB  24

/* Interrupt status bits */
#define J7OSPI_IRQ_MODE_ERR             (0x1U << 0)
#define J7OSPI_IRQ_UNDERFLOW            (0x1U << 1)
#define J7OSPI_IRQ_IND_COMP             (0x1U << 2)
#define J7OSPI_IRQ_IND_RD_REJECT        (0x1U << 3)
#define J7OSPI_IRQ_WR_PROTECTED_ERR     (0x1U << 4)
#define J7OSPI_IRQ_ILLEGAL_AHB_ERR      (0x1U << 5)
#define J7OSPI_IRQ_WATERMARK            (0x1U << 6)
#define J7OSPI_IRQ_IND_SRAM_FULL        (0x1U << 12)

#define J7OSPI_IRQ_MASK_RD              (J7OSPI_IRQ_WATERMARK | \
                                         J7OSPI_IRQ_IND_SRAM_FULL | \
                                         J7OSPI_IRQ_IND_COMP)

#define J7OSPI_IRQ_MASK_WR              (J7OSPI_IRQ_IND_COMP     | \
                                         J7OSPI_IRQ_WATERMARK    | \
                                         J7OSPI_IRQ_UNDERFLOW)

#define J7OSPI_IRQ_STATUS_MASK          0x1FFFF

#define J7OSPI_MAX_RX_DLL_DELAY         0x7F

#define J7OSPI_TIMEOUT_NS               500000000

#define J7OSPI_DFLT_MEMSIZE             (64 * 1024 * 1024)  // default 64MB

#define J7OSPI_PHY_INIT_RD              1
#define J7OSPI_PHY_MAX_RD               4
#define J7OSPI_PHY_MAX_RX               63
#define J7OSPI_PHY_MAX_TX               63
#define J7OSPI_PHY_LOW_RX_BOUND         15
#define J7OSPI_PHY_HIGH_RX_BOUND        25
#define J7OSPI_PHY_LOW_TX_BOUND         32
#define J7OSPI_PHY_HIGH_TX_BOUND        48
#define J7OSPI_PHY_TX_LOOKUP_LOW_BOUND  24
#define J7OSPI_PHY_TX_LOOKUP_HIGH_BOUND 38

#define J7OSPI_PHY_DEFAULT_TEMP         45
#define J7OSPI_PHY_MIN_TEMP             (-45)
#define J7OSPI_PHY_MAX_TEMP             130
#define J7OSPI_PHY_MID_TEMP             (J7OSPI_PHY_MIN_TEMP + \
                                        ((J7OSPI_PHY_MAX_TEMP - J7OSPI_PHY_MIN_TEMP) / 2))

#define J7OSPI_PHY_TX_START             16
#define J7OSPI_PHY_TX_END               48

#define J7OSPI_TUNING_PATTERN_SIZE      128
#define J7OSPI_TUNING_PATTERN_OFFSET    0x3FE0000

typedef struct _j7_phy_setting {
    uint8_t     rx;
    uint8_t     tx;
    uint8_t     read_delay;
} j7_phy_setting;

typedef struct _ospi_dev_t
{
    snor_ctrl_t ctrl;

    paddr_t     ctrl_pbase;
    uintptr_t   ctrl_vbase;
    paddr_t     mem_pbase;
    uintptr_t   mem_vbase;
    uint32_t    mem_size;
    uint32_t    refclk;
    uint32_t    busclk;
    uint32_t    tshsl_ns;
    uint32_t    tchsh_ns;
    uint32_t    tslch_ns;
    uint32_t    tsd2d_ns;
    uint32_t    fifo_depth;
    int         fifo_width;
    uint32_t    read_delay;
    uint8_t     dqs_en;
    uint8_t     rclk_en;
    uint8_t     is_decoded_cs;
    uint8_t     use_phy;
    uint32_t    pattern_offset;
    uint8_t     current_cs;
    snor_cfg_t  buscfg;

    uint8_t     inst_width;
    uint8_t     addr_width;
    uint8_t     data_width;
    uint8_t     dma_enable;
    uint8_t     dtr_enable;
    uint8_t     phy_enable;

    uint32_t    proto;

    uint32_t    chip_size;      /* size of the flash chip */
    uint16_t    page_size;      /* program page size */
    uint32_t    sector_size;    /* erase sector size */

    int         ch;             /* DMA channel select */
    int         tpmfd;          /* typed memory fd */
    void        *v_buf;         /* virtual address of DMA buffer */
    off64_t     p_buf;          /* physical address of DMA buffer */
    size_t      buflen;
} j7ospi_dev_t;

#ifdef  J7OSPI_UDMA_SUPPORT
int j7ospi_init_udma(j7ospi_dev_t *ospi);
int j7ospi_dinit_udma(j7ospi_dev_t *ospi);
int j7ospi_udma_xfer(j7ospi_dev_t *ospi, paddr_t src, paddr_t dst, size_t len);
#endif

extern int32_t f3s_j7ospi_open(f3s_socket_t *socket, const uint32_t flags);

#endif /* J7OSPI_H_ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
