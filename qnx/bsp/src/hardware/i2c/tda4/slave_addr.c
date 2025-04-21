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

/*
 The driver can support 7-bit and 10-bit slave addressing.
 However the driver is not tested for 10-bit slave addressing
 due to unavailabilty 10-bit slave.
 */
int
tda4_set_slave_addr(void *hdl, unsigned addr, i2c_addrfmt_t fmt)
{
    tda4_dev_t  *dev = hdl;

    if ((fmt != I2C_ADDRFMT_7BIT) && (fmt != I2C_ADDRFMT_10BIT)) {
        return -1;
    }

    dev->slave_addr = addr;
    dev->slave_addr_fmt = fmt;

    return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/i2c/tda4/slave_addr.c $ $Rev: 961968 $")
#endif
