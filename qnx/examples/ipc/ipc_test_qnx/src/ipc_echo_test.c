/*
 *  Copyright (c) Texas Instruments Incorporated 2018-2021
 *  All rights reserved.
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

/**
 *  \file ipc_echo_test.c
 *
 *  \brief IPC  echo sample application performing basic echo communication using
 *  IPC driver
 *  
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <stdio.h>
#include <stdint.h>

#include <ti/drv/ipc/ipc.h>
#include "ipc_setup.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */


/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */


/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

#if defined (SOC_AM65XX)
#define CORE_IN_TEST            2
#elif defined (SOC_J721E)
#define CORE_IN_TEST            9
#elif defined (SOC_J7200)
#define CORE_IN_TEST            4
#elif defined (SOC_J721S2)
#define CORE_IN_TEST            8
#elif defined (SOC_AM62X)
#define CORE_IN_TEST            2
#elif defined (SOC_AM62A)
#define CORE_IN_TEST            3
#elif defined (SOC_J784S4)
#define CORE_IN_TEST            12
#elif defined (SOC_J722S)
#define CORE_IN_TEST            5
#endif

/*
 * In the cfg file of R5F, C66x, default heap is 48K which is not
 * enough for 9 task_stack, so creating task_stack on global.
 */

uint8_t  g_taskStackBuf[(CORE_IN_TEST+2)*2*IPC_TASK_STACKSIZE];

#if 0
uint8_t  gCntrlBuf[RPMSG_DATA_SIZE] __attribute__ ((section("ipc_data_buffer"), aligned (8)));
uint8_t  sysVqBuf[VQ_BUF_SIZE]  __attribute__ ((section ("ipc_data_buffer"), aligned (8)));
uint8_t  g_sendBuf[RPMSG_DATA_SIZE * CORE_IN_TEST]  __attribute__ ((section ("ipc_data_buffer"), aligned (8)));
uint8_t  g_rspBuf[RPMSG_DATA_SIZE]  __attribute__ ((section ("ipc_data_buffer"), aligned (8)));
#else
uint8_t  gCntrlBuf[RPMSG_DATA_SIZE] __attribute__ ((aligned (8)));
uint8_t  sysVqBuf[VQ_BUF_SIZE]  __attribute__ ((aligned (8)));
uint8_t  g_sendBuf[RPMSG_DATA_SIZE * CORE_IN_TEST]  __attribute__ ((aligned (8)));
uint8_t  g_rspBuf[RPMSG_DATA_SIZE]  __attribute__ ((aligned (8)));
#endif
uint8_t *pCntrlBuf = gCntrlBuf;
uint8_t *pTaskBuf = g_taskStackBuf;
uint8_t *pSendTaskBuf = g_sendBuf;
uint8_t *pRecvTaskBuf = g_rspBuf;
uint8_t *pSysVqBuf = sysVqBuf;

#if defined(SOC_J722S)
uint32_t selfProcId = IPC_A53SS0_0;
#else
uint32_t selfProcId = IPC_MPU1_0;
#endif
uint32_t remoteProc[] =
{
#if defined (SOC_AM65XX)
    IPC_MCU1_0, IPC_MCU1_1
#elif defined (SOC_J721E)
    IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU2_1, IPC_MCU3_0, IPC_MCU3_1, IPC_C66X_1, IPC_C66X_2, IPC_C7X_1
#elif defined (SOC_J7200)
    IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU2_1
#elif defined (SOC_J721S2)
    IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU2_1, IPC_MCU3_0, IPC_MCU3_1, IPC_C7X_1, IPC_C7X_2
#elif defined (SOC_AM62X)
    IPC_M4F_0, IPC_MCU1_0
#elif defined (SOC_AM62A)
    IPC_MCU1_0, IPC_C7X_1, IPC_MCU2_0
#elif defined (SOC_J784S4)
    IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU2_1, IPC_MCU3_0, IPC_MCU3_1, IPC_MCU4_0, IPC_MCU4_1, IPC_C7X_1, IPC_C7X_2, IPC_C7X_3, IPC_C7X_4
#elif defined (SOC_J722S)
    IPC_WKUP_R5F, IPC_MCU1_0, IPC_MAIN_R5F, IPC_C7X_1, IPC_C7X_2
#endif
};

uint32_t *pRemoteProcArray = remoteProc;
uint32_t  gNumRemoteProc = sizeof(remoteProc)/sizeof(uint32_t);
