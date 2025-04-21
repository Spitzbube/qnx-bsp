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
i2c_master_getfuncs(i2c_master_funcs_t *funcs, int tabsize)
{
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            init, tda4_init, tabsize);
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            fini, tda4_fini, tabsize);
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            send, tda4_send, tabsize);
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            recv, tda4_recv, tabsize);
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            set_slave_addr, tda4_set_slave_addr, tabsize);
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            set_bus_speed, tda4_set_bus_speed, tabsize);
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            version_info, tda4_version_info, tabsize);
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            driver_info, tda4_driver_info, tabsize);
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            bus_reset, tda4_bus_reset, tabsize);

    return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/i2c/tda4/lib.c $ $Rev: 961968 $")
#endif
