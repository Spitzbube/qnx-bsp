/*
 *  Copyright (c) Texas Instruments Incorporated 2018-21
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

/*!
 * \file     enetlld_if_utils.c
 *
 * \brief    Common ENET LLD utility.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <ti/csl/soc.h>

#include <ti/drv/enet/enet.h>
#include <ti/drv/enet/enet_cfg.h>

#include <ti/drv/udma/udma.h>

#include "j7_cpsw.h"
#include "enetlld_if_utils.h"
#include "enetlld_if_memutils.h"
#include "enetlld_if.h"

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
static uintptr_t EnetIf_disableAllIntrDma(void);
static void EnetIf_restoreAllIntrDma(uintptr_t key);
static void EnetIf_printStatsNonZero(const char *pcString,
                                           uint64_t statVal);
static void EnetIf_printStatsWithIdxNonZero(const char *pcString,
                                                  uint32_t idx,
                                                  uint64_t statVal);
static EnetIf_MmrLockState EnetIf_unlockMmr(volatile uint32_t *kick0,
                                           volatile uint32_t *kick1);
static EnetIf_MmrLockState EnetIf_lockMmr(volatile uint32_t *kick0,
                                         volatile uint32_t *kick1);
static int32_t EnetIf_allocTxCh(Enet_Handle hEnet,
                                 uint32_t coreKey,
                                 uint32_t coreId,
                                 uint32_t *txPSILThreadId);
static void EnetIf_reduceCoreMacAllocation(EnetRm_ResPrms *rscPrms,
                                            uint32_t *pReduceCount,
                                            uint32_t coreMinCount,
                                            bool skipCore,
                                            uint32_t skipCoreId);
static void EnetIf_updateMacResourcePartition(EnetRm_ResPrms *rscPrms,
                                               uint32_t availMacCount,
                                               uint32_t selfCoreId);
static void EnetIf_delAddrEntry(Enet_Handle hEnet,
                                 uint32_t coreId,
                                 uint8_t *macAddr);
static void EnetIf_printDmaDescStats(EnetDma_DmaDescStats *descstats);
static void EnetIf_printCbStats(EnetDma_CbStats *cbStats);
static void EnetIf_printRxChStats(EnetDma_RxChStats *stats);
static void EnetIf_printTxChStats(EnetDma_TxChStats *stats);

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/* UDMA driver objects */
static struct Udma_DrvObj udmaDrvObj[ENET_TYPE_NUM];

extern EnetIf_Obj gEnetIfObj;

/* devnp driver object */
extern struct cpsw_dev *g_cpsw;

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

static uintptr_t EnetIf_disableAllIntrDma(void)
{
    pthread_mutex_lock(&gEnetIfObj.cpswMutex);
    return 0;
}

static void EnetIf_restoreAllIntrDma(uintptr_t key)
{
    pthread_mutex_unlock(&gEnetIfObj.cpswMutex);
}

uint32_t EnetIf_getNavSSInstanceId(Enet_Type enetType)
{
    uint32_t instId;

#if defined(BUILD_MCU1_0) || defined(BUILD_MCU1_1)
    instId = UDMA_INST_ID_MCU_0;
#else
    instId = UDMA_INST_ID_MAIN_0;
#endif
    return instId;
}

Udma_DrvHandle EnetIf_udmaOpen(Enet_Type enetType,
                                Udma_InitPrms *pInitPrms)
{
    Udma_InitPrms initPrms;
    Udma_DrvHandle hUdmaDrv;
    int32_t retVal;
    uint32_t instId;

    hUdmaDrv = &udmaDrvObj[enetType];
    memset(hUdmaDrv, 0U, sizeof(*hUdmaDrv));

    /* Use default params if not passed by caller */
    if (NULL == pInitPrms)
    {
        instId = EnetIf_getNavSSInstanceId(enetType);
        /* Initialize the UDMA driver based on NAVSS instance */
        UdmaInitPrms_init(instId, &initPrms);
        initPrms.printFxn = (Udma_PrintFxn) & EnetIf_print;
        initPrms.virtToPhyFxn = &EnetIf_udmaVirtToPhyFxn;
        initPrms.phyToVirtFxn = &EnetIf_udmaPhyToVirtFxn;

        //Note: Over-ride the osal api to disableAllIntr/restoreAllIntr
        initPrms.osalPrms.disableAllIntr = &EnetIf_disableAllIntrDma;
        initPrms.osalPrms.restoreAllIntr = &EnetIf_restoreAllIntrDma;
        //Note: Over-ride the osal api to registerIntr/restoreAllIntr
        initPrms.osalPrms.registerIntr = &EnetIf_registerIntr2;
        initPrms.osalPrms.unRegisterIntr = &EnetIf_unRegisterIntr;

        retVal            = Udma_init(hUdmaDrv, &initPrms);
    }
    else
    {
        retVal = Udma_init(hUdmaDrv, pInitPrms);
    }

    /* assert if UDMA failed to open */
    EnetIf_assert(UDMA_SOK == retVal);

    return hUdmaDrv;
}

void EnetIf_udmaclose(Udma_DrvHandle hUdmaDrv)
{
    EnetIf_assert(NULL != hUdmaDrv);
    /* App should make sure that all Rx & Tx channels & are closed */
    /* Deinitialize the UDMA driver */
    Udma_deinit(hUdmaDrv);
}

void EnetIf_print(const char *pcString, ...)
{
    char printBuffer[ENET_CFG_PRINT_BUF_LEN];
    va_list arguments;

    if (ENET_CFG_PRINT_BUF_LEN < strlen(pcString))
    {
        EnetIf_assert(false);
    }

    /* Start the varargs processing */
    va_start(arguments, pcString);
    vsnprintf(printBuffer, sizeof(printBuffer), pcString, arguments);

    slogf(_SLOGC_NETWORK, _SLOG_INFO, printBuffer);

    /* End the varargs processing */
    va_end(arguments);
}

static void EnetIf_printStatsNonZero(const char *pcString,
                                      uint64_t statVal)
{
    if (0U != statVal)
    {
        EnetIf_print(pcString, statVal);
    }
}

static void EnetIf_printStatsWithIdxNonZero(const char *pcString,
                                             uint32_t idx,
                                             uint64_t statVal)
{
    if (0U != statVal)
    {
        EnetIf_print(pcString, idx, statVal);
    }
}

void EnetIf_printMacAddr(uint8_t macAddr[])
{
    EnetIf_print("%02x:%02x:%02x:%02x:%02x:%02x",
                       macAddr[0] & 0xFF,
                       macAddr[1] & 0xFF,
                       macAddr[2] & 0xFF,
                       macAddr[3] & 0xFF,
                       macAddr[4] & 0xFF,
                       macAddr[5] & 0xFF);
}

void EnetIf_printFrame(EthFrame *frame,
                        uint32_t len)
{
    uint8_t *payload;
    uint32_t i;
    char printbuf[256], *bufPtr;

    EnetIf_print("Dst addr : ");
    EnetIf_printMacAddr(&frame->hdr.dstMac[0]);

    EnetIf_print("Src addr : ");
    EnetIf_printMacAddr(&frame->hdr.srcMac[0]);

    if (frame->hdr.etherType == htons(ETHERTYPE_VLAN_TAG))
    {
        EthVlanFrame *vlanFrame = (EthVlanFrame *)frame;

        EnetIf_print("TPID     : 0x%04x", ntohs(vlanFrame->hdr.tpid) & 0xFFFFU);
        EnetIf_print("Priority : %d", (ntohs(vlanFrame->hdr.tci) & 0xFFFFU) >> 13);
        EnetIf_print("VLAN Id  : %d", ntohs(vlanFrame->hdr.tci) & 0xFFFU);
        EnetIf_print("EtherType: 0x%04x", ntohs(vlanFrame->hdr.etherType) & 0xFFFFU);
        payload = vlanFrame->payload;
        len    -= ETH_VLAN_TAG_LEN;
    }
    else
    {
        EnetIf_print("EtherType: 0x%04x", ntohs(frame->hdr.etherType) & 0xFFFFU);
        payload = frame->payload;
    }

    EnetIf_print("Payload  : ");
    bufPtr = printbuf;
    for (i = 0; i < len; i++)
    {
        sprintf(bufPtr,"%02x ", payload[i]);
        bufPtr += 3;
        if (i && (((i + 1) % OCTETS_PER_ROW) == 0))
        {
            EnetIf_print("%s", printbuf);
            bufPtr = printbuf;
        }
    }
    EnetIf_print("%s", printbuf);
}

void EnetIf_printHostPortStats2G(CpswStats_HostPort_2g *st)
{
    EnetIf_printStatsNonZero("  rxGoodFrames            = %llu", st->rxGoodFrames);
    EnetIf_printStatsNonZero("  rxBcastFrames           = %llu", st->rxBcastFrames);
    EnetIf_printStatsNonZero("  rxMcastFrames           = %llu", st->rxMcastFrames);
    EnetIf_printStatsNonZero("  rxCrcErrors             = %llu", st->rxCrcErrors);
    EnetIf_printStatsNonZero("  rxOversizedFrames       = %llu",
                                   st->rxOversizedFrames);
    EnetIf_printStatsNonZero("  rxUndersizedFrames      = %llu",
                                   st->rxUndersizedFrames);
    EnetIf_printStatsNonZero("  rxFragments             = %llu", st->rxFragments);
    EnetIf_printStatsNonZero("  aleDrop                 = %llu", st->aleDrop);
    EnetIf_printStatsNonZero("  aleOverrunDrop          = %llu", st->aleOverrunDrop);
    EnetIf_printStatsNonZero("  rxOctets                = %llu", st->rxOctets);
    EnetIf_printStatsNonZero("  txGoodFrames            = %llu", st->txGoodFrames);
    EnetIf_printStatsNonZero("  txBcastFrames           = %llu", st->txBcastFrames);
    EnetIf_printStatsNonZero("  txMcastFrames           = %llu", st->txMcastFrames);
    EnetIf_printStatsNonZero("  txOctets                = %llu", st->txOctets);
    EnetIf_printStatsNonZero("  octetsFrames64          = %llu", st->octetsFrames64);
    EnetIf_printStatsNonZero("  octetsFrames65to127     = %llu",
                                   st->octetsFrames65to127);
    EnetIf_printStatsNonZero("  octetsFrames128to255    = %llu",
                                   st->octetsFrames128to255);
    EnetIf_printStatsNonZero("  octetsFrames256to511    = %llu",
                                   st->octetsFrames256to511);
    EnetIf_printStatsNonZero("  octetsFrames512to1023   = %llu",
                                   st->octetsFrames512to1023);
    EnetIf_printStatsNonZero("  octetsFrames1024        = %llu",
                                   st->octetsFrames1024);
    EnetIf_printStatsNonZero("  netOctets               = %llu", st->netOctets);
    EnetIf_printStatsNonZero("  rxBottomOfFifoDrop      = %llu",
                                   st->rxBottomOfFifoDrop);
    EnetIf_printStatsNonZero("  portMaskDrop            = %llu", st->portMaskDrop);
    EnetIf_printStatsNonZero("  rxTopOfFifoDrop         = %llu",
                                   st->rxTopOfFifoDrop);
    EnetIf_printStatsNonZero("  aleRateLimitDrop        = %llu",
                                   st->aleRateLimitDrop);
    EnetIf_printStatsNonZero("  aleVidIngressDrop       = %llu",
                                   st->aleVidIngressDrop);
    EnetIf_printStatsNonZero("  aleDAEqSADrop           = %llu", st->aleDAEqSADrop);
    EnetIf_printStatsNonZero("  aleBlockDrop            = %llu", st->aleBlockDrop);
    EnetIf_printStatsNonZero("  aleSecureDrop           = %llu", st->aleSecureDrop);
    EnetIf_printStatsNonZero("  aleAuthDrop             = %llu", st->aleAuthDrop);
    EnetIf_printStatsNonZero("  aleUnknownUcast         = %llu",
                                   st->aleUnknownUcast);
    EnetIf_printStatsNonZero("  aleUnknownUcastBcnt     = %llu",
                                   st->aleUnknownUcastBcnt);
    EnetIf_printStatsNonZero("  aleUnknownMcast         = %llu",
                                   st->aleUnknownMcast);
    EnetIf_printStatsNonZero("  aleUnknownMcastBcnt     = %llu",
                                   st->aleUnknownMcastBcnt);
    EnetIf_printStatsNonZero("  aleUnknownBcast         = %llu",
                                   st->aleUnknownBcast);
    EnetIf_printStatsNonZero("  aleUnknownBcastBcnt     = %llu",
                                   st->aleUnknownBcastBcnt);
    EnetIf_printStatsNonZero("  alePolicyMatch          = %llu", st->alePolicyMatch);
    EnetIf_printStatsNonZero("  alePolicyMatchRed       = %llu",
                                   st->alePolicyMatchRed);
    EnetIf_printStatsNonZero("  alePolicyMatchYellow    = %llu",
                                   st->alePolicyMatchYellow);
    EnetIf_printStatsNonZero("  txMemProtectError       = %llu",
                                   st->txMemProtectError);
}

