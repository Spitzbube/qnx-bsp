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

#include <internal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <hw/inout.h>
#include <hw/i2c.h>
#include <unistd.h>

#include <fcntl.h>

#include "sdhci.h"
#include "bs.h"

/**
 * Board specific interface
 *
 * @file       bs.c
 * @addtogroup sdmmc_bs
 * @{
 */

#define AM65x_SDMMC_SS_OFF      0x10000     // SDMMC subsystem registers offset
#define AM65x_SDMMC_SS_LEN      0x138       // SDMMC subsystem registers length

#define SDMMC_SS_CTL_CFG2       0x14        // SDMMC subsystem control config 2 register

#define SLOTTYPE_MASK           (3 << 30)
#define SLOTTYPE_EMBEDDED       (1 << 30)

#define SDMMC_SS_ID_REV         0x0
#define SDMMC_SS_PHY_CTRL1      0x100
#define SDMMC_SS_PHY_CTRL2      0x104
#define SDMMC_SS_PHY_CTRL3      0x108
#define SDMMC_SS_PHY_CTRL4      0x10C
#define SDMMC_SS_PHY_CTRL5      0x110
#define SDMMC_SS_PHY_CTRL6      0x114
#define SDMMC_SS_PHY_STAT1      0x130
#define SDMMC_SS_PHY_STAT2      0x134

#define IOMUX_ENABLE_SHIFT      31
#define IOMUX_ENABLE_MASK       (1 << IOMUX_ENABLE_SHIFT)
#define OTAPDLYENA_SHIFT        20
#define OTAPDLYENA_MASK         (1 << OTAPDLYENA_SHIFT)
#define OTAPDLYSEL_SHIFT        12
#define OTAPDLYSEL_MASK         (0x0F << OTAPDLYSEL_SHIFT)
#define ITAPDLYENA_SHIFT        8
#define ITAPDLYENA_MASK         (1 << ITAPDLYENA_SHIFT)
#define ITAPDLYSEL_SHIFT        0
#define ITAPDLYSEL_MASK         (0x1F << ITAPDLYSEL_SHIFT)
#define STRBSEL_SHIFT           24
#define STRBSEL_MASK            (0x0F << STRBSEL_SHIFT)
#define SEL50_SHIFT             8
#define SEL50_MASK              (1 << SEL50_SHIFT)
#define SEL100_SHIFT            9
#define SEL100_MASK             (1 << SEL100_SHIFT)
#define DLL_TRIM_ICP_SHIFT      4
#define DLL_TRIM_ICP_MASK       (0x0F << DLL_TRIM_ICP_SHIFT)
#define DR_TY_SHIFT             20
#define DR_TY_MASK              (0x07 << DR_TY_SHIFT)
#define ENDLL_SHIFT             1
#define ENDLL_MASK              (1 << ENDLL_SHIFT)
#define DLLRDY_SHIFT            0
#define DLLRDY_MASK             (1 << DLLRDY_SHIFT)
#define SS_ID_REV_MOD_ID_SHIFT  16

#define SDMMC_SS_ID_REV_PHY_HARD 0x6841
#define SDMMC_SS_ID_REV_PHY_SOFT 0x6842

#define DRIVER_STRENGTH_50_OHM  0x0
#define DRIVER_STRENGTH_33_OHM  0x1
#define DRIVER_STRENGTH_66_OHM  0x2
#define DRIVER_STRENGTH_100_OHM 0x3
#define DRIVER_STRENGTH_40_OHM  0x4

#define DLLRDY_TIMEOUT          1000

#define J721E_GPIO_SIZE         4096
#define J721E_GPIO_DIR(x)       ((uint32_t)(0x10 + ((x) / 32) * 0x28))
#define J721E_GPIO_SET_DATA(x)  ((uint32_t)(0x18 + ((x) / 32) * 0x28))
#define J721E_GPIO_CLR_DATA(x)  ((uint32_t)(0x1C + ((x) / 32) * 0x28))
#define J721E_GPIO_BIT(x)       ((uint32_t)(1 << ((x) % 32)))

static void am65x_phy_power_off(sdio_hc_t *hc);

/**
 * De-initialization of host controller (board specific part)
 * @param hc Host controller
 *
 * @return EOK always
 */
