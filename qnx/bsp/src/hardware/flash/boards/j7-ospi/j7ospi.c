/*
 * Copyright (c) 2022, BlackBerry Limited.
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

#include "soc/j7ospi/j7ospi.h"
#include "f3s_snor.h"

/*
 * This is the main function for f3s flash file system.
 */

int main(const int argc, char **const argv)
{
    int    error;
    static f3s_service_t service[] =
    {
        {
            .struct_size = sizeof(f3s_service_t),
            .open   = f3s_j7ospi_open,        // TI J7 SoCs
            .page   = f3s_snor_page,          // generic SNOR page callout
            .status = f3s_snor_status,        // generic SNOR status callout
            .close  = f3s_snor_close          // generic SNOR close callout
        },
        {
            // mandatory last entry
            .struct_size = 0,
            .open   = NULL,
            .page   = NULL,
            .status = NULL,
            .close  = NULL
        }
    };

#if MTD_VER == 2
    static f3s_flash_v2_t flash[] =
    {
        {
            .struct_size = sizeof(f3s_flash_v2_t),
            .ident = f3s_mt35x_ident,        // Ident
            .reset = f3s_snor_reset,        // Reset

            // v1 callouts, not supported
            .read = NULL, .write = NULL, .erase = NULL, .suspend = NULL, .resume = NULL, .sync = NULL,

            .v2read      = f3s_snor_read,        // v2 Read
            .v2write     = f3s_snor_program,     // v2 Write
            .v2erase     = f3s_snor_erase,       // v2 Erase
            .v2suspend   = f3s_snor_suspend,     // v2 Suspend
            .v2resume    = f3s_snor_resume,      // v2 Resume
            .v2sync      = f3s_snor_sync,        // v2 Sync
            .v2islock    = NULL,
            .v2lock      = NULL,
            .v2unlock    = NULL,
            .v2unlockall = NULL,
            .v2ssrop     = NULL
        },
        {
            .struct_size = sizeof(f3s_flash_v2_t),
            .ident = f3s_s28hx_ident,        // Ident
            .reset = f3s_s28hx_reset,        // S28HX specific reset callout

            // v1 callouts, not supported
            .read = NULL, .write = NULL, .erase = NULL, .suspend = NULL, .resume = NULL, .sync = NULL,

            .v2read      = f3s_snor_read,        // v2 Read
            .v2write     = f3s_snor_program,     // v2 Write
            .v2erase     = f3s_snor_erase,       // v2 Erase
            .v2suspend   = f3s_snor_suspend_sr2, // v2 Suspend
            .v2resume    = f3s_snor_resume,      // v2 Resume
            .v2sync      = f3s_snor_sync,        // v2 Sync
            .v2islock    = NULL,
            .v2lock      = NULL,
            .v2unlock    = NULL,
            .v2unlockall = NULL,
            .v2ssrop     = NULL
        },
        {
            // mandatory last entry
            .struct_size = 0,
            .ident = NULL,
            .reset = NULL,

            // v1 callouts
            .read = NULL, .write = NULL, .erase = NULL, .suspend = NULL, .resume = NULL, .sync = NULL,

            .v2read      = NULL,
            .v2write     = NULL,
            .v2erase     = NULL,
            .v2suspend   = NULL,
            .v2resume    = NULL,
            .v2sync      = NULL,
            .v2islock    = NULL,
            .v2lock      = NULL,
            .v2unlock    = NULL,
            .v2unlockall = NULL,
            .v2ssrop     = NULL
        }
    };
#else
#error "MTD version must be 2"
#endif

    /* init f3s */
    f3s_init(argc, argv, (f3s_flash_t *)flash);

    /* start f3s */
    error = f3s_start(service, (f3s_flash_t *)flash);

    return error;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
