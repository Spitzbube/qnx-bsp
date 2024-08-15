include qnx_tools_path.mak
include soc_configs.mk

.PHONY: all clean clean_all scrub
.PHONY: help print_build_vars psdk_qnx_version
.PHONY: qnx_libs qnx_libs_clean
.PHONY: qnx_bins qnx_bins_clean
.PHONY: qnx_codec qnx_codec_clean
.PHONY: examples examples_clean
.PHONY: qnx_codec_tests qnx_codec_tests_clean
.PHONY: utils utils_clean
.PHONY: bsp bsp_clean
.PHONY: pdk_deps pdk_deps_clean
.PHONY: qnx_docs qnx_docs_clean
.PHONY: sbl_echo_test sbl_echo_test_clean
.PHONY: sbl_echo_test_hs sbl_echo_test_clean_hs
.PHONY: sbl_echo_test_safertos sbl_echo_test_safertos_hs
.PHONY: echo_test_fw echo_test_fw_safertos echo_test_fw_clean
.PHONY: sbl_ethfw sbl_ethfw_clean
.PHONY: sbl_ethfw_hs sbl_ethfw_clean_hs
.PHONY: sbl_ethfw_safertos sbl_ethfw_safertos_hs
.PHONY: ethfw ethfw_safertos ethfw_clean
.PHONY: qnx_fs_create qnx_fs_clean
.PHONY: qnx_fs_copy_spl_uboot
.PHONY: sbl_echo_test_ospi_flashing sbl_echo_test_ospi_flashing_hs
.PHONY: sbl_ethfw_ospi_flashing sbl_ethfw_ospi_flashing_hs
.PHONY: sbl_combined_boot_echo_test sbl_combined_boot_echo_test_safertos sbl_combined_boot_echo_test_clean
.PHONY: sbl_combined_boot_ethfw sbl_combined_boot_ethfw_safertos sbl_combined_boot_ethfw_clean
.PHONY: sbl_combined_boot_echo_test_ospi_flashing sbl_combined_boot_ethfw_ospi_flashing
.PHONY: qnx_prep_sdk qnx_spl_create_copy
.PHONY: qnx_vision_apps_create_copy qnx_vision_apps_hs_create_copy
.PHONY: qnx_sbl_echo_create_copy qnx_sbl_echo_hs_create_copy
.PHONY: qnx_tar_built_filesystems
.PHONY: qnx_create_all_platform_packages

ifeq ($(QNX_SDP_VERSION),$(filter $(QNX_SDP_VERSION), 710))
.PHONY: qnx_sbl_ethfw_create_copy qnx_sbl_ethfw_hs_create_copy
endif

## Supported PROFILE -> release, debug, all
export PROFILE ?= release

ifeq ($(BOARD_IS_SUPPORTED), false)
    $(error Unsupported BOARD enviornment variable! BOARD=$(BOARD), Supported boards=$(SUPPORTED_BOARDS). Please export one of the supported boards.)
endif

export PLATFORM = aarch64le
export PSDK_QNX_VERSION_REL = PSDKQNX_10_00_00
export PSDK_QNX_TAGID ?= QNX$(QNX_SDP_VERSION)_$(BOARD)

## flag to denote if it is a vision app build
export VISION_APPS_BUILD_FLAGS_MAK ?=0


