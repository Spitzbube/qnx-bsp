/*
 * Copyright 2021-2024 BlackBerry Limited.
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


/*
 * TI J722S-EVM
 */
#include <startup.h>
#include "ti_sci.h"
#include "j722s_pinmux.h"
#include <hw/omap_i2c.h>
#include "board.h"

extern int r5_display;

/* USBx_VBP2AHB_WRAP_CONTROLLER_VBP_USB3_CORE_CAP */
#define USB0_WRAPPER_BASE   0x31000000
#define USB1_WRAPPER_BASE   0x31100000

// referenced u-boot code
/* USBx_MMR_MMRVBP_USB2SS_CFG */
#define USB0_SUBSYS_BASE    0x0F900000
#define USB1_SUBSYS_BASE    0x0F910000

/* PHY Config set register offset */
#define PHY_CONFIG_OFFSET   0x08
#define USBSS0_PHY_CONFIG    (USB0_SUBSYS_BASE + PHY_CONFIG_OFFSET)
#define USBSS1_PHY_CONFIG    (USB1_SUBSYS_BASE + PHY_CONFIG_OFFSET)
/* PHY CONFIG register bits */
#define USBSS_PHY_VBUS_SEL_SHIFT    1

/* PHY Mode control register offset */
#define MODE_CONTROL_OFFSET 0x1C
#define USBSS0_MODE_CONTROL  (USB0_SUBSYS_BASE + MODE_CONTROL_OFFSET)
#define USBSS1_MODE_CONTROL  (USB1_SUBSYS_BASE + MODE_CONTROL_OFFSET)
/* MODE CONTROL register bits */
#define USBSS_MODE_VALID        0x01  //BIT(0)

static int init_dss(void)
{
    // DSS power, functional clock
    if (ti_sci_set_device_state(TISCI_DEV_DSS0, 0, MSG_DEVICE_SW_STATE_ON) == -1) {
        kprintf("%s: Set DSS device state failed!\n", __func__);
        return -1;
    }
    if (ti_sci_set_clock_state(TISCI_DEV_DSS0, TISCI_DEV_DSS0_DSS_FUNC_CLK,
            0, MSG_CLOCK_SW_STATE_REQ) == -1) {
        kprintf("%s: Set DSS clock state failed!\n", __func__);
        return -1;
    }

    return 0;
}

