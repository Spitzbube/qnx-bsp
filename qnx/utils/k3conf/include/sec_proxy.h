/*
 * Secure Proxy header file
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

#include <stdio.h>
#include <stdint.h>

#ifndef __SEC_PROXY_H
#define __SEC_PROXY_H

#define DEFAULT_HOST_ID		14
#define INVALID_HOST_ID		0xff
#define DEFAULT_SEC_PROXY_RX_THREAD	21
#define DEFAULT_SEC_PROXY_TX_THREAD	23

#define SEC_PROXY_MAX_MSG_SIZE	60
#define SEC_PROXY_HOST		14

struct k3_sec_proxy_msg {
	size_t len;
	uint8_t *buf;
};

struct k3_sec_proxy_base {
	uint32_t src_target_data;
	uint32_t cfg_scfg;
	uint32_t cfg_rt;
};

int k3_sec_proxy_send(struct k3_sec_proxy_msg *msg);
int k3_sec_proxy_recv(struct k3_sec_proxy_msg *msg);
int k3_sec_proxy_init();

extern struct k3_sec_proxy_base k3_generic_sec_proxy_base;
extern struct k3_sec_proxy_base k3_lite_sec_proxy_base;

#endif /* __SEC_PROXY_H */
