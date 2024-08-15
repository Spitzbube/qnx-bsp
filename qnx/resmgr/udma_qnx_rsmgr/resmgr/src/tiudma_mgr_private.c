/*
 * $QNXLicenseC:
 * Copyright 2019, QNX Software Systems.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as
 * contributors under the License or as licensors under other terms.
 * Please review this entire file for other proprietary rights or license
 * notices, as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

/*
 * Modfications copyright (c) 2019-2022, Texas Instruments Incorporated
 *
 */

#include "psdkqnx_proto.h"
#include "tiudma_mgr_private.h"
#include <ti/drv/sciclient/sciclient.h>
#include "ti/drv/udma/include/udma_ch.h"
#include "ti/drv/udma/src/udma_priv.h"

extern struct Udma_DrvObj      gUdmaDrvObj[UDMA_INST_ID_MAX + 1];

int8_t gUdmaHasBeenInit = FALSE;

int32_t Udma_resmgr_open(Udma_DrvHandle drvHandle) {
    QNX_PR_ERR("%s ERROR: Must not come here", __func__);
    return UDMA_EFAIL;
}

int32_t Udma_resmgr_close(Udma_DrvHandle drvHandle) {
    QNX_PR_ERR("%s ERROR: Must not come here", __func__);
    return UDMA_EFAIL;
}

/* Called from udma-lld function Udma_printf */
void Udma_resmgr_print(const char *str)
{
    slogf(_SLOG_SETCODE(_SLOGC_PRIVATE_START, PSDKQA_SLOGC_TI_UDMA_RM), _SLOG_INFO, str);
}

uint32_t Udma_resmgr_rmAllocBlkCopyCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle) {
    return Udma_rmAllocBlkCopyCh(preferredChNum, drvHandle);
}

void Udma_resmgr_rmFreeBlkCopyCh(uint32_t chNum, Udma_DrvHandle drvHandle) {
    Udma_rmFreeBlkCopyCh(chNum, drvHandle);
}

uint32_t Udma_resmgr_rmAllocBlkCopyHcCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle)  {
    return Udma_rmAllocBlkCopyHcCh(preferredChNum, drvHandle);
}

void Udma_resmgr_rmFreeBlkCopyHcCh(uint32_t chNum, Udma_DrvHandle drvHandle) {
    Udma_rmFreeBlkCopyHcCh(chNum, drvHandle);
}

uint32_t Udma_resmgr_rmAllocBlkCopyUhcCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle) {
    return Udma_rmAllocBlkCopyUhcCh(preferredChNum, drvHandle);
}

void Udma_resmgr_rmFreeBlkCopyUhcCh(uint32_t chNum, Udma_DrvHandle drvHandle) {
    Udma_rmFreeBlkCopyUhcCh(chNum, drvHandle);
}

uint32_t Udma_resmgr_rmAllocTxCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle) {
    return Udma_rmAllocTxCh(preferredChNum, drvHandle);
}

void Udma_resmgr_rmFreeTxCh(uint32_t chNum, Udma_DrvHandle drvHandle) {
    Udma_rmFreeTxCh(chNum, drvHandle);
}

uint32_t Udma_resmgr_rmAllocRxCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle) {
    return Udma_rmAllocRxCh(preferredChNum, drvHandle);
}

void Udma_resmgr_rmFreeRxCh(uint32_t chNum, Udma_DrvHandle drvHandle) {
    Udma_rmFreeRxCh(chNum, drvHandle);
}

uint32_t Udma_resmgr_rmAllocTxHcCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle) {
    return Udma_rmAllocTxHcCh(preferredChNum, drvHandle);
}

void Udma_resmgr_rmFreeTxHcCh(uint32_t chNum, Udma_DrvHandle drvHandle) {
    Udma_rmFreeTxHcCh(chNum, drvHandle);
}

uint32_t Udma_resmgr_rmAllocRxHcCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle) {
    return Udma_rmAllocRxHcCh(preferredChNum, drvHandle);
}

