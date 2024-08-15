ifdef PSDK_QNX_PATH
# Error if PSDK_QNX_PATH is provided but doesn't exist
ifeq ("$(wildcard $(PSDK_QNX_PATH))","")
$(error TI PSDKQA ($(PSDK_QNX_PATH)): path not found)
endif

# FW_VER: Firmware version
# Refer to the prebuilt/custom/devf-j7-ospi/env_j722s.sh for details
FW_VER := 9
ifdef FW_VER
# FWPH_PATH: sciclient_fmwMsgParams.h path
FWPH_PATH=$(PSDK_QNX_PATH)/pdk/packages/ti/drv/sciclient/soc/V$(FW_VER)
# Error if FWPH_PATH doesn't exist
ifeq ("$(wildcard $(FWPH_PATH))","")
$(error TI PSDKQA ($(FWPH_PATH)): path not found)
endif
else
$(error FW_VER not provided!)
endif

# Error if SOC is not provided
CCFLAGS += -DSOC_J722S

$(info TI PSDKQA ($(PSDK_QNX_PATH)): driver will be built with DMA support!)
EXTRA_INCVPATH += $(PSDK_QNX_PATH)/pdk/packages
EXTRA_INCVPATH += $(PSDK_QNX_PATH)
EXTRA_INCVPATH += $(FWPH_PATH)

# UDMA support
CCFLAGS += -DJ7OSPI_UDMA_SUPPORT

# UDMA instance ID
CCFLAGS += -DJ7OSPI_UDMA_INST_ID=UDMA_INST_ID_BCDMA_0

# The device ID to query frequency
CCFLAGS += -DJ7OSPI_FREQ_DEV=TISCI_DEV_FSS0_OSPI_0

# The input clock to query frequency
CCFLAGS += -DJ7OSPI_FREQ_CLK=TISCI_DEV_FSS0_OSPI_0_OSPI_RCLK_CLK

else
$(warning TI PSDKQA not found: driver will be built without DMA support!)
endif
