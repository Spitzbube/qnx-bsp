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
 * mb86h60 specific timer support.
 */

#include "startup.h"
#include "arm/mb86h60.h"

#define MB86H60_CLOCK_FREQ      81000000
#define MB86H60_CLOCK_RATE      12345679
#define MB86H60_CLOCK_SCALE     -15

#define TIMER_PRESCALE		(0)
#define TIMER_LOAD_VAL		(~0UL)

extern struct callout_rtn   timer_load_mb86h60;
extern struct callout_rtn   timer_value_mb86h60;
extern struct callout_rtn   timer_reload_mb86h60;

static uintptr_t mb86h60_timer_base;

static const struct callout_slot	timer_callouts[] = {
	{ CALLOUT_SLOT(timer_load, _mb86h60) },
	{ CALLOUT_SLOT(timer_value, _mb86h60) },
	{ CALLOUT_SLOT(timer_reload, _mb86h60) },
};

static unsigned
timer_start_mb86h60(void) 
{
    out32(mb86h60_timer_base + MB86H60_TIMER_COUNT_PRE, TIMER_PRESCALE);
    out32(mb86h60_timer_base + MB86H60_TIMER_COUNT_LOW, TIMER_LOAD_VAL);
    out32(mb86h60_timer_base + MB86H60_TIMER_COUNT_HI, 0);
    out32(mb86h60_timer_base + MB86H60_TIMER_ENABLE, MB86H60_TIMER_EN_ENABLE);

    unsigned start_val = in32(mb86h60_timer_base + MB86H60_TIMER_COUNT_LOW);

//    kprintf("timer_start_mb86h60: start_val=%d\n", start_val);

	return start_val;
}

static unsigned
timer_diff_mb86h60(unsigned start)
{
    unsigned diff;    
    unsigned now = in32(mb86h60_timer_base + MB86H60_TIMER_COUNT_LOW);

    if (start >= now)
    {
        diff = (start - now);
    }
    else
    {
        diff = (start + TIMER_LOAD_VAL - now);
    }

    kprintf("timer_diff_mb86h60: start=%d, now=%d, diff=%d\n", start, now, diff);

    return diff;
}

void
init_qtime_mb86h60(void)
{
    kprintf("init_qtime_mb86h60: TODO\n");

	struct qtime_entry *qtime = alloc_qtime();

	/*
	 * Map the timer registers
	 */
	mb86h60_timer_base = startup_io_map(MB86H60_TIMER_SIZE, MB86H60_TIMER0_BASE);

    /*
     * Setup Timer0
     * Stop timer, timer_load will enable it
     */
    out32(mb86h60_timer_base + MB86H60_TIMER_ENABLE, 0);

    out32(mb86h60_timer_base + MB86H60_TIMER_COUNT_PRE, 0);
    out32(mb86h60_timer_base + MB86H60_TIMER_COUNT_LOW, TIMER_LOAD_VAL);
    out32(mb86h60_timer_base + MB86H60_TIMER_COUNT_HI, 0);

	timer_start = timer_start_mb86h60;
	timer_diff = timer_diff_mb86h60;

	qtime->intr = MB86H60_INTR_TIMER0;
	qtime->timer_rate  = MB86H60_CLOCK_RATE;
	qtime->timer_scale = MB86H60_CLOCK_SCALE;
	qtime->cycles_per_sec = (uint64_t)MB86H60_CLOCK_FREQ;

	add_callout_array(timer_callouts, sizeof(timer_callouts));
}
