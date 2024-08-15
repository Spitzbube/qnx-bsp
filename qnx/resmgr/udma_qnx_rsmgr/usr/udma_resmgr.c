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
 *  \file udma_resmgr.c
 *
 *  \brief File containing the UDMA resource manager APIs.
 *
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "tiudma_mgr.h"
#include "udma_resmgr.h"

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
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_UdmaFd = -1;
static int g_ref_cnt = 0;

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

int32_t Udma_resmgr_open(Udma_DrvHandle drvHandle)
{
    int32_t retVal = UDMA_SOK;

    pthread_mutex_lock(&g_mutex);
    if (g_ref_cnt++ == 0)
    {
        Udma_printf(drvHandle, "Opening resmgr!!!");
        g_UdmaFd = open(TIUDMA_DEVICE_NAME, O_SYNC | O_RDWR);
        if (g_UdmaFd < 0)
        {
            g_ref_cnt--;
            retVal = UDMA_EFAIL;
        }
        Udma_printf(drvHandle, "Opened resmgr fd=%d!!!", g_UdmaFd);
    }

    pthread_mutex_unlock(&g_mutex);
    return retVal;
}

int32_t Udma_resmgr_close(Udma_DrvHandle drvHandle)
{
    int32_t  retVal = UDMA_SOK;

    pthread_mutex_lock(&g_mutex);
    if (--g_ref_cnt == 0)
    {
        Udma_printf(drvHandle, "Closing resmgr fd=%d!!!", g_UdmaFd);
        close(g_UdmaFd);
        g_UdmaFd = -1;
    }

    pthread_mutex_unlock(&g_mutex);
    return (retVal);
}

/* Called from udma-lld function Udma_printf */
void Udma_resmgr_print(const char *str)
{
    slogf(_SLOG_SETCODE(_SLOGC_PRIVATE_START, 56), _SLOG_INFO, str);
}

uint32_t Udma_resmgr_rmAllocBlkCopyCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle)
{
    uint32_t chNum = UDMA_DMA_CH_INVALID;
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.blkcopy.preferredChNum = preferredChNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_BLKCOPYCH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            chNum = cargs.args.blkcopy.chNum;
        }
    }

    return chNum;
}

void Udma_resmgr_rmFreeBlkCopyCh(uint32_t chNum, Udma_DrvHandle drvHandle)
{
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.blkcopy.chNum = chNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_BLKCOPYCH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return;
}

uint32_t Udma_resmgr_rmAllocBlkCopyHcCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle)
{
    uint32_t chNum = UDMA_DMA_CH_INVALID;
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.instId = drvHandle->initPrms.instId;
        cargs.args.blkcopyhc.preferredChNum = preferredChNum;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_BLKCOPYHCCH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            chNum = cargs.args.blkcopyhc.chNum;
        }
    }

    return chNum;
}

void Udma_resmgr_rmFreeBlkCopyHcCh(uint32_t chNum, Udma_DrvHandle drvHandle)
{
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.instId = drvHandle->initPrms.instId;
        cargs.args.blkcopyhc.chNum = chNum;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_BLKCOPYHCCH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return;
}

uint32_t Udma_resmgr_rmAllocBlkCopyUhcCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle)
{
    uint32_t chNum = UDMA_DMA_CH_INVALID;
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.instId = drvHandle->initPrms.instId;
        cargs.args.blkcopyuhc.preferredChNum = preferredChNum;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_BLKCOPYUHCCH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            chNum = cargs.args.blkcopyuhc.chNum;
        }
    }

    return chNum;
}

void Udma_resmgr_rmFreeBlkCopyUhcCh(uint32_t chNum, Udma_DrvHandle drvHandle)
{
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.instId = drvHandle->initPrms.instId;
        cargs.args.blkcopyuhc.chNum = chNum;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_BLKCOPYUHCCH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return;
}

uint32_t Udma_resmgr_rmAllocTxCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle)
{
    uint32_t chNum = UDMA_DMA_CH_INVALID;
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.instId = drvHandle->initPrms.instId;
        cargs.args.tx.preferredChNum = preferredChNum;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_TXCH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            chNum = cargs.args.tx.chNum;
        }
    }

    return chNum;
}

