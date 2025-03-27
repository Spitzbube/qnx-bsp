ROOT_DIR := $(notdir $(CURDIR))
ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

HOST_MKIFS := mkifs

SUFFIXES := .build .ifs .raw

.PHONY: all clean

all: ifs-mb86h60.bin 

clean: 
	$(RM_HOST) ifs-mb86h60.bin 

ifs-mb86h60.bin: mb86h60.build
	$(HOST_MKIFS) -r../install -v $(MKIFSFLAGS) $^ $@

