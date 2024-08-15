/*
 *  Copyright (c) Texas Instruments Incorporated 2020-22
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
 *  \file udma_resmgr.h
 *
 *  \brief File containing the UDMA resource manager APIs.
 *
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include <ti/drv/udma/udma.h>
#include <ti/drv/udma/src/udma_priv.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resmgr.h>
#include <sys/neutrino.h>
#include <hw/inout.h>
#include <sys/iofunc.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>
#include <errno.h>
#include <sys/procmgr.h>
#include <drvr/hwinfo.h>
#include <string.h>
#include <stdarg.h>

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */


/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */


/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

int32_t Udma_resmgr_open(Udma_DrvHandle drvHandle);

int32_t Udma_resmgr_close(Udma_DrvHandle drvHandle);

void Udma_resmgr_print(const char *str);

uint32_t Udma_resmgr_rmAllocBlkCopyCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle);

void Udma_resmgr_rmFreeBlkCopyCh(uint32_t chNum, Udma_DrvHandle drvHandle);

uint32_t Udma_resmgr_rmAllocBlkCopyHcCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle);

void Udma_resmgr_rmFreeBlkCopyHcCh(uint32_t chNum, Udma_DrvHandle drvHandle);

uint32_t Udma_resmgr_rmAllocBlkCopyUhcCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle);

void Udma_resmgr_rmFreeBlkCopyUhcCh(uint32_t chNum, Udma_DrvHandle drvHandle);

uint32_t Udma_resmgr_rmAllocTxCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle);

void Udma_resmgr_rmFreeTxCh(uint32_t chNum, Udma_DrvHandle drvHandle);

uint32_t Udma_resmgr_rmAllocRxCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle);

void Udma_resmgr_rmFreeRxCh(uint32_t chNum, Udma_DrvHandle drvHandle);

uint32_t Udma_resmgr_rmAllocTxHcCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle);

void Udma_resmgr_rmFreeTxHcCh(uint32_t chNum, Udma_DrvHandle drvHandle);

uint32_t Udma_resmgr_rmAllocRxHcCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle);

void Udma_resmgr_rmFreeRxHcCh(uint32_t chNum, Udma_DrvHandle drvHandle);

uint32_t Udma_resmgr_rmAllocTxUhcCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle);

void Udma_resmgr_rmFreeTxUhcCh(uint32_t chNum, Udma_DrvHandle drvHandle);

uint32_t Udma_resmgr_rmAllocRxUhcCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle);

void Udma_resmgr_rmFreeRxUhcCh(uint32_t chNum, Udma_DrvHandle drvHandle);

#if (UDMA_NUM_UTC_INSTANCE > 0)
uint32_t Udma_resmgr_rmAllocExtCh(uint32_t preferredChNum,
                           Udma_DrvHandle drvHandle,
                           const Udma_UtcInstInfo *utcInfo);

void Udma_resmgr_rmFreeExtCh(uint32_t chNum,
                      Udma_DrvHandle drvHandle,
                      const Udma_UtcInstInfo *utcInfo);
#endif

#if (UDMA_NUM_MAPPED_TX_GROUP > 0)
uint32_t Udma_resmgr_rmAllocMappedTxCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle, const uint32_t mappedChGrp);
uint32_t Udma_resmgr_rmFreeMappedTxCh(uint32_t txChNum, Udma_DrvHandle drvHandle, const uint32_t mappedChGrp);
#endif

#if (UDMA_NUM_MAPPED_RX_GROUP > 0)
uint32_t Udma_resmgr_rmAllocMappedRxCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle, const uint32_t mappedChGrp);
uint32_t Udma_resmgr_rmFreeMappedRxCh(uint32_t rxChNum, Udma_DrvHandle drvHandle, const uint32_t mappedChGrp);
#endif

uint16_t Udma_resmgr_rmAllocProxy(uint16_t preferredProxyNum, Udma_DrvHandle drvHandle);

void Udma_resmgr_rmFreeProxy(uint16_t proxyNum, Udma_DrvHandle drvHandle);

uint16_t Udma_resmgr_rmAllocFreeRing(Udma_DrvHandle drvHandle);

void Udma_resmgr_rmFreeFreeRing(uint16_t ringNum, Udma_DrvHandle drvHandle);

#if (UDMA_SOC_CFG_PKTDMA_PRESENT == 1)
uint32_t Udma_resmgr_rmAllocMappedRing(Udma_DrvHandle drvHandle, uint32_t mappdRingGrp, uint32_t mappedChNum);

void Udma_resmgr_rmFreeMappedRing(uint32_t ringNum, Udma_DrvHandle drvHandle, uint32_t mappdRingGrp, uint32_t mappedChNum);
#endif

uint16_t Udma_resmgr_rmAllocRingMon(Udma_DrvHandle drvHandle);

void Udma_resmgr_rmFreeRingMon(uint16_t ringNum, Udma_DrvHandle drvHandle);

uint32_t Udma_resmgr_rmAllocVintr(Udma_DrvHandle drvHandle);

void Udma_resmgr_rmFreeVintr(uint32_t vintrNum, Udma_DrvHandle drvHandle);

uint32_t Udma_resmgr_rmAllocIrIntr(uint32_t preferredIrIntrNum, Udma_DrvHandle drvHandle);

void Udma_resmgr_rmFreeIrIntr(uint32_t irIntrNum, Udma_DrvHandle drvHandle);

uint32_t Udma_resmgr_rmAllocEvent(Udma_DrvHandle drvHandle);

void Udma_resmgr_rmFreeEvent(uint32_t globalEvent, Udma_DrvHandle drvHandle);

uint32_t Udma_resmgr_rmAllocVintrBit(Udma_DrvHandle drvHandle, Udma_EventHandle eventHandle);


void Udma_resmgr_rmFreeVintrBit(uint32_t vintrBitNum,
                         Udma_DrvHandle drvHandle,
                         Udma_EventHandle eventHandle);

uint32_t Udma_resmgr_rmTranslateIrOutput(Udma_DrvHandle drvHandle, uint32_t irIntrNum);

uint32_t Udma_resmgr_rmTranslateCoreIntrInput(Udma_DrvHandle drvHandle, uint32_t coreIntrNum);

uint32_t Udma_resmgr_rmAllocflow(uint32_t flowCnt, Udma_DrvHandle drvHandle);

void  Udma_resmgr_rmFreeflow(uint32_t flowStart, uint32_t flowCnt, Udma_DrvHandle drvHandle);
