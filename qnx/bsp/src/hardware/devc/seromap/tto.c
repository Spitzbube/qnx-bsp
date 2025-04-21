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
 * io-char tto callout
 * @param   dev     pointer to TTY device
 * @param   action  TTO action
 * @param   arg     TTO argument
 * @return  1 if event queue is required, 0 otherwise
 */
int
tto(TTYDEV *dev, int action, int arg)
{
    const TTYBUF* const bup = &dev->obuf;
    DEV_OMAP        *omap = (DEV_OMAP *)dev;
    const uintptr_t port = omap->port;
    unsigned char   c;

    switch (action) {
        case TTO_STTY:
            ser_stty(omap);
            return (0);

        case TTO_CTRL:
            if (arg & _SERCTL_BRK_CHG) {
                set_port(port + OMAP_UART_LCR, OMAP_LCR_BREAK, (arg &_SERCTL_BRK) ? OMAP_LCR_BREAK : 0);
            }

            if (arg & _SERCTL_DTR_CHG) {
                set_port(port + OMAP_UART_MCR, OMAP_MCR_DTR, (arg & _SERCTL_DTR) ? OMAP_MCR_DTR : 0);
            }

            if (arg & _SERCTL_RTS_CHG) {
                if (omap->auto_rts_enable) {
                    /* For auto-rts enable/disable RX & LS interrupts to assert/clear
                     * input flow control (the FIFO will automatically handle the RTS line)
                     */
                    if (arg & _SERCTL_RTS) {
                        write_omap(port + OMAP_UART_IER, read_omap(port + OMAP_UART_IER) | OMAP_IER_RHR | OMAP_IER_LS);
                    } else {
                        write_omap(port + OMAP_UART_IER, read_omap(port + OMAP_UART_IER) & ~(OMAP_IER_RHR | OMAP_IER_LS));
                    }
                } else {
                    set_port(port + OMAP_UART_MCR, OMAP_MCR_RTS, (arg & _SERCTL_RTS) ? OMAP_MCR_RTS : 0);
                }
            }
            return (0);

        case TTO_LINESTATUS:
            return (((read_omap(port + OMAP_UART_MSR) << 8) | read_omap(port + OMAP_UART_MCR)) & 0xf003);

        case TTO_DATA:
        case TTO_EVENT:
            break;

        default:
            return (0);
    }

    while ((bup->cnt > 0) && !(read_omap(port + OMAP_UART_SSR) & OMAP_SSR_TXFULL)) {
        /*
         * If the OSW_PAGED_OVERRIDE flag is set then allow
         * transmit of character even if output is suspended via
         * the OSW_PAGED flag. This flag implies that the next
         * character in the obuf is a software flow control
         * charater (STOP/START).
         * Note: tx_inject sets it up so that the contol
         * character is at the start (tail) of the buffer.
         */
        if ((omap->tty.flags & (OHW_PAGED | OSW_PAGED))
                && !(omap->tty.xflags & OSW_PAGED_OVERRIDE)) {
            break;
        }

        /* Get character from obuf and do any output processing */
        dev_lock(&omap->tty);
        c = tto_getchar(&omap->tty);
        dev_unlock(&omap->tty);

        /* Print the character */
        omap->tty.un.s.tx_tmr = 3;   /* Timeout 3 */
        write_omap(port + OMAP_UART_THR, (uint32_t)c);

        /* Clear the OSW_PAGED_OVERRIDE flag as we only want
         * one character to be transmitted in this case.
         */
        if ((omap->tty.xflags & OSW_PAGED_OVERRIDE)) {
            atomic_clr(&omap->tty.xflags, OSW_PAGED_OVERRIDE);
            break;
        }
    }

    /* If there is still data in the obuf and we are not in a flow
     * controlled state then turn TX interrupts back on to notify us
     * when the hardware is ready for more characters.
     */
    if ((bup->cnt > 0) && !(omap->tty.flags & (OHW_PAGED | OSW_PAGED))) {
        /* enable all Tx interrupt */
        set_port(port + OMAP_UART_IER, OMAP_IER_THR, OMAP_IER_THR);
    }

    /* Check the client lists for notify conditions */
    return (tto_checkclients(&omap->tty));
}

/**
 * TTY configuration
 * @param   omap     pointer to OMAP UART device
 * @return  none
 */
