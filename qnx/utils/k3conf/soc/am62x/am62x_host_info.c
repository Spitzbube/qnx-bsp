/*
 * AM62X Hosts Info
 *
 * Copyright (C) 2022 Texas Instruments Incorporated - https://www.ti.com/
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

struct ti_sci_host_info am62x_host_info[] = {
	{0, "TIFS", "Secure", "Device Management and Security Control"},
	{254, "DM", "Non Secure", "Device Management"},
	{35, "MAIN_0_R5_0", "Secure", "Cortex R5_0 context 0 on Main island(BOOT)"},
	{36, "MAIN_0_R5_1", "Non Secure", "Cortex R5_0 context 1 on Main island"},
	{37, "MAIN_0_R5_2", "Secure", "Cortex R5_0 context 2 on Main island"},
	{38, "MAIN_0_R5_3", "Non Secure", "Cortex R5_0 context 3 on Main island"},
	{10, "A53_0", "Secure", "Cortex a53 context 0 on Main islana - ATF"},
	{11, "A53_1", "Non Secure", "Cortex A53 context 1 on Main island - EL2/Hyp"},
	{12, "A53_2", "Non Secure", "Cortex A53 context 2 on Main island - VM/OS1"},
	{13, "A53_3", "Non Secure", "Cortex A53 context 3 on Main island - VM2/OS2"},
	{30, "M4_0", "Non Secure", "M4"},
	{31, "GPU", "Non Secure", "GPU context 0 on Main island"},
	{50, "ICSSG_0", "Non Secure", "ICSSG context 0 on Main island"},
	{250, "DM2TIFS", "Secure", "DM to TIFS communication"},
	{251, "TIFS2DM", "Non Secure", "TIFS to DM communication"},
};
