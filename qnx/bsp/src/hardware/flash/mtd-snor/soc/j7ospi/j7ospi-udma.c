/*
 * $QNXLicenseC:
 * Copyright 2023, QNX Software Systems.
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


#include "j7ospi.h"

#ifdef  J7OSPI_UDMA_SUPPORT

#include "ti/drv/udma/udma.h"

#include <sys/slog.h>
#include <sys/slogcodes.h>


//#define OSPI_UDMA_INTR

/*
 * Ring parameters
 */
/** \brief Number of ring entries - we can prime this much memcpy operations */
#define OSPI_UDMA_RING_ENTRIES      (1U)
/** \brief Size (in bytes) of each ring entry (Size of pointer - 64-bit) */
#define OSPI_UDMA_RING_ENTRY_SIZE   (sizeof(uint64_t))
/** \brief Total ring memory */
#define OSPI_UDMA_RING_MEM_SIZE     (OSPI_UDMA_RING_ENTRIES * OSPI_UDMA_RING_ENTRY_SIZE)
/** \brief This ensures every channel memory is aligned */
#define OSPI_UDMA_RING_MEM_SIZE_ALIGN   ((OSPI_UDMA_RING_MEM_SIZE + UDMA_CACHELINE_ALIGNMENT) & ~(UDMA_CACHELINE_ALIGNMENT - 1U))
/**
 *  \brief UDMA TR packet descriptor memory.
 *  This contains the CSL_UdmapCppi5TRPD + Padding to sizeof(CSL_UdmapTR15) +
 *  one Type_15 TR (CSL_UdmapTR15) + one TR response of 4 bytes.
 *  Since CSL_UdmapCppi5TRPD is less than CSL_UdmapTR15, size is just two times
 *  CSL_UdmapTR15 for alignment.
 */
#define OSPI_UDMA_TRPD_SIZE         ((sizeof(CSL_UdmapTR15) * 2U) + 4U)
/** \brief This ensures every channel memory is aligned */
#define OSPI_UDMA_TRPD_SIZE_ALIGN   ((OSPI_UDMA_TRPD_SIZE + UDMA_CACHELINE_ALIGNMENT) & ~(UDMA_CACHELINE_ALIGNMENT - 1U))

static struct Udma_DrvObj       gUdmaDrvObj;
static struct Udma_ChObj        gUdmaChObj;
static int32_t                  gUdmaResult;
#ifdef  OSPI_UDMA_INTR
static struct Udma_EventObj     gUdmaCqEventObj;
static struct Udma_EventObj     gUdmaTdCqEventObj;
static SemaphoreP_Handle        gUdmaDoneSem = NULL;
#endif

static uint8_t *gTxRingMem;
static uint8_t *gTxCompRingMem;
static uint8_t *gTxTdCompRingMem;
static uint8_t *gUdmaTrpdMem;
static paddr_t pTxRingMem;
static paddr_t pTxCompRingMem;
static paddr_t pTxTdCompRingMem;
static paddr_t pUdmaTrpdMem;

static uint8_t dma_init = 0;

static int get_cache_flag(void)
{
    struct    cacheattr_entry  const *cache_base;
    struct    cacheattr_entry  const *cache;
    uint32_t  cache_idx;
    int       cache_flg = 0;

    cache_base = SYSPAGE_ENTRY(cacheattr);
    for (cache_idx = (uint32_t)SYSPAGE_ENTRY(cpuinfo)->data_cache;
        cache_idx != CACHE_LIST_END; cache_idx = cache->next) {
        cache = &cache_base[cache_idx];
        if (!(cache->flags & CACHE_FLAG_SNOOPED)) {
            cache_flg = PROT_NOCACHE;
            break;
        }
    }
    snor_slogf(_SLOG_INFO, 0, 0, "%s: cache_flg = 0x%x", __func__, cache_flg);
    return cache_flg;
}

