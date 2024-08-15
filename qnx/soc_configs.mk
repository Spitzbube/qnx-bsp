##########################
## Supported BOARDs config
##########################

# Supported BOARDs -> j721e_evm, j7200_evm, j721s2_evm, j784s4_evm, j722s_evm
export SUPPORTED_BOARDS = j721e_evm j7200_evm j721s2_evm j784s4_evm j722s_evm
export BOARD?=UNSET_BOARD
export BOARD_IS_SUPPORTED=false

ifeq ($(BOARD), $(filter $(BOARD), $(SUPPORTED_BOARDS)))
  BOARD_IS_SUPPORTED=true
  # Strip off _.* (_evm, _sk, etc.) from board for SOC name.
  export SOC=$(word 1, $(subst _, ' ', $(BOARD)))
endif



#############################
## SOC Specific Configuration
#############################
ifeq ($(SOC), j721e)
  SCICLIENT_SOCVER = V1
  SOC_TAG = j7
  SOC_CAPS = J721E
  K3_USART=0
  CFG_CONSOLE_UART=0
  ATF_TARGET_BOARD=generic

else ifeq ($(SOC), j7200)
  SCICLIENT_SOCVER = V2
  SOC_TAG = j7200
  SOC_CAPS = J7200
  K3_USART=0
  CFG_CONSOLE_UART=0
  ATF_TARGET_BOARD=generic

else ifeq ($(SOC), j721s2)
  SCICLIENT_SOCVER = V4
  SOC_TAG = j721s2
  SOC_CAPS = J721S2
  K3_USART=0x8
  CFG_CONSOLE_UART=0x8
  ATF_TARGET_BOARD=generic

else ifeq ($(SOC), j784s4)
  SCICLIENT_SOCVER = V6
  SOC_TAG = j784s4
  SOC_CAPS = J784S4
  K3_USART=0x8
  CFG_CONSOLE_UART=0x8
  ATF_TARGET_BOARD=j784s4

else ifeq ($(SOC), j722s)
  SOC_TAG = j722s
  SOC_CAPS = J722S
  SOC_MCU_PLUS = j722s
  K3_USART=0x8
  CFG_CONSOLE_UART=0x8
  BOARD_TYPE ?= evm
  MCU_BOARD_TYPE = j722s-evm
  ATF_TARGET_BOARD=lite

endif

############################
## ATF / OPTEE configuration
############################
# With or without OP-TEE (1 or 0)
USE_OPTEE ?= 0

ifeq ($(USE_OPTEE),$(filter $(USE_OPTEE), 1))
  ATF_OPTEE_PARAM=SPD=opteed
else
  ATF_OPTEE_PARAM=
endif

############################
## Remote Firmware Repo Configuration
############################
# Supported: pdk, mcu_plus_sdk
ifeq ($(SOC),$(filter $(SOC), j7200 j721e j721s2 j784s4))
  REMOTE_FIRMWARE_REPO ?= pdk
else
  REMOTE_FIRMWARE_REPO ?= mcu_plus_sdk
endif

############################
## Device Tree Configuration
############################
# Define FDT_PATH, FDT_BIN_NAME, and FDT_LOAD_ADDR for each given SOC.
# If not defined then not FDT will be copied/built into Uboot or SPL.
ifeq ($(SOC), j721e)

  ifeq ($(QNX_SDP_VERSION), 800)
    FDT_PATH=$(PSDK_QNX_PATH)/qnx/bsp/prebuilt/usr/lib/
    FDT_BIN_NAME=psdk_linux_9.2_k3-j721e-common-proc-board-quad-port-eth-exp.dtb
    FDT_LOAD_ADDR=0x88000000
  endif

else ifeq ($(SOC), j721s2)

  ifeq ($(QNX_SDP_VERSION), 800)
    FDT_PATH=$(PSDK_QNX_PATH)/qnx/bsp/prebuilt/usr/lib
    FDT_BIN_NAME=psdk_linux_9.2_k3-j721s2-common-proc-board.dtb
    FDT_LOAD_ADDR=0x88000000
  endif

else ifeq ($(SOC), j784s4)

  ifeq ($(QNX_SDP_VERSION), 800)
    FDT_PATH=$(PSDK_QNX_PATH)/qnx/bsp/prebuilt/usr/lib
    #FDT_BIN_NAME=psdk_linux_9.2_k3-j784s4-evm.dtb
    FDT_BIN_NAME=psdk_linux_9.2_k3-j784s4-evm-quad-port-eth-exp1.dtb
    FDT_LOAD_ADDR=0x88000000
  endif

else ifeq ($(SOC), j722s)

  ifeq ($(QNX_SDP_VERSION), 800)
    FDT_PATH=$(PSDK_QNX_PATH)/qnx/bsp/prebuilt/usr/lib
    FDT_BIN_NAME=psdk_linux_9.2_k3-j722s-evm.dtb
    FDT_LOAD_ADDR=0x88000000
  endif

endif

ifdef FDT_BIN_NAME
  QNX_SBL_IFS_RAW=$(PSDK_QNX_PATH)/qnx/bsp/images/ifs-$(SOC)-evm-ti-sbl.raw
  ATF_FDT_LOAD_PARAM=K3_HW_CONFIG_BASE=$(FDT_LOAD_ADDR)
else
  QNX_SBL_IFS_RAW=$(PSDK_QNX_PATH)/qnx/bsp/images/ifs-$(SOC)-evm-ti.raw
  ATF_FDT_LOAD_PARAM=
endif