void EnetIf_printMacPortStats2G(CpswStats_MacPort_2g *st)
{
    uint32_t i;

    EnetIf_printStatsNonZero("  rxGoodFrames            = %llu", st->rxGoodFrames);
    EnetIf_printStatsNonZero("  rxBcastFrames           = %llu", st->rxBcastFrames);
    EnetIf_printStatsNonZero("  rxMcastFrames           = %llu", st->rxMcastFrames);
    EnetIf_printStatsNonZero("  rxPauseFrames           = %llu", st->rxPauseFrames);
    EnetIf_printStatsNonZero("  rxCrcErrors             = %llu", st->rxCrcErrors);
    EnetIf_printStatsNonZero("  rxAlignCodeErrors       = %llu",
                                   st->rxAlignCodeErrors);
    EnetIf_printStatsNonZero("  rxOversizedFrames       = %llu",
                                   st->rxOversizedFrames);
    EnetIf_printStatsNonZero("  rxJabberFrames          = %llu", st->rxJabberFrames);
    EnetIf_printStatsNonZero("  rxUndersizedFrames      = %llu",
                                   st->rxUndersizedFrames);
    EnetIf_printStatsNonZero("  rxFragments             = %llu", st->rxFragments);
    EnetIf_printStatsNonZero("  aleDrop                 = %llu", st->aleDrop);
    EnetIf_printStatsNonZero("  aleOverrunDrop          = %llu", st->aleOverrunDrop);
    EnetIf_printStatsNonZero("  rxOctets                = %llu", st->rxOctets);
    EnetIf_printStatsNonZero("  txGoodFrames            = %llu", st->txGoodFrames);
    EnetIf_printStatsNonZero("  txBcastFrames           = %llu", st->txBcastFrames);
    EnetIf_printStatsNonZero("  txMcastFrames           = %llu", st->txMcastFrames);
    EnetIf_printStatsNonZero("  txPauseFrames           = %llu", st->txPauseFrames);
    EnetIf_printStatsNonZero("  txDeferredFrames        = %llu",
                                   st->txDeferredFrames);
    EnetIf_printStatsNonZero("  txCollisionFrames       = %llu",
                                   st->txCollisionFrames);
    EnetIf_printStatsNonZero("  txSingleCollFrames      = %llu",
                                   st->txSingleCollFrames);
    EnetIf_printStatsNonZero("  txMultipleCollFrames    = %llu",
                                   st->txMultipleCollFrames);
    EnetIf_printStatsNonZero("  txExcessiveCollFrames   = %llu",
                                   st->txExcessiveCollFrames);
    EnetIf_printStatsNonZero("  txLateCollFrames        = %llu",
                                   st->txLateCollFrames);
    EnetIf_printStatsNonZero("  rxIPGError              = %llu", st->rxIPGError);
    EnetIf_printStatsNonZero("  txCarrierSenseErrors    = %llu",
                                   st->txCarrierSenseErrors);
    EnetIf_printStatsNonZero("  txOctets                = %llu", st->txOctets);
    EnetIf_printStatsNonZero("  octetsFrames64          = %llu", st->octetsFrames64);
    EnetIf_printStatsNonZero("  octetsFrames65to127     = %llu",
                                   st->octetsFrames65to127);
    EnetIf_printStatsNonZero("  octetsFrames128to255    = %llu",
                                   st->octetsFrames128to255);
    EnetIf_printStatsNonZero("  octetsFrames256to511    = %llu",
                                   st->octetsFrames256to511);
    EnetIf_printStatsNonZero("  octetsFrames512to1023   = %llu",
                                   st->octetsFrames512to1023);
    EnetIf_printStatsNonZero("  octetsFrames1024        = %llu",
                                   st->octetsFrames1024);
    EnetIf_printStatsNonZero("  netOctets               = %llu", st->netOctets);
    EnetIf_printStatsNonZero("  rxBottomOfFifoDrop      = %llu",
                                   st->rxBottomOfFifoDrop);
    EnetIf_printStatsNonZero("  portMaskDrop            = %llu", st->portMaskDrop);
    EnetIf_printStatsNonZero("  rxTopOfFifoDrop         = %llu",
                                   st->rxTopOfFifoDrop);
    EnetIf_printStatsNonZero("  aleRateLimitDrop        = %llu",
                                   st->aleRateLimitDrop);
    EnetIf_printStatsNonZero("  aleVidIngressDrop       = %llu",
                                   st->aleVidIngressDrop);
    EnetIf_printStatsNonZero("  aleDAEqSADrop           = %llu", st->aleDAEqSADrop);
    EnetIf_printStatsNonZero("  aleBlockDrop            = %llu", st->aleBlockDrop);
    EnetIf_printStatsNonZero("  aleSecureDrop           = %llu", st->aleSecureDrop);
    EnetIf_printStatsNonZero("  aleAuthDrop             = %llu", st->aleAuthDrop);
    EnetIf_printStatsNonZero("  aleUnknownUcast         = %llu",
                                   st->aleUnknownUcast);
    EnetIf_printStatsNonZero("  aleUnknownUcastBcnt     = %llu",
                                   st->aleUnknownUcastBcnt);
    EnetIf_printStatsNonZero("  aleUnknownMcast         = %llu",
                                   st->aleUnknownMcast);
    EnetIf_printStatsNonZero("  aleUnknownMcastBcnt     = %llu",
                                   st->aleUnknownMcastBcnt);
    EnetIf_printStatsNonZero("  aleUnknownBcast         = %llu",
                                   st->aleUnknownBcast);
    EnetIf_printStatsNonZero("  aleUnknownBcastBcnt     = %llu",
                                   st->aleUnknownBcastBcnt);
    EnetIf_printStatsNonZero("  alePolicyMatch          = %llu", st->alePolicyMatch);
    EnetIf_printStatsNonZero("  alePolicyMatchRed       = %llu",
                                   st->alePolicyMatchRed);
    EnetIf_printStatsNonZero("  alePolicyMatchYellow    = %llu",
                                   st->alePolicyMatchYellow);
    EnetIf_printStatsNonZero("  txMemProtectError       = %llu",
                                   st->txMemProtectError);

    for (i = 0U; i < ENET_ARRAYSIZE(st->txPri); i++)
    {
        EnetIf_printStatsWithIdxNonZero("  txPri[%u]                = %llu", i, st->txPri[i]);
    }

    for (i = 0U; i < ENET_ARRAYSIZE(st->txPriBcnt); i++)
    {
        EnetIf_printStatsWithIdxNonZero("  txPriBcnt[%u]            = %llu", i, st->txPriBcnt[i]);
    }

    for (i = 0U; i < ENET_ARRAYSIZE(st->txPriDrop); i++)
    {
        EnetIf_printStatsWithIdxNonZero("  txPriDrop[%u]            = %llu", i, st->txPriDrop[i]);
    }

    for (i = 0U; i < ENET_ARRAYSIZE(st->txPriDropBcnt); i++)
    {
        EnetIf_printStatsWithIdxNonZero("  txPriDropBcnt[%u]        = %llu", i, st->txPriDropBcnt[i]);
    }
}

void EnetIf_validatePacketState(EnetDma_PktQ *pQueue,
                                 uint32_t expectedState,
                                 uint32_t newState)
{
    uint32_t i;
    EnetDma_Pkt *pktInfo = (EnetDma_Pkt *)pQueue->head;

    for (i = 0; i < EnetQueue_getQCount(pQueue); i++)
    {
        EnetDma_checkPktState(&pktInfo->pktState,
                                ENET_PKTSTATE_MODULE_APP,
                                expectedState,
                                newState);
        pktInfo = (EnetDma_Pkt *)pktInfo->node.next;
    }
}

uint64_t EnetIf_virtToPhyFxn(const void *virtAddr,
                              void *appData)
{
    int ret;
    off64_t    phyAddr = 0;
    uint32_t   length;

    EnetIf_assert(virtAddr != NULL);
    if (virtAddr == 0) return 0;

    if(appData != NULL_PTR) {
        length = (uint32_t) *((uint32_t *) appData);
    }
    else {
        EnetIf_print("%s:%d Must specify memory size to map\n", __FUNCTION__, __LINE__);
        return 0;
    }

    /* Get destination physical address */
    ret = mem_offset64(virtAddr, NOFD, length, &phyAddr, NULL);
    if (ret) {
        if (errno != EAGAIN) {
            EnetIf_print("%s:%d: Error: failed mem_offset. errno=%d",
            __FUNCTION__, __LINE__, errno);
        }
        else if (phyAddr == 0) {
            EnetIf_print("%s:%d: Error: failed mem_offset. errno=%d",
            __FUNCTION__, __LINE__, errno);
        }
    }
    //EnetIf_print("%s: 0x%lx -> 0x%lx ",__FUNCTION__, virtAddr, phyAddr);

    return (uint64_t ) phyAddr;
}

