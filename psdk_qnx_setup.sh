###############################################################################
# Copyright (c) 2024, Texas Instruments Incorporated
# SPDX License Identifier: MIT
#
# This script in intended to minize the amount of manual steps required for
# PSDK QNX package installation for QNX SDP 710 or QNX SDP 800.
#
# This script MUST be run from the "psdkqa" directory, after the
# PSDK QNX package has been extracted.
#
# The invoking of this scripts MUST be
#
#    cd psdkqa
#    ./psdk_qnx_setup.sh
#
# The location of the BSP is determined by the user during installation of
# BSP from QNX Sofwtare Center.  This script is assuming that the BSP
# was downloaded to
#
#    ${QNX_BSP_PATH}/BSP_ti-j722s-evm_br-710_be-710_SVNxxxxxx_JBNyy.zip
#
#    OR
#
#    ${QNX_BSP_PATH}/BSP_ti-j722s-evm_br-800_be-800_SVNxxxxxx_JBNyy.zip
#
###############################################################################

if [ -z $QNX_SDP_VERSION ]
then
  echo "Set QNX_SDP_VERSION to one of the QNX SDP options (710, 800)"
  exit
fi

# Paths to different locations with PSDK QNX installation's "psdkqa" directory
PSDK_QNX_ROOT=${PWD}
QNX_BASE=/home/$USER/qnx/qnx${QNX_SDP_VERSION}
PLATFORM=j722s

# The QNX installation directory may differ, but the
# download location of BSP from QNX Software Center
# should be resident in the same relative location --> /home/$USER/qnx${QNX_SDP_VERSION}/bsp
QNX_BSP_PATH=${QNX_BASE}/bsp

if [[ ${QNX_SDP_VERSION} == "710" ]]
then
  # NOTE: Need to be updated based on BSP revision used
  QNX_BSP_VERSION=BSP_ti-j722s-evm_br-710_INVALID_OS                    ### No Version available on QNX Software Center
elif [[ ${QNX_SDP_VERSION} == "800" ]]
then
  # NOTE: Need to be updated based on BSP revision used
  QNX_BSP_VERSION=BSP_ti-j722s-evm_br-hw-rel_be-800_SVN997772_JBN64     ### Version available on QNX Software Center
fi

QNX_BSP_NAME=${QNX_BSP_VERSION}.zip

# Print off the configuration
print_variables()
{
  echo "For installation script to function below variables must be correct"
  echo "PSDK_QNX_ROOT=${PSDK_QNX_ROOT}"
  echo "QNX_BSP_NAME=${QNX_BSP_NAME}"
  echo "QNX_BSP_PATH=${QNX_BSP_PATH}"
}

# Lets check the setup, before running the script
check_variables()
{
  # Check PSDK_QNX Paths exists
  if [ ! -d ${PSDK_QNX_ROOT}/qnx ]
    then
    echo "Script not called from the PSDK QNX's psdkqa folder!"
    exit
  fi

  # Check BSP Path exists
  if [ ! -d ${QNX_BSP_PATH} ]
    then
    echo "Invalid path to ${QNX_BSP_PATH}"
    echo "${QNX_BSP_NAME} must be extracted and available at, ${QNX_BSP_PATH}"
    exit
  fi

  # Check BSP zip file exists
  if [ ! -f ${QNX_BSP_PATH}/${QNX_BSP_NAME} ]
    then
    echo "Invalid path to ${QNX_BSP_PATH}/${QNX_BSP_NAME}"
    echo "${QNX_BSP_NAME} must be extracted and available at, ${QNX_BSP_PATH}/bsp"
    exit
  fi
}

check_setup_psdk_qnx_done()
{
  if [ -f ${PSDK_QNX_ROOT}/psdk_qnx_setup.done ]
    then
    echo "The setup script already called previously. exiting!"
    exit
  fi
}

setup_qnx_bsp_in_psdkqa()
{
  echo "setup_qnx_bsp_in_psdkqa!!!"
  # Extract QNX BSP to psdkqa/qnx/bsp
  mkdir -p ${PSDK_QNX_ROOT}/qnx/bsp
  unzip ${QNX_BSP_PATH}/${QNX_BSP_NAME} -d ${PSDK_QNX_ROOT}/qnx/bsp

  # Update the QNX BSP with relevant TI BSP files
  cp -Rv ${PSDK_QNX_ROOT}/qnx/scripts/bsp/${QNX_BSP_VERSION}/* ${PSDK_QNX_ROOT}/qnx/bsp/
  echo "setup_qnx_bsp_in_psdkqa done!!!"
}

fixup_rtos_mcu_plus_sdk_dirname()
{
  if [ ! -d ${PSDK_QNX_ROOT}/../psdk_rtos ]
  then
    echo ""
    echo "******************************************************************"
    echo "Looks like PSDK QNX is not installed inside the PSDK RTOS!"
    echo "Please note building Vision Apps, EthFW and/or SBL may require"
    echo "enviornment updates to ${PSDK_QNX_ROOT}/qnx/qnx_tools_path.mak file."
    echo "******************************************************************"
    echo ""
  else
    echo "fixup_rtos_mcu_plus_sdk_dirname!!!"
    # make sure that rtos mcu_plus_sdk is correct
    CORRECT_MCU_PLUS_SDK_VERSION=`ls ${PSDK_QNX_ROOT}/.. | grep "mcu_plus_sdk_${PLATFORM}_.*"`

    if grep -Fxq "MCU_PLUS_PATH=$CORRECT_MCU_PLUS_SDK_VERSION" ${PSDK_QNX_ROOT}/qnx/qnx_tools_path.mak
    then
      echo "RTOS MCU_PLUS_SDK directory is already correct. ${CORRECT_MCU_PLUS_SDK_VERSION}"
    else
      echo "Updating MCU_PLUS_PATH=${CORRECT_MCU_PLUS_SDK_VERSION} in ${PSDK_QNX_ROOT}/qnx/qnx_tools_path.mak"
      sed -i -e "s|export MCU_PLUS_PATH ?= \$(PSDK_RTOS_PATH)/*|export MCU_PLUS_PATH ?= \$(PSDK_RTOS_PATH)/$CORRECT_MCU_PLUS_SDK_VERSION\nOLD_MCU_PLUS_PATH=|g" ${PSDK_QNX_ROOT}/qnx/qnx_tools_path.mak
    fi
    echo "fixup_rtos_mcu_plus_sdk_dirname done!!!"
  fi
}

fixup_bsp()
{
    echo "Fixing up bsp images/Makefile"
    # Remove the asix target from the makefile
    sed -i 's/ifs-$(BOARD)-ti-spl-nfs-with-.*.raw.*//g' ${PSDK_QNX_ROOT}/qnx/bsp/images/Makefile
    echo "fixup_bsp done!!!"
}

setup_psdk_qnx_done()
{
  touch psdk_qnx_setup.done
  echo "setup done!!!"
}

print_variables
check_variables
check_setup_psdk_qnx_done
setup_qnx_bsp_in_psdkqa
fixup_rtos_mcu_plus_sdk_dirname
fixup_bsp
setup_psdk_qnx_done