####################
# help
####################
help:
	### Important build env varibles:
	#
	# PROFILE                       Supported build profile for RTOS binaries ## release (Default), debug, all
	# BOARD                         Supported Boards  ## $(SUPPORTED_BOARDS)
	# SOC                           Supported SOCs    ## $(SUPPORTED_SOCS)
	# VISION_APPS_BUILD_FLAGS_MAK   Use Vision Apps make build flag ## 0 (Default), 1
	#
	### Use below make targets to build SDK and/or different components with the SDK
	#
	# make help                  # Show supported makefile target's
	# make print_build_vars      # Print the make environment variables being used
	# make all                   # Build all PSDK QNX components (qnx_libs qnx_bins qnx_codec bsp examples qnx_codec_tests utils)
	# make clean                 # Clean all PSDK QNX components. Same as "make scrub"
	# make clean_all             # Delete all generated files and folders in SDK including PSDK RTOS binaries.
	# make scrub             	 # Delete all generated files and folders in SDK including PSDK RTOS binaries.
	#
	# make qnx_fs_create         # Install PSDK QNX binaries & libraries to the local stage folder
	# make qnx_fs_copy_spl_uboot # Install spl-uboot bootloader  to the local stage folder
	# make qnx_fs_clean          # Cleanup the local stage folder
	#
	# make qnx_docs              # Generate the PSDK QNX documentation
	# make qnx_docs_clean        # Clean PSDK QNX documentation
	# make bsp                   # Build QNX BSP
	# make bsp_clean             # Clean QNX BSP
	#
	# make qnx_libs              # Build QNX PDK libraries
	# make qnx_libs_clean        # Clean QNX PDK libraries
	#
	# make pdk_deps              # Generate the dependencies for pdk
	# make pdk_deps_clean        # Clean the files generated for pdk dependencies
	#
	# make qnx_bins              # Build PSDK QNX binaries (DEVNP, RMs)
	# make qnx_bins_clean        # Clean PSDK QNX binaries
	#
	# make examples              # Build PSDK QNX examples
	# make examples_clean        # Clean PSDK QNX examples
	#
	# make utils                 # Build PSDK QNX utils (k3conf)
	# make utils_clean           # Clean PSDK QNX utils
	#
	# make qnx_codec             # Build PSDK QNX codec binary/libries
	# make qnx_codec_clean       # Clean PSDK QNX codec binary/libries
	# make qnx_codec_tests       # Build PSDK QNX codec tests
	# make qnx_codec_tests_clean # Clean PSDK QNX codec tests
	#
	# make ethfw                 # Build QNX version of EthFW remote core freertos images that can be use with Spl+Uboot
	# make ethfw_safertos        # Build QNX version of EthFW remote core safertos images that can be use with Spl+Uboot
	# make ethfw_clean           # Clean QNX version of EthFW remote core images
	# make echo_test_fw          # Build QNX version of IPC echo test remote core freertos images that can be use with Spl+Uboot
	# make echo_test_fw_safertos # Build QNX version of IPC echo test remote core safertos images that can be use with Spl+Uboot
	# make echo_test_fw_clean    # Clean QNX version of IPC echo test remote core images
	#
	### Use below make targets to build --> SBL + Boot image
	### NOTE: Need to call "make all" and "make qnx_fs_create" before call below:
	#
	# make sbl_echo_test                      # Build SBL + BootAPP + Echo test firmware freertos images + A72 App images
	# make sbl_echo_test_safertos             # Build SBL + BootAPP + Echo test firmware safertos images + A72 App images
	# make sbl_echo_test_clean                # Clean SBL + BootAPP + Echo test firmware freertos images + A72 App images
	# make sbl_ethfw                          # Build SBL + BootAPP + EthFw firmware freertos images + A72 App images
	# make sbl_ethfw_safertos                 # Build SBL + BootAPP + EthFw firmware safertos images + A72 App images
	# make sbl_ethfw_clean                    # Clean SBL + BootAPP + EthFw firmware freertos images + A72 App images
	# make sbl_echo_test_hs                   # Build SBL + BootAPP + Echo test firmware freertos images + A72 App images (Signed for HS device)
	# make sbl_echo_test_safertos_hs          # Build SBL + BootAPP + Echo test firmware safertos images + A72 App images (Signed for HS device)
	# make sbl_echo_test_clean_hs             # Clean SBL + BootAPP + Echo test firmware images + A72 App images
	# make sbl_ethfw_hs                       # Build SBL + BootAPP + EthFw firmware freertos images + A72 App images (Signed for HS device)
	# make sbl_ethfw_safertos_hs              # Build SBL + BootAPP + EthFw firmware safertos images + A72 App images (Signed for HS device)
	# make sbl_ethfw_clean_hs                 # Clean SBL + BootAPP + EthFw firmware freertos images + A72 App images
	#
	# make sbl_echo_test_ospi_flashing        # Flash SBL + BootAPP + Echo test firmware images + A72 App images to OSPI
	# make sbl_ethfw_ospi_flashing            # Flash SBL + BootAPP + EthFW firmware images + A72 App images to OSPI
	# make sbl_echo_test_ospi_flashing_hs     # Flash SBL + BootAPP + Echo test firmware images + A72 App images to OSPI (Signed for HS device)
	# make sbl_ethfw_ospi_flashing_hs         # Flash SBL + BootAPP + EthFW firmware images + A72 App images to OSPI (Signed for HS device)
	#
	# make sbl_combined_boot_echo_test        # Build SBL + Echo test firmware images + A72 App images (combined image)
	# make sbl_combined_boot_echo_test_clean  # Clean SBL + Echo test firmware images + A72 App images (combined image)
	# make sbl_combined_boot_ethfw            # Build SBL + EthFw firmware images + A72 App images (combined image)
	# make sbl_combined_boot_ethfw_clean      # Clean SBL + EthFw firmware images + A72 App images (combined image)
	#
	# make sbl_combined_boot_echo_test_ospi_flashing        # Flash SBL + Echo test firmware images + A72 App images (combined image) to OSPI
	# make sbl_combined_boot_ethfw_ospi_flashing            # Flash SBL + EthFw firmware images + A72 App images (combined image) to OSPI
	#
	### Use below make targets for automating the creation of QNX filesystems
	# make qnx_prep_sdk                		  # Clean SPL binaries + Build QNXFS Partition
	# make qnx_spl_create_copy                # Build SPL based QNX Filesystem
	# make qnx_sbl_ethfw_create_copy          # Build SBL based QNX Filesystem for Ethernet Firmware
	# make qnx_sbl_ethfw_hs_create_copy       # Build SBL based QNX Filesystem for Ethernet Firmware with Signed binaries for HS devices
	# make qnx_sbl_echo_create_copy           # Build SBL based QNX Filesystem for IPC Echo Test
	# make qnx_sbl_echo_hs_create_copy        # Build SBL based QNX Filesystem for IPC Echo Test with Signed binaries for HS devices
	# make qnx_tar_built_filesystems          # Package SPL and SBL based QNX Filesystems for Supported Jacinto Platforms ($(SUPPORTED_SOCS))
	# make qnx_create_all_platform_packages   # Scrub All binaries + Build SPL and SBL based QNX Filesystems for Supported Jacinto Platforms ($(SUPPORTED_SOCS))
	#

