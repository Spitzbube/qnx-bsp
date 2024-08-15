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
 *  \file ipc_soc.c
 *
 *  \brief File containing the IPC driver - J722S soc specific implementation.
 *
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include <ti/drv/ipc/ipc.h>
#include <ti/drv/ipc/soc/ipc_soc.h>
#include <ti/drv/ipc/src/ipc_priv.h>
#include <ti/drv/ipc/src/ipc_mailbox.h>
#include <ti/drv/ipc/src/ipc_utils.h>

#include <ti/csl/soc.h>
#include <ti/csl/csl_navss_main.h>
#include <ti/csl/csl_intr_router.h>
#include <ti/csl/csl_clec.h>
#include <ti/drv/sciclient/sciclient.h>

#include <stdio.h>

#define MAILBOX_REG_SIZE                    (0x200)

#define IPC_INTERRUPT_OFFSET                (6U)

extern uint32_t g_ipc_mBoxCnt;

/**
 * \brief Main GICSS0 - Mailbox input line
 */
uint32_t g_MailboxIntrNumber[IPC_MAILBOX_CLUSTER_CNT] =
{
    CSLR_GICSS0_SPI_MAILBOX0_MAILBOX_CLUSTER_0_MAILBOX_CLUSTER_PEND_0,
    CSLR_GICSS0_SPI_MAILBOX0_MAILBOX_CLUSTER_1_MAILBOX_CLUSTER_PEND_0,
    CSLR_GICSS0_SPI_MAILBOX0_MAILBOX_CLUSTER_2_MAILBOX_CLUSTER_PEND_0,
    CSLR_GICSS0_SPI_MAILBOX0_MAILBOX_CLUSTER_3_MAILBOX_CLUSTER_PEND_0
};

/**
 *  \brief Processor IDs to name mapping for all processor in j722s
 */
static Ipc_ProcInfo g_Ipc_mp_procInfo[CSL_CORE_ID_MAX] =
{
    {CSL_CORE_ID_MCU_R5FSS0_0,  "mcu1_0"},   /* MCU R5F */
    {CSL_CORE_ID_WKUP_R5FSS0_0, "wkup_r5f"}, /* WKUP R5F */
    {CSL_CORE_ID_MAIN_R5FSS0_0, "main_r5f"}, /* MAIN R5F */
    {CSL_CORE_ID_C75SS0_0,      "c7x_1"},    /* DSP C7x - core0 */
    {CSL_CORE_ID_C75SS1_0,      "c7x_2"},    /* DSP C7x - core1 */
    {CSL_CORE_ID_A53SS0_0,      "mpu1_0"},   /* MAIN A53 */
    {CSL_CORE_ID_A53SS0_1,      "mpu1_1"},   /* MAIN A53 */
    {CSL_CORE_ID_A53SS1_0,      "mpu2_0"},   /* MAIN A53 */
    {CSL_CORE_ID_A53SS1_1,      "mpu2_1"},   /* MAIN A53 */
    {CSL_CORE_ID_HSM_M4FSS0_0,  "HSM"},      /* MAIN HSM */
};

/* Mailbox Cluster Base Address */
static uintptr_t g_IPC_Mailbox_BasePhyAddr[IPC_MAILBOX_CLUSTER_CNT] =
{
    CSL_MAILBOX0_REGS0_BASE,
    CSL_MAILBOX0_REGS1_BASE,
    CSL_MAILBOX0_REGS2_BASE,
    CSL_MAILBOX0_REGS3_BASE,
    CSL_MAILBOX0_REGS4_BASE,
    CSL_MAILBOX0_REGS5_BASE,
    CSL_MAILBOX0_REGS6_BASE,
    CSL_MAILBOX0_REGS7_BASE
};

#if defined(BUILD_MPU1_0) && defined(QNX_OS)
/* Mailbox Cluster Base Address  - virtual address for HLOS */
static uintptr_t g_IPC_Mailbox_BaseVirtAddr[IPC_MAILBOX_CLUSTER_CNT] =
{
    0
};
#endif