void *EnetIf_phyToVirtFxn(uint64_t phyAddr,
                           void *appData)
{
    uint64_t *temp = 0;
    uint32_t length = 0;

    EnetIf_assert(phyAddr != 0);
    if (phyAddr == 0) return 0;

    if(appData != NULL_PTR) {
        length = (uint32_t) *((uint32_t *) appData);
    }
    else {
        EnetIf_print("%s:%d Must specify memory size to map\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    temp  = mmap_device_memory(0, length, PROT_READ|PROT_WRITE, 0, phyAddr);
    if((temp == MAP_FAILED))
    {
        EnetIf_print("%s:%d Error: mmmap_device_memory failed",
            __FUNCTION__, __LINE__);
    }
    //EnetIf_print("%s: 0x%lx -> 0x%lx size -> %d",__FUNCTION__, phyAddr, temp, length);

    return ((void *) temp);
}

uint64_t EnetIf_udmaVirtToPhyFxn(const void *virtAddr,
                                  uint32_t chNum,
                                  void *appData)
{
    return EnetIf_virtToPhyFxn(virtAddr, appData);
}

void *EnetIf_udmaPhyToVirtFxn(uint64_t phyAddr,
                               uint32_t chNum,
                               void *appData)
{
    return EnetIf_phyToVirtFxn(phyAddr, appData);
}

static EnetIf_MmrLockState EnetIf_unlockMmr(volatile uint32_t *kick0,
                                           volatile uint32_t *kick1)
{
    EnetIf_MmrLockState prevLockState;
    uint32_t kick0Val;

    /* if kick lock registers are locked*/
    if ((CSL_REG32_RD(kick0) & 0x1U) == 0U)
    {
        /* unlock the partition by writing the unlock values to the kick lock registers*/
        CSL_REG32_WR(kick0, MMR_KICK0_UNLOCK_VAL);
        CSL_REG32_WR(kick1, MMR_KICK1_UNLOCK_VAL);
        prevLockState = ENETIF_LOCK_MMR;
    }
    else
    {
        prevLockState = ENETIF_UNLOCK_MMR;
    }

    kick0Val = CSL_REG32_FEXT(kick0, MMR_KICK_UNLOCKED);

    /* check to see if unlock bit in lock0 register is set */
    if (1U != kick0Val)
    {
        EnetIf_assert(false);
    }
    return prevLockState;
}

static EnetIf_MmrLockState EnetIf_lockMmr(volatile uint32_t *kick0,
                                         volatile uint32_t *kick1)
{
    EnetIf_MmrLockState prevLockState;
    uint32_t kick0Val;

    /* check to see if either of the kick registers are unlocked.*/
    if ((CSL_REG32_RD(kick0) & 0x1) == 1U)
    {
        /* write the kick lock value to the kick lock registers to lock the partition*/
        CSL_REG32_WR(kick0, MMR_KICK_LOCK_VAL);
        CSL_REG32_WR(kick1, MMR_KICK_LOCK_VAL);

        prevLockState = ENETIF_LOCK_MMR;
    }
    else
    {
        prevLockState = ENETIF_UNLOCK_MMR;
    }

    /* check to see if unlock bit in lock0 register is set */
    kick0Val = CSL_REG32_FEXT(kick0, MMR_KICK_UNLOCKED);

    if (0U != kick0Val)
    {
        EnetIf_assert(false);
    }
    return prevLockState;
}

EnetIf_MmrLockState EnetIf_mcuMmrCtrl(EnetIf_CtrlMmrType mmrNum,
                                     EnetIf_MmrLockState lock)
{
#if defined(SOC_AM65XX) || defined(SOC_J721E) || defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4)
    CSL_mcu_ctrl_mmr_cfg0Regs *regs = NULL;
    volatile uint32_t *kick0 = NULL, *kick1 = NULL;
    EnetIf_MmrLockState prevLockState = ENETIF_LOCK_MMR;

    regs = (CSL_mcu_ctrl_mmr_cfg0Regs *)mmap_device_memory(0,
          sizeof(CSL_mcu_ctrl_mmr_cfg0Regs), PROT_READ|PROT_WRITE|PROT_NOCACHE, 0,
                (uint64_t)CSL_MCU_CTRL_MMR0_CFG0_BASE);

    EnetIf_assert(regs != NULL);

    switch (mmrNum)
    {
        case ENETIF_MMR_LOCK0:
            kick0 = &regs->LOCK0_KICK0;
            kick1 = &regs->LOCK0_KICK1;
            break;

        case ENETIF_MMR_LOCK1:
            kick0 = &regs->LOCK1_KICK0;
            kick1 = &regs->LOCK1_KICK1;
            break;

        case ENETIF_MMR_LOCK2:
            kick0 = &regs->LOCK2_KICK0;
            kick1 = &regs->LOCK2_KICK1;
            break;

        case ENETIF_MMR_LOCK3:
            kick0 = &regs->LOCK3_KICK0;
            kick1 = &regs->LOCK3_KICK1;
            break;

        case ENETIF_MMR_LOCK4:
            kick0 = &regs->LOCK4_KICK0;
            kick1 = &regs->LOCK4_KICK1;
            break;

        default:
            EnetIf_assert(false);
            break;
    }

    if ((NULL != kick0) && (NULL != kick1))
    {
        if (ENETIF_UNLOCK_MMR == lock)
        {
            prevLockState = EnetIf_unlockMmr(kick0, kick1);
        }
        else
        {
            prevLockState = EnetIf_lockMmr(kick0, kick1);
        }
    }

    munmap_device_memory((void *)regs,sizeof(CSL_mcu_ctrl_mmr_cfg0Regs));
    return prevLockState;
#else
#error "Unsupported platform"
#endif
}

EnetIf_MmrLockState EnetIf_mainMmrCtrl(EnetIf_CtrlMmrType mmrNum,
                                      EnetIf_MmrLockState lock)
{
#if defined(SOC_AM65XX) || defined(SOC_J721E) || defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4)
    CSL_main_ctrl_mmr_cfg0Regs *regs = NULL;
    volatile uint32_t *kick0 = NULL, *kick1 = NULL;
    EnetIf_MmrLockState prevLockState = ENETIF_LOCK_MMR;

    regs = (CSL_main_ctrl_mmr_cfg0Regs *)mmap_device_memory(0,
            sizeof(CSL_main_ctrl_mmr_cfg0Regs), PROT_READ|PROT_WRITE|PROT_NOCACHE, 0,
            (uint64_t)CSL_CTRL_MMR0_CFG0_BASE);

    EnetIf_assert(regs != NULL);

    switch (mmrNum)
    {
        case ENETIF_MMR_LOCK0:
            kick0 = &regs->LOCK0_KICK0;
            kick1 = &regs->LOCK0_KICK1;
            break;

        case ENETIF_MMR_LOCK1:
            kick0 = &regs->LOCK1_KICK0;
            kick1 = &regs->LOCK1_KICK1;
            break;

        case ENETIF_MMR_LOCK2:
            kick0 = &regs->LOCK2_KICK0;
            kick1 = &regs->LOCK2_KICK1;
            break;

        case ENETIF_MMR_LOCK3:
            kick0 = &regs->LOCK3_KICK0;
            kick1 = &regs->LOCK3_KICK1;
            break;

        case ENETIF_MMR_LOCK4:
            kick0 = &regs->LOCK4_KICK0;
            kick1 = &regs->LOCK4_KICK1;
            break;

#if defined(SOC_J721E) || defined(SOC_J7200)|| defined(SOC_J721S2) || defined(SOC_J784S4)
        case ENETIF_MMR_LOCK5:
            kick0 = &regs->LOCK5_KICK0;
            kick1 = &regs->LOCK5_KICK1;
            break;
#endif

#if defined (SOC_J721E) || defined (SOC_AM65XX)
        case ENETIF_MMR_LOCK6:
            kick0 = &regs->LOCK6_KICK0;
            kick1 = &regs->LOCK6_KICK1;
            break;
#endif

        case ENETIF_MMR_LOCK7:
            kick0 = &regs->LOCK7_KICK0;
            kick1 = &regs->LOCK7_KICK1;
            break;

        default:
            EnetIf_assert(false);
            break;
    }

    if ((NULL != kick0) && (NULL != kick1))
    {
        if (ENETIF_UNLOCK_MMR == lock)
        {
            prevLockState = EnetIf_unlockMmr(kick0, kick1);
        }
        else
        {
            prevLockState = EnetIf_lockMmr(kick0, kick1);
        }
    }

    munmap_device_memory((void *)regs,sizeof(CSL_main_ctrl_mmr_cfg0Regs));
    return prevLockState;
#else
#error "Unsupported platform"
#endif
}

void EnetIf_selectCptsClock(Enet_Type enetType,
                             EnetIf_CptsClkSelMux clkSelMux)
{
#if defined(SOC_AM65XX) || defined(SOC_J721E) || defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4)
    uint32_t muxVal;
    EnetIf_MmrLockState prevLockState;

    muxVal = (uint32_t) clkSelMux;
    switch (enetType)
    {
        case ENET_CPSW_2G:
        {
#if defined (CPSW2G)
            CSL_mcu_ctrl_mmr_cfg0Regs *mcuRegs;
            mcuRegs = (CSL_mcu_ctrl_mmr_cfg0Regs *)mmap_device_memory(0,
                sizeof(CSL_mcu_ctrl_mmr_cfg0Regs), PROT_READ|PROT_WRITE|PROT_NOCACHE, 0,
                (uint64_t)CSL_MCU_CTRL_MMR0_CFG0_BASE);

            EnetIf_assert(mcuRegs != NULL);

            prevLockState = EnetIf_mcuMmrCtrl(ENETIF_MMR_LOCK1, ENETIF_UNLOCK_MMR);
            CSL_REG32_FINS(&mcuRegs->MCU_ENET_CLKSEL,
                           MCU_CTRL_MMR_CFG0_MCU_ENET_CLKSEL_CPTS_CLKSEL,
                           muxVal);
            if (prevLockState == ENETIF_LOCK_MMR)
            {
                EnetIf_mcuMmrCtrl(ENETIF_MMR_LOCK1, ENETIF_LOCK_MMR);
            }

            EnetIf_print("Success - write to MCU_ENET_CLKSEL - 0x%x",
                CSL_REG32_RD(&mcuRegs->MCU_ENET_CLKSEL));

            munmap_device_memory((void *)mcuRegs,sizeof(CSL_mcu_ctrl_mmr_cfg0Regs));
#elif defined (CPSW2G_MAIN)
            CSL_main_ctrl_mmr_cfg0Regs *mainRegs;
            mainRegs = (CSL_main_ctrl_mmr_cfg0Regs *)mmap_device_memory(0,
                sizeof(CSL_main_ctrl_mmr_cfg0Regs), PROT_READ|PROT_WRITE|PROT_NOCACHE, 0,
                (uint64_t)CSL_CTRL_MMR0_CFG0_BASE);

            EnetIf_assert(mainRegs != NULL);

            prevLockState = EnetIf_mainMmrCtrl(ENETIF_MMR_LOCK1, ENETIF_UNLOCK_MMR);
            CSL_REG32_FINS(&mainRegs->CPSW2_CLKSEL,
                           MAIN_CTRL_MMR_CFG0_CPSW2_CLKSEL_CPTS_CLKSEL,
                           muxVal);
            if (prevLockState == ENETIF_LOCK_MMR)
            {
                EnetIf_mainMmrCtrl(ENETIF_MMR_LOCK1, ENETIF_LOCK_MMR);
            }

            EnetIf_print("Success - write to MAIN CPSW2_CLKSEL - 0x%x",
                CSL_REG32_RD(&mainRegs->CPSW_CLKSEL));

            munmap_device_memory((void *)mainRegs,sizeof(CSL_main_ctrl_mmr_cfg0Regs));
#endif
        }
        break;

#if defined(SOC_J721E) || defined(SOC_J7200) || defined(SOC_J784S4)
        case ENET_CPSW_5G:
        case ENET_CPSW_9G:
        {
            CSL_main_ctrl_mmr_cfg0Regs *mainRegs;
            mainRegs = (CSL_main_ctrl_mmr_cfg0Regs *)mmap_device_memory(0,
                sizeof(CSL_main_ctrl_mmr_cfg0Regs), PROT_READ|PROT_WRITE|PROT_NOCACHE, 0,
                (uint64_t)CSL_CTRL_MMR0_CFG0_BASE);

            EnetIf_assert(mainRegs != NULL);

#if defined(SOC_J721E) || defined(SOC_J7200)
            prevLockState = EnetIf_mainMmrCtrl(ENETIF_MMR_LOCK1, ENETIF_UNLOCK_MMR);
            CSL_REG32_FINS(&mainRegs->CPSW_CLKSEL,
                           MAIN_CTRL_MMR_CFG0_CPSW_CLKSEL_CPTS_CLKSEL,
                           muxVal);
            if (prevLockState == ENETIF_LOCK_MMR)
            {
                EnetIf_mainMmrCtrl(ENETIF_MMR_LOCK1, ENETIF_LOCK_MMR);
            }

            EnetIf_print("Success - write to CPSW_CLKSEL - 0x%x",
                CSL_REG32_RD(&mainRegs->CPSW_CLKSEL));
#elif defined(SOC_J784S4)
            prevLockState = EnetIf_mainMmrCtrl(ENETIF_MMR_LOCK2, ENETIF_UNLOCK_MMR);
            CSL_REG32_FINS(&mainRegs->CPSW_CLKSEL,
                           MAIN_CTRL_MMR_CFG0_CPSW_CLKSEL_CPTS_CLKSEL,
                           muxVal);
            if (prevLockState == ENETIF_LOCK_MMR)
            {
                EnetIf_mainMmrCtrl(ENETIF_MMR_LOCK2, ENETIF_LOCK_MMR);
            }

            EnetIf_print("Success - write to CPSW_CLKSEL - 0x%x",
                CSL_REG32_RD(&mainRegs->CPSW_CLKSEL));
#endif

            munmap_device_memory((void *)mainRegs,sizeof(CSL_main_ctrl_mmr_cfg0Regs));
        }
        break;
#endif
        default:
            EnetIf_assert(false);
            break;
    }

#else
#error "Unsupported platform"
#endif
}

int32_t EnetIf_allocRxFlow(Enet_Handle hEnet,
                            uint32_t coreKey,
                            uint32_t coreId,
                            uint32_t *rxFlowStartIdx,
                            uint32_t *flowIdx)
{
    int32_t status = ENET_SOK;
    Enet_IoctlPrms prms;
    EnetRm_AllocRxFlowInArgs inArgs;
    EnetRm_AllocRxFlow allocRxFlowOutArgs;

    inArgs.coreKey = coreKey;
    inArgs.chIdx   = 0U;

    ENET_IOCTL_SET_INOUT_ARGS(&prms, &inArgs, &allocRxFlowOutArgs);
    status = Enet_ioctl(hEnet,
                        coreId,
                        ENET_RM_IOCTL_ALLOC_RX_FLOW,
                        &prms);

    if (status == ENET_SOK)
    {
        *rxFlowStartIdx = allocRxFlowOutArgs.startIdx;
        *flowIdx        = allocRxFlowOutArgs.flowIdx;
    }
    else
    {
        EnetIf_print("%s:%d Error: IOCTL_ALLOC_RX_FLOW failed : %d",
            __FUNCTION__, __LINE__, status);
    }

    return status;
}

static int32_t EnetIf_allocTxCh(Enet_Handle hEnet,
                                 uint32_t coreKey,
                                 uint32_t coreId,
                                 uint32_t *txPSILThreadId)
{
    int32_t status = ENET_SOK;
    Enet_IoctlPrms prms;

    /* Allocate Tx Ch */
    ENET_IOCTL_SET_INOUT_ARGS(&prms, &coreKey, txPSILThreadId);
    status = Enet_ioctl(hEnet,
                        coreId,
                        ENET_RM_IOCTL_ALLOC_TX_CH_PEERID,
                        &prms);
    if (status != ENET_SOK)
    {
        *txPSILThreadId = ENET_RM_TXCHNUM_INVALID;
        EnetIf_print("%s:%d Error: IOCTL_ALLOC_TX_CH_PEERID failed: %d",
            __FUNCTION__, __LINE__, status);
    }

    return status;
}

int32_t EnetIf_allocMac(Enet_Handle hEnet,
                         uint32_t coreKey,
                         uint32_t coreId,
                         uint8_t *macAddress)
{
    int32_t status = ENET_SOK;
    Enet_IoctlPrms prms;
    EnetRm_AllocMacAddrOutArgs allocMacOutArgs;

    ENET_IOCTL_SET_INOUT_ARGS(&prms, &coreKey, &allocMacOutArgs);
    status = Enet_ioctl(hEnet,
                        coreId,
                        ENET_RM_IOCTL_ALLOC_MAC_ADDR,
                        &prms);

    if (status == ENET_SOK)
    {
        memcpy(macAddress, allocMacOutArgs.macAddr, sizeof(allocMacOutArgs.macAddr));
    }
    else
    {
        EnetIf_print("%s:%d Error: IOCTL_ALLOC_MAC_ADDR failed : %d",
            __FUNCTION__, __LINE__, status);
    }

    return status;
}

int32_t EnetIf_freeRxFlow(Enet_Handle hEnet,
                           uint32_t coreKey,
                           uint32_t coreId,
                           uint32_t rxFlowIdx)
{
    int32_t status = ENET_SOK;
    Enet_IoctlPrms prms;
    EnetRm_FreeRxFlowInArgs freeRxFlowInArgs;

    /*Free Rx Flow*/
    freeRxFlowInArgs.coreKey = coreKey;
    freeRxFlowInArgs.flowIdx = rxFlowIdx;
    freeRxFlowInArgs.chIdx   = 0U;

    ENET_IOCTL_SET_IN_ARGS(&prms, &freeRxFlowInArgs);
    status = Enet_ioctl(hEnet,
                        coreId,
                        ENET_RM_IOCTL_FREE_RX_FLOW,
                        &prms);

    return status;
}

int32_t EnetIf_freeTxCh(Enet_Handle hEnet,
                         uint32_t coreKey,
                         uint32_t coreId,
                         uint32_t txChNum)
{
    int32_t status = ENET_SOK;
    Enet_IoctlPrms prms;
    EnetRm_FreeTxChInArgs freeTxChInArgs;

    /* Release Tx Ch */
    freeTxChInArgs.coreKey = coreKey;
    freeTxChInArgs.txChNum = txChNum;

    ENET_IOCTL_SET_IN_ARGS(&prms, &freeTxChInArgs);
    status = Enet_ioctl(hEnet,
                        coreId,
                        ENET_RM_IOCTL_FREE_TX_CH_PEERID,
                        &prms);

    return status;
}

int32_t EnetIf_freeMac(Enet_Handle hEnet,
                        uint32_t coreKey,
                        uint32_t coreId,
                        uint8_t *macAddress)
{
    int32_t status = ENET_SOK;
    Enet_IoctlPrms prms;
    EnetRm_FreeMacAddrInArgs freeMacInArgs;

    freeMacInArgs.coreKey = coreKey;
    ENET_UTILS_ARRAY_COPY(freeMacInArgs.macAddr, macAddress);

    ENET_IOCTL_SET_IN_ARGS(&prms, &freeMacInArgs);
    status = Enet_ioctl(hEnet,
                        coreId,
                        ENET_RM_IOCTL_FREE_MAC_ADDR,
                        &prms);

    if (status != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: IOCTL_FREE_MAC_ADDR failed : %d",
            __FUNCTION__, __LINE__, status);
    }

    return status;
}

int32_t EnetIf_registerDefaultRxFlow(Enet_Handle hEnet,
                                      uint32_t coreKey,
                                      uint32_t coreId,
                                      uint32_t rxFlowStartIdx,
                                      uint32_t rxFlowIdx)
{
    int32_t status = ENET_SOK;
    Enet_IoctlPrms prms;
    Enet_DfltFlowInfo defaultFlow;

    defaultFlow.coreKey  = coreKey;
    defaultFlow.chIdx    = 0U;
    defaultFlow.startIdx = rxFlowStartIdx;
    defaultFlow.flowIdx  = rxFlowIdx;
    ENET_IOCTL_SET_IN_ARGS(&prms, &defaultFlow);
    status = Enet_ioctl(hEnet,
                        coreId,
                        ENET_IOCTL_REGISTER_RX_DEFAULT_FLOW,
                        &prms);

    if (status != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: REGISTER_RX_DEFAULT_FLOW failed : %d",
            __FUNCTION__, __LINE__, status);
    }

    return status;
}

