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

#define DEFAULT_CLK         48000000
#define DEFAULT_FIFO_TX     8
#define DEFAULT_FIFO_RX     16

/**
 * handle command line options and create UART device
 * @param   argc    argument count
 * @param   argv    argument vector
 * @return  number of devices that are created successfully
 */
int
options(const int argc, char *argv[])
{
    int    opt, numports = 0;
    const  DEV_OMAP *dev;
    static TTYINIT_OMAP devinit = {
        .ttyinit = {
            .port = 0,
            .port_shift = 0,
            .intr = 0,
            .baud = 115200,
            .isize = 2048,
            .osize = 2048,
            .csize = 256,
            .c_cflag = 0,
            .c_iflag = 0,
            .c_lflag = 0,
            .c_oflag = 0,
            .fifo = 0,
            .clk = DEFAULT_CLK,
            .div = 1,
            .name = "/dev/ser",
            .reserved1 = NULL,
            .reserved2 = 0,
            .verbose = 0,
            .highwater = 0,
            .logging_path = "",
            .lflags = 0,
            .unit = 1
        },
        .loopback = 0,
        .auto_rts_enable = 0,
        .no_msr_int = 0,
        .rxfifo = DEFAULT_FIFO_RX,
        .txfifo = DEFAULT_FIFO_TX
    };

    /* Initialize the devinit to raw mode */
    ttc(TTC_INIT_RAW, &devinit, 0);

    while (optind < argc) {
        /* Process dash options. */
        while (true) {
            opt = getopt(argc, argv, IO_CHAR_SERIAL_OPTIONS "al:Mt:T:");
            if (opt == -1) break;

            switch (ttc(TTC_SET_OPTION, &devinit, opt)) {
            case 'a':
                devinit.auto_rts_enable = 1;
                break;
            case 'l':
                devinit.loopback = (unsigned)strtoul(optarg, NULL, 0);
                break;
            case 'M':
                devinit.no_msr_int = 1;
                break;
            case 't':
                devinit.rxfifo = (unsigned)strtoul(optarg, NULL, 0);
                if (devinit.rxfifo > 63u) {
                    fprintf(stderr, "Illegal RX fifo trigger. \n"
                                    "Trigger number must be less than 64. Set to default value %d.\n", devinit.rxfifo);
                    devinit.rxfifo = DEFAULT_FIFO_RX;
                }
                break;
            case 'T':
                devinit.txfifo = (unsigned)strtoul(optarg, NULL, 0);
                if (devinit.txfifo > 63u) {
                    fprintf(stderr, "Illegal TX fifo trigger. \n"
                                    "Trigger number must be less than 64. Set to default value %d.\n", devinit.txfifo);
                    devinit.txfifo = DEFAULT_FIFO_TX;
                }
                break;
            case -1:
                return (-1);
            }
        }

        if ((devinit.auto_rts_enable) && (devinit.rxfifo > 60)) {
            devinit.rxfifo = 60;
        }
        if ((devinit.rxfifo != 0) || (devinit.txfifo != 0)) {
            devinit.rxfifo = max(devinit.rxfifo, 1);
            devinit.txfifo = max(devinit.txfifo, 1);
        }

        /* Process ports and interrupts. */
        while (optind < argc) {
            optarg = argv[optind];
            if (*optarg == '-') break;

            devinit.ttyinit.port = strtoull(optarg, &optarg, 16);

            if (*optarg == ',') {
                devinit.ttyinit.intr = (unsigned)strtoul(optarg + 1, &optarg, 0);
            } else {
                devinit.ttyinit.intr = 0u;
            }

            if ((devinit.ttyinit.port == 0) || (devinit.ttyinit.intr == 0)) {
                omap_slogf(ERROR, "io-char: Invalid port [%lx] or IRQ [%d]", devinit.ttyinit.port, devinit.ttyinit.intr);
                fprintf(stderr, "io-char: Invalid port [%lx] or IRQ [%d]\n", devinit.ttyinit.port, devinit.ttyinit.intr);
                ++optind;
                continue;
            }

            dev = create_device(&devinit);
            devinit.ttyinit.unit++;
            if (dev == NULL) {
                omap_slogf(ERROR, "io-char: Initialization of /dev/ser%d (port 0x%lx) failed", devinit.ttyinit.unit - 1, devinit.ttyinit.port);
                fprintf(stderr, "io-char: Initialization of port 0x%lx failed\n", devinit.ttyinit.port);
            } else {
                ++numports;
            }
            ++optind;

            if ((dev != NULL) && (dev->tty.verbose > 0)) {
                omap_slogf(INFO, "Port ...................... %s (0x%lx)", dev->tty.name, devinit.ttyinit.port);
                omap_slogf(INFO, "IRQ ....................... 0x%x", dev->intr);
                omap_slogf(INFO, "Rx fifo trigger ........... %d", devinit.rxfifo);
                omap_slogf(INFO, "Tx fifo trigger ........... %d", devinit.txfifo);
                omap_slogf(INFO, "Rx flow control highwater . %d", dev->tty.highwater);
                omap_slogf(INFO, "Input buffer size ......... %d", dev->tty.ibuf.size);
                omap_slogf(INFO, "Output buffer size ........ %d", dev->tty.obuf.size);
                omap_slogf(INFO, "Canonical buffer size ..... %d", dev->tty.cbuf.size);
            }
        }
    }

    return (numports);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/devc/seromap/options.c $ $Rev: 987527 $")
#endif