static int am65x_dinit(sdio_hc_t *const hc)
{
    am65x_ext_t *const ext = hc->bs_hdl;
    int         status;

    status = sdhci_dinit(hc);

    am65x_phy_power_off(hc);

    if (ext->pwr_fd != -1) {
        close(ext->pwr_fd);
    }

    if (ext->ldo_pin != -1) {
        munmap_device_io(ext->ldo_vbase, J721E_GPIO_SIZE);
    }

    munmap_device_io(ext->ss_base, AM65x_SDMMC_SS_LEN);

    free(ext);

    return (status);
}

static void am65x_phy_init(sdio_hc_t *const hc)
{
    am65x_ext_t *const ext = hc->bs_hdl;
    const uintptr_t   base = ext->ss_base;

    // Reset registers to default values
    if (hc->caps & HC_CAP_SLOT_TYPE_EMBEDDED) {
        sdhci_out32(base + SDMMC_SS_PHY_CTRL1, 0x10000);
    }
    sdhci_out32(base + SDMMC_SS_PHY_CTRL4, 0x0);
    sdhci_out32(base + SDMMC_SS_PHY_CTRL5, 0x0);

    // Enable pins by setting the IO mux to 0
    sdhci_out32(base + SDMMC_SS_PHY_CTRL1,
        sdhci_in32(base + SDMMC_SS_PHY_CTRL1) & ~IOMUX_ENABLE_MASK);

    ext->phy_is_on = 0;
}

static void am65x_phy_power_off(sdio_hc_t *const hc)
{
    am65x_ext_t *const ext = hc->bs_hdl;
    const uintptr_t   base = ext->ss_base;

    if (ext->phy_is_on) {

        // Disable DLL
        sdhci_out32(base + SDMMC_SS_PHY_CTRL1,
            sdhci_in32(base + SDMMC_SS_PHY_CTRL1) & ~ENDLL_MASK);

        // Reset registers to default value
        sdhci_out32(base + SDMMC_SS_PHY_CTRL1, 0x10000);
        sdhci_out32(base + SDMMC_SS_PHY_CTRL4, 0x0);
        sdhci_out32(base + SDMMC_SS_PHY_CTRL5, 0x0);

        ext->phy_is_on = 0;
    }
}

static void am65x_tap_delay(const sdio_hc_t *const hc)
{
    const am65x_ext_t *const ext = hc->bs_hdl;
    const uintptr_t   base = ext->ss_base;
    uint32_t    mask, val;

    mask = OTAPDLYENA_MASK | OTAPDLYSEL_MASK;
    val  = (uint32_t)((1 << OTAPDLYENA_SHIFT) | (ext->otap_del_sel << OTAPDLYSEL_SHIFT));
    sdhci_out32(base + SDMMC_SS_PHY_CTRL4,
        (sdhci_in32(base + SDMMC_SS_PHY_CTRL4) & ~mask) | val);
}

static int am65x_phy_power_on(sdio_hc_t *const hc)
{
    am65x_ext_t *const ext = hc->bs_hdl;
    const uintptr_t   base = ext->ss_base;
    uint32_t    mask, val;
    int         sel50, sel100;

    // Setup DLL Output TAP delay
    am65x_tap_delay(hc);

    // Select proper PHY clock base on operation clock
    if (hc->clk > 100000000) {
        sel50 = 0;
        sel100 = 0;
    } else if (hc->clk > 52000000) {
        sel50 = 0;
        sel100 = 1;
    } else {
        sel50 = 1;
        sel100 = 0;
    }

    // Configure PHY DLL frequency
    mask = SEL50_MASK | SEL100_MASK;
    val = (uint32_t)((sel50 << SEL50_SHIFT) | (sel100 << SEL100_SHIFT));
    sdhci_out32(base + SDMMC_SS_PHY_CTRL5,
        (sdhci_in32(base + SDMMC_SS_PHY_CTRL5) & ~mask) | val);

    // Configure DLL TRIM
    mask = DLL_TRIM_ICP_MASK;
    val = (uint32_t)(ext->trm_icp << DLL_TRIM_ICP_SHIFT);

    // Configure DLL driver strength
    mask |= DR_TY_MASK;
    val |= ext->drv_strength << DR_TY_SHIFT;
    sdhci_out32(base + SDMMC_SS_PHY_CTRL1,
        (sdhci_in32(base + SDMMC_SS_PHY_CTRL1) & ~mask) | val);

    // Enable DLL
    sdhci_out32(base + SDMMC_SS_PHY_CTRL1,
        (sdhci_in32(base + SDMMC_SS_PHY_CTRL1) & ~ENDLL_MASK) | (1 << ENDLL_SHIFT));

    // Poll for DLL ready. Use a one second timeout.
    for (val = 0; val < DLLRDY_TIMEOUT; val++) {
        if (sdhci_in32(base + SDMMC_SS_PHY_STAT1) & DLLRDY_MASK) {
            ext->phy_is_on = 1;
            return (EOK);
        }
        delay(1);
    }

    return (ETIMEDOUT);
}

