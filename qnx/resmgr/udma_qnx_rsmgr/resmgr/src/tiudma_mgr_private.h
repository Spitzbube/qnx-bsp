/*
 * $QNXLicenseC:
 * Copyright 2020, QNX Software Systems.
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

#ifndef _TI_UDMAMGR_PRIVATE_H_INCLUDED
#define _TI_UDMAMGR_PRIVATE_H_INCLUDED

#include <stdint.h>
#include <sys/iofunc.h>
#include "ti/drv/udma/udma.h"

typedef struct ti_ipc_ocb {
    iofunc_ocb_t ocb;
    pid_t pid;
} ti_udma_ocb_t;

uint16_t Resmgr_Udma_rmAllocProxy(uint16_t preferredProxyNum, Udma_DrvHandle drvHandle);

int32_t Resmgr_Udma_setup(void);

int32_t Resmgr_Udma_cleanup(void);

#endif