void Udma_resmgr_rmFreeRxHcCh(uint32_t chNum, Udma_DrvHandle drvHandle) {
    Udma_rmFreeRxHcCh(chNum, drvHandle);
}

uint32_t Udma_resmgr_rmAllocTxUhcCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle) {
    return Udma_rmAllocTxUhcCh(preferredChNum, drvHandle);
}

void Udma_resmgr_rmFreeTxUhcCh(uint32_t chNum, Udma_DrvHandle drvHandle) {
    Udma_rmFreeTxUhcCh(chNum, drvHandle);
}

uint32_t Udma_resmgr_rmAllocRxUhcCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle) {
    return Udma_rmAllocRxUhcCh(preferredChNum, drvHandle);
}

void Udma_resmgr_rmFreeRxUhcCh(uint32_t chNum, Udma_DrvHandle drvHandle) {
    Udma_rmFreeRxUhcCh(chNum, drvHandle);
}

#if (UDMA_NUM_MAPPED_TX_GROUP > 0)
uint32_t Udma_resmgr_rmAllocMappedTxCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle, const uint32_t mappedChGrp) {
    return Udma_rmAllocMappedTxCh(preferredChNum, drvHandle, mappedChGrp);
}

uint32_t Udma_resmgr_rmFreeMappedTxCh(uint32_t txChNum, Udma_DrvHandle drvHandle, const uint32_t mappedChGrp)
{
    Udma_rmFreeMappedTxCh(txChNum, drvHandle, mappedChGrp);
    return UDMA_SOK;
}
#endif

#if (UDMA_NUM_MAPPED_RX_GROUP > 0)
uint32_t Udma_resmgr_rmAllocMappedRxCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle, const uint32_t mappedChGrp) {
    return Udma_rmAllocMappedRxCh(preferredChNum, drvHandle, mappedChGrp);
}

uint32_t Udma_resmgr_rmFreeMappedRxCh(uint32_t rxChNum, Udma_DrvHandle drvHandle, const uint32_t mappedChGrp)
{
    Udma_rmFreeMappedRxCh(rxChNum, drvHandle, mappedChGrp - UDMA_NUM_MAPPED_TX_GROUP);
    return UDMA_SOK;
}
#endif

#if (UDMA_NUM_UTC_INSTANCE > 0)
uint32_t Udma_resmgr_rmAllocExtCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle, const Udma_UtcInstInfo *utcInfo) {
    return Udma_rmAllocExtCh(preferredChNum, drvHandle, utcInfo);
}

void Udma_resmgr_rmFreeExtCh(uint32_t chNum, Udma_DrvHandle drvHandle, const Udma_UtcInstInfo *utcInfo) {
    Udma_rmFreeExtCh(chNum, drvHandle, utcInfo);
}
#endif

uint16_t Udma_resmgr_rmAllocProxy(uint16_t preferredProxyNum, Udma_DrvHandle drvHandle) {
    return Resmgr_Udma_rmAllocProxy(preferredProxyNum, drvHandle);
}

void Udma_resmgr_rmFreeProxy(uint16_t proxyNum, Udma_DrvHandle drvHandle) {
    Udma_rmFreeProxy(proxyNum, drvHandle);
}

uint16_t Udma_resmgr_rmAllocFreeRing(Udma_DrvHandle drvHandle) {
    return Udma_rmAllocFreeRing(drvHandle);
}

void Udma_resmgr_rmFreeFreeRing(uint16_t ringNum, Udma_DrvHandle drvHandle) {
    Udma_rmFreeFreeRing(ringNum, drvHandle);
}

#if (UDMA_SOC_CFG_PKTDMA_PRESENT == 1)
uint32_t Udma_resmgr_rmAllocMappedRing(Udma_DrvHandle drvHandle, uint32_t mappdRingGrp, uint32_t mappedChNum) {
    return Udma_rmAllocMappedRing(drvHandle, mappdRingGrp, mappedChNum);
}

void Udma_resmgr_rmFreeMappedRing(uint32_t ringNum, Udma_DrvHandle drvHandle, uint32_t mappdRingGrp, uint32_t mappedChNum) {
    Udma_rmFreeMappedRing(ringNum, drvHandle, mappdRingGrp, mappedChNum);
}
#endif