static int init_hdmi(void)
{
    const uint16_t hdmi_pins[] = {
        PIN_VOUT0_VSYNC,    //offset 0x0100: (AC25) VOUT0_VSYNC
        PIN_VOUT0_HSYNC,    //offset 0x00f8: (AB24) VOUT0_HSYNC
        PIN_VOUT0_PCLK,     //offset 0x0104: (AC24) VOUT0_PCLK
        PIN_VOUT0_DE,       //offset 0x00fc:  (Y20) VOUT0_DE
        PIN_VOUT0_DATA0,    //offset 0x00b8:  (U22) VOUT0_DATA0
        PIN_VOUT0_DATA1,    //offset 0x00bc:  (V24) VOUT0_DATA1
        PIN_VOUT0_DATA2,    //offset 0x00c0:  (W25) VOUT0_DATA2
        PIN_VOUT0_DATA3,    //offset 0x00c4:  (W24) VOUT0_DATA3
        PIN_VOUT0_DATA4,    //offset 0x00c8:  (Y25) VOUT0_DATA4
        PIN_VOUT0_DATA5,    //offset 0x00cc:  (Y24) VOUT0_DATA5
        PIN_VOUT0_DATA6,    //offset 0x00d0:  (Y23) VOUT0_DATA6
        PIN_VOUT0_DATA7,    //offset 0x00d4: (AA25) VOUT0_DATA7
        PIN_VOUT0_DATA8,    //offset 0x00d8:  (V21) VOUT0_DATA8
        PIN_VOUT0_DATA9,    //offset 0x00dc:  (W21) VOUT0_DATA9
        PIN_VOUT0_DATA10,   //offset 0x00e0:  (V20) VOUT0_DATA10
        PIN_VOUT0_DATA11,   //offset 0x00e4: (AA23) VOUT0_DATA11
        PIN_VOUT0_DATA12,   //offset 0x00e8: (AB25) VOUT0_DATA12
        PIN_VOUT0_DATA13,   //offset 0x00ec: (AA24) VOUT0_DATA13
        PIN_VOUT0_DATA14,   //offset 0x00f0:  (Y22) VOUT0_DATA14
        PIN_VOUT0_DATA15,   //offset 0x00f4: (AA21) VOUT0_DATA15
        PIN_GPMC0_AD8,      //offset 0x005c:  (R24) GPMC0_AD8.VOUT0_DATA16
        PIN_GPMC0_AD9,      //offset 0x0060:  (R25) GPMC0_AD9.VOUT0_DATA17
        PIN_GPMC0_AD10,     //offset 0x0064:  (T25) GPMC0_AD10.VOUT0_DATA18
        PIN_GPMC0_AD11,     //offset 0x0068:  (R21) GPMC0_AD11.VOUT0_DATA19
        PIN_GPMC0_AD12,     //offset 0x006c:  (T22) GPMC0_AD12.VOUT0_DATA20
        PIN_GPMC0_AD13,     //offset 0x0070:  (T24) GPMC0_AD13.VOUT0_DATA21
        PIN_GPMC0_AD14,     //offset 0x0074:  (U25) GPMC0_AD14.VOUT0_DATA22
        PIN_GPMC0_AD15,     //offset 0x0078:  (U24) GPMC0_AD15.VOUT0_DATA23
    };

    uint32_t    hdmi_pincfg = PIN_OUTPUT | PIN_MODE(0);
    uint8_t     pin;

    // Pinmux
    for (pin = 0; pin < sizeof(hdmi_pins) / sizeof(uint16_t); pin++) {
            if (pin > 19) {
                hdmi_pincfg = PIN_OUTPUT | PIN_MODE(1);
            }
        out32(J722S_CTRLMMR_PADCONFIG + hdmi_pins[pin], hdmi_pincfg);
    }

    // Set 148.5Mhz pixel clock for 1080P output
    if (ti_sci_cmd_clk_set_freq(TISCI_DEV_DSS0, TISCI_DEV_DSS0_DPI_1_IN_CLK,
            148500000ULL, 148500000ULL, 148500000ULL) == -1) {
        kprintf("%s: Set DSS pixel clock frequency failed!\n", __func__);
        return -1;
    }

    if (ti_sci_set_clock_state(TISCI_DEV_DSS0, TISCI_DEV_DSS0_DPI_1_IN_CLK,
            0, MSG_CLOCK_SW_STATE_REQ) == -1) {
        kprintf("%s: Set DSS clock state failed!\n", __func__);
        return -1;
    }

    return 0;
}

static int init_gpu(void)
{
    uint8_t     c_state;
    // Enable GPU power
    if (ti_sci_get_device_state(TISCI_DEV_GPU0, NULL, NULL, NULL, &c_state) == -1) {
        kprintf("%s: get_device_state failed for TISCI_DEV_GPU0!\n", __func__);
        return -1;
    }

    if (c_state == MSG_DEVICE_HW_STATE_OFF) {
        if (ti_sci_set_device_state(TISCI_DEV_GPU0, 0, MSG_DEVICE_SW_STATE_ON) == -1) {
            kprintf("%s: ti_sci_set_device_state failed for TISCI_DEV_GPU0!\n", __func__);
            return -1;
        }
    }

    if (ti_sci_get_device_state(TISCI_DEV_GPU_RS_BW_LIMITER9, NULL, NULL, NULL, &c_state) == -1) {
        kprintf("%s: get_device_state failed for TISCI_DEV_GPU_RS_BW_LIMITER9!\n", __func__);
        return -1;
    }

    if (c_state == MSG_DEVICE_HW_STATE_OFF) {
        if (ti_sci_set_device_state(TISCI_DEV_GPU_RS_BW_LIMITER9, 0, MSG_DEVICE_SW_STATE_ON) == -1) {
            kprintf("%s: ti_sci_set_device_state failed for TISCI_DEV_GPU_RS_BW_LIMITER9!\n", __func__);
            return -1;
        }
    }

    if (ti_sci_get_device_state(TISCI_DEV_GPU_WS_BW_LIMITER10, NULL, NULL, NULL, &c_state) == -1) {
        kprintf("%s: get_device_state failed TISCI_DEV_GPU_WS_BW_LIMITER10!\n", __func__);
        return -1;
    }

    if (c_state == MSG_DEVICE_HW_STATE_OFF) {
        if (ti_sci_set_device_state(TISCI_DEV_GPU_WS_BW_LIMITER10, 0, MSG_DEVICE_SW_STATE_ON) == -1) {
            kprintf("%s: ti_sci_set_device_state failed for TISCI_DEV_GPU_WS_BW_LIMITER10!\n", __func__);
            return -1;
        }
    }

    if (ti_sci_get_device_state(TISCI_DEV_GPU0_CORE_VD, NULL, NULL, NULL, &c_state) == -1) {
        kprintf("%s: get_device_state failed for TISCI_DEV_GPU0_CORE_VD\n", __func__);
        return -1;
    }

    if (c_state == MSG_DEVICE_HW_STATE_OFF) {
        if (ti_sci_set_device_state(TISCI_DEV_GPU0_CORE_VD, 0, MSG_DEVICE_SW_STATE_ON) == -1) {
            kprintf("%s: ti_sci_set_device_state failed for TISCI_DEV_GPU0_CORE_VD!\n", __func__);
            return -1;
        }
    }
    return 0;
}

