/*
 * Copyright (c) 2014, 2022, BlackBerry Limited.
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


int tda4_i2c_reset(tda4_dev_t *dev)
{
    int     timeout = 2000;

    /* Disable interrupts */
    out32(dev->regbase + TDA4_I2C_IEC, TDA4_I2C_INT_ALL);
    /* Disable DMA */
    out32(dev->regbase + TDA4_I2C_BUF, 0);
    /* Disable test mode */
    out32(dev->regbase + TDA4_I2C_SYSTEST, 0);

    /* Reset module */
    if (in32(dev->regbase + TDA4_I2C_CON) & TDA4_I2C_CON_EN) {
        out32(dev->regbase + TDA4_I2C_CON, 0);
    }
    out32(dev->regbase + TDA4_I2C_CON, TDA4_I2C_CON_EN);
    /* poll for reset complete */
    while ((in32(dev->regbase + TDA4_I2C_SYSS) & TDA4_I2C_SYSS_RDONE) == 0) {
        if (--timeout <= 0) {
            i2c_slogf(dev->verbose, _SLOG_ERROR,
                "%s: timed out: TDA4_I2C_SYSS %x", __func__, in32(dev->regbase + TDA4_I2C_SYSS));
            return -1;
        }
        delay(1);
    }

    out32(dev->regbase + TDA4_I2C_CON, 0);

    /* Set I2C bus speed */
    tda4_set_bus_speed(dev, dev->speed, NULL);

    /* Set FIFO for Tx/RX Note: setup required fifo size - 1*/
    out32(dev->regbase + TDA4_I2C_BUF, ((dev->fifo_size - 1) << 8) |/* RTRSH */
                                        TDA4_I2C_BUF_RXFIF_CLR |
                                       (dev->fifo_size - 1) |       /* XTRSH */
                                        TDA4_I2C_BUF_TXFIF_CLR);

    /* Set Own Address */
    out32(dev->regbase + TDA4_I2C_OA, dev->own_addr);
    /* Take module out of reset */
    out32(dev->regbase + TDA4_I2C_CON, TDA4_I2C_CON_EN);
    delay(1);

    /* Check bus busy */
    if (in32(dev->regbase + TDA4_I2C_IS) & TDA4_I2C_INT_BB) {
        i2c_slogf(dev->verbose, _SLOG_ERROR,
            "%s: Bus busy after reset, try to recover it by sending SCK and STOP on the bus", __func__);

        /* clear the status register */
        out32(dev->regbase + TDA4_I2C_IS, in32(dev->regbase + TDA4_I2C_IS));

        delay(1);

        /* Check bus busy again */
        if (in32(dev->regbase + TDA4_I2C_IS) & TDA4_I2C_INT_BB) {
            i2c_slogf(dev->verbose, _SLOG_ERROR,
                "%s: Bus still busy after bus recovering", __func__);
            return -1;
        }
    }

    return 0;
}

int
tda4_bus_reset(void *hdl)
{
    return tda4_i2c_reset(hdl);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/i2c/tda4/reset.c $ $Rev: 992494 $")
#endif