print_build_vars:
	@echo PROFILE = $(PROFILE)
	@echo BOARD = $(BOARD)
	@echo SOC = $(SOC)
	@echo VISION_APPS_BUILD_FLAGS_MAK = $(VISION_APPS_BUILD_FLAGS_MAK)
	@echo PSDK_QNX_PATH = $(PSDK_QNX_PATH)

all: psdk_qnx_version qnx_libs qnx_bins qnx_codec bsp examples qnx_codec_tests utils

clean:
	$(MAKE) bsp_clean
	$(MAKE) qnx_libs_clean
	$(MAKE) qnx_bins_clean
	$(MAKE) examples_clean
	$(MAKE) utils_clean
	$(MAKE) qnx_codec_clean
	$(MAKE) qnx_codec_tests_clean
	$(MAKE) qnx_fs_clean
	rm -rf psdk_qnx_version.c
	rm -rf $(PSDK_QNX_STAGE)

clean_all: clean
	$(MAKE) qnx_docs_clean
	$(MAKE) sbl_echo_test_clean
ifeq ($(SOC),$(filter $(SOC), j721e j7200 j784s4))
	$(MAKE) sbl_ethfw_clean
endif
ifeq ($(SOC),$(filter $(SOC), j721e j7200 j721s2 j784s4))
	$(MAKE) sbl_combined_boot_echo_test_clean
endif
ifeq ($(SOC),$(filter $(SOC), j721e j7200 j784s4))
	$(MAKE) sbl_combined_boot_ethfw_clean
