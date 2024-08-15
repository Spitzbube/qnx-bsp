/*
 * J721S2 Sec Proxy Info
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

struct ti_sci_sec_proxy_info j721s2_main_sp_info[] = {
	{350, "read", 18, "DM", "nonsec_high_priority_rx"},
	{349, "read", 57, "DM", "nonsec_low_priority_rx"},
	{348, "read", 18, "DM", "nonsec_notify_resp_rx"},
	{347, "write", 2, "DM", "nonsec_A72_2_notify_tx"},
	{346, "write", 22, "DM", "nonsec_A72_2_response_tx"},
	{345, "write", 2, "DM", "nonsec_A72_3_notify_tx"},
	{344, "write", 7, "DM", "nonsec_A72_3_response_tx"},
	{343, "write", 2, "DM", "nonsec_A72_4_notify_tx"},
	{342, "write", 7, "DM", "nonsec_A72_4_response_tx"},
	{341, "write", 2, "DM", "nonsec_C7X_0_1_notify_tx"},
	{340, "write", 7, "DM", "nonsec_C7X_0_1_response_tx"},
	{339, "write", 2, "DM", "nonsec_C7X_1_1_notify_tx"},
	{338, "write", 7, "DM", "nonsec_C7X_1_1_response_tx"},
	{337, "write", 2, "DM", "nonsec_GPU_0_notify_tx"},
	{336, "write", 7, "DM", "nonsec_GPU_0_response_tx"},
	{335, "write", 2, "DM", "nonsec_MAIN_0_R5_0_notify_tx"},
	{334, "write", 7, "DM", "nonsec_MAIN_0_R5_0_response_tx"},
	{333, "write", 1, "DM", "nonsec_MAIN_0_R5_2_notify_tx"},
	{332, "write", 2, "DM", "nonsec_MAIN_0_R5_2_response_tx"},
	{331, "write", 2, "DM", "nonsec_MAIN_1_R5_0_notify_tx"},
	{330, "write", 7, "DM", "nonsec_MAIN_1_R5_0_response_tx"},
	{329, "write", 1, "DM", "nonsec_MAIN_1_R5_2_notify_tx"},
	{328, "write", 2, "DM", "nonsec_MAIN_1_R5_2_response_tx"},
	{0, "read", 2, "A72_0", "notify"},
	{1, "read", 30, "A72_0", "response"},
	{2, "write", 10, "A72_0", "high_priority"},
	{3, "write", 20, "A72_0", "low_priority"},
	{4, "write", 2, "A72_0", "notify_resp"},
	{5, "read", 2, "A72_1", "notify"},
	{6, "read", 30, "A72_1", "response"},
	{7, "write", 10, "A72_1", "high_priority"},
	{8, "write", 20, "A72_1", "low_priority"},
	{9, "write", 2, "A72_1", "notify_resp"},
	{10, "read", 2, "A72_2", "notify"},
	{11, "read", 22, "A72_2", "response"},
	{12, "write", 2, "A72_2", "high_priority"},
	{13, "write", 20, "A72_2", "low_priority"},
	{14, "write", 2, "A72_2", "notify_resp"},
	{15, "read", 2, "A72_3", "notify"},
	{16, "read", 7, "A72_3", "response"},
	{17, "write", 2, "A72_3", "high_priority"},
	{18, "write", 5, "A72_3", "low_priority"},
	{19, "write", 2, "A72_3", "notify_resp"},
	{20, "read", 2, "A72_4", "notify"},
	{21, "read", 7, "A72_4", "response"},
	{22, "write", 2, "A72_4", "high_priority"},
	{23, "write", 5, "A72_4", "low_priority"},
	{24, "write", 2, "A72_4", "notify_resp"},
	{25, "read", 2, "C7X_0_0", "notify"},
	{26, "read", 7, "C7X_0_0", "response"},
	{27, "write", 2, "C7X_0_0", "high_priority"},
	{28, "write", 5, "C7X_0_0", "low_priority"},
	{29, "write", 2, "C7X_0_0", "notify_resp"},
	{30, "read", 2, "C7X_0_1", "notify"},
	{31, "read", 7, "C7X_0_1", "response"},
	{32, "write", 2, "C7X_0_1", "high_priority"},
	{33, "write", 5, "C7X_0_1", "low_priority"},
	{34, "write", 2, "C7X_0_1", "notify_resp"},
	{35, "read", 2, "C7X_1_0", "notify"},
	{36, "read", 7, "C7X_1_0", "response"},
	{37, "write", 2, "C7X_1_0", "high_priority"},
	{38, "write", 5, "C7X_1_0", "low_priority"},
	{39, "write", 2, "C7X_1_0", "notify_resp"},
	{40, "read", 2, "C7X_1_1", "notify"},
	{41, "read", 7, "C7X_1_1", "response"},
	{42, "write", 2, "C7X_1_1", "high_priority"},
	{43, "write", 5, "C7X_1_1", "low_priority"},
	{44, "write", 2, "C7X_1_1", "notify_resp"},
	{45, "read", 2, "GPU_0", "notify"},
	{46, "read", 7, "GPU_0", "response"},
	{47, "write", 2, "GPU_0", "high_priority"},
	{48, "write", 5, "GPU_0", "low_priority"},
	{49, "write", 2, "GPU_0", "notify_resp"},
	{50, "read", 2, "MAIN_0_R5_0", "notify"},
	{51, "read", 7, "MAIN_0_R5_0", "response"},
	{52, "write", 2, "MAIN_0_R5_0", "high_priority"},
	{53, "write", 5, "MAIN_0_R5_0", "low_priority"},
	{54, "write", 2, "MAIN_0_R5_0", "notify_resp"},
	{55, "read", 2, "MAIN_0_R5_1", "notify"},
	{56, "read", 7, "MAIN_0_R5_1", "response"},
	{57, "write", 2, "MAIN_0_R5_1", "high_priority"},
	{58, "write", 5, "MAIN_0_R5_1", "low_priority"},
	{59, "write", 2, "MAIN_0_R5_1", "notify_resp"},
	{60, "read", 1, "MAIN_0_R5_2", "notify"},
	{61, "read", 2, "MAIN_0_R5_2", "response"},
	{62, "write", 1, "MAIN_0_R5_2", "high_priority"},
	{63, "write", 1, "MAIN_0_R5_2", "low_priority"},
	{64, "write", 1, "MAIN_0_R5_2", "notify_resp"},
	{65, "read", 1, "MAIN_0_R5_3", "notify"},
	{66, "read", 2, "MAIN_0_R5_3", "response"},
	{67, "write", 1, "MAIN_0_R5_3", "high_priority"},
	{68, "write", 1, "MAIN_0_R5_3", "low_priority"},
	{69, "write", 1, "MAIN_0_R5_3", "notify_resp"},
	{70, "read", 2, "MAIN_1_R5_0", "notify"},
	{71, "read", 7, "MAIN_1_R5_0", "response"},
	{72, "write", 2, "MAIN_1_R5_0", "high_priority"},
	{73, "write", 5, "MAIN_1_R5_0", "low_priority"},
	{74, "write", 2, "MAIN_1_R5_0", "notify_resp"},
	{75, "read", 2, "MAIN_1_R5_1", "notify"},
	{76, "read", 7, "MAIN_1_R5_1", "response"},
	{77, "write", 2, "MAIN_1_R5_1", "high_priority"},
	{78, "write", 5, "MAIN_1_R5_1", "low_priority"},
	{79, "write", 2, "MAIN_1_R5_1", "notify_resp"},
	{80, "read", 1, "MAIN_1_R5_2", "notify"},
	{81, "read", 2, "MAIN_1_R5_2", "response"},
	{82, "write", 1, "MAIN_1_R5_2", "high_priority"},
	{83, "write", 1, "MAIN_1_R5_2", "low_priority"},
	{84, "write", 1, "MAIN_1_R5_2", "notify_resp"},
	{85, "read", 1, "MAIN_1_R5_3", "notify"},
	{86, "read", 2, "MAIN_1_R5_3", "response"},
	{87, "write", 1, "MAIN_1_R5_3", "high_priority"},
	{88, "write", 1, "MAIN_1_R5_3", "low_priority"},
	{89, "write", 1, "MAIN_1_R5_3", "notify_resp"},
};

struct ti_sci_sec_proxy_info j721s2_mcu_sp_info[] = {
	{78, "read", 13, "DM", "nonsec_high_priority_rx"},
	{77, "read", 13, "DM", "nonsec_low_priority_rx"},
	{76, "read", 5, "DM", "nonsec_notify_resp_rx"},
	{75, "write", 2, "DM", "nonsec_MCU_0_R5_0_notify_tx"},
	{74, "write", 20, "DM", "nonsec_MCU_0_R5_0_response_tx"},
	{73, "write", 1, "DM", "nonsec_MCU_0_R5_2_notify_tx"},
	{72, "write", 2, "DM", "nonsec_MCU_0_R5_2_response_tx"},
	{71, "write", 2, "DM", "nonsec_TIFS2DM_notify_tx"},
	{70, "write", 4, "DM", "nonsec_TIFS2DM_response_tx"},
	{0, "read", 2, "MCU_0_R5_0", "notify"},
	{1, "read", 20, "MCU_0_R5_0", "response"},
	{2, "write", 10, "MCU_0_R5_0", "high_priority"},
	{3, "write", 10, "MCU_0_R5_0", "low_priority"},
	{4, "write", 2, "MCU_0_R5_0", "notify_resp"},
	{5, "read", 2, "MCU_0_R5_1", "notify"},
	{6, "read", 20, "MCU_0_R5_1", "response"},
	{7, "write", 10, "MCU_0_R5_1", "high_priority"},
	{8, "write", 10, "MCU_0_R5_1", "low_priority"},
	{9, "write", 2, "MCU_0_R5_1", "notify_resp"},
	{10, "read", 1, "MCU_0_R5_2", "notify"},
	{11, "read", 2, "MCU_0_R5_2", "response"},
	{12, "write", 1, "MCU_0_R5_2", "high_priority"},
	{13, "write", 1, "MCU_0_R5_2", "low_priority"},
	{14, "write", 1, "MCU_0_R5_2", "notify_resp"},
	{15, "read", 1, "MCU_0_R5_3", "notify"},
	{16, "read", 2, "MCU_0_R5_3", "response"},
	{17, "write", 1, "MCU_0_R5_3", "high_priority"},
	{18, "write", 1, "MCU_0_R5_3", "low_priority"},
	{19, "write", 1, "MCU_0_R5_3", "notify_resp"},
	{20, "read", 2, "DM2TIFS", "notify"},
	{21, "read", 4, "DM2TIFS", "response"},
	{22, "write", 2, "DM2TIFS", "high_priority"},
	{23, "write", 2, "DM2TIFS", "low_priority"},
	{24, "write", 2, "DM2TIFS", "notify_resp"},
	{25, "read", 2, "TIFS2DM", "notify"},
	{26, "read", 4, "TIFS2DM", "response"},
	{27, "write", 2, "TIFS2DM", "high_priority"},
	{28, "write", 2, "TIFS2DM", "low_priority"},
	{29, "write", 2, "TIFS2DM", "notify_resp"},
	{30, "read", 1, "HSM", "notify"},
	{31, "read", 2, "HSM", "response"},
	{32, "write", 1, "HSM", "high_priority"},
	{33, "write", 1, "HSM", "low_priority"},
	{34, "write", 1, "HSM", "notify_resp"},
};
