/*
 * TISCI clock ops library
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
#include <sec_proxy.h>
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

static const char clock_state[MAX_CLOCK_HW_STATES + 1][MAX_CLOCK_STATE_LENGTH] = {
    [MSG_CLOCK_HW_STATE_NOT_READY] = "CLK_STATE_NOT_READY",
    [MSG_CLOCK_HW_STATE_READY] = "CLK_STATE_READY",
    [MAX_CLOCK_HW_STATES] = "CLK_STATE_UNKNOWN"
};


int ti_sci_cmd_get_clk(uint32_t dev_id, uint32_t clk_id)
{
    return  Sciclient_pmModuleClkRequest(dev_id,
                                        clk_id,
                                        MSG_CLOCK_SW_STATE_REQ,
                                        MSG_FLAG_CLOCK_ALLOW_FREQ_CHANGE,
                                        SCICLIENT_SERVICE_WAIT_FOREVER);
}

int ti_sci_cmd_put_clk(uint32_t dev_id, uint32_t clk_id)
{
    return  Sciclient_pmModuleClkRequest(dev_id,
                                        clk_id,
                                        MSG_CLOCK_SW_STATE_UNREQ,
                                        MSG_FLAG_CLOCK_ALLOW_FREQ_CHANGE,
                                        SCICLIENT_SERVICE_WAIT_FOREVER);
}

const char *ti_sci_cmd_get_clk_state(uint32_t dev_id, uint32_t clk_id)
{
    uint32_t state = 0;
    int ret;


    ret = Sciclient_pmModuleGetClkStatus(dev_id,
                                         clk_id,
                                         &state,
                                         SCICLIENT_SERVICE_WAIT_FOREVER);
    if (ret == CSL_PASS)
    {
        return clock_state[state];
    }
    else
    {
        return "CLK_UNKNOWN";
    }
}

int ti_sci_cmd_set_clk_freq(uint32_t dev_id, uint32_t clk_id, uint64_t freq)
{
    uint32_t additionalFlag = TISCI_MSG_FLAG_CLOCK_ALLOW_FREQ_CHANGE;

    return Sciclient_pmSetModuleClkFreq(dev_id,
                                        clk_id,
                                        freq,
                                        additionalFlag,
                                        SCICLIENT_SERVICE_WAIT_FOREVER);
}

int ti_sci_cmd_get_clk_freq(uint32_t dev_id, uint32_t clk_id, uint64_t *freq)
{
    return Sciclient_pmGetModuleClkFreq(dev_id,
                                        clk_id,
                                        freq,
                                        SCICLIENT_SERVICE_WAIT_FOREVER);
}

int ti_sci_cmd_get_clk_parent(uint32_t dev_id, uint32_t clk_id, uint32_t *parent_clk_id)
{
    return Sciclient_pmGetModuleClkParent(dev_id,
                                         clk_id,
                                         parent_clk_id,
                                         SCICLIENT_SERVICE_WAIT_FOREVER);
}

int ti_sci_cmd_set_clk_parent(uint32_t dev_id, uint32_t clk_id, uint32_t parent_clk_id)
{
    return Sciclient_pmSetModuleClkParent(dev_id,
                                         clk_id,
                                         parent_clk_id,
                                         SCICLIENT_SERVICE_WAIT_FOREVER);
}
