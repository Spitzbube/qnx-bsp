/*
 * Copyright (c) 2019, 2022, BlackBerry Limited.
 * Copyright 2021, Texas Instruments Incorporated.
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
 * Update syspage RAM section
 */
#include <startup.h>
#include "board.h"


void init_raminfo(void)
{
#ifdef  IDK_DDR0_SIZE
    add_ram(IDK_DDR0_BASE, IDK_DDR0_SIZE);
#endif
#ifdef  IDK_DDR1_SIZE
    add_ram(IDK_DDR1_BASE, IDK_DDR1_SIZE);
#endif
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/startup/boards/ti-j7/init_raminfo.c $ $Rev: 984580 $")
#endif
