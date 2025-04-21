/*
 * Copyright (c) 2009, 2022, BlackBerry Limited.
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

#define NANOSECONDS 1000000000u

#define OPTION_STR  "a:c:dfh:i:l:p:v"

int
tda4_options(tda4_dev_t *dev, const int argc, char *argv[])
{
    int         opt;
    int         prev_optind;
    int         done = 0;
    unsigned    temp;

    /* defaults */
    dev->physbase   = TDA4_I2C0_BASE;
    dev->intr       = TDA4_I2C0_IRQ;
    dev->reglen     = TDA4_I2C_SIZE;
    dev->own_addr   = 1;
    dev->flags      = TDA4_FLG_FIFO_PREFILL;
    dev->iid        = -1;
    dev->re_start   = 0;
    dev->speed      = 100000;
    dev->pclk       = TDA4_PCLK;
    dev->high_adjust_fast = 0;
    dev->high_adjust_slow = 0;
    dev->low_adjust_fast  = 0;
    dev->low_adjust_slow  = 0;
    dev->verbose = _SLOG_ERROR;

    while (!done) {
        prev_optind = optind;
        opt = getopt(argc, argv, OPTION_STR);

        switch (opt) {
        case 'a':
            dev->own_addr = (unsigned)strtoul(optarg, &optarg, 0);
            break;

        case 'c':
            dev->pclk = (unsigned)strtoul(optarg, &optarg, 0);
            break;

        case 'd':
            dev->flags &= ~TDA4_FLG_FIFO_PREFILL;
            break;

        case 'f':
            dev->flags |= TDA4_FLG_ROVR_XUDF_OK;
            break;

        case 'h':
            temp = (unsigned)strtoul(optarg, &optarg, 0);
            dev->high_adjust_fast = (int)(temp / (NANOSECONDS / TDA4_I2C_ICLK_9600K));
            dev->high_adjust_slow = (int)(temp / (NANOSECONDS / TDA4_I2C_ICLK_4000K));
            break;

        case 'i':
            dev->intr = (int)strtol(optarg, &optarg, 0);
            break;

        case 'l':
            temp = (unsigned)strtoul(optarg, &optarg, 0);
            dev->low_adjust_fast = (int)(temp / (NANOSECONDS / TDA4_I2C_ICLK_9600K));
            dev->low_adjust_slow = (int)(temp / (NANOSECONDS / TDA4_I2C_ICLK_4000K));
            break;

        case 'p':
            dev->physbase = strtoul(optarg, &optarg, 0);
            break;

        case 'v':
            dev->verbose++;
            break;

        case '?':
            if (optopt == '-') {
                ++optind;
                break;
            }
            return -1;

        case -1:
            if (prev_optind < optind) { /* -- */
                return -1;
            }

            if (argv[optind] == NULL) {
                done = 1;
                break;
            }
            if (*argv[optind] != '-') {
                ++optind;
                break;
            }
            return -1;

        case ':':
        default:
            return -1;
        }
    }

    return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/i2c/tda4/options.c $ $Rev: 992494 $")
#endif
