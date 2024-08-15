/*
 * AM62PX Hosts Info
 *
 * Copyright (C) 2023 Texas Instruments Incorporated - https://www.ti.com/
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

struct ti_sci_host_info am62px_host_info[] = {
	{0, "TIFS", "Secure", "TI Foundational Security"},
	{10, "A53_0", "Secure", "Cortex A53 context 0 on MAIN domain"},
	{11, "A53_1", "Secure", "Cortex A53 context 1 on MAIN domain"},
	{12, "A53_2", "Non Secure", "Cortex A53 context 2 on MAIN domain"},
	{13, "A53_3", "Non Secure", "Cortex A53 context 3 on MAIN domain"},
	{14, "A53_4", "Non Secure", "Cortex A53 context 4 on MAIN domain"},
	{30, "MCU_0_R5_0", "Non Secure", "MCU R5"},
	{31, "GPU_0", "Non Secure", "GPU context 0 on MAIN domain"},
	{32, "GPU_1", "Non Secure", "GPU context 1 on MAIN domain"},
	{35, "WKUP_0_R5_0", "Secure", "Cortex R5_0 context 0 on WKUP domain (BOOT)"},
	{36, "WKUP_0_R5_1", "Non Secure", "Cortex R5_0 context 1 on WKUP domain"},
	{250, "DM2TIFS", "Secure", "DM to TIFS communication"},
	{251, "TIFS2DM", "Non Secure", "TIFS to DM communication"},
	{253, "HSM", "Secure", "HSM Controller"},
	{254, "DM", "Non Secure", "Device Management"},
};