int32_t EnetIf_unregisterDefaultRxFlow(Enet_Handle hEnet,
                                        uint32_t coreKey,
                                        uint32_t coreId,
                                        uint32_t rxFlowStartIdx,
                                        uint32_t rxFlowIdx)
{
    int32_t status = ENET_SOK;
    Enet_IoctlPrms prms;
    Enet_DfltFlowInfo defaultFlow;

    defaultFlow.coreKey  = coreKey;
    defaultFlow.chIdx    = 0U;
    defaultFlow.startIdx = rxFlowStartIdx;
    defaultFlow.flowIdx  = rxFlowIdx;
    ENET_IOCTL_SET_IN_ARGS(&prms, &defaultFlow);
    status = Enet_ioctl(hEnet,
                        coreId,
                        ENET_IOCTL_UNREGISTER_RX_DEFAULT_FLOW,
                        &prms);

    if (status != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: UNREGISTER_RX_DEFAULT_FLOW failed : %d",
            __FUNCTION__, __LINE__, status);
    }

    return status;
}

int32_t EnetIf_registerDstMacRxFlow(Enet_Handle hEnet,
                                     uint32_t coreKey,
                                     uint32_t coreId,
                                     uint32_t rxFlowStartIdx,
                                     uint32_t rxFlowIdx,
                                     uint8_t macAddress[ENET_MAC_ADDR_LEN])
{
    int32_t status = ENET_SOK;
    Enet_IoctlPrms prms;
    Enet_MacDstFlowInfo macDstFlow;

    macDstFlow.coreKey  = coreKey;
    macDstFlow.startIdx = rxFlowStartIdx;
    macDstFlow.flowIdx  = rxFlowIdx;
    ENET_UTILS_ARRAY_COPY(macDstFlow.macAddress, macAddress);
    ENET_IOCTL_SET_IN_ARGS(&prms, &macDstFlow);
    status = Enet_ioctl(hEnet,
                        coreId,
                        ENET_IOCTL_REGISTER_DSTMAC_RX_FLOW,
                        &prms);

    if (status != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: REGISTER_DSTMAC_RX_FLOW failed : %d",
            __FUNCTION__, __LINE__, status);
    }

    return status;
}

int32_t EnetIf_unregisterDstMacRxFlow(Enet_Handle hEnet,
                                       uint32_t coreKey,
                                       uint32_t coreId,
                                       uint32_t rxFlowStartIdx,
                                       uint32_t rxFlowIdx,
                                       uint8_t macAddress[ENET_MAC_ADDR_LEN])
{
    int32_t status = ENET_SOK;
    Enet_IoctlPrms prms;
    Enet_MacDstFlowInfo macDstFlow;

    macDstFlow.coreKey  = coreKey;
    macDstFlow.startIdx = rxFlowStartIdx;
    macDstFlow.flowIdx  = rxFlowIdx;
    ENET_UTILS_ARRAY_COPY(macDstFlow.macAddress, macAddress);
    ENET_IOCTL_SET_IN_ARGS(&prms, &macDstFlow);
    status = Enet_ioctl(hEnet,
                        coreId,
                        ENET_IOCTL_UNREGISTER_DSTMAC_RX_FLOW,
                        &prms);

    if (status != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: UNREGISTER_DSTMAC_RX_FLOW failed : %d",
            __FUNCTION__, __LINE__, status);
    }

    return status;
}

bool EnetIf_isPortLinkUp(Enet_Handle hEnet,
                          uint32_t coreId,
                          Enet_MacPort portNum)
{
    Enet_IoctlPrms prms;
    Enet_MacPort portLinkInArgs;
    bool linked = true;
    int32_t status;

    portLinkInArgs = portNum;
    ENET_IOCTL_SET_INOUT_ARGS(&prms, &portLinkInArgs, &linked);

    status = Enet_ioctl(hEnet, coreId, ENET_PER_IOCTL_IS_PORT_LINK_UP, &prms);
    if (status != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: failed to get port %u's link status: %d",
              __FUNCTION__, __LINE__, portNum, status);
        linked = false;
    }

    return linked;
}

static void EnetIf_reduceCoreMacAllocation(EnetRm_ResPrms *rscPrms,
                                            uint32_t *pReduceCount,
                                            uint32_t coreMinCount,
                                            bool skipCore,
                                            uint32_t skipCoreId)
{
    uint32_t i;

    for (i = 0; (i < rscPrms->numCores) && (*pReduceCount > 0); i++)
    {
        if ((rscPrms->coreDmaResInfo[i].numMacAddress > coreMinCount)
            &&
            ((skipCore == false) || (skipCoreId != rscPrms->coreDmaResInfo[i].coreId)))
        {
            uint32_t coreMacAddrReducedCount = (rscPrms->coreDmaResInfo[i].numMacAddress - coreMinCount);

            if (*pReduceCount >= coreMacAddrReducedCount)
            {
                *pReduceCount -= coreMacAddrReducedCount;
            }
            else
            {
                coreMacAddrReducedCount -= *pReduceCount;
                *pReduceCount            = 0;
            }

            EnetIf_print("EnetIf_reduceCoreMacAllocation: "
                           "Reduced Mac Address Allocation for CoreId:%u From %u To %u ",
                           rscPrms->coreDmaResInfo[i].coreId,
                           rscPrms->coreDmaResInfo[i].numMacAddress,
                           (rscPrms->coreDmaResInfo[i].numMacAddress - coreMacAddrReducedCount));
            rscPrms->coreDmaResInfo[i].numMacAddress -= coreMacAddrReducedCount;
        }
    }
}

static void EnetIf_updateMacResourcePartition(EnetRm_ResPrms *rscPrms,
                                               uint32_t availMacCount,
                                               uint32_t selfCoreId)
{
    uint32_t totalRscPatitionMacCount;
    uint32_t i;

    totalRscPatitionMacCount = 0;
    for (i = 0; i < rscPrms->numCores; i++)
    {
        totalRscPatitionMacCount += rscPrms->coreDmaResInfo[i].numMacAddress;
    }

    if (totalRscPatitionMacCount > availMacCount)
    {
        uint32_t reduceCount = totalRscPatitionMacCount - availMacCount;

        /* First reduce mac count for cores with more than one mac address allocation */
        EnetIf_reduceCoreMacAllocation(rscPrms, &reduceCount, 1, false, selfCoreId);
        if (reduceCount)
        {
            /* Next reduce mac address for core other than self core to 0 */
            EnetIf_reduceCoreMacAllocation(rscPrms, &reduceCount, 0, true, selfCoreId);
        }

        /* Finally reduce self core also to 0 */
        if (reduceCount)
        {
            /* Next reduce mac address for core other than self core to 0 */
            EnetIf_reduceCoreMacAllocation(rscPrms, &reduceCount, 0, false, selfCoreId);
        }

        EnetIf_assert(reduceCount == 0);
    }
}

void EnetIf_initResourceConfig(Enet_Type enetType,
                               uint32_t instId,
                               uint32_t selfCoreId,
                               EnetRm_ResCfg *rscCfg,
                               uint8_t *currentMacAddr)
{
    const EnetRm_ResPrms *rscPrms         = EnetIfRm_getResPartInfo(enetType, instId);
    const EnetRm_IoctlPermissionTable *ioPerms = EnetIfRm_getIoctlPermissionInfo(enetType, instId);

    EnetIf_assert(rscPrms != NULL);
    if (rscPrms)
        rscCfg->resPartInfo = *rscPrms;

    EnetIf_assert(ioPerms != NULL);
    if (ioPerms)
        rscCfg->ioctlPermissionInfo = *ioPerms;

#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    EnetIf_assert(enetType == ENET_CPSW_2G);
#elif defined (CPSW9G) || defined (CPSW5G)
#if defined(SOC_J721E) || defined(SOC_J784S4)
    EnetIf_assert(enetType == ENET_CPSW_9G);
#elif defined(SOC_J7200)
    EnetIf_assert(enetType == ENET_CPSW_5G);
#endif
#else
    #error "Unsupported!"
#endif

    // for qnx, we would need to use the current mac address passed by devnp
    memcpy(rscCfg->macList.macAddress[0], currentMacAddr, ETH_MAC_ADDR_LEN);
    rscCfg->macList.numMacAddress = 1;

    EnetIf_updateMacResourcePartition(&rscCfg->resPartInfo,
                                       rscCfg->macList.numMacAddress,
                                       selfCoreId);
    rscCfg->selfCoreId = selfCoreId;
}

void EnetIf_setCommonRxFlowPrms(EnetUdma_OpenRxFlowPrms *pRxFlowPrms, uint32_t numRxPkts)
{

    pRxFlowPrms->numRxPkts           = numRxPkts;
    pRxFlowPrms->disableCacheOpsFlag = false;
    pRxFlowPrms->rxFlowMtu           = MAX_ETH_PACKET_SIZE;

    pRxFlowPrms->ringMemAllocFxn = &EnetIfMem_allocRingMemFxn;
    pRxFlowPrms->ringMemFreeFxn  = &EnetIfMem_freeRingMemFxn;

    pRxFlowPrms->dmaDescAllocFxn = &EnetIfMem_allocDmaDescFxn;
    pRxFlowPrms->dmaDescFreeFxn  = &EnetIfMem_freeDmaDescFxn;
}

void EnetIf_addHostPortEntry(Enet_Handle hEnet,
                              uint32_t coreId,
                              uint8_t *macAddr)
{
    int32_t status;
    Enet_IoctlPrms prms;
    uint32_t setUcastOutArgs;
    CpswAle_SetUcastEntryInArgs setUcastInArgs;

    memset(&setUcastInArgs, 0, sizeof(setUcastInArgs));
    memcpy(&setUcastInArgs.addr.addr[0U], macAddr, sizeof(setUcastInArgs.addr.addr));
    setUcastInArgs.addr.vlanId  = 0U;
    setUcastInArgs.info.portNum = CPSW_ALE_HOST_PORT_NUM;
    setUcastInArgs.info.blocked = false;
    setUcastInArgs.info.secure  = true;
    setUcastInArgs.info.super   = false;
    setUcastInArgs.info.ageable = false;
    setUcastInArgs.info.trunk   = false;

    ENET_IOCTL_SET_INOUT_ARGS(&prms, &setUcastInArgs, &setUcastOutArgs);

    status = Enet_ioctl(hEnet, coreId, CPSW_ALE_IOCTL_ADD_UCAST,
                        &prms);
    if (status != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: failed CPSW_ALE_IOCTL_ADD_UCAST: %d",
             __FUNCTION__, __LINE__, status);
    }

    EnetIf_assert(status == ENET_SOK);
}

