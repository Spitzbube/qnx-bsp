## J784S4

#Add extra include path for csl
EXTRA_INCVPATH += \
$(PDK_INSTALL_PATH)/ti/csl/ \
$(PDK_INSTALL_PATH)/ti/csl/soc \
$(PDK_INSTALL_PATH)/ti/csl/arch/a53 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/cbass/V0/V0_1 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/cpts/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/clec/V1 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/csirx/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/csirx/V0/priv \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/csitx/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/csitx/V0/priv \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/crc/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/dcc/V1 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/lpddr/V2 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/lpddr/V2/priv \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/lpddr/V2/V2_1 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/dru/V1 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/dss/V4 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/dmpac/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/ecc_aggr/V1 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/elm/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/emif/V5 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/esm/V1/V1_1 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/fss/V0/V0_1 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/gpio/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/gpmc/V1 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/hyperbus/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/i2c/V2 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/i3c/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/intaggr/V0/V0_2 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/intr_router/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/mailbox/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/mailbox/V0/V0_1 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/proxy/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/proxy/V0/V0_0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/psilcfg/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/psilss/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/pvu/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/rat/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/ringacc/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/rti/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/sec_proxy/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/sec_proxy/V0/V0_1 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/spinlock/V1 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/timer/V1 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/timer_mgr/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/uart/V1 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/usb/V5/V5_2 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/udmap/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/wd_timer/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/tsc/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/chip/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/cpsw/V5 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/cpsw/V5/V5_0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/mdio/V5 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/serdes_cd/V3 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/sgmii/V5 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/vpac/V3 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/vpfe/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/ecap/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/arm_gic/V2 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/vtm/V1 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/hts/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/hts/V1 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/lse/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/lse/V1 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/ctset2/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/ctset2/V1 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/ale/V4 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/emac/V5 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/tog/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/pbist/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/gtc/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/lbist/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/psc/V2 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/bcdma/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/bcdma/V0/V0_2 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/lcdma_ringacc/V0 \

#Add source path for csl
EXTRA_SRCVPATH += \
$(PDK_INSTALL_PATH)/ti/csl/arch/a53/src/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/adc/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/cbass/V0/V0_1/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/cbass/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/cpts/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/clec/V1/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/csirx/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/crc/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/dcc/V1/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/lpddr/V2/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/dru/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/dss/V4/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/ecc_aggr/V1/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/elm/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/emif/V2/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/epwm/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/epwm/V0_1/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/esm/V1/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/fss/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/gpio/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/gpmc/V1/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/hyperbus/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/i2c/V2/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/intaggr/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/intr_router/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/mailbox/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/proxy/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/psilcfg/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/pvu/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/rat/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/ringacc/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/rti/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/sec_proxy/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/spinlock/V1/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/timer/V1/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/timer_mgr/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/uart/V1/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/udmap/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/wd_timer/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/cpsw/V5/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/cpsw/V5/V5_0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/mdio/V5/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/serdes_cd/V3/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/sgmii/V5/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/ecap/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/arm_gic/V2/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/vtm/V1/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/emac/V5/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/tog/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/pbist/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/lbist/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/psc/V2/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/bcdma/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/lcdma_ringacc/V0/priv/ \

## In $(PDK_INSTALL_PATH)/ti/csl/src/ip/cbass/V0/priv/
EXCLUDE_OBJS += csl_fw.o
## In $(PDK_INSTALL_PATH)/ti/csl/src/ip/timer/V1/priv/
EXCLUDE_OBJS += dmtimer1ms.o
## In $(PDK_INSTALL_PATH)/ti/csl/src/ip/lpddr/V2/priv/
EXCLUDE_OBJS += lpddr4_16bit.o lpddr4_16bit_ctl_regs_rw_masks.o
## In $(PDK_INSTALL_PATH)/ti/csl/src/ip/serdes_cd/V3/
EXCLUDE_OBJS += csl_wiz16m_ct3_20b_QSGMII.o csl_wiz16m_ct3_20b_SGMII.o csl_wiz16m_ct3_20b_USXGMII.o csl_wiz16m_ct3_20b_XAUI.o csl_wiz16m_ct3_20b_XFI.o
