#
#   Copyright (c) 2021, Texas Instruments Incorporated
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

NAME = k3conf
INSTALLDIR = bin
USEFILE=$(PROJECT_ROOT)/$(NAME).use

define PINFO
PINFO DESCRIPTION=K3conf qnx utility
PINFO STATE=Experimental
endef

SOC_UPPER = SOC_$(shell echo $(SOC) | tr a-z A-Z)
CCOPTS += -D$(SOC_UPPER)
CCOPTS += -DQNX_OS

#Add extra include path
EXTRA_INCVPATH += $(PDK_INSTALL_PATH)
EXTRA_INCVPATH += $(K3CONF_REPO)/include

#Add source path
EXTRA_SRCVPATH+=$(K3CONF_REPO)
EXTRA_SRCVPATH+=$(K3CONF_REPO)/soc/j721e
EXTRA_SRCVPATH+=$(K3CONF_REPO)/soc/j7200
EXTRA_SRCVPATH+=$(K3CONF_REPO)/soc/j721s2
EXTRA_SRCVPATH+=$(K3CONF_REPO)/soc/j784s4
EXTRA_SRCVPATH+=$(K3CONF_REPO)/soc/j722s
EXTRA_SRCVPATH+=$(K3CONF_REPO)/soc/am65x
EXTRA_SRCVPATH+=$(K3CONF_REPO)/soc/am65x_sr2
EXTRA_SRCVPATH+=$(K3CONF_REPO)/soc/am64x
EXTRA_SRCVPATH+=$(K3CONF_REPO)/soc/am62x
EXTRA_SRCVPATH+=$(K3CONF_REPO)/soc/am62ax
EXTRA_SRCVPATH+=$(K3CONF_REPO)/soc/am62px
EXTRA_SRCVPATH+=$(K3CONF_REPO)/common
EXTRA_SRCVPATH+=$(K3CONF_REPO)/qnx/tisci_qnx
EXTRA_SRCVPATH+=$(K3CONF_REPO)/qnx/

# C file to exlude for QNX build
EXCLUDE_OBJS+= mmio.o

LIBS = c

EXTRA_LIBVPATH += $(PSDK_QNX_PATH)/qnx/pdk_libs/pdk/aarch64/so.le/
EXTRA_LIBVPATH += $(PSDK_QNX_PATH)/qnx/pdk_libs/sciclient/aarch64/so.le/

LIBS += ti-pdk
LIBS += ti-sciclient

include $(MKFILES_ROOT)/qtargets.mk
