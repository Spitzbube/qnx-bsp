/*
 * TISCI Protocol header file.
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

#ifndef __TISCI_P_H
#define __TISCI_P_H

#include <stdint.h>
#include <sec_proxy.h>

#define TI_SCI_MSG_VERSION		0x0002
/* Device requests */
#define TI_SCI_MSG_SET_DEVICE_STATE	0x0200
#define TI_SCI_MSG_GET_DEVICE_STATE	0x0201
/* Clock requests */
#define TI_SCI_MSG_SET_CLOCK_STATE	0x0100
#define TI_SCI_MSG_GET_CLOCK_STATE	0x0101
#define TI_SCI_MSG_SET_CLOCK_PARENT	0x0102
#define TI_SCI_MSG_GET_CLOCK_PARENT	0x0103
#define TI_SCI_MSG_SET_CLOCK_FREQ	0x010c
#define TI_SCI_MSG_QUERY_CLOCK_FREQ	0x010d
#define TI_SCI_MSG_GET_CLOCK_FREQ	0x010e
/* Resource Management Requests */
#define TI_SCI_MSG_GET_RESOURCE_RANGE	0x1500

#define TI_SCI_MSG_FLAG(val)			(1 << (val))
#define TI_SCI_FLAG_REQ_GENERIC_NORESPONSE	0x0
#define TI_SCI_FLAG_REQ_ACK_ON_RECEIVED		TI_SCI_MSG_FLAG(0)
#define TI_SCI_FLAG_REQ_ACK_ON_PROCESSED	TI_SCI_MSG_FLAG(1)
#define TI_SCI_FLAG_RESP_GENERIC_NACK		0x0
#define TI_SCI_FLAG_RESP_GENERIC_ACK		TI_SCI_MSG_FLAG(1)

struct ti_sci_msg_hdr {
	uint16_t type;
	uint8_t host;
	uint8_t seq;
	uint32_t flags;
} __attribute__ ((__packed__));

struct ti_sci_msg_resp_version {
	struct ti_sci_msg_hdr hdr;
	char firmware_description[32];
	uint16_t version;
	uint8_t abi_major;
	uint8_t abi_minor;
} __attribute__ ((__packed__));

struct ti_sci_secure_msg_hdr {
	uint16_t checksum;
	uint16_t reserved;
} __attribute__ ((__packed__));

struct ti_sci_msg_req_set_device_state {
	/* Additional hdr->flags options */
#define MSG_FLAG_DEVICE_WAKE_ENABLED	TI_SCI_MSG_FLAG(8)
#define MSG_FLAG_DEVICE_RESET_ISO	TI_SCI_MSG_FLAG(9)
#define MSG_FLAG_DEVICE_EXCLUSIVE	TI_SCI_MSG_FLAG(10)
	struct ti_sci_msg_hdr hdr;
	uint32_t id;
	uint32_t reserved;
#define MSG_DEVICE_SW_STATE_AUTO_OFF	0
#define MSG_DEVICE_SW_STATE_RETENTION	1
#define MSG_DEVICE_SW_STATE_ON		2
	uint8_t state;
} __attribute__ ((__packed__));

struct ti_sci_msg_req_get_device_state {
	struct ti_sci_msg_hdr hdr;
	uint32_t id;
} __attribute__ ((__packed__));

struct ti_sci_msg_resp_get_device_state {
	struct ti_sci_msg_hdr hdr;
	uint32_t context_loss_count;
	uint32_t resets;
	uint8_t programmed_state;
#define MSG_DEVICE_HW_STATE_OFF		0
#define MSG_DEVICE_HW_STATE_ON		1
#define MSG_DEVICE_HW_STATE_TRANS	2
#define MAX_DEVICE_HW_STATES		3
	uint8_t current_state;
} __attribute__ ((__packed__));

struct ti_sci_msg_req_get_resource_range {
	struct ti_sci_msg_hdr hdr;
#define MSG_RM_RESOURCE_TYPE_MASK	0x3ff
#define MSG_RM_RESOURCE_SUBTYPE_MASK	0x3f
	uint16_t type;
	uint8_t subtype;
	uint8_t secondary_host;
} __attribute__ ((__packed__));

