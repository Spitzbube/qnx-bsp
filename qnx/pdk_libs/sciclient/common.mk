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

NAME = ti-sciclient

define PINFO
PINFO DESCRIPTION=TI SCI Client PDK Library
PINFO STATE=Experimental
PINFO VERSION_REL=$(PSDK_QNX_VERSION_REL)
PINFO TAGID=$(PSDK_QNX_TAGID)
endef

include $(PSDK_QNX_PATH)/qnx/psdk_qnx_build.mk
CCFLAGS += -fPIC -fshort-enums
ifdef GEN_PDKDEPS
    CCFLAGS += -Wc,-MT,$*.o -Wc,-MMD -Wc,-MP -Wc,-MF,$*.dep
endif

# SoC Specific source files
ifeq ($(SOC),$(filter $(SOC), j721e))
  SCICLIENT_SOCVER = V1
endif
ifeq ($(SOC),$(filter $(SOC), j7200))
  SCICLIENT_SOCVER = V2
endif
ifeq ($(SOC),$(filter $(SOC), j721s2))
  SCICLIENT_SOCVER = V4
endif
ifeq ($(SOC),$(filter $(SOC), j784s4))
  SCICLIENT_SOCVER = V6
endif
ifeq ($(SOC),$(filter $(SOC), j722s))
  SCICLIENT_SOCVER = V9
endif

#Add extra include path
EXTRA_INCVPATH += $(PDK_INSTALL_PATH)                                             \
                  $(PDK_INSTALL_PATH)/ti/drv/sciclient/src/sciclient              \
                  $(PDK_INSTALL_PATH)/ti/drv/sciclient/soc/$(SCICLIENT_SOCVER)    \
                  $(PDK_INSTALL_PATH)/ti/drv/sciclient/include

#Add source path
EXTRA_SRCVPATH += $(PDK_INSTALL_PATH)/ti/drv/sciclient/src/sciclient               \
                  $(PDK_INSTALL_PATH)/ti/drv/sciclient/soc/$(SCICLIENT_SOCVER)

EXCLUDE_OBJS = sciclient_direct.o sciclient_dummy.o sciclient_rm_csl.o
EXCLUDE_OBJS += sciclient_defaultBoardcfg.o sciclient_defaultBoardcfg_pm.o
EXCLUDE_OBJS += sciclient_defaultBoardcfg_rm.o sciclient_defaultBoardcfg_security.o
EXCLUDE_OBJS += sciclient_pmic.o sciclient_s2r.o sciclient_lpm.o

LDFLAGS += -M

include $(MKFILES_ROOT)/qtargets.mk

