.PHONY: prep_sdk spl_create_copy
.PHONY: spl_echo_create_copy
.PHONY: vision_apps_create_copy vision_apps_hs_create_copy
.PHONY: sbl_echo_create_copy sbl_echo_hs_create_copy
.PHONY: tar_built_filesystems
.PHONY: create_all_platform_packages

ifeq ($(QNX_SDP_VERSION),$(filter $(QNX_SDP_VERSION), 710))
.PHONY: spl_ethfw_create_copy
.PHONY: sbl_ethfw_create_copy sbl_ethfw_hs_create_copy
endif

FS_DIR_SPL_BOOTFS?=$(PSDK_QNX_PATH)/fs_dir/bootfs_spl
FS_DIR_SPL_VISION_APPS_BOOTFS?=$(PSDK_QNX_PATH)/fs_dir/bootfs_spl_vision_apps
FS_DIR_QNXFS?=$(PSDK_QNX_PATH)/fs_dir/qnxfs
FS_DIR_VISION_APPS_QNXFS?=$(PSDK_QNX_PATH)/fs_dir/qnxfs_vision_apps
FS_DIR_SBL_VISION_APPS_BOOTFS?=$(PSDK_QNX_PATH)/fs_dir/bootfs_sbl_vision_apps
FS_DIR_SBL_VISION_APPS_HS_BOOTFS?=$(PSDK_QNX_PATH)/fs_dir/bootfs_sbl_vision_apps_hs
FS_DIR_SBL_ETHFW_BOOTFS?=$(PSDK_QNX_PATH)/fs_dir/bootfs_sbl_ethfw
FS_DIR_SBL_ETHFW_HS_BOOTFS?=$(PSDK_QNX_PATH)/fs_dir/bootfs_sbl_ethfw_hs
FS_DIR_ETHFW_ROOTFS?=$(PSDK_QNX_PATH)/fs_dir/rootfs_ethfw
FS_DIR_SBL_ECHO_BOOTFS?=$(PSDK_QNX_PATH)/fs_dir/bootfs_sbl_echo
FS_DIR_SBL_ECHO_HS_BOOTFS?=$(PSDK_QNX_PATH)/fs_dir/bootfs_sbl_echo_hs
FS_DIR_ECHO_ROOTFS?=$(PSDK_QNX_PATH)/fs_dir/rootfs_echo
FS_DIR_VISION_APPS_ROOTFS?=$(PSDK_QNX_PATH)/fs_dir/rootfs_vision_apps

FS_TAR_SPL_BOOTFS?=$(PSDK_QNX_PATH)/fs_tar/bootfs_spl_$(PSDK_QNX_TAGID).tar.gz
FS_TAR_SPL_VISION_APPS_BOOTFS?=$(PSDK_QNX_PATH)/fs_tar/bootfs_spl_vision_apps_$(PSDK_QNX_TAGID).tar.gz
FS_TAR_QNXFS?=$(PSDK_QNX_PATH)/fs_tar/qnxfs_$(PSDK_QNX_TAGID).tar.gz
FS_TAR_VISION_APPS_QNXFS?=$(PSDK_QNX_PATH)/fs_tar/qnxfs_vision_apps_$(PSDK_QNX_TAGID).tar.gz
FS_TAR_SBL_VISION_APPS_BOOTFS?=$(PSDK_QNX_PATH)/fs_tar/bootfs_sbl_vision_apps_$(PSDK_QNX_TAGID).tar.gz
FS_TAR_SBL_VISION_APPS_HS_BOOTFS?=$(PSDK_QNX_PATH)/fs_tar/bootfs_sbl_vision_apps_hs_$(PSDK_QNX_TAGID).tar.gz
FS_TAR_SBL_ETHFW_BOOTFS?=$(PSDK_QNX_PATH)/fs_tar/bootfs_sbl_ethfw_$(PSDK_QNX_TAGID).tar.gz
FS_TAR_SBL_ETHFW_HS_BOOTFS?=$(PSDK_QNX_PATH)/fs_tar/bootfs_sbl_ethfw_hs_$(PSDK_QNX_TAGID).tar.gz
FS_TAR_SBL_ETHFW_ROOTFS?=$(PSDK_QNX_PATH)/fs_tar/rootfs_ethfw_$(PSDK_QNX_TAGID).tar.xz
FS_TAR_SBL_ECHO_BOOTFS?=$(PSDK_QNX_PATH)/fs_tar/bootfs_sbl_echo_$(PSDK_QNX_TAGID).tar.gz
FS_TAR_SBL_ECHO_HS_BOOTFS?=$(PSDK_QNX_PATH)/fs_tar/bootfs_sbl_echo_hs_$(PSDK_QNX_TAGID).tar.gz
FS_TAR_SBL_ECHO_ROOTFS?=$(PSDK_QNX_PATH)/fs_tar/rootfs_echo_$(PSDK_QNX_TAGID).tar.xz
FS_TAR_VISION_APPS_ROOTFS?=$(PSDK_QNX_PATH)/fs_tar/rootfs_vision_apps_$(PSDK_QNX_TAGID).tar.xz

