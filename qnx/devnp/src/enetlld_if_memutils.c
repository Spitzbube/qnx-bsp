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

/**
 *  \file cpsw_appmem.c
 *
 *  \brief CPSW DMA memory allocation utility functions.
 *
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

/* This is needed for memset/memcpy */
#include <string.h>

#include <ti/drv/enet/enet.h>
#include <ti/drv/enet/include/core/enet_utils.h>

#include "j7_cpsw.h"
#include "enetlld_if_utils.h"
#include "enetlld_if_memutils.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */
#define ENETIF_MEMUTILS_RING_MAX_ELEM_CNT          (CPSW_MEM_TX_PKTS_MAX + \
                                                    CPSW_MEM_RX_PKTS_MAX)

#define ENETIF_MEMUTILS_NUM_DESCS ((CPSW_MEM_TX_PKTS_MAX * ENET_CFG_TX_CHANNELS_NUM) + \
                                   (CPSW_MEM_RX_PKTS_MAX * ENET_CFG_RX_FLOWS_NUM))

/* TDCQ is allocated in driver, we allocate only FQ and CQ */
#define ENETIF_MEMUTILS_NUM_RINGS_TYPES (2U)

#define ENETIF_MEMUTILS_NUM_RINGS (ENETIF_MEMUTILS_NUM_RINGS_TYPES * \
                                  (ENET_CFG_TX_CHANNELS_NUM + ENET_CFG_RX_FLOWS_NUM))

#define ENETIF_MEMUTILS_RING_MAX_SIZE \
    (ENET_UDMA_RING_MEM_SIZE * ENETIF_MEMUTILS_RING_MAX_ELEM_CNT)

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/**
 *  \brief
 */
typedef struct EnetIfMem_DmaDescMem_s
{
    /*! The node element so this packet can be added to a queue
     * Note- Keep EnetQ_Node as first member always as driver uses generic Q functions
     *       and deferences to this member */
    EnetQ_Node node;

    /*! DMA descriptor element */
    EnetUdma_DmaDesc dmaDesc
        __attribute__ ((aligned(UDMA_CACHELINE_ALIGNMENT)));

    /*! DMA descriptor state, refer to CpswUtils_DescStateMemMgr */
    uint32_t dmaDescState;
}EnetIfMem_DmaDescMem;

/**
 *  \brief
 */
typedef struct EnetIfMem_RingMem_s
{
    /*! The node element so this packet can be added to a queue
     * Note- Keep EnetQ_Node as first member always as driver uses generic Q functions
     *       and deferences to this member */
    EnetQ_Node node;

    /*! Ring memory element */
    uint8_t ringEle[ENET_UTILS_ALIGN(ENETIF_MEMUTILS_RING_MAX_SIZE, UDMA_CACHELINE_ALIGNMENT)]
        __attribute__ ((aligned(UDMA_CACHELINE_ALIGNMENT)));
}EnetIfMem_RingMem;

typedef EnetQ EnetIfMem_DmaDescMemQ;

typedef EnetQ EnetIfMem_RingMemQ;


/**
 *  \brief
 */
typedef struct EnetIfMem_MemAllocObj_s
{
    /**< DMA packet Q */
    bool memInitFlag;

    /**< DMA packet Q */
    EnetIfMem_DmaDescMemQ dmaDescFreeQ;

    /**< Ring memory Q */
    EnetIfMem_RingMemQ ringMemQ;

} EnetIfMem_MemAllocObj;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/* Cache Op flag */
extern uint32_t gCache_ops;

typedef struct EnetIfMem_Mem_s
{
    EnetIfMem_DmaDescMem gDmaDescMemArray[ENETIF_MEMUTILS_NUM_DESCS]
                    __attribute__(( aligned(UDMA_CACHELINE_ALIGNMENT)));
    EnetIfMem_RingMem gRingMemArray[ENETIF_MEMUTILS_NUM_RINGS]
                    __attribute__(( aligned(UDMA_CACHELINE_ALIGNMENT)));
}EnetIfMem_Mem;

