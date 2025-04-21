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

#ifndef PROTO_H
#define PROTO_H

#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <atomic.h>
#include <sys/dispatch.h>
#include <sys/dcmd_chr.h>
#include <sys/io-char.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>
#include <hw/inout.h>

#define OMAP_UART_SIZE          0xB4

#define OMAP_UART_RHR           0x00
#define OMAP_UART_THR           0x00
#define OMAP_UART_DLL           0x00
#define OMAP_UART_IER           0x04
#define OMAP_UART_DLH           0x04
#define OMAP_UART_IIR           0x08
#define OMAP_UART_FCR           0x08
#define OMAP_UART_EFR           0x08
#define OMAP_UART_LCR           0x0C
#define OMAP_UART_MCR           0x10
#define OMAP_UART_XON1          0x10
#define OMAP_UART_ADDR1         0x10
#define OMAP_UART_LSR           0x14
#define OMAP_UART_XON2          0x14
#define OMAP_UART_ADDR2         0x14
#define OMAP_UART_MSR           0x18
#define OMAP_UART_TCR           0x18
#define OMAP_UART_XOFF1         0x18
#define OMAP_UART_SPR           0x1C
#define OMAP_UART_TLR           0x1C
#define OMAP_UART_XOFF2         0x1C
#define OMAP_UART_MDR1          0x20
#define OMAP_UART_MDR2          0x24
#define OMAP_UART_SFLSR         0x28
#define OMAP_UART_TXFLL         0x28
#define OMAP_UART_RESUME        0x2C
#define OMAP_UART_TXFLH         0x2C
#define OMAP_UART_SFREGL        0x30
#define OMAP_UART_RXFLL         0x30
#define OMAP_UART_SFREGH        0x34
#define OMAP_UART_RXFLH         0x34
#define OMAP_UART_BLR           0x38
#define OMAP_UART_UASR          0x38
#define OMAP_UART_ACREG         0x3C
#define OMAP_UART_DIV1_6        0x3C
#define OMAP_UART_SCR           0x40
#define OMAP_UART_SSR           0x44
#define OMAP_UART_EBLR          0x48
#define OMAP_UART_OSC_12M_SEL   0x4C

/* Bit definitions for interrupt identification */
#define OMAP_II_NOINTR          0x01u
#define OMAP_II_MS              0x00u
#define OMAP_II_TX              0x02u
#define OMAP_II_RX              0x04u
#define OMAP_II_RXTO            0x0Cu
#define OMAP_II_LS              0x06u
#define OMAP_II_MASK            0x3Fu
#define OMAP_II_FIFO            0x80u

/* Bit definitions for line control */
#define OMAP_LCR_BITS_MASK      0x03u
#define OMAP_LCR_STB2           0x04u
#define OMAP_LCR_PEN            0x08u
#define OMAP_LCR_EPS            0x10u
#define OMAP_LCR_SPS            0x20u
#define OMAP_LCR_BREAK          0x40u
#define OMAP_LCR_DLAB           0x80u

/* Bit definitions for modem control */
#define OMAP_MCR_DTR            0x01u
#define OMAP_MCR_RTS            0x02u
#define OMAP_MCR_CDSTSCH        0x08u
#define OMAP_MCR_LOOPBACK       0x10u
#define OMAP_MCR_XON            0x20u
#define OMAP_MCR_TCRTLR         0x40u
#define OMAP_MCR_CLKSEL         0x80u

/* Bit definitions for line status */
#define OMAP_LSR_RXRDY          0x01u
#define OMAP_LSR_OE             0x02u
#define OMAP_LSR_PE             0x04u
#define OMAP_LSR_FE             0x08u
#define OMAP_LSR_BI             0x10u
#define OMAP_LSR_TXRDY          0x20u
#define OMAP_LSR_TSRE           0x40u
#define OMAP_LSR_RCV_FIFO       0x80u

