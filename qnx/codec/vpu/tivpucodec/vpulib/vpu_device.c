/*
 *  Copyright (c) Texas Instruments Incorporated 2021
 *  All rights reserved.
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

#include <stdio.h>
#include <ti/drv/sciclient/sciclient.h>
#include <ti/csl/csl_types.h>

#if defined (SOC_J721S2)
#include <ti/drv/sciclient/soc/V4/sciclient_fmwMsgParams.h>
#elif defined (SOC_J784S4)
#include <ti/drv/sciclient/soc/V6/sciclient_fmwMsgParams.h>
#elif defined (SOC_J722S)
#include <ti/drv/sciclient/soc/V9/sciclient_fmwMsgParams.h>
#else
#error "Unsupported SoC"
#endif

/* Set device state for VPU */
static int set_vpudevice_state(unsigned long core_idx, uint32_t state)
{
    int status = CSL_EFAIL;
    struct tisci_msg_set_device_req reqState;

    /* Fill in payload */
    if (core_idx == 0)
    {
        reqState.id = TISCI_DEV_CODEC0;
    }
#if defined (SOC_J784S4)
    else if (core_idx == 1)
    {
        reqState.id = TISCI_DEV_CODEC1;
    }
#endif
    reqState.reserved  = 0;
    reqState.state = state;

    /* Create Request message */
    const Sciclient_ReqPrm_t      reqPrm =
    {
        TISCI_MSG_SET_DEVICE,
        TISCI_MSG_FLAG_AOP,
        (uint8_t *) &reqState,
        sizeof(reqState),
        SCICLIENT_SERVICE_WAIT_FOREVER
    };

    /* Create response buffer */
    Sciclient_RespPrm_t           respPrm =
    {
        0,
        NULL,
        0
    };

    /* Send request */
    status = Sciclient_service(&reqPrm, &respPrm);
    if (CSL_PASS == status)
    {
        if (respPrm.flags == TISCI_MSG_FLAG_ACK)
        {
            status = CSL_PASS;
        }
        else
        {
            printf("%s (%d): Set Device State, failed\n", __FUNCTION__, __LINE__);
        }
    }
    else
    {
        printf("%s (%d): DMSC Firmware Set Device state, failed\n", __FUNCTION__, __LINE__);
    }

    return status;
}

/* Retrieve device current state for VPU */
static int get_vpudevice_state(unsigned long core_idx)
{
    int status = CSL_PASS;
    struct tisci_msg_get_device_req reqState;
    struct tisci_msg_get_device_resp respState = {0};

    /* Fill in payload */
    if (core_idx == 0)
    {
        reqState.id = TISCI_DEV_CODEC0;
    }
#if defined (SOC_J784S4)
    else if (core_idx == 1)
    {
        reqState.id = TISCI_DEV_CODEC1;
    }
#endif

    /* Create Request message */
    const Sciclient_ReqPrm_t      reqPrm =
    {
        TISCI_MSG_GET_DEVICE,
        TISCI_MSG_FLAG_AOP,
        (uint8_t *) &reqState,
        sizeof(reqState),
        SCICLIENT_SERVICE_WAIT_FOREVER
    };

    /* Create response buffer */
    Sciclient_RespPrm_t           respPrm =
    {
        0,
        (uint8_t *) &respState,
        sizeof (respState)
    };

    /* Send request */
    status = Sciclient_service(&reqPrm, &respPrm);
    if (CSL_PASS == status)
    {
        if (respPrm.flags == TISCI_MSG_FLAG_ACK)
        {
            status = CSL_PASS;
        }
        else
        {
            printf("%s (%d): DeviceId State failed \n", __FUNCTION__, __LINE__);
        }
    }
    else
    {
        printf("%s (%d): DMSC Firmware Get device state, failed \n", __FUNCTION__, __LINE__);
    }

    return respState.current_state;
}

int vpu_enable_device(unsigned long core_idx)
{
    return set_vpudevice_state(core_idx, TISCI_MSG_VALUE_DEVICE_SW_STATE_ON);
}

int vpu_disable_device(unsigned long core_idx)
{
    return set_vpudevice_state(core_idx, TISCI_MSG_VALUE_DEVICE_SW_STATE_AUTO_OFF);
}

int vpu_get_device_state(unsigned long core_idx)
{
    return get_vpudevice_state(core_idx);
}
