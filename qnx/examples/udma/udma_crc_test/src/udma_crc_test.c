/*
 *  Copyright (c) Texas Instruments Incorporated 2018-2023
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
 *  \file udma_crc_test.c
 *
 *  \brief UDMA crc copy sample application performing block copy using
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
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "ti/drv/udma/udma.h"
#include "ti/csl/soc.h"
#include "ti/csl/csl_crc.h"
#include "ti/csl/cslr_ringacc.h"
#include "ti/csl/csl_ringacc.h"
#include "ti/csl/cslr_proxy.h"
#include "ti/csl/csl_proxy.h"
#include <udma_resmgr.h>

#include <sys/trace.h>



/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/*
 * Application test parameters
 */
/** \brief Number of times to perform the CRC operation */
#define UDMA_TEST_APP_LOOP_CNT          (1U)

/*
 * Ring parameters
 */
/** \brief Number of ring entries - we can prime this much CRC operations */
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

/* Pre-calculated crc signature value for given data pattern */
#define APP_CRC_REFERENCE_SIGN_VAL_L    (0x83A8C73AU)
#define APP_CRC_REFERENCE_SIGN_VAL_H    (0x18633761U)

/* Frame details - used as reference data */
//#define APP_FRAME_HEIGHT                ((uint32_t) 200U)
//#define APP_FRAME_WIDTH                 ((uint32_t) 100U)
//#define APP_FRAME_SIZE                  (APP_FRAME_HEIGHT * APP_FRAME_WIDTH)
//#define APP_FRAME_SIZE_ALIGN            ((APP_FRAME_SIZE + UDMA_CACHELINE_ALIGNMENT) & ~(UDMA_CACHELINE_ALIGNMENT - 1U))

/* CRC channel parameters */
#define APP_CRC_CHANNEL                 (CRC_CHANNEL_1)
#define APP_CRC_CH_CCITENR_MASK         (CRC_INTR_CH1_CCITENR_MASK)
#define APP_CRC_WATCHDOG_PRELOAD_VAL    ((uint32_t) 0U)
#define APP_CRC_BLOCK_PRELOAD_VAL       ((uint32_t) 0U)

/* CRC size parameters */
#define APP_CRC_PATTERN_SIZE            ((uint32_t) 4U)
#define APP_CRC_SECT_CNT                ((uint32_t) 1U)

#if defined (SOC_AM64X) || defined(SOC_J722S)
#define APP_CRC_BASE                    (CSL_MCU_MCRC64_0_REGS_BASE)
#define APP_CRC_SIZE                    (CSL_MCU_MCRC64_0_REGS_SIZE)
#else
/* Use MCU NAVSS/peripherals for MCU domain cores. Rest all uses Main NAVSS */
#if defined (BUILD_MCU1_0) || defined (BUILD_MCU1_1)
#define APP_CRC_BASE                    (CSL_MCU_NAVSS0_MCRC_BASE)
#else
#define APP_CRC_BASE                    (CSL_NAVSS0_MCRC_BASE)
#define APP_CRC_SIZE                    (CSL_NAVSS0_MCRC_SIZE)
#endif

#endif


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


static int32_t App_crcTest(Udma_ChHandle chHandle);
static int32_t App_udmaCrc(Udma_ChHandle chHandle,
                           void *srcBuf,
                           uint32_t length);

static void App_udmaEventCb(Udma_EventHandle eventHandle,
                            uint32_t eventType,
                            void *appData);

static int32_t App_init(Udma_DrvHandle drvHandle);
static int32_t App_deinit(Udma_DrvHandle drvHandle);

static int32_t App_create(Udma_DrvHandle drvHandle, Udma_ChHandle chHandle);
static int32_t App_delete(Udma_DrvHandle drvHandle, Udma_ChHandle chHandle);

static void App_udmaTrpdInit(Udma_ChHandle chHandle,
                             uint8_t *pTrpdMem,
                             const void *srcBuf,
                             const void *destBuf,
                             uint32_t length);
static void App_crcInit(void);

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
#define UDMA_TEST_INTR
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
off64_t   gUdmaTrpdPhys;

/* Semaphore to indicate transfer completion */
static SemaphoreP_Handle gUdmaAppDoneSem = NULL;


/*
 * Application Buffers
 */
uintptr_t gUdmaTestSrcBuf;
//uintptr_t gUdmaTestDestBuf;
off64_t   gUdmaTrpdPhys;

/*
 * Memory file descriptor
 */
int memFd = -1;


/*
 * Globals for command line options
 */