static int am65x_set_ios_post(sdio_hc_t *const hc)
{
    const am65x_ext_t *const ext = hc->bs_hdl;
    const sdhci_hc_t  *const sdhc = hc->cs_hdl;
    const uintptr_t   base = sdhc->base;
    int         status = EOK;

    // Stop clock
    sdhci_out32(base + SDHCI_SYSCTL,
        sdhci_in32(base + SDHCI_SYSCTL) & ~SDHCI_SYSCTL_CEN);

    // Power off phy
    am65x_phy_power_off(hc);
    // Restart clock
    ext->clk(hc, hc->clk);
    // Switch phy back on for high speed operation
    if (hc->clk > 26000000) {
        status = am65x_phy_power_on(hc);
    }

    return (status);
}

static int am65x_clk(sdio_hc_t *const hc, const int clk)
{
    hc->clk = (uint32_t)clk;
    return am65x_set_ios_post(hc);
}

static int am65x_cd(sdio_hc_t *const hc)
{
    am65x_ext_t *const ext = hc->bs_hdl;

    // Write protect pin is not connected, so no write protect detection
    return (ext->cd(hc) & ~CD_WP);
}

static int am65x_signal_voltage(sdio_hc_t *const hc, const int voltage)
{
    const am65x_ext_t *const ext = hc->bs_hdl;
    int         status = EOK;

    if (hc->signal_voltage == (uint32_t)voltage) {
        return (EOK);
    }

    if (ext->signal_voltage(hc, voltage) == EOK) {
        status = am65x_set_ios_post(hc);
    }

    return (status);
}

static int j721evm_sd_signal_voltage(sdio_hc_t *const hc, const int voltage)
{
    am65x_ext_t *const ext = hc->bs_hdl;

    if (hc->signal_voltage == (uint32_t)voltage) {
        return (EOK);
    }

    switch (voltage) {
        case SIGNAL_VOLTAGE_1_8:
            out32(ext->ldo_vbase + J721E_GPIO_CLR_DATA(ext->ldo_pin), J721E_GPIO_BIT(ext->ldo_pin));
            break;
        case SIGNAL_VOLTAGE_3_3:
            out32(ext->ldo_vbase + J721E_GPIO_SET_DATA(ext->ldo_pin), J721E_GPIO_BIT(ext->ldo_pin));
            break;
        default:
            return (EINVAL);
    }

    return ext->signal_voltage(hc, voltage);
}

// TCA6424 register read modified write
static int am65x_tca6424_regaccess(sdio_hc_t *const hc, const uint8_t cmd, const uint8_t bit, const uint32_t val)
{
    am65x_ext_t *const ext = hc->bs_hdl;
    iov_t       siov[3];
    iov_t       riov[2];
    i2c_regacc_t regacc = {.slave.addr = ext->pwr_addr, .slave.fmt = I2C_ADDRFMT_7BIT,
                           .addr = cmd,
                           .addrlen = sizeof(cmd),
                           .nregs = 1,
                           .nregops = 1,
                           .flags = REGACC_RD_STOP
                          };
    unsigned char recvbuf;
    i2c_regops_t regops = {.clr = 0x00, .set = 0x00, .toggle = 0x00};
    val ? (regops.set = (uint8_t)(1U << bit)) : (regops.clr = (uint8_t)(1U << bit));

    SETIOV(&siov[0], &regacc, sizeof(regacc));
    SETIOV(&siov[1], &recvbuf, sizeof(recvbuf));
    SETIOV(&siov[2], &regops, sizeof(regops));

    SETIOV(&riov[0], &regacc, sizeof(regacc));
    SETIOV(&riov[1], &recvbuf, sizeof(recvbuf));

    return (devctlv(ext->pwr_fd, DCMD_I2C_REGACC, 3, 2, siov, riov, NULL));
}

