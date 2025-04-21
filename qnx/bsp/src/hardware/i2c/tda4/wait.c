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

#define TDA4_I2C_IS_MASK \
            (TDA4_I2C_INT_XUDF | \
             TDA4_I2C_INT_RDR  | \
             TDA4_I2C_INT_XDR  | \
             TDA4_I2C_INT_ROVR | \
             TDA4_I2C_INT_AAS  | \
             TDA4_I2C_INT_GC   | \
             TDA4_I2C_INT_XRDY | \
             TDA4_I2C_INT_RRDY | \
             TDA4_I2C_INT_ARDY | \
             TDA4_I2C_INT_NACK | \
             TDA4_I2C_INT_AL)


int
tda4_wait_bus_not_busy(tda4_dev_t *dev, const unsigned stop)
{
    unsigned    tries = 1000000;

    if (dev->re_start) {
        if (stop) {
            dev->re_start = 0;
        }
    } else {
        while (in32(dev->regbase + TDA4_I2C_IS) & TDA4_I2C_INT_BB) {
            if (tries-- == 0) {
                /* reset the controller to see if it's able to recover */
                if (tda4_i2c_reset(dev) == -1) return -1;

                break;
            }
        }
        /* The I2C_STAT register should be 0, otherwise reset the I2C interface */
        if (in32(dev->regbase + TDA4_I2C_IS) != 0) {
            if (tda4_i2c_reset(dev) == -1) return -1;
        }
        if (!stop) {
            dev->re_start = 1;
        }
    }

    return 0;
}

static int tda4_process_intr(tda4_dev_t *dev)
{
    uint32_t    stat;
    uint32_t    transmit_stat = (TDA4_I2C_INT_XRDY | TDA4_I2C_INT_XDR);
    uint32_t    receive_stat = (TDA4_I2C_INT_RRDY | TDA4_I2C_INT_RDR);

    if (dev->flags & TDA4_FLG_ROVR_XUDF_OK) {
        transmit_stat |= TDA4_I2C_INT_XUDF;
        receive_stat |= TDA4_I2C_INT_ROVR;
    }

    if ((stat = in32(dev->regbase + TDA4_I2C_IS)) & TDA4_I2C_IS_MASK) {
        /* check errors and transaction done */
        if (stat & (TDA4_I2C_INT_NACK | TDA4_I2C_INT_AL)) {
            if (stat & TDA4_I2C_INT_NACK) {
                dev->status |= I2C_STATUS_NACK;
            }
            if (stat & TDA4_I2C_INT_AL) {
                /* when we get arbitration lost ARDY is not present, so force DONE */
                dev->status |= I2C_STATUS_ARBL | I2C_STATUS_DONE;
            }
            out32(dev->regbase + TDA4_I2C_CON,
                   in32(dev->regbase + TDA4_I2C_CON) | TDA4_I2C_CON_STP);

            stat = in32(dev->regbase + TDA4_I2C_IS);

            dev->re_start = 0;
        }

        if ((dev->flags & TDA4_FLG_ROVR_XUDF_OK) == 0) {
            if (stat & (TDA4_I2C_INT_ROVR | TDA4_I2C_INT_XUDF)) {
                dev->status |= I2C_STATUS_ERROR;
                out32(dev->regbase + TDA4_I2C_CON,
                       in32(dev->regbase + TDA4_I2C_CON) | TDA4_I2C_CON_STP);

                stat = in32(dev->regbase + TDA4_I2C_IS);

                dev->re_start = 0;
            }
        }

        if (stat & TDA4_I2C_INT_ARDY) {
            dev->status |= I2C_STATUS_DONE;
        }

        /* check receive interrupt */
        if (stat & receive_stat) {
            uint32_t    num_bytes = 1;
            if (dev->fifo_size) {
                if (stat & TDA4_I2C_INT_RRDY) {
                    num_bytes = dev->fifo_size;
                } else {
                    num_bytes = (in32(dev->regbase + TDA4_I2C_BUFSTAT) & TDA4_I2C_BUFSTAT_RXSTAT) >> 8;
                }
            }
            while (num_bytes--) {
                if (dev->xlen == 0) break;

                *dev->buf++ = (uint8_t)(in32(dev->regbase + TDA4_I2C_DATA) & 0x000000FF);
                dev->xlen--;
            }
        }

        /* check transmit interrupt */
        if (stat & transmit_stat) {
            uint32_t    num_bytes = 1;
            if (dev->fifo_size) {
                if (stat & TDA4_I2C_INT_XRDY) {
                    num_bytes = dev->fifo_size;
                } else {
                    num_bytes = in32(dev->regbase + TDA4_I2C_BUFSTAT) & TDA4_I2C_BUFSTAT_TXSTAT;
                }
            }
            while (num_bytes--) {
                if (dev->xlen == 0) break;

                out32(dev->regbase + TDA4_I2C_DATA, (uint32_t)(*dev->buf++));
                dev->xlen--;
            }
        }
    }

    /* clear the status */
    out32(dev->regbase + TDA4_I2C_IS, stat);

    /* check transaction done */
    if ((dev->status & I2C_STATUS_DONE) && dev->intexpected) {
        dev->intexpected = 0;
        return ((dev->status & ~I2C_STATUS_DONE) == 0) ? EOK : EIO;
    }

    return EAGAIN;
}

i2c_status_t
tda4_wait_complete(tda4_dev_t *dev)
{
    i2c_status_t    sts = 0;
    uint64_t        ntime;
    int             err;

    /* ntime should be at least 9 times because of 8-bit data + 1-bit ack */
    ntime = (1000000000ul) * (uint64_t)(dev->fifo_size + 1) * 10 / (uint64_t)dev->speed;

    while (1) {
        TimerTimeout(CLOCK_MONOTONIC, _NTO_TIMEOUT_INTR, NULL, &ntime, NULL);
        err = InterruptWait(0, NULL);

        if (err == -1) {
            i2c_slogf(dev->verbose, _SLOG_ERROR,
                "%s: InterruptWait: %s(%d)", __func__, strerror(errno), errno);
            tda4_i2c_reset(dev);
            return I2C_STATUS_BUSY;
        }

        err = tda4_process_intr(dev);
        InterruptUnmask(dev->intr, dev->iid);

        if (err != EAGAIN) {
            sts |= dev->status;
            if (dev->status & (I2C_STATUS_ARBL | I2C_STATUS_ERROR)) {
                tda4_i2c_reset(dev);
            }
            break;
        }
    }

    out32(dev->regbase + TDA4_I2C_IEC, TDA4_I2C_IE_MASK);

    return sts;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/i2c/tda4/wait.c $ $Rev: 992494 $")
#endif
