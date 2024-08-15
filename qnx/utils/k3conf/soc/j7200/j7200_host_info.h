/*
 * J7200 Host Info
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

#ifndef __J7200_HOST_INFO_H
#define __J7200_HOST_INFO_H

#define J7200_HOST_ID_DMSC	0
#define J7200_HOST_ID_MCU_0_R5_0	3
#define J7200_HOST_ID_MCU_0_R5_1	4
#define J7200_HOST_ID_MCU_0_R5_2	5
#define J7200_HOST_ID_MCU_0_R5_3	6
#define J7200_HOST_ID_A72_0	10
#define J7200_HOST_ID_A72_1	11
#define J7200_HOST_ID_A72_2	12
#define J7200_HOST_ID_A72_3	13
#define J7200_HOST_ID_A72_4	14
#define J7200_HOST_ID_MAIN_0_R5_0	35
#define J7200_HOST_ID_MAIN_0_R5_1	36
#define J7200_HOST_ID_MAIN_0_R5_2	37
#define J7200_HOST_ID_MAIN_0_R5_3	38
#define J7200_HOST_ID_DM2DMSC	250
#define J7200_HOST_ID_DMSC2DM	251
#define J7200_HOST_ID_DM	254

#define J7200_MAX_HOST_IDS	17

extern struct ti_sci_host_info j7200_host_info[];

#endif /* __J7200_HOST_INFO_H */
