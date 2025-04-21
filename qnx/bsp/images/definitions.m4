## Boot parameters
define(`__LD_QNX__',            `ldqnx-64.so.2')
define(`__BOOT_ADDR__',         `0x80080000')
define(`__ARCH__',              `aarch64le')
define(`__TYPE__',              `raw')
define(`__COMPRESS_ATTR__',     `+compress')
define(`__PROCNTO_MODULES__',   `')
define(`__STARTUP__',           `startup-j722s-evm')
define(`__STARTUP_OPTS__',      `-u arg -v -r0x99A00000,0x26600000,1 -r0xC0000000,0x30000000,1 -r0x880000000,0x30000000,1')
define(`__PROCNTO__',           `procnto-smp-instr')
define(`__PROCNTO_OPTS__',      `-v -mr')

## Console
define(`__CONSOLE__',           `/dev/ser1')

## ENV profile, use to overwrite the common /etc/profile
#define(`__ENV_PROFILE_FILE__',  `')
#define(`__PROFILE_CFG__',       `/__ENV_PROFILE_FILE__ = {
#export HOME=/
#export SYSNAME=nto
#export TERM=qansi
#export PATH=/proc/boot:/sbin:/bin:/usr/bin:/usr/sbin:/usr/libexec
#export LD_LIBRARY_PATH=/proc/boot:/lib:/usr/lib:/lib/dll:/lib/dll/pci
#}')

## Block driver
define(`__BLOCK_DRVR__', `devb-ram, devb-sdmmc-am65x')

#define(`__DEVB_EIDE_DRVR__', `')
#define(`__DEVB_EIDE_OPTS__', `')
#define(`__DEVB_EIDE_DEV__', `')

#define(`__DEVB_NVME_DRVR__', `')
#define(`__DEVB_NVME_OPTS__', `')
#define(`__DEVB_NVME_DEV__', `')

define(`__DEVB_RAM_DRVR__', `devb-ram')
#define(`__DEVB_RAM_OPTS__', `')
#define(`__DEVB_RAM_DEV__', `')

define(`__DEVB_SDMMC_DRVR__', `devb-sdmmc-am65x')
#define(`__DEVB_SDMMC_OPTS__', `')
#define(`__DEVB_SDMMC_DEV__', `')

define(`__DEVB_SDMMC_START__', `
    ############################################################################################
    ## SD memory card / eMMC driver
    ############################################################################################
    display_msg Starting MMC/SD memory card driver... eMMC
    __DEVB_SDMMC_DRVR__ sdio addr=0x0fa10000,bw=~4:8,irq=165,timing=~hs400,emmc,bs=sscfg=0x8000 disk name=emmc

    display_msg Starting MMC/SD memory card driver... SD
    __DEVB_SDMMC_DRVR__ sdio addr=0x0fa00000,irq=115,bs=sscfg=0x8000:ldo=0x600000^70:trm-icp=2 cam pnp disk name=sd
')

define(`__DEVB_DRVR_START__', `
__DEVB_SDMMC_START__
')

## Network driver
#define(`__NET_DRVR__', `')
#define(`__NET_OPTS__', `')
#define(`__NET_DEV__', `')
define(`__NET_START__', `
    #######################################################################
    ## Using CPSW driver that requires PSDK support
    #######################################################################
    `#' display_msg "Starting networking ..."
    `#' io-sock -m phy ifdef(`__NET_OPTS__', `__NET_OPTS__') ifdef(`__PCI_HW_MODULE__', `__NET_PCI_OPTS__') ifdef(`__USB_HOST_DRVR__', `__NET_USB_OPTS__') ifdef(`__HYP__', `__NET_HYP_OPTS__') -mfdt -dcpsw-j722s

    __NET_COMMON_START__
')


## USB host driver
define(`__USB_HOST_DRVR__', `devu-hcd-dwc3-xhci.so, devu-hcd-cdns3-xhci.so')
define(`__LOCAL_XHCI_OPTS__', `-d dwc3-xhci ioport=0x31000000,irq=220 -d cdns3-xhci ioport=0x31210000,irq=258,iosize=65536')
define(`__USB_HOST_OPTS__', `__LOCAL_XHCI_OPTS__')
define(`__USB_HOST_DEV__', `/dev/usb/io-usb-otg')

define(`__USB_START__', `
    #######################################################################
    ## XHCI on both USB-C and A ports
    #######################################################################
    display_msg Starting XHCI driver on USB3SS0 and USB3SS1
    io-usb-otg __LOCAL_XHCI_OPTS__
    waitfor __USB_HOST_DEV__ 4
')

## USB device driver
#define(`__USB_DEVICE_DRVR__', `')

## Persistent storage
define(`__PERSISTENT_STORAGE_DEVICE__', `/dev/sd0t179')
#define(`__PERSISTENT_STORAGE_MOUNT_POINT__', `')
#define(`__PERSISTENT_STORAGE_MOUNT_OPTS__', `')
#define(`__PERSISTENT_STORAGE_START__', `')
#define(`__PERSISTENT_STORAGE_FILES__', `')

## Serial driver
define(`__DEVC_DRVR__', `devc-seromap')
#define(`__DEVC_OPTS__', `')
#define(`__DEVC_DEV__', `')
define(`__DEVC_START__', `
    #######################################################################
    ## UART drivers
    #######################################################################
    display_msg "start serial driver"
    __DEVC_DRVR__ -e -F 0x02800000,210
    reopen /dev/ser1
')

## I2C driver
define(`__I2C_DRVR__', `i2c-tda4')
define(`__I2C_OPTS__', `-p0x20000000 -i193 -d, -p0x20010000 -i194 -d --u1, -p0x20020000 -i195 -d --u2')
define(`__I2C_DEV__', `/dev/i2c0, /dev/i2c1, /dev/i2c2')

define(`__I2C_START__',`
    __I2C_COMMON_START__

    #######################################################################
    ## Configure the IO expanders
    #######################################################################
    /scripts/configure_i2c_io_expanders.sh
')

## NOR flash driver
define(`__NOR_DRVR__', `devf-j7-ospi')

define(`__NOR_START__', `
    #######################################################################
    ## OSPI NOR flash driver without DMSS support
    #######################################################################
    display_msg "Starting Flash driver..."
    __NOR_DRVR__ -s soc=rclk=200000000:base=0xfc40000:clk=25000000:rdelay=4
    #######################################################################
    ## OSPI NOR flash driver with DMSS support
    ## need tisci-mgr and tiudma-mgr to access TI DMSS library
    ## need "-I" option to access DMSS registers
    ## need direct and PHY mode for DMA transfer
    #######################################################################
    `#' display_msg "Starting Flash driver..."
    `#' __NOR_DRVR__ -I -s soc=rclk=200000000:base=0xfc40000:clk=25000000:rdelay=4:poffset=0x3fc0000:phy=1:dma=1
')

## PCI driver
#define(`__PCI_HW_DRVR__', `')
#define(`__PCI_HW_MODULE__', `')
#define(`__PCI_BUS_SCAN_LIMIT__', `')
#define(`__PCI_MODULE_BLACKLIST__', `')
#define(`__PCI_SERVER_CONFIG_FILE__', `')
#define(`__PCI_SERVER_CFG__', `')
#define(`__PCI_START__', `')

## Random
#define(`__RANDOM_DRVR__', `')
#define(`__RANDOM_DRVR_OPTS__', `')

## DMA
#define(`__DMA_DRVR__', `libdma-edma.so')

## RTC
#define(`__RTC_DRVR__', `rtc')
#define(`__RTC_OPTS__', `hw')
#define(`__RTC_START__', `
    ############################################################################################
    ## RTC utility - requires i2c driver to be running
    ############################################################################################
#    display_msg "Setting OS clock from RTC ..."
#    __RTC_DRVR__ __RTC_OPTS__
#')

## Customize script
define(`__CUSTOMIZE_SCRIPT_NAME__', `/scripts/board_startup.sh')
#define(`__CUSTOMIZE_SCRIPT_START__', `')
#define(`__CUSTOMIZE_SCRIPT_FILES__', `')

## Board specific files
define(`__BOARD_EARLY_START__', `
    #######################################################################
    ## TI SCI / IPC Resource Managers
    #######################################################################
    #tisci-mgr
    #waitfor /dev/tisci 4
    #shmemallocator
    #waitfor /dev/shmemallocator 4
    #tiipc-mgr
    #waitfor /dev/tiipc 4
    #tiudma-mgr
    #waitfor /dev/tiudma 4
')

define(`__BOARD_LATE_START__', `')

define(`__BOARD_FILES__', `

###########################################################################
## CPSW files
## Using CPSW driver that requires PSDK support
###########################################################################
/lib/dll/devs-cpsw-j722s.so=devs-cpsw-j722s.so

###########################################################################
## Binaries from TI PSDK
## These binaries should NOT be distributed outside of QNX.
## These binaries are developed by TI and any customers or outside
## partners should be contacting TI to get their latest PSDK.
###########################################################################
#[search=../install/aarch64le/sbin${PFS}${PSDK_QNX_PATH}/qnx/resmgr/sciclient_qnx_rsmgr/aarch64/o.le/ perms=a+x] tisci-mgr
#[search=../install/aarch64le/sbin${PFS}${PSDK_QNX_PATH}/qnx/resmgr/ipc_qnx_rsmgr/resmgr/aarch64/o.le/ perms=a+x] tiipc-mgr
#[search=../install/aarch64le/sbin${PFS}${PSDK_QNX_PATH}/qnx/resmgr/udma_qnx_rsmgr/resmgr/aarch64/o.le/ perms=a+x] tiudma-mgr
#[search=../install/aarch64le/sbin${PFS}${PSDK_QNX_PATH}/qnx/sharedmemallocator/resmgr/aarch64/o.le/ perms=a+x] shmemallocator
#[search=../install/aarch64le/lib/dll${PFS}${PSDK_QNX_PATH}/qnx/pdk_libs/pdk/aarch64/so.le] libti-pdk.so
#[search=../install/aarch64le/lib/dll${PFS}${PSDK_QNX_PATH}/qnx/pdk_libs/sciclient/aarch64/so.le] libti-sciclient.so
#[search=../install/aarch64le/lib/dll${PFS}${PSDK_QNX_PATH}/qnx/pdk_libs/ipclld/aarch64/so.le] libti-ipclld.so
#[search=../install/aarch64le/lib/dll${PFS}${PSDK_QNX_PATH}/qnx/resmgr/ipc_qnx_rsmgr/usr/aarch64/so.le] libtiipc-usr.so
#[search=../install/aarch64le/lib/dll${PFS}${PSDK_QNX_PATH}/qnx/pdk_libs/udmalld/aarch64/so.le] libti-udmalld.so
#[search=../install/aarch64le/lib/dll${PFS}${PSDK_QNX_PATH}/qnx/resmgr/udma_qnx_rsmgr/usr/aarch64/so.le] libtiudma-usr.so

################################################################################################
## Configure IO expanders script
################################################################################################
[uid=0 gid=0 perms=0755]/scripts/configure_i2c_io_expanders.sh = {
#!/bin/sh

############################################################################################
## Graphics Configuration
############################################################################################
#### Take HDMI chip out of reset ####
## De-assert GPIO_HDMI_RSTn(P04) to set as outputs
sh -c "isend -n/dev/i2c1 -a0x20 0x6 > /dev/null 2>&1"
sh -c "isendrecv -n/dev/i2c1 -a0x20 -l1 > /dev/null 2>&1"
## Returns ffh
sh -c "isend -n/dev/i2c1 -a 0x20 0x6 0xef > /dev/null 2>&1"
sh -c "isend -n/dev/i2c1 -a 0x20 0x6 > /dev/null 2>&1"
sh -c "isendrecv -n/dev/i2c1 -a0x20 -l1 > /dev/null 2>&1"
## Read to confirm efh was written

}
################################################################################################
## END OF BUILD SCRIPT
################################################################################################
')