uint16_t gVerbose = 0;
uint64_t gUserProvidedPhysAddr = 0;
uint64_t gUserFromPhysAddr = 0;
uint64_t gUserToPhysAddr   = 0;
uint64_t gUserFromVirtAddr = 0;
uint64_t gUserToVirtAddr   = 0;
uint32_t gOutputToFile = 0;
char gFilename[256];
uintptr_t gAppCrcBase = 0;
uint32_t gAppCrcSize = APP_CRC_SIZE;
uint16_t gTrace = 0;
uint32_t gIters = 1;
int gMeasureTime = 0;
struct timespec gTransferStarted;
struct timespec gTransferCompleted;
double total_nsecs = 0;


/*
 * Size of buffer to be transferred
 */
/** \brief Number of bytes to copy and buffer allocation */
uint32_t gUdmaTestAppNumBytes = 20000;
/** \brief This ensures every channel memory is aligned */
uint32_t gUdmaTestAppNumBytesAlign = 20000;
/** \brief Number of bytes to dump out for debug */
uint32_t gUdmaTestAppNumBytesDump = 32;


/* Pre-calculated crc signature value for given data pattern */
uint32_t gUdmaTestAppCrcL = APP_CRC_REFERENCE_SIGN_VAL_L;
uint32_t gUdmaTestAppCrcH = APP_CRC_REFERENCE_SIGN_VAL_H;
/* Source buffer byte alignment, effects 2D UDMA descriptor configuration */
uint32_t gSrcPatternSize = 4;

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


/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */




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
 * UDMA crc test
 */
int32_t Udma_crcTest(void)
{
    int32_t         retVal;
    Udma_DrvHandle  drvHandle = gUdmaDrvObj;
    Udma_ChHandle   chHandle = gUdmaChObj;

    APP_PRINT(APP_LOG,"UDMA CRC application started...\n");

    retVal = App_init(drvHandle);

    if(UDMA_SOK != retVal)
    {
        APP_PRINT(APP_LOG,"[Error] UDMA App init failed!!\n");
    }

    if(UDMA_SOK == retVal)
    {
        retVal = App_create(drvHandle, chHandle);
        if(UDMA_SOK != retVal)
        {
            APP_PRINT(APP_LOG,"[Error] UDMA App create failed!!\n");
        }
    }

    sleep(2);

    if(UDMA_SOK == retVal)
    {
        retVal = App_crcTest(chHandle);
        if(UDMA_SOK != retVal)
        {
            APP_PRINT(APP_LOG,"[Error] UDMA App memcpy test failed!!\n");
        }
    }

    APP_PRINT(APP_LOG, "UDMA App delete !!\n");fflush(stdout);delay(100);

    retVal += App_delete(drvHandle, chHandle);
    if(UDMA_SOK != retVal)
    {
        APP_PRINT(APP_LOG, "[Error] UDMA App delete failed!!\n");
    }

    APP_PRINT(APP_LOG, "UDMA App deinit !!\n");fflush(stdout);delay(100);
    retVal += App_deinit(drvHandle);
    if(UDMA_SOK != retVal)
    {
        APP_PRINT(APP_LOG,"[Error] UDMA App deinit failed!!\n");
    }

    if(UDMA_SOK == retVal)
    {
        App_print("UDMA CRC Test Passed!!\n");
        App_print("All tests have passed!!\n");
    }
    else
    {
        App_print("UDMA CRC Test Failed!!\n");
        App_print("Some tests have failed!!\n");
    }

    return retVal;
}

