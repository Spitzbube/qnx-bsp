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
 *  \file enetlld_if_memutils.h
 *
 *  \brief Common ENET LLD mem utility header file
 */

#ifndef ENETLLD_IF_MEMUTILS_H_
#define ENETLLD_IF_MEMUTILS_H_

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <hw/inout.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <hw/nicinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */


/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/*! Ring memory allocation function  */
uint8_t *EnetIfMem_allocRingMemFxn(void *appPriv,
                                    uint32_t numRingEle,
                                    uint32_t alignSize);
/*! Ring memory free function  */
void EnetIfMem_freeRingMemFxn(void *appPriv,
                               void *ringMemPtr,
                               uint32_t numRingEle);
/*! DMA packet allocation function  */
EnetUdma_DmaDesc *EnetIfMem_allocDmaDescFxn(void *appPriv,
                                            uint32_t alignSize);
/*! DMA packet free function  */
void EnetIfMem_freeDmaDescFxn(void *appPriv,
                               EnetUdma_DmaDesc *dmaDescPtr);

/*! Initialize EnetIf memutils module
 * Note - This function should be called after Enet is opened as it uses EnetIf_Q
 * functions */
int32_t EnetIfMem_init(void);
/*! Mem utils deinit  */
void EnetIfMem_deInit(void);

/*! Mem alloc func  */
void *cpsw_alloc(size_t size, paddr64_t *paddr);
/*! Mem free func  */
void cpsw_free(void *addr, size_t size);

/* ========================================================================== */
/*                       Static Function Definitions                          */
/* ========================================================================== */

/* None */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef ENETLLD_IF_MEMUTILS_H_ */