NUM_PROCS?=$(shell nproc)
NUM_THREADS?=$(shell expr $(NUM_PROCS) / 2 - 1)

ifeq ($(SOC),$(filter $(SOC), j7200))
SR_TYPE?=.sr2.0
endif
ifeq ($(SOC),$(filter $(SOC), j721e))
SR_TYPE?=.sr1.1
endif
ifeq ($(SOC),$(filter $(SOC), j721s2 j784s4 j722s))
SR_TYPE?=
endif

FIRMWARE_OS_TYPE?=freertos
VISION_APPS_SBL_OUT_PATH?=$(VISION_APPS_PATH)/out/sbl_bootfiles
QNX_SBL_ETHFW_PATH?=$(PSDK_QNX_PATH)/bootfs_sbl_ethfw_$(FIRMWARE_OS_TYPE)
QNX_SBL_ETHFW_HS_PATH?=$(PSDK_QNX_PATH)/bootfs_sbl_ethfw_hs_$(FIRMWARE_OS_TYPE)
QNX_SBL_ECHO_TEST_PATH?=$(PSDK_QNX_PATH)/bootfs_sbl_echotest_$(FIRMWARE_OS_TYPE)
QNX_SBL_ECHO_TEST_HS_PATH?=$(PSDK_QNX_PATH)/bootfs_sbl_echotest_hs_$(FIRMWARE_OS_TYPE)
QNX_SPL_UBOOT_ETHFW_PATH?=$(PSDK_QNX_PATH)/rootfs_ethfw_$(FIRMWARE_OS_TYPE)
QNX_SPL_UBOOT_ETHFW_PATH_HS?=$(PSDK_QNX_PATH)/rootfs_ethfw_hs_$(FIRMWARE_OS_TYPE)
QNX_SPL_UBOOT_ECHO_TEST_PATH?=$(PSDK_QNX_PATH)/rootfs_echotest_$(FIRMWARE_OS_TYPE)
QNX_SPL_UBOOT_ECHO_TEST_PATH_HS?=$(PSDK_QNX_PATH)/rootfs_echotest_hs_$(FIRMWARE_OS_TYPE)

prep_sdk:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 scrub -j
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 all -j
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 qnx_fs_create -j