static void *gMemBuf        = NULL;
static uint64_t gMemBufPhys  = 0;
static EnetIfMem_Mem *gMem = NULL;

/* Cpsw mem utils driver object */
static EnetIfMem_MemAllocObj gEnetIfMemObj = {.memInitFlag = false};

/* Array storing the physical address of the DMA descriptor array */
uint64_t  gDmaDescMemArrayBase = 0;
uint64_t  gDmaDescMemArrayBasePhys = 0;

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */


/*! Ring memory allocation function  */
uint8_t *EnetIfMem_allocRingMemFxn(void *appPriv,
                                                 uint32_t numRingEle,
                                                 uint32_t alignSize)
{
    uint8_t *ringMemPtr = NULL;
    EnetIfMem_RingMem *pRingMemEle;

    if (gEnetIfMemObj.memInitFlag == true)
    {
        EnetIf_assert(numRingEle <= ENETIF_MEMUTILS_RING_MAX_ELEM_CNT);
        pRingMemEle = (EnetIfMem_RingMem *)EnetQueue_deq(&gEnetIfMemObj.ringMemQ);
        if (pRingMemEle != NULL)
        {
            ringMemPtr = &pRingMemEle->ringEle[0U];

            EnetIf_assert(ENET_UTILS_IS_ALIGNED(ringMemPtr, alignSize));
        }
        else
        {
            ringMemPtr = NULL;
        }
    }

    return ringMemPtr;
}

/*! Ring memory free function  */
void EnetIfMem_freeRingMemFxn(void *appPriv,
                                            void *ringMemPtr,
                                            uint32_t numRingEle)
{
    EnetIfMem_RingMem *pRingMemEle;

    if (gEnetIfMemObj.memInitFlag == true)
    {
        pRingMemEle = container_of((const uint8_t *)ringMemPtr, EnetIfMem_RingMem, ringEle[0U]);
        /* TODO - just to get it compiling - Need to fix this after discussion with Misa/Badri */
        EnetQueue_enq(&gEnetIfMemObj.ringMemQ, &pRingMemEle->node);
    }

    return;
}

/*! DMA packet allocation function  */
EnetUdma_DmaDesc *EnetIfMem_allocDmaDescFxn(void *appPriv,
                                                          uint32_t alignSize)
{
    EnetUdma_DmaDesc *dmaDescPtr = NULL;
    EnetIfMem_DmaDescMem *pDmaDescMem;

    if (gEnetIfMemObj.memInitFlag == true)
    {
        pDmaDescMem = (EnetIfMem_DmaDescMem *)
                      EnetQueue_deq(&gEnetIfMemObj.dmaDescFreeQ);
        if (NULL != pDmaDescMem)
        {
            dmaDescPtr = &pDmaDescMem->dmaDesc;
            if (!ENET_UTILS_IS_ALIGNED(dmaDescPtr, alignSize))
            {
                EnetQueue_enq(&gEnetIfMemObj.dmaDescFreeQ,
                              &pDmaDescMem->node);
                dmaDescPtr = NULL;
            }
            else
            {
                EnetDma_checkDescState(&pDmaDescMem->dmaDescState,
                                       ENET_DESCSTATE_MEMMGR_FREE,
                                       ENET_DESCSTATE_MEMMGR_ALLOC);
            }
        }
    }

    return dmaDescPtr;
}

/*! DMA packet free function  */
void EnetIfMem_freeDmaDescFxn(void *appPriv,
                               EnetUdma_DmaDesc *dmaDescPtr)
{
    EnetIfMem_DmaDescMem *pDmaDescMem;

    if (gEnetIfMemObj.memInitFlag == true)
    {
        pDmaDescMem = container_of(dmaDescPtr, EnetIfMem_DmaDescMem, dmaDesc);
        EnetDma_checkDescState(&pDmaDescMem->dmaDescState,
                               ENET_DESCSTATE_MEMMGR_ALLOC,
                               ENET_DESCSTATE_MEMMGR_FREE);
        EnetQueue_enq(&gEnetIfMemObj.dmaDescFreeQ, &pDmaDescMem->node);
    }

    return;
}