static uint64_t Udma_qnxVirtToPhyFxn(const void *virtAddr, uint32_t chNum, void *appData)
{
    int         ret;
    off64_t     phyAddr;
    uint32_t    length;

    if ((uint8_t *)virtAddr >= gTxRingMem &&
                    (uint8_t *)virtAddr < (gTxRingMem + OSPI_UDMA_RING_MEM_SIZE_ALIGN * 3 + OSPI_UDMA_TRPD_SIZE_ALIGN)) {
        return (pTxRingMem + ((paddr_t)virtAddr - (paddr_t)gTxRingMem));
    }

    fprintf(stderr, "%s: WARNING! memory[%p] is not allocated by the driver.\n", __func__, virtAddr);

    if (appData != NULL_PTR) {
        length = (uint32_t)(*((uint32_t *)appData));
    } else {
        fprintf(stderr, "%s: Must specify memory size\n", __func__);
        return (-1);
    }

    /* Get destination physical address */
    ret = mem_offset64(virtAddr, NOFD, length, &phyAddr, NULL);
    if (ret) {
        fprintf(stderr, "%s:Error from mem_offset\n", __func__);
        return (-1);
    }

    return (uint64_t)phyAddr;
}

static void *Udma_qnxPhyToVirtFxn(uint64_t phyAddr, uint32_t chNum, void *appData)
{
    void        *temp;
    uint32_t    length = 0;
    const       int prot = PROT_READ | PROT_WRITE;

    if (phyAddr >= pTxRingMem &&
                    phyAddr < (pTxRingMem + OSPI_UDMA_RING_MEM_SIZE_ALIGN * 3 + OSPI_UDMA_TRPD_SIZE_ALIGN)) {
        return (gTxRingMem + (phyAddr - pTxRingMem));
    }

    fprintf(stderr, "%s: WARNING! memory[%lx] is not allocated by the driver.\n", __func__, phyAddr);

    if (appData != NULL_PTR) {
        length = (uint32_t)(*((uint32_t *)appData));
    } else {
        fprintf(stderr, "%s: Must specify memory size\n", __func__);
        return (NULL);
    }

    temp = mmap_device_memory(NULL, length, prot | get_cache_flag(), 0, phyAddr);
    if ((temp == MAP_FAILED)) {
        fprintf(stderr, "%s: mmmap_device_memory failed\n", __func__);
        return (NULL);
    }

    return (temp);
}

static void j7ospi_udma_print(const char *str)
{
    fprintf(stderr, "%s", str);
}

#ifdef  OSPI_UDMA_INTR
static void j7ospi_udmaEventDmaCb(Udma_EventHandle eventHandle, uint32_t eventType, void *appData)
{
    if (UDMA_EVENT_TYPE_DMA_COMPLETION == eventType) {
        gUdmaResult = UDMA_SOK;
        SemaphoreP_post(gUdmaDoneSem);
    }

    return;
}

static void j7ospi_udmaEventTdCb(Udma_EventHandle eventHandle, uint32_t eventType, void *appData)
{
    CSL_UdmapTdResponse tdResp;

    if (UDMA_EVENT_TYPE_TEARDOWN_PACKET == eventType) {
        /* Response received in Teardown completion queue */
        if (Udma_chDequeueTdResponse(&gUdmaChObj, &tdResp) == UDMA_SOK) {
            gUdmaResult = UDMA_SOK;
        }
    }

    return;
}
#endif

