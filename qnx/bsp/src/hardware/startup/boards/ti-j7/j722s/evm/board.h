/*
 * Copyright 2021,2023-2024, BlackBerry Limited.
 * Copyright 2021, Texas Instruments Incorporated.
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

/* This file can contain board specific preprocessor macros and function declarations */

#ifndef __BOARD_H
#define __BOARD_H


// J722S includes two DDR data regions: 2GB at 0x8000_0000 and 30GB at 0x8_8000_0000, J722S TRM 1.0
#define IDK_DDR0_BASE   0x80000000ul
#define IDK_DDR0_SIZE   (MEG(2048))     /* 2GB */
#define IDK_DDR1_BASE   0x880000000ul
#define IDK_DDR1_SIZE   (MEG(6*1024))   /* 6GB */

#define CTRL_MMR0_CFG0_BASE         (0x0100000UL)

#define CTRLMMR_SERDES0_LN0_CTRL        (0x4080U)

#define CTRLMMR_LOCK1_KICK0             (0x5008U)
#define CTRLMMR_LOCK1_KICK1             (0x500CU)
#define CTRLMMR_LOCK2_KICK0             (0x9008U)
#define CTRLMMR_LOCK2_KICK1             (0x900CU)

// Kick unlock vals found in J722S TRM 1.0
#define CTRLMMR_KICK0_UNLOCK_VAL        (0x83E70B13U)
#define CTRLMMR_KICK1_UNLOCK_VAL        (0x95A4F1E0U)
#define CTRLMMR_KICK0_LOCK_VAL          (0x0U)
#define CTRLMMR_KICK1_LOCK_VAL          (0x0U)
#define CTRLMMR_WAS_UNLOCKED            (0x0U)
#define CTRLMMR_WAS_LOCKED              (0x1U)

// From J722S TRM 1.0
#define J722S_CTRLMMR_PADCONFIG         (0x000f4000UL)
#define J722S_WKUP_CTRLMMR_PADCONFIG    (0x0004080000L)

#define J722S_GPIO0_BASE             (0x0000600000UL)
#define J722S_GPIO1_BASE             (0x0000601000UL)
#define J722S_MCU_GPIO0_BASE         (0x0004201000UL)

#define J722S_GPIO_DIR(x)              (0x10 + ((x) / 32) * 0x28)
#define J722S_GPIO_SET_DATA(x)         (0x18 + ((x) / 32) * 0x28)
#define J722S_GPIO_CLR_DATA(x)         (0x1C + ((x) / 32) * 0x28)
#define J722S_GPIO_IN_DATA(x)          (0x20 + ((x) / 32) * 0x28)
#define J722S_GPIO_BIT(x)              (1 << ((x) % 32))

/* WKUP_MMR0_USB0_PHY_CTRL Register, AM62P TRM 6.1.1.3.1.960&961, MMR0_BASE from J722S TRM 1.0 */
#define WKUP_CTRL_MMR0_BASE         (0x43000000UL)

/* USB PHY Control Registers, set Voltage and Pll*/
#define CTRLMMR_USB0_PHY_CTRL   (WKUP_CTRL_MMR0_BASE + 0x4008)
#define CTRLMMR_USB1_PHY_CTRL   (WKUP_CTRL_MMR0_BASE + 0x4018)
#define CORE_VOLTAGE            (1 << 31)  //BIT(31), Core Voltage select: 0 - 0.85v, 1 - 0.75/0.80v

#define ARRAY_SIZE(x)       (sizeof(x) / sizeof((x)[0]))
//Referenced to Table 6-1923&6-1925 in TRM with Field USBx_PHY_CTRL_PLL_REF_SEL / USBx_PHY_CTRL[3:0]
#define USBSS_CLKCTL_MASK   0x0F  //BIT(3,0)

#define PULLUDEN_SHIFT      (16)
#define PULLTYPESEL_SHIFT   (17)
#define RXACTIVE_SHIFT      (18)

#define PULL_DISABLE        (1 << PULLUDEN_SHIFT)
#define PULL_ENABLE         (0 << PULLUDEN_SHIFT)

#define PULL_UP             (1 << PULLTYPESEL_SHIFT | PULL_ENABLE)
#define PULL_DOWN           (0 << PULLTYPESEL_SHIFT | PULL_ENABLE)

#define INPUT_EN            (1 << RXACTIVE_SHIFT)
#define INPUT_DISABLE       (0 << RXACTIVE_SHIFT)

/* Only these macros are expected be used directly in device tree files */
#define PIN_OUTPUT          (INPUT_DISABLE | PULL_DISABLE)
#define PIN_OUTPUT_PULLUP   (INPUT_DISABLE | PULL_UP)
#define PIN_OUTPUT_PULLDOWN (INPUT_DISABLE | PULL_DOWN)
#define PIN_INPUT           (INPUT_EN | PULL_DISABLE)
#define PIN_INPUT_PULLUP    (INPUT_EN | PULL_UP)


extern struct callout_rtn   reboot_ti_sci;
extern int r5_display;

void hw_init(void);
void hwi_j722s(void);

#endif    /* __BOARD_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/startup/boards/ti-j7/j722s/evm/board.h $ $Rev: 994909 $")
#endif