static Ipc_MailboxInfo   g_IPC_MailboxInfo[CSL_CORE_ID_MAX][CSL_CORE_ID_MAX] =
{       /* rx {cluster, user, fifo}, tx {cluster, user, fifo} */
    { /* from MCU R5FSS0-0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to MCU R5FSS0-0 */
        { {    0U ,    2U ,    3U }, {    0U ,    3U ,    2U } }, /* to WKUP R5FSS0-0 */
        { {    6U ,    0U ,    0U }, {    6U ,    3U ,    2U } }, /* to MAIN R5FSS0-0 */
        { {    6U ,    0U ,    1U }, {    6U ,    1U ,    4U } }, /* to C75SS0_0 */
        { {    7U ,    0U ,    0U }, {    7U ,    2U ,    2U } }, /* to C75SS1_0 */
        { {    1U ,    2U ,    1U }, {    1U ,    0U ,    0U } }, /* to A53SS0_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS0_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS1_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS1_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to HSM_R5FSS0_0 */
    },
    { /* from WKUP R5FSS0-0 */
        { {    0U ,    3U ,    2U }, {    0U ,    2U ,    3U } }, /* to MCU R5FSS0-0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to WKUP R5FSS0-0 */
        { {    4U ,    0U ,    0U }, {    4U ,    3U ,    2U } }, /* to MAIN R5FSS0-0 */
        { {    4U ,    0U ,    1U }, {    4U ,    1U ,    3U } }, /* to C75SS0_0 */
        { {    5U ,    0U ,    0U }, {    5U ,    2U ,    2U } }, /* to C75SS1_0 */
        { {    0U ,    3U ,    1U }, {    0U ,    0U ,    0U } }, /* to A53SS0_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS0_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS1_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS1_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to HSM_R5FSS0_0 */
    },
    { /* from MAIN R5FSS0-0 */
        { {    6U ,    3U ,    2U }, {    6U ,    0U ,    0U } }, /* to MCU R5FSS0-0 */
        { {    4U ,    3U ,    2U }, {    4U ,    0U ,    0U } }, /* to WKUP R5FSS0-0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to MAIN R5FSS0-0 */
        { {    6U ,    3U ,    3U }, {    6U ,    1U ,    5U } }, /* to C75SS0_0 */
        { {    7U ,    3U ,    1U }, {    7U ,    2U ,    3U } }, /* to C75SS1_0 */
        { {    3U ,    3U ,    1U }, {    3U ,    0U ,    0U } }, /* to A53SS0_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS0_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS1_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS1_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to HSM_R5FSS0_0 */
    },
    { /* from C75SS0_0 */
        { {    6U ,    1U ,    4U }, {    6U ,    0U ,    1U } }, /* to MCU R5FSS0-0 */
        { {    4U ,    1U ,    3U }, {    4U ,    0U ,    1U } }, /* to WKUP R5FSS0-0 */
        { {    6U ,    1U ,    5U }, {    6U ,    3U ,    3U } }, /* to MAIN R5FSS0-0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to C75SS0_0 */
        { {    5U ,    1U ,    1U }, {    5U ,    2U ,    3U } }, /* to C75SS1_0 */
        { {    2U ,    1U ,    1U }, {    2U ,    0U ,    0U } }, /* to A53SS0_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS0_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS1_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS1_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to HSM_R5FSS0_0 */
    },
    { /* from C75SS1_0 */
        { {    7U ,    2U ,    2U }, {    7U ,    0U ,    0U } }, /* to MCU R5FSS0-0 */
        { {    5U ,    2U ,    2U }, {    5U ,    0U ,    0U } }, /* to WKUP R5FSS0-0 */
        { {    7U ,    2U ,    3U }, {    7U ,    3U ,    1U } }, /* to MAIN R5FSS0-0 */
        { {    5U ,    2U ,    3U }, {    5U ,    1U ,    1U } }, /* to C75SS0_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to C75SS1_0 */
        { {    3U ,    2U ,    3U }, {    3U ,    0U ,    2U } }, /* to A53SS0_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS0_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS1_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS1_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to HSM_R5FSS0_0 */
    },
    { /* from A53SS0_0 */
        { {    1U ,    0U ,    0U }, {    1U ,    2U ,    1U } }, /* to MCU R5FSS0-0 */
        { {    0U ,    0U ,    0U }, {    0U ,    3U ,    1U } }, /* to WKUP R5FSS0-0 */
        { {    3U ,    0U ,    0U }, {    3U ,    3U ,    1U } }, /* to MAIN R5FSS0-0 */
        { {    2U ,    0U ,    0U }, {    2U ,    1U ,    1U } }, /* to C75SS0_0 */
        { {    3U ,    0U ,    2U }, {    3U ,    2U ,    3U } }, /* to C75SS1_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS0_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS0_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS1_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS1_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to HSM_R5FSS0_0 */
    },
    { /* from A53SS0_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to MCU R5FSS0-0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to WKUP R5FSS0-0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to MAIN R5FSS0-0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to C75SS0_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to C75SS1_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS0_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS0_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS1_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS1_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to HSM_R5FSS0_0 */
    },
    { /* from A53SS1_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to MCU R5FSS0-0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to WKUP R5FSS0-0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to MAIN R5FSS0-0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to C75SS0_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to C75SS1_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS0_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS0_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS1_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS1_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to HSM_R5FSS0_0 */
    },
    { /* from A53SS1_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to MCU R5FSS0-0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to WKUP R5FSS0-0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to MAIN R5FSS0-0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to C75SS0_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to C75SS1_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS0_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS0_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS1_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS1_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to HSM_R5FSS0_0 */
    },
    { /* from HSM_R5FSS0_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to MCU R5FSS0-0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to WKUP R5FSS0-0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to MAIN R5FSS0-0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to C75SS0_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to C75SS1_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS0_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS0_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS1_0 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to A53SS1_1 */
        { { 0xFFU , 0xFFU ,    0U }, { 0xFFU , 0xFFU ,    0U } }, /* to HSM_R5FSS0_0 */
    }
};

