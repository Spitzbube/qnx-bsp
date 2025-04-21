/*
 * Copyright (c) 2008, 2023, BlackBerry Limited.
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
#include <sys/mman.h>

/**
 * Set/clear certain bits of UART register
 * @param   port    UART register address
 * @param   mask    mask bits
 * @param   data    set bits
 * @return  none
 */
void
set_port(const uintptr_t regadr, const unsigned mask, const unsigned data)
{
    unsigned c;

    c = read_omap(regadr);
    write_omap(regadr, (c & ~mask) | (data & mask));
}

/**
 * Clean up the device
 * @param   port    UART port address
 * @return  none
 */
static void
clear_device(const uintptr_t port)
{
    write_omap(port + OMAP_UART_IER, 0);    /* Disable all interrupts */
    /* Clear FIFOs */
    set_port(port + OMAP_UART_FCR, OMAP_FCR_RXCLR | OMAP_FCR_TXCLR, OMAP_FCR_RXCLR | OMAP_FCR_TXCLR);
    read_omap(port + OMAP_UART_LSR);        /* Clear Line Status Interrupt */
    read_omap(port + OMAP_UART_MSR);        /* Clear Modem Interrupt */
}

/**
 * attach to interrupt and enable it.
 * @param   port    UART port address
 * @return  EOK if success, -1 otherwise
 */
static int
ser_attach_intr(DEV_OMAP *dev)
{
    const uintptr_t   port = dev->port;
    /* interrupt sources except transmit and modem status interrupt */
    unsigned    ier = OMAP_IER_RHR | OMAP_IER_LS;

    /* According to the National bug sheet you must wait for the transmit
     * holding register to be empty.
     */
    do {
    } while ((read_omap(port + OMAP_UART_LSR) & OMAP_LSR_TXRDY) == 0);

    clear_device(port);

    struct sigevent event;
    int             sigevcode;
    const int       pulse_prio = min(ttyctrl.event.sigev_priority + 1, sched_get_priority_max(SCHED_RR));

    /* Associate a pulse which will call the event handler. */
    sigevcode = pulse_attach(ttyctrl.dpp,
            MSG_FLAG_ALLOC_PULSE, 0, &interrupt_event_handler, dev);
    if (sigevcode == -1) {
        omap_slogf(ERROR, "io-char: Unable to attach event pulse(%d)", errno);
        return (-1);
    }

    /* Use TTC pulse priority + 1 for interrupt event pulse. Make sure it does not exceed max prio */
    SIGEV_PULSE_INIT(&event, ttyctrl.coid, (short)pulse_prio, (short)sigevcode, 0);
    dev->iid = InterruptAttachEvent(dev->intr, &event, _NTO_INTR_FLAGS_TRK_MSK);

    if (dev->iid == -1) {
        omap_slogf(ERROR, "io-char: Unable to attach to interrupt %d (%d)", dev->intr, errno);
        return (-1);
    }

    /* Enable modem status interrupt (default) */
    if (!dev->no_msr_int) {
        ier |= OMAP_IER_MS;
    }
    /* Enable interrupt sources. */
    write_omap(port + OMAP_UART_IER, ier);

    return (EOK);
}

/**
 * create UART device.
 * @param   dip     device initialization pointer
 * @return  device handle if success, NULL otherwise
 */
