export QNX_SDP_VERSION ?= 800

ifeq ($(QNX_SDP_VERSION),800)
    export QNX_BASE ?= $(HOME)/qnx800
    export QNX_CROSS_COMPILER_TOOL ?= aarch64-unknown-nto-qnx8.0.0-
    export QNX_TARGET ?= $(QNX_BASE)/target/qnx
else ifeq ($(QNX_SDP_VERSION),710)
    export QNX_BASE ?= $(HOME)/qnx710
    export QNX_CROSS_COMPILER_TOOL ?= aarch64-unknown-nto-qnx7.1.0-
    export QNX_TARGET ?= $(QNX_BASE)/target/qnx7
endif

export PSDK_QNX_PATH ?= $(abspath ..)
export PDK_QNX_PATH ?= $(PSDK_QNX_PATH)/pdk
export QNX_HOST ?= $(QNX_BASE)/host/linux/x86_64
export QNX_BOOT_PATH ?= $(PSDK_QNX_PATH)/bootfs
export QNX_FS_PATH ?= $(PSDK_QNX_PATH)/qnxfs
export QNX_AUX_FS_PATH ?= $(PSDK_QNX_PATH)/rootfs
export QNX_BSP_PATH ?= $(PSDK_QNX_PATH)/qnx/bsp
export MAKEFLAGS=-I$(QNX_TARGET)/usr/include
export PATH=$(QNX_HOST)/usr/bin:$(shell printenv PATH)
export QNX_ARCH=aarch64le
export PSDK_QNX_STAGE = $(PSDK_QNX_PATH)/stage
export QCONF_OVERRIDE = $(PSDK_QNX_PATH)/qnx/qconf-override.mk

##### Rules depending on PSDK RTOS #####

export PSDK_TOOLS_PATH ?= $(HOME)/ti
export PSDK_RTOS_PATH ?= $(abspath $(PSDK_QNX_PATH)/..)
export LINUX_FS_BOOT_PATH ?= $(PSDK_RTOS_PATH)/bootfs
export VISION_APPS_PATH ?= $(PSDK_RTOS_PATH)/vision_apps
export SDK_BUILDER_PATH ?= $(PSDK_RTOS_PATH)/sdk_builder
ifneq ($(SOC),j722s)
endif
ifeq ($(SOC),j722s)
export MCU_PLUS_PATH ?= $(PSDK_RTOS_PATH)/mcu_plus_sdk
endif
export GCC_ARCH64_LINUX_VERSION ?= 13.2.Rel1
export GCC_LINUX_ARM_ROOT ?= $(PSDK_TOOLS_PATH)/arm-gnu-toolchain-${GCC_ARCH64_LINUX_VERSION}-x86_64-aarch64-none-linux-gnu
export ETHFW_PATH ?= $(PSDK_RTOS_PATH)/ethfw
export TOOLS_INSTALL_PATH=$(PSDK_TOOLS_PATH)
