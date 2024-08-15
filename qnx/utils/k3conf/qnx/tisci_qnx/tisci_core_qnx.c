/*
 * TISCI core library
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

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/neutrino.h>
#include <string.h>

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

#include <sec_proxy.h>
#include <tisci.h>
#include <tisci_protocol.h>
#include <socinfo.h>

static int seq = 0;

void ti_sci_setup_header(struct ti_sci_msg_hdr *hdr, uint16_t type,
             uint32_t flags)
{
    hdr->type = type;
    hdr->host = soc_info.host_id;
    hdr->seq = seq++;
    hdr->flags = TI_SCI_FLAG_REQ_ACK_ON_PROCESSED | flags;
}


int ti_sci_init(void)
{
    struct ti_sci_version_info *glb_ver;

    int32_t status = CSL_PASS;

    /* Fill in version request message */
    struct tisci_msg_version_req request;
    const Sciclient_ReqPrm_t      reqPrm =
    {
        TISCI_MSG_VERSION,
        TISCI_MSG_FLAG_AOP,
        (uint8_t *) &request,
        sizeof(request),
        SCICLIENT_SERVICE_WAIT_FOREVER
    };

    struct tisci_msg_version_resp response;
    Sciclient_RespPrm_t           respPrm =
    {
        0,
        (uint8_t *) &response,
        sizeof (response)
    };

    /* Request version */
    status = Sciclient_service(&reqPrm, &respPrm);

    if (CSL_PASS != status)
    {
        return -1;
    }

    glb_ver = &soc_info.sci_info.version;
    glb_ver->abi_major = response.abi_major;
    glb_ver->abi_minor = response.abi_minor;
    glb_ver->firmware_version = response.version;
    strncpy(glb_ver->firmware_description, response.str,
        sizeof(glb_ver->firmware_description));

    return 0;
}

int ti_sci_cmd_get_range(uint16_t type, uint16_t subtype, uint16_t host_id,
                struct ti_sci_rm_desc *desc)
{
    struct tisci_msg_rm_get_resource_range_req req = {0};
    struct tisci_msg_rm_get_resource_range_resp resp = {0};
    int ret = 0;

    req.type = type;
    req.subtype = subtype;
    req.secondary_host = host_id;

    ret = Sciclient_rmGetResourceRange(&req, &resp, SCICLIENT_SERVICE_WAIT_FOREVER);
    if (ret == CSL_PASS)
    {
        desc->start = resp.range_start;
        desc->num = resp.range_num;
        desc->start_sec = resp.range_start_sec;
        desc->num_sec = resp.range_num_sec;
    }

    return 0;
}

