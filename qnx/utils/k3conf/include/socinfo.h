/*
 * SoC info header file
 *
 * Copyright (C) 2019 Texas Instruments Incorporated - https://www.ti.com/
 *	Lokesh Vutla <lokeshvutla@ti.com>
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __SOCINFO_H
#define __SOCINFO_H

#include <stdint.h>
#include <tisci.h>
#include "sec_proxy.h"

#define SOC_NAME_MAX_LENGTH			10
#define SOC_REVISION_MAX_LENGTH			10
#define SOC_FULL_NAME_MAX_LENGTH		20

#define AM62X	0xbb7e
#define AM62AX	0xbb8d
#define AM62PX	0xbb9d
#define AM65X	0xbb5a
#define J721E	0xbb64
#define J7200	0xbb6d
#define AM64X	0xbb38
#define J721S2	0xbb75
#define J722S	0xbba0
#define J784S4	0xbb80

typedef enum {
	REV_1,
	REV_2,
	REV_PG_MAX
} k3_soc_rev;

struct k3conf_soc_info {
	uint16_t soc;
	k3_soc_rev rev;
	char soc_full_name[SOC_FULL_NAME_MAX_LENGTH];
	uint8_t host_id;
	uint8_t ti_sci_enabled;
	struct ti_sci_info sci_info;
	struct k3_sec_proxy_base *sec_proxy;
};

static const char soc_revision_j721e[REV_PG_MAX + 1][SOC_REVISION_MAX_LENGTH] = {
	[REV_1] = " SR1.0",
	[REV_2] = " SR1.1",
	[REV_PG_MAX] = " SRNULL"
};

static const char soc_revision_generic[REV_PG_MAX + 1][SOC_REVISION_MAX_LENGTH] = {
	[REV_1] = " SR1.0",
	[REV_2] = " SR2.0",
	[REV_PG_MAX] = " SRNULL"
};

extern struct k3conf_soc_info soc_info;
int soc_init(uint32_t host_id);
int soc_is_j721e(void);
int soc_is_am654(void);

#endif