DEV_OMAP *
create_device(const TTYINIT_OMAP* const dip)
{
    DEV_OMAP    *dev;
    uintptr_t   port;
    unsigned    msr;
    unsigned    tcr = 0;
    unsigned    tlr = 0;
    unsigned    scr = 0;
    unsigned    fcr = OMAP_FCR_RXCLR | OMAP_FCR_TXCLR;

    if ((dip->ttyinit.port == 0) || (dip->ttyinit.intr == 0)) {
        omap_slogf(ERROR, "io-char: Invalid port [%lx] or IRQ [%d]", dip->ttyinit.port, dip->ttyinit.intr);
        return (NULL);
    }

    /* Get a device entry and the input/output buffers for it. */
    dev = malloc(sizeof(*dev));
    if (dev == NULL) {
        omap_slogf(ERROR, "io-char: Allocation of device entry failed (%d)", errno);
        return (dev);
    }
    memset(dev, 0, sizeof(*dev));

    /* Get buffers. */
    dev->tty.ibuf.buff = malloc((size_t)dip->ttyinit.isize);
    if (dev->tty.ibuf.buff == NULL) {
        omap_slogf(ERROR, "io-char: Allocation of input buffer failed (%d)", errno);
        free(dev);
        return (NULL);
    }
    dev->tty.ibuf.size = dip->ttyinit.isize;
    dev->tty.ibuf.tail = dev->tty.ibuf.buff;
    dev->tty.ibuf.head = dev->tty.ibuf.buff;

    dev->tty.obuf.buff = malloc((size_t)dip->ttyinit.osize);
    if (dev->tty.obuf.buff == NULL) {
        omap_slogf(ERROR, "io-char: Allocation of output buffer failed (%d)", errno);
        free(dev->tty.ibuf.buff);
        free(dev);
        return (NULL);
    }
    dev->tty.obuf.size = dip->ttyinit.osize;
    dev->tty.obuf.head = dev->tty.obuf.buff;
    dev->tty.obuf.tail = dev->tty.obuf.buff;

    dev->tty.cbuf.buff = malloc((size_t)dip->ttyinit.csize);
    if (dev->tty.cbuf.buff == NULL) {
        omap_slogf(ERROR, "io-char: Allocation of canonical buffer failed (%d)", errno);
        free(dev->tty.ibuf.buff);
        free(dev->tty.obuf.buff);
        free(dev);
        return (NULL);
    }
    dev->tty.cbuf.size = dip->ttyinit.csize;
    dev->tty.cbuf.head = dev->tty.cbuf.buff;
    dev->tty.cbuf.tail = dev->tty.cbuf.buff;

    if (dip->ttyinit.highwater) {
        dev->tty.highwater = dip->ttyinit.highwater;
    } else {
        dev->tty.highwater = dev->tty.ibuf.size - (OMAP_FIFO_SIZE * 2);
    }

    strcpy(dev->tty.name, dip->ttyinit.name);

    dev->tty.baud = dip->ttyinit.baud;
    dev->tty.verbose = dip->ttyinit.verbose;

    port = mmap_device_io(OMAP_UART_SIZE, dip->ttyinit.port);
    if (port == (uintptr_t)MAP_FAILED) {
        omap_slogf(ERROR, "io-char: port map failed (%d)", errno);
        free(dev->tty.cbuf.buff);
        free(dev->tty.ibuf.buff);
        free(dev->tty.obuf.buff);
        free(dev);
        return (NULL);
    }

    dev->port = port;

    dev->intr = (int)dip->ttyinit.intr;
    dev->clk = (unsigned)dip->ttyinit.clk;

    dev->tty.flags = EDIT_INSERT | LOSES_TX_INTR;
    dev->tty.c_cflag = dip->ttyinit.c_cflag;
    dev->tty.c_iflag = dip->ttyinit.c_iflag;
    dev->tty.c_lflag = dip->ttyinit.c_lflag;
    dev->tty.c_oflag = dip->ttyinit.c_oflag;
    dev->tty.lflags = dip->ttyinit.lflags;

    /* Set auto_rts mode */
    dev->auto_rts_enable = dip->auto_rts_enable;

    /* Do not enable MSR interrupt */
    dev->no_msr_int = dip->no_msr_int;

    /* Initialize termios cc codes to an ANSI terminal. */
    ttc(TTC_INIT_CC, &dev->tty, 0);

    /* Initialize the device's name.
     * Assume that the basename is set in device name. This will attach
     * to the path assigned by the unit number/minor number combination
     */
    ttc(TTC_INIT_TTYNAME, &dev->tty, (int)(SET_NAME_NUMBER(dip->ttyinit.unit) | NUMBER_DEV_FROM_USER));

    /*
     * TRM states: Before initializing or modifying clock parameter controls
     * (DLH, DLL), MODE_SELECT must be set to 0x7 (DISABLE). Failure to observe
     * this rule can result in unpredictable module behavior.
     * MDR1 is re-enabled  MDR1 later when ser_stty() is called
     */
    write_omap(port + OMAP_UART_MDR1, OMAP_MDR1_MODE_DISABLE);

    /* Enable access to divisor registers */
    write_omap(port + OMAP_UART_LCR, OMAP_LCR_DLAB);
    write_omap(port + OMAP_UART_DLL, 0);
    write_omap(port + OMAP_UART_DLH, 0);

    /* Switch to config mode B to get access to EFR Register */
    write_omap(port + OMAP_UART_LCR, 0xBFu);
    /* Enable access to TLR register */
    set_port(port + OMAP_UART_EFR, OMAP_EFR_ENHANCED, OMAP_EFR_ENHANCED);
    /* Switch to operational mode to get acces to MCR register */
    write_omap(port + OMAP_UART_LCR, 0x00);
    /* Set MCR bit 6 to enable access to TCR and TLR registers */
    set_port(port + OMAP_UART_MCR, OMAP_MCR_TCRTLR, OMAP_MCR_TCRTLR);

    if (dip->auto_rts_enable) {
        tcr = (dip->rxfifo > 56) ? 0x0fu : 0x0eu;
    }

    if ((dip->rxfifo != 0) || (dip->txfifo != 0)) {
        fcr |= OMAP_FCR_ENABLE;
        fcr |= (dip->rxfifo & 3) << 6;  /* Rx FIFO trigger level, 2 LSB */
        fcr |= (dip->txfifo & 3) << 4;  /* Tx FIFO trigger level, 2 LSB */
        scr |= OMAP_SCR_TTG1 | OMAP_SCR_RTG1;
        tlr |= (dip->rxfifo >> 2) << 4; /* Rx FIFO trigger level, 4 MSB */
        tlr |= (dip->txfifo >> 2);      /* Tx FIFO trigger level, 4 MSB */
    }

    write_omap(port + OMAP_UART_FCR, fcr);
    write_omap(port + OMAP_UART_TCR, tcr);
    write_omap(port + OMAP_UART_SCR, scr);
    write_omap(port + OMAP_UART_TLR, tlr);
    /* Switch back to Config mode B to gain access to EFR again */
    write_omap(port + OMAP_UART_LCR, 0xBF);
    /* Disable access to TLR register */
    set_port(port + OMAP_UART_EFR, OMAP_EFR_ENHANCED, 0);
    /* Switch to operational mode to get acces to MCR register */
    write_omap(port + OMAP_UART_LCR, 0x00);
    /* Clear MCR bit 6 to disable access to TCR and TLR registers */
    set_port(port + OMAP_UART_MCR, OMAP_MCR_TCRTLR, 0);

    ser_stty(dev);
    if (ser_attach_intr(dev) != EOK) {
        free(dev->tty.cbuf.buff);
        free(dev->tty.ibuf.buff);
        free(dev->tty.obuf.buff);
        free(dev);
        return (NULL);
    }

    /* tty.fifo type is unsigned char */
    dev->tty.fifo = (unsigned char)(((min(15u, dip->rxfifo)) << 4) | min(15u, dip->txfifo));

    /* Set modem control lines to a ready state */
    if (dev->auto_rts_enable) {
        set_port(port + OMAP_UART_MCR, OMAP_MCR_DTR, OMAP_MCR_DTR);
    } else {
        set_port(port + OMAP_UART_MCR, OMAP_MCR_DTR | OMAP_MCR_RTS, OMAP_MCR_DTR | OMAP_MCR_RTS);
    }

    /* Config loopback mode */
    if (dip->loopback) {
        set_port(port + OMAP_UART_MCR, OMAP_MCR_LOOPBACK, OMAP_MCR_LOOPBACK);
    } else {
        set_port(port + OMAP_UART_MCR, OMAP_MCR_LOOPBACK, 0);
    }

    /* Modem status change? */
    msr = read_omap(port + OMAP_UART_MSR);

    if ((msr & OMAP_MSR_DDCD)) {
        tti(&dev->tty, (msr & OMAP_MSR_DCD) ? TTI_CARRIER : TTI_HANGUP);
    }

    if ((msr & OMAP_MSR_DCTS) && (dev->tty.c_cflag & OHFLOW)) {
        tti(&dev->tty, (msr & OMAP_MSR_CTS) ? TTI_OHW_CONT : TTI_OHW_STOP);
    }

    /* Attach the resource manager */
    ttc(TTC_INIT_ATTACH, &dev->tty, 0);

    return (dev);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/devc/seromap/init.c $ $Rev: 987518 $")
#endif
