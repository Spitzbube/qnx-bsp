#/*
# * Makefile for k3conf
# *
# * Copyright (C) 2019 Texas Instruments Incorporated - https://www.ti.com/
# *	Lokesh Vutla <lokeshvutla@ti.com>
# *
# *  Redistribution and use in source and binary forms, with or without
# *  modification, are permitted provided that the following conditions
# *  are met:
# *
# *    Redistributions of source code must retain the above copyright
# *    notice, this list of conditions and the following disclaimer.
# *
# *    Redistributions in binary form must reproduce the above copyright
# *    notice, this list of conditions and the following disclaimer in the
# *    documentation and/or other materials provided with the
# *    distribution.
# *
# *    Neither the name of Texas Instruments Incorporated nor the names of
# *    its contributors may be used to endorse or promote products derived
# *    from this software without specific prior written permission.
# *
# *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# */

VERSION_MAJOR=0
VERSION_MINOR=3

CROSS_COMPILE ?=aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc

DEF_INC := $(shell $(CROSS_COMPILE)gcc  -print-file-name=include)

DEF_INC_PATH = -I. -Iinclude

STATIC_BUILD ?= -static

MYCFLAGS+=$(CFLAGS) -c -Wall -Wextra -Wno-missing-field-initializers \
		-I$(DEF_INC) $(DEF_INC_PATH) $(STATIC_BUILD)

ifdef DEBUG
MYCFLAGS+=-g3 -fno-inline -O0 -DDEBUG
else
MYCFLAGS+=-Os
endif

DESTDIR ?= target

COMMONSOURCES=\
		common/k3conf.c \
		common/help.c \
		common/mmio.c \
		common/socinfo.c \
		common/sec_proxy.c \
		common/tisci/tisci_core.c \
		common/tisci/tisci_device.c \
		common/tisci/tisci_clock.c \
		common/autoadjust_table.c \
		common/cmd_show.c \
		common/cmd_dump.c \
		common/cmd_enable.c \
		common/cmd_disable.c \
		common/cmd_set.c \
		common/cmd_rw.c

AM65XSOURCES =\
	      soc/am65x/am65x_host_info.c \
	      soc/am65x/am65x_sec_proxy_info.c \
	      soc/am65x/am65x_processors_info.c \
	      soc/am65x/am65x_devices_info.c \
	      soc/am65x/am65x_clocks_info.c \
	      soc/am65x/am65x_rm_info.c \
	      soc/am65x_sr2/am65x_sr2_host_info.c \
	      soc/am65x_sr2/am65x_sr2_sec_proxy_info.c \
	      soc/am65x_sr2/am65x_sr2_processors_info.c \
	      soc/am65x_sr2/am65x_sr2_devices_info.c \
	      soc/am65x_sr2/am65x_sr2_clocks_info.c \
	      soc/am65x_sr2/am65x_sr2_rm_info.c

J721ESOURCES =\
	      soc/j721e/j721e_host_info.c \
	      soc/j721e/j721e_sec_proxy_info.c \
	      soc/j721e/j721e_processors_info.c \
	      soc/j721e/j721e_devices_info.c \
	      soc/j721e/j721e_clocks_info.c \
	      soc/j721e/j721e_rm_info.c \
	      soc/j7200/j7200_host_info.c \
	      soc/j7200/j7200_sec_proxy_info.c \
	      soc/j7200/j7200_processors_info.c \
	      soc/j7200/j7200_devices_info.c \
	      soc/j7200/j7200_clocks_info.c \
	      soc/j7200/j7200_rm_info.c


AM64XSOURCES =\
	      soc/am64x/am64x_host_info.c \
	      soc/am64x/am64x_sec_proxy_info.c \
	      soc/am64x/am64x_processors_info.c \
	      soc/am64x/am64x_devices_info.c \
	      soc/am64x/am64x_clocks_info.c \
	      soc/am64x/am64x_rm_info.c

AM62XSOURCES =\
	      soc/am62x/am62x_devices_info.c \
	      soc/am62x/am62x_clocks_info.c \
	      soc/am62x/am62x_host_info.c \
	      soc/am62x/am62x_processors_info.c \
	      soc/am62x/am62x_rm_info.c \
	      soc/am62x/am62x_sec_proxy_info.c