static void EnetIf_delAddrEntry(Enet_Handle hEnet,
                                 uint32_t coreId,
                                 uint8_t *macAddr)
{
    int32_t status;
    Enet_IoctlPrms prms;
    CpswAle_MacAddrInfo addrInfo;

    memset(&addrInfo, 0, sizeof(addrInfo));
    memcpy(&addrInfo.addr[0U], macAddr, sizeof(addrInfo.addr));
    addrInfo.vlanId = 0U;

    ENET_IOCTL_SET_IN_ARGS(&prms, &addrInfo);

    status = Enet_ioctl(hEnet, coreId, CPSW_ALE_IOCTL_REMOVE_ADDR, &prms);
    if (status != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: failed CPSW_ALE_IOCTL_REMOVE_ADDR: %d",
             __FUNCTION__, __LINE__, status);
    }

    EnetIf_assert(status == ENET_SOK);
}

void EnetIf_setCommonTxChPrms(EnetUdma_OpenTxChPrms *pTxChPrms, uint32_t numTxPkts)
{

    pTxChPrms->numTxPkts           = numTxPkts;
    pTxChPrms->disableCacheOpsFlag = false;

    pTxChPrms->ringMemAllocFxn = &EnetIfMem_allocRingMemFxn;
    pTxChPrms->ringMemFreeFxn  = &EnetIfMem_freeRingMemFxn;

    pTxChPrms->dmaDescAllocFxn = &EnetIfMem_allocDmaDescFxn;
    pTxChPrms->dmaDescFreeFxn  = &EnetIfMem_freeDmaDescFxn;
}
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
void EnetIf_openTxCh(Enet_Handle hEnet,
                      uint32_t coreKey,
                      uint32_t coreId,
                      uint32_t *pTxChNum,
                      EnetDma_TxChHandle *pTxChHandle,
                      EnetUdma_OpenTxChPrms *pCpswTxChCfg)
{
    EnetDma_Handle hDma = Enet_getDmaHandle(hEnet);
    int32_t status;

    EnetIf_assert(hDma != NULL);

    status = EnetIf_allocTxCh(hEnet,
                                    coreKey,
                                    coreId,
                                    pTxChNum);
    EnetIf_assert(ENET_SOK == status);

    pCpswTxChCfg->chNum = *pTxChNum;

    *pTxChHandle = EnetDma_openTxCh(hDma, pCpswTxChCfg);
    EnetIf_assert(NULL != *pTxChHandle);
}

void EnetIf_closeTxCh(Enet_Handle hEnet,
                       uint32_t coreKey,
                       uint32_t coreId,
                       EnetDma_PktQ *pFqPktInfoQ,
                       EnetDma_PktQ *pCqPktInfoQ,
                       EnetDma_TxChHandle hTxChHandle,
                       uint32_t txChNum)
{
    int32_t status;

    EnetQueue_initQ(pFqPktInfoQ);
    EnetQueue_initQ(pCqPktInfoQ);

    // Note: No need to call disable as we are not enabling TX Event
    //EnetDma_disableTxEvent(hTxChHandle);

    status = EnetDma_closeTxCh(hTxChHandle, pFqPktInfoQ, pCqPktInfoQ);
    EnetIf_assert(ENET_SOK == status);

    status = EnetIf_freeTxCh(hEnet,
                               coreKey,
                               coreId,
                               txChNum);
    EnetIf_assert(ENET_SOK == status);
}

void EnetIf_openRxFlow(Enet_Handle hEnet,
                        uint32_t coreKey,
                        uint32_t coreId,
                        bool useDefaultFlow,
                        uint32_t *pRxFlowStartIdx,
                        uint32_t *pRxFlowIdx,
                        uint8_t macAddr[ENET_MAC_ADDR_LEN],
                        EnetDma_RxChHandle *pRxFlowHandle,
                        EnetUdma_OpenRxFlowPrms *pRxFlowPrms)
{
    EnetDma_Handle hDma = Enet_getDmaHandle(hEnet);
    int32_t status = ENET_SOK;

    EnetIf_assert(hDma != NULL);

    status = EnetIf_allocRxFlow(hEnet,
                                  coreKey,
                                  coreId,
                                  pRxFlowStartIdx,
                                  pRxFlowIdx);
    EnetIf_assert(status == ENET_SOK);

    pRxFlowPrms->startIdx = *pRxFlowStartIdx;
    pRxFlowPrms->flowIdx  = *pRxFlowIdx;

    *pRxFlowHandle = EnetDma_openRxCh(hDma, pRxFlowPrms);
    EnetIf_assert(*pRxFlowHandle != NULL);

    status = EnetIf_allocMac(hEnet, coreKey, coreId, macAddr);
    EnetIf_assert(status == ENET_SOK);

    EnetIf_addHostPortEntry(hEnet, coreId, macAddr);
    if (useDefaultFlow)
    {
        status = EnetIf_registerDefaultRxFlow(hEnet,
                                                coreKey,
                                                coreId,
                                                *pRxFlowStartIdx,
                                                *pRxFlowIdx);
    }
    else
    {
        status = EnetIf_registerDstMacRxFlow(hEnet,
                                               coreKey,
                                               coreId,
                                               *pRxFlowStartIdx,
                                               *pRxFlowIdx,
                                               macAddr);
    }

    EnetIf_assert(status == ENET_SOK);
}

void EnetIf_closeRxFlow(Enet_Handle hEnet,
                         uint32_t coreKey,
                         uint32_t coreId,
                         bool useDefaultFlow,
                         EnetDma_PktQ *pFqPktInfoQ,
                         EnetDma_PktQ *pCqPktInfoQ,
                         uint32_t rxFlowStartIdx,
                         uint32_t rxFlowIdx,
                         uint8_t macAddr[ENET_MAC_ADDR_LEN],
                         EnetDma_RxChHandle hRxFlow)
{
    int32_t status = ENET_SOK;

    EnetQueue_initQ(pFqPktInfoQ);
    EnetQueue_initQ(pCqPktInfoQ);

    if (g_cpsw->rx_pacing == 0) {
        EnetDma_disableRxEvent(hRxFlow);
    }

    if (useDefaultFlow)
    {
        status = EnetIf_unregisterDefaultRxFlow(hEnet,
                                                 coreKey,
                                                 coreId,
                                                 rxFlowStartIdx,
                                                 rxFlowIdx);
    }
    else
    {
        status = EnetIf_unregisterDstMacRxFlow(hEnet,
                                                coreKey,
                                                coreId,
                                                rxFlowStartIdx,
                                                rxFlowIdx,
                                                macAddr);
    }

    EnetIf_assert(status == ENET_SOK);
    EnetIf_delAddrEntry(hEnet, coreId, macAddr);
    status = EnetDma_closeRxCh(hRxFlow,
                                 pFqPktInfoQ,
                                 pCqPktInfoQ);
    EnetIf_assert(status == ENET_SOK);

    status = EnetIf_freeMac(hEnet,
                                  coreKey,
                                  coreId,
                                  macAddr);
    EnetIf_assert(status == ENET_SOK);

    status = EnetIf_freeRxFlow(hEnet,
                                     coreKey,
                                     coreId,
                                     rxFlowIdx);
    EnetIf_assert(status == ENET_SOK);
}
#elif defined (CPSW9G) || defined (CPSW5G)
void EnetIf_openTxCh(EnetIf_Obj *obj,
                      EnetUdma_OpenTxChPrms *pCpswTxChCfg)
{
    uint32_t chRelPriority;

    /* Single TX channel is supported by this driver */
    chRelPriority = 0U;

    CpswProxy_allocTxCh(obj->hCpswProxy,
                        &obj->txChNum,
                        chRelPriority);
    pCpswTxChCfg->chNum = obj->txChNum;

    obj->hTxCh = EnetDma_openTxCh(obj->dma, pCpswTxChCfg);
    EnetIf_assert(NULL != obj->hTxCh);
}

void EnetIf_closeTxCh(EnetIf_Obj *obj,
                       EnetDma_PktQ *pFqPktInfoQ,
                       EnetDma_PktQ *pCqPktInfoQ)
{
    int32_t status;

    EnetQueue_initQ(pFqPktInfoQ);
    EnetQueue_initQ(pCqPktInfoQ);

    // Note: No need to call disable as we are not enabling TX Event
    //EnetDma_disableTxEvent(obj->hTxCh);

    status = EnetDma_closeTxCh(obj->hTxCh, pFqPktInfoQ, pCqPktInfoQ);
    EnetIf_assert(ENET_SOK == status);

    CpswProxy_freeTxCh(obj->hCpswProxy,
                       obj->txChNum);
}

void EnetIf_openRxFlow(EnetIf_Obj *obj,
                        bool useDefaultFlow,
                        EnetUdma_OpenRxFlowPrms *pRxFlowPrms)
{
    uint32_t flowIdx;

    /* Single RX flow is supported by this driver */
    flowIdx = 0U;

    CpswProxy_allocRxFlow(obj->hCpswProxy,
                          &obj->rxStartFlowIdx,
                          &obj->rxFlowIdx,
                          flowIdx);

    pRxFlowPrms->startIdx = obj->rxStartFlowIdx;
    pRxFlowPrms->flowIdx  = obj->rxFlowIdx;

    obj->hRxFlow = EnetDma_openRxCh(obj->dma, pRxFlowPrms);
    EnetIf_assert(obj->hRxFlow != NULL);

    if (useDefaultFlow)
    {
        CpswProxy_registerDefaultRxFlow(obj->hCpswProxy,
                                        obj->rxStartFlowIdx,
                                        obj->rxFlowIdx);
    }
    else
    {
        CpswProxy_registerDstMacRxFlow(obj->hCpswProxy,
                                       obj->rxStartFlowIdx,
                                       obj->rxFlowIdx,
                                       obj->hostMacAddr);
    }
}

void EnetIf_closeRxFlow(EnetIf_Obj *obj,
                         bool useDefaultFlow,
                         EnetDma_PktQ *pFqPktInfoQ,
                         EnetDma_PktQ *pCqPktInfoQ)
{
    int32_t status = ENET_SOK;

    EnetQueue_initQ(pFqPktInfoQ);
    EnetQueue_initQ(pCqPktInfoQ);

    if (g_cpsw->rx_pacing == 0) {
        EnetDma_disableRxEvent(obj->hRxFlow);
    }

    if (useDefaultFlow)
    {
        CpswProxy_unregisterDefaultRxFlow(obj->hCpswProxy,
                                          obj->rxStartFlowIdx,
                                          obj->rxFlowIdx);
    }
    else
    {
        CpswProxy_unregisterDstMacRxFlow(obj->hCpswProxy,
                                         obj->rxStartFlowIdx,
                                         obj->rxFlowIdx,
                                         obj->hostMacAddr);
    }

    status = EnetDma_closeRxCh(obj->hRxFlow,
                                 pFqPktInfoQ,
                                 pCqPktInfoQ);
    EnetIf_assert(status == ENET_SOK);

    CpswProxy_freeRxFlow(obj->hCpswProxy,
                         obj->rxStartFlowIdx,
                         obj->rxFlowIdx);
}
#else
    error "Unsupported!"
#endif


static void EnetIf_printDmaDescStats(EnetDma_DmaDescStats *descstats)
{
    // TODO when DMA stats are enabled.
}

static void EnetIf_printCbStats(EnetDma_CbStats *cbStats)
{
    uint32_t i;

    EnetIf_printStatsNonZero("Data Notify Count          = %llu", cbStats->dataNotifyCnt);
    EnetIf_printStatsNonZero("Zero Notify Count          = %llu", cbStats->zeroNotifyCnt);
    EnetIf_printStatsNonZero("Total Packets Count        = %llu", cbStats->totalPktCnt);
    EnetIf_printStatsNonZero("Total Cycles Count         = %llu", cbStats->totalCycleCnt);
    EnetIf_printStatsNonZero("Packets per Notify Max     = %llu", cbStats->pktsPerNotifyMax);
    for (i = 0U; i < ENET_DMA_STATS_HISTORY_CNT; i++)
    {
        EnetIf_printStatsWithIdxNonZero("Packets per Notify[%d] = %llu", i, cbStats->pktsPerNotify[i]);
    }

    EnetIf_printStatsNonZero("Cycles per Notify Max      = %llu", cbStats->cycleCntPerNotifyMax);
    for (i = 0U; i < ENET_DMA_STATS_HISTORY_CNT; i++)
    {
        EnetIf_printStatsWithIdxNonZero("Cycles per Notify[%d]  = %llu", i, cbStats->cycleCntPerNotify[i]);
    }

    EnetIf_printStatsNonZero("Cycles per Packet Max      = %llu", cbStats->cycleCntPerPktMax);
    for (i = 0U; i < ENET_DMA_STATS_HISTORY_CNT; i++)
    {
        EnetIf_printStatsWithIdxNonZero("Cycles per Packet[%d]  = %llu", i, cbStats->cycleCntPerPkt[i]);
    }

    for (i = 0U; i < ENET_DMA_STATS_HISTORY_CNT; i++)
    {
        EnetIf_printStatsWithIdxNonZero("Ready Desc Q Count[%d] = %llu", i, cbStats->readyDmaDescQCnt[i]);
    }
}

