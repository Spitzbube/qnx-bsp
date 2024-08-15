/*
 *  Copyright (c) Texas Instruments Incorporated 2023
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
 *  \ingroup DRV_IPC_MODULE
 *  \defgroup DRV_IPC_SOC_MODULE IPC SoC Config
 *            This is IPC documentation specific to J722S SoC
 *
 *  @{
 */

/**
 *  \file ipc_soc.h
 *
 *  \brief IPC Low Level Driver J722S SOC specific file.
 */
#ifndef IPC_SOC_V8_H_
#define IPC_SOC_V8_H_

#include <ti/drv/ipc/include/ipc_config.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief VRing Buffer Size required for all core
 * combinations.
 */
#define IPC_VRING_BUFFER_SIZE   (0x1C00000U)

/** \brief Core definitions */
#define    IPC_MCU1_0       (0x0U)    /*< ARM MCU R5F >*/
#define    IPC_WKUP_R5F     (0x1U)    /*< ARM WKUP R5F >*/
#define    IPC_MAIN_R5F     (0x2U)    /*< ARM MAIN R5F >*/
#define    IPC_C7X_1        (0x3U)    /*< C7x 1 >*/
#define    IPC_C7X_2        (0x4U)    /*< C7x 2 >*/
#define    IPC_A53SS0_0     (0x5U)    /*< ARM MAIN A53 0 >*/
#define    IPC_MAX_PROCS    (0x6U)    /*< Maximum Processors >*/

#define    IPC_MPU1_0       IPC_A53SS0_0 /*< ARM MAIN A53 0 - Used by Virtio. >*/

#define    IPC_MAILBOX_CLUSTER_CNT           (8U)
#define    IPC_MAILBOX_USER_CNT              (4U)

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

/* None */


/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */



/* @} */

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

#ifdef IPC_SUPPORT_SCICLIENT
int32_t Ipc_sciclientIrqRelease(uint16_t remoteId, uint32_t clusterId,
        uint32_t userId, uint32_t intNumber);
int32_t Ipc_sciclientIrqSet(uint16_t remoteId, uint32_t clusterId,
        uint32_t userId, uint32_t intNumber);
int32_t Ipc_getIntNumRange(uint32_t coreIndex, uint16_t *rangeStartP,
        uint16_t *rangeNumP);
#endif

/* ========================================================================== */
/*                       Static Function Definitions                          */
/* ========================================================================== */

/* None */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef IPC_SOC_V8_H_ */

/* @} */