void *cpsw_alloc(size_t size, paddr64_t *paddr)
{
    off64_t offset = 0;
    void  *buf;
    int prot  = PROT_READ | PROT_WRITE;

    buf = alloc_typed_mem(size, paddr);
    if (buf != MAP_FAILED)
        return buf;

    if (gCache_ops) {
        EnetIf_print("%s:%d: Adding PROT_NOCACHE for cpsw_alloc", __FUNCTION__,__LINE__);
        prot |= PROT_NOCACHE;
    }

    /* Will come here if the typed memory is not specified */
    buf = mmap(0, size, prot, MAP_ANON | MAP_PHYS | MAP_PRIVATE, NOFD, 0);
    if(buf != MAP_FAILED) {
        if (mem_offset64(buf, NOFD, 1, &offset, 0) == -1)
        {
            if (errno != EAGAIN) {
                EnetIf_print("%s:%d: Error: Could not obtain buffer physical address. errno=%d",
                __FUNCTION__, __LINE__, errno);
                munmap(buf, size);
                return MAP_FAILED;
            }
            else if (offset == 0) {
                EnetIf_print("%s:%d: Error: Could not obtain buffer physical address. errno=%d",
                __FUNCTION__, __LINE__, errno);
                munmap(buf, size);
                return MAP_FAILED;
            }
        }
        if (paddr)
        {
            *paddr = (uintptr_t) offset;
        }
    }
    EnetIf_print("%s:%d: Alloc successfull; Virt: 0x%lx, Phys: 0x%lx",
                __FUNCTION__, __LINE__, buf, offset);
    return buf;
}

void cpsw_free(void *addr, size_t size)
{
    munmap(addr, size);
}

#ifdef DEBUG_MODE
#include <assert.h>
#endif

void error_log(const char *str, const char *fileName, int32_t lineNum)
{
    EnetIf_print("\nAssertion @ Line: %d in %s: %s : failed !!!\n",
                         lineNum, fileName, str);
#ifdef DEBUG_MODE
    assert(0);
#endif
}