static int32_t App_crcTest(Udma_ChHandle chHandle)
{
    int32_t             retVal = UDMA_SOK;
    uint32_t            i;
    uint32_t            loopCnt = 0U;
    uint8_t             *srcBuf = (uint8_t *) gUdmaTestSrcBuf;
    uint32_t            *srcBuf32 = (uint32_t *) gUdmaTestSrcBuf;


    /* Init buffers */
    for(i = 0U; i < (gUdmaTestAppNumBytes / sizeof(uint32_t)); i++)
    {
        srcBuf32[i] = i;
    }

    /*
     * Open file to dump buffer under test.  This file output can be used
     * to generate offline checksum.
     */
    if(gOutputToFile)
    {
        FILE                *fp;
        uint32_t            bytesWritten = 0;

        fp = fopen(gFilename, "w");

        if (fp == NULL)
        {
            APP_PRINT(APP_LOG, "%s: Failed to open file <%s> errno/%d.\n", __FUNCTION__, gFilename, errno);
            return UDMA_EFAIL;
        }

        bytesWritten = fwrite(srcBuf, 1, gUdmaTestAppNumBytes, fp);
        if(bytesWritten != gUdmaTestAppNumBytes)
        {
            APP_PRINT(APP_LOG, "%s: Failed to write file %d %d file <%s>.\n",
                __FUNCTION__, bytesWritten, gUdmaTestAppNumBytes, gFilename);
            fclose(fp);
            return UDMA_EFAIL;
        }
        else
        {
            APP_PRINT(APP_DBG, "%s: Wrote %d bytes to file <%s>.\n",
                        __FUNCTION__, gUdmaTestAppNumBytes, gFilename);
            fclose(fp);
        }
    }

    /* Writeback source buffer */
    Udma_appUtilsCacheWb((void *)srcBuf, (int32_t) gUdmaTestAppNumBytes);
    while(loopCnt < UDMA_TEST_APP_LOOP_CNT)
    {
        /* Perform UDMA CRC */
        retVal = App_udmaCrc(chHandle, srcBuf, gUdmaTestAppNumBytes);
        if(UDMA_SOK == retVal)
        {
            if(gMeasureTime)
            {
                double total_msecs  = (double) (( double) total_nsecs / (double) 1000000);
                double size_mb     = (double) ( ((double) gUdmaTestAppNumBytes * (double) gIters) / (double)1048576);
                size_mb = size_mb;  // Read and write are occurring

                APP_PRINT(APP_LOG, "%s: Memory copy of %d, iterations %d, total_bytes/%f MB, took %f ms, which is %2.f MB/s\n",
                        __FUNCTION__,
                        gUdmaTestAppNumBytes,
                        gIters,
                        size_mb,
                        (double) total_msecs,
                        (double) ((double) ((double)size_mb/ (double) total_msecs)) * (double) 1000);
            }

        }

        if(UDMA_SOK != retVal)
        {
            APP_PRINT(APP_LOG, "%s: CRC  failed\n",__FUNCTION__);delay(100);
            break;
        }

        loopCnt++;
    }

    return (retVal);
}