/* Bit definitions for modem status  */
#define OMAP_MSR_DCTS           0x01u
#define OMAP_MSR_DDSR           0x02u
#define OMAP_MSR_DRING          0x04u
#define OMAP_MSR_DDCD           0x08u
#define OMAP_MSR_CTS            0x10u
#define OMAP_MSR_DSR            0x20u
#define OMAP_MSR_RING           0x40u
#define OMAP_MSR_DCD            0x80u

/* Bit definitions for interrupt enable register  */
#define OMAP_IER_RHR            0x01u
#define OMAP_IER_THR            0x02u
#define OMAP_IER_LS             0x04u
#define OMAP_IER_MS             0x08u
#define OMAP_IER_SLEEP          0x10u
#define OMAP_IER_XOFF           0x20u
#define OMAP_IER_RTS            0x40u
#define OMAP_IER_CTS            0x80u

/* Bit definitions for fifo control register  */
#define OMAP_FCR_ENABLE         0x01u
#define OMAP_FCR_RXCLR          0x02u
#define OMAP_FCR_TXCLR          0x04u
#define OMAP_FCR_DMA            0x08u

/* Bit definitions for supplementary control register */
#define OMAP_SCR_WAKEUPEN       0x10u
#define OMAP_SCR_TTG1           0x40u
#define OMAP_SCR_RTG1           0x80u

/* Bit definitions for supplementary status register  */
#define OMAP_SSR_TXFULL         0x01u
#define OMAP_SSR_WU_STS         0x02u

/* Bit definitions for enhanced feature register  */
#define OMAP_EFR_ENHANCED       0x10u
#define OMAP_EFR_AUTO_RTS       0x40u
#define OMAP_EFR_AUTO_CTS       0x80u

/* Mode settings for mode definition register 1  */
#define OMAP_MDR1_MODE_16X      0x00u
#define OMAP_MDR1_MODE_AUTOBAUD 0x02u
#define OMAP_MDR1_MODE_13X      0x03u
#define OMAP_MDR1_MODE_DISABLE  0x07u
#define OMAP_MDR1_MODE_MSK      0x07u

#define OMAP_UART_LCR_DLAB      0x80u
#define OMAP_UART_LSR_THRE      0x20u
#define OMAP_UART_LSR_BI        0x10u
#define OMAP_UART_LSR_DR        0x01u
#define OMAP_UART_FIFO_ENABLE   0x07u

#define OMAP_FIFO_SIZE          64  /* size of the rx and tx fifo's */

typedef struct dev_omap {
    TTYDEV          tty;
    uintptr_t       port;
    int             intr;
    int             iid;
    unsigned        clk;

    unsigned        brd;            /* Baud rate divisor */
    unsigned        lcr;            /* Saved LCR value */
    unsigned        efr;            /* Saved EFR value */
    unsigned        baud;           /* Baud rate */

    unsigned        auto_rts_enable;/* Enable auto RTS */
    unsigned        no_msr_int;     /* Do not enable MSR interrupt */
} DEV_OMAP;

typedef struct ttyinit_omap {
    TTYINIT         ttyinit;
    unsigned        loopback;       /* loopback mode*/
    unsigned        auto_rts_enable;/* Enable auto RTS */
    unsigned        no_msr_int;     /* Do not enable MSR interrupt */
    unsigned        rxfifo;         /* Receive FIFO trigger level */
    unsigned        txfifo;         /* Transmit FIFO trigger level */
} TTYINIT_OMAP;

DEV_OMAP *create_device(const TTYINIT_OMAP* const dip);
void ser_stty(DEV_OMAP *dev);
void ser_ctrl(DEV_OMAP *dev, unsigned flags);
void set_port(const uintptr_t regadr, const unsigned mask, const unsigned data);
int  options(const int argc, char *argv[]);
int  interrupt_event_handler(message_context_t *msgctp, int code, unsigned flags, void *handle);

#ifndef write_omap
#define write_omap(__port,__val)    out32(__port, __val)
#endif

#ifndef read_omap
#define read_omap(__port)           in32(__port)
#endif

#define omap_slogf(level, ...)      slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_##level, __VA_ARGS__)

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/devc/seromap/proto.h $ $Rev: 987518 $")
#endif
