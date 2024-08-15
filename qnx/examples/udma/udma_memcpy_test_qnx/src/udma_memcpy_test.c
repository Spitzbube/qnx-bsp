/*
 *  Copyright (c) Texas Instruments Incorporated 2018-2024
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
 *  \file udma_memcpy_test.c
 *
 *  \brief UDMA memory copy sample application performing block copy using
 *  Type 15 Transfer Record (TR15) using Transfer Record Packet
 *  Descriptor (TRPD).
 *
 *  Requirement: DOX_REQ_TAG(PDK-2634)
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include <hw/inout.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "ti/drv/udma/udma.h"
#include "ti/csl/cslr_ringacc.h"
#include "ti/csl/csl_ringacc.h"
#include "ti/csl/cslr_proxy.h"
#include "ti/csl/csl_proxy.h"
#include <udma_resmgr.h>

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/*
 * Application test parameters
 */
/** \brief Number of bytes to copy and buffer allocation */
#define UDMA_TEST_APP_NUM_BYTES         (1000U)
/** \brief This ensures every channel memory is aligned */
#define UDMA_TEST_APP_NUM_BYTES_ALIGN    ((UDMA_TEST_APP_NUM_BYTES + UDMA_CACHELINE_ALIGNMENT) & ~(UDMA_CACHELINE_ALIGNMENT - 1U))
#define UDMA_TEST_APP_BYTES_ALIGN(X)     ((((X)) + UDMA_CACHELINE_ALIGNMENT) & ~(UDMA_CACHELINE_ALIGNMENT - 1U))

/*
 * Ring parameters
 */
/** \brief Number of ring entries - we can prime this much memcpy operations */
#define UDMA_TEST_APP_RING_ENTRIES      (1U)
/** \brief Size (in bytes) of each ring entry (Size of pointer - 64-bit) */
#define UDMA_TEST_APP_RING_ENTRY_SIZE   (sizeof(uint64_t))
/** \brief Total ring memory */
#define UDMA_TEST_APP_RING_MEM_SIZE     (UDMA_TEST_APP_RING_ENTRIES * \
                                         UDMA_TEST_APP_RING_ENTRY_SIZE)
/** \brief This ensures every channel memory is aligned */
#define UDMA_TEST_APP_RING_MEM_SIZE_ALIGN ((UDMA_TEST_APP_RING_MEM_SIZE + UDMA_CACHELINE_ALIGNMENT) & ~(UDMA_CACHELINE_ALIGNMENT - 1U))
/**
 *  \brief UDMA TR packet descriptor memory.
 *  This contains the CSL_UdmapCppi5TRPD + Padding to sizeof(CSL_UdmapTR15) +
 *  one Type_15 TR (CSL_UdmapTR15) + one TR response of 4 bytes.
 *  Since CSL_UdmapCppi5TRPD is less than CSL_UdmapTR15, size is just two times
 *  CSL_UdmapTR15 for alignment.
 */
#define UDMA_TEST_APP_TRPD_SIZE         ((sizeof(CSL_UdmapTR15) * 2U) + 4U)
/** \brief This ensures every channel memory is aligned */
#define UDMA_TEST_APP_TRPD_SIZE_ALIGN   ((UDMA_TEST_APP_TRPD_SIZE + UDMA_CACHELINE_ALIGNMENT) & ~(UDMA_CACHELINE_ALIGNMENT - 1U))



//#if !defined (QT_BUILD)
#define UDMA_TEST_INTR
//#endif



/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

uint64_t Udma_qnxVirtToPhyFxn(const void *virtAddr,
                              uint32_t chNum,
                              void *appData);
void * Udma_qnxPhyToVirtFxn(uint64_t phyAddr,
                            uint32_t chNum,
                            void *appData);



static int32_t App_memcpyTest(Udma_ChHandle chHandle);
static int32_t App_udmaMemcpy(Udma_ChHandle chHandle,
                              void *destBuf,
                              void *srcBuf,
                              uint32_t length);
#if defined (UDMA_TEST_INTR)
static void App_udmaEventDmaCb(Udma_EventHandle eventHandle,
                               uint32_t eventType,
                               void *appData);
static void App_udmaEventTdCb(Udma_EventHandle eventHandle,
                              uint32_t eventType,
                              void *appData);
#endif

static int32_t App_init(Udma_DrvHandle drvHandle);
static int32_t App_deinit(Udma_DrvHandle drvHandle);

static int32_t App_create(Udma_DrvHandle drvHandle, Udma_ChHandle chHandle);
static int32_t App_delete(Udma_DrvHandle drvHandle, Udma_ChHandle chHandle);

static void App_udmaTrpdInit(Udma_ChHandle chHandle,
                             uint8_t *pTrpdMem,
                             const void *destBuf,
                             const void *srcBuf,
                             uint32_t length);

static void App_print(const char *str);

void parseCmdLine (int argc, char *argv[]);

/* For linker errors */
uint64_t TTBR3_BASE_ADDR, TTBR2_BASE_ADDR, TTBR1_BASE_ADDR;

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/*
 * UDMA driver objects
 */
struct Udma_DrvObj      *gUdmaDrvObj;
struct Udma_ChObj       *gUdmaChObj;
#if defined (UDMA_TEST_INTR)
struct Udma_EventObj    *gUdmaCqEventObj;
struct Udma_EventObj    *gUdmaTdCqEventObj;
#endif

/*
 * UDMA Memories
 */
uint8_t *gTxRingMem;
uint8_t *gTxCompRingMem;
uint8_t *gTxTdCompRingMem;
uint8_t *gUdmaTrpdMem;

/*
 * Application Buffers
 */
uintptr_t gUdmaTestSrcBuf;
uintptr_t gUdmaTestDestBuf;
off64_t   gUdmaTrpdPhys;

/*
 * Memory file descriptor
 */
int memFd = -1;

/*
 * Globals for command line options
 */
uint16_t gTestIterations = 1;
uint16_t gSmmu = 0;
uint16_t gNoSmmuMan = 0;
uint16_t gVirtid = 4;
int      gChannelNum   = -1;
uint16_t gVerbose = 0;
uint16_t gSkipVerification = 0;
uint32_t gUserProvidedPhysAddr = 0;
uint64_t gUserFromPhysAddr = 0;
uint64_t gUserToPhysAddr   = 0;
uint64_t gUserFromVirtAddr = 0;
uint64_t gUserToVirtAddr   = 0;
uint64_t gBufferBytesToCopy = UDMA_TEST_APP_NUM_BYTES;
uint64_t gBufferBytesToCopyAligned = 0;
int gNoCache = 0;
char  memStr[128];

