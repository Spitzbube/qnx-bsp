/*
 * Copyright (c) 2008, 2022, BlackBerry Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "proto.h"
#include "externs.h"

int
main(int argc, char *argv[])
{
    /*
     * in SMP, the first time to call nanospin_calibrate() could cause the
     * devc driver to be rescheduled.
     * nanospin_ns() is called several times right after the UART is disabled,
     * if the startup serial driver callouts are called in this time window,
     * it could cause problems.
     * So we call nanospin_calibrate() before any nanospin_ns() is called
     * to minimize this problem
     */
    nanospin_calibrate(0);

    ttyctrl.max_devs = 16;
    ttc(TTC_INIT_PROC, &ttyctrl, 24);

    if (options(argc, argv) <= 0) {
        fprintf(stderr, "%s: No serial ports found\n", argv[0]);
        exit(0);
    }

    ttc(TTC_INIT_START, &ttyctrl, 0);

    return (0);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/devc/seromap/main.c $ $Rev: 962454 $")
#endif
