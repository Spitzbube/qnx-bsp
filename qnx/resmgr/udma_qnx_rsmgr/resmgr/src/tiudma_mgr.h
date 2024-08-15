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

#ifndef _CLK_TI_UDMAMGR_H_INCLUDED
#define _CLK_TI_UDMAMGR_H_INCLUDED

#include <stdint.h>
#include "ti/drv/udma/udma.h"
#include "ti/csl/csl_types.h"

/*
 * The following devctls are used by a client application to access UDMA
 */
#include <devctl.h>

#define TIUDMA_DEVICE_NAME       "/dev/tiudma"

typedef struct TIUDMA_CmdArgs {
    union {
        struct {
            uint32_t preferredChNum;
            uint32_t chNum;
        } blkcopy;
        struct {
            uint16_t ringNum;
        } freering;
        struct {
            uint32_t vintrNum;
        } vintr;
        struct {
            uint32_t preferredIrIntrNum;
            uint32_t irIntrNum;
        } irintr;
        struct {
            uint32_t globalEvent;
        } event;
        struct {
            uint32_t vintrBitNum;
        } vintrbit;
        struct {
            uint32_t preferredChNum;
            uint32_t chNum;
        } blkcopyhc;
        struct {
            uint32_t preferredChNum;
            uint32_t chNum;
        } blkcopyuhc;
        struct {
            uint32_t preferredChNum;
            uint32_t chNum;
        } tx;
        struct {
            uint32_t preferredChNum;
            uint32_t chNum;
        } rx;
        struct {
            uint32_t preferredChNum;
            uint32_t chNum;
        } txhc;
        struct {
            uint32_t preferredChNum;
            uint32_t chNum;
        } rxhc;
        struct {
            uint32_t preferredChNum;
            uint32_t chNum;
        } txuhc;
        struct {
            uint32_t preferredChNum;
            uint32_t chNum;
        } rxuhc;
#if (UDMA_NUM_MAPPED_TX_GROUP > 0)
        struct {
            uint32_t preferredChNum;
            uint32_t chNum;
            uint32_t mappedChGrp;
        } mappedtx;
#endif
#if (UDMA_NUM_MAPPED_RX_GROUP > 0)
        struct {
            uint32_t preferredChNum;
            uint32_t chNum;
            uint32_t mappedChGrp;
        } mappedrx;
#endif
#if (UDMA_SOC_CFG_PKTDMA_PRESENT == 1)
        struct {
            uint32_t mappdRingGrp;
            uint32_t mappedChNum;
            uint32_t ringNum;
        } allocmapring;
        struct {
            uint32_t ringNum;
            uint32_t mappdRingGrp;
            uint32_t mappedChNum;
        } freemapring;
#endif
#if (UDMA_NUM_UTC_INSTANCE > 0)
        struct {
            uint32_t preferredChNum;
            uint32_t chNum;
            const Udma_UtcInstInfo *utcInfo;
        } ext;
#endif
        struct {
            uint16_t preferredProxyNum;
            uint16_t proxyNum;
        } proxy;
        struct {
            uint16_t ringNum;
        } ringmon;
        struct {
              uint32_t irIntrNum;
              uint32_t coreIntrNum;
        } irOutput;
        struct {
            uint32_t coreIntrNum;
            uint32_t irIntrNum;
        } coreIntInput;
        struct {
            uint32_t flowCnt;
            uint32_t flowStart;
        } flow;
    } args;
    uint32_t instId;
    uint32_t status;
} TIUDMA_CmdArgs;

#define DCMD_TIUDMA_ALLOC_BLKCOPYCH    __DIOTF(_DCMD_MISC,  0, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_BLKCOPYCH     __DIOTF(_DCMD_MISC,  1, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_ALLOC_FREERING     __DIOTF(_DCMD_MISC,  2, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_FREERING      __DIOTF(_DCMD_MISC,  3, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_ALLOC_VINTR        __DIOTF(_DCMD_MISC,  4, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_VINTR         __DIOTF(_DCMD_MISC,  5, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_ALLOC_IRINTR       __DIOTF(_DCMD_MISC,  6, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_IRINTR        __DIOTF(_DCMD_MISC,  7, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_ALLOC_EVENT        __DIOTF(_DCMD_MISC,  8, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_EVENT         __DIOTF(_DCMD_MISC,  9, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_ALLOC_VINTRBIT     __DIOTF(_DCMD_MISC, 10, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_VINTRBIT      __DIOTF(_DCMD_MISC, 11, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_ALLOC_BLKCOPYHCCH  __DIOTF(_DCMD_MISC, 12, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_BLKCOPYHCCH   __DIOTF(_DCMD_MISC, 13, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_ALLOC_BLKCOPYUHCCH __DIOTF(_DCMD_MISC, 14, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_BLKCOPYUHCCH  __DIOTF(_DCMD_MISC, 15, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_ALLOC_TXCH         __DIOTF(_DCMD_MISC, 16, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_TXCH          __DIOTF(_DCMD_MISC, 17, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_ALLOC_RXCH         __DIOTF(_DCMD_MISC, 18, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_RXCH          __DIOTF(_DCMD_MISC, 19, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_ALLOC_TXHCCH       __DIOTF(_DCMD_MISC, 20, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_TXHCCH        __DIOTF(_DCMD_MISC, 21, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_ALLOC_RXHCCH       __DIOTF(_DCMD_MISC, 22, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_RXHCCH        __DIOTF(_DCMD_MISC, 23, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_ALLOC_TXUHCCH      __DIOTF(_DCMD_MISC, 24, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_TXUHCCH       __DIOTF(_DCMD_MISC, 25, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_ALLOC_RXUHCCH      __DIOTF(_DCMD_MISC, 26, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_RXUHCCH       __DIOTF(_DCMD_MISC, 27, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_ALLOC_EXTCH        __DIOTF(_DCMD_MISC, 28, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_EXTCH         __DIOTF(_DCMD_MISC, 29, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_ALLOC_PROXY        __DIOTF(_DCMD_MISC, 30, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_PROXY         __DIOTF(_DCMD_MISC, 31, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_ALLOC_RINGMON      __DIOTF(_DCMD_MISC, 32, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_RINGMON       __DIOTF(_DCMD_MISC, 33, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_TRANSLATE_IR_OUTPUT         __DIOTF(_DCMD_MISC, 34, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_TRANSLATE_CORE_INTR_INPUT   __DIOTF(_DCMD_MISC, 35, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_ALLOC_FLOW         __DIOTF(_DCMD_MISC, 36, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_FLOW          __DIOTF(_DCMD_MISC, 37, TIUDMA_CmdArgs)

// Add PKTDMA devctls here. TODO:: Refactor the devctls. TX and RX can be encoded
// into a direction member and hence have a single devctl
#define DCMD_TIUDMA_ALLOC_MAPPED_TX_CH __DIOTF(_DCMD_MISC, 38, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_ALLOC_MAPPED_RX_CH __DIOTF(_DCMD_MISC, 37, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_MAPPED_TX_CH  __DIOTF(_DCMD_MISC, 39, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_MAPPED_RX_CH  __DIOTF(_DCMD_MISC, 40, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_ALLOC_MAPPEDRING   __DIOTF(_DCMD_MISC, 41, TIUDMA_CmdArgs)
#define DCMD_TIUDMA_FREE_MAPPEDRING    __DIOTF(_DCMD_MISC, 42, TIUDMA_CmdArgs)
#endif