uint32_t Ipc_getNavss512MailboxInputIntr(uint32_t clusterId, uint32_t userId);
int32_t Ipc_setCoreEventId(uint32_t selfId, Ipc_MbConfig* cfg, uint32_t intrCnt);
int32_t Ipc_sciclientIrqTranslate(uint16_t coreId, uint32_t eventId,
        uint16_t *procIrq);

int32_t Ipc_getMailboxInfoTx(uint32_t selfId, uint32_t remoteId,
                 uint32_t *clusterId, uint32_t *userId, uint32_t *queueId)
{
    int32_t retVal = -1;

    if( (selfId < IPC_MAX_PROCS) &&
        (remoteId < IPC_MAX_PROCS))
    {
        Ipc_MailboxInfo   *pMailboxInfo = &g_IPC_MailboxInfo[selfId][remoteId];

        *clusterId = pMailboxInfo->tx.cluster;
        *userId    = pMailboxInfo->tx.user;
        *queueId   = pMailboxInfo->tx.fifo;
        retVal = 0;
    }

    return retVal;
}

int32_t Ipc_getMailboxInfoRx(uint32_t selfId, uint32_t remoteId,
                 uint32_t *clusterId, uint32_t *userId, uint32_t *queueId)
{
    int32_t retVal = -1;

    if( (selfId < IPC_MAX_PROCS) &&
        (remoteId < IPC_MAX_PROCS))
    {
        Ipc_MailboxInfo   *pMailboxInfo = &g_IPC_MailboxInfo[selfId][remoteId];

        *clusterId = pMailboxInfo->rx.cluster;
        *userId    = pMailboxInfo->rx.user;
        *queueId   = pMailboxInfo->rx.fifo;
        retVal = 0;
    }

    return retVal;

}

uintptr_t Ipc_getMailboxBaseAddr(uint32_t clusterId)
{
    uintptr_t baseAddr = 0x00000000U;

    if( clusterId < IPC_MAILBOX_CLUSTER_CNT)
    {
        baseAddr = g_IPC_Mailbox_BasePhyAddr[clusterId];

#if defined(BUILD_MPU1_0) && defined(QNX_OS)
        if (g_IPC_Mailbox_BaseVirtAddr[clusterId] == 0)
        {
            g_IPC_Mailbox_BaseVirtAddr[clusterId] =
                IpcUtils_getMemoryAddress(baseAddr, MAILBOX_REG_SIZE);
        }
        baseAddr = g_IPC_Mailbox_BaseVirtAddr[clusterId];
#endif
    }

    return baseAddr;
}