static int init_usb_host_dwc3(const int host)
{

    const uint64_t dwc3_ti_rate_table[] = {       /* in KHZ */
        9600,
        10000,
        12000,
        19200,
        20000,
        24000,
        25000,
        26000,
        38400,
        40000,
        58000,
        50000,
        52000,
    };

    uint32_t    dev_id, dev_main;
    uint32_t    val, i;
    uint64_t    rate;
    paddr_t     phy_ctrl, mode_ctrl, pinmux_ctrl/*, phy_cfg*/; //'phy_cfg' is reserved for setting configs to PHY if need

    kprintf("%s: USB%d\n", __func__, host);

    // output revision information from hw
    ti_sci_init();

    if(host == 0)
    {
        dev_id = (uint32_t)TISCI_DEV_USB0;
        dev_main = (uint32_t)TISCI_DEV_MAIN_USB0_ISO_VD;
        // phy_cfg = USBSS0_PHY_CONFIG;  // RESERVED
        phy_ctrl = CTRLMMR_USB0_PHY_CTRL;
        mode_ctrl = USBSS0_MODE_CONTROL;
        pinmux_ctrl = J722S_CTRLMMR_PADCONFIG + PIN_USB0_DRVVBUS;
    }
    else // host == 1
    {
        dev_id = (uint32_t)TISCI_DEV_USB1;
        dev_main = (uint32_t)TISCI_DEV_MAIN_USB2_ISO_VD;
        // phy_cfg = USBSS1_PHY_CONFIG;  // RESERVED
        phy_ctrl = CTRLMMR_USB1_PHY_CTRL;
        mode_ctrl = USBSS1_MODE_CONTROL;
        pinmux_ctrl = J722S_CTRLMMR_PADCONFIG + PIN_USB1_DRVVBUS;
    }

    // Set pinmux
    out32(pinmux_ctrl, PIN_PULL_DISABLE | PIN_MODE(0));  // VBUS control

    /* For clock settings
     * Since clock ID sets of device USB0 and USB1 have the same definitions in tisci_clock.h
     * In order to simplify the routine, here use '_USB0' for both (alternate '_USB1')
     */
    // USB main clock
    if (ti_sci_set_device_state(dev_main, 0, MSG_DEVICE_SW_STATE_ON) == -1) {
        kprintf("%s: HOST %d, failed! %d\n", __func__, host, __LINE__);
        return (-1);
    }
    if (ti_sci_set_device_state(dev_main, TI_SCI_MSG_SET_MAIN_USB0, TISCI_DEV_USB0_BUS_CLK) == -1) {
        kprintf("%s: HOST %d, failed! %d\n", __func__, host, __LINE__);
        return (-1);
    }

    // USB sub-system clock, parent clock
    if (ti_sci_set_device_state(dev_id, 0, MSG_DEVICE_SW_STATE_ON) == -1) {
        kprintf("%s: HOST %d, failed! %d\n", __func__, host, __LINE__);
        return (-1);
    }
    if (ti_sci_cmd_set_clk_parent(dev_id, TISCI_DEV_USB0_USB2_REFCLOCK_CLK,
        TISCI_DEV_USB0_USB2_REFCLOCK_CLK_PARENT_GLUELOGIC_HFOSC0_CLKOUT) == -1) {
        kprintf("%s: HOST %d, failed! %d\n", __func__, host, __LINE__);
        return (-1);
    }
    if (ti_sci_set_clock_state(dev_id, TISCI_DEV_USB0_USB2_APB_PCLK_CLK, 0, MSG_CLOCK_SW_STATE_AUTO) == -1) {
        kprintf("%s: HOST %d, failed! %d\n", __func__, host, __LINE__);
        return (-1);
    }
    if (ti_sci_set_clock_state(dev_id, TISCI_DEV_USB0_USB2_REFCLOCK_CLK, 0, MSG_CLOCK_SW_STATE_AUTO) == -1) {
        kprintf("%s: HOST %d, failed! %d\n", __func__, host, __LINE__);
        return (-1);
    }

    /* PHY Core voltage enable */
    /* Set USB PHY core voltage to 0.85V */
    val = in32(phy_ctrl);
    val &= ~(CORE_VOLTAGE);
    out32(phy_ctrl, val);

    /*Get USB Rate*/
    if (ti_sci_cmd_clk_get_freq(dev_id, TISCI_DEV_USB0_USB2_REFCLOCK_CLK, &rate) == -1){
        kprintf("%s: HOST %d, failed! %d\n", __func__, host, __LINE__);
        return (-1);
    }

    /* Calcuate the rate code */
    rate /= 1000;   /* To KHz */
    for (i = 0; i < ARRAY_SIZE(dwc3_ti_rate_table); i++) {
        if (dwc3_ti_rate_table[i] == rate) {
            break;
        }
    }

    if (i == ARRAY_SIZE(dwc3_ti_rate_table)) {
        //Unsupported usb2_refclk rate
        kprintf("%s: HOST %d, failed! %d\n", __func__, host, __LINE__);
        return (-1);
    }

    /*PHY clock feq*/
    val = in32(phy_ctrl);
    val &= ~(USBSS_CLKCTL_MASK);
    val |= i;
    out32(phy_ctrl, val);

    /* VBUS divider selected */ // no divider used, RESERVED for future need
    // val = in32(phy_cfg);
    // if(divider_selected)
    //     val |= 1 << USBSS_PHY_VBUS_SEL_SHIFT;
    // out32(phy_cfg, val); // write again

    /* Set mode valid */
    val = in32(mode_ctrl);
    val |= USBSS_MODE_VALID;
    out32(mode_ctrl, val);

    return (0);
}

