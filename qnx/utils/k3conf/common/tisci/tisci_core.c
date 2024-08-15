/*
 * TISCI core library
 *
 * Copyright (C) 2019 Texas Instruments Incorporated - https://www.ti.com/
 *	Lokesh Vutla <lokeshvutla@ti.com>
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

int ti_sci_xfer_msg(struct k3_sec_proxy_msg *msg)
{
	int ret;

	if (!msg->len || !msg->buf)
		return -1;

	ret = k3_sec_proxy_send(msg);
	if (ret)
		return ret;

	memset(msg->buf, 0, msg->len);
	ret = k3_sec_proxy_recv(msg);
	if (ret)
		return ret;

	if (!ti_sci_is_response_ack(msg->buf))
		return -1;

	return 0;
}

int ti_sci_init(void)
{
	struct ti_sci_msg_resp_version *version;
	struct ti_sci_version_info *glb_ver;
	uint8_t buf[SEC_PROXY_MAX_MSG_SIZE];
	struct k3_sec_proxy_msg msg;
	int ret;

	memset(buf, 0, sizeof(buf));
	ti_sci_setup_header((struct ti_sci_msg_hdr *)buf, TI_SCI_MSG_VERSION,
			    0);

	msg.len = sizeof(struct ti_sci_msg_hdr);
	msg.buf = buf;
	ret = ti_sci_xfer_msg(&msg);
	if (ret)
		return ret;

	version = (struct ti_sci_msg_resp_version *)buf;
	glb_ver = &soc_info.sci_info.version;
	glb_ver->abi_major = version->abi_major;
	glb_ver->abi_minor = version->abi_minor;
	glb_ver->firmware_version = version->version;
	strncpy(glb_ver->firmware_description, version->firmware_description,
		sizeof(glb_ver->firmware_description));

	return 0;
}

int ti_sci_cmd_get_range(uint16_t type, uint16_t subtype, uint16_t host_id,
				struct ti_sci_rm_desc *desc)
{
	struct ti_sci_msg_resp_get_resource_range *resp;
	struct ti_sci_msg_req_get_resource_range *req;
	uint8_t buf[SEC_PROXY_MAX_MSG_SIZE];
	struct k3_sec_proxy_msg msg;
	int ret = 0;

	memset(buf, 0, sizeof(buf));
	ti_sci_setup_header((struct ti_sci_msg_hdr *)buf,
			    TI_SCI_MSG_GET_RESOURCE_RANGE, 0);
	req = (struct ti_sci_msg_req_get_resource_range *)buf;
	req->type = type;
	req->subtype = subtype;
	req->secondary_host = host_id;

	msg.len = sizeof(*req);
	msg.buf = buf;
	ret = ti_sci_xfer_msg(&msg);
	if (ret)
		return ret;

	resp = (struct ti_sci_msg_resp_get_resource_range *)buf;
	desc->start = resp->range_start;
	desc->num = resp->range_num;
	desc->start_sec = resp->range_start_sec;
	desc->num_sec = resp->range_num_sec;

	return 0;
}