uint32_t Ipc_getMailboxInputIntr(uint32_t clusterId, uint32_t userId)
{
    uint32_t   mailboxIntrNum = 0U;

    if( (clusterId != MAILBOX_CLUSTER_INVALID) &&
        (clusterId < IPC_MAILBOX_CLUSTER_CNT)  &&
        (userId != MAILBOX_USER_INVALID)       &&
        (userId < IPC_MAILBOX_USER_CNT))
    {
        mailboxIntrNum = g_MailboxIntrNumber[clusterId];
    }

    return mailboxIntrNum;
}

int32_t Ipc_setCoreEventId(uint32_t selfId, Ipc_MbConfig* cfg, uint32_t intrCnt)
{
    int32_t    retVal          = IPC_SOK;
    uint32_t   outIntrNum      = 0;
    uint32_t   vimEventBaseNum = 0;
#ifdef IPC_SUPPORT_SCICLIENT
    uint32_t   vimEventNum     = 0;
    uint32_t   outIntrBaseNum  = 0;
    uint16_t   proc_irq        = 0;

    /*
     * static variable used to store the base and count of
     * Interrupt Router o/p # allocated for the core.
     * In subsequent call, it uses the offset of intrCnt from base
     */
    static uint16_t   start    = 0;
    static uint16_t   range    = 0;
    uint16_t   offset   = 0;


    /* Get available CorePack IRQ number from DMSC */
    if( (start == 0U) && (range == 0U))
    {
        /* Query the Interrupt Router o/p # alloacted for the core. */
        retVal = Ipc_getIntNumRange(selfId, &start, &range);
        if(retVal != 0)
        {
            SystemP_printf("Failed to get int num range...%d\n", retVal);
        }
        else
        {
            SystemP_printf("Success to get int num range...%d\n", retVal);
        }
    }

    if((retVal == IPC_SOK) && (range >= 1U))
    {
        /* Allocate the last 6 interrupts for IPC. Note that the IR allocation is
         * static so this needs to be carefully set. Currently first interrupt is
         * used by UDMA and middle one's are used by other modules like CPSW9G so
         * we are using last 6 as a safe option.
         */
        if(range >= IPC_INTERRUPT_OFFSET)
        {
            offset = IPC_INTERRUPT_OFFSET;
        }
        else
        {
            offset = range;
        }
        outIntrBaseNum = (uint32_t)((start + range) - offset);
        outIntrNum = outIntrBaseNum + intrCnt;
        SystemP_printf("Requested interrupt ranges: outIntrBaseNum=%u, outIntrNum=%u", outIntrBaseNum, outIntrNum);

        /* Translate to CorePack IRQ number */
        /* Translation must happen after this offset */
        retVal = Ipc_sciclientIrqTranslate((uint16_t)selfId, outIntrBaseNum,
                                           &proc_irq);
        if (CSL_PASS == retVal)
        {
            vimEventBaseNum = proc_irq;
        }
        if(retVal != 0)
        {
            SystemP_printf("Failed to translate base irq via sciclient...%d\n", retVal);
        }
        else
        {
            SystemP_printf("Success to translate base irq via sciclient...%d\n", retVal);
        }

        /* Translation must happen after this offset */
        proc_irq = 0;
        retVal = Ipc_sciclientIrqTranslate((uint16_t)selfId, outIntrNum,
                                           &proc_irq);
        if (CSL_PASS == retVal)
        {
            vimEventNum = proc_irq;
        }
        if(retVal != 0)
        {
            SystemP_printf("Failed to translate last irq via sciclient...%d\n", retVal);
        }
        else
        {
            SystemP_printf("Success to translate last irq via sciclient...%d\n", retVal);
        }

    }
    else
    {
        retVal = IPC_EFAIL;
    }
    cfg->eventId       = vimEventNum;
#endif

#ifndef IPC_SUPPORT_SCICLIENT
    /* Need to be filled in */
    cfg->eventId = cfg->inputIntrNum;
#endif
    cfg->outputIntrNum = outIntrNum;
    cfg->eventIdBase   = vimEventBaseNum;

    return retVal;
}

int32_t Ipc_getMailboxIntrRouterCfg(uint32_t selfId, uint32_t clusterId,
        uint32_t userId, Ipc_MbConfig* cfg, uint32_t cnt)
{
    int32_t    retVal         = IPC_SOK;
    uint32_t   mailboxIntrNum = 0;

    /* Get input interrupt number for mailbox */
    mailboxIntrNum = Ipc_getMailboxInputIntr(clusterId, userId);

    cfg->inputIntrNum  = mailboxIntrNum;
#ifdef QNX_OS
    cfg->priority      = 10U;
#else
    cfg->priority      = 1U;
#endif
    retVal = Ipc_setCoreEventId(selfId, cfg, cnt);

    return retVal;
}