static int init_usb_host_cdns3(void)
{
    /* set clock state on for serdes 0 */
    if (ti_sci_cmd_set_clk_parent(TISCI_DEV_SERDES_10G0, TISCI_DEV_SERDES_10G0_CORE_REF_CLK,
        TISCI_DEV_SERDES_10G0_CORE_REF_CLK_PARENT_POSTDIV4_16FF_MAIN_0_HSDIVOUT9_CLK) == -1) {
        kprintf("%s: Set SERDES10G0 Ref clock parent failed!\n", __func__);
        return -1;
    }

    if (ti_sci_set_device_state(TISCI_DEV_SERDES_10G0, 0, MSG_DEVICE_SW_STATE_ON) == -1) {
        kprintf("%s: Set SERDES10G0 device state failed!\n", __func__);
        return -1;
    }

    if (ti_sci_set_clock_state(TISCI_DEV_SERDES_10G0,
        TISCI_DEV_SERDES_10G0_CORE_REF_CLK_PARENT_POSTDIV4_16FF_MAIN_0_HSDIVOUT9_CLK, 0, MSG_CLOCK_SW_STATE_REQ) == -1) {
        kprintf("%s: ti_sci_set_clock_state failed! %d\n", __func__, __LINE__);
        return (-1);
    }

    if (ti_sci_set_clock_state(TISCI_DEV_SERDES_10G0, TISCI_DEV_SERDES_10G0_CORE_REF_CLK, 0, MSG_CLOCK_SW_STATE_REQ) == -1) {
        kprintf("%s: ti_sci_set_clock_state failed! %d\n", __func__, __LINE__);
        return (-1);
    }

    /* Power on USB module */
    if (ti_sci_set_device_state(TISCI_DEV_USB1, 0, MSG_DEVICE_SW_STATE_ON) == -1) {
        kprintf("%s: Set USB1 device state failed!\n", __func__);
        return (-1);
    }

    /* USB2 clocks */
    if (ti_sci_cmd_set_clk_parent(TISCI_DEV_USB1, TISCI_DEV_USB1_USB2_REFCLOCK_CLK,
            TISCI_DEV_USB1_USB2_REFCLOCK_CLK_PARENT_GLUELOGIC_HFOSC0_CLKOUT) == -1) {
        kprintf("%s: Set USB1_USB2 Ref clock parent failed!\n", __func__);
        return (-1);
    }
    if (ti_sci_set_clock_state(TISCI_DEV_USB1, TISCI_DEV_USB1_CLK_LPM_CLK,
            0, MSG_CLOCK_SW_STATE_REQ) == -1) {
        kprintf("%s: ti_sci_set_clock_state failed! %d\n", __func__, __LINE__);
        return (-1);
    }
    if (ti_sci_set_clock_state(TISCI_DEV_USB1, TISCI_DEV_USB1_USB2_REFCLOCK_CLK,
            0, MSG_CLOCK_SW_STATE_REQ) == -1) {
        kprintf("%s: ti_sci_set_clock_state failed! %d\n", __func__, __LINE__);
        return (-1);
    }

    /* Set pinmux for USB1_DRVVBUS */
    out32(J722S_CTRLMMR_PADCONFIG + PIN_USB1_DRVVBUS, PIN_INPUT_ENABLE | PIN_PULL_DISABLE | PIN_MODE(0));

    /* CSL_serdesIPSelect: USB3_0 */
    out32(CTRL_MMR0_CFG0_BASE + CTRLMMR_SERDES0_LN0_CTRL, 0);

    return (0);
}

