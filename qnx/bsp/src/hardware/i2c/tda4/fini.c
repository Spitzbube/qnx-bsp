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

void
tda4_fini(void *hdl)
{
    tda4_dev_t  *dev = hdl;

    out32(dev->regbase + TDA4_I2C_CON, 0);
    out32(dev->regbase + TDA4_I2C_IEC, TDA4_I2C_INT_ALL);

    InterruptDetach(dev->iid);

    munmap_device_io(dev->regbase, dev->reglen);

    free(hdl);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/i2c/tda4/fini.c $ $Rev: 961968 $")
#endif
