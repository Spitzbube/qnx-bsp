#
#   Copyright (c) 2021-23, Texas Instruments Incorporated
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions
#   are met:
#
#   *  Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#
#   *  Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#
#   *  Neither the name of Texas Instruments Incorporated nor the names of
#      its contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
#   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
#   EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

NAME = vpu_encoder_test
INSTALLDIR = usr/bin
USEFILE=$(PROJECT_ROOT)/vpu_encoder_test.use

define PINFO
PINFO DESCRIPTION=Wave VPU Encoder test
PINFO STATE=Experimental
PINFO VERSION_REL=$(PSDK_QNX_VERSION_REL)
PINFO TAGID=$(PSDK_QNX_TAGID)
endef

include $(PSDK_QNX_PATH)/qnx/psdk_qnx_build.mk
CCOPTS += -DUSE_SINGLE_THREAD
CCOPTS += -DTEST_ENCODER
CCOPTS += -DCODEC_USE_HIGHMEM
CCOPTS += -w

# Add extra include path
EXTRA_INCVPATH += $(PDK_INSTALL_PATH)
EXTRA_INCVPATH += $(PSDK_QNX_PATH)/qnx/codec/vpu/tivpucodec/vpulib/public
EXTRA_INCVPATH += $(PROJECT_ROOT)/../common/component
EXTRA_INCVPATH += $(PROJECT_ROOT)/../common/component_encoder
EXTRA_INCVPATH += $(PROJECT_ROOT)/../common/helper
EXTRA_INCVPATH += $(PROJECT_ROOT)/../common/helper/misc

# Add source path
EXTRA_SRCVPATH += $(PROJECT_ROOT)/../common/component
EXTRA_SRCVPATH += $(PROJECT_ROOT)/../common/component_encoder
EXTRA_SRCVPATH += $(PROJECT_ROOT)/../common/helper
EXTRA_SRCVPATH += $(PROJECT_ROOT)/../common/helper/misc
EXTRA_SRCVPATH += $(PROJECT_ROOT)/../common/helper/comparator
EXTRA_SRCVPATH += $(PROJECT_ROOT)/../common/helper/yuv
EXTRA_SRCVPATH += $(PROJECT_ROOT)/../common/helper/bitstream

LIBS = c

EXTRA_LIBVPATH += $(PSDK_QNX_PATH)/qnx/pdk_libs/pdk/aarch64/so.le/
EXTRA_LIBVPATH += $(PSDK_QNX_PATH)/qnx/pdk_libs/sciclient/aarch64/so.le/
EXTRA_LIBVPATH += $(PSDK_QNX_PATH)/qnx/codec/vpu/tivpucodec/vpulib/aarch64/so.le/
EXTRA_LIBVPATH += $(PSDK_QNX_PATH)/qnx/sharedmemallocator/usr/aarch64/so.le/

LIBS += ti-pdk
LIBS += ti-sciclient
LIBS += ti-vpucodec
LIBS += sharedmemallocator

include $(MKFILES_ROOT)/qtargets.mk
