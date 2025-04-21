/*
 * Copyright 2024 BlackBerry Limited.
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


#include "sys/hwinfo.h"
#include <hw/hwinfo_private.h>
#include <startup.h>
#include "board.h"

void hwi_j722s(void)
{
    /* USB3 Host Module */
    {
        unsigned hwi_off;
        hwiattr_common_t attr = HWIATTR_COMMON_INITIALIZER;

        hwi_off = hwidev_add("cdns3-host", hwi_devclass_NONE, HWI_NULL_OFF);

        HWIATTR_SET_OPTSTR(&attr,
                "soc=j722s"
                ",phy_name=torrent"
                ",typec"
                ",otg_irq=277"
                ",usbss_addr=0xf920000"
                ",otgreg_addr=0x31200000"
                ",serdes_addr=0xf000000"
                ",serdes_start_lane=0"
                ",serdes_num_lanes=1"
                ",pll_ref_clk=25000"
                );

        hwitag_add_common(hwi_off, &attr);
    }
}
#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/startup/boards/ti-j7/j722s/evm/hwi_j722s.c $ $Rev: 994909 $")
#endif
