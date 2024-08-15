#
#   Copyright (c) 2022-23, Texas Instruments Incorporated
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

NAME=omxil_comp_vpu_enc
INSTALLDIR=lib/dll

define PINFO
PINFO DESCRIPTION=OMX IL VPU encoder component
PINFO STATE=Experimental
PINFO VERSION_REL=$(PSDK_QNX_VERSION_REL)
PINFO TAGID=$(PSDK_QNX_TAGID)
endef

include $(PSDK_QNX_PATH)/qnx/psdk_qnx_build.mk

EXTRA_INCVPATH+=$(PDK_INSTALL_PATH) \
                $(PSDK_QNX_PATH)/qnx/codec/vpu/tivpucodec/vpulib/public \
                $(PSDK_QNX_PATH)/qnx/codec/vpu/OpenMAXIL/khronos/openmaxil \
                $(PSDK_QNX_PATH)/qnx/codec/vpu/OpenMAXIL/utility \
                $(PSDK_QNX_PATH)/qnx/codec/vpu/OpenMAXIL/core \
                $(PSDK_QNX_PATH)/qnx/codec/vpu/tivpucodec/common \
                $(PSDK_QNX_PATH)/qnx/codec/vpu/tivpucodec/encoder \
                $(PSDK_QNX_PATH)/qnx/codec/vpu/tivpucodec/helper \
                $(PSDK_QNX_PATH)/qnx/codec/vpu/tivpucodec/helper/misc \
                $(PSDK_QNX_PATH)/qnx/codec/vpu/tivpucodec/helper/yuv \
                $(PSDK_QNX_PATH)/qnx/codec/vpu/resmgrlib/ \
                $(PSDK_QNX_PATH)/qnx/sharedmemallocator/usr/public/ \
                $(PSDK_QNX_PATH)/qnx/sharedmemallocator/resmgr/public/

EXTRA_SRCVPATH+=$(PSDK_QNX_PATH)/qnx/codec/vpu/OpenMAXIL/components/common

EXCLUDE_OBJS+=omxil_component_dec.o

LIBS = c

EXTRA_LIBVPATH += $(PSDK_QNX_PATH)/qnx/pdk_libs/pdk/aarch64/so.le/
EXTRA_LIBVPATH += $(PSDK_QNX_PATH)/qnx/pdk_libs/sciclient/aarch64/so.le/
EXTRA_LIBVPATH += $(PSDK_QNX_PATH)/qnx/codec/vpu/OpenMAXIL/utility/nto/aarch64/so.le/
EXTRA_LIBVPATH += $(PSDK_QNX_PATH)/qnx/codec/vpu/resmgrlib/aarch64/so.le/
EXTRA_LIBVPATH += $(PSDK_QNX_PATH)/qnx/sharedmemallocator/usr/aarch64/so.le/

LIBS += ti-pdk
LIBS += ti-sciclient
LIBS += vpu_usr_lib
LIBS += omxil_j7_utility
LIBS += sharedmemallocator

include $(MKFILES_ROOT)/qtargets.mk