static void init_i2c(void)
{
    // pinmux for I2C0
    out32(J722S_CTRLMMR_PADCONFIG + PIN_I2C0_SCL, PIN_PULL_DIRECTION | PIN_INPUT_ENABLE | PIN_MODE(0));
    out32(J722S_CTRLMMR_PADCONFIG + PIN_I2C0_SDA, PIN_PULL_DIRECTION | PIN_INPUT_ENABLE | PIN_MODE(0));

    // pinmux for I2C1
    out32(J722S_CTRLMMR_PADCONFIG + PIN_I2C1_SCL, PIN_PULL_DIRECTION | PIN_INPUT_ENABLE | PIN_MODE(0));
    out32(J722S_CTRLMMR_PADCONFIG + PIN_I2C1_SDA, PIN_PULL_DIRECTION | PIN_INPUT_ENABLE | PIN_MODE(0));

    // pinmux for I2C2
    out32(J722S_CTRLMMR_PADCONFIG +  PIN_GPMC0_CSN2, PIN_PULL_DIRECTION | PIN_INPUT_ENABLE | PIN_MODE(1));
    out32(J722S_CTRLMMR_PADCONFIG +  PIN_GPMC0_CSN3, PIN_PULL_DIRECTION | PIN_INPUT_ENABLE | PIN_MODE(1));
}

