# Copyright (c) 2024, Texas Instruments Incorporated
# SPDX License Identifier: MIT

.PHONY: qnx_sdk copy_spl_uboot

# override the default QNX_TARGET to point to staged mirror
QNX_TARGET=$(PSDK_QNX_PATH)/qnx/qnx710_stage/qnx7

PROFILE ?= release
BOARD ?= j721e_evm

ifeq ($(QNX_SDP_VERSION), 700)
  $(error SDP700 not support for BOARD=j7200_evm!)
endif

ifeq ($(BOARD),$(filter $(BOARD), j721e_evm))
  SOC = j721e
else ifeq ($(BOARD),$(filter $(BOARD), j7200_evm))
  SOC = j7200
else ifeq ($(BOARD),$(filter $(BOARD), j721s2_evm))
  SOC = j721s2
else ifeq ($(BOARD),$(filter $(BOARD), j784s4_evm))
  SOC = j784s4
else ifeq ($(BOARD),$(filter $(BOARD), j722s_evm))
  SOC = j722s
else
	$(error BOARD not support at present!)
endif

qnx_sdk:
	########################
	##### Common Stuff #####
	########################
	## create needed dirs
	mkdir -p ${QNX_FS_PATH}/boot
	mkdir -p ${QNX_FS_PATH}/bin
	mkdir -p ${QNX_FS_PATH}/etc/system/config
	mkdir -p ${QNX_FS_PATH}/lib
	mkdir -p ${QNX_FS_PATH}/lib/dll
	mkdir -p ${QNX_FS_PATH}/sbin
	mkdir -p ${QNX_FS_PATH}/usr/bin
	mkdir -p ${QNX_FS_PATH}/usr/sbin
	mkdir -p ${QNX_FS_PATH}/usr/lib
	mkdir -p ${QNX_FS_PATH}/root
	mkdir -p ${QNX_FS_PATH}/etc/ssh
	mkdir -p ${QNX_FS_PATH}/var/chroot/sshd
	mkdir -p ${QNX_FS_PATH}/version/images
	chmod 0700 ${QNX_FS_PATH}/var/chroot/sshd

ifeq ($(SOC),$(filter $(SOC), j721e j721s2 j784s4))
	mkdir -p ${QNX_FS_PATH}/usr/lib/graphics
	mkdir -p ${QNX_FS_PATH}/usr/lib/graphics/$(SOC)
endif
	mkdir -p ${QNX_FS_PATH}/tilib
	mkdir -p ${QNX_FS_PATH}/tibin
	mkdir -p ${QNX_FS_PATH}/scripts
ifeq ($(SOC),$(filter $(SOC), j721e j721s2 j784s4 j722s))
	mkdir -p ${QNX_FS_PATH}/codec_test/output
endif
ifeq ($(SOC),$(filter $(SOC), j721s2 j784s4 j722s))
	mkdir -p ${QNX_FS_PATH}/tilib/firmware
