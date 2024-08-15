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

NAME = ti-enetlld

define PINFO
PINFO DESCRIPTION=TI ENET LLD Library
PINFO STATE=Experimental
PINFO VERSION_REL=$(PSDK_QNX_VERSION_REL)
PINFO TAGID=$(PSDK_QNX_TAGID)
endef

include $(PSDK_QNX_PATH)/qnx/psdk_qnx_build.mk
CCFLAGS += -fPIC  -fshort-enums
CCFLAGS += -DENET_CFG_ASSERT=1 -DENET_CFG_PRINT_ENABLE -DENET_CFG_TRACE_LEVEL=5
ifdef GEN_PDKDEPS
    CCFLAGS += -Wc,-MT,$*.o -Wc,-MMD -Wc,-MP -Wc,-MF,$*.dep
endif
ifeq ($(SOC),$(filter $(SOC), j721e j7200 j721s2))
# Enable MDIO manual mode in j721e SoC due to errata i2329
CCFLAGS += -DENABLE_MDIO_MANUAL_MODE
endif

#Add extra include path
EXTRA_INCVPATH += $(PDK_INSTALL_PATH)                                   \
                  $(PDK_INSTALL_PATH)/ti/drv/enet/src                   \
                  $(PDK_INSTALL_PATH)/ti/drv/enet/soc/j7x               \
                  $(PDK_INSTALL_PATH)/ti/drv/enet/include

#Add source path
EXTRA_SRCVPATH += $(PDK_INSTALL_PATH)/ti/drv/enet/src/common             \
                  $(PDK_INSTALL_PATH)/ti/drv/enet/src/core               \
                  $(PDK_INSTALL_PATH)/ti/drv/enet/src/dma/udma           \
                  $(PDK_INSTALL_PATH)/ti/drv/enet/src/mod                \
                  $(PDK_INSTALL_PATH)/ti/drv/enet/src/per                \
                  $(PDK_INSTALL_PATH)/ti/drv/enet/src/phy                \
                  $(PDK_INSTALL_PATH)/ti/drv/enet/soc/j7x                \
                  $(PDK_INSTALL_PATH)/ti/drv/enet/soc/j7x/$(SOC)

EXCLUDE_OBJS += icssg_stats.o icssg_tas.o icssg_timesync.o
EXCLUDE_OBJS += icssg.o icssg_utils.o

LDFLAGS += -M

include $(MKFILES_ROOT)/qtargets.mk

