ROOT_DIR := $(notdir $(CURDIR))
ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

HOST_MKIFS := mkifs

SUFFIXES := .build .ifs .raw

.PHONY: all clean

all: bsp-qemu-virt.bin 

clean: 
	$(RM_HOST) bsp-qemu-virt.bin 

bsp-qemu-virt.bin: virt.build
	$(HOST_MKIFS) -r../install -v $(MKIFSFLAGS) $^ $@

run: bsp-qemu-virt.bin
	qemu-system-arm \
	-M virt \
	-m 256 \
	-cpu cortex-a15 \
	-bios ./u-boot.bin \
	-hda fat:rw:./ \
	-vga none 
	-serial stdio
