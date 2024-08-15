#! /bin/sh
# Script to create ifs_qnx.appimage for the specified qnx ifs file

QNX_SDP_VERSION=710
if [ ${QNX_SDP_VERSION} != "700" ]
then
  QNX_BASE=/home/$USER/qnx700
  QNX_CROSS_COMPILER_TOOL=aarch64-unknown-nto-qnx7.0.0-
else
  QNX_BASE=/home/$USER/qnx710
  QNX_CROSS_COMPILER_TOOL=aarch64-unknown-nto-qnx7.1.0-
fi

PDKQA_PATH=
QNX_IFS_FILE=
PDK_PATH=

# Usage
usage ()
{
  echo "
Usage: `basename $1` <options>

Mandatory options:
  --psdkqa_path      path to PSDKQA
  --file             path to the qnx-ifs file (e.g qnx-ifs)
"
  exit 1
}

# Lets check the setup, before running the script
check_variables()
{
  echo "QNX_IFS_FILE is ${QNX_IFS_FILE}"
  echo "PDKQA_PATH is ${PDKQA_PATH}"

  test -z ${QNX_IFS_FILE} && usage $0;
  test -z ${PDKQA_PATH} && usage $0;
  
  # Check the input ifs file
  if [ ! -f ${QNX_IFS_FILE} ]
  then
   echo "ERROR: $QNX_IFS_FILE is not found"
   exit 1;
  fi

  # Check that PDKQA directory exists
  if [ ! -d ${PDKQA_PATH} ]
  then
    echo "Invalid PDKQA_PATH ${PDKQA_PATH}"
    exit
  fi

  PDK_PATH=${PDKQA_PATH}/pdk
  # Check that PDK directory exists
  if [ ! -d ${PDK_PATH} ]
  then
    echo "Invalid PDK_PATH ${PDK_PATH}"
    exit
  fi

  # Check QNX BASE Path exists
  if [ ! -d ${QNX_BASE} ]
  then
    echo "Invalid path to QNX BASE, ${QNX_BASE}"
    exit
  fi
}

create_lds_file()
{
  echo "creating lds file"
  echo "
OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
TARGET(binary)
INPUT(qnx-ifs)
SECTIONS
{
 . = 0x0000000080080000;
 qnx = .;
 .qnx : { qnx-ifs }
}
" > ifs_qnx.lds
}

run ()
{
  CURR_DIR=${PWD}
  OUT_DIR=$(dirname "${QNX_IFS_FILE}")
  mkdir -p temp
  cp ${QNX_IFS_FILE} temp/qnx-ifs
  cd temp
  create_lds_file
  ${QNX_BASE}/host/linux/x86_64/usr/bin/${QNX_CROSS_COMPILER_TOOL}ld -T ifs_qnx.lds -o ifs_qnx.elf
  ${PDK_PATH}/packages/ti/boot/sbl/tools/out2rprc/bin/out2rprc.exe ifs_qnx.elf ifs_qnx.rprc
  ${PDK_PATH}/packages/ti/boot/sbl/tools/multicoreImageGen/bin/MulticoreImageGen LE 55 ifs_qnx.appimage 0 ifs_qnx.rprc
  cd ${CURR_DIR}
  cp temp/ifs_qnx.appimage ${OUT_DIR}
  rm -rf temp
  
  echo "ifs_qnx.appimage created in the path:${OUT_DIR}"
}

# Process command line...
while [ $# -gt 0 ]; do
  case $1 in
    --file) shift; QNX_IFS_FILE=$1; shift; ;;
    --psdkqa_path) shift; PDKQA_PATH=$1; shift; ;;
    *) copy="$copy $1"; shift; ;;
  esac
done

check_variables
run
