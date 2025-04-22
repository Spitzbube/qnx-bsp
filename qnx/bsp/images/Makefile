ROOT_DIR := $(notdir $(CURDIR))
ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

HOST_MKIFS := mkifs
HOST_MKI := mki
SUFFIXES := .build .ifs

# NOTE: This value must match the '[image=<start_addr>]' value in the build file.
IMAGE_LOAD_ADDR = 0x80080000

BUILD_TEMPLATE = $(CURDIR)/../../Templates_build_sdp800
INSTALL=../install
#BOARD=j722s-evm
BOARD=j722s-beagley-ai

export BSP_TARGET_EXTRA=$(CURDIR)/target-extra
# Always specify PSDK_QNX_PATH as path to PSDKQA: this backup path is for internal builds only
export PSDK_QNX_PATH?=../src/hardware/support/j7/j722s/psdkqa

.PHONY: all clean

#QNX_IFS_LIST=ifs-$(BOARD).raw ifs-$(BOARD)-ti.raw 
QNX_IFS_LIST=ifs-$(BOARD).raw

ifneq (, $(wildcard $(BOARD)-ti-sbl.build))
QNX_IFS_LIST+=ifs-$(BOARD)-ti-sbl.raw
endif

all: $(QNX_IFS_LIST)

clean:
	$(RM_HOST) *.raw *.sym

ifs-$(BOARD).raw: $(BOARD).build
	$(HOST_MKIFS) -r$(INSTALL) $(MKIFSFLAGS) $^ $@

ifs-$(BOARD)-ti.raw: $(BOARD)-ti.build
	$(HOST_MKIFS) -r$(INSTALL) $(MKIFSFLAGS) $^ $@

ifs-$(BOARD)-ti-sbl.raw: $(BOARD)-ti-sbl.build
	$(HOST_MKIFS) -r$(INSTALL) $(MKIFSFLAGS) $^ $@

ifs-$(BOARD)-graphics.raw: $(BOARD)-graphics.build
	$(HOST_MKIFS) -r$(INSTALL) $(MKIFSFLAGS) $^ $@


	$(HOST_MKIFS) -r$(INSTALL) $(MKIFSFLAGS) $^ $@


	$(HOST_MKIFS) -r$(INSTALL) $(MKIFSFLAGS) $^ $@

-include $(BUILD_TEMPLATE)/template.mk