static void EnetIf_printRxChStats(EnetDma_RxChStats *stats)
{
    EnetIf_print(" RX Flow Statistics");
    EnetIf_print("-----------------------------------------");
    EnetIf_printCbStats(&stats->submitPktStats);
    EnetIf_printCbStats(&stats->retrievePktStats);
    EnetIf_printDmaDescStats(&stats->dmaDescStats);

    EnetIf_printStatsNonZero("RX Submit Packet EnQ count           = %llu", stats->rxSubmitPktEnq);
    EnetIf_printStatsNonZero("RX Submit Packet Underflow           = %llu", stats->rxSubmitPktUnderFlowCnt);
    EnetIf_printStatsNonZero("RX Submit Packet DeQ count           = %llu", stats->rxRetrievePktDeq);
}

static void EnetIf_printTxChStats(EnetDma_TxChStats *stats)
{
    EnetIf_print(" RX Flow Statistics");
    EnetIf_print("-----------------------------------------");
    EnetIf_printCbStats(&stats->submitPktStats);
    EnetIf_printCbStats(&stats->retrievePktStats);
    EnetIf_printDmaDescStats(&stats->dmaDescStats);

    EnetIf_printStatsNonZero("TX Submit Packet EnQ count           = %llu", stats->txSubmitPktEnq);
    EnetIf_printStatsNonZero("TX Submit Packet Underflow           = %llu", stats->txSubmitPktOverFlowCnt);
    EnetIf_printStatsNonZero("TX Submit Packet DeQ count           = %llu", stats->txRetrievePktDeq);
}

int32_t EnetIf_showRxFlowStats(EnetDma_RxChHandle hRxFlow)
{
    int32_t retVal = ENET_SOK;

    EnetDma_RxChStats rxFlowStats;

    retVal = EnetDma_getRxChStats(hRxFlow, &rxFlowStats);
    if (ENET_ENOTSUPPORTED != retVal)
    {
        EnetIf_printRxChStats(&rxFlowStats);
        EnetDma_resetRxChStats(hRxFlow);
    }

    return retVal;
}

int32_t EnetIf_showTxChStats(EnetDma_TxChHandle hTxCh)
{
    int32_t retVal = ENET_SOK;

    EnetDma_TxChStats txChStats;

    retVal = EnetDma_getTxChStats(hTxCh, &txChStats);
    if (ENET_ENOTSUPPORTED != retVal)
    {
        EnetIf_printTxChStats(&txChStats);
        EnetDma_resetTxChStats(hTxCh);
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/**
 * \brief  Configures the CPSW2G Subsytem for RGMII mode
 *
 * \param  portNum [IN]    EMAC port number
 * \param  mode    [IN]    Mode selection for the specified port number
 *                         00 - GMII
 *                         01 - RMII
 *                         10 - RGMII
 *                         11 - SGMII
 *
 * \return  ENET_SOK in case of success or appropriate error code
 */
int32_t EnetIfBoard_cpsw2gMacModeConfig(uint32_t portNum, uint8_t mode)
{
#if defined (CPSW2G)
    uint32_t status;
    uintptr_t ethModeCtrl;
    uint32_t regData;
    EnetIf_MmrLockState prevLockState;

    ethModeCtrl = (uintptr_t)mmap_device_memory(0,
        sizeof(uintptr_t), PROT_READ|PROT_WRITE|PROT_NOCACHE, 0,
        (uint64_t)CSL_MCU_CTRL_MMR0_CFG0_BASE + CSL_MCU_CTRL_MMR_CFG0_MCU_ENET_CTRL);

    regData = CSL_REG32_RD(ethModeCtrl);
    regData = mode;
    if (RGMII == mode)
    {
        regData |= (RGMII_ID_DISABLE_MASK);
    }

    prevLockState = EnetIf_mcuMmrCtrl(ENETIF_MMR_LOCK1, ENETIF_UNLOCK_MMR);
    CSL_REG32_WR(ethModeCtrl , regData);
    if (prevLockState == ENETIF_LOCK_MMR)
    {
        EnetIf_mcuMmrCtrl(ENETIF_MMR_LOCK1, ENETIF_LOCK_MMR);
    }

    status = CSL_REG32_RD(ethModeCtrl);
    if (status != regData)
    {
        EnetIf_print("%s:%d Error: Failed to write MCU_ENET_CTRL",
            __FUNCTION__, __LINE__);
        status =  ENET_EFAIL;
    }
    else
    {
        EnetIf_print("Success - write to MCU_ENET_CTRL - 0x%x", status);
        status = ENET_SOK;
    }

    munmap_device_memory((void *)ethModeCtrl,sizeof(uintptr_t));

    return status;
#elif defined (CPSW2G_MAIN)
    uint32_t status;
    uintptr_t ethModeCtrl;
    uint32_t regData;
    EnetIf_MmrLockState prevLockState;

    ethModeCtrl = (uintptr_t)mmap_device_memory(0,
        sizeof(uintptr_t), PROT_READ|PROT_WRITE|PROT_NOCACHE, 0,
        (uint64_t)CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_CPSW2_ENET1_CTRL);

    regData = CSL_REG32_RD(ethModeCtrl);
    regData = mode;
    if (RGMII == mode)
    {
        regData |= (RGMII_ID_DISABLE_MASK);
    }

    prevLockState = EnetIf_mainMmrCtrl(ENETIF_MMR_LOCK1, ENETIF_UNLOCK_MMR);
    CSL_REG32_WR(ethModeCtrl , regData);
    if (prevLockState == ENETIF_LOCK_MMR)
    {
        EnetIf_mainMmrCtrl(ENETIF_MMR_LOCK1, ENETIF_LOCK_MMR);
    }

    status = CSL_REG32_RD(ethModeCtrl);
    if (status != regData)
    {
        EnetIf_print("%s:%d Error: Failed to write MAIN_ENET_CTRL",
            __FUNCTION__, __LINE__);
        status =  ENET_EFAIL;
    }
    else
    {
        EnetIf_print("Success - write to MAIN_ENET_CTRL - 0x%x", status);
        status = ENET_SOK;
    }



    munmap_device_memory((void *)ethModeCtrl,sizeof(uintptr_t));
    return status;
#elif defined (CPSW9G) || defined (CPSW5G)
    /* Nothing to do */
    return ENET_SOK;
#endif
    
}

uint32_t EnetIfBoard_getPhyAddr(Enet_Type enetType,
                                 Enet_MacPort portNum)
{
    uint32_t phyAddr = ENETPHY_INVALID_PHYADDR;

    switch (enetType)
    {
        case ENET_CPSW_2G:
            phyAddr = 0x00U;
            break;

        case ENET_CPSW_9G:
            switch (portNum)
            {
                case ENET_MAC_PORT_1:
                    phyAddr = 0x0CU;
                    break;

                case ENET_MAC_PORT_2:
                    phyAddr = 0x0FU;
                    break;

                case ENET_MAC_PORT_3:
                    phyAddr = 0x00U;
                    break;

                case ENET_MAC_PORT_4:
                    phyAddr = 0x03U;
                    break;

                case ENET_MAC_PORT_5:
                    phyAddr = 0x17U;
                    break;

                default:
                    EnetIf_assert(false);
                    break;
            }

            break;

        default:
            EnetIf_assert(false);
    }

    return phyAddr;
}


void EnetIfBoard_setPhyConfig(Enet_Type enetType,
                               Enet_MacPort portNum,
                               EnetMacPort_Interface *interface,
                               EnetPhy_Cfg *phyCfg)
{
    Dp83867_Cfg extendedConfig;

    Dp83867_initCfg(&extendedConfig);
    EnetPhy_initCfg(phyCfg);
    phyCfg->phyAddr = EnetIfBoard_getPhyAddr(enetType, portNum);

    switch (enetType)
    {
        case ENET_CPSW_2G:
            EnetIf_assert(portNum == ENET_MAC_PORT_FIRST);

            /* DP83867 specific configuration */
            extendedConfig.txClkShiftEn         = true;
            extendedConfig.rxClkShiftEn         = true;
            extendedConfig.txFifoDepth          = 4U;
            extendedConfig.idleCntThresh        = 4U; /* Improves short cable performance */
            extendedConfig.impedanceInMilliOhms = 35000; /* 35 ohms */
            extendedConfig.gpio0Mode            = DP83867_GPIO0_LED3;
            extendedConfig.ledMode[1]           = DP83867_LED_LINKED_100BTX;
            extendedConfig.ledMode[2]           = DP83867_LED_RXTXACT;
            extendedConfig.ledMode[3]           = DP83867_LED_LINKED_1000BT;

            /* RGMII interface type */
            interface->layerType    = ENET_MAC_LAYER_GMII;
            interface->sublayerType = ENET_MAC_SUBLAYER_REDUCED;
            interface->variantType  = ENET_MAC_VARIANT_NONE;
            break;

        case ENET_CPSW_9G:
            /* DP83867 specific configuration */
            extendedConfig.txDelayInPs          = 2750U; /* 2.75 ns */
            extendedConfig.rxDelayInPs          = 2500U; /* 2.50 ns */
            extendedConfig.txFifoDepth          = 4U;
            extendedConfig.idleCntThresh        = 4U; /* Improves short cable performance */
            extendedConfig.txClkShiftEn         = true;
            extendedConfig.rxClkShiftEn         = true;
            extendedConfig.impedanceInMilliOhms = 35000; /* 35 ohms */
            extendedConfig.gpio0Mode            = DP83867_GPIO0_LED3;
            extendedConfig.ledMode[1]           = DP83867_LED_LINKED_100BTX;
            extendedConfig.ledMode[2]           = DP83867_LED_RXTXACT;
            extendedConfig.ledMode[3]           = DP83867_LED_LINKED_1000BT;

            /* RGMII interface type */
            interface->layerType    = ENET_MAC_LAYER_GMII;
            interface->sublayerType = ENET_MAC_SUBLAYER_REDUCED;
            interface->variantType  = ENET_MAC_VARIANT_NONE;
            break;

        default:
            break;
    }

    EnetPhy_setExtendedCfg(phyCfg,
                           &extendedConfig,
                           sizeof(extendedConfig));
}

#if defined (CPSW2G_MAIN)

#define PIN_MODE(mode)                  (mode)
#define PINMUX_END                      (-1)

/** \brief Active mode configurations */
/** \brief Resistor enable */
#define PIN_PULL_DISABLE                (0x1U << 16U)
/** \brief Pull direction */
#define PIN_PULL_DIRECTION              (0x1U << 17U)
/** \brief Receiver enable */
#define PIN_INPUT_ENABLE                (0x1U << 18U)
/** \brief Driver disable */
#define PIN_OUTPUT_DISABLE              (0x1U << 21U)
/** \brief Wakeup enable */
#define PIN_WAKEUP_ENABLE               (0x1U << 29U)

enum EnetIfBoard_pinMainOffsetsCpsw2g
{
    PIN_MCASP1_AXR0      = 0x0C0,
    PIN_MCASP1_AFSX      = 0x0BC,
    PIN_MCASP1_ACLKX     = 0x0B8,
    PIN_MCASP0_AXR12     = 0x0A0,
    PIN_MCASP0_AXR13     = 0x0A4,
    PIN_MCASP0_AXR14     = 0x0A8,
    PIN_MCASP1_AXR3      = 0x0B0,
    PIN_MCASP0_AXR15     = 0x0AC,
    PIN_MCASP0_AXR7      = 0x08C,
    PIN_MCASP0_AXR8      = 0x090,
    PIN_MCASP0_AXR9      = 0x094,
    PIN_MCASP0_AXR10     = 0x098,
    PIN_MCASP1_AXR4      = 0x0B4,
    PIN_MCASP0_AXR11     = 0x09C,
};

/**
 *  \brief Structure defining the pin configuration parameters.
 *
 */
typedef struct EnetIfBoard_pinmuxPerCfg
{
    int16_t pinOffset;
    /**< Register offset for configuring the pin */
    int32_t pinSettings;
    /**< Value to be configured,
          - Active mode configurations like mux mode, pull resistor, and buffer mode
    */
}EnetIfBoard_pinmuxPerCfg_t;

static EnetIfBoard_pinmuxPerCfg_t gCpsw2gPinCfg[] =
{
    /* MyCPSW2G0 -> MDIO0_MDC -> T28 */
    {
        PIN_MCASP1_AXR0, PIN_MODE(6) | \
        ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION))
    },
    /* MyCPSW2G0 -> MDIO0_MDIO -> V28 */
    {
        PIN_MCASP1_AFSX, PIN_MODE(6) | \
        ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION))
    },
    /* MyCPSW2G0 -> RGMII1_RD0 -> AA24 */
    {
        PIN_MCASP1_ACLKX, PIN_MODE(6) | \
        ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION))
    },
    /* MyCPSW2G0 -> RGMII1_RD1 -> AB25 */
    {
        PIN_MCASP0_AXR12, PIN_MODE(6) | \
        ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION))
    },
    /* MyCPSW2G0 -> RGMII1_RD2 -> T23 */
    {
        PIN_MCASP0_AXR13, PIN_MODE(6) | \
        ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION))
    },
    /* MyCPSW2G0 -> RGMII1_RD3 -> U24 */
    {
        PIN_MCASP0_AXR14, PIN_MODE(6) | \
        ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION))
    },
    /* MyCPSW2G0 -> RGMII1_RXC -> AD26 */
    {
        PIN_MCASP1_AXR3, PIN_MODE(6) | \
        ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION))
    },
    /* MyCPSW2G0 -> RGMII1_RX_CTL -> AC25 */
    {
        PIN_MCASP0_AXR15, PIN_MODE(6) | \
        ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION))
    },
    /* MyCPSW2G0 -> RGMII1_TD0 -> T25 */
    {
        PIN_MCASP0_AXR7, PIN_MODE(6) | \
        ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION))
    },
    /* MyCPSW2G0 -> RGMII1_TD1 -> W24 */
    {
        PIN_MCASP0_AXR8, PIN_MODE(6) | \
        ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION))
    },
    /* MyCPSW2G0 -> RGMII1_TD2 -> AA25 */
    {
        PIN_MCASP0_AXR9, PIN_MODE(6) | \
        ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION))
    },
    /* MyCPSW2G0 -> RGMII1_TD3 -> V25 */
    {
        PIN_MCASP0_AXR10, PIN_MODE(6) | \
        ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION))
    },
    /* MyCPSW2G0 -> RGMII1_TXC -> U25 */
    {
        PIN_MCASP1_AXR4, PIN_MODE(6) | \
        ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION))
    },
    /* MyCPSW2G0 -> RGMII1_TX_CTL -> T24 */
    {
        PIN_MCASP0_AXR11, PIN_MODE(6) | \
        ((PIN_PULL_DISABLE | PIN_INPUT_ENABLE) & (~PIN_PULL_DIRECTION))
    },
    {PINMUX_END}
};

