/*
 * AM62AX Host Info
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

#ifndef __AM62AX_HOST_INFO_H
#define __AM62AX_HOST_INFO_H

#define AM62AX_HOST_ID_TIFS	0
#define AM62AX_HOST_ID_A53_0	10
#define AM62AX_HOST_ID_A53_1	11
#define AM62AX_HOST_ID_A53_2	12
#define AM62AX_HOST_ID_A53_3	13
#define AM62AX_HOST_ID_A53_4	14
#define AM62AX_HOST_ID_C7X_0_0	20
#define AM62AX_HOST_ID_MCU_0_R5_0	30
#define AM62AX_HOST_ID_MAIN_0_R5_0	35
#define AM62AX_HOST_ID_MAIN_0_R5_1	36
#define AM62AX_HOST_ID_MAIN_0_R5_2	37
#define AM62AX_HOST_ID_MAIN_0_R5_3	38
#define AM62AX_HOST_ID_DM2TIFS	250
#define AM62AX_HOST_ID_TIFS2DM	251
#define AM62AX_HOST_ID_HSM	253
#define AM62AX_HOST_ID_DM	254

#define AM62AX_MAX_HOST_IDS	16

extern struct ti_sci_host_info am62ax_host_info[];

#endif /* __AM62AX_HOST_INFO_H */