static int32_t App_udmaCrc(Udma_ChHandle chHandle,
                           void *srcBuf,
                           uint32_t length)
{
    int32_t     retVal = UDMA_SOK;
    uint32_t   *pTrResp, trRespStatus;
    uint64_t    pDesc = 0;
    uint8_t    *trpdMem = &gUdmaTrpdMem[0U];
    uint32_t    trpdMemSize = UDMA_TEST_APP_TRPD_SIZE_ALIGN;
    uint64_t    trpdMemPhysAddr = 0;
    uint32_t    count = 0;
    crcSignature_t        sectSignVal;
    crcSignatureRegAddr_t psaSignRegAddr;
    uint32_t              patternCnt;

    sectSignVal.regL = 0U;
    sectSignVal.regH = 0U;
    patternCnt = length / gSrcPatternSize; //APP_CRC_PATTERN_SIZE;


    /* Get CRC PSA signature register address */
    CRCGetPSASigRegAddr(gAppCrcBase, APP_CRC_CHANNEL, &psaSignRegAddr);

    CRCChannelReset(gAppCrcBase, APP_CRC_CHANNEL);
    CRCConfigure(gAppCrcBase, APP_CRC_CHANNEL, patternCnt, APP_CRC_SECT_CNT, CRC_OPERATION_MODE_SEMICPU);


    /* Update TR packet descriptor */
    App_udmaTrpdInit(chHandle, trpdMem, srcBuf, (void *)(uintptr_t) psaSignRegAddr.regL, length);

    /* Store physical address of descriptor */
    trpdMemPhysAddr = (uint64_t) Udma_qnxVirtToPhyFxn((void *) trpdMem, 0, (void*) &trpdMemSize);

    while(count < gIters)
    {
        /* Submit TRPD to channel */
        if(gMeasureTime)
        {
           clock_gettime( CLOCK_REALTIME , &gTransferStarted );
        }

        if(gTrace)
        {
            trace_logf(123, "CRC Started");
        }

        /* Re-use descriptor if iteration is > 1 */
        retVal = Udma_ringQueueRaw(Udma_chGetFqRingHandle(chHandle), trpdMemPhysAddr);

        if(gTrace)
        {
            trace_logf(123, "CRC Sent");
        }
        if(UDMA_SOK != retVal)
        {
            APP_PRINT(APP_LOG,"[Error] Channel queue failed!!\n");
        }

#if defined (UDMA_TEST_INTR)
        if(UDMA_SOK == retVal)
        {
            /* Wait for return descriptor in completion ring - this marks the
             * transfer completion */
            //APP_PRINT(APP_DBG, "%s: Waiting on semaphore for return descriptor in completion ring\n",__func__);
            SemaphoreP_pend(gUdmaAppDoneSem, SemaphoreP_WAIT_FOREVER);

            if(gTrace)
            {
                trace_logf(456, "Udma Complete");
            }

            /* Response received in completion queue */
            retVal = Udma_ringDequeueRaw(Udma_chGetCqRingHandle(chHandle), &pDesc);
            if(UDMA_SOK != retVal)
            {
                APP_PRINT(APP_LOG, "[Error] No descriptor after callback!!\n");
                retVal = UDMA_EFAIL;
            }

        }

#else

        if(UDMA_SOK == retVal)
        {
            /* Wait for return descriptor in completion ring - this marks the
             * transfer completion */
            //APP_PRINT(APP_DBG, "%s: Polling for return descriptor in completion ring\n",__func__);
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
        if(gMeasureTime)
        {
            uint64_t nsecs_ts1;
            uint64_t nsecs_ts2;

            clock_gettime( CLOCK_REALTIME, &gTransferCompleted );

            nsecs_ts1 = timespec2nsec(&gTransferStarted);
            nsecs_ts2 = timespec2nsec(&gTransferCompleted);
            total_nsecs += (double) ((double) nsecs_ts2 - (double) nsecs_ts1);
            APP_PRINT(APP_DBG,"%s: count/%d total_nsecs/%f\n",__FUNCTION__, count, total_nsecs);
        }
        count++;
    }
    if(UDMA_SOK == retVal)
    {
        /*
         * Sanity check
         */
        /* Check returned descriptor pointer */
        if(pDesc != ((uint64_t) Udma_qnxVirtToPhyFxn((void *)trpdMem, 0, (void*) &trpdMemSize)))
        {
            APP_PRINT(APP_LOG,"[Error] TR descriptor pointer returned doesn't "
                   "match the submitted address!!\n");
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
            APP_PRINT(APP_LOG,"[Error] TR Response not completed!!\n");
            retVal = UDMA_EFAIL;
        }

    }

    if(UDMA_SOK == retVal)
    {
        uint32_t intrStatus;

        while (1U)
        {
            CRCGetIntrStatus(gAppCrcBase, APP_CRC_CHANNEL, &intrStatus);
            if((intrStatus & APP_CRC_CH_CCITENR_MASK) == 0x1U)
            {
                break;
            }
            /* Wait here till CRC compression complete is set. */
        }

        CRCGetPSASectorSig(gAppCrcBase, APP_CRC_CHANNEL, &sectSignVal);
        /* Compare CRC signature value against reference CRC signature */
        if((sectSignVal.regH == gUdmaTestAppCrcH) &&
           (sectSignVal.regL == gUdmaTestAppCrcL))
        {
            /* Sector signature matches - Passed */
            APP_PRINT(APP_DBG, "%s: Found regH/0x%x regL/0x%x complete\n ", __FUNCTION__,sectSignVal.regH, sectSignVal.regL);
            APP_PRINT(APP_DBG, "%s: Expected regH/0x%x regL/0x%x complete\n ", __FUNCTION__,gUdmaTestAppCrcH, gUdmaTestAppCrcL);
        }
        else
        {
            APP_PRINT(APP_LOG,"Sector signature does not match with pre-calculated value.\n");
            APP_PRINT(APP_DBG, "%s: Found regH/0x%x regL/0x%x complete\n ", __FUNCTION__,sectSignVal.regH, sectSignVal.regL);
            APP_PRINT(APP_DBG, "%s: Expected regH/0x%x regL/0x%x complete\n ", __FUNCTION__,gUdmaTestAppCrcH, gUdmaTestAppCrcL);
        }

        CRCClearIntr(gAppCrcBase, APP_CRC_CHANNEL, CRC_CHANNEL_IRQSTATUS_RAW_MAIN_ALL);
    }

    return (retVal);
}


static void App_udmaEventCb(Udma_EventHandle eventHandle,
                            uint32_t eventType,
                            void *appData)
{
    int32_t         retVal;
    CSL_UdmapTdResponse tdResp;

    if(UDMA_EVENT_TYPE_DMA_COMPLETION == eventType)
    {
        SemaphoreP_post(gUdmaAppDoneSem);
    }
    if(UDMA_EVENT_TYPE_TEARDOWN_PACKET == eventType)
    {
        /* Response received in Teardown completion queue */
        retVal = Udma_chDequeueTdResponse((Udma_ChHandle) gUdmaChObj, &tdResp);
        if(UDMA_SOK != retVal)
        {
            /* [Error] No TD response after callback!! */
        }
    }

    return;
}



uint64_t Udma_qnxVirtToPhyFxn(const void * virtAddr,
                              uint32_t chNum,
                              void *appData)
{
    off64_t    phyAddr = 0;
    uint32_t   length;



    if(appData != NULL_PTR) {
        length = (uint32_t) *((uint32_t *) appData);
    }
    else {
        APP_PRINT(APP_LOG,"%s: Must specify memory size to map\n",__FUNCTION__);
        return -1;
    }

    int     tmp_fd = -1;
    size_t  contig_len = 0;
    if (posix_mem_offset64((void *) virtAddr, length, &phyAddr, &contig_len, &tmp_fd) != 0)
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
    uint32_t length = 0;
    int prot = PROT_READ | PROT_WRITE;
    int flags;

    if(appData != NULL_PTR) {
        length = (uint32_t) *((uint32_t *) appData);
    }
    else {
        APP_PRINT(APP_LOG,"%s: Must specify memory size to map\n",__FUNCTION__);
        return NULL;
    }

    flags =  MAP_SHARED;
    temp = mmap64(0, length, prot, flags, memFd, phyAddr);
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
#if defined (BUILD_MCU1_0) || defined (BUILD_MCU1_1)
    instId = UDMA_INST_ID_MCU_0;
#else
#if defined (SOC_AM62X) || defined(SOC_AM62A) || defined(SOC_J722S)
    instId = UDMA_INST_ID_BCDMA_0;
#else
    instId = UDMA_INST_ID_MAIN_0;
#endif //#if defined (SOC_AM62X) || defined(SOC_AM62A) || defined(SOC_J722S)
#endif //#if defined (BUILD_MCU1_0) || defined (BUILD_MCU1_1)

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
        APP_PRINT(APP_LOG, "[Error] UDMA init failed!!\n");
    }

    return (retVal);
}