void Udma_resmgr_rmFreeTxCh(uint32_t chNum, Udma_DrvHandle drvHandle)
{
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.tx.chNum = chNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_TXCH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return;
}

uint32_t Udma_resmgr_rmAllocRxCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle)
{
    uint32_t chNum = UDMA_DMA_CH_INVALID;
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.rx.preferredChNum = preferredChNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_RXCH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            chNum = cargs.args.rx.chNum;
        }
    }

    return chNum;
}

void Udma_resmgr_rmFreeRxCh(uint32_t chNum, Udma_DrvHandle drvHandle)
{
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.rx.chNum = chNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_RXCH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return;
}

uint32_t Udma_resmgr_rmAllocTxHcCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle)
{
    uint32_t chNum = UDMA_DMA_CH_INVALID;
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.txhc.preferredChNum = preferredChNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_TXHCCH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            chNum = cargs.args.txhc.chNum;
        }
    }

    return chNum;
}

void Udma_resmgr_rmFreeTxHcCh(uint32_t chNum, Udma_DrvHandle drvHandle)
{
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.txhc.chNum = chNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_TXHCCH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return;
}

uint32_t Udma_resmgr_rmAllocRxHcCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle)
{
    uint32_t chNum = UDMA_DMA_CH_INVALID;
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.rxhc.preferredChNum = preferredChNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_RXHCCH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            chNum = cargs.args.rxhc.chNum;
        }
    }

    return chNum;
}

void Udma_resmgr_rmFreeRxHcCh(uint32_t chNum, Udma_DrvHandle drvHandle)
{
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.rxhc.chNum = chNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_RXHCCH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return;
}

uint32_t Udma_resmgr_rmAllocTxUhcCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle)
{
    uint32_t chNum = UDMA_DMA_CH_INVALID;
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.txuhc.preferredChNum = preferredChNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_TXUHCCH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            chNum = cargs.args.txuhc.chNum;
        }
    }

    return chNum;
}

void Udma_resmgr_rmFreeTxUhcCh(uint32_t chNum, Udma_DrvHandle drvHandle)
{
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.txuhc.chNum = chNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_TXUHCCH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return;
}

uint32_t Udma_resmgr_rmAllocRxUhcCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle)
{
    uint32_t chNum = UDMA_DMA_CH_INVALID;
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.rxuhc.preferredChNum = preferredChNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_RXUHCCH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            chNum = cargs.args.rxuhc.chNum;
        }
    }

    return chNum;
}

void Udma_resmgr_rmFreeRxUhcCh(uint32_t chNum, Udma_DrvHandle drvHandle)
{
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.rxuhc.chNum = chNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_RXUHCCH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return;
}

#if (UDMA_NUM_MAPPED_TX_GROUP > 0)
uint32_t Udma_resmgr_rmAllocMappedTxCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle, const uint32_t mappedChGrp)
{
    uint32_t chNum = UDMA_DMA_CH_INVALID;
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.mappedtx.preferredChNum = preferredChNum;
        cargs.args.mappedtx.mappedChGrp = mappedChGrp;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_MAPPED_TX_CH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            chNum = cargs.args.mappedtx.chNum;
        }
    }

    return chNum;
}

uint32_t Udma_resmgr_rmFreeMappedTxCh(uint32_t txChNum, Udma_DrvHandle drvHandle, const uint32_t mappedChGrp)
{
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.mappedtx.mappedChGrp = mappedChGrp;
        cargs.args.mappedtx.chNum = txChNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_MAPPED_TX_CH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return retVal;
}
#endif

#if (UDMA_NUM_MAPPED_RX_GROUP > 0)
uint32_t Udma_resmgr_rmAllocMappedRxCh(uint32_t preferredChNum, Udma_DrvHandle drvHandle, const uint32_t mappedChGrp)
{
    uint32_t chNum = UDMA_DMA_CH_INVALID;
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.mappedrx.preferredChNum = preferredChNum;
        cargs.args.mappedrx.mappedChGrp = mappedChGrp;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_MAPPED_RX_CH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            chNum = cargs.args.mappedrx.chNum;
        }
    }

    return chNum;
}

