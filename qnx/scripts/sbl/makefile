include ../../soc_configs.mk

.PHONY: all clean

ifeq ($(PSDK_QNX_PATH), )
  $(error PSDK_QNX_PATH not set!)
endif

ifeq ($(PSDK_RTOS_PATH), )
  $(error PSDK_RTOS_PATH not set!)
endif

ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), pdk))
ifeq ($(PDK_PATH), )
	$(error PDK_PATH not set!)
endif
else ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), mcu_plus_sdk))
ifeq ($(MCU_PLUS_PATH), )
	$(error MCU_PLUS_PATH not set!)
endif
endif

# FW build type: Supported IPC Echo test (default), EthFW
BUILD_ETHFW ?= 0

# Firmware OS type: supported : freertos, safertos
FIRMWARE_OS_TYPE ?= freertos

# Supported BOARDs and SOCs
SUPPORTED_BOARDS = j721e_evm j7200_evm j721s2_evm j784s4_evm j722s_evm
SUPPORTED_SOCS = $(SUPPORTED_BOARDS:_evm=)

ifeq ($(BOARD),$(filter $(BOARD), $(SUPPORTED_BOARDS)))
    export SOC=$(BOARD:_evm=)
else ifeq ($(BOARD),$(filter $(BOARD), $(SUPPORTED_BOARDS:_evm=_evm_hs)))
    export SOC=$(BOARD:_evm_hs=)
else
    $(error Unsupported BOARD! BOARD=$(BOARD), Supported boards=$(SUPPORTED_BOARDS))
endif

ifeq ($(BUILD_ETHFW),$(filter $(BUILD_ETHFW), 1))
  ifeq ($(ETHFW_PATH), )
    $(error ETHFW_PATH not set!)
  endif
  ifeq ($(SOC),$(filter $(SOC), j721s2, j722s))
    $(error ETHFW build not supported for j721s2!)
  endif
endif

# Build profile for EthFW build
ifeq ($(PROFILE), $(filter $(PROFILE), release all))
  BUILD_PROFILE=release
  STRIP_SUFFIX=_strip
endif
ifeq ($(PROFILE),debug)
  BUILD_PROFILE=debug
  STRIP_SUFFIX=
endif

# Supported : pdk, mcusw
BOOTAPP ?= pdk

SBL_CORE=mcu1_0
MCUSW_PATH=$(PSDK_RTOS_PATH)/mcusw
SBL_REPO_PATH=$(PDK_PATH)/packages/ti/boot/sbl
MULTICORE_APPIMAGE_GEN_TOOL_PATH=$(SBL_REPO_PATH)/tools/multicoreImageGen/bin
SBL_OUT2RPRC_GEN_TOOL_PATH=$(SBL_REPO_PATH)/tools/out2rprc/bin
ETHFW_FIRMWARE_PATH=$(PSDK_QNX_PATH)/rootfs_ethfw_$(FIRMWARE_OS_TYPE)/lib/firmware
ECHO_FIRMWARE_PATH=$(PSDK_QNX_PATH)/rootfs_echotest_$(FIRMWARE_OS_TYPE)/lib/firmware
SBL_BOOTFILES_PATH?=$(PSDK_QNX_PATH)/qnx/scripts/sbl/out
SBL_ETHFW_BOOTFILES_PATH=$(PSDK_QNX_PATH)/bootfs_sbl_ethfw_$(FIRMWARE_OS_TYPE)
SBL_ECHO_BOOTFILES_PATH=$(PSDK_QNX_PATH)/bootfs_sbl_echotest_$(FIRMWARE_OS_TYPE)
SBL_ETHFW_BOOTFILES_HS_PATH=$(PSDK_QNX_PATH)/bootfs_sbl_ethfw_hs_$(FIRMWARE_OS_TYPE)
SBL_ECHO_BOOTFILES_HS_PATH=$(PSDK_QNX_PATH)/bootfs_sbl_echotest_hs_$(FIRMWARE_OS_TYPE)
ATF_OPTEE_PATH=$(SBL_BOOTFILES_PATH)/atf_optee_dir
LDS_PATH=$(PSDK_QNX_PATH)/qnx/scripts/sbl
DEV_ID=55

#APPIMAGE LINKER SCRIPTS
ifdef FDT_BIN_NAME
  QNX_APPIMAGE_LINKER_SCRIPT=ifs_qnx_dtb.lds
else
  QNX_APPIMAGE_LINKER_SCRIPT=ifs_qnx.lds
endif

ifeq ($(USE_OPTEE),$(filter $(USE_OPTEE), 1))
  ATF_APPIMAGE_LINKER_SCRIPT=atf_optee.lds
else
  ATF_APPIMAGE_LINKER_SCRIPT=atf_only.lds
endif

#OSPI DATA
OSPI_LOCATION_SBL_IMAGE=0
OSPI_LOCATION_SYSFW_BIN=80000
OSPI_LOCATION_BOOT_APP=100000
OSPI_LOCATION_MULTI_CORE_IMAGE_1=1FC0000
OSPI_LOCATION_MULTI_CORE_IMAGE_2=27C0000
OSPI_LOCATION_ATF_IMAGE=1C0000
OSPI_LOCATION_LINUX_DTB=1EC0000
OSPI_LOCATION_HLOS_KERNEL_IMAGE=7C0000
ifeq ($(SOC),$(filter $(SOC), j721e))
  OSPI_PATTERN=3FE0000
endif
ifeq ($(SOC),$(filter $(SOC), j7200 j721s2 j784s4))
  OSPI_PATTERN=3FC0000
endif
OSPI_PATTERN_FILE=$(PDK_PATH)/packages/ti/board/src/flash/nor/ospi/nor_spi_patterns.bin

#UNIFLASH INFO
ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), pdk))
UNIFLASH_VERSION=uniflash_8.6.0
UNIFLASH_DIR=${HOME}/ti/$(UNIFLASH_VERSION)
UNIFLASH_COM_PORT=/dev/ttyUSB5
UNIFLASH_SCRIPT=$(UNIFLASH_DIR)/dslite.sh
else ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), mcu_plus_sdk))
UNIFLASH_DIR=$(MCU_PLUS_PATH)/tools/boot
UNIFLASH_COM_PORT=/dev/ttyUSB2
UNIFLASH_SCRIPT=uart_uniflash.py
UART_BOOTLOADER_SCRIPT=uart_bootloader.py
endif

ifneq ("$(wildcard $(UNIFLASH_SCRIPT))","")
  UNIFLASH_FOUND=yes
else
  UNIFLASH_FOUND=no
endif

CERT_SCRIPT=$(PDK_PATH)/packages/ti/build/makerules/x509CertificateGen.sh

#Remote Core List
REMOTE_CORE_LIST_LATEAPP1=
REMOTE_CORE_LIST_LATEAPP1+=10 $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-main-r5f0_0-fw.rprc
REMOTE_CORE_LIST_LATEAPP1+=11 $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-main-r5f0_1-fw.rprc

REMOTE_CORE_LIST_LATEAPP2=
REMOTE_CORE_LIST_LATEAPP2=12 $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-main-r5f1_0-fw.rprc
REMOTE_CORE_LIST_LATEAPP2+=13 $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-main-r5f1_1-fw.rprc
ifeq ($(SOC),$(filter $(SOC), j721e))
  REMOTE_CORE_LIST_LATEAPP2+=16 $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-c66_0-fw.rprc
  REMOTE_CORE_LIST_LATEAPP2+=17 $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-c66_1-fw.rprc
  REMOTE_CORE_LIST_LATEAPP2+=18 $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-c71_0-fw.rprc
endif
ifeq ($(SOC),$(filter $(SOC), j721s2))
  REMOTE_CORE_LIST_LATEAPP2+=18 $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-c71_0-fw.rprc
  REMOTE_CORE_LIST_LATEAPP2+=19 $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-c71_1-fw.rprc
endif
ifeq ($(SOC),$(filter $(SOC), j784s4))
  REMOTE_CORE_LIST_LATEAPP2+=14 $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-main-r5f2_0-fw.rprc
  REMOTE_CORE_LIST_LATEAPP2+=15 $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-main-r5f2_1-fw.rprc
  REMOTE_CORE_LIST_LATEAPP2+=18 $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-c71_0-fw.rprc
  REMOTE_CORE_LIST_LATEAPP2+=19 $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-c71_1-fw.rprc
  REMOTE_CORE_LIST_LATEAPP2+=20 $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-c71_2-fw.rprc
  REMOTE_CORE_LIST_LATEAPP2+=21 $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-c71_3-fw.rprc