static int32_t App_deinit(Udma_DrvHandle drvHandle)
{
    int32_t     retVal = UDMA_SOK;

    retVal = Udma_deinit(drvHandle);
    if(UDMA_SOK != retVal)
    {
        APP_PRINT(APP_LOG, "[Error] UDMA deinit failed\n");
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
        APP_PRINT(APP_LOG, "[Error] Sem create failed!!\n");
        retVal = UDMA_EFAIL;
    }
#endif

    App_crcInit();

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

        /* Open channel for block copy */
        retVal = Udma_chOpen(drvHandle, chHandle, chType, &chPrms);
        if(UDMA_SOK != retVal)
        {
            APP_PRINT(APP_LOG, "[Error] UDMA channel open failed!!\n");
        }
    }

    if(UDMA_SOK == retVal)
    {


        /* Config TX channel */
        UdmaChTxPrms_init(&txPrms, chType);

        APP_PRINT(APP_DBG,"%s: ChannelType = %d, BurstSize = %d\n",__FUNCTION__,chType,txPrms.burstSize);

        retVal = Udma_chConfigTx(chHandle, &txPrms);
        if(UDMA_SOK != retVal)
        {
            APP_PRINT(APP_LOG,"[Error] UDMA TX channel config failed!!\n");
        }
    }
    APP_PRINT(APP_DBG, "%s: Udma_chConfigTx complete\n ", __FUNCTION__);

    if(UDMA_SOK == retVal)
    {

        /* Config RX channel - which is implicitly paired to TX channel in
         * block copy mode */
        UdmaChRxPrms_init(&rxPrms, chType);

        retVal = Udma_chConfigRx(chHandle, &rxPrms);
        if(UDMA_SOK != retVal)
        {
            APP_PRINT(APP_LOG,"[Error] UDMA RX channel config failed!!\n");
        }

    }
    APP_PRINT(APP_DBG, "%s: Udma_chConfigRx complete\n ", __FUNCTION__);

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
        eventPrms.eventCb           = &App_udmaEventCb;
        retVal = Udma_eventRegister(drvHandle, eventHandle, &eventPrms);
        if(UDMA_SOK != retVal)
        {
            APP_PRINT(APP_LOG,"[Error] UDMA CQ event register failed!!\n");
        }
    }

    if(UDMA_SOK == retVal)
    {
        /* Register teardown ring completion callback */
        eventHandle = (Udma_EventHandle) gUdmaTdCqEventObj;
        UdmaEventPrms_init(&eventPrms);
        eventPrms.eventType         = UDMA_EVENT_TYPE_TEARDOWN_PACKET;
        eventPrms.eventMode         = UDMA_EVENT_MODE_SHARED;
        eventPrms.chHandle          = chHandle;
        eventPrms.masterEventHandle = Udma_eventGetGlobalHandle(drvHandle);
        eventPrms.eventCb           = &App_udmaEventCb;
        retVal = Udma_eventRegister(drvHandle, eventHandle, &eventPrms);
        if(UDMA_SOK != retVal)
        {
            APP_PRINT(APP_LOG,"[Error] UDMA Teardown CQ event register failed!!\n");
        }
    }