endif
	## Other misc QNX binaries
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/ability                       ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/cat                           ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/cdpub                         ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/chgrp                         ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/chmod                         ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/chown                         ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/confstr                       ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/cp                            ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/cpio                          ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/csplit                        ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/dd                            ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/df                            ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/du                            ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/echo                          ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/isend                         ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/isendrecv                     ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/kill                          ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/ksh                           ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/ln                            ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/ls                            ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/mkdir                         ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/mount                         ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/mv                            ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/on                            ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/pidin                         ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/ps                            ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/pwd                           ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/restart                       ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/rm                            ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/sed                           ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/sh                            ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/slay                          ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/slog2info                     ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/slogger2                      ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/stty                          ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/sync                          ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/tar                           ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/umount                        ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/uname                         ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/usblauncher_otg               ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/vi                            ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/waitfor                       ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/ptpd_ctrl                     ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/sbin/mq                           ${QNX_FS_PATH}/sbin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/sbin/mqueue                       ${QNX_FS_PATH}/sbin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/sbin/mkdosfs                      ${QNX_FS_PATH}/sbin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/sbin/ifconfig                     ${QNX_FS_PATH}/sbin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/sbin/brconfig                     ${QNX_FS_PATH}/sbin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/sbin/route                        ${QNX_FS_PATH}/sbin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/which                     ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/nice                      ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/coreinfo                  ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/diff                      ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/find                      ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/grep                      ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/iperf2                    ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/iperf3                    ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/netstat                   ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/ssh                       ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/ssh-keyscan               ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/ssh-agent                 ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/ssh-add                   ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/ssh-keygen                ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/hogs                      ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/cksum                     ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/cut                       ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/tee                       ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/tr                        ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/sbin/tcpdump                  ${QNX_FS_PATH}/usr/sbin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/sbin/ptpd                     ${QNX_FS_PATH}/usr/sbin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/sbin/ptpd-avb                 ${QNX_FS_PATH}/usr/sbin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/sbin/tracelogger              ${QNX_FS_PATH}/usr/sbin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/sbin/nicinfo                  ${QNX_FS_PATH}/usr/sbin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/sbin/sshd                     ${QNX_FS_PATH}/usr/sbin/
	## QNX libs
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/libhiddi.so*                  ${QNX_FS_PATH}/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/libm.so*                      ${QNX_FS_PATH}/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/libpps.so*                    ${QNX_FS_PATH}/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/libslog2.so*                  ${QNX_FS_PATH}/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/libmtouch-devi.so*            ${QNX_FS_PATH}/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/libcatalog.so*                ${QNX_FS_PATH}/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/libtracelog.so*               ${QNX_FS_PATH}/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/libmq.so*                 ${QNX_FS_PATH}/usr/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/libxml2.so*               ${QNX_FS_PATH}/usr/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/libiconv.so*              ${QNX_FS_PATH}/usr/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/libc++.so*                ${QNX_FS_PATH}/usr/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/librpc.so*                ${QNX_FS_PATH}/usr/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/libstringsa64.so*         ${QNX_FS_PATH}/usr/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/libaoi.so*                ${QNX_FS_PATH}/usr/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/libbz2.so*                ${QNX_FS_PATH}/usr/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/liblzma.so*               ${QNX_FS_PATH}/usr/lib/
ifeq ($(SOC),$(filter $(SOC), j721e j721s2 j784s4))
	######## screen package start ######
	##### Graphics support - apps
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/screencmd                     ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/bin/screeninfo                    ${QNX_FS_PATH}/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/sbin/screen                       ${QNX_FS_PATH}/sbin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/calib-touch               ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/events                    ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/egl-configs               ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/gles1-gears               ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/gles1-vsync               ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/gles2-gears               ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/gles3-gears               ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/screen-gles2-tools        ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/screenshot                ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/sharewin                  ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/sw-vsync                  ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/bin/win-vsync                 ${QNX_FS_PATH}/usr/bin/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/sbin/mtouch                   ${QNX_FS_PATH}/usr/sbin/
	##### Graphics base libraries
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/dll/libwfdcfg-sample.so       ${QNX_FS_PATH}/lib/dll/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/dll/screen-stdbuf.so          ${QNX_FS_PATH}/lib/dll/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/dll/screen-debug.so           ${QNX_FS_PATH}/lib/dll/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/dll/screen-gles1.so           ${QNX_FS_PATH}/lib/dll/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/dll/screen-gles2.so           ${QNX_FS_PATH}/lib/dll/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/dll/screen-gles2blt.so        ${QNX_FS_PATH}/lib/dll/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/dll/screen-sw.so              ${QNX_FS_PATH}/lib/dll/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/libmtouch-devi.so             ${QNX_FS_PATH}/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/libmtouch-fake.so             ${QNX_FS_PATH}/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/libmtouch-hid.so              ${QNX_FS_PATH}/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/libmtouch-inject.so           ${QNX_FS_PATH}/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/libgestures.so.1              ${QNX_FS_PATH}/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/libscrmem.so                  ${QNX_FS_PATH}/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/libmemobj.so                  ${QNX_FS_PATH}/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/libinputevents.so.1           ${QNX_FS_PATH}/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/libkalman.so.1                ${QNX_FS_PATH}/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/libmtouch-calib.so.1          ${QNX_FS_PATH}/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/libscrmem.so.1                ${QNX_FS_PATH}/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/lib/libmemobj.so.1                ${QNX_FS_PATH}/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/libCL.so                  ${QNX_FS_PATH}/usr/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/libfontconfig.so          ${QNX_FS_PATH}/usr/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/libfreetype.so            ${QNX_FS_PATH}/usr/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/libGLESv2.so.1            ${QNX_FS_PATH}/usr/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/libGLESv1_CL.so.1         ${QNX_FS_PATH}/usr/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/libGLESv1_CM.so.1         ${QNX_FS_PATH}/usr/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/libEGL.so.1               ${QNX_FS_PATH}/usr/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/libscreen.so.1            ${QNX_FS_PATH}/usr/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/libswblit.so.1            ${QNX_FS_PATH}/usr/lib/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/libWFD.so.1               ${QNX_FS_PATH}/usr/lib/
endif
ifeq ($(SOC),$(filter $(SOC), j721e))
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/graphics/dummy/libWFDdummy.so ${QNX_FS_PATH}/usr/lib/graphics/j721e/
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/graphics/j721e/*              ${QNX_FS_PATH}/usr/lib/graphics/j721e/
else ifeq ($(SOC),$(filter $(SOC), j721s2))
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/graphics/j721s2/*             ${QNX_FS_PATH}/usr/lib/graphics/j721s2/
else ifeq ($(SOC),$(filter $(SOC), j784s4))
	cp -rfL ${QNX_TARGET}/${QNX_ARCH}/usr/lib/graphics/j784s4/*             ${QNX_FS_PATH}/usr/lib/graphics/j784s4/
endif
	######## screen package end ######
	######## bsp ########
ifeq ($(SOC),$(filter $(SOC), j721e))
	cp -rfL ${QNX_BSP_PATH}/images/ifs-j721e-evm-ti.raw                 ${QNX_BOOT_PATH}/qnx-ifs
else ifeq ($(SOC),$(filter $(SOC), j7200))
	cp -rfL ${QNX_BSP_PATH}/images/ifs-j7200-evm-ti.raw                 ${QNX_BOOT_PATH}/qnx-ifs
else ifeq ($(SOC),$(filter $(SOC), j721s2))
	cp -rfL ${QNX_BSP_PATH}/images/ifs-j721s2-evm-ti.raw                ${QNX_BOOT_PATH}/qnx-ifs
else ifeq ($(SOC),$(filter $(SOC), j784s4))
	cp -rfL ${QNX_BSP_PATH}/images/ifs-j784s4-evm-ti.raw                ${QNX_BOOT_PATH}/qnx-ifs
else ifeq ($(SOC),$(filter $(SOC), j722s))
	cp -rfL ${QNX_BSP_PATH}/images/ifs-j722s-evm-ti.raw                 ${QNX_BOOT_PATH}/qnx-ifs
endif
	cp -rfL ${QNX_BSP_PATH}/images/*.raw                                ${QNX_FS_PATH}/version/images/
	cp -rfL ${QNX_BSP_PATH}/images/*.build                              ${QNX_FS_PATH}/version/images/
	cp -rfL ${QNX_BSP_PATH}/readme.txt                                  ${QNX_FS_PATH}/version/bsp_version_info.txt
	######## bsp end ########
	######## ti bins & libs ######
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/lib/libti-pdk.so              $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/lib/libti-pdk.so.1            $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/lib/libti-sciclient.so        $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/lib/libti-sciclient.so.1      $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/lib/libti-udmalld.so          $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/lib/libti-udmalld.so.1        $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/lib/libti-ipclld.so           $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/lib/libti-ipclld.so.1         $(QNX_FS_PATH)/tilib/
ifeq ($(SOC),$(filter $(SOC), j721e j7200 j721s2 j784s4))
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/lib/libti-enetlld.so          $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/lib/libti-enetlld.so.1        $(QNX_FS_PATH)/tilib/
endif
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/bin/tisci-mgr                 $(QNX_FS_PATH)/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/bin/tiipc-mgr                 $(QNX_FS_PATH)/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/bin/shmemallocator            $(QNX_FS_PATH)/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/bin/tiudma-mgr                $(QNX_FS_PATH)/tibin/
ifeq ($(SOC),$(filter $(SOC), j721s2 j784s4 j722s))
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/bin/ti-vpu-codec-mgr          $(QNX_FS_PATH)/tibin/
endif
ifeq ($(SOC),$(filter $(SOC), j721e j7200 j721s2 j784s4))
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/lib/dll/devnp-cpsw2g.so       $(QNX_FS_PATH)/tilib/
endif
ifeq ($(SOC),$(filter $(SOC), j721e))
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/bin/vxe_enc                   $(QNX_FS_PATH)/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/bin/vxd_dec                   $(QNX_FS_PATH)/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/lib/dll/devnp-cpsw9g.so       $(QNX_FS_PATH)/tilib/
else ifeq ($(SOC),$(filter $(SOC), j7200))
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/lib/dll/devnp-cpsw5g.so       $(QNX_FS_PATH)/tilib/
else ifeq ($(SOC),$(filter $(SOC), j721s2))
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/lib/dll/devnp-cpsw2g-main.so  $(QNX_FS_PATH)/tilib/
else ifeq ($(SOC),$(filter $(SOC), j784s4))
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/lib/dll/devnp-cpsw9g.so       $(QNX_FS_PATH)/tilib/
endif
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libtiipc-usr.so               $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libtiipc-usr.so.1             $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libtiudma-usr.so              $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libtiudma-usr.so.1            $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libsharedmemallocator.so      ${QNX_FS_PATH}/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libsharedmemallocator.so.1    ${QNX_FS_PATH}/tilib/
ifeq ($(SOC),$(filter $(SOC), j721e))
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libtimmenc.so                 $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libtimmenc.so.1               $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libvxe_enc-cli.so             $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libvxe_enc-cli.so.1           $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libomxcore_j7.so              $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libomxcore_j7.so.1            $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/lib/dll/omxil_comp_j7_enc.so          $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libomxil_j7_utility.so        $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libomxil_j7_utility.so.1      $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libtimmdec.so                 $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libtimmdec.so.1               $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libvxd_dec-cli.so             $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libvxd_dec-cli.so.1           $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/lib/dll/omxil_comp_j7_dec.so          $(QNX_FS_PATH)/tilib/
endif
ifeq ($(SOC),$(filter $(SOC), j721s2 j784s4 j722s))
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libti-vpucodec.so             $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libti-vpucodec.so.1           $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libvpu_usr_lib.so             $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libvpu_usr_lib.so.1           $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libomxcore_j7.so              $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libomxcore_j7.so.1            $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/lib/dll/omxil_comp_vpu_enc.so         $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libomxil_j7_utility.so        $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libomxil_j7_utility.so.1      $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libvpu_usr_lib.so             $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libvpu_usr_lib.so.1           $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libti-vpucodec.so             $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/lib/libti-vpucodec.so.1           $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/lib/dll/omxil_comp_vpu_dec.so         $(QNX_FS_PATH)/tilib/
	cp -rfL $(PSDK_QNX_PATH)/qnx/codec/vpu/firmware/wave521c_k3_codec_fw.bin    ${QNX_FS_PATH}/tilib/firmware/
endif
	######## ti pdk qnx test bin ######
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/ipc_test                      ${QNX_FS_PATH}/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/udma_memcpy_testapp           ${QNX_FS_PATH}/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/udma_crc_testapp              ${QNX_FS_PATH}/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/sciclient_app                 ${QNX_FS_PATH}/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/sciclient_clk_test            ${QNX_FS_PATH}/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/SharedMemoryAllocatorTestApp  ${QNX_FS_PATH}/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/osal_sem_testapp              ${QNX_FS_PATH}/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/gpio                          ${QNX_FS_PATH}/tibin/
ifneq ($(SOC),$(filter $(SOC), j722s))
	# NOTE: This will be re-enabled at a later point.
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/decrypt_app                   ${QNX_FS_PATH}/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/ptp_test                      ${QNX_FS_PATH}/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/cpsw_test                     ${QNX_FS_PATH}/tibin/
endif
ifeq ($(SOC),$(filter $(SOC), j721e))
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/enc_cli_ut            $(QNX_FS_PATH)/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/timmlibenc            $(QNX_FS_PATH)/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/omxil_video_enc       $(QNX_FS_PATH)/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/omxil_video_enc_p     $(QNX_FS_PATH)/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/timmlibdec            $(QNX_FS_PATH)/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/omxil_video_dec       $(QNX_FS_PATH)/tibin/
endif
ifeq ($(SOC),$(filter $(SOC), j721s2 j784s4 j722s))
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/vpu_decoder_test      ${QNX_FS_PATH}/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/vpu_encoder_test      ${QNX_FS_PATH}/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/vpu_multi_inst_test   ${QNX_FS_PATH}/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/omxil_video_enc       $(QNX_FS_PATH)/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/omxil_video_enc_p     $(QNX_FS_PATH)/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/usr/bin/omxil_video_dec       $(QNX_FS_PATH)/tibin/
endif
	######## ti pdk qnx test bin end ######
	######## qnx codec test files ######
ifeq ($(SOC),$(filter $(SOC), j721e j721s2 j784s4 j722s))
	cp -rfL $(PSDK_QNX_PATH)/qnx/codec/codec_clips/*                                        ${QNX_FS_PATH}/codec_test/
	rm -rf ${QNX_FS_PATH}/codec_test/.git
endif
ifeq ($(SOC),$(filter $(SOC), j721e))
	cp -rfL $(PSDK_QNX_PATH)/qnx/codec/img/qnx/OpenMAXIL/test/enc/encoder_parameters.conf   $(QNX_FS_PATH)/codec_test/
endif
ifeq ($(SOC),$(filter $(SOC), j721s2 j784s4 j722s))
	cp -rfL $(PSDK_QNX_PATH)/qnx/codec/vpu/test_files/cfg                                   ${QNX_FS_PATH}/codec_test/
	cp -rfL $(PSDK_QNX_PATH)/qnx/codec/vpu/OpenMAXIL/test/enc/encoder_parameters.conf       $(QNX_FS_PATH)/codec_test/
endif
	######## qnx codec test files end ######
	######## ti qnx utils ######
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/bin/ipc_trace_logger          $(QNX_FS_PATH)/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/bin/k3conf                    $(QNX_FS_PATH)/tibin/
	cp -rfL $(PSDK_QNX_STAGE)/${QNX_ARCH}/bin/ddr_bw                    $(QNX_FS_PATH)/tibin/
	######## ti qnx utils end ######
	#### copy scripts & extra ####
ifeq ($(SOC),$(filter $(SOC), j721e))
	# This update the user.sh script which is invoked from the .build file, as well as overwriting
	# the graphics.conf that is use
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/user_j721e.sh                      ${QNX_FS_PATH}/scripts/user.sh
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/user__dss_on_a72.sh                ${QNX_FS_PATH}/scripts/
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/user__dss_on_r5.sh                 ${QNX_FS_PATH}/scripts/
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/screen/usr/lib/graphics/j721e/*    ${QNX_FS_PATH}/usr/lib/graphics/j721e/
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/start_qconn_with_usb-asix.sh       ${QNX_FS_PATH}/scripts/
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/start_qconn_with_devnp-cpsw2g.sh   ${QNX_FS_PATH}/scripts/
else ifeq ($(SOC),$(filter $(SOC), j7200))
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/user_j7200.sh                      ${QNX_FS_PATH}/scripts/user.sh
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/start_qconn_with_usb-asix.sh       ${QNX_FS_PATH}/scripts/
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/start_qconn_with_devnp-cpsw2g.sh   ${QNX_FS_PATH}/scripts/
else ifeq ($(SOC),$(filter $(SOC), j721s2))
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/user_j721s2.sh                     ${QNX_FS_PATH}/scripts/user.sh
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/start_qconn_with_usb-asix.sh       ${QNX_FS_PATH}/scripts/
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/start_qconn_with_devnp-cpsw2g.sh   ${QNX_FS_PATH}/scripts/
else ifeq ($(SOC),$(filter $(SOC), j784s4))
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/user_j784s4.sh                     ${QNX_FS_PATH}/scripts/user.sh
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/start_qconn_with_usb-asix.sh       ${QNX_FS_PATH}/scripts/
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/start_qconn_with_devnp-cpsw2g.sh   ${QNX_FS_PATH}/scripts/
else ifeq ($(SOC),$(filter $(SOC), j722s))
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/user_j722s.sh                      ${QNX_FS_PATH}/scripts/user.sh
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/start_qconn_with_usb-asix.sh       ${QNX_FS_PATH}/scripts/
endif
	#### copy scripts & extra - end ####


copy_spl_uboot:
	###### PSDKLA SPL Boot Binaries #########
ifeq ($(SOC),$(filter $(SOC), j721e))
	cp ${LINUX_FS_BOOT_PATH}/tiboot3.bin                                ${QNX_BOOT_PATH}/
	cp ${LINUX_FS_BOOT_PATH}/sysfw.itb                                  ${QNX_BOOT_PATH}/
	cp ${LINUX_FS_BOOT_PATH}/tispl.bin                                  ${QNX_BOOT_PATH}/
	cp ${LINUX_FS_BOOT_PATH}/u-boot.img                                 ${QNX_BOOT_PATH}/
else ifeq ($(SOC),$(filter $(SOC), j7200 j721s2 j784s4 j722s))
	cp ${LINUX_FS_BOOT_PATH}/tiboot3.bin                                ${QNX_BOOT_PATH}/
	cp ${LINUX_FS_BOOT_PATH}/tispl.bin                                  ${QNX_BOOT_PATH}/
	cp ${LINUX_FS_BOOT_PATH}/u-boot.img                                 ${QNX_BOOT_PATH}/
endif
	# Add the uEnv.txt to load the QNX-IFS
ifeq ($(SOC),$(filter $(SOC), j721e))
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/u-boot/uEnv_j721e.txt          ${QNX_BOOT_PATH}/uEnv.txt
else ifeq ($(SOC),$(filter $(SOC), j7200))
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/u-boot/uEnv_j7200.txt          ${QNX_BOOT_PATH}/uEnv.txt
else ifeq ($(SOC),$(filter $(SOC), j721s2))
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/u-boot/uEnv_j721s2.txt         ${QNX_BOOT_PATH}/uEnv.txt
else ifeq ($(SOC),$(filter $(SOC), j784s4))
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/u-boot/uEnv_j784s4.txt         ${QNX_BOOT_PATH}/uEnv.txt
else ifeq ($(SOC),$(filter $(SOC), j722s))
	cp -rfL ${PSDK_QNX_PATH}/qnx/scripts/u-boot/uEnv_j722s.txt          ${QNX_BOOT_PATH}/uEnv.txt
endif