ifeq ($(QNX_SDP_VERSION),$(filter $(QNX_SDP_VERSION), 710))
spl_ethfw_create_copy:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 ethfw_clean -j
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 ethfw -j
	mkdir -p $(FS_DIR_ETHFW_ROOTFS)/lib/firmware/
	cp -rfv $(QNX_SPL_UBOOT_ETHFW_PATH)/lib/firmware/*                              $(FS_DIR_ETHFW_ROOTFS)/lib/firmware/
endif

spl_echo_create_copy:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 echo_test_fw_clean -j
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 echo_test_fw -j
	mkdir -p $(FS_DIR_ECHO_ROOTFS)/lib/firmware/
	cp -rfv $(QNX_SPL_UBOOT_ECHO_TEST_PATH)/lib/firmware/*                          $(FS_DIR_ECHO_ROOTFS)/lib/firmware/

spl_create_copy:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 qnx_fs_copy_spl_uboot -j
	mkdir -p $(FS_DIR_SPL_BOOTFS)/
	mkdir -p $(FS_DIR_QNXFS)/
	cp -rfL $(PSDK_QNX_PATH)/bootfs/*                                               $(FS_DIR_SPL_BOOTFS)/
	cp -rfL $(PSDK_QNX_PATH)/qnxfs/*                                                $(FS_DIR_QNXFS)/
	mkdir -p $(FS_DIR_QNXFS)/version/images
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak           spl_echo_create_copy -j
ifeq ($(QNX_SDP_VERSION),$(filter $(QNX_SDP_VERSION), 710))
ifneq ($(SOC),$(filter $(SOC), j721s2 j722s))
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak           spl_ethfw_create_copy -j
endif
endif

vision_apps_create_copy:
	cd $(SDK_BUILDER_PATH) && \
	$(MAKE) sdk_scrub             -j                                                BUILD_QNX_MPU=yes BUILD_LINUX_MPU=no BUILD_EMULATION_MODE=no && \
	$(MAKE) sdk                   -j                                                BUILD_QNX_MPU=yes BUILD_LINUX_MPU=no BUILD_EMULATION_MODE=no && \
	$(MAKE) qnx_fs_create         -j                                                BUILD_QNX_MPU=yes BUILD_LINUX_MPU=no BUILD_EMULATION_MODE=no && \
	$(MAKE) qnx_fs_copy_spl_uboot -j                                                BUILD_QNX_MPU=yes BUILD_LINUX_MPU=no BUILD_EMULATION_MODE=no && \
	$(MAKE) qnx_fs_install        -j                                                BUILD_QNX_MPU=yes BUILD_LINUX_MPU=no BUILD_EMULATION_MODE=no && \
	$(MAKE) sbl_bootimage                                                           BUILD_QNX_MPU=yes BUILD_LINUX_MPU=no BUILD_EMULATION_MODE=no
	mkdir -p $(FS_DIR_SPL_VISION_APPS_BOOTFS)/
	mkdir -p $(FS_DIR_SBL_VISION_APPS_BOOTFS)/
	mkdir -p $(FS_DIR_SBL_VISION_APPS_HS_BOOTFS)/
	mkdir -p $(FS_DIR_VISION_APPS_ROOTFS)/lib/firmware/
	cp -rfL $(PSDK_QNX_PATH)/bootfs/*                                               $(FS_DIR_SPL_VISION_APPS_BOOTFS)/
	cp -rfL $(PSDK_QNX_PATH)/qnxfs/*                                                $(FS_DIR_VISION_APPS_QNXFS)/
	cp -rfL $(VISION_APPS_SBL_OUT_PATH)/tiboot3.bin                                 $(FS_DIR_SBL_VISION_APPS_BOOTFS)/
	cp -rfL $(VISION_APPS_SBL_OUT_PATH)/tifs.bin                                    $(FS_DIR_SBL_VISION_APPS_BOOTFS)/
	cp -rfL $(VISION_APPS_SBL_OUT_PATH)/app                                         $(FS_DIR_SBL_VISION_APPS_BOOTFS)/
ifneq ($(SOC),$(filter $(SOC), j722s))
	cp -rfL $(VISION_APPS_SBL_OUT_PATH)/lateapp*                                    $(FS_DIR_SBL_VISION_APPS_BOOTFS)/
	cp -rfL $(VISION_APPS_SBL_OUT_PATH)/atf_optee.appimage                          $(FS_DIR_SBL_VISION_APPS_BOOTFS)/
	cp -rfL $(VISION_APPS_SBL_OUT_PATH)/ifs_qnx.appimage                            $(FS_DIR_SBL_VISION_APPS_BOOTFS)/
endif
	cp -rfv $(PSDK_QNX_PATH)/rootfs/*                                               $(FS_DIR_VISION_APPS_ROOTFS)/

vision_apps_hs_create_copy:
	cd $(SDK_BUILDER_PATH) && \
	$(MAKE) sdk_scrub             -j                                                BUILD_QNX_MPU=yes BUILD_LINUX_MPU=no BUILD_EMULATION_MODE=no && \
	$(MAKE) sdk                   -j                                                BUILD_QNX_MPU=yes BUILD_LINUX_MPU=no BUILD_EMULATION_MODE=no && \
	$(MAKE) qnx_fs_create         -j                                                BUILD_QNX_MPU=yes BUILD_LINUX_MPU=no BUILD_EMULATION_MODE=no && \
	$(MAKE) qnx_fs_copy_spl_uboot -j                                                BUILD_QNX_MPU=yes BUILD_LINUX_MPU=no BUILD_EMULATION_MODE=no && \
	$(MAKE) qnx_fs_install        -j                                                BUILD_QNX_MPU=yes BUILD_LINUX_MPU=no BUILD_EMULATION_MODE=no && \
	$(MAKE) sbl_bootimage_hs                                                        BUILD_QNX_MPU=yes BUILD_LINUX_MPU=no BUILD_EMULATION_MODE=no
	mkdir -p $(FS_DIR_SBL_VISION_APPS_HS_BOOTFS)/
	cp -rfL $(VISION_APPS_SBL_OUT_PATH)/tiboot3.bin.signed                          $(FS_DIR_SBL_VISION_APPS_HS_BOOTFS)/tiboot3.bin
	cp -rfL $(VISION_APPS_SBL_OUT_PATH)/tifs.bin.signed                             $(FS_DIR_SBL_VISION_APPS_HS_BOOTFS)/tifs.bin
	cp -rfL $(VISION_APPS_SBL_OUT_PATH)/app.signed                                  $(FS_DIR_SBL_VISION_APPS_HS_BOOTFS)/app
ifneq ($(SOC),$(filter $(SOC), j722s))
	cp -rfL $(VISION_APPS_SBL_OUT_PATH)/lateapp1.signed                             $(FS_DIR_SBL_VISION_APPS_HS_BOOTFS)/lateapp1
	cp -rfL $(VISION_APPS_SBL_OUT_PATH)/lateapp2.signed                             $(FS_DIR_SBL_VISION_APPS_HS_BOOTFS)/lateapp2
	cp -rfL $(VISION_APPS_SBL_OUT_PATH)/atf_optee.appimage.signed                   $(FS_DIR_SBL_VISION_APPS_HS_BOOTFS)/atf_optee.appimage
	cp -rfL $(VISION_APPS_SBL_OUT_PATH)/ifs_qnx.appimage.signed                     $(FS_DIR_SBL_VISION_APPS_HS_BOOTFS)/ifs_qnx.appimage
endif

ifeq ($(QNX_SDP_VERSION),$(filter $(QNX_SDP_VERSION), 710))
sbl_ethfw_create_copy:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 scrub -j
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 all -j
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 qnx_fs_create -j
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 sbl_ethfw
	mkdir -p $(FS_DIR_SBL_ETHFW_BOOTFS)/
	cp -rfL $(QNX_SBL_ETHFW_PATH)/tiboot3.bin                                       $(FS_DIR_SBL_ETHFW_BOOTFS)/
	cp -rfL $(QNX_SBL_ETHFW_PATH)/tifs.bin                                          $(FS_DIR_SBL_ETHFW_BOOTFS)/
	cp -rfL $(QNX_SBL_ETHFW_PATH)/app                                               $(FS_DIR_SBL_ETHFW_BOOTFS)/
ifneq ($(SOC),$(filter $(SOC), j722s))
	cp -rfL $(QNX_SBL_ETHFW_PATH)/lateapp*                                          $(FS_DIR_SBL_ETHFW_BOOTFS)/
	cp -rfL $(QNX_SBL_ETHFW_PATH)/atf_optee.appimage                                $(FS_DIR_SBL_ETHFW_BOOTFS)/
	cp -rfL $(QNX_SBL_ETHFW_PATH)/ifs_qnx.appimage                                  $(FS_DIR_SBL_ETHFW_BOOTFS)/
endif

sbl_ethfw_hs_create_copy:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 scrub -j
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 all -j
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 qnx_fs_create -j
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 sbl_ethfw_hs
	mkdir -p $(FS_DIR_SBL_ETHFW_HS_BOOTFS)/
	cp -rfL $(QNX_SBL_ETHFW_HS_PATH)/tiboot3.bin                                    $(FS_DIR_SBL_ETHFW_HS_BOOTFS)/
	cp -rfL $(QNX_SBL_ETHFW_HS_PATH)/tifs.bin$(SR_TYPE)                             $(FS_DIR_SBL_ETHFW_HS_BOOTFS)/tifs.bin
	cp -rfL $(QNX_SBL_ETHFW_HS_PATH)/app                                            $(FS_DIR_SBL_ETHFW_HS_BOOTFS)/
ifneq ($(SOC),$(filter $(SOC), j722s))
	cp -rfL $(QNX_SBL_ETHFW_HS_PATH)/lateapp*                                       $(FS_DIR_SBL_ETHFW_HS_BOOTFS)/
	cp -rfL $(QNX_SBL_ETHFW_HS_PATH)/atf_optee.appimage                             $(FS_DIR_SBL_ETHFW_HS_BOOTFS)/
	cp -rfL $(QNX_SBL_ETHFW_HS_PATH)/ifs_qnx.appimage                               $(FS_DIR_SBL_ETHFW_HS_BOOTFS)/
endif
endif

sbl_echo_create_copy:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 scrub -j
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 all -j
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 qnx_fs_create -j
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 sbl_echo_test
	mkdir -p $(FS_DIR_SBL_ECHO_BOOTFS)/
	cp -rfL $(QNX_SBL_ECHO_TEST_PATH)/tiboot3.bin                                   $(FS_DIR_SBL_ECHO_BOOTFS)/
	cp -rfL $(QNX_SBL_ECHO_TEST_PATH)/tifs.bin                                      $(FS_DIR_SBL_ECHO_BOOTFS)/
	cp -rfL $(QNX_SBL_ECHO_TEST_PATH)/app                                           $(FS_DIR_SBL_ECHO_BOOTFS)/
ifneq ($(SOC),$(filter $(SOC), j722s))
	cp -rfL $(QNX_SBL_ECHO_TEST_PATH)/lateapp*                                      $(FS_DIR_SBL_ECHO_BOOTFS)/
	cp -rfL $(QNX_SBL_ECHO_TEST_PATH)/atf_optee.appimage                            $(FS_DIR_SBL_ECHO_BOOTFS)/
	cp -rfL $(QNX_SBL_ECHO_TEST_PATH)/ifs_qnx.appimage                              $(FS_DIR_SBL_ECHO_BOOTFS)/
endif

sbl_echo_hs_create_copy:
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 scrub -j
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 all -j
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 qnx_fs_create -j
	$(MAKE) -C $(PSDK_QNX_PATH)/qnx                                                 sbl_echo_test_hs
	mkdir -p $(FS_DIR_SBL_ECHO_HS_BOOTFS)/
	cp -rfL $(QNX_SBL_ECHO_TEST_HS_PATH)/tiboot3.bin                                $(FS_DIR_SBL_ECHO_HS_BOOTFS)/
	cp -rfL $(QNX_SBL_ECHO_TEST_HS_PATH)/tifs.bin$(SR_TYPE)                         $(FS_DIR_SBL_ECHO_HS_BOOTFS)/tifs.bin
	cp -rfL $(QNX_SBL_ECHO_TEST_HS_PATH)/app                                        $(FS_DIR_SBL_ECHO_HS_BOOTFS)/
ifneq ($(SOC),$(filter $(SOC), j722s))
	cp -rfL $(QNX_SBL_ECHO_TEST_HS_PATH)/lateapp*                                   $(FS_DIR_SBL_ECHO_HS_BOOTFS)/
	cp -rfL $(QNX_SBL_ECHO_TEST_HS_PATH)/atf_optee.appimage                         $(FS_DIR_SBL_ECHO_HS_BOOTFS)/
	cp -rfL $(QNX_SBL_ECHO_TEST_HS_PATH)/ifs_qnx.appimage                           $(FS_DIR_SBL_ECHO_HS_BOOTFS)/
endif


tar_built_filesystems:
	mkdir -p $(PSDK_QNX_PATH)/fs_tar/
	if [ -f $(FS_TAR_SPL_BOOTFS) ];                                                 then rm -rf $(FS_TAR_SPL_BOOTFS); fi
	if [ -f $(FS_TAR_QNXFS) ];                                                      then rm -rf $(FS_TAR_QNXFS); fi
ifeq ($(QNX_SDP_VERSION),$(filter $(QNX_SDP_VERSION), 710))
ifneq ($(SOC),$(filter $(SOC), j721s2 j722s))
	if [ -f $(FS_TAR_SBL_ETHFW_BOOTFS) ];                                           then rm -rf $(FS_TAR_SBL_ETHFW_BOOTFS); fi
	if [ -f $(FS_TAR_SBL_ETHFW_HS_BOOTFS) ];                                        then rm -rf $(FS_TAR_SBL_ETHFW_HS_BOOTFS); fi
	if [ -f $(FS_TAR_SBL_ETHFW_ROOTFS) ];                                           then rm -rf $(FS_TAR_SBL_ETHFW_ROOTFS); fi
endif
endif
	if [ -f $(FS_TAR_SBL_ECHO_BOOTFS) ];                                            then rm -rf $(FS_TAR_SBL_ECHO_BOOTFS); fi
	if [ -f $(FS_TAR_SBL_ECHO_HS_BOOTFS) ];                                         then rm -rf $(FS_TAR_SBL_ECHO_HS_BOOTFS); fi
	if [ -f $(FS_TAR_SBL_ECHO_ROOTFS) ];                                            then rm -rf $(FS_TAR_SBL_ECHO_ROOTFS); fi
ifneq ($(SOC),$(filter $(SOC), j7200))
	if [ -f $(FS_TAR_SPL_VISION_APPS_BOOTFS) ];                                     then rm -rf $(FS_TAR_SPL_VISION_APPS_BOOTFS); fi
	if [ -f $(FS_TAR_VISION_APPS_QNXFS) ];                                          then rm -rf $(FS_TAR_VISION_APPS_QNXFS); fi
	if [ -f $(FS_TAR_SBL_VISION_APPS_BOOTFS) ];                                     then rm -rf $(FS_TAR_SBL_VISION_APPS_BOOTFS); fi
	if [ -f $(FS_TAR_SBL_VISION_APPS_HS_BOOTFS) ];                                  then rm -rf $(FS_TAR_SBL_VISION_APPS_HS_BOOTFS); fi
	if [ -f $(FS_TAR_VISION_APPS_ROOTFS) ];                                         then rm -rf $(FS_TAR_VISION_APPS_ROOTFS); fi
endif

	if [ -d $(FS_DIR_QNXFS) ];                                                      then cd $(FS_DIR_QNXFS);                        tar -zcvf $(FS_TAR_QNXFS) .; cd -; fi
	if [ -d $(FS_DIR_SPL_BOOTFS) ];                                                 then cd $(FS_DIR_SPL_BOOTFS);                   tar -zcvf $(FS_TAR_SPL_BOOTFS) .; cd -; fi
ifeq ($(QNX_SDP_VERSION),$(filter $(QNX_SDP_VERSION), 710))
ifneq ($(SOC),$(filter $(SOC), j721s2 j722s))
	if [ -d $(FS_DIR_SBL_ETHFW_BOOTFS) ];                                           then cd $(FS_DIR_SBL_ETHFW_BOOTFS);             tar -zcvf $(FS_TAR_SBL_ETHFW_BOOTFS) .; cd -; fi
	if [ -d $(FS_DIR_SBL_ETHFW_HS_BOOTFS) ];                                        then cd $(FS_DIR_SBL_ETHFW_HS_BOOTFS);          tar -zcvf $(FS_TAR_SBL_ETHFW_HS_BOOTFS) .; cd -; fi
	if [ -d $(FS_DIR_ETHFW_ROOTFS) ];                                               then cd $(FS_DIR_ETHFW_ROOTFS);                 tar -pczvf $(FS_TAR_SBL_ETHFW_ROOTFS) .; cd -; fi
endif
endif
	if [ -d $(FS_DIR_SBL_ECHO_BOOTFS) ];                                            then cd $(FS_DIR_SBL_ECHO_BOOTFS);              tar -zcvf $(FS_TAR_SBL_ECHO_BOOTFS) .; cd -; fi
	if [ -d $(FS_DIR_SBL_ECHO_HS_BOOTFS) ];                                         then cd $(FS_DIR_SBL_ECHO_HS_BOOTFS);           tar -zcvf $(FS_TAR_SBL_ECHO_HS_BOOTFS) .; cd -; fi
	if [ -d $(FS_DIR_ECHO_ROOTFS) ];                                                then cd $(FS_DIR_ECHO_ROOTFS);                  tar -pczvf $(FS_TAR_SBL_ECHO_ROOTFS) .; cd -; fi
ifneq ($(SOC),$(filter $(SOC), j7200))
	if [ -d $(FS_DIR_SPL_VISION_APPS_BOOTFS) ];                                     then cd $(FS_DIR_SPL_VISION_APPS_BOOTFS);       tar -zcvf $(FS_TAR_SPL_VISION_APPS_BOOTFS) .; cd -; fi
	if [ -d $(FS_DIR_VISION_APPS_QNXFS) ];                                          then cd $(FS_DIR_VISION_APPS_QNXFS);            tar -zcvf $(FS_TAR_VISION_APPS_QNXFS) .; cd -; fi
	if [ -d $(FS_DIR_SBL_VISION_APPS_BOOTFS) ];                                     then cd $(FS_DIR_SBL_VISION_APPS_BOOTFS);       tar -zcvf $(FS_TAR_SBL_VISION_APPS_BOOTFS) .; cd -; fi
	if [ -d $(FS_DIR_SBL_VISION_APPS_HS_BOOTFS) ];                                  then cd $(FS_DIR_SBL_VISION_APPS_HS_BOOTFS);    tar -zcvf $(FS_TAR_SBL_VISION_APPS_HS_BOOTFS) .; cd -; fi
	if [ -d $(FS_DIR_VISION_APPS_ROOTFS) ];                                         then cd $(FS_DIR_VISION_APPS_ROOTFS);           tar -pczvf $(FS_TAR_VISION_APPS_ROOTFS) .; cd -; fi
endif

create_all_platform_packages:
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak           prep_sdk
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak           qnx_spl_create_copy -j
ifeq ($(QNX_SDP_VERSION),$(filter $(QNX_SDP_VERSION), 710))
ifneq ($(SOC),$(filter $(SOC), j721s2 j722s))
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak           sbl_ethfw_create_copy -j
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak           sbl_ethfw_hs_create_copy
endif
endif
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak           sbl_echo_create_copy -j
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak           sbl_echo_hs_create_copy
ifneq ($(SOC),$(filter $(SOC), j7200))
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak           vision_apps_create_copy
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak           vision_apps_hs_create_copy
endif
	$(MAKE) -f $(PSDK_QNX_PATH)/qnx/scripts/automation/qnx_automation.mak           tar_built_filesystems
