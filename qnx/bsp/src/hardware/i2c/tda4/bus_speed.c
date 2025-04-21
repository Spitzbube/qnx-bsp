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

#define SCLL_BIAS   7
#define SCLH_BIAS   5


int
tda4_set_bus_speed(void *hdl, unsigned speed, unsigned *ospeed)
{
    tda4_dev_t  *dev = hdl;
    int         iclk;
    int         scll_plus_sclh;
    int         high_adjust;
    int         low_adjust;
    int         scll_to_write;
    int         sclh_to_write;
    unsigned    psc;

    /* This driver support bus speed range from 8KHz to 400KHz
     * limit the low bus speed to 8KHz to protect SCLL/SCLH from overflow(large than 0xff)
     * if speed=8KHz, iclk=4MHz, then SCLL=0xf3, SCLH=0xf5
     */
    if ((speed > TDA4_SPEED_MAX) || (speed < TDA4_SPEED_MIN)) {
        i2c_slogf(dev->verbose, _SLOG_ERROR, "%s: i2c-tda4:  Invalid bus speed(%d)", __func__, speed);
        errno = EINVAL;
        return -1;
    }

    /* Set the I2C prescaler register to obtain the maximum I2C bit rates
     * and the maximum period of the filtered spikes in F/S mode:
     * Stander Mode: I2Ci_INTERNAL_CLK = 4 MHz
     * Fast Mode:    I2Ci_INTERNAL_CLK = 9.6 MHz
     */
    if (speed <= TDA4_SPEED_STANDARD) {
        psc = dev->pclk / TDA4_I2C_ICLK_4000K;
        high_adjust = dev->high_adjust_slow;
        low_adjust  = dev->low_adjust_slow;
    } else {
        psc = dev->pclk / TDA4_I2C_ICLK_9600K;
        high_adjust = dev->high_adjust_fast;
        low_adjust = dev->low_adjust_fast;
    }

    iclk = (int)(dev->pclk / psc);

    /* Set clock based on "speed" bps */
    scll_plus_sclh = (iclk / (int)speed - (SCLL_BIAS + SCLH_BIAS));

    scll_to_write = ((scll_plus_sclh / 2) - 1);
    sclh_to_write = (scll_plus_sclh - scll_to_write);
    scll_to_write += low_adjust;
    sclh_to_write += high_adjust;

    if (scll_to_write < 0) {
        scll_to_write = 0;
    }
    if (sclh_to_write < 3) {
        sclh_to_write = 3;
    }

    out32(dev->regbase + TDA4_I2C_PSC, psc - 1);
    out32(dev->regbase + TDA4_I2C_SCLL, (uint32_t)scll_to_write);
    out32(dev->regbase + TDA4_I2C_SCLH, (uint32_t)sclh_to_write);

    dev->speed = (unsigned)(iclk / (scll_to_write + SCLL_BIAS + sclh_to_write + SCLH_BIAS));

    if (ospeed) {
        *ospeed = dev->speed;
    }

    return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/i2c/tda4/bus_speed.c $ $Rev: 992494 $")
#endif