int32_t EnetIfMem_init(void)
{
    uint32_t i;
    int32_t retVal = ENET_SOK;
    uint32_t alignSize = UDMA_CACHELINE_ALIGNMENT;
    uint32_t alignment_offset = 0;

    gMemBuf = cpsw_alloc(sizeof(EnetIfMem_Mem) + UDMA_CACHELINE_ALIGNMENT,
                         (paddr64_t *)&gMemBufPhys);
    if (gMemBuf == MAP_FAILED)
    {
        EnetIf_print("%%s:%d: Error: alloc fail ",__FUNCTION__, __LINE__);
        return ENET_EFAIL;
    }

    if (!ENET_UTILS_IS_ALIGNED(gMemBuf, UDMA_CACHELINE_ALIGNMENT))
    {
        EnetIf_print("%s:%d: Error: addr=0x%lx is not aligned",
            __FUNCTION__, __LINE__, (uintptr_t)gMemBuf);
        gMem = (EnetIfMem_Mem *)(((uintptr_t)gMemBuf + UDMA_CACHELINE_ALIGNMENT) & ~(UDMA_CACHELINE_ALIGNMENT-1));
        alignment_offset = (uintptr_t)gMem - (uintptr_t)gMemBuf;
        gMemBufPhys += alignment_offset;
    }
    else
    {
        EnetIf_print("%s: addr=0x%lx is  aligned", __FUNCTION__, (uintptr_t)gMemBuf);
        gMem = gMemBuf;
    }

    EnetIf_print("%s: addr=0x%lx, size=0x%x, gMem=0x%lx, gMemBufPhys=0x%lx ",
        __FUNCTION__, (uintptr_t)gMemBuf, (unsigned)sizeof(EnetIfMem_Mem), (uintptr_t)gMem, (uintptr_t)gMemBufPhys);
    EnetIf_print("%s: gDmaDescMemArray=0x%lx, size=0x%x ",
        __FUNCTION__, (uintptr_t)&gMem->gDmaDescMemArray,(unsigned)sizeof(gMem->gDmaDescMemArray));
    EnetIf_print("%s: gRingMemArray=0x%lx, size=0x%x ",
        __FUNCTION__, (uintptr_t)&gMem->gRingMemArray,(unsigned)sizeof(gMem->gRingMemArray));

    if (gEnetIfMemObj.memInitFlag == false)
    {
        memset(&gEnetIfMemObj, 0U, sizeof(EnetIfMem_MemAllocObj));
        memset(gMem, 0U, sizeof(EnetIfMem_Mem));
        gDmaDescMemArrayBase = (uint64_t)gMem->gDmaDescMemArray;
        gDmaDescMemArrayBasePhys = gMemBufPhys + ((uint64_t)gDmaDescMemArrayBase - (uint64_t)gMem);
        EnetIf_print("%s: gDmaDescMemArrayBasePhys=0x%lx, gDmaDescMemArrayBase=0x%lx",
            __FUNCTION__, gDmaDescMemArrayBasePhys,gDmaDescMemArrayBase);

        if (ENET_SOK == retVal)
        {
            EnetQueue_initQ(&gEnetIfMemObj.dmaDescFreeQ);
            for (i = 0U; i < ENETIF_MEMUTILS_NUM_DESCS; i++)
            {
                gMem->gDmaDescMemArray[i].dmaDescState = 0;
                if (!ENET_UTILS_IS_ALIGNED(&gMem->gDmaDescMemArray[i].dmaDesc, alignSize))
                {
                    EnetIf_print("%s:%d: Error: alignment issue! ",__FUNCTION__, __LINE__);
                    retVal = ENET_EFAIL;
                    break;
                }
                EnetQueue_enq(&gEnetIfMemObj.dmaDescFreeQ, &gMem->gDmaDescMemArray[i].node);
                ENET_UTILS_SET_DESC_MEMMGR_STATE(&gMem->gDmaDescMemArray[i].dmaDescState, ENET_DESCSTATE_MEMMGR_FREE);
            }
        }

        if (ENET_SOK == retVal)
        {
            EnetQueue_initQ(&gEnetIfMemObj.ringMemQ);
            for (i = 0U; i < ENETIF_MEMUTILS_NUM_RINGS; i++)
            {
                if (!ENET_UTILS_IS_ALIGNED(&gMem->gRingMemArray[i].ringEle, alignSize))
                {
                    EnetIf_print("%s:%d: Error: alignment error! ",__FUNCTION__, __LINE__);
                    retVal = ENET_EFAIL;
                    break;
                }
                EnetQueue_enq(&gEnetIfMemObj.ringMemQ, &gMem->gRingMemArray[i].node);
            }
        }

        if (ENET_SOK == retVal)
        {
            gEnetIfMemObj.memInitFlag = true;
        }
    }

    EnetIf_assert(retVal == ENET_SOK);
    return retVal;
}

void EnetIfMem_deInit(void)
{
    uint32_t i;

    if (gEnetIfMemObj.memInitFlag)
    {
        EnetIf_assert(EnetQueue_getQCount(&gEnetIfMemObj.ringMemQ) ==
                            ENETIF_MEMUTILS_NUM_RINGS);
        EnetIf_assert(EnetQueue_getQCount(&gEnetIfMemObj.dmaDescFreeQ) ==
                            ENETIF_MEMUTILS_NUM_DESCS);

        memset(&gEnetIfMemObj, 0U, sizeof(EnetIfMem_MemAllocObj));

        for (i = 0U; i < ENETIF_MEMUTILS_NUM_DESCS; i++)
        {
            EnetDma_checkDescState(&gMem->gDmaDescMemArray[i].dmaDescState,
                                   ENET_DESCSTATE_MEMMGR_FREE,
                                   ENET_DESCSTATE_MEMMGR_FREE);
        }

        // reset the memInitFlag to let the next init allocate the required memory
        gEnetIfMemObj.memInitFlag = false;
    }

    cpsw_free(gMemBuf, (sizeof(EnetIfMem_Mem) + UDMA_CACHELINE_ALIGNMENT));

}

/* ========================================================================== */
/*                          Static Function Definitions                       */
/* ========================================================================== */

/* end of file */
