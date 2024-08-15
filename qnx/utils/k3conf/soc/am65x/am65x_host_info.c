/*
 * AM65X Hosts Info
 *
 * Copyright (C) 2020 Texas Instruments Incorporated - https://www.ti.com/
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

#include <tisci.h>
#include <socinfo.h>

struct ti_sci_host_info am65x_host_info[] = {
	{0, "DMSC", "Secure", "Device Management and Security Control"},
	{3, "R5_0", "Non Secure", "Cortex R5 Context 0 on MCU island"},
	{4, "R5_1", "Secure", "Cortex R5 Context 1 on MCU island(Boot)"},
	{5, "R5_2", "Non Secure", "Cortex R5 Context 2 on MCU island"},
	{6, "R5_3", "Secure", "Cortex R5 Context 3 on MCU island"},
	{10, "A53_0", "Secure", "Cortex A53 context 0 on Main island"},
	{11, "A53_1", "Secure", "Cortex A53 context 1 on Main island"},
	{12, "A53_2", "Non Secure", "Cortex A53 context 2 on Main island"},
	{13, "A53_3", "Non Secure", "Cortex A53 context 3 on Main island"},
	{14, "A53_4", "Non Secure", "Cortex A53 context 4 on Main island"},
	{15, "A53_5", "Non Secure", "Cortex A53 context 5 on Main island"},
	{16, "A53_6", "Non Secure", "Cortex A53 context 6 on Main island"},
	{17, "A53_7", "Non Secure", "Cortex A53 context 7 on Main island"},
	{30, "GPU_0", "Non Secure", "SGX544 Context 0 on Main island"},
	{31, "GPU_1", "Non Secure", "SGX544 Context 1 on Main island"},
	{50, "ICSSG_0", "Non Secure", "ICSS Context 0 on Main island"},
	{51, "ICSSG_1", "Non Secure", "ICSS Context 1 on Main island"},
	{52, "ICSSG_2", "Non Secure", "ICSS Context 2 on Main island"},
};
