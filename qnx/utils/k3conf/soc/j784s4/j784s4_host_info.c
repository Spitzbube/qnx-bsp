/*
 * J784S4 Hosts Info
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

struct ti_sci_host_info j784s4_host_info[] = {
	{0, "TIFS", "Secure", "Security Controller"},
	{3, "MCU_0_R5_0", "Non Secure", "Cortex R5 context 0 on MCU island"},
	{4, "MCU_0_R5_1", "Secure", "Cortex R5 context 1 on MCU island(Boot)"},
	{5, "MCU_0_R5_2", "Non Secure", "Cortex R5 context 2 on MCU island"},
	{6, "MCU_0_R5_3", "Secure", "Cortex R5 context 3 on MCU island"},
	{10, "A72_0", "Secure", "Cortex A72 context 0 on Main island"},
	{11, "A72_1", "Secure", "Cortex A72 context 1 on Main island"},
	{12, "A72_2", "Non Secure", "Cortex A72 context 2 on Main island"},
	{13, "A72_3", "Non Secure", "Cortex A72 context 3 on Main island"},
	{14, "A72_4", "Non Secure", "Cortex A72 context 4 on Main island"},
	{15, "A72_5", "Non Secure", "Cortex A72 context 5 on Main island"},
	{16, "A72_6", "Non Secure", "Cortex A72 context 6 on Main island"},
	{17, "A72_7", "Non Secure", "Cortex A72 context 7 on Main island"},
	{20, "C7X_0_0", "Secure", "C7x_0 Context 0 on Main island"},
	{21, "C7X_0_1", "Non Secure", "C7x_0 context 1 on Main island"},
	{22, "C7X_1_0", "Secure", "C7x_1 Context 0 on Main island"},
	{23, "C7X_1_1", "Non Secure", "C7x_1 context 1 on Main island"},
	{24, "C7X_2_0", "Secure", "C7x_2 Context 0 on Main island"},
	{25, "C7X_2_1", "Non Secure", "C7x_2 context 1 on Main island"},
	{26, "C7X_3_0", "Secure", "C7x_2 Context 0 on Main island"},
	{27, "C7X_3_1", "Non Secure", "C7x_3 context 1 on Main island"},
	{30, "GPU_0", "Non Secure", "BXS context 0 on Main island"},
	{35, "MAIN_0_R5_0", "Non Secure", "Cortex R5_0 context 0 on Main island"},
	{36, "MAIN_0_R5_1", "Secure", "Cortex R5_0 context 1 on Main island"},
	{37, "MAIN_0_R5_2", "Non Secure", "Cortex R5_0 context 2 on Main island"},
	{38, "MAIN_0_R5_3", "Secure", "Cortex R5_0 context 3 on Main island"},
	{40, "MAIN_1_R5_0", "Non Secure", "Cortex R5_1 context 0 on Main island"},
	{41, "MAIN_1_R5_1", "Secure", "Cortex R5_1 context 1 on Main island"},
	{42, "MAIN_1_R5_2", "Non Secure", "Cortex R5_1 context 2 on Main island"},
	{43, "MAIN_1_R5_3", "Secure", "Cortex R5_1 context 3 on Main island"},
	{45, "MAIN_2_R5_0", "Non Secure", "Cortex R5_2 context 0 on Main island"},
	{46, "MAIN_2_R5_1", "Secure", "Cortex R5_2 context 1 on Main island"},
	{47, "MAIN_2_R5_2", "Non Secure", "Cortex R5_2 context 2 on Main island"},
	{48, "MAIN_2_R5_3", "Secure", "Cortex R5_2 context 3 on Main island"},
	{250, "DM2TIFS", "Secure", "DM to TIFS communication"},
	{251, "TIFS2DM", "Non Secure", "TIFS to DM communication"},
	{253, "HSM", "Secure", "HSM Controller"},
	{254, "DM", "Non Secure", "Device Management"},
};