uint16_t Udma_resmgr_rmAllocRingMon(Udma_DrvHandle drvHandle) {
    return Udma_rmAllocRingMon(drvHandle);
}

void Udma_resmgr_rmFreeRingMon(uint16_t ringNum, Udma_DrvHandle drvHandle) {
    Udma_rmFreeRingMon(ringNum,  drvHandle);
}

uint32_t Udma_resmgr_rmAllocVintr(Udma_DrvHandle drvHandle) {
    return Udma_rmAllocVintr(drvHandle);
}

void Udma_resmgr_rmFreeVintr(uint32_t vintrNum, Udma_DrvHandle drvHandle) {
    Udma_rmFreeVintr(vintrNum, drvHandle);
}

uint32_t Udma_resmgr_rmAllocIrIntr(uint32_t preferredIrIntrNum, Udma_DrvHandle drvHandle) {
    return Udma_rmAllocIrIntr(preferredIrIntrNum, drvHandle);
}

void Udma_resmgr_rmFreeIrIntr(uint32_t irIntrNum, Udma_DrvHandle drvHandle) {
    Udma_rmFreeIrIntr(irIntrNum, drvHandle);
}

uint32_t Udma_resmgr_rmAllocEvent(Udma_DrvHandle drvHandle) {
    return Udma_rmAllocEvent(drvHandle);
}

void Udma_resmgr_rmFreeEvent(uint32_t globalEvent, Udma_DrvHandle drvHandle) {
    Udma_rmFreeEvent(globalEvent, drvHandle);
}

uint32_t Udma_resmgr_rmAllocVintrBit(Udma_DrvHandle drvHandle, Udma_EventHandle eventHandle) {
    return Udma_rmAllocVintrBit(eventHandle);
}

void Udma_resmgr_rmFreeVintrBit(uint32_t vintrBitNum, Udma_DrvHandle drvHandle, Udma_EventHandle eventHandle) {
    Udma_rmFreeVintrBit(vintrBitNum, drvHandle, eventHandle);
}

uint32_t Udma_resmgr_rmTranslateIrOutput(Udma_DrvHandle drvHandle, uint32_t irIntrNum) {
    return Udma_rmTranslateIrOutput(drvHandle, irIntrNum);
}

uint32_t Udma_resmgr_rmTranslateCoreIntrInput(Udma_DrvHandle drvHandle, uint32_t coreIntrNum) {
    return Udma_rmTranslateCoreIntrInput(drvHandle, coreIntrNum);
}

uint32_t Udma_resmgr_rmAllocflow(uint32_t flowCnt, Udma_DrvHandle drvHandle)
{
#if defined (SOC_J721S2) || defined (SOC_J784S4)
    return Udma_rmAllocflow(flowCnt, drvHandle);
#else
    QNX_PR_ERR("[Error] API not supported for this SOC!!!!");
    return -1;
#endif
}

void  Udma_resmgr_rmFreeflow(uint32_t flowStart, uint32_t flowCnt, Udma_DrvHandle drvHandle)
{
#if defined (SOC_J721S2) || defined (SOC_J784S4)
    Udma_rmFreeflow(flowStart, flowCnt, drvHandle);
#else
    QNX_PR_ERR("[Error] API not supported for this SOC!!!!");
#endif
}

static uint64_t Resmgr_Udma_qnxVirtToPhyFxn(const void *virtAddr,
                                            uint32_t chNum,
                                            void *appData)
{
    int ret;
    off64_t    phyAddr = 0;
    uint32_t   length;

    if(appData != NULL_PTR) {
        length = (uint32_t) *((uint32_t *) appData);
    }
    else {
        QNX_PR_ERR("%s Must specify memory size to map", __func__);
        return -1;
    }

    /* Get destination physical address */
    ret = mem_offset64(virtAddr, NOFD, length, &phyAddr, NULL);
    if (ret) {
        if (errno != EAGAIN) {
            QNX_PR_ERR("%s:Error from mem_offset - errno=%d", __func__, errno);
        }
        else if (phyAddr == 0) {
            QNX_PR_ERR("%s:Error from mem_offset - errno=%d and phyAddr is NULL ", __func__, errno);
        }
    }
    return (uint64_t ) phyAddr;
}