/*
 * Definition and Macro for debug vs. logging
 */
#define APP_LOG 1
#define APP_DBG 2
#define APP_PRINT(level, f_, ...)   if(((gVerbose == 1) && (level == APP_DBG)) || \
                                      (level == APP_LOG)) \
                                      printf((f_), ##__VA_ARGS__)


#if defined (UDMA_TEST_INTR)
/* Semaphore to indicate transfer completion */
static SemaphoreP_Handle gUdmaAppDoneSem = NULL;
#endif

/* Global test pass/fail flag */
static volatile int32_t gUdmaAppResult = UDMA_SOK;

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

void udma_dump_bufs()
{
    int i;
    uint8_t *tempBuf;
    uint64_t srcPhysAddr = 0;
    uint64_t dstPhysAddr = 0;
    uint64_t bufSize = gBufferBytesToCopyAligned;
    uint64_t bytesToDump = gBufferBytesToCopy / 100;

    if (gUserProvidedPhysAddr == 1)
    {
        srcPhysAddr = gUserFromPhysAddr;
        dstPhysAddr = gUserToPhysAddr;
    }
    else
    {
        srcPhysAddr = Udma_qnxVirtToPhyFxn((void *)gUdmaTestSrcBuf, 0, (void *)&bufSize);
        dstPhysAddr = Udma_qnxVirtToPhyFxn((void *)gUdmaTestDestBuf, 0, (void *)&bufSize);
    }

    APP_PRINT(APP_DBG, "Source Buffer virtual=0x%lx(phys=0x%lx):\n", gUdmaTestSrcBuf, srcPhysAddr);

    tempBuf = (uint8_t *)gUdmaTestSrcBuf;
    if (!gSkipVerification)
    {
        APP_PRINT(APP_DBG, "%s: printing %ld bytes of %ld byte transfer\n", __FUNCTION__, bytesToDump, gBufferBytesToCopy);
        for (i = 0; i < bytesToDump; i++)
        {
            APP_PRINT(APP_DBG, "src[%d] = 0x%02X\n", i, tempBuf[i]);
        }
    }

    APP_PRINT(APP_DBG, "Destination Buffer virtual=0x%lx(phys=0x%lx):\n", gUdmaTestDestBuf, dstPhysAddr);
    tempBuf = (uint8_t *)gUdmaTestDestBuf;
    if (!gSkipVerification)
    {
        for (i = 0; i < bytesToDump; i++)
        {
            APP_PRINT(APP_DBG, "dst[%d] = 0x%02X\n", i, tempBuf[i]);
        }
    }
}

void Udma_appUtilsCacheWb(const void *addr, int32_t size)
{
    uint32_t    isCacheCoherent = Udma_isCacheCoherent();

    if(isCacheCoherent != TRUE)
    {
        CacheP_wb(addr, size);
    }

    return;
}

void Udma_appUtilsCacheInv(const void * addr, int32_t size)
{
    uint32_t    isCacheCoherent = Udma_isCacheCoherent();

    if(isCacheCoherent != TRUE)
    {
        CacheP_Inv(addr, size);
    }

    return;
}

void Udma_appUtilsCacheWbInv(const void * addr, int32_t size)
{
    uint32_t    isCacheCoherent = Udma_isCacheCoherent();

    if(isCacheCoherent != TRUE)
    {
        CacheP_wbInv(addr, size);
    }

    return;
}

/*
 * UDMA memcpy test
 */
int32_t Udma_memcpyTest(void)
{
    int32_t         retVal;
    Udma_DrvHandle  drvHandle = gUdmaDrvObj;
    Udma_ChHandle   chHandle = gUdmaChObj;

    retVal = App_init(drvHandle);

    if(UDMA_SOK != retVal)
    {
        APP_PRINT(APP_LOG,"%s:%d: [Error] UDMA App init failed!!\n", __func__, __LINE__);
    }

    if(UDMA_SOK == retVal)
    {
        retVal = App_create(drvHandle, chHandle);
        if(UDMA_SOK != retVal)
        {
            APP_PRINT(APP_LOG,"%s:%d: [Error] UDMA App create failed!!\n", __func__, __LINE__);
        }
    }

    sleep(2);

    if(UDMA_SOK == retVal)
    {
        retVal = App_memcpyTest(chHandle);
        if(UDMA_SOK != retVal)
        {
            APP_PRINT(APP_LOG,"%s:%d: [Error] UDMA App memcpy test failed!!\n", __func__, __LINE__);
        }
    }

    retVal += App_delete(drvHandle, chHandle);
    if(UDMA_SOK != retVal)
    {
        APP_PRINT(APP_LOG, "%s:%d: [Error] UDMA App delete failed!!\n", __func__, __LINE__);
    }

    retVal += App_deinit(drvHandle);
    if(UDMA_SOK != retVal)
    {
        APP_PRINT(APP_LOG,"%s:%d: [Error] UDMA App deinit failed!!\n", __func__, __LINE__);
    }

    if((UDMA_SOK == retVal) && (UDMA_SOK == gUdmaAppResult))
    {
        APP_PRINT(APP_LOG, "UDMA memcpy using TR15 block copy Passed!!\n");
        APP_PRINT(APP_LOG, "All tests have passed!!\n");
    }
    else
    {
        APP_PRINT(APP_LOG, "UDMA memcpy using TR15 block copy Failed!!\n");
        APP_PRINT(APP_LOG, "Some tests have failed!!\n");
    }

    return retVal;
}

static int32_t App_memcpyTest(Udma_ChHandle chHandle)
{
    int32_t             retVal = UDMA_SOK;
    uint64_t            i, j;
    uint64_t            loopCnt = 0U;
    uint8_t             *srcBuf = (uint8_t *) gUdmaTestSrcBuf;
    uint8_t             *destBuf = (uint8_t *)gUdmaTestDestBuf;
    uint64_t            length = gBufferBytesToCopy;
    uint32_t            transferSize;

    while(loopCnt < gTestIterations)
    {
        /* Init buffers */
        if (!gSkipVerification)
        {
            APP_PRINT(APP_DBG, "%s: Setting Src/0x%lx and Dest/0x%lx to 0\n", __FUNCTION__, gUdmaTestSrcBuf, gUdmaTestDestBuf);
            for(i = 0U; i < gBufferBytesToCopy; i++)
            {
                srcBuf[i] = i & 0xFF;
                destBuf[i] = 0U;
            }
            /* Writeback source and destination buffer */
            Udma_appUtilsCacheWb((void *)srcBuf, (int32_t) gBufferBytesToCopy);
            Udma_appUtilsCacheWb((void *)destBuf, (int32_t) gBufferBytesToCopy);
        }

        j = 0;

        while(length > 0)
        {
            /* UDMA transfer size limited to 64K (64K = 0xFFFF) */
            transferSize = (length > 0xFFFF) ? (uint32_t)0xFFFF : (uint32_t)length;
            APP_PRINT(APP_DBG, "%s: Transfer size: %d\n", __FUNCTION__, transferSize);

            /* Perform UDMA memcpy */
            retVal = App_udmaMemcpy(
                        chHandle,
                        destBuf + j * 0xFFFF,
                        srcBuf + j * 0xFFFF,
                        transferSize);
            length -= (uint64_t)transferSize;
            j++;
        }
        length = gBufferBytesToCopy;

        if(UDMA_SOK == retVal)
        {
            /* Compare data */
            /* Invalidate destination buffer */
            Udma_appUtilsCacheInv((void *)gUdmaTestDestBuf, gBufferBytesToCopy);

            if (!gSkipVerification) {
                for(i = 0U; i < gBufferBytesToCopy; i++)
                {
                    if(srcBuf[i] != destBuf[i])
                    {
                        APP_PRINT(APP_LOG, "%s:%d: [Error] Data mismatch!!\n", __func__, __LINE__);
                        retVal = UDMA_EFAIL;
                        break;
                    }
                }
            }


            APP_PRINT(APP_LOG, "%s: Memory check complete\n",__FUNCTION__);delay(100);
        }
        else
        {
            APP_PRINT(APP_LOG, "%s: error returned from App_udmaMemcpy()\n",__FUNCTION__);
        }

        if(UDMA_SOK != retVal)
        {
            break;
        }

        loopCnt++;
        APP_PRINT(APP_LOG, "%s: loop count:%ld\n", __FUNCTION__, loopCnt);
    }

    /* Clean up memory maps */
    munmap_device_memory(srcBuf, length);
    munmap_device_memory(destBuf,length);

    return (retVal);
}

static int32_t App_udmaMemcpy(Udma_ChHandle chHandle,
                              void *destBuf,
                              void *srcBuf,
                              uint32_t length)
{
    int32_t     retVal = UDMA_SOK;
    uint32_t   *pTrResp, trRespStatus;
    uint64_t    pDesc = 0;
    uint8_t    *trpdMem = &gUdmaTrpdMem[0U];
    uint32_t    trpdMemSize = UDMA_TEST_APP_TRPD_SIZE_ALIGN;

    /* Dump buffers before transfer */
    udma_dump_bufs();

    /* Update TR packet descriptor */
    App_udmaTrpdInit(chHandle, trpdMem, destBuf, srcBuf, length);
    APP_PRINT(APP_DBG, "%s: TR Packet descriptor updated\n",__func__);

    /* Submit TRPD to channel */
    retVal = Udma_ringQueueRaw(
                 Udma_chGetFqRingHandle(chHandle), (uint64_t) Udma_qnxVirtToPhyFxn((void *) trpdMem, 0, (void*) &trpdMemSize));
    if(UDMA_SOK != retVal)
    {
        APP_PRINT(APP_LOG,"%s:%d: [Error] Channel queue failed!!\n", __func__, __LINE__);
    }
    APP_PRINT(APP_DBG,"%s: TRPD submitted to channel\n",__func__);

#if defined (UDMA_TEST_INTR)
    if(UDMA_SOK == retVal)
    {
        /* Wait for return descriptor in completion ring - this marks the
         * transfer completion */
        APP_PRINT(APP_DBG, "%s: Waiting on semaphore for return descriptor in completion ring\n",__func__);
        SemaphoreP_pend(gUdmaAppDoneSem, SemaphoreP_WAIT_FOREVER); 
        /* Response received in completion queue */
        retVal = Udma_ringDequeueRaw(Udma_chGetCqRingHandle(chHandle), &pDesc);
        if(UDMA_SOK != retVal)
        {
            APP_PRINT(APP_LOG, "%s:%d: [Error] No descriptor after callback!!\n", __func__, __LINE__);
            retVal = UDMA_EFAIL;
        }

        /* Dump buffer after transfer */
        udma_dump_bufs();
    }
#else
    if(UDMA_SOK == retVal)
    {
        /* Wait for return descriptor in completion ring - this marks the
         * transfer completion */
        APP_PRINT(APP_DBG, "%s: Polling for return descriptor in completion ring\n",__func__);
        while(1)
        {
            /* Response received in completion queue */
            retVal = Udma_ringDequeueRaw(Udma_chGetCqRingHandle(chHandle), &pDesc);
            if(UDMA_SOK == retVal)
            {
                break;
            }
            else {
                /* Delay so other threads can run */
                delay(100);
            }

        }
    }
#endif
    if(UDMA_SOK == retVal)
    {
        /*
         * Sanity check
         */
        /* Check returned descriptor pointer */
        if(pDesc != ((uint64_t) Udma_qnxVirtToPhyFxn((void *)trpdMem, 0, (void*) &trpdMemSize)))
        {
            APP_PRINT(APP_LOG,"%s:%d: [Error] TR descriptor pointer returned doesn't "
                   "match the submitted address!!\n", __func__, __LINE__);
            retVal = UDMA_EFAIL;
        }
    }

    if(UDMA_SOK == retVal)
    {
        /* Invalidate cache */
        Udma_appUtilsCacheInv(&gUdmaTrpdMem[0U], UDMA_TEST_APP_TRPD_SIZE);

        /* check TR response status */
        pTrResp = (uint32_t *) (trpdMem + (sizeof(CSL_UdmapTR15) * 2U));
        trRespStatus = CSL_FEXT(*pTrResp, UDMAP_TR_RESPONSE_STATUS_TYPE);
        if(trRespStatus != CSL_UDMAP_TR_RESPONSE_STATUS_COMPLETE)
        {
            APP_PRINT(APP_LOG,"%s:%d: [Error] TR Response not completed!!\n", __func__, __LINE__);
            retVal = UDMA_EFAIL;
        }
    }

    return (retVal);
}

#if defined (UDMA_TEST_INTR)
static void App_udmaEventDmaCb(Udma_EventHandle eventHandle,
                               uint32_t eventType,
                               void *appData)
{
    if(UDMA_EVENT_TYPE_DMA_COMPLETION == eventType)
    {
        SemaphoreP_post(gUdmaAppDoneSem);
    }
    else
    {
        gUdmaAppResult = UDMA_EFAIL;
    }

    return;
}

static void App_udmaEventTdCb(Udma_EventHandle eventHandle,
                              uint32_t eventType,
                              void *appData)
{
    int32_t             retVal;
    CSL_UdmapTdResponse tdResp;

    if(UDMA_EVENT_TYPE_TEARDOWN_PACKET == eventType)
    {
        /* Response received in Teardown completion queue */
        retVal = Udma_chDequeueTdResponse(gUdmaChObj, &tdResp);
        if(UDMA_SOK != retVal)
        {
            /* [Error] No TD response after callback!! */
            gUdmaAppResult = UDMA_EFAIL;
        }
    }
    else
    {
        gUdmaAppResult = UDMA_EFAIL;
    }

    return;
}
#endif


uint64_t Udma_qnxVirtToPhyFxn(const void * virtAddr,
                              uint32_t chNum,
                              void *appData)
{
    off64_t    phyAddr = 0;
    uint64_t   length;

    if(appData != NULL_PTR) {
        length = (uint64_t) *((uint64_t *) appData);
    }
    else {
        APP_PRINT(APP_LOG,"%s: Must specify memory size to map\n",__FUNCTION__);
        return -1;
    }

    int     tmp_fd = -1;
    size_t  contig_len = 0;
    if (posix_mem_offset64((void *) virtAddr, (size_t)length, &phyAddr, &contig_len, &tmp_fd) != 0)
    {
        if (errno != EAGAIN) {
            APP_PRINT(APP_LOG, "%s:Error from mem_offset - errno=%d\n", __func__, errno);
        }
        else if (phyAddr == 0) {
            APP_PRINT(APP_LOG, "%s:Error from mem_offset - errno=%d and phyAddr is NULL \n", __func__, errno);
        }
    }

    APP_PRINT(APP_DBG,"%s: virt/%p phy/0x%lx\n",__FUNCTION__, virtAddr, phyAddr);

    return (uint64_t ) phyAddr;
}

void * Udma_qnxPhyToVirtFxn(uint64_t phyAddr,
                            uint32_t chNum,
                            void *appData)
{
    uint64_t *temp = 0;
    uint64_t length = 0;
    int prot = PROT_READ | PROT_WRITE;
    int flags;

    if(appData != NULL_PTR) {
        length = (uint64_t) *((uint64_t *) appData);
    }
    else {
        APP_PRINT(APP_LOG,"%s: Must specify memory size to map\n",__FUNCTION__);
        return NULL;
    }

    flags =  MAP_SHARED;
    temp = mmap64(0, (size_t)length, prot, flags, memFd, phyAddr);
    if(temp == MAP_FAILED)
    {
        APP_PRINT(APP_LOG,"%s: mmap64 failed errno/%d, phy/0x%lx\n",__FUNCTION__, errno, phyAddr);
    }
    else
    {
        APP_PRINT(APP_DBG,"%s: virt/%p phy/0x%lx\n",__FUNCTION__, temp, phyAddr);
    }

    return ((void *) temp);
}

static int32_t App_init(Udma_DrvHandle drvHandle)
{
    int32_t         retVal = UDMA_SOK;
    Udma_InitPrms   initPrms;
    uint32_t        instId;

    /* Use MCU NAVSS for MCU domain cores. Rest cores all uses Main NAVSS */
#if defined (SOC_AM62X) || defined(SOC_AM62A) || defined(SOC_J722S)
    instId = UDMA_INST_ID_BCDMA_0;
#else
    instId = UDMA_INST_ID_MAIN_0;
#endif
    /* UDMA driver init */
    UdmaInitPrms_init(instId, &initPrms);

    /* Set virtToPhy and PhytoVirt to support QNX */
    initPrms.virtToPhyFxn = &Udma_qnxVirtToPhyFxn;
    initPrms.phyToVirtFxn = &Udma_qnxPhyToVirtFxn;
#if defined (QT_BUILD)
    initPrms.skipRmOverlapCheck = TRUE;
#endif
    initPrms.printFxn = &App_print;
    retVal = Udma_init(drvHandle, &initPrms);
    if(UDMA_SOK != retVal)
    {
        APP_PRINT(APP_LOG, "%s:%d: [Error] UDMA init failed!!\n", __func__, __LINE__);
    }

    return (retVal);
}

static int32_t App_deinit(Udma_DrvHandle drvHandle)
{
    int32_t     retVal = UDMA_SOK;

    retVal = Udma_deinit(drvHandle);
    if(UDMA_SOK != retVal)
    {
        APP_PRINT(APP_LOG, "%s:%d: [Error] UDMA deinit failed\n", __func__, __LINE__);
    }

    return (retVal);
}

static int32_t App_create(Udma_DrvHandle drvHandle, Udma_ChHandle chHandle)
{
    int32_t             retVal = UDMA_SOK;
    uint32_t            chType;
    Udma_ChPrms         chPrms;
    Udma_ChTxPrms       txPrms;
    Udma_ChRxPrms       rxPrms;
#if defined (UDMA_TEST_INTR)
    Udma_EventHandle    eventHandle;
    Udma_EventPrms      eventPrms;
    SemaphoreP_Params   semPrms;

    SemaphoreP_Params_init(&semPrms);

    gUdmaAppDoneSem = SemaphoreP_create(0, &semPrms);
    if(NULL == gUdmaAppDoneSem)
    {
        APP_PRINT(APP_LOG, "%s:%d: [Error] Sem create failed!!\n", __func__, __LINE__);
        retVal = UDMA_EFAIL;
    }
#endif

    if(UDMA_SOK == retVal)
    {
        /* Init channel parameters */
        chType = UDMA_CH_TYPE_TR_BLK_COPY;

        UdmaChPrms_init(&chPrms, chType);
        chPrms.fqRingPrms.ringMem   = &gTxRingMem[0U];
        chPrms.cqRingPrms.ringMem   = &gTxCompRingMem[0U];
        chPrms.tdCqRingPrms.ringMem = &gTxTdCompRingMem[0U];
        chPrms.fqRingPrms.ringMemSize   = UDMA_TEST_APP_RING_MEM_SIZE;
        chPrms.cqRingPrms.ringMemSize   = UDMA_TEST_APP_RING_MEM_SIZE;
        chPrms.tdCqRingPrms.ringMemSize = UDMA_TEST_APP_RING_MEM_SIZE;
        chPrms.fqRingPrms.elemCnt   = UDMA_TEST_APP_RING_ENTRIES;
        chPrms.cqRingPrms.elemCnt   = UDMA_TEST_APP_RING_ENTRIES;
        chPrms.tdCqRingPrms.elemCnt = UDMA_TEST_APP_RING_ENTRIES;

        if (gSmmu)
        {
            if(gChannelNum != -1) 
            {
                APP_PRINT(APP_DBG,"Setting  preferred channel\n");
                chPrms.chNum = gChannelNum;
            }

            if (gNoSmmuMan)
            {
                APP_PRINT(APP_DBG,"Setting  VirtId to %d\n", gVirtid);
                chPrms.fqRingPrms.virtId        = gVirtid;
                chPrms.cqRingPrms.virtId        = gVirtid;
                chPrms.tdCqRingPrms.virtId      = gVirtid;
            }
        }

        /* Open channel for block copy */
        retVal = Udma_chOpen(drvHandle, chHandle, chType, &chPrms);
        if(UDMA_SOK != retVal)
        {
            APP_PRINT(APP_LOG, "%s:%d: [Error] UDMA channel open failed!!\n", __func__, __LINE__);
        }
    }

    if(UDMA_SOK == retVal)
    {


        /* Config TX channel */
        UdmaChTxPrms_init(&txPrms, chType);

        if ((gSmmu) && (gNoSmmuMan))
        {
            APP_PRINT(APP_DBG,"Setting Tx Parameters Type to Virtual\n");
            txPrms.addrType = TISCI_MSG_VALUE_RM_UDMAP_CH_ATYPE_VIRTUAL;
        }

        retVal = Udma_chConfigTx(chHandle, &txPrms);
        if(UDMA_SOK != retVal)
        {
            APP_PRINT(APP_LOG,"%s:%d: [Error] UDMA TX channel config failed!!\n", __func__, __LINE__);
        }
    }

    if(UDMA_SOK == retVal)
    {

        /* Config RX channel - which is implicitly paired to TX channel in
         * block copy mode */
        UdmaChRxPrms_init(&rxPrms, chType);

        if ((gSmmu) && (gNoSmmuMan))
        {
            APP_PRINT(APP_DBG,"Setting Rx Parameters Type to Virtual\n");
            rxPrms.addrType = TISCI_MSG_VALUE_RM_UDMAP_CH_ATYPE_VIRTUAL;
        }

        retVal = Udma_chConfigRx(chHandle, &rxPrms);
        if(UDMA_SOK != retVal)
        {
            APP_PRINT(APP_LOG,"%s:%d: [Error] UDMA RX channel config failed!!\n", __func__, __LINE__);
        }

    }

#if defined (UDMA_TEST_INTR)
    if(UDMA_SOK == retVal)
    {
        /* Register ring completion callback */
        eventHandle = gUdmaCqEventObj;
        UdmaEventPrms_init(&eventPrms);
        eventPrms.eventType         = UDMA_EVENT_TYPE_DMA_COMPLETION;
        eventPrms.eventMode         = UDMA_EVENT_MODE_SHARED;
        eventPrms.chHandle          = chHandle;
        eventPrms.masterEventHandle = Udma_eventGetGlobalHandle(drvHandle);
        eventPrms.eventCb           = &App_udmaEventDmaCb;
        retVal = Udma_eventRegister(drvHandle, eventHandle, &eventPrms);
        if(UDMA_SOK != retVal)
        {
            APP_PRINT(APP_LOG,"%s:%d: [Error] UDMA CQ event register failed!!\n", __func__, __LINE__);
        }
    }

    if(UDMA_SOK == retVal)
    {
        /* Register teardown ring completion callback */
        eventHandle = gUdmaTdCqEventObj;
        UdmaEventPrms_init(&eventPrms);
        eventPrms.eventType         = UDMA_EVENT_TYPE_TEARDOWN_PACKET;
        eventPrms.eventMode         = UDMA_EVENT_MODE_SHARED;
        eventPrms.chHandle          = chHandle;
        eventPrms.masterEventHandle = Udma_eventGetGlobalHandle(drvHandle);
        eventPrms.eventCb           = &App_udmaEventTdCb;
        retVal = Udma_eventRegister(drvHandle, eventHandle, &eventPrms);
        if(UDMA_SOK != retVal)
        {
            APP_PRINT(APP_LOG,"%s:%d: [Error] UDMA Teardown CQ event register failed!!\n", __func__, __LINE__);
        }
    }
#endif
    if(UDMA_SOK == retVal)
    {
        /* Channel enable */
        retVal = Udma_chEnable(chHandle);
        if(UDMA_SOK != retVal)
        {
            APP_PRINT(APP_LOG,"%s:%d: [Error] UDMA channel enable failed!!\n", __func__, __LINE__);
        }
    }
    return (retVal);
}

static int32_t App_delete(Udma_DrvHandle drvHandle, Udma_ChHandle chHandle)
{
    int32_t             retVal, tempRetVal;
    uint64_t            pDesc;
#if defined (UDMA_TEST_INTR)
    Udma_EventHandle    eventHandle;
#endif

    retVal = Udma_chDisable(chHandle, UDMA_DEFAULT_CH_DISABLE_TIMEOUT);
    if(UDMA_SOK != retVal)
    {
        APP_PRINT(APP_LOG,"%s:%d: [Error] UDMA channel disable failed!!\n", __func__, __LINE__);
    }

#if defined (UDMA_TEST_INTR)
    /* Unregister all events */
    eventHandle = gUdmaTdCqEventObj;
    retVal += Udma_eventUnRegister(eventHandle);
    eventHandle = gUdmaCqEventObj;
    retVal += Udma_eventUnRegister(eventHandle);
    if(UDMA_SOK != retVal)
    {
        APP_PRINT(APP_LOG,"%s:%d: [Error] UDMA event unregister failed!!\n", __func__, __LINE__);
    }
#endif

    /* Flush any pending request from the free queue */
    while(1)
    {
        tempRetVal = Udma_ringFlushRaw(
                         Udma_chGetFqRingHandle(chHandle), &pDesc);
        if(UDMA_ETIMEOUT == tempRetVal)
        {
            break;
        }

        if (tempRetVal == UDMA_EBADARGS || tempRetVal == UDMA_EFAIL)
        {
            APP_PRINT(APP_LOG,"%s:%d: [Error] UDMA ring flush failed!!\n", __func__, __LINE__);
            break;
        }
    }

    retVal += Udma_chClose(chHandle);
    if(UDMA_SOK != retVal)
    {
        APP_PRINT(APP_LOG,"%s:%d: [Error] UDMA channel close failed!!\n", __func__, __LINE__);
    }

#if defined (UDMA_TEST_INTR)
    if(gUdmaAppDoneSem != NULL)
    {
        SemaphoreP_delete(gUdmaAppDoneSem);
    }
#endif

    return (retVal);
}

static void App_udmaTrpdInit(Udma_ChHandle chHandle,
                             uint8_t *pTrpdMem,
                             const void *destBuf,
                             const void *srcBuf,
                             uint32_t length)
{
    CSL_UdmapCppi5TRPD *pTrpd = (CSL_UdmapCppi5TRPD *) pTrpdMem;
    CSL_UdmapTR15 *pTr = (CSL_UdmapTR15 *)(pTrpdMem + sizeof(CSL_UdmapTR15));
    uint32_t *pTrResp = (uint32_t *) (pTrpdMem + (sizeof(CSL_UdmapTR15) * 2U));
    uint32_t cqRingNum = Udma_chGetCqRingNum(chHandle);
    uint32_t tr15_size = sizeof(CSL_UdmapTR15) ;
    uint64_t srcPhysAddr = 0;
    uint64_t dstPhysAddr = 0;

    /* Make TRPD */
    UdmaUtils_makeTrpd(pTrpd, UDMA_TR_TYPE_15, 1U, cqRingNum);

    /* UDMA descriptor passes physical addresses */
    if(gUserProvidedPhysAddr == 1)
    {
        srcPhysAddr = gUserFromPhysAddr;
        dstPhysAddr = gUserToPhysAddr;
    }
    else
    {
        srcPhysAddr = (uint64_t) Udma_qnxVirtToPhyFxn(srcBuf, 0, &tr15_size);
        dstPhysAddr = (uint64_t) Udma_qnxVirtToPhyFxn(destBuf, 0, &tr15_size);
    }

    /* Setup TR */
    pTr->flags    = CSL_FMK(UDMAP_TR_FLAGS_TYPE, 15)                                            |
                    CSL_FMK(UDMAP_TR_FLAGS_STATIC, 0U)                                          |
                    CSL_FMK(UDMAP_TR_FLAGS_EOL, 0U)                                             |   /* NA */
                    CSL_FMK(UDMAP_TR_FLAGS_EVENT_SIZE, CSL_UDMAP_TR_FLAGS_EVENT_SIZE_COMPLETION)|
                    CSL_FMK(UDMAP_TR_FLAGS_TRIGGER0, CSL_UDMAP_TR_FLAGS_TRIGGER_NONE)           |
                    CSL_FMK(UDMAP_TR_FLAGS_TRIGGER0_TYPE, CSL_UDMAP_TR_FLAGS_TRIGGER_TYPE_ALL)  |
                    CSL_FMK(UDMAP_TR_FLAGS_TRIGGER1, CSL_UDMAP_TR_FLAGS_TRIGGER_NONE)           |
                    CSL_FMK(UDMAP_TR_FLAGS_TRIGGER1_TYPE, CSL_UDMAP_TR_FLAGS_TRIGGER_TYPE_ALL)  |
                    CSL_FMK(UDMAP_TR_FLAGS_CMD_ID, 0x25U)                                       |   /* This will come back in TR response */
                    CSL_FMK(UDMAP_TR_FLAGS_SA_INDIRECT, 0U)                                     |
                    CSL_FMK(UDMAP_TR_FLAGS_DA_INDIRECT, 0U)                                     |
                    CSL_FMK(UDMAP_TR_FLAGS_EOP, 1U);
    pTr->icnt0    = length;
    pTr->icnt1    = 1U;
    pTr->icnt2    = 1U;
    pTr->icnt3    = 1U;
    pTr->dim1     = pTr->icnt0;
    pTr->dim2     = (pTr->icnt0 * pTr->icnt1);
    pTr->dim3     = (pTr->icnt0 * pTr->icnt1 * pTr->icnt2);
    pTr->addr     = srcPhysAddr;
    pTr->fmtflags = 0x00000000U;        /* Linear addressing, 1 byte per elem.
                                           Replace with CSL-FL API */
    pTr->dicnt0   = length;
    pTr->dicnt1   = 1U;
    pTr->dicnt2   = 1U;
    pTr->dicnt3   = 1U;
    pTr->ddim1    = pTr->dicnt0;
    pTr->ddim2    = (pTr->dicnt0 * pTr->dicnt1);
    pTr->ddim3    = (pTr->dicnt0 * pTr->dicnt1 * pTr->dicnt2);
    pTr->daddr    = dstPhysAddr;

    /* Clear TR response memory */
    *pTrResp = 0xFFFFFFFFU;

    /* Writeback cache */
    Udma_appUtilsCacheWb(pTrpdMem, UDMA_TEST_APP_TRPD_SIZE);

    return;
}
static void App_print(const char *str)
{

    APP_PRINT(APP_LOG, "%s\n", str);
    return;
}

void *App_alloc(size_t size, paddr64_t *paddr, int noCache)
{
    off64_t offset;
    size_t contig_len;
    uint64_t physAddr = 0;

    void *buf = MAP_FAILED;
    int fd    = -1;
    int flags = 0;
    int prot  = PROT_READ | PROT_WRITE;


    /* The caller of the API, may or may not have provided a physical address to be mapped */
    if(paddr != NULL)
    {
        physAddr = *paddr;
    }

    if(noCache)
    {
        APP_PRINT(APP_DBG, "%s: Cache Disabled\n",__func__);
        prot |= PROT_NOCACHE;
    }

    /*
     * User did not specify a physical address, then use the memory pool.
     * This is the expected usage.
     *
     * Specifying physical address only required for failure testing
     */
    if(physAddr == 0)
    {
        fd = memFd;
        flags = MAP_SHARED;
    }
    /*
     * User did specify a src/dst physical address so create a memory
     * mapping directly to that address.
     *
     */
    else
    {
        fd = NOFD;
        flags = MAP_PHYS | MAP_PRIVATE;
        APP_PRINT(APP_DBG,
               "%s: physical addr specified, mmap flags = MAP_PHYS | MAP_PRIVATE\n",
               __FUNCTION__);
    }

    buf = mmap64(0, size, prot, flags, fd, physAddr);
    if(buf != MAP_FAILED)
    {
        /*
        * If physical address was not provided, figure out which
        * address was mapped.
        */
        if(physAddr == 0)
        {
            int tmp_fd = -1;
            if (posix_mem_offset64(buf, size, &offset, &contig_len, &tmp_fd) != 0)
            {

                APP_PRINT(APP_LOG, "%s:%d: Error: Could not obtain buffer physical address\n",
                __FUNCTION__, __LINE__);

                munmap(buf, size);

                return MAP_FAILED;
            }
            if (paddr != NULL)
            {
                *paddr = (uint64_t) offset;
            }
            physAddr = offset;
        }

        APP_PRINT(APP_DBG, "%s: Alloc successful; Virt: 0x%p, Phy: 0x%lx Contig_len: %ld\n",
              __FUNCTION__, buf, physAddr, contig_len);
    }
    else
    {
        perror("mmap64 failed");
        APP_PRINT(APP_LOG, "%s: Alloc failed; Virt: 0x%p, Phys: 0x%lx\n",
                    __FUNCTION__,  buf, offset);
    }


    if(paddr != NULL)
    {
        *paddr = physAddr;
    }

    return buf;
}

void App_free(void *addr, size_t size)
{
    munmap(addr, size);
}

/*
 * Print usage information
 */
void printUsage(void)
{
    APP_PRINT(APP_LOG, "udma_memcpy_testapp <options>                                  \n");
    APP_PRINT(APP_LOG, "                                                               \n");
    APP_PRINT(APP_LOG, "Options:                                                       \n");
    APP_PRINT(APP_LOG, "    v          - Be verbose                                    \n");
    APP_PRINT(APP_LOG, "    V<virt_id> - virtid and without smmuman                    \n");
    APP_PRINT(APP_LOG, "    c<channel> - sMMU will be used, with channel number        \n");
    APP_PRINT(APP_LOG, "    f          - Optionally specify source address in hex      \n");
    APP_PRINT(APP_LOG, "    t          - Optionally specify destination address in hex \n");
    APP_PRINT(APP_LOG, "    n          - Optionally disable cache for memory           \n");
    APP_PRINT(APP_LOG, "    m<string>  - Memory region to allocate from                \n");
    APP_PRINT(APP_LOG, "                 default is 'ram'                              \n");
    APP_PRINT(APP_LOG, "    l          - Length to copy, in bytes (default is 1k)      \n");
    APP_PRINT(APP_LOG, "    s          - Skip verification of copied data. This is     \n");
    APP_PRINT(APP_LOG, "                 useful for generating large amount of UDMA    \n");
    APP_PRINT(APP_LOG, "                 traffic quickly. This will also skip the      \n");
    APP_PRINT(APP_LOG, "                 printing of the input and output buffers in   \n");
    APP_PRINT(APP_LOG, "                 verbose mode.                                 \n");
    APP_PRINT(APP_LOG, "    i          - number of iterations to run test back to back \n");
    APP_PRINT(APP_LOG, "                                                               \n");
    APP_PRINT(APP_LOG, "Normal test :                                                  \n");
    APP_PRINT(APP_LOG, "  udma_memcpy_testapp -v                                       \n");
    APP_PRINT(APP_LOG, "Normal test with preferred channel:                            \n");
    APP_PRINT(APP_LOG, "  udma_memcpy_testapp -v -c16                                  \n");
    APP_PRINT(APP_LOG, "Test with preferred channel & smmuman:                         \n");
    APP_PRINT(APP_LOG, "  udma_memcpy_testapp -v -c16 -n                               \n");
    APP_PRINT(APP_LOG, "Test with preferred channel & virt_id4 & no smmuman:           \n");
    APP_PRINT(APP_LOG, "  udma_memcpy_testapp -v -c16 -V4                             \n");
    exit(0);
}

void parseCmdLine(int argc, char *argv[])
{
    int c;
    char * end_p;

    while ((c = getopt(argc, argv, "V:c:vf:t:nsm:hl:i:")) != -1)
    {
        switch (c)
        {
        case 'c':
            gSmmu = 1;
            gChannelNum = (uint16_t)strtol(optarg, 0, 0);
            APP_PRINT(APP_DBG, "Udma memory transfer will use sMMU with chNum/%d\n", gChannelNum);
            break;
        case 'V':
            gSmmu = 1;
            gNoSmmuMan = 1;
            gVirtid = (uint16_t)strtol(optarg, 0, 0);
            APP_PRINT(APP_DBG, "Udma memory transfer without smmuman and virt_id %d\n", gVirtid);
            break;
        case 'v':
            gVerbose = 1;
            break;
        case 'f':
            gUserProvidedPhysAddr = 1;
            gUserFromPhysAddr = strtoll(optarg, NULL, 16);
            APP_PRINT(APP_DBG, "Set From Physical Address to 0x%10lX\n", gUserFromPhysAddr);
            break;
        case 't':
            gUserToPhysAddr = strtoll(optarg, NULL, 16);
            APP_PRINT(APP_DBG, "Set To Physical Address to 0x%10lX\n", gUserToPhysAddr);
            break;
        case 'n':
            gNoCache = 1;
            APP_PRINT(APP_DBG, "Memory will be mapped as PROT_NOCACHE\n");
            break;
        case 'm':
            strcpy(memStr, optarg);
            APP_PRINT(APP_DBG, "User Specified '%s' for memory\n", memStr);
            break;
        case 'l':
            
            gBufferBytesToCopy = (uint64_t)strtol(optarg, &end_p, 0);
            if (tolower(*end_p) == 'g')
            {
                gBufferBytesToCopy *= 1000000000;
            }
            else if (tolower(*end_p) == 'm')
            {
                gBufferBytesToCopy *= 1000000;
            }
            else if (tolower(*end_p) == 'k')
            {
                gBufferBytesToCopy *= 1000;
            }

            if (gBufferBytesToCopy == 0)
            {
                APP_PRINT(APP_LOG, "Invalid number of bytes to copy.\n");
                printUsage();
                exit(-1);
            }
            else
            {
                APP_PRINT(APP_LOG, "To copy %ld bytes.\n", gBufferBytesToCopy);
            }
            break;
        case 's':
            gSkipVerification = 1;
            break;
        case 'i':
            gTestIterations = (uint16_t)strtol(optarg, 0, 0);
            APP_PRINT(APP_DBG, "udma_memcpy_test_app will run %d times\n", gTestIterations);
            break;
        case '?':
            if (optopt == 'l')
            {
                APP_PRINT(APP_LOG, "Option -l requires an argument.\n");
            }
            else
            {
                APP_PRINT(APP_LOG, "Unrecognized option -%c\n", optopt);
            }
        case 'h':
        default:
            printUsage();
        }
    }
}

int main (int argc, char **argv)
{
    strcpy(memStr, "ram");
    parseCmdLine(argc, argv);

    gBufferBytesToCopyAligned = UDMA_TEST_APP_BYTES_ALIGN(gBufferBytesToCopy);

    int32_t status = UDMA_SOK;

    APP_PRINT(APP_LOG,"Using '%s' for allocating memory\n",memStr);

    /*
     * Create a file descriptor to the memory pool that will be used.
     */
    memFd = posix_typed_mem_open(memStr, O_RDWR, POSIX_TYPED_MEM_ALLOCATE_CONTIG);
    if(memFd == -1)
    {
        perror("Unable to open memory region.\n");
        APP_PRINT(APP_LOG, "%s: Failed to open memory regions %s, errno/%d",__func__,memStr, errno);
        exit(-1);
    }
    APP_PRINT(APP_DBG, "%s: Opened typed memory '%s' fd/%d\n",__func__,memStr, memFd);

    if(gVerbose)
    {
        struct posix_typed_mem_info info;
        int ret = 0;
        ret = posix_typed_mem_get_info(
              memFd,
              &info);
        if(ret == 0)
        {
            APP_PRINT(APP_DBG, "%s: posix_tmi_length/%ld posix_tmi_total/%ld\n",__func__, info.posix_tmi_length, info.__posix_tmi_total);
        }
        else
        {
            APP_PRINT(APP_DBG, "%s: posix_typed_mem_get_info failed\n",__func__ );
        }
    }

    /* Allocate Source and Destination address spaces */
    gUdmaTestSrcBuf = (uintptr_t) App_alloc(gBufferBytesToCopyAligned,  &gUserFromPhysAddr, gNoCache);
    gUdmaTestDestBuf = (uintptr_t) App_alloc(gBufferBytesToCopyAligned, &gUserToPhysAddr, gNoCache );

    /* Allocate UDMA Objects */
    gUdmaDrvObj = (struct Udma_DrvObj *) App_alloc(sizeof(struct Udma_DrvObj), NULL, 0);
    gUdmaChObj  =  (struct Udma_ChObj *) App_alloc(sizeof(struct Udma_ChObj),  NULL, 0);
    if ((gUdmaDrvObj == MAP_FAILED) || (gUdmaDrvObj == MAP_FAILED))
    {
        APP_PRINT(APP_LOG, "%s: Allocation of UDMA objects failed\n",__func__);
        exit(-1);
    }
    #if defined (UDMA_TEST_INTR)
    gUdmaCqEventObj  = (struct Udma_EventObj *) App_alloc(sizeof(struct Udma_EventObj), NULL, 0);
    gUdmaTdCqEventObj= (struct Udma_EventObj *) App_alloc(sizeof(struct Udma_EventObj), NULL, 0);
    if ((gUdmaCqEventObj == MAP_FAILED) || (gUdmaTdCqEventObj == MAP_FAILED))
    {
        APP_PRINT(APP_LOG,"%s: Allocation of UDMA Event objects failed\n",__func__);
        exit(-1);
    }
    #endif

    /* Allocate descriptor and ring memory */
    gTxRingMem       = (uint8_t *) App_alloc(UDMA_TEST_APP_RING_MEM_SIZE_ALIGN, NULL, gNoCache);
    gTxCompRingMem   = (uint8_t *) App_alloc(UDMA_TEST_APP_RING_MEM_SIZE_ALIGN, NULL, gNoCache);
    gTxTdCompRingMem = (uint8_t *) App_alloc(UDMA_TEST_APP_RING_MEM_SIZE_ALIGN, NULL, gNoCache);
    gUdmaTrpdMem     = (uint8_t *) App_alloc(UDMA_TEST_APP_TRPD_SIZE_ALIGN, NULL, gNoCache);
    if ((gTxRingMem == MAP_FAILED) || (gTxCompRingMem == MAP_FAILED) ||
        (gTxTdCompRingMem == MAP_FAILED) || (gUdmaTrpdMem == MAP_FAILED))
    {
      APP_PRINT(APP_LOG,"%s: Allocation of descriptor and ring memory failed\n",__func__);
      exit(-1);
    }

    /* Get IO priveleges */
    if (ThreadCtl(_NTO_TCTL_IO, NULL) == -1) {
        perror("ThreadCtl(_NTO_TCTL_IO");
        return 1;
    }

    status = Udma_memcpyTest();

    return status;
}