#endif
    if(UDMA_SOK == retVal)
    {
        /* Channel enable */
        retVal = Udma_chEnable(chHandle);
        if(UDMA_SOK != retVal)
        {
            APP_PRINT(APP_LOG,"[Error] UDMA channel enable failed!!\n");
        }
    }
    APP_PRINT(APP_DBG, "%s: Udma_chEnable complete\n ", __FUNCTION__);
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
        APP_PRINT(APP_LOG,"[Error] UDMA channel disable failed!!\n");
    }

#if defined (UDMA_TEST_INTR)
    /* Unregister all events */
    eventHandle = gUdmaTdCqEventObj;
    retVal += Udma_eventUnRegister(eventHandle);
    eventHandle = gUdmaCqEventObj;
    retVal += Udma_eventUnRegister(eventHandle);
    if(UDMA_SOK != retVal)
    {
        APP_PRINT(APP_LOG,"[Error] UDMA event unregister failed!!\n");
    }
#endif

    /* Flush any pending request from the free queue */
    while(1)
    {
        APP_PRINT(APP_LOG,"UDMA channel flushing requests from free queue!!\n");
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
        APP_PRINT(APP_LOG,"[Error] UDMA channel close failed!!\n");
    }

#if defined (UDMA_TEST_INTR)
    if(gUdmaAppDoneSem != NULL)
    {
        SemaphoreP_delete(gUdmaAppDoneSem);
        gUdmaAppDoneSem = NULL;
    }
#endif

    return (retVal);
}

