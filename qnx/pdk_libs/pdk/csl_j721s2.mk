## J721S2

#Add extra include path for csl
EXTRA_INCVPATH += \
$(PDK_INSTALL_PATH)/ti/csl/ \
$(PDK_INSTALL_PATH)/ti/csl/soc \
$(PDK_INSTALL_PATH)/ti/csl/arch/a53 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/cpts/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/crc/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/dru/V1 \
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
$(PDK_INSTALL_PATH)/ti/csl/src/ip/sec_proxy/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/sec_proxy/V0/V0_1 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/udmap/V0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/cpsw/V5 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/cpsw/V5/V5_0 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/mdio/V5 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/serdes_cd/V2 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/sgmii/V5 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/arm_gic/V2 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/ale/V4 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/emac/V5 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/bcdma/V0/V0_2 \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/lcdma_ringacc/V0 \

#Add source path for csl
EXTRA_SRCVPATH += \
$(PDK_INSTALL_PATH)/ti/csl/arch/a53/src/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/cpts/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/crc/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/dru/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/dss/V4/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/ecc_aggr/V1/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/intaggr/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/intr_router/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/mailbox/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/proxy/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/psilcfg/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/pvu/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/rat/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/ringacc/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/sec_proxy/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/udmap/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/cpsw/V5/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/cpsw/V5/V5_0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/mdio/V5/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/serdes_cd/V2/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/sgmii/V5/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/arm_gic/V2/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/emac/V5/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/bcdma/V0/priv/ \
$(PDK_INSTALL_PATH)/ti/csl/src/ip/lcdma_ringacc/V0/priv/ \

## In $(PDK_INSTALL_PATH)/ti/csl/src/ip/cbass/V0/priv/
EXCLUDE_OBJS += csl_fw.o
## In $(PDK_INSTALL_PATH)/ti/csl/src/ip/timer/V1/priv/
EXCLUDE_OBJS += dmtimer1ms.o
## In $(PDK_INSTALL_PATH)/ti/csl/src/ip/lpddr/V2/priv/
EXCLUDE_OBJS += lpddr4_16bit.o lpddr4_16bit_ctl_regs_rw_masks.o
## In $(PDK_INSTALL_PATH)/ti/csl/src/ip/serdes_cd/V2/
EXCLUDE_OBJS += csl_wiz16m_ct2_32b_PCIe.o csl_wiz16m_ct2_32b_USB.o