static int am65x_pwr(sdio_hc_t *const hc, const int pwr)
{
    am65x_ext_t *const ext = hc->bs_hdl;
    int         status;

    // Port config, output
    status = am65x_tca6424_regaccess(hc, (uint8_t)(0x0CU + ext->pwr_port), ext->pwr_pin, 0U);
    if (status != EOK) {
        return (status);
    }

    // output data
    status = am65x_tca6424_regaccess(hc, (uint8_t)(0x04U + ext->pwr_port), ext->pwr_pin, pwr ? 1U : 0U);
    if (status != EOK) {
        return (status);
    }

#if 0
    // Port config, flip back to input
    if ((status = am65x_tca6424_regaccess(hc, 0x0C + ext->pwr_port, ext->pwr_pin, 1)) != EOK) {
        return (status);
    }
#endif

    return ext->pwr(hc, pwr);
}

static int am65x_args(sdio_hc_t *const hc, char *options)
{
    am65x_ext_t *const ext = hc->bs_hdl;
    int         opt;
    int         status;
    char        *value;

    enum {
        OPTION_WITHOUT_ARGS = 0,
            OPTION_NONE = OPTION_WITHOUT_ARGS,

        OPTION_WITH_ARGS,
            OPTION_OTAP_DEL_SEL = OPTION_WITH_ARGS,
            OPTION_TRM_ICP,
            OPTION_DRV_STRENGTH,
            OPTION_SDSS_CFG,
            OPTION_PWR_DEV,
            OPTION_PWR_PORT,
            OPTION_PWR_PIN,
            OPTION_PWR_ADDR,
            OPTION_LDO_PIN,

        OPTION_VAR_ARGS,
    };

    static char *opts[] = {
        [OPTION_NONE]           = "none",

        [OPTION_OTAP_DEL_SEL]   = "otap-del-sel",
        [OPTION_TRM_ICP]        = "trm-icp",
        [OPTION_DRV_STRENGTH]   = "driver-strength-ohm",
        [OPTION_SDSS_CFG]       = "sscfg",
        [OPTION_PWR_DEV]        = "pwrdev",
        [OPTION_PWR_PORT]       = "pwrport",
        [OPTION_PWR_PIN]        = "pwrpin",
        [OPTION_PWR_ADDR]       = "pwraddr",
        [OPTION_LDO_PIN]        = "ldo",

        NULL
    };

    status            = EOK;
    ext->otap_del_sel = 2;
    ext->trm_icp      = 8;
    ext->drv_strength = DRIVER_STRENGTH_50_OHM;
    ext->ss_offset    = AM65x_SDMMC_SS_OFF;
    ext->pwr_fd       = -1;
    ext->pwr_port     = 0;      // port 0
    ext->pwr_pin      = 2;      // pin 2
    ext->pwr_addr     = 0x22;   // default TCA6424 address
    ext->phy_type     = HARD_PHY;

    while ((options != NULL) && (*options != '\0') && (status == EOK) ) {
        opt = sdio_hc_getsubopt(&options, opts, &value);
        if (opt == -1) {
            sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 0, 0, "%s: Invalid bs option '%s'", __func__, value );
            status = EINVAL;
            break;
        }

        if (opt < OPTION_VAR_ARGS) {
            if (opt < OPTION_WITH_ARGS) {
                if( value != NULL ) {
                    sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 0, 0, "%s: Unexpected argument for '%s'", __func__, opts[opt] );
                    status = EINVAL;
                    break;
                }
            }
            else {
                if ((value == NULL) || (*value == '\0')) {
                    sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 0, 0, "%s: Missing argument for '%s'", __func__, opts[opt] );
                    status = EINVAL;
                    break;
                }
            }
        }

        switch (opt) {
            case OPTION_OTAP_DEL_SEL:
                ext->otap_del_sel = (int)strtoul(value, NULL, 0);
                break;

            case OPTION_TRM_ICP:
                ext->trm_icp = (int)strtoul(value, NULL, 0);
                break;

            case OPTION_DRV_STRENGTH:
                switch (strtoul(value, NULL, 0)) {
                    case 50:
                        ext->drv_strength = DRIVER_STRENGTH_50_OHM;
                        break;
                    case 33:
                        ext->drv_strength = DRIVER_STRENGTH_33_OHM;
                        break;
                    case 66:
                        ext->drv_strength = DRIVER_STRENGTH_66_OHM;
                        break;
                    case 100:
                        ext->drv_strength = DRIVER_STRENGTH_100_OHM;
                        break;
                    case 40:
                        ext->drv_strength = DRIVER_STRENGTH_40_OHM;
                        break;
                    default:
                        sdio_slogf(_SLOGC_SDIODI, _SLOG_ERROR,
                                        hc->cfg.verbosity, 0, "%s: Invalid driver strength option %s", __func__, value);
                        status = EINVAL;
                        break;
                }
                break;

            case OPTION_SDSS_CFG:
                ext->ss_offset = (uint32_t)strtoul(value, NULL, 0);
                break;

            case OPTION_PWR_DEV:
                ext->pwr_fd = open(value, O_RDWR);
                if (ext->pwr_fd == -1) {
                    sdio_slogf(_SLOGC_SDIODI, _SLOG_ERROR,
                            hc->cfg.verbosity, 0, "%s: unable to open %s", __func__, value);
                    status = EINVAL;
                    break;
                }
                break;

            case OPTION_PWR_PORT:
                ext->pwr_port = (uint8_t)strtoul(value, NULL, 0);
                break;

            case OPTION_PWR_PIN:
                ext->pwr_pin = (uint8_t)strtoul(value, NULL, 0);
                break;

            case OPTION_PWR_ADDR:
                ext->pwr_addr = (uint8_t)strtoul(value, NULL, 0);
                break;

            case OPTION_LDO_PIN:
                ext->ldo_pbase = (paddr_t)strtoul(value, &value, 0);
                if (*value == '^') {
                    ext->ldo_pin  = (int)strtol(value + 1, NULL, 0);
                }
                break;

            default:
                break;
        }
    }

    return (status);
}