struct ti_sci_msg_resp_get_resource_range {
	struct ti_sci_msg_hdr hdr;
	uint16_t range_start;
	uint16_t range_num;
	uint16_t range_start_sec;
	uint16_t range_num_sec;
} __attribute__ ((__packed__));

struct ti_sci_msg_req_set_clock_state {
	/* Additional hdr->flags options */
#define MSG_FLAG_CLOCK_ALLOW_SSC		TI_SCI_MSG_FLAG(8)
#define MSG_FLAG_CLOCK_ALLOW_FREQ_CHANGE	TI_SCI_MSG_FLAG(9)
#define MSG_FLAG_CLOCK_INPUT_TERM		TI_SCI_MSG_FLAG(10)
	struct ti_sci_msg_hdr hdr;
	uint32_t dev_id;
	uint8_t clk_id;
#define MSG_CLOCK_SW_STATE_UNREQ	0
#define MSG_CLOCK_SW_STATE_AUTO		1
#define MSG_CLOCK_SW_STATE_REQ		2
	uint8_t request_state;
	uint32_t clk_id_32;
} __attribute__ ((__packed__));

struct ti_sci_msg_req_get_clock_state {
	struct ti_sci_msg_hdr hdr;
	uint32_t dev_id;
	uint8_t clk_id;
	uint32_t clk_id_32;
} __attribute__ ((__packed__));

struct ti_sci_msg_resp_get_clock_state {
	struct ti_sci_msg_hdr hdr;
	uint8_t programmed_state;
#define MSG_CLOCK_HW_STATE_NOT_READY	0
#define MSG_CLOCK_HW_STATE_READY	1
#define MAX_CLOCK_HW_STATES		2
	uint8_t current_state;
} __attribute__ ((__packed__));

struct ti_sci_msg_req_query_clock_freq {
	struct ti_sci_msg_hdr hdr;
	uint32_t dev_id;
	uint64_t min_freq_hz;
	uint64_t target_freq_hz;
	uint64_t max_freq_hz;
	uint8_t clk_id;
	uint32_t clk_id_32;
} __attribute__ ((__packed__));

struct ti_sci_msg_resp_query_clock_freq {
	struct ti_sci_msg_hdr hdr;
	uint64_t freq_hz;
} __attribute__ ((__packed__));

struct ti_sci_msg_req_set_clock_freq {
	struct ti_sci_msg_hdr hdr;
	uint32_t dev_id;
	uint64_t min_freq_hz;
	uint64_t target_freq_hz;
	uint64_t max_freq_hz;
	uint8_t clk_id;
	uint32_t clk_id_32;
} __attribute__ ((__packed__));

struct ti_sci_msg_req_get_clock_freq {
	struct ti_sci_msg_hdr hdr;
	uint32_t dev_id;
	uint8_t clk_id;
	uint32_t clk_id_32;
} __attribute__ ((__packed__));

struct ti_sci_msg_resp_get_clock_freq {
	struct ti_sci_msg_hdr hdr;
	uint64_t freq_hz;
} __attribute__ ((__packed__));

struct ti_sci_msg_get_clock_parent_req {
	struct ti_sci_msg_hdr hdr;
	uint32_t dev_id;
	uint8_t clk_id;
	uint32_t clk_id_32;
} __attribute__((__packed__));

struct ti_sci_msg_get_clock_parent_resp {
	struct ti_sci_msg_hdr hdr;
	uint8_t parent_clk_id;
	uint32_t parent_clk_id_32;
} __attribute__((__packed__));

struct ti_sci_msg_set_clock_parent_req {
	uint32_t dev_id;
	uint8_t clk_id;
	uint8_t parent_clk_id;
	uint32_t clk_id_32;
	uint32_t parent_clk_id_32;
} __attribute__((__packed__));

void ti_sci_setup_header(struct ti_sci_msg_hdr *hdr, uint16_t type,
			 uint32_t flags);
int ti_sci_xfer_msg(struct k3_sec_proxy_msg *msg);

static inline uint8_t ti_sci_is_response_ack(uint8_t *resp)
{
	struct ti_sci_msg_hdr *hdr = (struct ti_sci_msg_hdr *)resp;

	return hdr->flags & TI_SCI_FLAG_RESP_GENERIC_ACK ? 1 : 0;
}

#endif