J721S2SOURCES =\
	      soc/j721s2/j721s2_devices_info.c \
	      soc/j721s2/j721s2_clocks_info.c \
	      soc/j721s2/j721s2_host_info.c \
	      soc/j721s2/j721s2_processors_info.c \
	      soc/j721s2/j721s2_rm_info.c \
	      soc/j721s2/j721s2_sec_proxy_info.c

J784S4SOURCES =\
	      soc/j784s4/j784s4_clocks_info.c \
	      soc/j784s4/j784s4_devices_info.c \
	      soc/j784s4/j784s4_host_info.c \
	      soc/j784s4/j784s4_processors_info.c \
	      soc/j784s4/j784s4_rm_info.c \
	      soc/j784s4/j784s4_sec_proxy_info.c

AM62AXSOURCES =\
	      soc/am62ax/am62ax_clocks_info.c \
	      soc/am62ax/am62ax_devices_info.c \
	      soc/am62ax/am62ax_host_info.c \
	      soc/am62ax/am62ax_processors_info.c \
	      soc/am62ax/am62ax_rm_info.c \
	      soc/am62ax/am62ax_sec_proxy_info.c

AM62PXSOURCES =\
              soc/am62px/am62px_clocks_info.c \
              soc/am62px/am62px_devices_info.c \
              soc/am62px/am62px_host_info.c \
              soc/am62px/am62px_processors_info.c \
              soc/am62px/am62px_rm_info.c \
              soc/am62px/am62px_sec_proxy_info.c

COMMONOBJECTS=	$(COMMONSOURCES:.c=.o)
AM65XOBJECTS=	$(AM65XSOURCES:.c=.o)
J721EOBJECTS=	$(J721ESOURCES:.c=.o)
AM64XOBJECTS=	$(AM64XSOURCES:.c=.o)
AM62XOBJECTS=	$(AM62XSOURCES:.c=.o)
J721S2OBJECTS=  $(J721S2SOURCES:.c=.o)
J784S4OBJECTS=	$(J784S4SOURCES:.c=.o)
AM62AXOBJECTS=	$(AM62AXSOURCES:.c=.o)
AM62PXOBJECTS=	$(AM62PXSOURCES:.c=.o)

ALLSOURCES=	$(COMMONSOURCES) $(AM65XSOURCES) $(J721ESOURCES) \
		$(AM64XSOURCES) $(AM62XSOURCES) $(J721S2SOURCES) \
		$(J784S4SOURCES) $(AM62AXSOURCES) $(AM62PXSOURCES)

ALLOBJECTS=	$(COMMONOBJECTS) $(AM65XOBJECTS) $(J721EOBJECTS) \
		$(AM64XOBJECTS) $(AM62XOBJECTS) $(J721S2OBJECTS) \
		$(J784S4OBJECTS) $(AM62AXOBJECTS) $(AM62PXOBJECTS)

#
# Pretty print
#
V             = @
Q             = $(V:1=)
QUIET_CC      = $(Q:@=@echo    '     CC       '$@;)
QUIET_GEN     = $(Q:@=@echo    '     GEN      '$@;)
QUIET_LINK    = $(Q:@=@echo    '     LINK     '$@;)

EXECUTABLE=	k3conf

.PHONY:	tags cscope

all: 		$(EXECUTABLE)

$(EXECUTABLE):	$(ALLOBJECTS) builddate.o version.o
		$(QUIET_CC) $(CC) $(STATIC_BUILD) $(LDFLAGS) $(ALLOBJECTS) builddate.o version.o\
		-lrt -o $@

.c.o:
		$(QUIET_CC) $(CC) $(MYCFLAGS) $(LDFLAGS) -c $< -o $@

builddate.c:	$(ALLOBJECTS)
		@echo 'char *builddate="'`date`'";' > builddate.c

version.c:	$(ALLOBJECTS)
		@echo 'char *k3conf_version="'`git describe --dirty 2>/dev/null||echo "$(VERSION_MAJOR).$(VERSION_MINOR)-nogit"`'";' > version.c

tags: $(ALLSOURCES)
	ctags $(shell $(CC) $(MYCFLAGS) -MM -MG $(ALLSOURCES) |\
			sed -e "s/^.*\.o://g"|tr -d '\\')

cscope: $(ALLSOURCES)
	cscope -R -b

clean:
		@echo "Cleaning up..."
		-$(shell rm -f $(EXECUTABLE) *.o builddate.c version.c)
		-$(shell rm -f $(ALLOBJECTS))
		-$(shell rm -f tags cscope.out)
		@echo "Done."
