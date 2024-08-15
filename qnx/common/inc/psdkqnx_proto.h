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
 * Modfications copyright (c) 2020-2023, Texas Instruments Incorporated
 *
 */

#ifndef __PSDKQNX_PROTO_H_INCLUDED
#define __PSDKQNX_PROTO_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resmgr.h>
#include <sys/neutrino.h>
#include <hw/inout.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>
#include <errno.h>
#include <sys/procmgr.h>
#include <drvr/hwinfo.h>
#include <string.h>
#include <stdarg.h>


enum {
    PSDKQA_SLOGC_PRIVATE_START = 128,
    PSDKQA_SLOGC_TI_SHMEM,
    PSDKQA_SLOGC_TI_SCI_RM,
    PSDKQA_SLOGC_TI_UDMA_RM,
    PSDKQA_SLOGC_TI_UDMA_RM_LIB,
    PSDKQA_SLOGC_TI_IPC_RM,
    PSDKQA_SLOGC_TI_IPC_RM_LIB,
    /* Add additional defines here */
    PSDKQA_SLOGC_PRIVATE_END,
};

void qnx_logger(int level, const char *fmt, ...);
void printVersion(char *banner);

#define QNX_PR_ERR(...)                 qnx_logger(_SLOG_ERROR, __VA_ARGS__)
#define QNX_PR_WARN(...)                qnx_logger(_SLOG_WARNING, __VA_ARGS__)
#define QNX_PR_NOTICE(...)              qnx_logger(_SLOG_NOTICE, __VA_ARGS__)
#define QNX_PR_INFO(...)                qnx_logger(_SLOG_INFO, __VA_ARGS__)
#define QNX_PR_DEBUG(...)               qnx_logger(_SLOG_DEBUG1, __VA_ARGS__)

extern char *buildVersion;
extern char *buildDate;
extern char *buildSoc;
extern int  module_num;
extern int  g_log_level;

#endif //__PSDKQNX_PROTO_H_INCLUDED
