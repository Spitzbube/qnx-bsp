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

/*
 * AT91SAM9xx Advanced Interrupt Controller (AIC) support.
 */

#include "startup.h"
#include "arm/mb86h60.h"


static paddr_t	mb86h60_irq_ctrl_arm_base  = MB86H60_IRQ_CTRL_ARM_BASE;
static paddr_t	mb86h60_vic_base  = MB86H60_VIC_BASE;

extern struct callout_rtn interrupt_id_mb86h60;
extern struct callout_rtn interrupt_eoi_mb86h60;
extern struct callout_rtn interrupt_mask_mb86h60;
extern struct callout_rtn interrupt_unmask_mb86h60;


const static struct startup_intrinfo	intrs[] = {
	{	_NTO_INTR_CLASS_EXTERNAL, 	// vector base
		32,							// number of vectors
		_NTO_INTR_SPARE,			// cascade vector
		0,							// CPU vector base
		0,							// CPU vector stride
		0,							// flags

		{ INTR_GENFLAG_LOAD_SYSPAGE,	0, &interrupt_id_mb86h60 },
		{ INTR_GENFLAG_LOAD_SYSPAGE | INTR_GENFLAG_LOAD_INTRMASK, 0, &interrupt_eoi_mb86h60 },
		&interrupt_mask_mb86h60,	// mask   callout
		&interrupt_unmask_mb86h60,	// unmask callout
		0,							// config callout
		&mb86h60_vic_base,
	},
};


void init_intrinfo(void)
{
	int i;

    kprintf("init_intrinfo: TODO\n");

    out32(mb86h60_irq_ctrl_arm_base + MB86H60_IRQ_CTRL_IRQMASK, 
		(1 << MB86H60_INTR_TIMER0)|
		(1 << MB86H60_INTR_UART0)|
		(1 << MB86H60_INTR_DMA)|
		(1 << MB86H60_INTR_USB) 
		);
    out32(mb86h60_irq_ctrl_arm_base + MB86H60_IRQ_CTRL_IRQXOR, 0);

	for (i = 0; i < 32; i++)
	{
		out32(mb86h60_vic_base + MB86H60_VIC_VECTPRIORITYX + i*4, 5);
	}

	out32(mb86h60_vic_base + MB86H60_VIC_INTENABLE, 0);
	out32(mb86h60_vic_base + MB86H60_VIC_INTSELECT, 0);

    add_interrupt_array(intrs, sizeof(intrs));
}