uint32_t Udma_resmgr_rmFreeMappedRxCh(uint32_t rxChNum, Udma_DrvHandle drvHandle, const uint32_t mappedChGrp)
{
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.mappedrx.mappedChGrp = mappedChGrp;
        cargs.args.mappedrx.chNum = rxChNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_MAPPED_RX_CH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return retVal;
}
#endif

#if (UDMA_NUM_UTC_INSTANCE > 0)
uint32_t Udma_resmgr_rmAllocExtCh(uint32_t preferredChNum,
                           Udma_DrvHandle drvHandle,
                           const Udma_UtcInstInfo *utcInfo)
{
    uint32_t chNum = UDMA_DMA_CH_INVALID;
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.ext.preferredChNum = preferredChNum;
        cargs.args.ext.utcInfo = utcInfo;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_EXTCH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            chNum = cargs.args.ext.chNum;
        }
    }

    return chNum;
}

void Udma_resmgr_rmFreeExtCh(uint32_t chNum,
                      Udma_DrvHandle drvHandle,
                      const Udma_UtcInstInfo *utcInfo)
{
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.ext.chNum = chNum;
        cargs.args.ext.utcInfo = utcInfo;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_EXTCH, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return;
}
#endif

uint16_t Udma_resmgr_rmAllocProxy(uint16_t preferredProxyNum, Udma_DrvHandle drvHandle)
{
    uint16_t proxyNum = UDMA_PROXY_INVALID;
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.proxy.preferredProxyNum = preferredProxyNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_PROXY, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            proxyNum = cargs.args.proxy.proxyNum;
        }
    }

    return proxyNum;
}

void Udma_resmgr_rmFreeProxy(uint16_t proxyNum, Udma_DrvHandle drvHandle)
{
    int32_t retVal = UDMA_EFAIL;
    TIUDMA_CmdArgs cargs;

    if (g_UdmaFd != -1)
    {
        cargs.args.proxy.proxyNum = proxyNum;
        cargs.instId = drvHandle->initPrms.instId;
        
        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_PROXY, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return;
}

uint16_t Udma_resmgr_rmAllocFreeRing(Udma_DrvHandle drvHandle)
{
    uint16_t            ringNum = UDMA_RING_INVALID;
    TIUDMA_CmdArgs cargs;
    int32_t  retVal = UDMA_SOK;

    if (g_UdmaFd != -1)
    {
        cargs.instId = drvHandle->initPrms.instId;
        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_FREERING, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            ringNum = cargs.args.freering.ringNum;
        }
    }

    return ringNum;
}

#if (UDMA_SOC_CFG_PKTDMA_PRESENT == 1)
uint32_t Udma_resmgr_rmAllocMappedRing(Udma_DrvHandle drvHandle, uint32_t mappdRingGrp, uint32_t mappedChNum)
{
    uint32_t            ringNum = UDMA_RING_INVALID;
    TIUDMA_CmdArgs cargs;
    int32_t  retVal = UDMA_SOK;

    if (g_UdmaFd != -1)
    {
        cargs.instId = drvHandle->initPrms.instId;
        cargs.args.allocmapring.mappdRingGrp = mappdRingGrp;
        cargs.args.allocmapring.mappedChNum = mappedChNum;
        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_MAPPEDRING, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            ringNum = cargs.args.allocmapring.ringNum;
        }
    }

    return ringNum;
}