const char* Ipc_getCoreName(uint32_t procId)
{
    char*     p = (char*)0;
    uint32_t id = procId;

    if(id < IPC_MAX_PROCS)
    {
       p = g_Ipc_mp_procInfo[id].name;
    }
    return p;
}

// Sciclient support functions

#ifdef IPC_SUPPORT_SCICLIENT

/* Indexed list of dst ids */
static const uint16_t map_dst_id[] =
{
    TISCI_DEV_MCU_R5FSS0_CORE0,
    TISCI_DEV_WKUP_R5FSS0_CORE0,
    TISCI_DEV_R5FSS0_CORE0,
    TISCI_DEV_C7X256V0_CLEC,
    TISCI_DEV_C7X256V1_CLEC,
    TISCI_DEV_A53SS0_CORE_0,
    TISCI_DEV_A53SS0_CORE_1,
    TISCI_DEV_A53SS0_CORE_2,
    TISCI_DEV_A53SS0_CORE_3,
    TISCI_DEV_HSM0
};

/* Indexed list of src ids */
static const uint16_t map_src_id[] =
{
    TISCI_DEV_MAILBOX0
};

/* Indexed list of req type */
static const uint16_t req_type[] =
{
    TISCI_DEV_MAIN_GPIOMUX_INTROUTER0,
    TISCI_DEV_MAIN_GPIOMUX_INTROUTER0,
    TISCI_DEV_MAIN_GPIOMUX_INTROUTER0,
    TISCI_DEV_MAIN_GPIOMUX_INTROUTER0,
    TISCI_DEV_MAIN_GPIOMUX_INTROUTER0,
    TISCI_DEV_MAIN_GPIOMUX_INTROUTER0,
    TISCI_DEV_MAIN_GPIOMUX_INTROUTER0,
    TISCI_DEV_MAIN_GPIOMUX_INTROUTER0,
    TISCI_DEV_MAIN_GPIOMUX_INTROUTER0
};

/* Indexed list of req subtype */
static const uint16_t req_subtype[] =
{
    /* NOTE: This list should match the Core index */
    TISCI_RESASG_SUBTYPE_IR_OUTPUT,
    TISCI_RESASG_SUBTYPE_IR_OUTPUT,
    TISCI_RESASG_SUBTYPE_IR_OUTPUT,
    TISCI_RESASG_SUBTYPE_IR_OUTPUT,
    TISCI_RESASG_SUBTYPE_IR_OUTPUT,
    TISCI_RESASG_SUBTYPE_IR_OUTPUT,
    TISCI_RESASG_SUBTYPE_IR_OUTPUT,
    TISCI_RESASG_SUBTYPE_IR_OUTPUT,
    TISCI_RESASG_SUBTYPE_IR_OUTPUT
};

int32_t Ipc_sciclientIrqTranslate(uint16_t coreId, uint32_t eventId,
        uint16_t *procIrq)
{
    return Sciclient_rmIrqTranslateIrOutput(req_type[coreId],
                                            (uint16_t)eventId,
                                            map_dst_id[coreId],
                                            procIrq);
}

int32_t Ipc_sciclientIrqRelease(uint16_t coreId, uint32_t clusterId,
        uint32_t userId, uint32_t intNumber)
{
    int32_t                               retVal = IPC_SOK;
    struct tisci_msg_rm_irq_release_req   rmIrqRel;

    rmIrqRel.ia_id                  = 0U;
    rmIrqRel.vint                   = 0U;
    rmIrqRel.global_event           = 0U;
    rmIrqRel.vint_status_bit_index  = 0U;

    rmIrqRel.valid_params   = TISCI_MSG_VALUE_RM_DST_ID_VALID |
                              TISCI_MSG_VALUE_RM_DST_HOST_IRQ_VALID;
    rmIrqRel.src_id         = map_src_id[clusterId];
    rmIrqRel.src_index      = (uint16_t)userId;
    rmIrqRel.dst_id         = (uint16_t)map_dst_id[coreId];
    rmIrqRel.dst_host_irq   = (uint16_t)intNumber;
    rmIrqRel.secondary_host = (uint8_t)TISCI_MSG_VALUE_RM_UNUSED_SECONDARY_HOST;

    retVal = Sciclient_rmIrqRelease(&rmIrqRel, IPC_SCICLIENT_TIMEOUT);

    return retVal;
}