#define EVM_SDIO_1V8_EN_PIN_NUM        (70U)  /* GPIO0_70 */
static void init_sdmmc(void)
{
    uint8_t     c_state;

    // pinmux for SD(MMC1)
    out32(J722S_CTRLMMR_PADCONFIG + PIN_MMC1_DAT3, PIN_PULL_DIRECTION | PIN_INPUT_ENABLE | PIN_MODE(0));
    out32(J722S_CTRLMMR_PADCONFIG + PIN_MMC1_DAT2, PIN_PULL_DIRECTION | PIN_INPUT_ENABLE | PIN_MODE(0));
    out32(J722S_CTRLMMR_PADCONFIG + PIN_MMC1_DAT1, PIN_PULL_DIRECTION | PIN_INPUT_ENABLE | PIN_MODE(0));
    out32(J722S_CTRLMMR_PADCONFIG + PIN_MMC1_DAT0, PIN_PULL_DIRECTION | PIN_INPUT_ENABLE | PIN_MODE(0));
    out32(J722S_CTRLMMR_PADCONFIG + PIN_MMC1_CMD, PIN_PULL_DIRECTION | PIN_INPUT_ENABLE | PIN_MODE(0));
    out32(J722S_CTRLMMR_PADCONFIG + PIN_MMC1_CLK, PIN_INPUT_ENABLE | PIN_MODE(0)); // Pull Down
    // GPIO0_70 for signal voltage switch
    out32(J722S_CTRLMMR_PADCONFIG + PIN_MMC2_CMD, PIN_PULL_DISABLE | PIN_MODE(7));
    // GPIO0_70 as output
    out32(J722S_GPIO0_BASE + J722S_GPIO_DIR(EVM_SDIO_1V8_EN_PIN_NUM),
        in32(J722S_GPIO0_BASE + J722S_GPIO_DIR(EVM_SDIO_1V8_EN_PIN_NUM)) & ~(J722S_GPIO_BIT(EVM_SDIO_1V8_EN_PIN_NUM)));
    // Default signal voltage 3.3v, output high
    out32(J722S_GPIO0_BASE + J722S_GPIO_SET_DATA(EVM_SDIO_1V8_EN_PIN_NUM), (J722S_GPIO_BIT(EVM_SDIO_1V8_EN_PIN_NUM)));

    // Enable eMMC power
    if (ti_sci_get_device_state(TISCI_DEV_MMCSD0, NULL, NULL, NULL, &c_state) == -1) {
        kprintf("%s: get_device_state failed!\n", __func__);
        return;
    }

    if (c_state == MSG_DEVICE_HW_STATE_OFF) {
        kprintf("no MMCSD0 power: trying to turn it on\n");
        if (ti_sci_set_device_state(TISCI_DEV_MMCSD0, 0, MSG_DEVICE_SW_STATE_ON) == -1) {
            kprintf("%s: ti_sci_set_device_state failed!\n", __func__);
            return;
        }
    }

    // Enable micro-SD power
    if (ti_sci_get_device_state(TISCI_DEV_MMCSD1, NULL, NULL, NULL, &c_state) == -1) {
        kprintf("%s: get_device_state failed! %d\n", __func__);
        return;
    }

    if (c_state == MSG_DEVICE_HW_STATE_OFF) {
        kprintf("no MMCSD1 power: trying to turn it on\n");
        if (ti_sci_set_device_state(TISCI_DEV_MMCSD1, MSG_FLAG_DEVICE_RESET_ISO, MSG_DEVICE_SW_STATE_ON) == -1) {
            kprintf("%s: ti_sci_set_device_state failed!\n", __func__);
            return;
        }
    }
}

#define EVM_CPSW_RGMII_INT_PIN_NUM     (31U)  /* GPIO1_31 */
static void init_cpsw3g(void)
{
    uint8_t     c_state;

    /* MDIO 1 */
    out32(J722S_CTRLMMR_PADCONFIG + PIN_MDIO0_MDC, ((PIN_PULL_DISABLE) & (~PIN_PULL_DIRECTION & ~PIN_INPUT_ENABLE)) | PIN_MODE(0)); /* MDIO0_MDC */
    out32(J722S_CTRLMMR_PADCONFIG + PIN_MDIO0_MDIO, ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION)) | PIN_MODE(0)); /* MDIO0_MDIO */

    /* RGMII1 */
    out32(J722S_CTRLMMR_PADCONFIG + PIN_RGMII1_RD0, ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION)) | PIN_MODE(0)); /* RGMII1_RD0 */
    out32(J722S_CTRLMMR_PADCONFIG + PIN_RGMII1_RD1, ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION)) | PIN_MODE(0)); /* RGMII1_RD1 */
    out32(J722S_CTRLMMR_PADCONFIG + PIN_RGMII1_RD2, ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION)) | PIN_MODE(0)); /* RGMII1_RD2 */
    out32(J722S_CTRLMMR_PADCONFIG + PIN_RGMII1_RD3, ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION)) | PIN_MODE(0)); /* RGMII1_RD3 */
    out32(J722S_CTRLMMR_PADCONFIG + PIN_RGMII1_RXC, ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION)) | PIN_MODE(0)); /* RGMII1_RXC */
    out32(J722S_CTRLMMR_PADCONFIG + PIN_RGMII1_RX_CTL, ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION)) | PIN_MODE(0)); /* RGMII1_RX_CTL */
    out32(J722S_CTRLMMR_PADCONFIG + PIN_RGMII1_TD0, ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION)) | PIN_MODE(0)); /* RGMII1_TD0 */
    out32(J722S_CTRLMMR_PADCONFIG + PIN_RGMII1_TD1, ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION)) | PIN_MODE(0)); /* RGMII1_TD1 */
    out32(J722S_CTRLMMR_PADCONFIG + PIN_RGMII1_TD2, ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION)) | PIN_MODE(0)); /* RGMII1_TD2 */
    out32(J722S_CTRLMMR_PADCONFIG + PIN_RGMII1_TD3, ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION)) | PIN_MODE(0)); /* RGMII1_TD3 */
    out32(J722S_CTRLMMR_PADCONFIG + PIN_RGMII1_TXC, ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION)) | PIN_MODE(0)); /* RGMII1_TXC */
    out32(J722S_CTRLMMR_PADCONFIG + PIN_RGMII1_TX_CTL, ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION)) | PIN_MODE(0)); /* RGMII1_TX_CTL */

    /* EXTINTn */
    // GPIO1_31 as Interrupt
    out32(J722S_CTRLMMR_PADCONFIG + PIN_EXTINTN, ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION)) | PIN_MODE(0));
    // GPIO1_31 as input
    out32(J722S_GPIO1_BASE + J722S_GPIO_DIR(EVM_CPSW_RGMII_INT_PIN_NUM),
        in32(J722S_GPIO1_BASE + J722S_GPIO_DIR(EVM_CPSW_RGMII_INT_PIN_NUM)) | (J722S_GPIO_BIT(EVM_CPSW_RGMII_INT_PIN_NUM)));
    // Default signal output high
    out32(J722S_GPIO1_BASE + J722S_GPIO_SET_DATA(EVM_CPSW_RGMII_INT_PIN_NUM), (unsigned int)(J722S_GPIO_BIT(EVM_CPSW_RGMII_INT_PIN_NUM)));

    // Enable CPSW0 power
    if (ti_sci_get_device_state(TISCI_DEV_CPSW0, NULL, NULL, NULL, &c_state) == -1) {
        kprintf("%s: get_device_state failed!\n", __func__);
        return;
    }

    if (c_state == MSG_DEVICE_HW_STATE_OFF) {
        kprintf("no CPSW0 power: trying to turn it on\n");
        if (ti_sci_set_device_state(TISCI_DEV_CPSW0, 0, MSG_DEVICE_SW_STATE_ON) == -1) {
            kprintf("%s: ti_sci_set_device_state failed!\n", __func__);
            return;
        }
    }
}