void Udma_resmgr_rmFreeMappedRing(uint32_t ringNum, Udma_DrvHandle drvHandle, uint32_t mappdRingGrp, uint32_t mappedChNum)
{
    TIUDMA_CmdArgs cargs;
    int32_t  retVal = UDMA_SOK;

    if (g_UdmaFd != -1)
    {
        cargs.args.freemapring.ringNum = ringNum;
        cargs.args.freemapring.mappdRingGrp = mappdRingGrp;
        cargs.args.freemapring.mappedChNum = mappedChNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_MAPPEDRING, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return;
}
#endif //(UDMA_SOC_CFG_PKTDMA_PRESENT == 1)

void Udma_resmgr_rmFreeFreeRing(uint16_t ringNum, Udma_DrvHandle drvHandle)
{
    TIUDMA_CmdArgs cargs;
    int32_t  retVal = UDMA_SOK;

    if (g_UdmaFd != -1)
    {
        cargs.args.freering.ringNum = ringNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_FREERING, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return;
}

uint16_t Udma_resmgr_rmAllocRingMon(Udma_DrvHandle drvHandle)
{
    uint16_t            ringNum = UDMA_RING_INVALID;
    TIUDMA_CmdArgs cargs;
    int32_t  retVal = UDMA_SOK;

    if (g_UdmaFd != -1)
    {
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_RINGMON, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            ringNum = cargs.args.ringmon.ringNum;
        }
    }

    return ringNum;
}

void Udma_resmgr_rmFreeRingMon(uint16_t ringNum, Udma_DrvHandle drvHandle)
{
    TIUDMA_CmdArgs cargs;
    int32_t  retVal = UDMA_SOK;

    if (g_UdmaFd != -1)
    {
        cargs.args.ringmon.ringNum = ringNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_RINGMON, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return;
}

uint32_t Udma_resmgr_rmAllocVintr(Udma_DrvHandle drvHandle)
{
    uint32_t            vintrNum = UDMA_EVENT_INVALID;
    TIUDMA_CmdArgs cargs;
    int32_t  retVal = UDMA_SOK;

    if (g_UdmaFd != -1)
    {
        cargs.instId = drvHandle->initPrms.instId;
        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_VINTR, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            vintrNum = cargs.args.vintr.vintrNum;
        }
    }

    return vintrNum;
}

void Udma_resmgr_rmFreeVintr(uint32_t vintrNum, Udma_DrvHandle drvHandle)
{
    TIUDMA_CmdArgs cargs;
    int32_t  retVal = UDMA_SOK;

    if (g_UdmaFd != -1)
    {
        cargs.args.vintr.vintrNum = vintrNum;
        cargs.instId = drvHandle->initPrms.instId;
        
        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_VINTR, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return;
}

uint32_t Udma_resmgr_rmAllocIrIntr(uint32_t preferredIrIntrNum, Udma_DrvHandle drvHandle)
{
    uint32_t            irIntrNum = UDMA_INTR_INVALID;
    TIUDMA_CmdArgs cargs;
    int32_t  retVal = UDMA_SOK;

    if (g_UdmaFd != -1)
    {
        cargs.args.irintr.preferredIrIntrNum = preferredIrIntrNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_IRINTR, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            irIntrNum = cargs.args.irintr.irIntrNum;
        }
    }

    return irIntrNum;
}

void Udma_resmgr_rmFreeIrIntr(uint32_t irIntrNum, Udma_DrvHandle drvHandle)
{
    TIUDMA_CmdArgs cargs;
    int32_t  retVal = UDMA_SOK;

    if (g_UdmaFd != -1)
    {
        cargs.args.irintr.irIntrNum = irIntrNum;
        cargs.instId = drvHandle->initPrms.instId;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_IRINTR, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return;
}

uint32_t Udma_resmgr_rmAllocEvent(Udma_DrvHandle drvHandle)
{
    uint32_t globalEvent = UDMA_EVENT_INVALID;
    TIUDMA_CmdArgs cargs;
    int32_t  retVal = UDMA_SOK;

    if (g_UdmaFd != -1)
    {
        cargs.instId = drvHandle->initPrms.instId;
        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_EVENT, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            globalEvent = cargs.args.event.globalEvent;
        }
    }

    return globalEvent;
}

void Udma_resmgr_rmFreeEvent(uint32_t globalEvent, Udma_DrvHandle drvHandle)
{
    TIUDMA_CmdArgs cargs;
    int32_t  retVal = UDMA_SOK;

    if (g_UdmaFd != -1)
    {
        cargs.instId = drvHandle->initPrms.instId;
        cargs.args.event.globalEvent = globalEvent;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_EVENT, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return;
}

uint32_t Udma_resmgr_rmAllocVintrBit(Udma_DrvHandle drvHandle, Udma_EventHandle eventHandle)
{
    uint32_t vintrBitNum = UDMA_EVENT_INVALID;
    TIUDMA_CmdArgs cargs;
    int32_t  retVal = UDMA_SOK;

    if (g_UdmaFd != -1)
    {
        cargs.instId = drvHandle->initPrms.instId;
        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_VINTRBIT, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            vintrBitNum = cargs.args.vintrbit.vintrBitNum;
        }
    }

    return vintrBitNum;
}

void Udma_resmgr_rmFreeVintrBit(uint32_t vintrBitNum,
                         Udma_DrvHandle drvHandle,
                         Udma_EventHandle eventHandle)
{
    TIUDMA_CmdArgs cargs;
    int32_t  retVal = UDMA_SOK;

    if (g_UdmaFd != -1)
    {
        cargs.instId = drvHandle->initPrms.instId;
        cargs.args.vintrbit.vintrBitNum = vintrBitNum;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_VINTRBIT, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return;
}

uint32_t Udma_resmgr_rmTranslateIrOutput(Udma_DrvHandle drvHandle, uint32_t irIntrNum)
{
    TIUDMA_CmdArgs cargs;
    int32_t  retVal = UDMA_SOK;
    uint32_t coreIntrNum = UDMA_INTR_INVALID;

    if (g_UdmaFd != -1)
    {
        cargs.instId = drvHandle->initPrms.instId;
        cargs.args.irOutput.irIntrNum = irIntrNum;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_TRANSLATE_IR_OUTPUT, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            coreIntrNum = cargs.args.irOutput.coreIntrNum;
        }
    }

    return coreIntrNum;
}

uint32_t Udma_resmgr_rmTranslateCoreIntrInput(Udma_DrvHandle drvHandle, uint32_t coreIntrNum)
{
    TIUDMA_CmdArgs cargs;
    int32_t  retVal = UDMA_SOK;
    uint32_t irIntrNum = UDMA_INTR_INVALID;

    if (g_UdmaFd != -1)
    {
        cargs.instId = drvHandle->initPrms.instId;
        cargs.args.coreIntInput.coreIntrNum = coreIntrNum;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_TRANSLATE_CORE_INTR_INPUT, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            irIntrNum = cargs.args.coreIntInput.irIntrNum;
        }
    }

    return irIntrNum;
}