/**
 * Initialization of host controller (board specific part)
 * @param hc Host controller handle
 *
 * @return Execution status
 */
static int am65x_init(sdio_hc_t *const hc)
{
    am65x_ext_t     *ext = NULL;
    sdio_hc_cfg_t   *const cfg = &hc->cfg;
    int             status;
    uint32_t        ctrlcfg2, ctrlcfg_ssid;
    sdhci_hc_t const *sdhc;
    uintptr_t base;

    ext = calloc(1, sizeof(am65x_ext_t));
    if (ext == NULL) {
        return ENOMEM;
    }

    hc->bs_hdl = ext;

    ext->ldo_pin = -1;

    // Parse board specific options
    status = am65x_args(hc, cfg->options);
    if( status != EOK ) {
        free(ext);
        return status;
    }

    ext->ss_base = mmap_device_io(AM65x_SDMMC_SS_LEN, cfg->base_addr[0] + ext->ss_offset);
    if (ext->ss_base == MAP_DEVICE_FAILED) {
        sdio_slogf(_SLOGC_SDIODI, _SLOG_ERROR, cfg->verbosity, 0, "%s: MMCSD subsystem register mmap failed.", __func__);
        return ENOMEM;
    }
    /** SS_ID_REV_REG: MOD_ID field Indicates whether Hard or Soft Phy is used.
    * For J7, AM64x, AM62P - MOD_ID [31:16] is 6841h (hard phy)
    * For AM62x, AM62A - MOD_ID [31:16] is 6842h (soft phy) **/
    ctrlcfg_ssid = in32(ext->ss_base + SDMMC_SS_ID_REV);
    ctrlcfg_ssid = ctrlcfg_ssid >> SS_ID_REV_MOD_ID_SHIFT; // Shift to get bits 31:16

    if (ctrlcfg_ssid == SDMMC_SS_ID_REV_PHY_HARD) {
        ext->phy_type = HARD_PHY;
        sdio_slogf(_SLOGC_SDIODI, _SLOG_INFO, cfg->verbosity, 0, "%s: Phy_type: HARD_PHY", __func__);
    } else if (ctrlcfg_ssid == SDMMC_SS_ID_REV_PHY_SOFT) {
        ext->phy_type = SOFT_PHY;
        sdio_slogf(_SLOGC_SDIODI, _SLOG_INFO, cfg->verbosity, 0, "%s: Phy_type: SOFT_PHY", __func__);
    }
    else {
        sdio_slogf(_SLOGC_SDIODI, _SLOG_ERROR, cfg->verbosity, 0, "%s: Unknown Phy_type! Defaulting to HARD_PHY", __func__);
        ext->phy_type = HARD_PHY;
    }

    hc->caps |= HC_CAP_HS200 | HC_CAP_HS400;    // | HC_CAP_HS400ES;

    status = sdhci_init(hc);
    if (status != EOK) {
        munmap_device_io(ext->ss_base, AM65x_SDMMC_SS_LEN);
        free(ext);
        return status;
    }

    /* Some devices startup with the chip inserted interupt on.
     * This can cause a race condition during startup where the driver flags a media
     * change before it can service it */
    if( (hc->caps & HC_CAP_SLOT_TYPE_EMBEDDED) == 0 ) {
        sdhc    = hc->cs_hdl;
        base    = sdhc->base;
        sdhci_out32( base + SDHCI_IS, (SDHCI_INTR_CINS | SDHCI_INTR_CREM) ); // Clear Status
    }

    // Set slot type based on SD card or eMMC
    ctrlcfg2 = in32(ext->ss_base + SDMMC_SS_CTL_CFG2);
    ctrlcfg2 &= ~SLOTTYPE_MASK;
    if (hc->caps & HC_CAP_SLOT_TYPE_EMBEDDED) {
        ctrlcfg2 |= SLOTTYPE_EMBEDDED;
    }
    out32(ext->ss_base + SDMMC_SS_CTL_CFG2, ctrlcfg2);

    // Initialize PHY
    am65x_phy_init(hc);

    // Overwrite functions
    hc->entry.dinit = am65x_dinit;

    ext->cd = hc->entry.cd;
    hc->entry.cd = am65x_cd;

    if (hc->caps & HC_CAP_SLOT_TYPE_EMBEDDED) {
        ext->clk = hc->entry.clk;
        ext->signal_voltage = hc->entry.signal_voltage;
        hc->entry.clk = am65x_clk;
        hc->entry.signal_voltage = am65x_signal_voltage;
        //Call tap delay for Soft Phy devices to resolve the hs200 issue
        if (ext->phy_type == SOFT_PHY) {
           am65x_tap_delay(hc);
        }

    } else {
        if (ext->ldo_pin != -1) {
            ext->ldo_vbase = mmap_device_io(J721E_GPIO_SIZE, ext->ldo_pbase);
            if (ext->ldo_vbase == MAP_DEVICE_FAILED) {
                sdio_slogf(_SLOGC_SDIODI, _SLOG_ERROR, cfg->verbosity, 0, "%s: LDO GPIO register mmap failed.", __func__);
                return ENOMEM;
            }
            ext->signal_voltage = hc->entry.signal_voltage;
            hc->entry.signal_voltage = j721evm_sd_signal_voltage;
        }
        am65x_tap_delay(hc);
    }
    if (ext->pwr_fd != -1) {
        ext->pwr = hc->entry.pwr;
        hc->entry.pwr = am65x_pwr;
    }

    return EOK;
}

sdio_product_t sdio_fs_products[] = {
    { .did = SDIO_DEVICE_ID_WILDCARD, .class = 0, .aflags = 0, .name = "am65x", .init = am65x_init },
};

sdio_vendor_t sdio_vendors[] = {
    { .vid = SDIO_VENDOR_ID_WILDCARD, .name = "TI", .chipsets = sdio_fs_products },
    { .vid = 0, .name = NULL, .chipsets = NULL }
};

/** @} */


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/devb/sdmmc/aarch64/am65x.le/bs.c $ $Rev: 994788 $")
#endif