static void *Resmgr_Udma_qnxPhyToVirtFxn(uint64_t phyAddr,
                                         uint32_t chNum,
                                         void *appData)
{
    uint64_t *temp = 0;
    uint32_t length = 0;


    if(appData != NULL_PTR) {
        length = (uint32_t) *((uint32_t *) appData);
    }
    else {
        QNX_PR_ERR("%s Must specify memory size to map", __func__);
        return NULL;
    }

    temp  = mmap_device_memory(0, length, PROT_READ|PROT_WRITE, 0, phyAddr);
    if((temp == MAP_FAILED))
    {
        QNX_PR_ERR("%s: mmmap_device_memory failed",__func__);
    }

    return ((void *) temp);
}

uint16_t Resmgr_Udma_rmAllocProxy(uint16_t preferredProxyNum, Udma_DrvHandle drvHandle)
{
    uint16_t            i, offset, proxyNum = UDMA_PROXY_INVALID, temp;
    uint32_t            bitPos, bitMask;
    Udma_RmInitPrms    *rmInitPrms = &drvHandle->initPrms.rmInitPrms;

    drvHandle->initPrms.osalPrms.lockMutex(drvHandle->rmLock);

    if(UDMA_PROXY_ANY == preferredProxyNum)
    {
        for(i = 0U; i < rmInitPrms->numProxy; i++)
        {
            offset = i >> 5U;
            temp = i - (offset << 5U);
            bitPos = (uint32_t) temp;
            bitMask = (uint32_t) 1U << bitPos;
            if((drvHandle->proxyFlag[offset] & bitMask) == bitMask)
            {
                drvHandle->proxyFlag[offset] &= ~bitMask;
                proxyNum = (uint16_t)rmInitPrms->startProxy;  /* Add start offset */
                proxyNum += i;
                break;
            }
        }
    }
    else
    {
        /* Array bound check */
        if((preferredProxyNum >= rmInitPrms->startProxy) &&
           (preferredProxyNum < (rmInitPrms->startProxy + rmInitPrms->numProxy)))
        {
            i = preferredProxyNum - rmInitPrms->startProxy;
            offset = i >> 5U;
            bitPos = i - (offset << 5U);
            bitMask = (uint32_t) 1U << bitPos;
            if((drvHandle->proxyFlag[offset] & bitMask) == bitMask)
            {
                drvHandle->proxyFlag[offset] &= ~bitMask;
                proxyNum = preferredProxyNum;
            }
        }
    }

    drvHandle->initPrms.osalPrms.unlockMutex(drvHandle->rmLock);

    return (proxyNum);
}

