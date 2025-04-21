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

int
tda4_driver_info(void *hdl, i2c_driver_info_t *info)
{
    const tda4_dev_t  *const dev = hdl;
    info->speed_mode = I2C_SPEED_STANDARD | I2C_SPEED_FAST;
    info->addr_mode = I2C_ADDRFMT_7BIT | I2C_ADDRFMT_10BIT;
    info->verbosity = dev->verbose;
    return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/i2c/tda4/info.c $ $Rev: 992494 $")
#endif
