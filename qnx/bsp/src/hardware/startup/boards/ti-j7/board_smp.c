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

/**
 * @file       board_smp.c
 * @addtogroup startup
 * @{
 */

/**
 * Return CPU core number.
 *
 * @return CPU core number.
 */
unsigned board_smp_num_cpu(void)
{
    return 2;
}

/**
 * Perform any board specific SMP initialisation.
 *
 * @param smp      Pointer to smp_entry structure.
 * @param num_cpus CPU cores number.
 */
void board_smp_init(struct smp_entry *smp, const unsigned num_cpus)
{
    smp->send_ipi = (void *)&sendipi_gic_v3_sr;
}

/**
 * Initialize and start secondary CPU core.
 *
 * @param cpu   CPU core index.
 * @param start CPU reset address.
 *
 * @return  CPU core start status.
 * @retval  0   CPU core start failed.
 * @retval  1   Success, OK.
 */
int board_smp_start(const unsigned cpu, void (*start)(void))
{
    psci_smp_start(cpu, start);

    return 1;
}

/**
 * Perform any board/cpu-specific actions required to adjust the cpu number.
 *
 * @param cpu CPU core number.
 *
 * @return    Adjusted CPU core number.
 */
unsigned board_smp_adjust_num(const unsigned cpu)
{
    return cpu;
}

/** @} */ /* End of startup */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/startup/boards/ti-j7/board_smp.c $ $Rev: 984580 $")
#endif
