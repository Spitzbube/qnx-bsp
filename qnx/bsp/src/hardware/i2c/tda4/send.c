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

#define TDA4_START_TX(dev, stop) out32(dev->regbase + TDA4_I2C_CON, \
                                    TDA4_I2C_CON_EN | TDA4_I2C_CON_MST | \
                                    TDA4_I2C_CON_TRX | TDA4_I2C_CON_STT | \
                                    (stop? TDA4_I2C_CON_STP : 0) | \
                                    (in32(dev->regbase + TDA4_I2C_CON) & TDA4_I2C_CON_XSA))

static void
tda4_fill_fifo(tda4_dev_t *dev)
{
    unsigned    nbytes = min(dev->fifo_size, dev->xlen);

    /* fill the fifo with outgoing data */
    while (nbytes) {
        out32(dev->regbase + TDA4_I2C_DATA, (uint32_t)(*dev->buf++));
        dev->xlen--;
        nbytes--;
    }
}

i2c_status_t
tda4_send(void *hdl, void *buf, unsigned int len, unsigned int stop)
{
    tda4_dev_t  *dev = hdl;

    if (len <= 0) {
        return I2C_STATUS_DONE;
    }

    if (-1 == tda4_wait_bus_not_busy(dev, stop)) {
        return I2C_STATUS_BUSY;
    }

    dev->xlen = len;
    dev->buf = buf;
    dev->status = 0;
    dev->intexpected = 1;

    /* set slave address */
    if (dev->slave_addr_fmt == I2C_ADDRFMT_7BIT) {
        out32(dev->regbase + TDA4_I2C_CON, in32(dev->regbase + TDA4_I2C_CON) & (~TDA4_I2C_CON_XSA));
    } else {
        out32(dev->regbase + TDA4_I2C_CON, in32(dev->regbase + TDA4_I2C_CON) | TDA4_I2C_CON_XSA);
    }

    out32(dev->regbase + TDA4_I2C_SA, dev->slave_addr);

    /* set data count */
    out32(dev->regbase + TDA4_I2C_CNT, len);

    /* Clear the FIFO Buffers */
    out32(dev->regbase + TDA4_I2C_BUF,
            in32(dev->regbase + TDA4_I2C_BUF) | TDA4_I2C_BUF_RXFIF_CLR | TDA4_I2C_BUF_TXFIF_CLR);

    if (dev->flags & TDA4_FLG_FIFO_PREFILL) {
        /* pre-fill the fifo with outgoing data */
        tda4_fill_fifo(dev);
        /* set start condition */
        TDA4_START_TX(dev, stop);
    } else {
        // guard transfer start and FIFO write to prevent Tx FIFO under run
        intrspin_t spinlock = { 0 };
        InterruptLock(&spinlock);
        /* set start condition */
        TDA4_START_TX(dev, stop);
        /* fill FIFO with outgoing data */
        tda4_fill_fifo(dev);
        InterruptUnlock(&spinlock);
    }

    /* Eanble interrupts */
    out32(dev->regbase + TDA4_I2C_IES, TDA4_I2C_IE_MASK);

    return tda4_wait_complete(dev);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/i2c/tda4/send.c $ $Rev: 961968 $")
#endif
