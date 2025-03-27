/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems.
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




#include "startup.h"

void init_mb86h60_debug(unsigned, const char *, const char *);
void put_mb86h60(int);
extern struct callout_rtn	display_char_mb86h60;

extern void init_qtime_mb86h60();

const struct debug_device debug_devices[] = {
	{ 	"mb86h60", //base^shift.baud.clk
		{	"0xFFFFEE00^0.0.133000000.16",	/* Use whatever boot loader baud rate */
		},
		init_mb86h60_debug,
		put_mb86h60,
		{	&display_char_mb86h60,
#if 0 //TODO
			&poll_key_mb86h60,
			&break_detect_mb86h60,
#endif
		}
	},
};


/*
 * main()
 *	Startup program executing out of RAM
 *
 * 1. It gathers information about the system and places it in a structure
 *    called the system page. The kernel references this structure to
 *    determine everything it needs to know about the system. This structure
 *    is also available to user programs (read only if protection is on)
 *    via _syspage->.
 *
 * 2. It (optionally) turns on the MMU and starts the next program
 *    in the image file system.
 */
int
main(int argc, char **argv, char **envv)
{
	int opt;

	mb86h60_board_init();

	console_send_string("Hello QNX!\n");

    while ((opt = getopt(argc, argv, COMMON_OPTIONS_STRING)) != -1) {
        switch (opt) {
            default:
                handle_common_option(opt);
                break;
        }
    }

	/*
	 * Initialize debugging output
	 */
	select_debug(debug_devices, sizeof(debug_devices));

	kprintf("Hello World!\n");

    /*
     * Collect information on all free RAM in the system
     */
    init_raminfo();

	/*
	 * Remove RAM used by modules in the image
	 */
	alloc_ram(shdr->ram_paddr, shdr->ram_size, 1);

	if (shdr->flags1 & STARTUP_HDR_FLAGS1_VIRTUAL)
	{
		init_mmu();
	}

	init_intrinfo();

	init_qtime_mb86h60();

	init_cacheattr();

	init_cpuinfo();

	init_hwinfo();

    /*
     * Load bootstrap executables in the image file system and Initialise
     * various syspage pointers. This must be the _last_ initialisation done
     * before transferring control to the next program.
     */
    init_system_private();

    /*
     * This is handy for debugging a new version of the startup program.
     * Commenting this line out will save a great deal of code.
     */
    print_syspage();

    kprintf("Jumping to QNX\n");

	return 0;
}


__SRCVERSION( "$URL: http://svn/product/tags/public/bsp/nto650/ti-omap3530-beagle/latest/src/hardware/startup/boards/omap3530/main.c $ $Rev: 604842 $" );
