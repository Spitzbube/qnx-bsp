/*
 * $QNXLicenseC:
 * Copyright 2019,2023-2024 BlackBerry Limited.
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

#ifndef BS_H_
#define BS_H_

/**
 * Board specific interface
 *
 * @file       bs.h
 * @addtogroup sdmmc_bs
 * @{
 */

#include <sys/utsname.h>

// Add new chipset externs here
#define SDIO_HC_SDHCI

#define SDIO_SOC_SUPPORT


struct _sdio_hc;

typedef enum _phy_types {
	HARD_PHY,
	SOFT_PHY
}phy_types;

/** Structure describing board specific configuration */
typedef struct _am65x_ext {
    uintptr_t       ss_base;        /* MMCSD subsystem registers base address */
    uint32_t        ss_offset;
    int             phy_is_on;
    int             otap_del_sel;
    int             trm_icp;
    int             drv_strength;
    int             pwr_fd;
    uint8_t         pwr_port;
    uint8_t         pwr_pin;
    uint8_t         pwr_addr;
    paddr_t         ldo_pbase;
    uintptr_t       ldo_vbase;
    int             ldo_pin;
    int             (*pwr)(struct _sdio_hc *, int);
    int             (*clk)(struct _sdio_hc *, int);
    int             (*cd)(struct _sdio_hc *);
    int             (*signal_voltage)(struct _sdio_hc *, int);
    phy_types       phy_type;
} am65x_ext_t;

/** @} */ /* End of sdmmc_bs */

#endif /* BS_H_ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/devb/sdmmc/aarch64/am65x.le/bs.h $ $Rev: 994453 $")
#endif
