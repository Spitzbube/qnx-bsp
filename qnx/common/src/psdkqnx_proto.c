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

#include "psdkqnx_proto.h"

void qnx_logger(int level, const char *fmt, ...)
{
    va_list arglist;
    va_start(arglist, fmt);

    if (level <= g_log_level) {
        vslogf(_SLOG_SETCODE(_SLOGC_PRIVATE_START, module_num), level, fmt, arglist);
    }
    va_end(arglist);
}

void printVersion(char *banner)
{
    if (banner)
        QNX_PR_NOTICE("%s for %s (version=%s, date=%s)", banner, buildSoc, buildVersion, buildDate);
    else
        QNX_PR_NOTICE("TI Module for %s (version=%s, date=%s)", buildSoc, buildVersion, buildDate);
}


