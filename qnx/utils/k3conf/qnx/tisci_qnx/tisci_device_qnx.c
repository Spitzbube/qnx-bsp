/*
 * TISCI device ops library
 *
 * Copyright (C) 2019 Texas Instruments Incorporated - https://www.ti.com/
 *  Lokesh Vutla <lokeshvutla@ti.com>
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <tisci_protocol.h>
#include <tisci.h>

#include "ti/drv/sciclient/sciclient.h"
#include "ti/csl/csl_types.h"
#if defined (SOC_J721E)
#include "ti/drv/sciclient/soc/V1/sciclient_fmwMsgParams.h"
#elif defined (SOC_J7200)
#include "ti/drv/sciclient/soc/V2/sciclient_fmwMsgParams.h"
#elif defined (SOC_J721S2)
#include "ti/drv/sciclient/soc/V4/sciclient_fmwMsgParams.h"
#elif defined (SOC_AM62X)
#include "ti/drv/sciclient/soc/V5/sciclient_fmwMsgParams.h"
#elif defined (SOC_J784S4)
#include "ti/drv/sciclient/soc/V6/sciclient_fmwMsgParams.h"
#elif defined (SOC_AM62A)
#include "ti/drv/sciclient/soc/V7/sciclient_fmwMsgParams.h"
#elif defined (SOC_AM62PX)
#include "ti/drv/sciclient/soc/V8/sciclient_fmwMsgParams.h"
#elif defined (SOC_J722S)
#include "ti/drv/sciclient/soc/V9/sciclient_fmwMsgParams.h"
#else
#error "unsupported SOC"
#endif

static const char device_state[MAX_DEVICE_HW_STATES + 1][MAX_DEVICE_STATE_LENGTH] = {
    [MSG_DEVICE_HW_STATE_OFF] = "DEVICE_STATE_OFF",
    [MSG_DEVICE_HW_STATE_ON] = "DEVICE_STATE_ON",
    [MSG_DEVICE_HW_STATE_TRANS] = "DEVICE_STATE_TRANS",
    [MAX_DEVICE_HW_STATES] = "DEVICE_STATE_UNKNOWN"
};

static int ti_sci_set_device_state(uint32_t id, uint32_t flags, uint8_t state)
{
    return Sciclient_pmSetModuleState(id,
                                      state,
                                      flags,
                                      SCICLIENT_SERVICE_WAIT_FOREVER);
}

int ti_sci_cmd_enable_device(uint32_t dev_id)
{
    return ti_sci_set_device_state(dev_id, 0, MSG_DEVICE_SW_STATE_ON);
}

int ti_sci_cmd_disable_device(uint32_t dev_id)
{
    return ti_sci_set_device_state(dev_id, 0, MSG_DEVICE_SW_STATE_AUTO_OFF);
}

const char *ti_sci_cmd_get_device_status(uint32_t dev_id)
{
    uint32_t moduleState = 0;
    uint32_t resetState = 0;
    uint32_t contextLossState = 0;
    int ret = 0;

    ret  = Sciclient_pmGetModuleState(dev_id,
                                      &moduleState,
                                      &resetState,
                                      &contextLossState,
                                      SCICLIENT_SERVICE_WAIT_FOREVER);

    if (ret == CSL_PASS)
    {
        return device_state[moduleState];
    }
	else
	{
    	return "DEVICE_UNKNOWN";
	}
}