endif
	rm -rf ../rootfs*
	rm -rf ../bootfs*

scrub: clean_all

####################
# qnx docs
####################
qnx_docs:
	mkdir -p ./docs/datasheet
	mkdir -p ./docs/user_guide/api
	doxygen ./internal_docs/doxygen_files/doxy_cfg_datasheet/psdkqa_datasheet_$(SOC).cfg 2> ./docs/datasheet/doxy_warnings.txt
	doxygen ./internal_docs/doxygen_files/api_guide/psdkqa_api_guide_$(SOC).cfg 2> ./docs/datasheet/api_guide_warnings.txt
	$(MAKE) -C ./internal_docs/sphinx_user_guide/ USERGUIDE_OUT_DIR=$(shell  pwd)/docs/user_guide DEVFAMILY=$(shell echo $(SOC) | tr a-z A-Z)

qnx_docs_clean:
	rm -rf ./docs

####################
# psdk qnx packaging
####################

qnx_fs_create:
	mkdir -p $(QNX_BOOT_PATH)
	mkdir -p $(QNX_FS_PATH)
ifeq ($(QNX_SDP_VERSION), 710)
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/qnx_fs_create_qnx710.mk qnx_sdk  BOARD=$(BOARD) SOC=$(SOC)
else ifeq ($(QNX_SDP_VERSION), 800)
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/qnx_fs_create_qnx800.mk qnx_sdk  BOARD=$(BOARD) SOC=$(SOC)
endif
ifdef FDT_BIN_NAME
  ifeq ($(REMOTE_FIRMWARE_REPO), mcu_plus_sdk)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl                     dtb_copy PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) QNX_SDP_VERSION=$(QNX_SDP_VERSION) FIRMWARE_OS_TYPE=freertos
  endif
endif

qnx_fs_copy_spl_uboot:
	mkdir -p $(QNX_BOOT_PATH)
	mkdir -p $(QNX_FS_PATH)
ifeq ($(QNX_SDP_VERSION), 710)
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/qnx_fs_create_qnx710.mk copy_spl_uboot BOARD=$(BOARD) SOC=$(SOC)
else ifeq ($(QNX_SDP_VERSION), 800)
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/qnx_fs_create_qnx800.mk copy_spl_uboot BOARD=$(BOARD) SOC=$(SOC)
endif
ifdef FDT_BIN_NAME
	cp $(FDT_PATH)/$(FDT_BIN_NAME) $(QNX_BOOT_PATH)
endif

qnx_fs_clean:
	rm -rf $(QNX_BOOT_PATH)
	rm -rf $(QNX_FS_PATH)
	rm -rf $(QNX_AUX_FS_PATH)

####################
# firmware build
####################
ethfw:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl                    ethfw_image   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) ETHFW_PATH=$(ETHFW_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) FIRMWARE_OS_TYPE=freertos

ethfw_safertos:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl                    ethfw_image   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) ETHFW_PATH=$(ETHFW_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) FIRMWARE_OS_TYPE=safertos

ethfw_clean:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl                    ethfw_scrub   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) ETHFW_PATH=$(ETHFW_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE)

echo_test_fw:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl                    echo_firmware PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) FIRMWARE_OS_TYPE=freertos

echo_test_fw_safertos:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl                    echo_firmware PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) FIRMWARE_OS_TYPE=safertos

echo_test_fw_clean:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl                    echo_firmware_scrub PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE)

######################################################
# sbl bootapp with Echo test freertos firmware build
######################################################
sbl_echo_test:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl                    all   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) QNX_SDP_VERSION=$(QNX_SDP_VERSION) FIRMWARE_OS_TYPE=freertos

sbl_echo_test_clean:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl                    clean PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) QNX_SDP_VERSION=$(QNX_SDP_VERSION)

sbl_echo_test_hs:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl                    sbl_bootimage_hs   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) QNX_SDP_VERSION=$(QNX_SDP_VERSION) FIRMWARE_OS_TYPE=freertos

