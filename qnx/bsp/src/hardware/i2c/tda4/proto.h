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

#ifndef __PROTO_H_INCLUDED
#define __PROTO_H_INCLUDED

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <semaphore.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <hw/inout.h>
#include <hw/i2c.h>

#define TDA4_I2C0_BASE              0x02000000ul
#define TDA4_I2C0_IRQ               232
#define TDA4_I2C_SIZE               0x1000

#define TDA4_I2C_IS                 0x28            /* IRQ status */
#define TDA4_I2C_IES                0x2C            /* IRQ enable set */
#define TDA4_I2C_IEC                0x30            /* IRQ enable clear */
 #define TDA4_I2C_INT_XDR           (1u << 14)
 #define TDA4_I2C_INT_RDR           (1u << 13)
 #define TDA4_I2C_INT_BB            (1u << 12)
 #define TDA4_I2C_INT_ROVR          (1u << 11)
 #define TDA4_I2C_INT_XUDF          (1u << 10)
 #define TDA4_I2C_INT_AAS           (1u << 9)
 #define TDA4_I2C_INT_BF            (1u << 8)
 #define TDA4_I2C_INT_AERR          (1u << 7)
 #define TDA4_I2C_INT_STC           (1u << 6)
 #define TDA4_I2C_INT_GC            (1u << 5)
 #define TDA4_I2C_INT_XRDY          (1u << 4)
 #define TDA4_I2C_INT_RRDY          (1u << 3)
 #define TDA4_I2C_INT_ARDY          (1u << 2)
 #define TDA4_I2C_INT_NACK          (1u << 1)
 #define TDA4_I2C_INT_AL            (1u << 0)

#define TDA4_I2C_IE_MASK            (TDA4_I2C_INT_AL | TDA4_I2C_INT_NACK | TDA4_I2C_INT_ARDY | \
                                     TDA4_I2C_INT_RRDY | TDA4_I2C_INT_XRDY |  \
                                     TDA4_I2C_INT_RDR | TDA4_I2C_INT_XDR)

#define TDA4_I2C_INT_ALL            (TDA4_I2C_INT_AL | TDA4_I2C_INT_NACK | TDA4_I2C_INT_ARDY | \
                                     TDA4_I2C_INT_RRDY | TDA4_I2C_INT_XRDY | TDA4_I2C_INT_ROVR | \
                                     TDA4_I2C_INT_RDR | TDA4_I2C_INT_XDR | TDA4_I2C_INT_XUDF | \
                                     TDA4_I2C_INT_AAS | TDA4_I2C_INT_BF | TDA4_I2C_INT_AERR | \
                                     TDA4_I2C_INT_STC | TDA4_I2C_INT_GC)

#define TDA4_I2C_SYSS               0x90    /* System Status register */
 #define TDA4_I2C_SYSS_RDONE        (1u << 0)       /* Reset done */

#define TDA4_I2C_BUF                0x94    /* Buffer Configuration register */
 #define TDA4_I2C_BUF_RXFIF_CLR     (1u << 14)      /* Receive FIFO clear */
 #define TDA4_I2C_BUF_TXFIF_CLR     (1u << 6)       /* Transmit FIFO clear */

#define TDA4_I2C_CNT                0x98
#define TDA4_I2C_DATA               0x9C
#define TDA4_I2C_CON                0xA4    /* Control register */
 #define TDA4_I2C_CON_EN            (1u << 15)      /* I2C module enable */
 #define TDA4_I2C_CON_STB           (1u << 11)      /* Start byte mode */
 #define TDA4_I2C_CON_MST           (1u << 10)      /* Master/slave mode */
 #define TDA4_I2C_CON_TRX           (1u << 9)       /* Transmitter/Receiver mode */
 #define TDA4_I2C_CON_XSA           (1u << 8)       /* Expand Slave address */
 #define TDA4_I2C_CON_STP           (1u << 1)       /* Stop condition */
 #define TDA4_I2C_CON_STT           (1u << 0)       /* Start condition */

#define TDA4_I2C_OA                 0xA8
#define TDA4_I2C_SA                 0xAC
#define TDA4_I2C_PSC                0xB0
#define TDA4_I2C_SCLL               0xB4
#define TDA4_I2C_SCLH               0xB8
#define TDA4_I2C_SYSTEST            0xBC
#define TDA4_I2C_BUFSTAT            0xC0
 #define TDA4_I2C_BUFSTAT_TXSTAT    (0x3Fu)
 #define TDA4_I2C_BUFSTAT_RXSTAT    (0x3Fu << 8)

typedef struct _tda4_dev {
    unsigned        reglen;
    uintptr_t       regbase;
    paddr_t         physbase;

    unsigned        re_start;
    int             intr;
    int             iid;
    unsigned        fifo_size;
    unsigned        xlen;
    uint8_t         *buf;
    unsigned        speed;
    int             intexpected;
    i2c_status_t    status;

    unsigned        pclk;
    unsigned        own_addr;
    unsigned        slave_addr;
    i2c_addrfmt_t   slave_addr_fmt;
    unsigned        flags;
    int             high_adjust_fast;
    int             high_adjust_slow;
    int             low_adjust_fast;
    int             low_adjust_slow;
    unsigned        verbose;
} tda4_dev_t;

#define TDA4_PCLK               96000000    /* peripheral clock 96MHz */
#define TDA4_I2C_ICLK_4000K     4000000     /* internal clock for 100K bus speed */
#define TDA4_I2C_ICLK_9600K     9600000     /* internal clock for 400K bus speed */

#define TDA4_SPEED_MAX          400000
#define TDA4_SPEED_STANDARD     100000
#define TDA4_SPEED_MIN          8000

#define TDA4_FLG_ROVR_XUDF_OK   0x00000001
#define TDA4_FLG_FIFO_PREFILL   0x00000002

void *tda4_init(int argc, char *argv[]);
void tda4_fini(void *hdl);
int  tda4_options(tda4_dev_t *dev, const int argc, char *argv[]);
int  tda4_set_slave_addr(void *hdl, unsigned addr, i2c_addrfmt_t fmt);
int  tda4_set_bus_speed(void *hdl, unsigned speed, unsigned *ospeed);
int  tda4_version_info(i2c_libversion_t *version);
int  tda4_driver_info(void *hdl, i2c_driver_info_t *info);
int  tda4_bus_reset(void *hdl);
int  tda4_wait_bus_not_busy(tda4_dev_t *dev, const unsigned stop);
int  tda4_i2c_reset(tda4_dev_t *dev);
i2c_status_t tda4_recv(void *hdl, void *buf, unsigned len, unsigned stop);
i2c_status_t tda4_send(void *hdl, void *buf, unsigned len, unsigned stop);
i2c_status_t tda4_wait_complete(tda4_dev_t *dev);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/i2c/tda4/proto.h $ $Rev: 992494 $")
#endif