static void OSPI_udmaTrpdInit(paddr_t dst, paddr_t src, size_t length)
{
    CSL_UdmapCppi5TRPD  *pTrpd = (CSL_UdmapCppi5TRPD *)gUdmaTrpdMem;
    CSL_UdmapTR15       *pTr = (CSL_UdmapTR15 *)(gUdmaTrpdMem + sizeof(CSL_UdmapTR15));
    uint32_t            *pTrResp = (uint32_t *)(gUdmaTrpdMem + (sizeof(CSL_UdmapTR15) * 2U));
    uint32_t            cqRingNum = Udma_chGetCqRingNum(&gUdmaChObj);
    uint32_t            descType = CSL_UDMAP_CPPI5_PD_DESCINFO_DTYPE_VAL_TR;

    /* Setup descriptor */
    CSL_udmapCppi5SetDescType(pTrpd, descType);
    CSL_udmapCppi5TrSetReload(pTrpd, 0U, 0U);
    CSL_udmapCppi5SetPktLen(pTrpd, descType, 1U);       /* Only one TR in TRPD */
    CSL_udmapCppi5SetIds(pTrpd, descType, 0U, 0x3FFFU); /* Flow ID and Packet ID */
    CSL_udmapCppi5SetSrcTag(pTrpd, 0x0000);     /* Not used */
    CSL_udmapCppi5SetDstTag(pTrpd, 0x0000);     /* Not used */
    CSL_udmapCppi5TrSetEntryStride(pTrpd, CSL_UDMAP_CPPI5_TRPD_PKTINFO_RECSIZE_VAL_64B);
    CSL_udmapCppi5SetReturnPolicy(pTrpd,
                                  descType,
                                  CSL_UDMAP_CPPI5_PD_PKTINFO2_RETPOLICY_VAL_ENTIRE_PKT,   /* Don't care for TR */
                                  CSL_UDMAP_CPPI5_PD_PKTINFO2_EARLYRET_VAL_NO,
                                  CSL_UDMAP_CPPI5_PD_PKTINFO2_RETPUSHPOLICY_VAL_TO_TAIL,
                                  cqRingNum);

    /* Setup TR */
    pTr->flags    = CSL_FMK(UDMAP_TR_FLAGS_TYPE, (uint32_t)15U)                                 |
                    CSL_FMK(UDMAP_TR_FLAGS_STATIC, (uint32_t)0U)                                |
                    CSL_FMK(UDMAP_TR_FLAGS_EOL, (uint32_t)0U)                                   |   /* NA */
                    CSL_FMK(UDMAP_TR_FLAGS_EVENT_SIZE, CSL_UDMAP_TR_FLAGS_EVENT_SIZE_COMPLETION)|
                    CSL_FMK(UDMAP_TR_FLAGS_TRIGGER0, CSL_UDMAP_TR_FLAGS_TRIGGER_NONE)           |
                    CSL_FMK(UDMAP_TR_FLAGS_TRIGGER0_TYPE, CSL_UDMAP_TR_FLAGS_TRIGGER_TYPE_ALL)  |
                    CSL_FMK(UDMAP_TR_FLAGS_TRIGGER1, CSL_UDMAP_TR_FLAGS_TRIGGER_NONE)           |
                    CSL_FMK(UDMAP_TR_FLAGS_TRIGGER1_TYPE, CSL_UDMAP_TR_FLAGS_TRIGGER_TYPE_ALL)  |
                    CSL_FMK(UDMAP_TR_FLAGS_CMD_ID, (uint32_t)0x25U)                             |   /* This will come back in TR response */
                    CSL_FMK(UDMAP_TR_FLAGS_SA_INDIRECT, (uint32_t)0U)                           |
                    CSL_FMK(UDMAP_TR_FLAGS_DA_INDIRECT, (uint32_t)0U)                           |
                    CSL_FMK(UDMAP_TR_FLAGS_EOP, (uint32_t)1U);
    pTr->icnt0    = length;
    pTr->icnt1    = 1;
    pTr->icnt2    = 1;
    pTr->icnt3    = 1;
    pTr->dim1     = (int32_t)pTr->icnt0;
    pTr->dim2     = (int32_t)pTr->icnt0 * (int32_t)pTr->icnt1;
    pTr->dim3     = (int32_t)pTr->icnt0 * (int32_t)pTr->icnt1 * (int32_t)pTr->icnt2;
    pTr->addr     = (uint64_t)src;
    pTr->fmtflags = 0x00000000U;    /* Linear addressing, 1 byte per elem.
                                       Replace with CSL-FL API */

    pTr->dicnt0   = length;
    pTr->dicnt1   = 1U;
    pTr->dicnt2   = 1U;
    pTr->dicnt3   = 1U;
    pTr->ddim1    = pTr->dicnt0;
    pTr->ddim2    = (pTr->dicnt0 * pTr->dicnt1);
    pTr->ddim3    = (pTr->dicnt0 * pTr->dicnt1 * pTr->dicnt2);
    pTr->daddr    = (uint64_t)dst;

    /* Clear TR response memory */
    *pTrResp      = 0xFFFFFFFFU;
}