void
ser_stty(DEV_OMAP *dev)
{
    unsigned        lcr = 0;
    unsigned        efr = 0;
    const uintptr_t port = dev->port;
    unsigned        brd = 0;
    unsigned        multiple;

    /* determine data bits */
    switch (dev->tty.c_cflag & CSIZE) {
        case CS8: ++lcr;
            /* fallthrough */
        case CS7: ++lcr;
            /* fallthrough */
        case CS6: ++lcr;
    }

    /* determine stop bits */
    if (dev->tty.c_cflag & CSTOPB) {
        lcr |= OMAP_LCR_STB2;
    }
    /* determine parity bits */
    if (dev->tty.c_cflag & PARENB) {
        lcr |= OMAP_LCR_PEN;
    }
    if ((dev->tty.c_cflag & PARODD) == 0) {
        lcr |= OMAP_LCR_EPS;
    }
    if (dev->tty.c_cflag & OHFLOW) {
        efr = OMAP_EFR_AUTO_CTS;
    }
    if ((dev->tty.c_cflag & IHFLOW) && (dev->auto_rts_enable)) {
        efr |= OMAP_EFR_AUTO_RTS;
    }

    /* Apply EFR value if changed */
    if (dev->efr != efr) {
        /* Switch to Config mode B to access the Enhanced Feature Register (EFR) */
        write_omap(port + OMAP_UART_LCR, 0xbf);
        /* turn off S/W flow control, Config AUTO hw flow control, enable writes to MCR[7:5], FCR[5:4], and IER[7:4] */
        set_port(port + OMAP_UART_EFR, efr, efr);
        /* Switch back to operational mode */
        write_omap(port + OMAP_UART_LCR, 0);
        /* Restore LCR config values */
        write_omap(port + OMAP_UART_LCR, lcr);
        dev->lcr = lcr;
        dev->efr = efr;
    }

    /* Change baud rate */
    if (dev->tty.baud != (int)dev->baud) {
        /* Get acces to Divisor Latch registers */
        write_omap(port + OMAP_UART_LCR, OMAP_LCR_DLAB);

        /*
         * TRM states: Before initializing or modifying clock parameter controls
         * (DLH, DLL), MODE_SELECT must be set to 0x7 (DISABLE). Failure to observe
         * this rule can result in unpredictable module behavior.
         */
        set_port(port + OMAP_UART_MDR1, OMAP_MDR1_MODE_MSK, OMAP_MDR1_MODE_DISABLE);    /* Disable UART */

        multiple = (dev->tty.baud > 230400) ? 13u : 16u;

        brd = (dev->tty.baud == 0) ? 0 : (dev->clk / (multiple * (unsigned)dev->tty.baud));
        write_omap(port + OMAP_UART_DLL, brd);
        write_omap(port + OMAP_UART_DLH, (brd >> 8) & 0xff);

        /*
         * Restore LCR config values, before enabling UART. Otherwise bad things can
         * happen if RX occurs between MODE_SELECT and LCR.
         */
        write_omap(port + OMAP_UART_LCR, lcr);
        if (multiple == 13u) {
            set_port(port + OMAP_UART_MDR1, OMAP_MDR1_MODE_MSK, OMAP_MDR1_MODE_13X); /* Enable UART in 13x mode */
        } else {
            set_port(port + OMAP_UART_MDR1, OMAP_MDR1_MODE_MSK, OMAP_MDR1_MODE_16X); /* Enable UART in 16x mode */
        }

        dev->lcr = lcr;
        dev->baud = (unsigned)dev->tty.baud;
        dev->brd = brd;
    }

    /* Apply LCR value if changed */
    if (dev->lcr != lcr) {
        write_omap(port + OMAP_UART_LCR, lcr);
        dev->lcr = lcr;
    }
}

/**
 * drain check
 * @param   dev  pointer to TTY device
 * @param   count   pointer to drain check timer
 * @return  1 if UART is drained, 0 therwise
 */
int drain_check(TTYDEV *dev, uintptr_t *cnt)
{
    const TTYBUF* const bup = &dev->obuf;
    const DEV_OMAP* const omap = (DEV_OMAP *)dev;
    const uintptr_t port = omap->port;

    /* if the device has DRAINED, return 1 */
    if ((bup->cnt == 0) &&
        (read_omap(port + OMAP_UART_LSR) & OMAP_LSR_TSRE)) return 1;

    /* if the device has not DRAINED, set a timer based on 50ms counts
     * wait for the time it takes for one character to be transmitted
     * out the shift register. We do this dynamically since the
     * baud rate can change.
     */
    if (cnt != NULL) {
        if (dev->baud == 0) {
            *cnt = 0;
        } else {
            *cnt = (uintptr_t)(((IO_CHAR_DEFAULT_BITSIZE * 20) / dev->baud) + 1);
        }
    }

    return (0);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/devc/seromap/tto.c $ $Rev: 962454 $")
#endif