int32_t Resmgr_Udma_setup(void)
{
    int32_t         retVal = UDMA_SOK;

    Udma_DrvHandle  drvHandle;
    Udma_InitPrms   initPrms;
    uint32_t        instId;

#if defined (SOC_J721S2) || defined (SOC_J784S4) || defined (SOC_J721E) || defined (SOC_J7200)
    /* MAIN UDMA Instance */
    instId = UDMA_INST_ID_MAIN_0;
    drvHandle = &gUdmaDrvObj[instId];
    retVal = UdmaInitPrms_init(instId, &initPrms);
    if(UDMA_SOK != retVal)
    {
        QNX_PR_ERR("[Error] UdmaInitPrms_init failed for instance %d", instId);
        return retVal;
    }

    /* Set virtToPhy and PhytoVirt to support QNX */
    initPrms.virtToPhyFxn = &Resmgr_Udma_qnxVirtToPhyFxn;
    initPrms.phyToVirtFxn = &Resmgr_Udma_qnxPhyToVirtFxn;

    /* This is set only for UDMA RM, Client must not set this */
    initPrms.isQnxRMInstance = 1;

    retVal = Udma_init(drvHandle, &initPrms);
    if(UDMA_SOK != retVal)
    {
        QNX_PR_ERR("[Error] Udma_init failed for instance %d!!!", instId);
        return retVal;
    }
#endif

#if defined (SOC_J721S2) || defined (SOC_J784S4) || defined (SOC_J722S)
    if (UDMA_NUM_BCDMA_INST_ID > 0)
    {
        /* BCDMA Instance */
        instId= UDMA_INST_ID_BCDMA_0;
        drvHandle = &gUdmaDrvObj[instId];
        retVal = UdmaInitPrms_init(instId, &initPrms);
        if(UDMA_SOK != retVal)
        {
            QNX_PR_ERR("[Error] UdmaInitPrms_init failed for instance %d", instId);
            return retVal;
        }

        /* Set virtToPhy and PhytoVirt to support QNX */
        initPrms.virtToPhyFxn = &Resmgr_Udma_qnxVirtToPhyFxn;
        initPrms.phyToVirtFxn = &Resmgr_Udma_qnxPhyToVirtFxn;

        /* This is set only for UDMA RM, Client must not set this */
        initPrms.isQnxRMInstance = 1;

        retVal = Udma_init(drvHandle, &initPrms);
        if(UDMA_SOK != retVal)
        {
            QNX_PR_ERR("[Error] Udma_init failed for instance %d!!!", instId);
            return retVal;
        }
    }

#if (UDMA_SOC_CFG_PKTDMA_PRESENT == 1)
        /* PKTDMA Instance */
        instId= UDMA_INST_ID_PKTDMA_0;
        drvHandle = &gUdmaDrvObj[instId];
        retVal = UdmaInitPrms_init(instId, &initPrms);
        if(UDMA_SOK != retVal)
        {
            QNX_PR_ERR("[Error] UdmaInitPrms_init failed for instance %d", instId);
            return retVal;
        }

        /* Set virtToPhy and PhytoVirt to support QNX */
        initPrms.virtToPhyFxn = &Resmgr_Udma_qnxVirtToPhyFxn;
        initPrms.phyToVirtFxn = &Resmgr_Udma_qnxPhyToVirtFxn;

        /* This is set only for UDMA RM, Client must not set this */
        initPrms.isQnxRMInstance = 1;

        retVal = Udma_init(drvHandle, &initPrms);
        if(UDMA_SOK != retVal)
        {
            QNX_PR_ERR("[Error] Udma_init failed for instance %d!!!", instId);
            return retVal;
        }
#endif //UDMA_SOC_CFG_PKTDMA_PRESENT

#endif //(SOC_J721S2) || defined (SOC_J784S4) || defined (SOC_J722S)

    gUdmaHasBeenInit = TRUE;

    return retVal;
}

int32_t Resmgr_Udma_cleanup(void)
{
    int32_t         retVal = UDMA_SOK;

    if (gUdmaHasBeenInit == TRUE)
    {
        Udma_DrvHandle  drvHandle;
        uint32_t        instId;

#if defined (SOC_J721S2) || defined (SOC_J784S4) || defined (SOC_J721E) || defined (SOC_J7200)
        /* MAIN UDMA Instance */
        instId = UDMA_INST_ID_MAIN_0;
        drvHandle = &gUdmaDrvObj[instId];
        retVal = Udma_deinit(drvHandle);
        if (retVal != UDMA_SOK)
        {
            QNX_PR_ERR("[Error] Udma_deinit failed for instance %d!!!", instId);
        }
#endif

#if defined (SOC_J721S2) || defined (SOC_J784S4) || defined (SOC_J722S)
        if (UDMA_NUM_BCDMA_INST_ID > 0)
        {
            /* BCDMA Instance */
            instId = UDMA_INST_ID_BCDMA_0;
            drvHandle = &gUdmaDrvObj[instId];
            retVal = Udma_deinit(drvHandle);
            if (retVal != UDMA_SOK)
            {
                QNX_PR_ERR("[Error] Udma_deinit failed for instance %d!!!", instId);
            }
        }
#endif
    }

    return retVal;
}