int j7ospi_udma_xfer(j7ospi_dev_t *ospi, paddr_t src, paddr_t dst, size_t len)
{
    uint64_t    pDesc = 0;
    uint32_t   *pTrResp, trRespStatus;

    if (len > J7OSPI_DMABUF_SIZE) {
        fprintf(stderr, "%s: Buffer size %ld too big\n", __func__, len);
        return (EINVAL);
    }

    gUdmaResult = UDMA_EFAIL;

    /* Update TR packet descriptor */
    OSPI_udmaTrpdInit(dst, src, len);

    /* Submit TRPD to channel */
    if (Udma_ringQueueRaw(Udma_chGetFqRingHandle(&gUdmaChObj), pUdmaTrpdMem) != UDMA_SOK) {
        fprintf(stderr, "%s: [Error] Channel queue failed!!\n", __func__);
        return (EIO);
    }

#ifdef  OSPI_UDMA_INTR
    /* Wait for return descriptor in completion ring - this marks the transfer completion */
    SemaphoreP_pend(gUdmaDoneSem, SemaphoreP_WAIT_FOREVER);
    /* Response received in completion queue */
    if (Udma_ringDequeueRaw(Udma_chGetCqRingHandle(&gUdmaChObj), &pDesc) != UDMA_SOK) {
        gUdmaResult = UDMA_EFAIL;
    }
#else
    int32_t     lpc = 10000;
    while (lpc--) {
        if (Udma_ringDequeueRaw(Udma_chGetCqRingHandle(&gUdmaChObj), &pDesc) == UDMA_SOK) {
            gUdmaResult = UDMA_SOK;
            break;
        }
        nanospin_ns(100);
    }
#endif

    if (gUdmaResult != UDMA_SOK) {
        fprintf(stderr, "%s: [Error] No descriptor after callback!!\n", __func__);
        return (ETIMEDOUT);
    }

    if (pDesc == pUdmaTrpdMem) {
        pTrResp = (uint32_t *)(gUdmaTrpdMem + (sizeof(CSL_UdmapTR15) * 2U));
        trRespStatus = CSL_FEXT(*pTrResp, UDMAP_TR_RESPONSE_STATUS_TYPE);
        if (trRespStatus == CSL_UDMAP_TR_RESPONSE_STATUS_COMPLETE) {
            return (EOK);
        }
        fprintf(stderr, "%s: [Error] TR Response not completed!!\n", __func__);
    } else {
        fprintf(stderr, "%s: [Error] TR descriptor pointer returned doesn't match the submitted address!!\n", __func__);
    }

    return (EIO);
}

static int j7ospi_alloc_mem(j7ospi_dev_t *ospi)
{
    int    mapf;
    const  int prot = PROT_READ | PROT_WRITE;

    ospi->buflen = J7OSPI_DMABUF_SIZE + OSPI_UDMA_RING_MEM_SIZE_ALIGN * 3 + OSPI_UDMA_TRPD_SIZE_ALIGN;

    mapf = (ospi->tpmfd == -1) ? (MAP_ANON | MAP_PRIVATE | MAP_PHYS) : MAP_SHARED;
    ospi->v_buf = mmap(NULL, ospi->buflen, prot | get_cache_flag(), mapf, ospi->tpmfd, 0);

    if (ospi->v_buf == MAP_FAILED) return (errno);

    mem_offset64(ospi->v_buf, NOFD, J7OSPI_DMABUF_SIZE, &ospi->p_buf, NULL);

    gTxRingMem = (uint8_t *)ospi->v_buf + J7OSPI_DMABUF_SIZE;
    gTxCompRingMem = gTxRingMem + OSPI_UDMA_RING_MEM_SIZE_ALIGN;
    gTxTdCompRingMem = gTxCompRingMem + OSPI_UDMA_RING_MEM_SIZE_ALIGN;
    gUdmaTrpdMem = gTxTdCompRingMem + OSPI_UDMA_RING_MEM_SIZE_ALIGN;

    pTxRingMem = ospi->p_buf + J7OSPI_DMABUF_SIZE;
    pTxCompRingMem = pTxRingMem + OSPI_UDMA_RING_MEM_SIZE_ALIGN;
    pTxTdCompRingMem = pTxCompRingMem + OSPI_UDMA_RING_MEM_SIZE_ALIGN;
    pUdmaTrpdMem = pTxTdCompRingMem + OSPI_UDMA_RING_MEM_SIZE_ALIGN;

    return (EOK);
}