endif

all: sbl_bootimage

ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), pdk))
clean: sbl_bootimage_scrub
else ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), mcu_plus_sdk))
clean:
	$(MAKE) -C $(MCU_PLUS_PATH) scrub DEVICE=$(SOC_MCU_PLUS) -sj
	rm -rf $(SBL_BOOTFILES_PATH)
endif

ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), pdk))
sbl_bootimage: sbl tifs bootapp lateapps a72_appimages
ifeq ($(BUILD_ETHFW),$(filter $(BUILD_ETHFW), 1))
	mkdir -p $(SBL_ETHFW_BOOTFILES_PATH)
	cp $(SBL_BOOTFILES_PATH)/app $(SBL_ETHFW_BOOTFILES_PATH)
	cp $(SBL_BOOTFILES_PATH)/*.appimage $(SBL_ETHFW_BOOTFILES_PATH)
	cp $(SBL_BOOTFILES_PATH)/lateapp* $(SBL_ETHFW_BOOTFILES_PATH)
	cp $(SBL_BOOTFILES_PATH)/tiboot3.bin $(SBL_ETHFW_BOOTFILES_PATH)
	cp $(SBL_BOOTFILES_PATH)/tifs.bin $(SBL_ETHFW_BOOTFILES_PATH)
else
	mkdir -p $(SBL_ECHO_BOOTFILES_PATH)
	cp $(SBL_BOOTFILES_PATH)/app $(SBL_ECHO_BOOTFILES_PATH)
	cp $(SBL_BOOTFILES_PATH)/*.appimage $(SBL_ECHO_BOOTFILES_PATH)
	cp $(SBL_BOOTFILES_PATH)/lateapp* $(SBL_ECHO_BOOTFILES_PATH)
	cp $(SBL_BOOTFILES_PATH)/tiboot3.bin $(SBL_ECHO_BOOTFILES_PATH)
	cp $(SBL_BOOTFILES_PATH)/tifs.bin $(SBL_ECHO_BOOTFILES_PATH)
endif
else ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), mcu_plus_sdk))
sbl_bootimage: sbl_sd tifs a53_appimages
	mkdir -p $(SBL_ECHO_BOOTFILES_PATH)
	cp $(SBL_BOOTFILES_PATH)/qnx.appimage.hs_fs $(SBL_ECHO_BOOTFILES_PATH)/app
	cp $(SBL_BOOTFILES_PATH)/tiboot3.bin        $(SBL_ECHO_BOOTFILES_PATH)/tiboot3.bin
	cp $(SBL_BOOTFILES_PATH)/tifs.bin           $(SBL_ECHO_BOOTFILES_PATH)/tifs.bin
endif


sbl_bootimage_hs: sbl_hs tifs_hs bootapp_hs lateapps_hs a72_appimages_hs
ifeq ($(BUILD_ETHFW),$(filter $(BUILD_ETHFW), 1))
	mkdir -p $(SBL_ETHFW_BOOTFILES_HS_PATH)
	cp $(SBL_BOOTFILES_PATH)/hs_signed/* $(SBL_ETHFW_BOOTFILES_HS_PATH)
else
	mkdir -p $(SBL_ECHO_BOOTFILES_HS_PATH)
	cp $(SBL_BOOTFILES_PATH)/hs_signed/* $(SBL_ECHO_BOOTFILES_HS_PATH)
endif

pre_step:
	echo "Board is $(BOARD)"
	echo "SOC is $(SOC)"
	mkdir -p $(SBL_BOOTFILES_PATH)
ifeq ($(BUILD_ETHFW),$(filter $(BUILD_ETHFW), 1))
	mkdir -p $(ETHFW_FIRMWARE_PATH)
else
	mkdir -p $(ECHO_FIRMWARE_PATH)
endif

#################
## SBL
#################
sbl: sbl_sd sbl_ospi

sbl_hs: sbl_sd_hs sbl_ospi_hs

ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), pdk))
sbl_sd: pre_step
	$(MAKE) -C $(PDK_PATH)/packages/ti/build sbl_mmcsd_img DISABLE_RECURSE_DEPS=no BOARD=$(BOARD) CORE=$(SBL_CORE) -s
	mkdir -p $(SBL_BOOTFILES_PATH)
	cp $(PDK_PATH)/packages/ti/boot/sbl/binary/$(BOARD)/mmcsd/bin/sbl_mmcsd_img_$(SBL_CORE)_release.tiimage $(SBL_BOOTFILES_PATH)/tiboot3.bin
else ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), mcu_plus_sdk))
sbl_sd: pre_step mcu_plus_sdk_libs
	$(MAKE) -C $(MCU_PLUS_PATH)/examples/drivers/boot/sbl_sd_hlos/$(MCU_BOARD_TYPE)/wkup-r5fss0-0_nortos/ti-arm-clang all
	cp $(MCU_PLUS_PATH)/examples/drivers/boot/sbl_sd_hlos/$(MCU_BOARD_TYPE)/wkup-r5fss0-0_nortos/ti-arm-clang/sbl_sd_hlos.release.hs_fs.tiimage $(SBL_BOOTFILES_PATH)/tiboot3.bin
endif

sbl_sd_hs: sbl_sd
	$(MAKE) -C $(PDK_PATH)/packages/ti/build sbl_mmcsd_img_hs DISABLE_RECURSE_DEPS=no BOARD=$(BOARD) CORE=$(SBL_CORE) -s
	mkdir -p $(SBL_BOOTFILES_PATH)/hs_signed
	cp $(PDK_PATH)/packages/ti/boot/sbl/binary/$(BOARD)_hs/mmcsd/bin/sbl_mmcsd_img_$(SBL_CORE)_release.tiimage $(SBL_BOOTFILES_PATH)/hs_signed/tiboot3.bin

sbl_ospi: pre_step sbl_sd
	$(MAKE) -C $(PDK_PATH)/packages/ti/build sbl_cust_img DISABLE_RECURSE_DEPS=no BOARD=$(BOARD) CORE=$(SBL_CORE) -s
	mkdir -p $(SBL_BOOTFILES_PATH)
	cp $(PDK_PATH)/packages/ti/boot/sbl/binary/$(BOARD)/cust/bin/sbl_cust_img_$(SBL_CORE)_release.tiimage $(SBL_BOOTFILES_PATH)/tiboot3.bin.ospi.tiimage

sbl_ospi_hs: sbl_ospi
	$(MAKE) -C $(PDK_PATH)/packages/ti/build sbl_cust_img_hs DISABLE_RECURSE_DEPS=no BOARD=$(BOARD) CORE=$(SBL_CORE) -s
	mkdir -p $(SBL_BOOTFILES_PATH)/hs_signed
	cp $(PDK_PATH)/packages/ti/boot/sbl/binary/$(BOARD)_hs/cust/bin/sbl_cust_img_$(SBL_CORE)_release.tiimage $(SBL_BOOTFILES_PATH)/hs_signed/tiboot3.bin.ospi.tiimage


#################
# TIFS
#################
tifs: pre_step
ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), pdk))
	cp $(PDK_PATH)/packages/ti/drv/sciclient/soc/$(SCICLIENT_SOCVER)/tifs.bin $(SBL_BOOTFILES_PATH)/tifs.bin
else ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), mcu_plus_sdk))
	cp $(MCU_PLUS_PATH)/source/drivers/sciclient/soc/$(SOC)/tifs-hs-fs-enc.bin $(SBL_BOOTFILES_PATH)/tifs.bin
endif

tifs_hs: tifs
ifeq ($(SOC),$(filter $(SOC), j721e))
	mkdir -p $(SBL_BOOTFILES_PATH)
	mkdir -p $(SBL_BOOTFILES_PATH)/hs_signed
	cp $(PDK_PATH)/packages/ti/drv/sciclient/soc/$(SCICLIENT_SOCVER)/tifs_sr2-hs-enc.bin $(SBL_BOOTFILES_PATH)/hs_signed/tifs.bin.sr2.0
	cp $(PDK_PATH)/packages/ti/drv/sciclient/soc/$(SCICLIENT_SOCVER)/tifs_sr1.1-hs-enc.bin $(SBL_BOOTFILES_PATH)/hs_signed/tifs.bin.sr1.1
	cp $(PDK_PATH)/packages/ti/drv/sciclient/soc/$(SCICLIENT_SOCVER)/tifs-hs-enc.bin $(SBL_BOOTFILES_PATH)/hs_signed/tifs.bin.sr1.0
	## default to SR1.1
	cp $(PDK_PATH)/packages/ti/drv/sciclient/soc/$(SCICLIENT_SOCVER)/tifs_sr1.1-hs-enc.bin $(SBL_BOOTFILES_PATH)/hs_signed/tifs.bin
endif
ifeq ($(SOC),$(filter $(SOC), j7200))
	mkdir -p $(SBL_BOOTFILES_PATH)
	mkdir -p $(SBL_BOOTFILES_PATH)/hs_signed
	cp $(PDK_PATH)/packages/ti/drv/sciclient/soc/$(SCICLIENT_SOCVER)/tifs-hs-enc.bin $(SBL_BOOTFILES_PATH)/hs_signed/tifs.bin.sr1.0
	cp $(PDK_PATH)/packages/ti/drv/sciclient/soc/$(SCICLIENT_SOCVER)/tifs_sr2-hs-enc.bin $(SBL_BOOTFILES_PATH)/hs_signed/tifs.bin.sr2.0
	## default to SR2.0
	cp $(PDK_PATH)/packages/ti/drv/sciclient/soc/$(SCICLIENT_SOCVER)/tifs_sr2-hs-enc.bin $(SBL_BOOTFILES_PATH)/hs_signed/tifs.bin
endif
ifeq ($(SOC),$(filter $(SOC),  j721s2 j784s4))
	mkdir -p $(SBL_BOOTFILES_PATH)/hs_signed
	cp $(PDK_PATH)/packages/ti/drv/sciclient/soc/$(SCICLIENT_SOCVER)/tifs-hs-enc.bin $(SBL_BOOTFILES_PATH)/hs_signed/tifs.bin
endif

#################
## Device Tree Blob (DTB)
#################
dtb_copy:
ifdef FDT_BIN_NAME
	mkdir -p $(SBL_BOOTFILES_PATH)
	cp $(FDT_PATH)/$(FDT_BIN_NAME) $(SBL_BOOTFILES_PATH)
endif

#################
## MCU+ SDK Libs
#################
mcu_plus_sdk_libs:
	make -sj -C $(MCU_PLUS_PATH) -f makefile.$(SOC) libs

#################
## BootApp
#################
ifeq ($(BOOTAPP),$(filter $(BOOTAPP), pdk))
bootapp: pdk_bootapp
bootapp_hs: pdk_bootapp_hs
endif
ifeq ($(BOOTAPP),$(filter $(BOOTAPP), mcusw))
bootapp: mcusw_bootapp
bootapp_hs: mcusw_bootapp_hs
endif

#################
## MCU BootApp
#################
mcusw_bootapp: mcusw_bootapp_sd mcusw_bootapp_ospi

mcusw_bootapp_hs: mcusw_bootapp_sd_hs mcusw_bootapp_ospi_hs

sbl_mcusw_bootimage_touch:
	touch $(MCUSW_PATH)/mcuss_demos/boot_app_mcu_rtos/boot.c
	touch $(MCUSW_PATH)/mcuss_demos/boot_app_mcu_rtos/main_tirtos.c
	touch $(MCUSW_PATH)/mcuss_demos/boot_app_mcu_rtos/soc/$(SOC)/boot_core_defs.c

mcusw_bootapp_sd: sbl
	$(MAKE) sbl_mcusw_bootimage_touch
	$(MAKE) -C $(MCUSW_PATH)/build can_boot_app_mcu_rtos -s HLOSBOOT=qnx BOOTMODE=mmcsd BOARD=$(BOARD) CORE=$(SBL_CORE) BUILD_OS_TYPE=freertos CANFUNC=none
	mkdir -p $(SBL_BOOTFILES_PATH)
	cp $(MCUSW_PATH)/binary/can_boot_app_mcu_rtos/bin/$(BOARD)/can_boot_app_mcu_rtos_$(SBL_CORE)_release.appimage $(SBL_BOOTFILES_PATH)/app

mcusw_bootapp_sd_hs: mcusw_bootapp_sd
	mkdir -p $(SBL_BOOTFILES_PATH)/hs_signed
	cp -f $(SBL_BOOTFILES_PATH)/app $(SBL_BOOTFILES_PATH)/app.save
	$(CERT_SCRIPT)  -b $(SBL_BOOTFILES_PATH)/app -o $(SBL_BOOTFILES_PATH)/hs_signed/app -c R5 -l 0x0 -k $(PDK_PATH)/packages/ti/build/makerules/k3_dev_mpk.pem
	mv -f $(SBL_BOOTFILES_PATH)/app.save $(SBL_BOOTFILES_PATH)/app

mcusw_bootapp_ospi: sbl mcusw_bootapp_sd
	$(MAKE) sbl_mcusw_bootimage_touch
	$(MAKE) -C $(MCUSW_PATH)/build can_boot_app_mcu_rtos -s HLOSBOOT=qnx BOOTMODE=ospi BOARD=$(BOARD) CORE=$(SBL_CORE) BUILD_OS_TYPE=freertos CANFUNC=none
	mkdir -p $(SBL_BOOTFILES_PATH)
	cp $(MCUSW_PATH)/binary/can_boot_app_mcu_rtos/bin/$(BOARD)/can_boot_app_mcu_rtos_$(SBL_CORE)_release.appimage $(SBL_BOOTFILES_PATH)/app_ospi

mcusw_bootapp_ospi_hs: mcusw_bootapp_ospi
	mkdir -p $(SBL_BOOTFILES_PATH)/hs_signed
	cp -f $(SBL_BOOTFILES_PATH)/app_ospi $(SBL_BOOTFILES_PATH)/app_ospi.save
	$(CERT_SCRIPT)  -b $(SBL_BOOTFILES_PATH)/app_ospi -o $(SBL_BOOTFILES_PATH)/hs_signed/app_ospi -c R5 -l 0x0 -k $(PDK_PATH)/packages/ti/build/makerules/k3_dev_mpk.pem
	mv -f $(SBL_BOOTFILES_PATH)/app_ospi.save $(SBL_BOOTFILES_PATH)/app_ospi

#################
## PDK BootApp
#################
pdk_bootapp: pdk_bootapp_sd pdk_bootapp_ospi

pdk_bootapp_hs: pdk_bootapp_sd_hs pdk_bootapp_ospi_hs

pdk_bootapp_sd: sbl
	$(MAKE) -C $(PDK_PATH)/packages/ti/build boot_app_mmcsd_qnx -s BOARD=$(BOARD) CORE=$(SBL_CORE) DISABLE_RECURSE_DEPS=no
	mkdir -p $(SBL_BOOTFILES_PATH)
	cp $(PDK_PATH)/packages/ti/boot/sbl/example/boot_app/binary/$(BOARD)/mmcsd/sbl_boot_app_mmcsd_qnx_$(BOARD)_$(SBL_CORE)_freertos_TestApp_release.appimage $(SBL_BOOTFILES_PATH)/app

pdk_bootapp_sd_hs: pdk_bootapp_sd
	$(MAKE) -C $(PDK_PATH)/packages/ti/build boot_app_mmcsd_qnx_hs -s BOARD=$(BOARD) CORE=$(SBL_CORE) DISABLE_RECURSE_DEPS=no
	mkdir -p $(SBL_BOOTFILES_PATH)/hs_signed
	cp $(PDK_PATH)/packages/ti/boot/sbl/example/boot_app/binary/$(BOARD)_hs/mmcsd/sbl_boot_app_mmcsd_qnx_hs_$(BOARD)_$(SBL_CORE)_freertos_TestApp_release.appimage.signed $(SBL_BOOTFILES_PATH)/hs_signed/app

pdk_bootapp_ospi: sbl pdk_bootapp_sd
	$(MAKE) -C $(PDK_PATH)/packages/ti/build boot_app_ospi_qnx -s BOARD=$(BOARD) CORE=$(SBL_CORE) DISABLE_RECURSE_DEPS=no
	mkdir -p $(SBL_BOOTFILES_PATH)
	cp $(PDK_PATH)/packages/ti/boot/sbl/example/boot_app/binary/$(BOARD)/ospi/sbl_boot_app_ospi_qnx_$(BOARD)_$(SBL_CORE)_freertos_TestApp_release.appimage $(SBL_BOOTFILES_PATH)/app_ospi

pdk_bootapp_ospi_hs: pdk_bootapp_ospi
	$(MAKE) -C $(PDK_PATH)/packages/ti/build boot_app_ospi_qnx_hs -s BOARD=$(BOARD) CORE=$(SBL_CORE) DISABLE_RECURSE_DEPS=no
	mkdir -p $(SBL_BOOTFILES_PATH)/hs_signed
	cp $(PDK_PATH)/packages/ti/boot/sbl/example/boot_app/binary/$(BOARD)_hs/ospi/sbl_boot_app_ospi_qnx_hs_$(BOARD)_$(SBL_CORE)_freertos_TestApp_release.appimage.signed $(SBL_BOOTFILES_PATH)/hs_signed/app_ospi

#################
## ETHFW
#################
ethfw_image:
ifeq ($(FIRMWARE_OS_TYPE),$(filter $(FIRMWARE_OS_TYPE), freertos))
	$(MAKE) -C $(ETHFW_PATH) ethfw_all BUILD_CPU_MCU2_0=yes BUILD_CPU_MCU2_1=yes BUILD_APP_TIRTOS=no BUILD_APP_FREERTOS=yes BUILD_SOC_LIST=$(SOC_CAPS) PROFILE=$(BUILD_PROFILE) -s -j
	mkdir -p $(ETHFW_FIRMWARE_PATH)
	cp $(ETHFW_PATH)/out/$(SOC_CAPS)/R5Ft/FREERTOS/$(BUILD_PROFILE)/app_remoteswitchcfg_server_qnx.xer5f $(ETHFW_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f0_0-fw
	cp $(ETHFW_PATH)/out/$(SOC_CAPS)/R5Ft/FREERTOS/$(BUILD_PROFILE)/app_remoteswitchcfg_client.xer5f $(ETHFW_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f0_1-fw
endif
ifeq ($(FIRMWARE_OS_TYPE),$(filter $(FIRMWARE_OS_TYPE), safertos))
	$(MAKE) -C $(ETHFW_PATH) ethfw_all BUILD_CPU_MCU2_0=yes BUILD_CPU_MCU2_1=yes BUILD_APP_TIRTOS=no BUILD_APP_FREERTOS=no BUILD_APP_SAFERTOS=yes BUILD_SOC_LIST=$(SOC_CAPS) PROFILE=$(BUILD_PROFILE) -s -j
	mkdir -p $(ETHFW_FIRMWARE_PATH)
	cp $(ETHFW_PATH)/out/$(SOC_CAPS)/R5Ft/SAFERTOS/$(BUILD_PROFILE)/app_remoteswitchcfg_server_qnx.xer5f $(ETHFW_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f0_0-fw
	cp $(ETHFW_PATH)/out/$(SOC_CAPS)/R5Ft/SAFERTOS/$(BUILD_PROFILE)/app_remoteswitchcfg_client.xer5f $(ETHFW_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f0_1-fw
endif

##############################
## IPC ECHO TEST FIRMWARE
##############################
ifeq ($(SOC),$(filter $(SOC), j721e))
echo_firmware: echo_firmware_mcu1_0 echo_firmware_mcu1_1 echo_firmware_mcu2_0 echo_firmware_mcu2_1 echo_firmware_mcu3_0 echo_firmware_mcu3_1 echo_firmware_c66xdsp_1 echo_firmware_c66xdsp_2 echo_firmware_c7x_1
endif
ifeq ($(SOC),$(filter $(SOC), j7200))
echo_firmware: echo_firmware_mcu1_0 echo_firmware_mcu1_1 echo_firmware_mcu2_0 echo_firmware_mcu2_1
endif
ifeq ($(SOC),$(filter $(SOC), j721s2))
echo_firmware: echo_firmware_mcu1_0 echo_firmware_mcu1_1 echo_firmware_mcu2_0 echo_firmware_mcu2_1 echo_firmware_mcu3_0 echo_firmware_mcu3_1 echo_firmware_c7x_1 echo_firmware_c7x_2
endif
ifeq ($(SOC),$(filter $(SOC), j784s4))
echo_firmware: echo_firmware_mcu1_0 echo_firmware_mcu1_1 echo_firmware_mcu2_0 echo_firmware_mcu2_1 echo_firmware_mcu3_0 echo_firmware_mcu3_1 echo_firmware_mcu4_0 echo_firmware_mcu4_1 echo_firmware_c7x_1 echo_firmware_c7x_2 echo_firmware_c7x_3 echo_firmware_c7x_4
endif
ifeq ($(SOC),$(filter $(SOC), j722s))
echo_firmware: echo_firmware_mcu1_0 echo_firmware_c7x_1 echo_firmware_c7x_2 echo_firmware_main1_0
endif

ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), pdk))
echo_firmware_mcu1_0:
	$(MAKE) -C $(PDK_PATH)/packages/ti/build   ipc_qnx_echo_testb_$(FIRMWARE_OS_TYPE) CORE=mcu1_0 BOARD=$(BOARD) SOC=$(SOC) BUILD_PROFILE=$(PROFILE) DISABLE_RECURSE_DEPS=no -s
	mkdir -p $(ECHO_FIRMWARE_PATH)
	cp $(PDK_PATH)/packages/ti/binary/ipc_qnx_echo_testb_$(FIRMWARE_OS_TYPE)/bin/$(BOARD)/ipc_qnx_echo_testb_$(FIRMWARE_OS_TYPE)_mcu1_0_$(PROFILE)$(STRIP_SUFFIX).xer5f $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-mcu-r5f0_0-fw
else ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), mcu_plus_sdk))
echo_firmware_mcu1_0: mcu_plus_sdk_libs
	make -C $(MCU_PLUS_PATH)/examples/drivers/ipc/ipc_rpmsg_echo_qnx/$(MCU_BOARD_TYPE)/mcu-r5fss0-0_freertos/ti-arm-clang all BOARD=$(BOARD) SOC=$(SOC)
	mkdir -p $(ECHO_FIRMWARE_PATH)
	cp $(MCU_PLUS_PATH)/examples/drivers/ipc/ipc_rpmsg_echo_qnx/$(MCU_BOARD_TYPE)/mcu-r5fss0-0_freertos/ti-arm-clang/ipc_rpmsg_echo_qnx.release.out $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-mcu-r5f0_0-fw
endif

echo_firmware_mcu1_1:
	$(MAKE) -C $(PDK_PATH)/packages/ti/build   ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE) CORE=mcu1_1 BOARD=$(BOARD) SOC=$(SOC) BUILD_PROFILE=$(PROFILE) DISABLE_RECURSE_DEPS=no -s
	mkdir -p $(ECHO_FIRMWARE_PATH)
	cp $(PDK_PATH)/packages/ti/binary/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)/bin/$(BOARD)/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)_mcu1_1_$(PROFILE)$(STRIP_SUFFIX).xer5f $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-mcu-r5f0_1-fw

echo_firmware_mcu2_0:
	$(MAKE) -C $(PDK_PATH)/packages/ti/build   ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE) CORE=mcu2_0 BOARD=$(BOARD) SOC=$(SOC) BUILD_PROFILE=$(PROFILE) DISABLE_RECURSE_DEPS=no -s
	mkdir -p $(ECHO_FIRMWARE_PATH)
	cp $(PDK_PATH)/packages/ti/binary/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)/bin/$(BOARD)/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)_mcu2_0_$(PROFILE)$(STRIP_SUFFIX).xer5f $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f0_0-fw

echo_firmware_mcu2_1:
	$(MAKE) -C $(PDK_PATH)/packages/ti/build   ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE) CORE=mcu2_1 BOARD=$(BOARD) SOC=$(SOC) BUILD_PROFILE=$(PROFILE) DISABLE_RECURSE_DEPS=no -s
	mkdir -p $(ECHO_FIRMWARE_PATH)
	cp $(PDK_PATH)/packages/ti/binary/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)/bin/$(BOARD)/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)_mcu2_1_$(PROFILE)$(STRIP_SUFFIX).xer5f $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f0_1-fw

echo_firmware_mcu3_0:
	$(MAKE) -C $(PDK_PATH)/packages/ti/build   ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE) CORE=mcu3_0 BOARD=$(BOARD) SOC=$(SOC) BUILD_PROFILE=$(PROFILE) DISABLE_RECURSE_DEPS=no -s
	mkdir -p $(ECHO_FIRMWARE_PATH)
	cp $(PDK_PATH)/packages/ti/binary/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)/bin/$(BOARD)/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)_mcu3_0_$(PROFILE)$(STRIP_SUFFIX).xer5f $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f1_0-fw

echo_firmware_mcu3_1:
	$(MAKE) -C $(PDK_PATH)/packages/ti/build   ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE) CORE=mcu3_1 BOARD=$(BOARD) SOC=$(SOC) BUILD_PROFILE=$(PROFILE) DISABLE_RECURSE_DEPS=no -s
	mkdir -p $(ECHO_FIRMWARE_PATH)
	cp $(PDK_PATH)/packages/ti/binary/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)/bin/$(BOARD)/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)_mcu3_1_$(PROFILE)$(STRIP_SUFFIX).xer5f $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f1_1-fw

echo_firmware_mcu4_0:
	$(MAKE) -C $(PDK_PATH)/packages/ti/build   ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE) CORE=mcu4_0 BOARD=$(BOARD) SOC=$(SOC) BUILD_PROFILE=$(PROFILE) DISABLE_RECURSE_DEPS=no -s
	mkdir -p $(ECHO_FIRMWARE_PATH)
	cp $(PDK_PATH)/packages/ti/binary/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)/bin/$(BOARD)/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)_mcu4_0_$(PROFILE)$(STRIP_SUFFIX).xer5f $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f2_0-fw

echo_firmware_mcu4_1:
	$(MAKE) -C $(PDK_PATH)/packages/ti/build   ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE) CORE=mcu4_1 BOARD=$(BOARD) SOC=$(SOC) BUILD_PROFILE=$(PROFILE) DISABLE_RECURSE_DEPS=no -s
	mkdir -p $(ECHO_FIRMWARE_PATH)
	cp $(PDK_PATH)/packages/ti/binary/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)/bin/$(BOARD)/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)_mcu4_1_$(PROFILE)$(STRIP_SUFFIX).xer5f $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f2_1-fw

echo_firmware_c66xdsp_1:
	$(MAKE) -C $(PDK_PATH)/packages/ti/build   ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE) CORE=c66xdsp_1 BOARD=$(BOARD) SOC=$(SOC) BUILD_PROFILE=$(PROFILE) DISABLE_RECURSE_DEPS=no -s
	mkdir -p $(ECHO_FIRMWARE_PATH)
	cp $(PDK_PATH)/packages/ti/binary/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)/bin/$(BOARD)/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)_c66xdsp_1_$(PROFILE)$(STRIP_SUFFIX).xe66 $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-c66_0-fw

echo_firmware_c66xdsp_2: echo_firmware_c66xdsp_1
	$(MAKE) -C $(PDK_PATH)/packages/ti/build   ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE) CORE=c66xdsp_2 BOARD=$(BOARD) SOC=$(SOC) BUILD_PROFILE=$(PROFILE) DISABLE_RECURSE_DEPS=no -s
	mkdir -p $(ECHO_FIRMWARE_PATH)
	cp $(PDK_PATH)/packages/ti/binary/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)/bin/$(BOARD)/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)_c66xdsp_2_$(PROFILE)$(STRIP_SUFFIX).xe66 $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-c66_1-fw

ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), pdk))
echo_firmware_c7x_1:
	$(MAKE) -C $(PDK_PATH)/packages/ti/build   ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE) CORE=c7x_1 BOARD=$(BOARD) SOC=$(SOC) BUILD_PROFILE=$(PROFILE) DISABLE_RECURSE_DEPS=no -s
	mkdir -p $(ECHO_FIRMWARE_PATH)
	cp $(PDK_PATH)/packages/ti/binary/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)/bin/$(BOARD)/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)_c7x_1_$(PROFILE)$(STRIP_SUFFIX).xe71 $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-c71_0-fw
else ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), mcu_plus_sdk))
echo_firmware_c7x_1: mcu_plus_sdk_libs
	make -C $(MCU_PLUS_PATH)/examples/drivers/ipc/ipc_rpmsg_echo_qnx/$(MCU_BOARD_TYPE)/c75ss0-0_freertos/ti-c7000 all BOARD=$(BOARD) SOC=$(SOC)
	mkdir -p $(ECHO_FIRMWARE_PATH)
	cp $(MCU_PLUS_PATH)/examples/drivers/ipc/ipc_rpmsg_echo_qnx/$(MCU_BOARD_TYPE)/c75ss0-0_freertos/ti-c7000/ipc_rpmsg_echo_qnx.release.out $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-c71_0-fw
endif

ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), pdk))
echo_firmware_c7x_2: echo_firmware_c7x_1
	$(MAKE) -C $(PDK_PATH)/packages/ti/build   ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE) CORE=c7x_2 BOARD=$(BOARD) SOC=$(SOC) BUILD_PROFILE=$(PROFILE) DISABLE_RECURSE_DEPS=no -s
	mkdir -p $(ECHO_FIRMWARE_PATH)
	cp $(PDK_PATH)/packages/ti/binary/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)/bin/$(BOARD)/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)_c7x_2_$(PROFILE)$(STRIP_SUFFIX).xe71 $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-c71_1-fw
else ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), mcu_plus_sdk))
echo_firmware_c7x_2: mcu_plus_sdk_libs
	make -C $(MCU_PLUS_PATH)/examples/drivers/ipc/ipc_rpmsg_echo_qnx/$(MCU_BOARD_TYPE)/c75ss1-0_freertos/ti-c7000 all BOARD=$(BOARD) SOC=$(SOC)
	mkdir -p $(ECHO_FIRMWARE_PATH)
	cp $(MCU_PLUS_PATH)/examples/drivers/ipc/ipc_rpmsg_echo_qnx/$(MCU_BOARD_TYPE)/c75ss1-0_freertos/ti-c7000/ipc_rpmsg_echo_qnx.release.out $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-c71_1-fw
endif

echo_firmware_c7x_3: echo_firmware_c7x_2
	$(MAKE) -C $(PDK_PATH)/packages/ti/build   ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE) CORE=c7x_3 BOARD=$(BOARD) SOC=$(SOC) BUILD_PROFILE=$(PROFILE) DISABLE_RECURSE_DEPS=no -s
	mkdir -p $(ECHO_FIRMWARE_PATH)
	cp $(PDK_PATH)/packages/ti/binary/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)/bin/$(BOARD)/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)_c7x_3_$(PROFILE)$(STRIP_SUFFIX).xe71 $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-c71_2-fw

echo_firmware_c7x_4: echo_firmware_c7x_3
	$(MAKE) -C $(PDK_PATH)/packages/ti/build   ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE) CORE=c7x_4 BOARD=$(BOARD) SOC=$(SOC) BUILD_PROFILE=$(PROFILE) DISABLE_RECURSE_DEPS=no -s
	mkdir -p $(ECHO_FIRMWARE_PATH)
	cp $(PDK_PATH)/packages/ti/binary/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)/bin/$(BOARD)/ipc_qnx_echo_test_$(FIRMWARE_OS_TYPE)_c7x_4_$(PROFILE)$(STRIP_SUFFIX).xe71 $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-c71_3-fw

ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), pdk))
echo_firmware_main1_0:
	$(error this firmware is not suppoorted via PDK)
else ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), mcu_plus_sdk))
echo_firmware_main1_0: mcu_plus_sdk_libs
	make -C $(MCU_PLUS_PATH)/examples/drivers/ipc/ipc_rpmsg_echo_qnx/$(MCU_BOARD_TYPE)/main-r5fss0-0_freertos/ti-arm-clang all BOARD=$(BOARD) SOC=$(SOC)
	mkdir -p $(ECHO_FIRMWARE_PATH)
	cp $(MCU_PLUS_PATH)/examples/drivers/ipc/ipc_rpmsg_echo_qnx/$(MCU_BOARD_TYPE)/main-r5fss0-0_freertos/ti-arm-clang/ipc_rpmsg_echo_qnx.release.out $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f0_0-fw
endif

#################
## LATEAPPS
#################
ifeq ($(SOC),$(filter $(SOC), j721e j721s2 j784s4))
lateapps: bootapp sbl_lateapp1 sbl_lateapp2

lateapps_hs: bootapp_hs sbl_lateapp1_hs sbl_lateapp2_hs
endif
ifeq ($(SOC),$(filter $(SOC), j7200))
lateapps: bootapp sbl_lateapp1

lateapps_hs: bootapp_hs sbl_lateapp1_hs
endif

ifeq ($(BUILD_ETHFW),$(filter $(BUILD_ETHFW), 1))
sbl_lateapp1: sbl ethfw_image
	mkdir -p $(SBL_BOOTFILES_PATH)/rprcs
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ETHFW_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f0_0-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-main-r5f0_0-fw.rprc
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ETHFW_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f0_1-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-main-r5f0_1-fw.rprc
	$(MULTICORE_APPIMAGE_GEN_TOOL_PATH)/MulticoreImageGen LE $(DEV_ID) $(SBL_BOOTFILES_PATH)/lateapp1 $(REMOTE_CORE_LIST_LATEAPP1)
else
sbl_lateapp1: sbl echo_firmware
	mkdir -p $(SBL_BOOTFILES_PATH)/rprcs
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f0_0-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-main-r5f0_0-fw.rprc
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f0_1-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-main-r5f0_1-fw.rprc
	$(MULTICORE_APPIMAGE_GEN_TOOL_PATH)/MulticoreImageGen LE $(DEV_ID) $(SBL_BOOTFILES_PATH)/lateapp1 $(REMOTE_CORE_LIST_LATEAPP1)
endif

sbl_lateapp1_hs: sbl sbl_lateapp1
	mkdir -p $(SBL_BOOTFILES_PATH)/hs_signed
	cp -f $(SBL_BOOTFILES_PATH)/lateapp1 $(SBL_BOOTFILES_PATH)/lateapp1.save
	$(CERT_SCRIPT) -b $(SBL_BOOTFILES_PATH)/lateapp1 -o $(SBL_BOOTFILES_PATH)/hs_signed/lateapp1 -c R5 -l 0x0 -k $(PDK_PATH)/packages/ti/build/makerules/k3_dev_mpk.pem
	mv -f $(SBL_BOOTFILES_PATH)/lateapp1.save $(SBL_BOOTFILES_PATH)/lateapp1

ifeq ($(BUILD_ETHFW),$(filter $(BUILD_ETHFW), 1))
sbl_lateapp2:

sbl_lateapp2_hs:
else
ifeq ($(SOC),$(filter $(SOC), j721e))
sbl_lateapp2: sbl echo_firmware
	mkdir -p $(SBL_BOOTFILES_PATH)/rprcs
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f1_0-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-main-r5f1_0-fw.rprc
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f1_1-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-main-r5f1_1-fw.rprc
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-c66_0-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-c66_0-fw.rprc
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-c66_1-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-c66_1-fw.rprc
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-c71_0-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-c71_0-fw.rprc
	$(MULTICORE_APPIMAGE_GEN_TOOL_PATH)/MulticoreImageGen LE $(DEV_ID) $(SBL_BOOTFILES_PATH)/lateapp2 $(REMOTE_CORE_LIST_LATEAPP2)
endif

ifeq ($(SOC),$(filter $(SOC), j721s2))
sbl_lateapp2: sbl echo_firmware
	mkdir -p $(SBL_BOOTFILES_PATH)/rprcs
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f1_0-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-main-r5f1_0-fw.rprc
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f1_1-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-main-r5f1_1-fw.rprc
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-c71_0-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-c71_0-fw.rprc
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-c71_1-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-c71_1-fw.rprc
	$(MULTICORE_APPIMAGE_GEN_TOOL_PATH)/MulticoreImageGen LE $(DEV_ID) $(SBL_BOOTFILES_PATH)/lateapp2 $(REMOTE_CORE_LIST_LATEAPP2)
endif

ifeq ($(SOC),$(filter $(SOC), j784s4))
sbl_lateapp2: sbl echo_firmware
	mkdir -p $(SBL_BOOTFILES_PATH)/rprcs
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f1_0-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-main-r5f1_0-fw.rprc
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f1_1-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-main-r5f1_1-fw.rprc
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f2_0-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-main-r5f2_0-fw.rprc
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-main-r5f2_1-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-main-r5f2_1-fw.rprc
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-c71_0-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-c71_0-fw.rprc
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-c71_1-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-c71_1-fw.rprc
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-c71_2-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-c71_2-fw.rprc
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(ECHO_FIRMWARE_PATH)/$(SOC_TAG)-c71_3-fw $(SBL_BOOTFILES_PATH)/rprcs/$(SOC_TAG)-c71_3-fw.rprc
	$(MULTICORE_APPIMAGE_GEN_TOOL_PATH)/MulticoreImageGen LE $(DEV_ID) $(SBL_BOOTFILES_PATH)/lateapp2 $(REMOTE_CORE_LIST_LATEAPP2)
endif

sbl_lateapp2_hs: sbl_lateapp2
	mkdir -p $(SBL_BOOTFILES_PATH)/hs_signed
	cp -f $(SBL_BOOTFILES_PATH)/lateapp2 $(SBL_BOOTFILES_PATH)/lateapp2.save
	$(CERT_SCRIPT) -b $(SBL_BOOTFILES_PATH)/lateapp2 -o $(SBL_BOOTFILES_PATH)/hs_signed/lateapp2 -c R5 -l 0x0 -k $(PDK_PATH)/packages/ti/build/makerules/k3_dev_mpk.pem
	mv -f $(SBL_BOOTFILES_PATH)/lateapp2.save $(SBL_BOOTFILES_PATH)/lateapp2
endif

#################
## ATF (/OPTEE)
#################

atf_optee:
	mkdir -p $(ATF_OPTEE_PATH)
	# For ATF, setting HANDLE_EA_EL3_FIRST_NS=0 for QNX so that the all runtime exception to be routed to current exception level (or in EL1 if the current exception level is EL0)
	$(MAKE) -C $(PDK_PATH)/../trusted-firmware-a -s -j32 CROSS_COMPILE=$(GCC_LINUX_ARM_ROOT)/bin/aarch64-none-linux-gnu- PLAT=k3 TARGET_BOARD=$(ATF_TARGET_BOARD) HANDLE_EA_EL3_FIRST_NS=0 K3_USART=$(K3_USART) $(ATF_OPTEE_PARAM) $(ATF_FDT_LOAD_PARAM)
	cp $(PDK_PATH)/../trusted-firmware-a/build/k3/$(ATF_TARGET_BOARD)/release/bl31.bin $(ATF_OPTEE_PATH)/bl31.bin
ifeq ($(USE_OPTEE),$(filter $(USE_OPTEE), 1))
	$(MAKE) -C $(PDK_PATH)/../optee_os -s -j32 CROSS_COMPILE_core=$(GCC_LINUX_ARM_ROOT)/bin/aarch64-none-linux-gnu- CROSS_COMPILE_ta_arm32=$(GCC_LINUX_ARM_ROOT)/bin/aarch64-none-linux-gnu- CROSS_COMPILE_ta_arm64=$(GCC_LINUX_ARM_ROOT)/bin/aarch64-none-linux-gnu- NOWERROR=1 CFG_TEE_TA_LOG_LEVEL=0 CFG_TEE_CORE_LOG_LEVEL=2 CFG_ARM64_core=y ta-targets=ta_arm64 PLATFORM=k3 PLATFORM_FLAVOR=j7 CFG_CONSOLE_UART=$(CFG_CONSOLE_UART)
	cp $(PDK_PATH)/../optee_os/out/arm-plat-k3/core/tee-pager_v2.bin $(ATF_OPTEE_PATH)/bl32.bin
endif

atf_optee_a53:
	$(MAKE) -C $(PDK_QNX_PATH)/../../trusted-firmware-a -s -j32 CROSS_COMPILE=$(GCC_LINUX_ARM_ROOT)/bin/aarch64-none-linux-gnu- PLAT=k3 TARGET_BOARD=$(ATF_TARGET_BOARD) SPD=opteed HANDLE_EA_EL3_FIRST_NS=0 $(ATF_FDT_LOAD_PARAM)
	$(MAKE) -C $(PDK_QNX_PATH)/../../optee_os -s -j32 CROSS_COMPILE_core=$(GCC_LINUX_ARM_ROOT)/bin/aarch64-none-linux-gnu- CROSS_COMPILE_ta_arm32=$(GCC_LINUX_ARM_ROOT)/bin/aarch64-none-linux-gnu- CROSS_COMPILE_ta_arm64=$(GCC_LINUX_ARM_ROOT)/bin/aarch64-none-linux-gnu- NOWERROR=1 CFG_TEE_TA_LOG_LEVEL=0 CFG_TEE_CORE_LOG_LEVEL=2 CFG_ARM64_core=y ta-targets=ta_arm64 PLATFORM=k3 PLATFORM_FLAVOR=j722s
	mkdir -p $(ATF_OPTEE_PATH)
	cp $(PDK_QNX_PATH)/../../trusted-firmware-a/build/k3/lite/release/bl31.bin $(SBL_BOOTFILES_PATH)/bl31.bin
	cp $(PDK_QNX_PATH)/../../optee_os/out/arm-plat-k3/core/tee-pager_v2.bin $(SBL_BOOTFILES_PATH)/bl32.bin

#################
## QNX APPIMAGES
#################
a72_appimages: atf_optee
ifeq ("$(wildcard $(QNX_SBL_IFS_RAW))","")
	$(error $(QNX_SBL_IFS_RAW) is still not built!)
else
	mkdir -p $(SBL_BOOTFILES_PATH)/rprcs
	cp $(QNX_SBL_IFS_RAW) $(SBL_BOOTFILES_PATH)/qnx-ifs
endif
ifdef FDT_BIN_NAME
	cp $(FDT_PATH)/$(FDT_BIN_NAME) $(SBL_BOOTFILES_PATH)/dtb.bin
endif
	cd $(ATF_OPTEE_PATH) && \
	$(QNX_BASE)/host/linux/x86_64/usr/bin/$(QNX_CROSS_COMPILER_TOOL)ld -T $(LDS_PATH)/$(ATF_APPIMAGE_LINKER_SCRIPT) -o $(SBL_BOOTFILES_PATH)/rprcs/atf_optee.elf
	cd $(SBL_BOOTFILES_PATH) && \
	$(QNX_BASE)/host/linux/x86_64/usr/bin/$(QNX_CROSS_COMPILER_TOOL)ld -T $(LDS_PATH)/$(QNX_APPIMAGE_LINKER_SCRIPT) -o $(SBL_BOOTFILES_PATH)/rprcs/ifs_qnx.elf
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(SBL_BOOTFILES_PATH)/rprcs/atf_optee.elf $(SBL_BOOTFILES_PATH)/rprcs/atf_optee.rprc
	mono $(SBL_OUT2RPRC_GEN_TOOL_PATH)/out2rprc.exe $(SBL_BOOTFILES_PATH)/rprcs/ifs_qnx.elf $(SBL_BOOTFILES_PATH)/rprcs/ifs_qnx.rprc
	$(MULTICORE_APPIMAGE_GEN_TOOL_PATH)/MulticoreImageGen LE $(DEV_ID) $(SBL_BOOTFILES_PATH)/atf_optee.appimage 0 $(SBL_BOOTFILES_PATH)/rprcs/atf_optee.rprc
	$(MULTICORE_APPIMAGE_GEN_TOOL_PATH)/MulticoreImageGen LE $(DEV_ID) $(SBL_BOOTFILES_PATH)/ifs_qnx.appimage 0 $(SBL_BOOTFILES_PATH)/rprcs/ifs_qnx.rprc

a72_appimages_hs: a72_appimages
	mkdir -p $(SBL_BOOTFILES_PATH)/hs_signed
	$(CERT_SCRIPT) -b $(SBL_BOOTFILES_PATH)/atf_optee.appimage -o $(SBL_BOOTFILES_PATH)/hs_signed/atf_optee.appimage -c R5 -l 0x0 -k $(PDK_PATH)/packages/ti/build/makerules/k3_dev_mpk.pem
	$(CERT_SCRIPT) -b $(SBL_BOOTFILES_PATH)/ifs_qnx.appimage -o $(SBL_BOOTFILES_PATH)/hs_signed/ifs_qnx.appimage -c R5 -l 0x0 -k $(PDK_PATH)/packages/ti/build/makerules/k3_dev_mpk.pem

a53_appimages: pre_step atf_optee_a53 echo_firmware mcu_plus_sdk_libs
ifeq ("$(wildcard $(PSDK_QNX_PATH)/qnx/bsp/images/ifs-$(SOC)-evm-ti-sbl.raw)","")
	$(error qnx-ifs is still not built!)
else
	cp $(PSDK_QNX_PATH)/qnx/bsp/images/ifs-$(SOC)-evm-ti-sbl.raw $(SBL_BOOTFILES_PATH)/qnx-ifs
endif
	$(MAKE) -C $(MCU_PLUS_PATH)/tools/boot/qnxAppimageGen clean BOARD=$(MCU_BOARD_TYPE) DEVICE_TYPE=HS_FS
	$(MAKE) -C $(MCU_PLUS_PATH)/tools/boot/qnxAppimageGen all BOARD=$(MCU_BOARD_TYPE) DEVICE_TYPE=HS_FS BOARD_CONFIG=$(PSDK_QNX_PATH)/qnx/scripts/sbl/mcu_plus_sdk/$(SOC)_sbl_echo_test_config.mak FDT_BIN_NAME=$(FDT_BIN_NAME) FDT_LOAD_ADDR=$(FDT_LOAD_ADDR)
	cp $(MCU_PLUS_PATH)/tools/boot/qnxAppimageGen/board/$(MCU_BOARD_TYPE)/qnx.appimage $(SBL_BOOTFILES_PATH)/qnx.appimage
	cp $(MCU_PLUS_PATH)/tools/boot/qnxAppimageGen/board/$(MCU_BOARD_TYPE)/qnx.appimage.hs_fs $(SBL_BOOTFILES_PATH)/qnx.appimage.hs_fs


#################
## CLEAN UPS
#################
ethfw_scrub:
ifeq ($(BUILD_ETHFW),$(filter $(BUILD_ETHFW), 1))
	$(MAKE) -C $(ETHFW_PATH) ethfw_all_clean BUILD_CPU_MCU2_0=yes BUILD_CPU_MCU2_1=yes BUILD_SOC_LIST=$(SOC_CAPS) PROFILE=$(PROFILE) -s -j
	rm -rf $(ETHFW_FIRMWARE_PATH)
	rm -rf $(SBL_ETHFW_BOOTFILES_PATH)
	rm -rf $(SBL_ETHFW_BOOTFILES_HS_PATH)
endif

echo_firmware_scrub:
ifeq ($(BUILD_ETHFW),$(filter $(BUILD_ETHFW), 0))
	rm -rf $(ECHO_FIRMWARE_PATH)
	rm -rf $(SBL_ECHO_BOOTFILES_PATH)
	rm -rf $(SBL_ECHO_BOOTFILES_HS_PATH)
endif

atf_optee_scrub:
	$(MAKE) -C $(PDK_PATH)/../trusted-firmware-a clean
	$(MAKE) -C $(PDK_PATH)/../optee_os clean CFG_ARM64_core=y PLATFORM=k3 PLATFORM_FLAVOR=j7
	rm -rf $(PDK_PATH)/../trusted-firmware-a/build/k3
	rm -rf $(PDK_PATH)/../optee_os/out/arm-plat-k3

sbl_bootimage_scrub: atf_optee_scrub ethfw_scrub echo_firmware_scrub
	$(MAKE) -C $(PDK_PATH)/packages/ti/build    clean BOARD=$(BOARD) SOC=$(SOC) PROFILE=$(PROFILE) -s
	rm -rf $(PDK_PATH)/packages/ti/binary/*
	rm -rf $(PDK_PATH)/packages/ti/boot/sbl/binary
	rm -rf $(MCUSW_PATH)/binary
	rm -rf $(SBL_BOOTFILES_PATH)

#################
## OSPI FLASHING
#################
ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), pdk))
sbl_bootimage_install_ospi:
	$(MAKE) sbl_bootimage_flash_uniflash_programmer
	$(MAKE) sbl_bootimage_flash_sbl_sysfw_bootapp
ifeq ($(BUILD_ETHFW),$(filter $(BUILD_ETHFW), 1))
	$(MAKE) sbl_bootimage_flash_ethfw
else
	$(MAKE) sbl_bootimage_flash_echotest
endif
	$(MAKE) sbl_bootimage_flash_hlos
else ifeq ($(REMOTE_FIRMWARE_REPO), $(filter $(REMOTE_FIRMWARE_REPO), mcu_plus_sdk))
sbl_bootimage_install_ospi:
ifeq ("$(wildcard $(SBL_BOOTFILES_PATH)/qnx.appimage.hs_fs)","")
	$(error $(SBL_BOOTFILES_PATH)/qnx.appimage.hs_fs is still not built! Please build a SBL target first!)
endif
	$(MAKE) -C $(MCU_PLUS_PATH)/examples/drivers/boot/sbl_uart_uniflash/$(MCU_BOARD_TYPE)/wkup-r5fss0-0_nortos/ti-arm-clang all
	$(MAKE) -C $(MCU_PLUS_PATH)/examples/drivers/boot/sbl_ospi_hlos/$(MCU_BOARD_TYPE)/wkup-r5fss0-0_nortos/ti-arm-clang all
	mkdir -p $(MCU_PLUS_PATH)/tools/boot/hlos_prebuilt/$(MCU_BOARD_TYPE)/qnx
	cp $(MCU_PLUS_PATH)/examples/drivers/boot/sbl_ospi_hlos/$(MCU_BOARD_TYPE)/wkup-r5fss0-0_nortos/ti-arm-clang/sbl_ospi_hlos.release.hs_fs.tiimage $(MCU_PLUS_PATH)/tools/boot/sbl_prebuilt/$(MCU_BOARD_TYPE)/sbl_ospi_hlos.release.hs_fs.tiimage
	cp $(SBL_BOOTFILES_PATH)/qnx.appimage.hs_fs $(MCU_PLUS_PATH)/tools/boot/hlos_prebuilt/$(MCU_BOARD_TYPE)/qnx/qnx.appimage.hs_fs
	cd $(UNIFLASH_DIR) && \
	python3 $(UNIFLASH_SCRIPT) -p $(UNIFLASH_COM_PORT) --cfg=sbl_prebuilt/$(MCU_BOARD_TYPE)/default_sbl_ospi_qnx_hs_fs.cfg
endif


sbl_bootimage_flash_uniflash_programmer:
ifeq ($(UNIFLASH_FOUND), yes)
	$(UNIFLASH_SCRIPT) --mode processors -c $(UNIFLASH_COM_PORT) -f $(UNIFLASH_DIR)/processors/FlashWriter/$(BOARD)/uart_$(SOC)_evm_flash_programmer_release.tiimage -i 0 || true
else
	echo "Uniflash not found! Please install uniflash or update the uniflash path in makefile"
endif

sbl_bootimage_flash_sbl_sysfw_bootapp:
ifeq ($(UNIFLASH_FOUND), yes)
	#SBL Image
	$(UNIFLASH_SCRIPT) --mode processors -c $(UNIFLASH_COM_PORT) -f $(SBL_BOOTFILES_PATH)/tiboot3.bin.ospi.tiimage -d 3 -o $(OSPI_LOCATION_SBL_IMAGE) || true
	echo "*************************"
	echo "SBL flashed"
	echo "*************************"
	#OSPI Patten file
	$(UNIFLASH_SCRIPT) --mode processors -c $(UNIFLASH_COM_PORT) -f $(OSPI_PATTERN_FILE) -d 3 -o $(OSPI_PATTERN) || true
	echo "*************************"
	echo "OSPI Patten flashed"
	echo "*************************"
	#SYSFW BIN
	$(UNIFLASH_SCRIPT) --mode processors -c $(UNIFLASH_COM_PORT) -f $(SBL_BOOTFILES_PATH)/tifs.bin -d 3 -o $(OSPI_LOCATION_SYSFW_BIN) || true
	echo "*************************"
	echo "SYSFW BIN flashed"
	echo "*************************"
	#Boot App
	cp -fv $(SBL_BOOTFILES_PATH)/app_ospi $(SBL_BOOTFILES_PATH)/app_ospi.appimage
	$(UNIFLASH_SCRIPT) --mode processors -c $(UNIFLASH_COM_PORT) -f $(SBL_BOOTFILES_PATH)/app_ospi.appimage -d 3 -o $(OSPI_LOCATION_BOOT_APP) || true
	rm -rf $(SBL_BOOTFILES_PATH)/app_ospi.appimage
	echo "*************************"
	echo "Boot App flashed"
	echo "*************************"
else
	echo "Uniflash not found! Please install uniflash or update the uniflash path in makefile"
endif

sbl_bootimage_flash_ethfw:
ifeq ($(UNIFLASH_FOUND), yes)
	#Multicore Image 1
	cp -fv $(SBL_BOOTFILES_PATH)/lateapp1 $(SBL_BOOTFILES_PATH)/lateapp1.appimage
	$(UNIFLASH_SCRIPT) --mode processors -c $(UNIFLASH_COM_PORT) -f $(SBL_BOOTFILES_PATH)/lateapp1.appimage -d 3 -o $(OSPI_LOCATION_MULTI_CORE_IMAGE_1) || true
	rm -rf $(SBL_BOOTFILES_PATH)/lateapp1.appimage
	echo "*************************"
	echo "Multicore Image 1 flashed"
	echo "*************************"
else
	echo "Uniflash not found! Please install uniflash or update the uniflash path in makefile"
endif

sbl_bootimage_flash_echotest:
ifeq ($(UNIFLASH_FOUND), yes)
	#Multicore Image 1
	cp -fv $(SBL_BOOTFILES_PATH)/lateapp1 $(SBL_BOOTFILES_PATH)/lateapp1.appimage
	$(UNIFLASH_SCRIPT) --mode processors -c $(UNIFLASH_COM_PORT) -f $(SBL_BOOTFILES_PATH)/lateapp1.appimage -d 3 -o $(OSPI_LOCATION_MULTI_CORE_IMAGE_1) || true
	rm -rf $(SBL_BOOTFILES_PATH)/lateapp1.appimage
	echo "*************************"
	echo "Multicore Image 1 flashed"
	echo "*************************"
ifeq ($(BUILD_ETHFW),$(filter $(BUILD_ETHFW), 1))
	### no lateapp2 imaage for ethfw build
else
ifeq ($(SOC),$(filter $(SOC), j721e j721s2 j784s4))
	#Multicore Image 2
	cp -fv $(SBL_BOOTFILES_PATH)/lateapp2 $(SBL_BOOTFILES_PATH)/lateapp2.appimage
	$(UNIFLASH_SCRIPT) --mode processors -c $(UNIFLASH_COM_PORT) -f $(SBL_BOOTFILES_PATH)/lateapp2.appimage -d 3 -o $(OSPI_LOCATION_MULTI_CORE_IMAGE_2) || true
	rm -rf $(SBL_BOOTFILES_PATH)/lateapp2.appimage
	echo "*************************"
	echo "Multicore Image 2 flashed"
	echo "*************************"
endif
endif
else
	echo "Uniflash not found! Please install uniflash or update the uniflash path in makefile"
endif

sbl_bootimage_flash_hlos:
ifeq ($(UNIFLASH_FOUND), yes)
	#ATF Image
	$(UNIFLASH_SCRIPT) --mode processors -c $(UNIFLASH_COM_PORT) -f $(SBL_BOOTFILES_PATH)/atf_optee.appimage -d 3 -o $(OSPI_LOCATION_ATF_IMAGE) || true
	echo "*************************"
	echo "ATF Image flashed"
	echo "*************************"
	#HLOS Image
	$(UNIFLASH_SCRIPT) --mode processors -c $(UNIFLASH_COM_PORT) -f $(SBL_BOOTFILES_PATH)/ifs_qnx.appimage -d 3 -o $(OSPI_LOCATION_HLOS_KERNEL_IMAGE) || true
	echo "*************************"
	echo "HLOS Image flashed"
	echo "*************************"
else
	echo "Uniflash not found! Please install uniflash or update the uniflash path in makefile"
endif
