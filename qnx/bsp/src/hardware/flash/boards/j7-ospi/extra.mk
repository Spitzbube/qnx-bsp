ifdef PSDK_QNX_PATH
# Error if PSDK_QNX_PATH is provided but doesn't exist
ifeq ("$(wildcard $(PSDK_QNX_PATH))","")
$(error TI PSDKQA ($(PSDK_QNX_PATH)): path not found)
endif

$(info TI PSDKQA ($(PSDK_QNX_PATH)): driver will be built with DMA support!)
LIBS += $(PSDK_QNX_PATH)/qnx/pdk_libs/pdk/aarch64/so.le/libti-pdk.so
LIBS += $(PSDK_QNX_PATH)/qnx/pdk_libs/sciclient/aarch64/so.le/libti-sciclient.so
LIBS += $(PSDK_QNX_PATH)/qnx/pdk_libs/udmalld/aarch64/so.le/libti-udmalld.so
LIBS += $(PSDK_QNX_PATH)/qnx/resmgr/udma_qnx_rsmgr/usr/aarch64/so.le/libtiudma-usr.so

else
$(warning TI PSDKQA not found: driver will be built without DMA support!)
endif