int j7ospi_init_udma(j7ospi_dev_t *ospi)
{
    Udma_InitPrms       initPrms;
    Udma_ChPrms         chPrms;
    Udma_ChTxPrms       txPrms;
    Udma_ChRxPrms       rxPrms;
#ifdef  OSPI_UDMA_INTR
    Udma_EventHandle    eventHandle;
    Udma_EventPrms      eventPrms;
    SemaphoreP_Params   semPrms;
#endif

    if (j7ospi_alloc_mem(ospi) != EOK) return (errno);

    // TI UDMA calls
    snor_slogf(_SLOG_INFO, ospi->ctrl.verbosity, 5, "%s: J7OSPI_UDMA_INST_ID = %d", __func__, J7OSPI_UDMA_INST_ID);
    UdmaInitPrms_init(J7OSPI_UDMA_INST_ID, &initPrms);

    /* Set virtToPhy and PhytoVirt */
    initPrms.virtToPhyFxn = &Udma_qnxVirtToPhyFxn;
    initPrms.phyToVirtFxn = &Udma_qnxPhyToVirtFxn;
    initPrms.printFxn     = &j7ospi_udma_print;

    if (Udma_init(&gUdmaDrvObj, &initPrms) != UDMA_SOK) {
        fprintf(stderr, "%s: [Error] UDMA init failed!!\n", __func__);
        goto fail0;
    }

#ifdef  OSPI_UDMA_INTR
    SemaphoreP_Params_init(&semPrms);
    gUdmaDoneSem = SemaphoreP_create(0, &semPrms);
    if (NULL == gUdmaDoneSem) {
        fprintf(stderr, "%s: [Error] Sem create failed!!\n", __func__);
        goto fail0;
    }
#endif

    UdmaChPrms_init(&chPrms, UDMA_CH_TYPE_TR_BLK_COPY);

    chPrms.fqRingPrms.ringMem       = gTxRingMem;
    chPrms.cqRingPrms.ringMem       = gTxCompRingMem;
    chPrms.tdCqRingPrms.ringMem     = gTxTdCompRingMem;
    chPrms.fqRingPrms.ringMemSize   = OSPI_UDMA_RING_MEM_SIZE;
    chPrms.cqRingPrms.ringMemSize   = OSPI_UDMA_RING_MEM_SIZE;
    chPrms.tdCqRingPrms.ringMemSize = OSPI_UDMA_RING_MEM_SIZE;
    chPrms.fqRingPrms.elemCnt       = OSPI_UDMA_RING_ENTRIES;
    chPrms.cqRingPrms.elemCnt       = OSPI_UDMA_RING_ENTRIES;
    chPrms.tdCqRingPrms.elemCnt     = OSPI_UDMA_RING_ENTRIES;
    if(ospi->ch != -1) {
        chPrms.chNum = ospi->ch;
    }

    if (Udma_chOpen(&gUdmaDrvObj, &gUdmaChObj, UDMA_CH_TYPE_TR_BLK_COPY, &chPrms) != UDMA_SOK) {
        fprintf(stderr, "%s: [Error] UDMA channel open failed!!\n", __func__);
        goto fail1;
    }

    /* Config TX channel */
    UdmaChTxPrms_init(&txPrms, UDMA_CH_TYPE_TR_BLK_COPY);
    if (Udma_chConfigTx(&gUdmaChObj, &txPrms) != UDMA_SOK) {
        fprintf(stderr, "%s: [Error] UDMA TX channel config failed!!\n", __func__);
        goto fail2;
    }

    /* Config RX channel - which is implicitly paired to TX channel in
     * block copy mode */
    UdmaChRxPrms_init(&rxPrms, UDMA_CH_TYPE_TR_BLK_COPY);
    if (Udma_chConfigRx(&gUdmaChObj, &rxPrms) != UDMA_SOK) {
        fprintf(stderr, "%s: [Error] UDMA RX channel config failed!!\n", __func__);
        goto fail2;
    }

#ifdef  OSPI_UDMA_INTR
    eventHandle = &gUdmaCqEventObj;
    UdmaEventPrms_init(&eventPrms);
    eventPrms.eventType         = UDMA_EVENT_TYPE_DMA_COMPLETION;
    eventPrms.eventMode         = UDMA_EVENT_MODE_SHARED;
    eventPrms.chHandle          = &gUdmaChObj;
    eventPrms.masterEventHandle = Udma_eventGetGlobalHandle(&gUdmaDrvObj);
    eventPrms.eventCb           = &j7ospi_udmaEventDmaCb;
    if (Udma_eventRegister(&gUdmaDrvObj, eventHandle, &eventPrms) != UDMA_SOK) {
        fprintf(stderr, "%s: [Error] UDMA CQ event register failed!!\n", __func__);
        goto fail2;
    }

    /* Register teardown ring completion callback */
    eventHandle = &gUdmaTdCqEventObj;
    UdmaEventPrms_init(&eventPrms);
    eventPrms.eventType         = UDMA_EVENT_TYPE_TEARDOWN_PACKET;
    eventPrms.eventMode         = UDMA_EVENT_MODE_SHARED;
    eventPrms.chHandle          = &gUdmaChObj;
    eventPrms.masterEventHandle = Udma_eventGetGlobalHandle(&gUdmaDrvObj);
    eventPrms.eventCb           = &j7ospi_udmaEventTdCb;
    if (Udma_eventRegister(&gUdmaDrvObj, eventHandle, &eventPrms) != UDMA_SOK) {
        fprintf(stderr, "%s: [Error] UDMA Teardown CQ event register failed!!\n", __func__);
        goto fail3;
    }
#endif

    if (Udma_chEnable(&gUdmaChObj) != UDMA_SOK) {
        fprintf(stderr, "%s: [Error] UDMA channel enable failed!!\n", __func__);
        goto fail4;
    }

    dma_init = 1;
    return (EOK);

fail4:
#ifdef  OSPI_UDMA_INTR
    Udma_eventUnRegister(&gUdmaTdCqEventObj);
fail3:
    Udma_eventUnRegister(&gUdmaCqEventObj);
#endif
fail2:
    Udma_chClose(&gUdmaChObj);
    Udma_deinit(&gUdmaDrvObj);
fail1:
#ifdef  OSPI_UDMA_INTR
    SemaphoreP_delete(gUdmaDoneSem);
#endif
fail0:
    munmap(ospi->v_buf, ospi->buflen);

    return (EIO);
}

int j7ospi_dinit_udma(j7ospi_dev_t *ospi)
{
    uint64_t    pDesc;

    if(dma_init == 0) {
        return (EOK);
    }
    if(ospi != NULL) {
        munmap(ospi->v_buf, ospi->buflen);
    }

    Udma_chDisable(&gUdmaChObj, UDMA_DEFAULT_CH_DISABLE_TIMEOUT);

#ifdef  OSPI_UDMA_INTR
    Udma_eventUnRegister(&gUdmaTdCqEventObj);
    Udma_eventUnRegister(&gUdmaCqEventObj);
#endif

    while (Udma_ringFlushRaw(Udma_chGetFqRingHandle(&gUdmaChObj), &pDesc) != UDMA_ETIMEOUT) {}

    Udma_chClose(&gUdmaChObj);

    Udma_deinit(&gUdmaDrvObj);

#ifdef  OSPI_UDMA_INTR
    SemaphoreP_delete(gUdmaDoneSem);
#endif

    dma_init = 0;

    return (EOK);
}
#endif      // J7OSPI_UDMA_SUPPORT

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/flash/mtd-snor/soc/j7ospi/j7ospi-udma.c $ $Rev: 993630 $")
#endif
