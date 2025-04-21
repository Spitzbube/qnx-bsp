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

/**
 * Process data in a line status register
 * @param   dev     pointer to seromap
 * @param   lsr     line status
 * @return  return value from tti().
 *                  1   event queue required
 *                  0   event queue not required
 */
static int
process_lsr(DEV_OMAP* const dev, const unsigned lsr)
{
    unsigned key = 0;

    /* Return immediately if no errors. */
    if ((lsr & (OMAP_LSR_BI | OMAP_LSR_OE | OMAP_LSR_FE | OMAP_LSR_PE)) == 0) {
        return (0);
    }

    /* Save the error as out-of-band data which can be retrieved via devctl(). */
    dev->tty.oband_data |= (lsr >> 1) & 0x0f;

    atomic_set(&dev->tty.flags, OBAND_DATA);

    /* Read whatever input data happens to be in the buffer to "eat" the
     * spurious data associated with break, parity error, etc.
     */
    key = read_omap(dev->port + OMAP_UART_RHR);

    if (lsr & OMAP_LSR_BI) {
        key |= TTI_BREAK;
    } else if (lsr & OMAP_LSR_OE) {
        key |= TTI_OVERRUN;
    } else if (lsr & OMAP_LSR_FE) {
        key |= TTI_FRAME;
    } else if (lsr & OMAP_LSR_PE) {
        key |= TTI_PARITY;
    }

    return tti(&dev->tty, key);
}

/**
 * Process UART interrupt
 * @param   dev     pointer to seromap
 * @return  none
 */
static void process_intr(DEV_OMAP *dev)
{
    int         status = 0;
    unsigned    msr, lsr;
    unsigned    iir;
    const uintptr_t   port = dev->port;
    bool        done = (bool)false;

    while (done == false) {
        iir = read_omap(port + OMAP_UART_IIR) & OMAP_II_MASK;

        switch (iir) {
            case OMAP_II_RX:        /* Receive data */
            case OMAP_II_RXTO:      /* Receive data timeout */
            case OMAP_II_LS:        /* Line status change */
                lsr = read_omap(port + OMAP_UART_LSR);
                do {
                    if (lsr & (OMAP_LSR_BI | OMAP_LSR_OE | OMAP_LSR_FE | OMAP_LSR_PE)) {
                        /* Error character */
                        status |= process_lsr(dev, lsr);
                    } else {
                        /* Good character */
                        status |= tti(&dev->tty, (read_omap(port + OMAP_UART_RHR)) & 0xff);
                    }
                    lsr = read_omap(port + OMAP_UART_LSR);
                } while (lsr & OMAP_LSR_RXRDY);
                break;

            case OMAP_II_TX:        /* Transmit buffer empty */
                /* disable thr interrupt */
                set_port(port + OMAP_UART_IER, OMAP_IER_THR, 0);

                dev->tty.un.s.tx_tmr = 0;
                /* Send event to io-char, tto() will be processed at thread time */
                atomic_set(&dev->tty.flags, EVENT_TTO);
                status |= 1;
                break;

            case OMAP_II_MS:        /* Modem change */
                msr = read_omap(port + OMAP_UART_MSR);

                if (msr & OMAP_MSR_DDCD) {
                    status |= tti(&dev->tty, (msr & OMAP_MSR_DCD) ? TTI_CARRIER : TTI_HANGUP);
                }

                if ((msr & OMAP_MSR_DCTS) && (dev->tty.c_cflag & OHFLOW)) {
                    status |= tti(&dev->tty, (msr & OMAP_MSR_CTS) ? TTI_OHW_CONT : TTI_OHW_STOP);
                }

                /* OBAND notification of Modem status change */
                dev->tty.oband_data |= _OBAND_SER_MS;
                atomic_set(&dev->tty.flags, OBAND_DATA);
                atomic_set(&dev->tty.flags, EVENT_NOTIFY_OBAND);
                status |= 1;
                break;

            case OMAP_II_NOINTR:    /* No interrupt */
                if (read_omap(port + OMAP_UART_SSR) & OMAP_SSR_WU_STS) {    /* Wake up interrupt */
                    set_port(port + OMAP_UART_SCR, OMAP_SCR_WAKEUPEN, 0);   /* clear wakeup interrupt */
                    set_port(port + OMAP_UART_SCR, OMAP_SCR_WAKEUPEN, OMAP_SCR_WAKEUPEN);   /* re-enable wakeup interrupt */
                }
            default:
                done = (bool)true;
                break;
        }
    }

    /*
     * Interrupt event handler does not return a tte event,
     * so we must issue the tte pulse manually
     */
    if ((status != 0) && ((dev->tty.flags & EVENT_QUEUED) == 0)) {
        iochar_send_event(&dev->tty);
    }
}

/**
 * Serial interrupt pulse handler
 * @param   msgctp  message context pointer
 * @param   code    pulse code
 * @param   flags   flags
 * @param   handle  handler pointer, serial device handle
 * @return  EOK
 */
int interrupt_event_handler(message_context_t *msgctp, int code, unsigned flags, void *handle)
{
    DEV_OMAP* const dev = handle;

    process_intr(dev);

    InterruptUnmask(dev->intr, dev->iid);

    return (EOK);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/devc/seromap/intr.c $ $Rev: 962454 $")
#endif