int32_t Ipc_sciclientIrqSet(uint16_t coreId, uint32_t clusterId,
        uint32_t userId, uint32_t intNumber)
{
    int32_t                           retVal = IPC_SOK;
    struct tisci_msg_rm_irq_set_req   rmIrqReq;
    struct tisci_msg_rm_irq_set_resp  rmIrqResp;

    memset(&rmIrqResp, 0, sizeof(rmIrqResp));
    rmIrqReq.ia_id                  = 0U;
    rmIrqReq.vint                   = 0U;
    rmIrqReq.global_event           = 0U;
    rmIrqReq.vint_status_bit_index  = 0U;
    rmIrqReq.valid_params   = TISCI_MSG_VALUE_RM_DST_ID_VALID |
                              TISCI_MSG_VALUE_RM_DST_HOST_IRQ_VALID;
    rmIrqReq.src_id         = map_src_id[clusterId];
    rmIrqReq.src_index      = (uint16_t)userId;
    rmIrqReq.dst_id         = (uint16_t)map_dst_id[coreId];
    rmIrqReq.dst_host_irq   = (uint16_t)intNumber;
    rmIrqReq.secondary_host = (uint8_t)TISCI_MSG_VALUE_RM_UNUSED_SECONDARY_HOST;

    /* Config event */
    retVal = Sciclient_rmIrqSet(&rmIrqReq, &rmIrqResp, IPC_SCICLIENT_TIMEOUT);

    return retVal;
}

int32_t Ipc_getIntNumRange(uint32_t coreIndex,
        uint16_t *rangeStartP, uint16_t *rangeNumP)
{
    int32_t                                     retVal = IPC_SOK;
    struct tisci_msg_rm_get_resource_range_resp res;
    struct tisci_msg_rm_get_resource_range_req  req;

    memset(&res, 0, sizeof(res));
    memset(&req, 0, sizeof(req));

    req.type           = req_type[coreIndex];
    req.subtype        = (uint8_t)req_subtype[coreIndex];
    req.secondary_host = (uint8_t)TISCI_MSG_VALUE_RM_UNUSED_SECONDARY_HOST;

    res.range_num = 0;
    res.range_start = 0;

    /* Get interrupt number range */
    retVal =  Sciclient_rmGetResourceRange(
                &req,
                &res,
                IPC_SCICLIENT_TIMEOUT);
    if ((CSL_PASS != retVal) || (res.range_num == 0U)) {
        /* Try with HOST_ID_ALL */
        req.type           = req_type[coreIndex];
        req.subtype        = (uint8_t)req_subtype[coreIndex];
        req.secondary_host = TISCI_HOST_ID_ALL;

        retVal = Sciclient_rmGetResourceRange(
                &req,
                &res,
                IPC_SCICLIENT_TIMEOUT);
    }
    if ((CSL_PASS == retVal) && (res.range_start != 0) && (res.range_num != 0))
    {
        *rangeStartP = res.range_start;
        *rangeNumP = res.range_num;
    }
    else
    {
        retVal = CSL_EFAIL;
    }
    return retVal;
}

#endif

uint32_t Ipc_getCoreId(void)
{
    uint32_t selfId =  IPC_INVALID_PROCID;

#if defined(BUILD_M4F_0) //Wakeup R5F
    selfId = IPC_M4F_0;
#elif defined(BUILD_MCU1_0)
    selfId = IPC_MCU1_0;
#elif defined(BUILD_MPU1_0)
    selfId = IPC_A53SS0_0;
#elif defined(BUILD_C7X_1)
    selfId = IPC_C7X_1;
#elif defined(BUILD_C7X_2)
    selfId = IPC_C7X_2;
#else
#error "Unsupported core Id"
#endif

    return (selfId);
}

uint32_t Ipc_isCacheCoherent(void)
{
    uint32_t isCacheCoherent;

#if defined (BUILD_MPU1_0)  || defined(BUILD_C7X)
    isCacheCoherent = TRUE;
#else
    isCacheCoherent = FALSE;
#endif

    return (isCacheCoherent);
}