sbl_echo_test_clean_hs:
	$(MAKE) sbl_echo_test_clean

######################################################
# sbl bootapp with Echo test safertos firmware build
######################################################
sbl_echo_test_safertos:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl                    all   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) QNX_SDP_VERSION=$(QNX_SDP_VERSION) FIRMWARE_OS_TYPE=safertos

sbl_echo_test_safertos_hs:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl                    sbl_bootimage_hs   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) QNX_SDP_VERSION=$(QNX_SDP_VERSION) FIRMWARE_OS_TYPE=safertos

########################################
# sbl bootapp with EthFw freertos build
########################################
sbl_ethfw:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl                    all   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) ETHFW_PATH=$(ETHFW_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) QNX_SDP_VERSION=$(QNX_SDP_VERSION) BUILD_ETHFW=1 FIRMWARE_OS_TYPE=freertos

sbl_ethfw_clean:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl                    clean PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) ETHFW_PATH=$(ETHFW_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) QNX_SDP_VERSION=$(QNX_SDP_VERSION) BUILD_ETHFW=1

sbl_ethfw_hs:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl                    sbl_bootimage_hs   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) ETHFW_PATH=$(ETHFW_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) QNX_SDP_VERSION=$(QNX_SDP_VERSION) BUILD_ETHFW=1 FIRMWARE_OS_TYPE=freertos

sbl_ethfw_clean_hs:
	$(MAKE) sbl_ethfw_clean

########################################
# sbl bootapp with EthFw safertos build
########################################
sbl_ethfw_safertos:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl                    all   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) ETHFW_PATH=$(ETHFW_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) QNX_SDP_VERSION=$(QNX_SDP_VERSION) BUILD_ETHFW=1 FIRMWARE_OS_TYPE=safertos

sbl_ethfw_safertos_hs:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl                    sbl_bootimage_hs   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) ETHFW_PATH=$(ETHFW_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) QNX_SDP_VERSION=$(QNX_SDP_VERSION) BUILD_ETHFW=1 FIRMWARE_OS_TYPE=safertos


####################
# OSPI boot flashing
####################
sbl_echo_test_ospi_flashing:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl    sbl_bootimage_install_ospi   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE)

sbl_echo_test_ospi_flashing_hs:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl    sbl_bootimage_install_ospi   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) BOARD=$(BOARD)_hs SOC=$(SOC) PROFILE=$(PROFILE) SBL_BOOTFILES_PATH=$(PSDK_QNX_PATH)/bootfs_sbl_echotest_hs_freertos

sbl_ethfw_ospi_flashing:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl    sbl_bootimage_install_ospi   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) ETHFW_PATH=$(ETHFW_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) BUILD_ETHFW=1

sbl_ethfw_ospi_flashing_hs:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl    sbl_bootimage_install_ospi   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) ETHFW_PATH=$(ETHFW_PATH) BOARD=$(BOARD)_hs SOC=$(SOC) PROFILE=$(PROFILE) BUILD_ETHFW=1 SBL_BOOTFILES_PATH=$(PSDK_QNX_PATH)/bootfs_sbl_ethfw_hs_freertos


####################
# sbl_combined_boot
####################
sbl_combined_boot_echo_test:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl_combined_boot      all   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) FIRMWARE_OS_TYPE=freertos

sbl_combined_boot_echo_test_safertos:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl_combined_boot      all   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) FIRMWARE_OS_TYPE=safertos

sbl_combined_boot_echo_test_clean:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl_combined_boot      clean PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE)

sbl_combined_boot_ethfw:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl_combined_boot      all   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) ETHFW_PATH=$(ETHFW_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) BUILD_ETHFW=1 FIRMWARE_OS_TYPE=freertos

sbl_combined_boot_ethfw_safertos:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl_combined_boot      all   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) ETHFW_PATH=$(ETHFW_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) BUILD_ETHFW=1 FIRMWARE_OS_TYPE=safertos

sbl_combined_boot_ethfw_clean:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl_combined_boot      clean PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) ETHFW_PATH=$(ETHFW_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE)  BUILD_ETHFW=1