static void App_udmaTrpdInit(Udma_ChHandle chHandle,
                             uint8_t *pTrpdMem,
                             const void *srcBuf,
                             const void *destBuf,
                             uint32_t length)
{
    CSL_UdmapCppi5TRPD *pTrpd = (CSL_UdmapCppi5TRPD *) pTrpdMem;
    CSL_UdmapTR15 *pTr = (CSL_UdmapTR15 *)(pTrpdMem + sizeof(CSL_UdmapTR15));
    uint32_t *pTrResp = (uint32_t *) (pTrpdMem + (sizeof(CSL_UdmapTR15) * 2U));
    uint32_t cqRingNum = Udma_chGetCqRingNum(chHandle);
    uint32_t cCnt;

    uint32_t tr15_size = 32; //sizeof(CSL_UdmapTR15) ;
    uint32_t destSize = sizeof(crcSignatureRegAddr_t);
    uint64_t srcPhysAddr = 0;
    uint64_t dstPhysAddr = 0;



    /* Make TRPD */
    UdmaUtils_makeTrpd(pTrpd, UDMA_TR_TYPE_15, 1U, cqRingNum);

    /* Setup TR */
   cCnt = 1;
   while ((length / cCnt) > 0x7FFFU)
   {
       cCnt = cCnt * 2;
   }


    srcPhysAddr = (uint64_t) Udma_qnxVirtToPhyFxn(srcBuf, 0,  &tr15_size);
    dstPhysAddr = (uint64_t) Udma_qnxVirtToPhyFxn(destBuf, 0, &destSize);
    APP_PRINT(APP_DBG,"%s: srcPhysAdrr/%lx dstPhysAddr %lx\n",__FUNCTION__, srcPhysAddr, dstPhysAddr);


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

    /* Possibly not optimal just getting it running with 128 byte chunks, to max of 8MB*/
    pTr->icnt0 = gSrcPatternSize;
    pTr->icnt1 = length / gSrcPatternSize;
    pTr->icnt2 = 1;
    pTr->icnt3 = 1;
    pTr->dim1     = pTr->icnt0;
    pTr->dim2     = (pTr->icnt0 * pTr->icnt1);
    pTr->dim3     = (pTr->icnt0 * pTr->icnt1 * pTr->icnt2);
    pTr->addr     = srcPhysAddr;
    pTr->fmtflags = 0x00000000U;        /* Linear addressing, 1 byte per elem.
                                           Replace with CSL-FL API */
    APP_PRINT(APP_DBG,"length/%d icnt0/%d icnt1/%d icnt2/%d icnt3/%d\n", length, pTr->icnt0, pTr->icnt1, pTr->icnt2, pTr->icnt3);

    pTr->dicnt0   = gSrcPatternSize; //APP_CRC_PATTERN_SIZE; // TODO Pattern SIZE
    pTr->dicnt1   = (length / pTr->dicnt0) / cCnt;
    pTr->dicnt2   = cCnt;
    pTr->dicnt3   = 1U;
    pTr->ddim1    = 0U;
    pTr->ddim2    = 0U;
    pTr->ddim3    = 0U;
    pTr->daddr    = dstPhysAddr;
    APP_PRINT(APP_DBG,"length/%d dicnt0/%d dicnt1/%d dicnt2/%d dicnt3/%d\n", length, pTr->dicnt0, pTr->dicnt1, pTr->dicnt2, pTr->dicnt3);

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


static void App_crcInit(void)
{
    /* Configure CRC channel */
    CRCInitialize(
            gAppCrcBase,
            APP_CRC_CHANNEL,
            APP_CRC_WATCHDOG_PRELOAD_VAL,
            APP_CRC_BLOCK_PRELOAD_VAL);

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
                *paddr = (uint32_t) offset;
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
    APP_PRINT(APP_LOG, "    p          - Optionally specify pattern size, dflt 4       \n");
    APP_PRINT(APP_LOG,"                 Used for alignment as well                     \n");
    APP_PRINT(APP_LOG, "    v          - Be verbose                                    \n");
    APP_PRINT(APP_LOG, "    s          - Optionally specify source address in hex      \n");
    APP_PRINT(APP_LOG, "    n          - Optionally disable cache for memory           \n");
    APP_PRINT(APP_LOG, "    m<string>  - Memory region to allocate from                \n");
    APP_PRINT(APP_LOG, "                 default is 'ram'                              \n");
    APP_PRINT(APP_LOG, "    t          - Optionally print transfer time                \n");
    APP_PRINT(APP_LOG, "    b          - Optionally specify buffer size in bytes       \n");
    APP_PRINT(APP_LOG, "                 default is 1MB, must be 4K aligned            \n");
    APP_PRINT(APP_LOG, "    l          - Optionally specify pre-calc CRC lower 32 bits \n");
    APP_PRINT(APP_LOG, "                 default is for 1MB buffer                     \n");
    APP_PRINT(APP_LOG, "    h          - Optionally specify pre-calc CRC upper 32 bits \n");
    APP_PRINT(APP_LOG, "                 default is for 1MB buffer                     \n");
    APP_PRINT(APP_LOG, "    q          - Optionally enable trace event logs            \n");
    APP_PRINT(APP_LOG, "    f<file>    - Optionally dump source buffer to file         \n");
    APP_PRINT(APP_LOG, "    i          - Number of times to trigger the same udma      \n");
    APP_PRINT(APP_LOG, "                 descriptor / transaction                      \n");
    APP_PRINT(APP_LOG, "                                                               \n");
    APP_PRINT(APP_LOG, "Example 1:                                                     \n");
    APP_PRINT(APP_LOG, " Verbosity enabled, 4 byte pattern, 20000 bytes                \n");
    APP_PRINT(APP_LOG, " pre-calculated CRC specified                                  \n");
    APP_PRINT(APP_LOG, "                                                               \n");
    APP_PRINT(APP_LOG, " udma_crc_testapp -p4 -v -b20000 -l0x83A8C73A -h0x18633761     \n");
    APP_PRINT(APP_LOG, "                                                               \n");
    APP_PRINT(APP_LOG, "Example 2:                                                     \n");
    APP_PRINT(APP_LOG, "  128 byte pattern, 1MB, pre-calculated CRC specified          \n");
    APP_PRINT(APP_LOG, "  100 iterations, measure time                                 \n");
    APP_PRINT(APP_LOG, "                                                               \n");
    APP_PRINT(APP_LOG, "  udma_crc_testapp -p128 -b1048576 -h0xa3e4158b -l0xe4985934 -i100 -t  \n");
    exit(0);
}

void parseCmdLine (int argc, char *argv[])
{
  int c;

  while ((c = getopt (argc, argv, "p:b:ns:tvl:h:qi:f:")) != -1)
  {
    switch (c)
    {
      case 'p':
        gSrcPatternSize = atoi(optarg);
        APP_PRINT(APP_LOG, "Source buffer must align to %d bytes, maximum size is %d\n",gSrcPatternSize, 65536 * gSrcPatternSize);
        break;
      case 'f':
        gOutputToFile = 1;
        APP_PRINT(APP_LOG, "Output file is %s\n",optarg);
        strcpy(gFilename, optarg);
        break;
      case 'v':
        gVerbose = 1;
        break;
      case 's':
        gUserProvidedPhysAddr = 1;
        gUserFromPhysAddr =  strtoll(optarg, NULL, 16);
        APP_PRINT(APP_DBG, "Set From Physical Address to 0x%10lX\n",gUserFromPhysAddr);
        break;
      case 'l':
        gUdmaTestAppCrcL =  strtoll(optarg, NULL, 16);
        APP_PRINT(APP_DBG, "Lower 32 bits of Pre-calculated crc signature value for given data pattern 0x%08X\n",gUdmaTestAppCrcL);
        break;
      case 'h':
        gUdmaTestAppCrcH =  strtoll(optarg, NULL, 16);
        APP_PRINT(APP_DBG, "Upper 32 bits of Pre-calculated crc signature value for given data pattern 0x%08X\n",gUdmaTestAppCrcH);
        break;
      case 'i':
        gIters = atoi(optarg);
        APP_PRINT(APP_LOG, "Will run '%d' iterations of UDMA memory copy\n",gIters);
        break;
      case 'n':
        gNoCache =  1;
        APP_PRINT(APP_DBG, "Memory will be mapped as PROT_NOCACHE\n");
        break;
      case 'm':
        strcpy(memStr, optarg);
        APP_PRINT(APP_DBG, "User Specified '%s' for memory\n",memStr);
        break;
      case 't':
        gMeasureTime =  1;
        APP_PRINT(APP_DBG, "Setup and transfer time will be measured\n");
        break;
      case 'q':
        gTrace =  1;
        APP_PRINT(APP_LOG, "Tracing has been activated\n");
        break;
      case 'b':
        gUdmaTestAppNumBytes = atol(optarg);
        gUdmaTestAppNumBytesAlign = gUdmaTestAppNumBytes; //((gUdmaTestAppNumBytes + UDMA_CACHELINE_ALIGNMENT) & ~(UDMA_CACHELINE_ALIGNMENT - 1U));
        APP_PRINT(APP_DBG, "User specified %d bytes, after alignment %d bytes\n", gUdmaTestAppNumBytes, gUdmaTestAppNumBytesAlign );
        if(gUdmaTestAppNumBytes % gSrcPatternSize != 0)
        {
            APP_PRINT(APP_DBG, "User specified %d bytes, %d pattern size,  alignment may be off \n", gUdmaTestAppNumBytes, gSrcPatternSize);
        }
        break;
      default:
        printUsage();
    }
  }
}


int main (int argc, char **argv)
{
    int32_t retVal = UDMA_SOK;

    strcpy(memStr, "ram");
    parseCmdLine(argc, argv);

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
    gUdmaTestSrcBuf = (uintptr_t) App_alloc(gUdmaTestAppNumBytesAlign,  &gUserFromPhysAddr, gNoCache);
    if (gUdmaTestSrcBuf == 0) //|| (gUdmaTestDestBuf == MAP_FAILED))
    {
       APP_PRINT(APP_LOG, "%s: Allocation of buffers failed\n",__func__);
       exit(-1);
    }

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

    /* Create mapping to MCRC Base Address */
    gAppCrcBase = (uintptr_t) mmap_device_memory(0,gAppCrcSize, PROT_READ|PROT_WRITE|PROT_NOCACHE, 0, APP_CRC_BASE);
    if(gAppCrcBase == 0)
    {
        APP_PRINT(APP_LOG,"%s: Memory map of CRC Base failed\n",__func__);
        exit(-1);
    }
    APP_PRINT(APP_DBG, "%s: gAppCrcBase phys/0x%lx virt/0x%lx size/%d\n",__FUNCTION__,
            APP_CRC_BASE, gAppCrcBase, gAppCrcSize);

    /* Get IO priveleges */
    if (ThreadCtl(_NTO_TCTL_IO_PRIV, NULL) == -1) {
        perror("ThreadCtl(_NTO_TCTL_IO");
        return 1;
    }

    retVal = Udma_crcTest();

    return retVal;
}