uint32_t Udma_resmgr_rmAllocflow(uint32_t flowCnt, Udma_DrvHandle drvHandle)
{
    TIUDMA_CmdArgs cargs;
    int32_t  retVal = UDMA_SOK;
    uint32_t flowStart = UDMA_FLOW_INVALID;

    if (g_UdmaFd != -1)
    {
        cargs.instId = drvHandle->initPrms.instId;
        cargs.args.flow.flowCnt = flowCnt;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_ALLOC_FLOW, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
        else
        {
            flowStart = cargs.args.flow.flowStart;
        }
    }

    return flowStart;
}

void Udma_resmgr_rmFreeflow(uint32_t flowStart, uint32_t flowCnt, Udma_DrvHandle drvHandle)
{
    TIUDMA_CmdArgs cargs;
    int32_t  retVal = UDMA_SOK;

    if (g_UdmaFd != -1)
    {
        cargs.instId = drvHandle->initPrms.instId;
        cargs.args.flow.flowCnt = flowCnt;
        cargs.args.flow.flowStart = flowStart;

        retVal = devctl(g_UdmaFd, DCMD_TIUDMA_FREE_FLOW, &cargs,
                        sizeof(TIUDMA_CmdArgs), NULL);

        if (UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "%s: Error ret=%d!!!", __func__, retVal);
        }
    }

    return;
}