################################################
# OSPI boot flashing of sbl_combined_boot image
################################################
sbl_combined_boot_echo_test_ospi_flashing:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl_combined_boot    sbl_combined_bootimage_install_ospi   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE)

sbl_combined_boot_ethfw_ospi_flashing:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/scripts/sbl_combined_boot    sbl_combined_bootimage_install_ospi   PSDK_RTOS_PATH=$(PSDK_RTOS_PATH) PDK_PATH=$(PDK_PATH) ETHFW_PATH=$(ETHFW_PATH) BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) BUILD_ETHFW=1


####################
# PSDK QNX version
####################
SOC_UPPER=SOC $(shell echo $(SOC) | tr a-z A-Z)
PSDK_QNX_VERSION=$(PSDK_QNX_VERSION_REL)
psdk_qnx_version:
	@echo 'char *buildSoc="$(SOC_UPPER)";' > psdk_qnx_version.c
	@echo 'char *buildDate="'`date`'";' >> psdk_qnx_version.c
	@echo 'char *buildVersion="'PSDKQNX_10_00_00'";' >> psdk_qnx_version.c
	mkdir -p $(PSDK_QNX_STAGE)
	@echo '########################################################'
	@echo 'SOC is $(SOC)'
	@echo 'BOARD is $(BOARD)'
	@echo 'QNX_SDP_VERSION is $(QNX_SDP_VERSION)'
	@echo '########################################################'
	@echo 'The PSDK QNX stage dir is $(PSDK_QNX_STAGE)'
	@echo 'The override is $(QCONF_OVERRIDE)'
	@echo '########################################################'

####################
# qnx bsp
####################
bsp: qnx_bins examples utils
ifdef FDT_BIN_NAME
	#Copying the default qnx-ifs build script to make a sbl+dtb compatiable qnx-ifs build script
	sed "s/startup-$(SOC)-evm -u arg/startup-$(SOC)-evm -u reg/1" $(PSDK_QNX_PATH)/qnx/bsp/images/$(SOC)-evm-ti.build > $(PSDK_QNX_PATH)/qnx/bsp/images/$(SOC)-evm-ti-sbl.build
endif
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/bsp all SOC=$(SOC)

bsp_clean:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/bsp SOC=$(SOC) clean

####################
# qnx pdk libs
####################
qnx_libs: psdk_qnx_version
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/pdk_libs/pdk                   install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/pdk_libs/sciclient             install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/pdk_libs/ipclld                install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/pdk_libs/udmalld               install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
ifneq "$(or $(filter 710, $(QNX_SDP_VERSION)), $(filter true, $(BUILD_ENETLLD_FOR_TRIM_PKG)))" ""
  ifeq ($(SOC),$(filter $(SOC), j721e j7200 j721s2 j784s4))
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/pdk_libs/enetlld               install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
  endif
endif


qnx_libs_clean:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/pdk_libs/pdk                   clean  PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/pdk_libs/sciclient             clean  PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/pdk_libs/ipclld                clean  PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/pdk_libs/udmalld               clean  PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
ifneq "$(or $(filter 710, $(QNX_SDP_VERSION)), $(filter true, $(BUILD_ENETLLD_FOR_TRIM_PKG)))" ""
  ifeq ($(SOC),$(filter $(SOC), j721e j7200 j721s2 j784s4))
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/pdk_libs/enetlld               clean  PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
  endif
endif

####################
# pdk lib deps
####################
pdk_deps: psdk_qnx_version
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/pdk_libs/pdk                   install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID) GEN_PDKDEPS=1
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/pdk_libs/sciclient             install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID) GEN_PDKDEPS=1
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/pdk_libs/ipclld                install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID) GEN_PDKDEPS=1
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/pdk_libs/udmalld               install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID) GEN_PDKDEPS=1
ifeq ($(SOC),$(filter $(SOC), j721e j7200 j721s2 j784s4))
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/pdk_libs/enetlld               install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID) GEN_PDKDEPS=1
endif