static void init_ospi(void)
{
    uint8_t     c_state;
    // Enable OSPI power
    if (ti_sci_get_device_state(TISCI_DEV_FSS0_OSPI_0, NULL, NULL, NULL, &c_state) == -1) {
        kprintf("%s: get_device_state failed!\n", __func__);
        return;
    }

    if (c_state == MSG_DEVICE_HW_STATE_OFF) {
        kprintf("no OSPI0 power: trying to turn it on\n");
        if (ti_sci_set_device_state(TISCI_DEV_FSS0_OSPI_0, 0, MSG_DEVICE_SW_STATE_ON) == -1) {
            kprintf("%s: ti_sci_set_device_state failed!\n", __func__);
            return;
        }
    }

    // Set reference clock parent
    if (ti_sci_cmd_set_clk_parent(TISCI_DEV_FSS0_OSPI_0, TISCI_DEV_FSS0_OSPI_0_OSPI_RCLK_CLK,
            TISCI_DEV_FSS0_OSPI_0_OSPI_RCLK_CLK_PARENT_HSDIV4_16FFT_MAIN_0_HSDIVOUT1_CLK) == -1) {
        kprintf("%s: Set OSPI0 RCLK clock parent failed!\n", __func__);
        return;
    }

    // Set 200Mhz reference clock
    if (ti_sci_cmd_clk_set_freq(TISCI_DEV_FSS0_OSPI_0, TISCI_DEV_FSS0_OSPI_0_OSPI_RCLK_CLK,
            200000000ULL, 200000000ULL, 200000000ULL) == -1) {
        kprintf("%s: Set OSPI0 RCLK frequency failed!\n", __func__);
        return;
    }

    // Pinmux
    out32(J722S_CTRLMMR_PADCONFIG + 0x0,  PIN_MODE(0) | PIN_PULL_DISABLE); //CLK
    out32(J722S_CTRLMMR_PADCONFIG + 0x8,  PIN_INPUT_ENABLE | PIN_PULL_DISABLE | PIN_MODE(0)); //DQS
    out32(J722S_CTRLMMR_PADCONFIG + 0xc,  PIN_INPUT_ENABLE | PIN_PULL_DISABLE | PIN_MODE(0)); //D0
    out32(J722S_CTRLMMR_PADCONFIG + 0x10, PIN_INPUT_ENABLE | PIN_PULL_DISABLE | PIN_MODE(0)); //D1
    out32(J722S_CTRLMMR_PADCONFIG + 0x14, PIN_INPUT_ENABLE | PIN_PULL_DISABLE | PIN_MODE(0)); //D2
    out32(J722S_CTRLMMR_PADCONFIG + 0x18, PIN_INPUT_ENABLE | PIN_PULL_DISABLE | PIN_MODE(0)); //D3
    out32(J722S_CTRLMMR_PADCONFIG + 0x1C, PIN_INPUT_ENABLE | PIN_PULL_DISABLE | PIN_MODE(0)); //D4
    out32(J722S_CTRLMMR_PADCONFIG + 0x20, PIN_INPUT_ENABLE | PIN_PULL_DISABLE | PIN_MODE(0)); //D5
    out32(J722S_CTRLMMR_PADCONFIG + 0x24, PIN_INPUT_ENABLE | PIN_PULL_DISABLE | PIN_MODE(0)); //D6
    out32(J722S_CTRLMMR_PADCONFIG + 0x28, PIN_INPUT_ENABLE | PIN_PULL_DISABLE | PIN_MODE(0)); //D7
    out32(J722S_CTRLMMR_PADCONFIG + 0x2c, PIN_MODE(0) | PIN_PULL_DISABLE); //CSN0
}

