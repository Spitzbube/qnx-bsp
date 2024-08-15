/*
 * $QNXLicenseC:
 * Copyright 2019, QNX Software Systems.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as
 * contributors under the License or as licensors under other terms.
 * Please review this entire file for other proprietary rights or license
 * notices, as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

/*
 * Modfications copyright (c) 2020-2022, Texas Instruments Incorporated
 *
 */

#ifndef _CLK_TI_SCIMGR_H_INCLUDED
#define _CLK_TI_SCIMGR_H_INCLUDED

#include <stdint.h>
#include "ti/drv/sciclient/sciclient.h"
#include "ti/csl/csl_types.h"
#if defined (SOC_J721E)
#include "ti/drv/sciclient/soc/V1/sciclient_fmwMsgParams.h"
#elif defined (SOC_J7200)
#include "ti/drv/sciclient/soc/V2/sciclient_fmwMsgParams.h"
#elif defined (SOC_J721S2)
#include "ti/drv/sciclient/soc/V4/sciclient_fmwMsgParams.h"
#elif defined (SOC_AM62X)
#include "ti/drv/sciclient/soc/V5/sciclient_fmwMsgParams.h"
#elif defined (SOC_J784S4)
#include "ti/drv/sciclient/soc/V6/sciclient_fmwMsgParams.h"
#elif defined (SOC_AM62A)
#include "ti/drv/sciclient/soc/V7/sciclient_fmwMsgParams.h"
#elif defined (SOC_J722S)
#include "ti/drv/sciclient/soc/V9/sciclient_fmwMsgParams.h"
#else
#error "unsupported SOC"
#endif

/*
 * The following devctls are used by a client application to access SCI
 */
#include <devctl.h>

typedef struct
{
    Sciclient_ReqPrm_t  reqPrm;
    Sciclient_RespPrm_t respPrm;
} tisci_msg_t;


#define DCMD_TISCI_MESSAGE      __DIOTF(_DCMD_MISC, 0, tisci_msg_t) 


#define TI_SCIMGR_VERBOSE_ERROR 0
#define TI_SCIMGR_VERBOSE_INFO  1
#define TI_SCIMGR_VERBOSE_DEBUG 2

#endif

