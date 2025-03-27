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
#include <arm/bcm2835.h>


void init_bcm2835_debug(unsigned, const char *, const char *);
void put_bcm2835(int);
extern struct callout_rtn	display_char_bcm2835;
extern struct callout_rtn	poll_key_bcm2835;
extern struct callout_rtn	break_detect_bcm2835;

extern void init_qtime_bcm2835();

const struct debug_device debug_devices[] = {
	{ 	"bcm2835", //base^shift.baud.clk
		{	"0x9000000^2.115200.24000000",	/* Use whatever boot loader baud rate */
		},
		init_bcm2835_debug,
		put_bcm2835,
		{	&display_char_bcm2835,
			&poll_key_bcm2835,
			&break_detect_bcm2835,
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
	// Disable pull up/down for all GPIO pins & delay for 150 cycles.
	//out32(BCM2835_GPPUD, 0x00000000);
	out32(BCM2835_GPIO_BASE+BCM2835_GPIO_GPPUD, 0x00000000);
	//delay(150);

	// Disable pull up/down for pin 14,15 & delay for 150 cycles.
	//out32(BCM2835_GPPUDCLK0, (1 << 14) | (1 << 15));
	out32(BCM2835_GPIO_BASE+BCM2835_GPIO_GPPUDCLK0, (1 << 14) | (1 << 15));

	// Write 0 to GPPUDCLK0 to make it take effect.
	//out32(BCM2835_GPPUDCLK0, 0x00000000);
	out32(BCM2835_GPIO_BASE+BCM2835_GPIO_GPPUDCLK0, 0x00000000);

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

	init_qtime_bcm2835();

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
