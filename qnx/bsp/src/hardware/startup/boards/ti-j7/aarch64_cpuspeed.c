/*
 * Copyright (C) 2019, 2022, BlackBerry Limited.
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

#include <startup.h>
#include "ti_sci.h"

/*
 * Calculate the core frequency (in MHz).
 */
uint32_t
aarch64_cpuspeed(void)
{
    uint64_t    mpidr;
    uint64_t    freq;
    uint32_t    devid;
    uint8_t     clkid = 0xff;

    mpidr = aa64_sr_rd64(mpidr_el1);
    if ((mpidr & (uint64_t)0xff) == 0x0) {
        devid = TISCI_DEV_A72SS0_CORE0;
        clkid = TISCI_DEV_A72SS0_CORE0_ARM_CLK_CLK;
    } else if ((mpidr & (uint64_t)0xff) == 0x1) {
        devid = TISCI_DEV_A72SS0_CORE1;
        clkid = TISCI_DEV_A72SS0_CORE1_ARM_CLK_CLK;
    }

    if ((clkid != 0xff) && (ti_sci_cmd_clk_get_freq(devid, clkid, &freq) == 0)) {
        kprintf("%s: core speed %d\n", __FUNCTION__, freq / 1000000);
        return (freq / 1000000);
    }

    return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/startup/boards/ti-j7/aarch64_cpuspeed.c $ $Rev: 984580 $")
#endif