static uint8_t ctrl_mmr_unlock(const uint64_t base, const uint32_t kick0, const uint32_t kick1)
{
    /* The partition is already unlocked */
    if((in32(base + kick0) & 0x01)) {
        return CTRLMMR_WAS_UNLOCKED;
    }

    /* Unlock the partition by writing the unlock values to the kick lock registers */
    out32(base + kick0, CTRLMMR_KICK0_UNLOCK_VAL);
    out32(base + kick1, CTRLMMR_KICK1_UNLOCK_VAL);
    return CTRLMMR_WAS_LOCKED;
}

static void ctrl_mmr_lock(const uint64_t base, const uint32_t kick0, const uint32_t kick1)
{
    out32(base + kick0, CTRLMMR_KICK0_LOCK_VAL);
    out32(base + kick1, CTRLMMR_KICK1_LOCK_VAL);
}

/*
 * hw_init()
 *    Board specific initialization
 */
void
hw_init(void)
{
    uint8_t lock1_was_locked, lock2_was_locked, wkup_lock1_was_locked;

    lock1_was_locked = ctrl_mmr_unlock(CTRL_MMR0_CFG0_BASE, CTRLMMR_LOCK1_KICK0, CTRLMMR_LOCK1_KICK1);
    lock2_was_locked = ctrl_mmr_unlock(CTRL_MMR0_CFG0_BASE, CTRLMMR_LOCK2_KICK0, CTRLMMR_LOCK2_KICK1);
    wkup_lock1_was_locked = ctrl_mmr_unlock(WKUP_CTRL_MMR0_BASE, CTRLMMR_LOCK1_KICK0, CTRLMMR_LOCK1_KICK1);

    init_i2c();
    init_sdmmc();
    init_ospi();
    if (r5_display == 0) {
        kprintf("Display set to A72 DSS\n");
        init_dss();
        init_hdmi();
    }
    else {
        kprintf("Display set to R5\n");
    }
    init_gpu();
    init_cpsw3g();

    init_usb_host_dwc3(0);
    init_usb_host_cdns3();

    if (lock1_was_locked) {
        ctrl_mmr_lock(CTRL_MMR0_CFG0_BASE, CTRLMMR_LOCK1_KICK0, CTRLMMR_LOCK1_KICK1);
    }
    if (lock2_was_locked) {
        ctrl_mmr_lock(CTRL_MMR0_CFG0_BASE, CTRLMMR_LOCK2_KICK0, CTRLMMR_LOCK2_KICK1);
    }
    if (wkup_lock1_was_locked) {
        ctrl_mmr_lock(WKUP_CTRL_MMR0_BASE, CTRLMMR_LOCK1_KICK0, CTRLMMR_LOCK1_KICK1);
    }
}
#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/startup/boards/ti-j7/j722s/evm/hw_init.c $ $Rev: 996494 $")
#endif
