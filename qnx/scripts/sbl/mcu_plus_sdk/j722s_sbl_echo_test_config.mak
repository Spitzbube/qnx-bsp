# export MCU_PLUS_SDK_PATH?=$(abspath ../../../)
include $(MCU_PLUS_SDK_PATH)/imports.mak

# To-Do : Update PSDK QNX Path
# #Processor SDK QNX image path
# PSDK_QNX_PATH?=<update path here>
QNX_IFS_PATH=$(PSDK_QNX_PATH)/qnx/scripts/sbl/out

#Path for prebuit images in Processor SDK QNX
PSDK_QNX_PREBUILT_IMAGES=$(PSDK_QNX_PATH)/qnx/scripts/sbl/out

#Input qnx binaries
ATF_BIN_NAME=bl31.bin
OPTEE_BIN_NAME=bl32.bin
QNX_BIN_NAME=qnx-ifs

#QNX image load address
ATF_LOAD_ADDR=0x9e780000
OPTEE_LOAD_ADDR=0x9e800000
QNX_LOAD_ADDR=0x80080000

#Output appimage name
QNX_BOOTIMAGE_NAME=qnx.appimage

#core Ids to generate app image
BOOTIMAGE_CORE_ID_mcu-r5fss0-0  = 0
BOOTIMAGE_CORE_ID_wkup-r5fss0-0 = 1
BOOTIMAGE_CORE_ID_main-r5fss0-0 = 2
BOOTIMAGE_CORE_ID_c75ss0-0      = 3
BOOTIMAGE_CORE_ID_c75ss1-0      = 4
BOOTIMAGE_CORE_ID_a53ss0-0      = 5
BOOTIMAGE_CORE_ID_a53ss0-1      = 6
BOOTIMAGE_CORE_ID_a53ss1-0      = 7
BOOTIMAGE_CORE_ID_a53ss1-1      = 8

# Add path to executable binaries that you want to run along with the Linux
IMG1 = $(BOOTIMAGE_CORE_ID_wkup-r5fss0-0) $(MCU_PLUS_SDK_PATH)/tools/sysfw/sciserver_binary/j722s/sciclient_get_version.release.rprc
IMG2 = $(BOOTIMAGE_CORE_ID_mcu-r5fss0-0)  $(MCU_PLUS_SDK_PATH)/examples/drivers/ipc/ipc_rpmsg_echo_qnx/j722s-evm/mcu-r5fss0-0_freertos/ti-arm-clang/ipc_rpmsg_echo_qnx.release.rprc
IMG3 = $(BOOTIMAGE_CORE_ID_main-r5fss0-0) $(MCU_PLUS_SDK_PATH)/examples/drivers/ipc/ipc_rpmsg_echo_qnx/j722s-evm/main-r5fss0-0_freertos/ti-arm-clang/ipc_rpmsg_echo_qnx.release.rprc
IMG4 = $(BOOTIMAGE_CORE_ID_c75ss0-0)      $(MCU_PLUS_SDK_PATH)/examples/drivers/ipc/ipc_rpmsg_echo_qnx/j722s-evm/c75ss0-0_freertos/ti-c7000/ipc_rpmsg_echo_qnx.release.rprc
IMG5 = $(BOOTIMAGE_CORE_ID_c75ss1-0)      $(MCU_PLUS_SDK_PATH)/examples/drivers/ipc/ipc_rpmsg_echo_qnx/j722s-evm/c75ss1-0_freertos/ti-c7000/ipc_rpmsg_echo_qnx.release.rprc

RTOS_IMG_LIST = $(IMG1) $(IMG2) $(IMG3) $(IMG4) $(IMG5)