pdk_deps_clean:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/pdk_libs/pdk                   clean  PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/pdk_libs/sciclient             clean  PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/pdk_libs/ipclld                clean  PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/pdk_libs/udmalld               clean  PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
ifeq ($(SOC),$(filter $(SOC), j721e j7200 j721s2 j784s4))
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/pdk_libs/enetlld               clean  PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
endif

####################
# qnx bins
####################
qnx_bins: psdk_qnx_version qnx_libs
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/sharedmemallocator                     install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID) VISION_APPS_BUILD_FLAGS_MAK=$(VISION_APPS_BUILD_FLAGS_MAK)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/resmgr/sciclient_qnx_rsmgr             install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/resmgr/ipc_qnx_rsmgr/resmgr            install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/resmgr/ipc_qnx_rsmgr/usr               install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/resmgr/udma_qnx_rsmgr/usr              install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/resmgr/udma_qnx_rsmgr/resmgr           install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
ifeq ($(QNX_SDP_VERSION),$(filter $(QNX_SDP_VERSION), 710))
  ifeq ($(SOC),$(filter $(SOC), j721e j7200 j721s2 j784s4))
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/devnp/cpsw2g                           install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
  endif
  ifeq ($(SOC),$(filter $(SOC), j721e j784s4))
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/devnp/ethremotecfg                     install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/devnp/cpsw9g                           install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
  endif
  ifeq ($(SOC),$(filter $(SOC), j7200))
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/devnp/ethremotecfg                     install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/devnp/cpsw5g                           install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
  endif
  ifeq ($(SOC),$(filter $(SOC), j721s2))
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/devnp/cpsw2g-main                      install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
  endif
endif

qnx_bins_clean:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/sharedmemallocator                     clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/resmgr/sciclient_qnx_rsmgr             clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/resmgr/ipc_qnx_rsmgr/resmgr            clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/resmgr/ipc_qnx_rsmgr/usr               clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/resmgr/udma_qnx_rsmgr/usr              clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/resmgr/udma_qnx_rsmgr/resmgr           clean
ifeq ($(QNX_SDP_VERSION),$(filter $(QNX_SDP_VERSION), 710))
  ifeq ($(SOC),$(filter $(SOC), j721e j7200 j721s2 j784s4))
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/devnp                                  clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/devnp/ethremotecfg                     clean
  endif
endif

test_target:
	echo $(QNX_BASE)

####################
# qnx codec
####################
qnx_codec: qnx_libs
ifeq ($(SOC),$(filter $(SOC), j721e))
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/codec/img/qnx                  install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages PSDK_QNX_PATH=$(PSDK_QNX_PATH) BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
endif
ifeq ($(SOC),$(filter $(SOC), j721s2 j784s4 j722s))
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/codec/vpu/tivpucodec/vpulib    install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/codec/vpu/resmgrlib            install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/codec/vpu/resmgr               install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/codec/vpu/OpenMAXIL            install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
endif

qnx_codec_clean:
ifeq ($(SOC),$(filter $(SOC), j721e))
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/codec/img/qnx                  clean
endif
ifeq ($(SOC),$(filter $(SOC), j721s2 j784s4 j722s))
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/codec/vpu/tivpucodec/vpulib    clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/codec/vpu/OpenMAXIL            clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/codec/vpu/resmgrlib            clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/codec/vpu/resmgr               clean
endif

####################
# qnx examples
####################
examples: qnx_bins
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/ipc/ipc_test_qnx              install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/ipc/ipc_perf                  install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/udma/udma_memcpy_test_qnx     install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/udma/udma_crc_test            install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/sciclient/sciclient_qnx_app   install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/sciclient/sciclient_clk_test  install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/osal/sem_test                 install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/gpio                          install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/crypto/decrypt_app            install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
ifeq ($(QNX_SDP_VERSION),$(filter $(QNX_SDP_VERSION), 710))
  ifeq ($(SOC),$(filter $(SOC), j721e j7200 j721s2 j784s4))
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/cpsw/ptp_test                 install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/cpsw/cpsw_test                install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
  endif
