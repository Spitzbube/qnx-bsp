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

void *
tda4_init(int argc, char *argv[])
{
    tda4_dev_t  *dev;
    uint32_t    regval;

    if (ThreadCtl(_NTO_TCTL_IO, NULL) == -1) {
        perror("ThreadCtl");
        return NULL;
    }

    dev = malloc(sizeof(tda4_dev_t));
    if (!dev) {
        return NULL;
    }

    if (tda4_options(dev, argc, argv) != EOK) {
        i2c_slogf(dev->verbose, _SLOG_ERROR, "%s: parse I2C option failed", __func__);
        goto fail0;
    }

    dev->regbase = mmap_device_io(dev->reglen, dev->physbase);
    if (dev->regbase == (uintptr_t)MAP_FAILED) {
        i2c_slogf(dev->verbose, _SLOG_ERROR, "%s: mmap_device_io failed", __func__);
        goto fail0;
    }

    regval = (in32(dev->regbase + TDA4_I2C_BUFSTAT) >> 14) & 0x3;
    dev->fifo_size = 0x8u << regval;
    /* Set up notification threshold as half the total available size. */
    dev->fifo_size >>= 1;

    if (tda4_i2c_reset(dev) == -1) {
        i2c_slogf(dev->verbose, _SLOG_ERROR, "%s: reset I2C interface failed", __func__);
        goto fail1;
    }

    struct sigevent intrevent;

    SIGEV_INTR_INIT(&intrevent);
    dev->iid = InterruptAttachEvent(dev->intr, &intrevent, _NTO_INTR_FLAGS_TRK_MSK);
    if (dev->iid == -1) {
        i2c_slogf(dev->verbose, _SLOG_ERROR, "%s: Failed to attach to interrupt", __func__);
        goto fail1;
    }

    return dev;

fail1:
    munmap_device_io(dev->regbase, dev->reglen);
fail0:
    free(dev);

    return NULL;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/i2c/tda4/init.c $ $Rev: 992494 $")
#endif
