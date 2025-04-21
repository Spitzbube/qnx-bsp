ifdef PSDK_QNX_PATH
# Error if PSDK_QNX_PATH is provided but doesn't exist
ifeq ("$(wildcard $(PSDK_QNX_PATH))","")
$(error TI PSDKQA ($(PSDK_QNX_PATH)): path not found)
endif

# FW_VER: Firmware version
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
ifdef SOC
SOC_UPPER = SOC_$(shell echo $(SOC) | tr a-z A-Z)
CCFLAGS += -D$(SOC_UPPER)
else
$(error SOC not provided!)
endif

$(info TI PSDKQA ($(PSDK_QNX_PATH)): driver will be built with DMA support!)
EXTRA_INCVPATH += $(PSDK_QNX_PATH)/pdk/packages
EXTRA_INCVPATH += $(PSDK_QNX_PATH)
EXTRA_INCVPATH += $(FWPH_PATH)

# UDMA support
CCFLAGS += -DJ7OSPI_UDMA_SUPPORT

# UDMA instance ID
ifdef J7OSPI_UDMA_INST_ID
CCFLAGS += -DJ7OSPI_UDMA_INST_ID=${J7OSPI_UDMA_INST_ID}
else
$(error J7OSPI_UDMA_INST_ID not provided!)
endif

# The device ID to query frequency
ifdef J7OSPI_FREQ_DEV
CCFLAGS += -DJ7OSPI_FREQ_DEV=${J7OSPI_FREQ_DEV}
else
$(error J7OSPI_FREQ_DEV not provided!)
endif

# The input clock to query frequency
ifdef J7OSPI_FREQ_CLK
CCFLAGS += -DJ7OSPI_FREQ_CLK=${J7OSPI_FREQ_CLK}
else
$(error J7OSPI_FREQ_CLK not provided!)
endif

else
$(warning TI PSDKQA not found: driver will be built without DMA support!)
endif
