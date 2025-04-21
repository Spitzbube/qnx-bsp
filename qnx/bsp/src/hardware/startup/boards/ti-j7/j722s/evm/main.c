/*
 * Copyright 2019,2022-2024, BlackBerry Limited.
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
 * TI J722S EVM Board
 */
#include <startup.h>
#include <time.h>
#include "board.h"

int     r5_display = 0;

int
main(const int argc, char **const argv, const char **const envv)
{
    int     opt;

    /* Reboot callout */
    const struct callout_slot callouts[] = {
        {
            .offset  = offsetof(struct callout_entry, reboot),
            .callout = &reboot_ti_sci
        },
    };

    /* Debug port is on UART0 */
    const struct debug_device debug_devices[] = {
        {   .name = "8250",
            .defaults = {[0] = "0x02800000^2.0.0.1",
                         [1] = NULL
                        },
            .init = init_8250,
            .put = put_8250,
            .callouts[DEBUG_DISPLAY_CHAR] = &display_char_8250,
            .callouts[DEBUG_POLL_KEY] = &poll_key_8250,
            .callouts[DEBUG_BREAK_DETECT] = &break_detect_8250,
        },
    };

    /* Initialize debugging output */
    select_debug(debug_devices, sizeof(debug_devices));

    /* Adding callouts */
    add_callout_array(callouts, sizeof(callouts));

    /* Common options that should be avoided are:
     * "AD:F:f:I:i:K:M:N:o:P:R:S:Tvr:j:Z"
     */
    while ((opt = getopt(argc, argv, COMMON_OPTIONS_STRING "d")) != -1) {
        switch (opt) {
            case 'd':
                /* Display is managed on R5 core */
                r5_display = 1;
                break;

            default:
                handle_common_option(opt);
                break;
        }
    }

    /*
     * Collect information on all free RAM in the system
     */
    init_raminfo();

    /*
     * Remove RAM used by modules in the image
     */
    alloc_ram(shdr->ram_paddr, shdr->ram_size, 1);

    /* Enable Hypervisor if requested (and possible) */
    hypervisor_init(0);

    /*
     * Initialize SMP
     */
    init_smp();

    /* Initialize MMU */
    if (shdr->flags1 & STARTUP_HDR_FLAGS1_VIRTUAL) {
        init_mmu();
        board_mmu_enable();
        board_alignment_check_disable();
    }

    /* Initialize the Interrupts related Information */
    init_intrinfo();

    /* Initialize the Timer related information */
    init_qtime();

    /* Init L2 Cache Controller */
    init_cacheattr();

    /* Initialize the CPU related information */
    init_cpuinfo();

    add_typed_string(_CS_MACHINE, "TI J722S EVM Board");

    // hardware specific initialization
    hw_init();

    /* Initialize the Hwinfo section of the Syspage */
    hwi_j722s();

    /*
     * Load bootstrap executables in the image file system and Initialize
     * various syspage pointers. This must be the _last_ initialization done
     * before transferring control to the next program.
     */
    init_system_private();

    board_mmu_disable();

    /*
     * This is handy for debugging a new version of the startup program.
     * Commenting this line out will save a great deal of code.
     */
    print_syspage();

    return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/startup/boards/ti-j7/j722s/evm/main.c $ $Rev: 994909 $")
#endif