#define BOARD_MAIN_PMUX_CTRL_ADDR   (CSL_CTRL_MMR0_CFG0_BASE + 0x1C000)


void EnetBoard_setupGESI()
{
    uint32_t rdRegVal;
    uintptr_t baseAddr;
    EnetIfBoard_pinmuxPerCfg_t *pInstanceData = gCpsw2gPinCfg;

    baseAddr = (uintptr_t)mmap_device_memory(0,
        0x1000, PROT_READ|PROT_WRITE|PROT_NOCACHE, 0,
        (uint64_t)BOARD_MAIN_PMUX_CTRL_ADDR);

    for (uint32_t k = 0; (PINMUX_END != pInstanceData[k].pinOffset); k++)
    {
        rdRegVal = CSL_REG32_RD((baseAddr + pInstanceData[k].pinOffset));
        CSL_REG32_WR((baseAddr + pInstanceData[k].pinOffset), pInstanceData[k].pinSettings);
        EnetIf_print("Pinmux for 0x%x=0x%x (org=0x%x)", (baseAddr + pInstanceData[k].pinOffset),
            pInstanceData[k].pinSettings, rdRegVal);
    }
    munmap_device_memory((void *)baseAddr, 0x1000);

#if 0
    /* TODO */
    /* GESI PHY connected to Main CPSW2G MDIO */
    boardStatus = Board_control(BOARD_CTRL_CMD_SET_GESI_CPSW_MDIO_MUX, NULL);
    EnetIf_assert(boardStatus == BOARD_SOK);
#endif
}

#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void EnetIf_enableClkOut(Enet_Type enetType,
                         uint32_t instId,
                         EnetIf_ClkOutFreqType clkOut)
{
    EnetIf_MmrLockState prevLockState;
    uint32_t clkVal;

    if (ENETIF_CLKOUT_FREQ_50MHZ == clkOut)
    {
        clkVal = 0U;
    }
    else
    {
        /* 25 MHz */
        clkVal = 1U;
    }

    switch (enetType)
    {
        case ENET_CPSW_2G:
        {
#if defined (CPSW2G)
            CSL_mcu_ctrl_mmr_cfg0Regs *mcuRegs;
            mcuRegs = (CSL_mcu_ctrl_mmr_cfg0Regs *)(uintptr_t)CSL_MCU_CTRL_MMR0_CFG0_BASE;

            prevLockState = EnetIf_mcuMmrCtrl(ENETIF_MMR_LOCK1, ENETIF_UNLOCK_MMR);
            CSL_REG32_FINS(&mcuRegs->MCU_CLKOUT0_CTRL,
                           MCU_CTRL_MMR_CFG0_MCU_CLKOUT0_CTRL_CLK_SEL,
                           clkVal);
            CSL_REG32_FINS(&mcuRegs->MCU_CLKOUT0_CTRL,
                           MCU_CTRL_MMR_CFG0_MCU_CLKOUT0_CTRL_CLK_EN,
                           1U);
            if (prevLockState == ENETIF_LOCK_MMR)
            {
                EnetIf_mcuMmrCtrl(ENETIF_MMR_LOCK1, ENETIF_LOCK_MMR);
            }
#elif defined (CPSW2G_MAIN)
            CSL_main_ctrl_mmr_cfg0Regs *mainRegs;
            mainRegs = (CSL_main_ctrl_mmr_cfg0Regs *)(uintptr_t)CSL_CTRL_MMR0_CFG0_BASE;

            prevLockState = EnetIf_mainMmrCtrl(ENETIF_MMR_LOCK1, ENETIF_UNLOCK_MMR);
            CSL_REG32_FINS(&mainRegs->CLKOUT_CTRL,
                           MAIN_CTRL_MMR_CFG0_CLKOUT_CTRL_CLK_SEL,
                           clkVal);
            CSL_REG32_FINS(&mainRegs->CLKOUT_CTRL,
                           MAIN_CTRL_MMR_CFG0_CLKOUT_CTRL_CLK_EN,
                           1U);
            if (prevLockState == ENETIF_LOCK_MMR)
            {
                EnetIf_mainMmrCtrl(ENETIF_MMR_LOCK1, ENETIF_LOCK_MMR);
            }
#endif
        }
        break;

#if defined (SOC_J721E) || defined (SOC_J7200) || defined(SOC_J784S4)
        case ENET_CPSW_5G:
        case ENET_CPSW_9G:
        {
            CSL_main_ctrl_mmr_cfg0Regs *mainRegs;
            mainRegs = (CSL_main_ctrl_mmr_cfg0Regs *)(uintptr_t)CSL_CTRL_MMR0_CFG0_BASE;

            prevLockState = EnetIf_mainMmrCtrl(ENETIF_MMR_LOCK1, ENETIF_UNLOCK_MMR);
            CSL_REG32_FINS(&mainRegs->CLKOUT_CTRL,
                           MAIN_CTRL_MMR_CFG0_CLKOUT_CTRL_CLK_SEL,
                           clkVal);
            CSL_REG32_FINS(&mainRegs->CLKOUT_CTRL,
                           MAIN_CTRL_MMR_CFG0_CLKOUT_CTRL_CLK_EN,
                           1U);
            if (prevLockState == ENETIF_LOCK_MMR)
            {
                EnetIf_mainMmrCtrl(ENETIF_MMR_LOCK1, ENETIF_LOCK_MMR);
            }
        }
        break;
#endif
        default:
            /* Assert if case new mac mode gets added - ex. for SGMII clock config */
            EnetIf_assert(false);
            break;
    }

}

#if defined (CPSW2G_MAIN)

/* NOTE: copied from pmlib_clkrate.c */
#define PM_SUCCESS                                              (0)
/**< Error Code for SUCCESS. */
#define PM_FAIL                                                 (-((int32_t)1))
/**< Error Code for FAIL. */
#define PM_BADARGS                                              (-((int32_t)2))
/**< Error Code for BADARGS. */
#define PM_TIMEOUT                                              (-((int32_t)3))

static int32_t PMLIBClkRateGet(uint32_t modId,
                               uint32_t clkId,
                               uint64_t *clkRate)
{
    int32_t status   = PM_SUCCESS;
    int32_t retval   = PM_SUCCESS;
    
    retval = Sciclient_pmGetModuleClkFreq(  modId,
                                            clkId,
                                            clkRate,
                                            SCICLIENT_SERVICE_WAIT_FOREVER);

    if (retval == CSL_ETIMEOUT)
    {
        status = PM_TIMEOUT;
    }
    else if (retval == CSL_EFAIL)
    {
        status = PM_FAIL;
    }
    else
    {
        /* Nothing to be done. return success. */
    }

    return status;
}

static int32_t PMLIBClkRateSet(uint32_t modId,
                               uint32_t clkId,
                               uint64_t clkRate)
{
    uint32_t i = 0U;
    int32_t status   = PM_FAIL;
    int32_t finalStatus = PM_FAIL;
    uint64_t respClkRate = 0;
    uint32_t numParents = 0U;
    uint32_t moduleClockParentChanged = 0U;
    uint32_t clockStatus = 0U;
    uint32_t origParent = 0U;
    uint32_t foundParent = 0U;

    /* Check if the clock is enabled or not */
    status = Sciclient_pmModuleGetClkStatus(modId,
                                            clkId,
                                            &clockStatus,
                                            SCICLIENT_SERVICE_WAIT_FOREVER);
    if (status == CSL_PASS)
    {
        /* Get the number of parents for the clock */
        status = Sciclient_pmGetModuleClkNumParent(modId,
                                                clkId,
                                                &numParents,
                                                SCICLIENT_SERVICE_WAIT_FOREVER);
        if ((status == CSL_PASS) && (numParents > 1U))
        {
            status = Sciclient_pmGetModuleClkParent(modId, clkId, &origParent,
                                       SCICLIENT_SERVICE_WAIT_FOREVER);
        }
    }
    if (status == CSL_PASS)
    {
        /* Disabling the clock */
        status = Sciclient_pmModuleClkRequest(
                                            modId,
                                            clkId,
                                            TISCI_MSG_VALUE_CLOCK_SW_STATE_UNREQ,
                                            0U,
                                            SCICLIENT_SERVICE_WAIT_FOREVER);
    }
    if (status == CSL_PASS)
    {
        foundParent = 0U;
        /* Try to loop and change parents of the clock */
        for(i=0U;i<numParents;i++)
        {
            if (numParents > 1U)
            {
                /* Setting the new parent */
                status = Sciclient_pmSetModuleClkParent(
                                            modId,
                                            clkId,
                                            clkId+i+1U,
                                            SCICLIENT_SERVICE_WAIT_FOREVER);
                /* Check if the clock can be set to desirable freq. */
                if (status == CSL_PASS)
                {
                    moduleClockParentChanged = 1U;
                }
            }
            if (status == CSL_PASS)
            {
                status = Sciclient_pmQueryModuleClkFreq(modId,
                                                        clkId,
                                                        clkRate,
                                                        &respClkRate,
                                                        SCICLIENT_SERVICE_WAIT_FOREVER);
            }
            if ((status == CSL_PASS) && (respClkRate == clkRate))
            {
                foundParent = 1U;
                break;
            }
        }
    }
    if ((status == CSL_PASS) && (numParents == 0U))
    {
        status = Sciclient_pmQueryModuleClkFreq(modId,
                                                clkId,
                                                clkRate,
                                                &respClkRate,
                                                SCICLIENT_SERVICE_WAIT_FOREVER);
        if ((status == CSL_PASS) && (respClkRate == clkRate))
        {
            foundParent = 1U;
        }
    }
    if (foundParent == 1U)
    {
        /* Set the clock at the desirable frequency*/
        status = Sciclient_pmSetModuleClkFreq(
                                modId,
                                clkId,
                                clkRate,
                                TISCI_MSG_FLAG_CLOCK_ALLOW_FREQ_CHANGE,
                                SCICLIENT_SERVICE_WAIT_FOREVER);
    }
    else
    {
        status = CSL_EFAIL;
    }
    if ((status == CSL_PASS) &&
        (clockStatus == (uint32_t) TISCI_MSG_VALUE_CLOCK_SW_STATE_UNREQ))
    {
        /* Restore the clock again to original state */
        status = Sciclient_pmModuleClkRequest(
                                            modId,
                                            clkId,
                                            clockStatus,
                                            0U,
                                            SCICLIENT_SERVICE_WAIT_FOREVER);
    }
    finalStatus = status;
    if ((status != CSL_PASS) && (moduleClockParentChanged == 1U))
    {
        /* Setting the original parent if failure */
        (void) Sciclient_pmSetModuleClkParent(
                                    modId,
                                    clkId,
                                    origParent,
                                    SCICLIENT_SERVICE_WAIT_FOREVER);
    }
    return finalStatus;
}

