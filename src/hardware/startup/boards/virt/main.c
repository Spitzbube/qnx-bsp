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
#include <arm/pl011.h>


void init_pl011(unsigned, const char *, const char *);
void put_pl011(int);
extern struct callout_rtn	display_char_pl011;
extern struct callout_rtn	poll_key_pl011;
extern struct callout_rtn	break_detect_pl011;

const struct debug_device debug_devices[] = {
	{ 	"pl011", //base^shift.baud.clk
		{	"0x9000000^2.115200.24000000",	/* Use whatever boot loader baud rate */
		},
		init_pl011,
		put_pl011,
		{	&display_char_pl011,
			&poll_key_pl011,
			&break_detect_pl011,
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

	cpu_freq = 100000000;
	lsp.syspage.p->num_cpu = 1;

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

	init_qtime_virt();

	init_cacheattr();

	init_cpuinfo();

	init_hwinfo();

    /*
     * This is handy for debugging a new version of the startup program.
     * Commenting this line out will save a great deal of code.
     */
    print_syspage();

    kprintf("Jumping to QNX\n");

	return 0;
}


__SRCVERSION( "$URL: http://svn/product/tags/public/bsp/nto650/ti-omap3530-beagle/latest/src/hardware/startup/boards/omap3530/main.c $ $Rev: 604842 $" );
