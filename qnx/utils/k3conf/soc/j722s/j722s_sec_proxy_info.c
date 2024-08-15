/*
 * J722S Sec Proxy Info
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

struct ti_sci_sec_proxy_info j722s_main_sp_info[] = {
	{69, "read", 44, "DM", "nonsec_low_priority_rx"},
	{68, "write", 11, "DM", "nonsec_WKUP_0_R5_1_response_tx"},
	{67, "write", 6, "DM", "nonsec_MAIN_0_R5_1_response_tx"},
	{66, "write", 6, "DM", "nonsec_A53_2_response_tx"},
	{65, "write", 6, "DM", "nonsec_A53_3_response_tx"},
	{64, "write", 6, "DM", "nonsec_A53_4_response_tx"},
	{63, "write", 5, "DM", "nonsec_MCU_0_R5_0_response_tx"},
	{62, "write", 4, "DM", "nonsec_C7X_0_0_response_tx"},
	{61, "write", 4, "DM", "nonsec_C7X_1_0_response_tx"},
	{60, "write", 1, "DM", "nonsec_GPU_0_response_tx"},
	{59, "write", 1, "DM", "nonsec_GPU_1_response_tx"},
	{58, "write", 4, "DM", "nonsec_TIFS2DM_response_tx"},
	{0, "read", 11, "WKUP_0_R5_0", "response"},
	{1, "write", 10, "WKUP_0_R5_0", "low_priority"},
	{2, "read", 11, "WKUP_0_R5_1", "response"},
	{3, "write", 10, "WKUP_0_R5_1", "low_priority"},
	{4, "read", 6, "MAIN_0_R5_0", "response"},
	{5, "write", 5, "MAIN_0_R5_0", "low_priority"},
	{6, "read", 6, "MAIN_0_R5_1", "response"},
	{7, "write", 5, "MAIN_0_R5_1", "low_priority"},
	{8, "read", 11, "A53_0", "response"},
	{9, "write", 10, "A53_0", "low_priority"},
	{10, "read", 11, "A53_1", "response"},
	{11, "write", 10, "A53_1", "low_priority"},
	{12, "read", 6, "A53_2", "response"},
	{13, "write", 5, "A53_2", "low_priority"},
	{14, "read", 6, "A53_3", "response"},
	{15, "write", 5, "A53_3", "low_priority"},
	{16, "read", 6, "A53_4", "response"},
	{17, "write", 5, "A53_4", "low_priority"},
	{18, "read", 5, "MCU_0_R5_0", "response"},
	{19, "write", 4, "MCU_0_R5_0", "low_priority"},
	{20, "read", 4, "C7X_0_0", "response"},
	{21, "write", 3, "C7X_0_0", "low_priority"},
	{22, "read", 4, "C7X_1_0", "response"},
	{23, "write", 3, "C7X_1_0", "low_priority"},
	{24, "read", 1, "GPU_0", "response"},
	{25, "write", 1, "GPU_0", "low_priority"},
	{26, "read", 1, "GPU_1", "response"},
	{27, "write", 1, "GPU_1", "low_priority"},
	{28, "read", 4, "DM2TIFS", "response"},
	{29, "write", 2, "DM2TIFS", "low_priority"},
	{30, "read", 4, "TIFS2DM", "response"},
	{31, "write", 2, "TIFS2DM", "low_priority"},
};

struct ti_sci_sec_proxy_info j722s_mcu_sp_info[] = {
	{15, "read", 8, "TIFS_HSM", "sec_low_priority_rx"},
	{14, "write", 8, "TIFS_HSM", "sec_HSM_response_tx"},
	{0, "read", 8, "HSM", "response"},
	{1, "write", 8, "HSM", "low_priority"},
};