static void EnetIf_setDeviceState(uint32_t moduleId,
                                  uint32_t requiredState,
                                  uint32_t appFlags)
{
    int32_t status;
    uint32_t moduleState      = 0U;
    uint32_t resetState       = 0U;
    uint32_t contextLossState = 0U;
    bool turnOn;
    bool turnOff;

    /* Get the module state */
    status = Sciclient_pmGetModuleState(moduleId,
                                        &moduleState,
                                        &resetState,
                                        &contextLossState,
                                        SCICLIENT_SERVICE_WAIT_FOREVER);
    assert(status == CSL_PASS);

    /* No need to change if already in the requested state. Note that
     * getter (HW_STATE_*) and setter (SW_STATE_*) can't be directly compared */
    turnOn = (moduleState == TISCI_MSG_VALUE_DEVICE_HW_STATE_OFF) &&
             (requiredState == TISCI_MSG_VALUE_DEVICE_SW_STATE_ON);
    turnOff = (moduleState == TISCI_MSG_VALUE_DEVICE_HW_STATE_ON) &&
              (requiredState == TISCI_MSG_VALUE_DEVICE_SW_STATE_AUTO_OFF);

    /* We assert in this function upon failure as this is first function called
     * from the app to enable clocks and app can't recover from this failure */
    if (turnOn || turnOff)
    {
        status = Sciclient_pmSetModuleState(moduleId,
                                            requiredState,
                                            (appFlags |
                                             TISCI_MSG_FLAG_AOP |
                                             TISCI_MSG_FLAG_DEVICE_RESET_ISO),
                                            SCICLIENT_SERVICE_WAIT_FOREVER);
        assert(status == CSL_PASS);

        if (requiredState == TISCI_MSG_VALUE_DEVICE_SW_STATE_ON)
        {
            /* Reset if changed state to enabled */
            status = Sciclient_pmSetModuleRst(moduleId,
                                              0x0U /*resetBit*/,
                                              SCICLIENT_SERVICE_WAIT_FOREVER);
            assert(status == CSL_PASS);
        }
    }

    /* Dummy assignment to avoid -Werror=unused-but-set-variable error. The GCC compiler
     * doesn't consider usage of variable in assert */
    status = status;
}

static void EnetIf_clkRateSetState(uint32_t moduleId,
                                   uint32_t clockId,
                                   uint32_t additionalFlag,
                                   uint32_t state)
{
    int32_t status;

    status = Sciclient_pmModuleClkRequest
                 (moduleId,
                 clockId,
                 state,
                 additionalFlag,
                 SCICLIENT_SERVICE_WAIT_FOREVER);
    if (status != CSL_PASS)
    {
        EnetIf_print("Sciclient_pmModuleClkRequest failed for module=%d, clockId=%d\n",
                           moduleId, clockId);
        EnetIf_assert(false);
    }
}

static void EnetIf_clkRateSet(uint32_t moduleId,
                              uint32_t clkId,
                              uint64_t clkRateHz)
{
    int32_t status;
    uint64_t currClkFreqHz;

    status = PMLIBClkRateGet(moduleId, clkId, &currClkFreqHz);
    if ((status == CSL_PASS) &&
        (currClkFreqHz != clkRateHz))
    {
        status = PMLIBClkRateSet(moduleId, clkId, clkRateHz);
        if (status != CSL_PASS)
        {
            EnetIf_print("PMLIBClkRateSet failed for clock Id = %d\n", clkId);
            EnetIf_assert(false);
        }
    }
}

#define CPSW_SOC_RGMII_MHZ_250_CLK_VAL        (250000000U)
#define CPSW_SOC_RGMII_MHZ_50_CLK_VAL         (50000000U)
#define CPSW_SOC_RGMII_MHZ_5_CLK_VAL          (5000000U)

void EnetIf_enableClocks(Enet_Type enetType,
                         uint32_t instId)
{
    uint32_t moduleId = 0U;
    uint32_t appFlags = 0U;
    uint64_t cppiClkFreqHz;
    uint32_t cppiClkId;
    uint32_t rgmii250MHzClkId;
    uint32_t rgmii50MHzClkId;
    uint32_t rgmii5MHzClkId;

    EnetIf_print("Enabling clocks!\n");

    cppiClkFreqHz = EnetSoc_getClkFreq(enetType, instId, CPSW_CPPI_CLK);

    switch (enetType)
    {
#if defined (SOC_J721S2) || defined(SOC_J784S4)
        case ENET_CPSW_2G:
        {
            if (instId == 0)
            {
                moduleId = TISCI_DEV_MCU_CPSW0;
                cppiClkId        = TISCI_DEV_MCU_CPSW0_CPPI_CLK_CLK,
                rgmii250MHzClkId = TISCI_DEV_MCU_CPSW0_RGMII_MHZ_250_CLK,
                rgmii50MHzClkId  = TISCI_DEV_MCU_CPSW0_RGMII_MHZ_50_CLK,
                rgmii5MHzClkId   = TISCI_DEV_MCU_CPSW0_RGMII_MHZ_5_CLK,
                EnetIf_assert(cppiClkFreqHz != 0U);
                EnetIf_clkRateSet(moduleId, cppiClkId       , cppiClkFreqHz);
                EnetIf_clkRateSet(moduleId, rgmii250MHzClkId, CPSW_SOC_RGMII_MHZ_250_CLK_VAL);
                EnetIf_clkRateSet(moduleId, rgmii50MHzClkId , CPSW_SOC_RGMII_MHZ_50_CLK_VAL);
                EnetIf_clkRateSet(moduleId, rgmii5MHzClkId  , CPSW_SOC_RGMII_MHZ_5_CLK_VAL);
            }
            else if (instId == 1)
            {
                moduleId = TISCI_DEV_CPSW1;
                /* mcu2_0 as a owner of CPSW9G/CPSW5G needs exclusive access, preventing clock
                 * changes by any other cores */
                appFlags = TISCI_MSG_FLAG_DEVICE_EXCLUSIVE;
                EnetIf_assert(cppiClkFreqHz != 0U);
                EnetIf_clkRateSet(moduleId, TISCI_DEV_CPSW1_CPPI_CLK_CLK, cppiClkFreqHz);
                EnetIf_clkRateSet(moduleId, TISCI_DEV_CPSW1_RGMII_MHZ_250_CLK, CPSW_SOC_RGMII_MHZ_250_CLK_VAL);
                EnetIf_clkRateSet(moduleId, TISCI_DEV_CPSW1_RGMII_MHZ_50_CLK, CPSW_SOC_RGMII_MHZ_50_CLK_VAL);
                EnetIf_clkRateSet(moduleId, TISCI_DEV_CPSW1_RGMII_MHZ_5_CLK, CPSW_SOC_RGMII_MHZ_5_CLK_VAL);
            }
            else
            {
                EnetIf_assert(false);
            }
        }
        break;
#else
        case ENET_CPSW_2G:
        {
            moduleId = TISCI_DEV_MCU_CPSW0;
            cppiClkId        = TISCI_DEV_MCU_CPSW0_CPPI_CLK_CLK,
            rgmii250MHzClkId = TISCI_DEV_MCU_CPSW0_RGMII_MHZ_250_CLK,
            rgmii50MHzClkId  = TISCI_DEV_MCU_CPSW0_RGMII_MHZ_50_CLK,
            rgmii5MHzClkId   = TISCI_DEV_MCU_CPSW0_RGMII_MHZ_5_CLK,
            EnetIf_assert(cppiClkFreqHz != 0U);
            EnetIf_clkRateSet(moduleId, cppiClkId       , cppiClkFreqHz);
            EnetIf_clkRateSet(moduleId, rgmii250MHzClkId, CPSW_SOC_RGMII_MHZ_250_CLK_VAL);
            EnetIf_clkRateSet(moduleId, rgmii50MHzClkId , CPSW_SOC_RGMII_MHZ_50_CLK_VAL);
            EnetIf_clkRateSet(moduleId, rgmii5MHzClkId  , CPSW_SOC_RGMII_MHZ_5_CLK_VAL);
        }
        break;
#endif
        default:
            EnetIf_assert(false);
    }

    EnetIf_setDeviceState(moduleId, TISCI_MSG_VALUE_DEVICE_SW_STATE_ON, appFlags);
}

void EnetIf_disableClocks(Enet_Type enetType,
                          uint32_t instId)
{
    uint32_t moduleId = 0U;
    uint32_t appFlags = 0U;
    uint32_t cppiClkId;
    uint32_t rgmii250MHzClkId;
    uint32_t rgmii50MHzClkId;
    uint32_t rgmii5MHzClkId;

    EnetIf_print("Disabling clocks for CPSW!\n");

    switch (enetType)
    {
#if defined (SOC_J721S2) || defined(SOC_J784S4)
        case ENET_CPSW_2G:
        {
            if (instId == 0)
            {
                moduleId = TISCI_DEV_MCU_CPSW0;
                cppiClkId        = TISCI_DEV_MCU_CPSW0_CPPI_CLK_CLK,
                rgmii250MHzClkId = TISCI_DEV_MCU_CPSW0_RGMII_MHZ_250_CLK,
                rgmii50MHzClkId  = TISCI_DEV_MCU_CPSW0_RGMII_MHZ_50_CLK,
                rgmii5MHzClkId   = TISCI_DEV_MCU_CPSW0_RGMII_MHZ_5_CLK,
                /* set clock set to auto so when module is disabled, clocks shuts off */
                EnetIf_clkRateSetState(moduleId,
                                             cppiClkId,
                                             0,
                                             TISCI_MSG_VALUE_CLOCK_SW_STATE_AUTO);
                EnetIf_clkRateSetState(moduleId,
                                             rgmii250MHzClkId,
                                             0,
                                             TISCI_MSG_VALUE_CLOCK_SW_STATE_AUTO);
                EnetIf_clkRateSetState(moduleId,
                                             rgmii50MHzClkId,
                                             0,
                                             TISCI_MSG_VALUE_CLOCK_SW_STATE_AUTO);
                EnetIf_clkRateSetState(moduleId,
                                             rgmii5MHzClkId,
                                             0,
                                             TISCI_MSG_VALUE_CLOCK_SW_STATE_AUTO);
            }
            else if (instId == 1)
            {
                moduleId = TISCI_DEV_CPSW1;
                /* mcu2_0 as a owner of CPSW9G/CPSW5G needs exclusive access, preventing clock
                 * changes by any other cores */
                appFlags = TISCI_MSG_FLAG_DEVICE_EXCLUSIVE;
                /* set clock set to auto so when module is disabled, clocks shuts off */
                EnetIf_clkRateSetState(moduleId,
                                             TISCI_DEV_CPSW1_CPPI_CLK_CLK,
                                             0,
                                             TISCI_MSG_VALUE_CLOCK_SW_STATE_AUTO);
                EnetIf_clkRateSetState(moduleId,
                                             TISCI_DEV_CPSW1_RGMII_MHZ_250_CLK,
                                             0,
                                             TISCI_MSG_VALUE_CLOCK_SW_STATE_AUTO);
                EnetIf_clkRateSetState(moduleId,
                                             TISCI_DEV_CPSW1_RGMII_MHZ_50_CLK,
                                             0,
                                             TISCI_MSG_VALUE_CLOCK_SW_STATE_AUTO);
                EnetIf_clkRateSetState(moduleId,
                                             TISCI_DEV_CPSW1_RGMII_MHZ_5_CLK,
                                             0,
                                             TISCI_MSG_VALUE_CLOCK_SW_STATE_AUTO);
            }
            else
            {
                EnetIf_assert(false);
            }
        }
        break;
#else
        case ENET_CPSW_2G:
        {
            moduleId = TISCI_DEV_MCU_CPSW0;
            cppiClkId        = TISCI_DEV_MCU_CPSW0_CPPI_CLK_CLK,
            rgmii250MHzClkId = TISCI_DEV_MCU_CPSW0_RGMII_MHZ_250_CLK,
            rgmii50MHzClkId  = TISCI_DEV_MCU_CPSW0_RGMII_MHZ_50_CLK,
            rgmii5MHzClkId   = TISCI_DEV_MCU_CPSW0_RGMII_MHZ_5_CLK,
            /* set clock set to auto so when module is disabled, clocks shuts off */
            EnetIf_clkRateSetState(moduleId,
                                         cppiClkId,
                                         0,
                                         TISCI_MSG_VALUE_CLOCK_SW_STATE_AUTO);
            EnetIf_clkRateSetState(moduleId,
                                         rgmii250MHzClkId,
                                         0,
                                         TISCI_MSG_VALUE_CLOCK_SW_STATE_AUTO);
            EnetIf_clkRateSetState(moduleId,
                                         rgmii50MHzClkId,
                                         0,
                                         TISCI_MSG_VALUE_CLOCK_SW_STATE_AUTO);
            EnetIf_clkRateSetState(moduleId,
                                         rgmii5MHzClkId,
                                         0,
                                         TISCI_MSG_VALUE_CLOCK_SW_STATE_AUTO);
        }
        break;
#endif
        default:
            EnetIf_assert(false);
    }

    /* Set module set to HW AUTO */
    EnetIf_setDeviceState(moduleId, TISCI_MSG_VALUE_DEVICE_SW_STATE_AUTO_OFF, appFlags);
}
#endif