endif

examples_clean:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/ipc/ipc_test_qnx              clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/ipc/ipc_perf                  clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/udma/udma_memcpy_test_qnx     clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/udma/udma_crc_test            clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/sciclient/sciclient_qnx_app   clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/sciclient/sciclient_clk_test  clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/osal/sem_test                 clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/gpio                          clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/crypto/decrypt_app            clean
ifeq ($(QNX_SDP_VERSION),$(filter $(QNX_SDP_VERSION), 710))
  ifeq ($(SOC),$(filter $(SOC), j721e j7200 j721s2 j784s4))
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/cpsw/ptp_test                 clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/examples/cpsw/cpsw_test                clean
  endif
endif

####################
# qnx codec tests
####################
qnx_codec_tests: qnx_codec
ifeq ($(SOC),$(filter $(SOC), j721s2 j784s4 j722s))
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/codec/vpu/tests/decoder_test            install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID) QNX_SDP_VERSION=$(QNX_SDP_VERSION)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/codec/vpu/tests/encoder_test            install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID) QNX_SDP_VERSION=$(QNX_SDP_VERSION)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/codec/vpu/tests/multi_inst_test         install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID) QNX_SDP_VERSION=$(QNX_SDP_VERSION)
endif

qnx_codec_tests_clean:
ifeq ($(SOC),$(filter $(SOC), j721s2 j784s4 j722s))
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/codec/vpu/tests/decoder_test            clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/codec/vpu/tests/encoder_test            clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/codec/vpu/tests/multi_inst_test         clean
endif

####################
# qnx utils
####################
utils: qnx_bins
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/utils/ipc_trace_logger                  install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/utils/k3conf                           -f Makefile_qnx INSTALL_CMD=install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
ifeq ($(SOC),$(filter $(SOC), j721e j7200 j721s2 j784s4 j722s))
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/utils/ddr_bw                            install-debug-strip PDK_INSTALL_PATH=$(PDK_QNX_PATH)/packages BOARD=$(BOARD) SOC=$(SOC) PSDK_QNX_VERSION_REL=$(PSDK_QNX_VERSION_REL) PSDK_QNX_TAGID=$(PSDK_QNX_TAGID)
endif

utils_clean:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/utils/ipc_trace_logger                  clean
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/utils/k3conf                           -f Makefile_qnx clean
ifeq ($(SOC),$(filter $(SOC), j721e j7200 j721s2 j784s4 j722s))
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx/utils/ddr_bw                           clean
endif

####################
# qnx automated packaging
####################
ifeq ($(SOC),$(filter $(SOC), j7200 j721e j721s2 j784s4 j722s))
qnx_prep_sdk:
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak   prep_sdk

qnx_spl_create_copy:
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak   spl_create_copy

qnx_vision_apps_create_copy:
ifeq ($(SOC),$(filter $(SOC), j721e j721s2 j784s4 j722s))
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak   vision_apps_create_copy
endif

qnx_vision_apps_hs_create_copy:
ifeq ($(SOC),$(filter $(SOC), j721e j721s2 j784s4 j722s))
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak   vision_apps_hs_create_copy
endif

ifeq ($(QNX_SDP_VERSION),$(filter $(QNX_SDP_VERSION), 710))
qnx_sbl_ethfw_create_copy:
ifeq ($(SOC),$(filter $(SOC), j7200 j721e j784s4))
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak   sbl_ethfw_create_copy
endif

qnx_sbl_ethfw_hs_create_copy:
ifeq ($(SOC),$(filter $(SOC), j7200 j721e j784s4))
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak   sbl_ethfw_hs_create_copy
endif
endif

qnx_sbl_echo_create_copy:
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak   sbl_echo_create_copy

qnx_sbl_echo_hs_create_copy:
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak   sbl_echo_hs_create_copy

qnx_tar_built_filesystems:
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak   tar_built_filesystems

qnx_create_all_platform_packages:
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak   create_all_platform_packages
endif
