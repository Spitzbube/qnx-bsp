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

/*
 * Define THREAD_POOL_PARAM_T such that we can avoid a compiler
 * warning when we use the dispatch*() functions below
 */
#define THREAD_POOL_PARAM_T dispatch_context_t

#include <signal.h>
#include "tiudma_mgr.h"
#include "tiudma_mgr_private.h"
#include "psdkqnx_proto.h"
#include <ti/drv/sciclient/sciclient.h>
#include "ti/drv/udma/include/udma_ch.h"
#include "ti/drv/udma/src/udma_priv.h"


#define MAX_UDMA_PROXIES 3 // (4 - 1) 4 proxies assigned for A72 and 1 dedicated as global proxy.

#if defined (PSDK_QNX_ENABLE_GCOV)
    extern void __gcov_flush();
#endif

/* log level set to info by default */
int g_log_level = _SLOG_NOTICE;

/* init the module number for the qnx_logger */
int module_num = PSDKQA_SLOGC_TI_UDMA_RM;

pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static int udma_io_devctl(resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *ocb);

// Global UDMA instance Object
struct Udma_DrvObj      gUdmaDrvObj[UDMA_INST_ID_MAX + 1];

static int options(int argc, char *const argv[])
{
    int opt;
    int loglevel = _SLOG_NOTICE;
    int err = EOK;

    while ((opt = getopt(argc, argv, "v")) != -1) {
        switch (opt) {
            case 'v':
                loglevel++;
                break;
            default:
               err = EINVAL;
                break;
        }
    }
    if(loglevel > _SLOG_NOTICE)
        g_log_level = loglevel;

    return err;
}


IOFUNC_OCB_T *
ocb_calloc (resmgr_context_t * ctp, IOFUNC_ATTR_T * device)
{
    ti_udma_ocb_t *ocb = NULL;

    /* Allocate the OCB */
    ocb = (ti_udma_ocb_t *) calloc (1, sizeof (ti_udma_ocb_t));
    if (ocb == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    ocb->pid = ctp->info.pid;
    /* Initialize OCB */

    QNX_PR_INFO("%s: Called for %d", __func__, ctp->info.pid);
    return (IOFUNC_OCB_T *)(ocb);
}

void
ocb_free (IOFUNC_OCB_T * ocb)
{
    ti_udma_ocb_t *udma_ocb = (ti_udma_ocb_t *)ocb;

    if (udma_ocb) {
        QNX_PR_INFO("%s: Called for %d", __func__, udma_ocb->pid);
        free (udma_ocb);
    }
#if defined (PSDK_QNX_ENABLE_GCOV)
    // Flush coverage info
    __gcov_flush();
#endif
}

void cleanupOnSIGTERM(int signo)
{
    int32_t retVal = UDMA_SOK;

    QNX_PR_NOTICE("%s:%d: SIGTERM recieved...exiting resource manager\n", __FUNCTION__, __LINE__);
    
    retVal = Resmgr_Udma_cleanup();
    if (retVal != UDMA_SOK)
    {
        QNX_PR_NOTICE("%s:%d: Resmgr_Udma_cleanup failed!\n", __FUNCTION__, __LINE__);
    }
    else
    {
        QNX_PR_NOTICE("%s:%d: Resmgr_Udma_cleanup succeeded!\n", __FUNCTION__, __LINE__);
    }
    exit(retVal == UDMA_SOK ? 0 : -1);
}

int32_t ResMgrCheckProxyIdx(Udma_DrvHandle  drvHandle, pid_t pid)
{
    for(int i = 0; i < MAX_UDMA_PROXIES; i++ )
    {
        if(drvHandle->proxyPids[i] == pid)
            return i;
    }
    return -1;
}

int main(int argc, char *argv[])
{
    struct stat             sbuf;
    int                     id;
    resmgr_connect_funcs_t  connect_funcs;
    resmgr_io_funcs_t       io_funcs;
    dispatch_t             *dpp;
    resmgr_attr_t           rattr;
    dispatch_context_t     *ctp;
    iofunc_attr_t           ioattr;
    iofunc_mount_t          mattr;
    iofunc_funcs_t          ocb_funcs;
    thread_pool_attr_t      tattr;
    thread_pool_t          *tpool;

    signal(SIGTERM, cleanupOnSIGTERM);

    /* Only allow one instance */
    if (-1 != stat(TIUDMA_DEVICE_NAME, &sbuf)) {
        perror("UDMA resmgr already running...");
        return (-1);
    }

    /* Make sure tisci_mgr is already running */
    if (-1 == stat("/dev/tisci", &sbuf)) {
        perror("TISCI resmgr is not running... Please start it first");
        return (-1);
    }

    /* Get IO priveleges */
    if (ThreadCtl(_NTO_TCTL_IO, NULL) == -1) {
        perror("ThreadCtl(_NTO_TCTL_IO");
        return (-1);
    }

    if ((options(argc, argv)) != EOK) {
        printf ("Starting UDMA resource manager failed, invalid arguments\n");
        return (-1);
    }

    printVersion("TI UDMA ResMgr");

    if (Resmgr_Udma_setup() != 0) {
        perror("UDMA initialization failed.");
        return (-1);
    }

    /* Initialize the dispatch interface */
    dpp = dispatch_create();
    if (!dpp) {
        QNX_PR_ERR("%s: Failed to create dispatch interface", argv[0]);
        return (errno);
    }

    /* Initialize the resource manager attributes */
    memset(&rattr, 0, sizeof(rattr));
    rattr.nparts_max = 10;
    rattr.msg_max_size = 2048;

    memset (&tattr, 0x00, sizeof(thread_pool_attr_t));
    tattr.handle = dpp;
    tattr.context_alloc = dispatch_context_alloc;
    tattr.context_free = dispatch_context_free;
    tattr.block_func = dispatch_block;
    tattr.unblock_func = dispatch_unblock;
    tattr.handler_func = dispatch_handler;
    tattr.lo_water = 2;
    tattr.hi_water = 8;
    tattr.increment = 1;
    tattr.maximum = 50;

    memset (&mattr, 0, sizeof(iofunc_mount_t));
    mattr.flags = 0;
    mattr.conf = IOFUNC_PC_CHOWN_RESTRICTED | IOFUNC_PC_NO_TRUNC | IOFUNC_PC_SYNC_IO;
    mattr.dev = 0;
    mattr.funcs = &ocb_funcs;
    memset(&ocb_funcs, 0, sizeof(iofunc_funcs_t));
    ocb_funcs.nfuncs = _IOFUNC_NFUNCS;
    ocb_funcs.ocb_calloc = ocb_calloc;
    ocb_funcs.ocb_free = ocb_free;

    /* Initialize the connect functions */
    memset(&io_funcs, 0, sizeof(resmgr_io_funcs_t));
    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs,
                     _RESMGR_IO_NFUNCS, &io_funcs);
    io_funcs.devctl = udma_io_devctl;

    iofunc_attr_init(&ioattr, S_IFCHR | 0644, NULL, NULL);
    ioattr.mount = &mattr;

    /* Attach the device name */
    id = resmgr_attach(dpp,
                       &rattr,
                       TIUDMA_DEVICE_NAME,
                       _FTYPE_ANY,
                       0,
                       &connect_funcs,
                       &io_funcs,
                       &ioattr);
    if (id == -1) {
        QNX_PR_ERR("%s: Failed to attach pathname", argv[0]);
        return (errno);
    }

    if ((tpool = thread_pool_create(&tattr, 0)) == NULL) {
        QNX_PR_ERR("thread pool create failed");
        return (errno);
    }

    /* Allocate a context structure */
    ctp = dispatch_context_alloc(dpp);

    /* Run in the background */
    if (procmgr_daemon(EXIT_SUCCESS,
                       PROCMGR_DAEMON_NOCLOSE | PROCMGR_DAEMON_NODEVNULL ) == -1) {
        QNX_PR_ERR("%s: procmgr_daemon", argv[0]);
        goto fail0;
    }

    thread_pool_start(tpool);

    while (1) {
        if ((ctp = dispatch_block(ctp)) == NULL) {
            QNX_PR_ERR("%s: Block error", argv[0]);
            goto fail0;
        }
        dispatch_handler(ctp);
    }

fail0:
    return (-errno);
}

static int udma_io_devctl(resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *ocb)
{
    int     status, nbytes;
    int     err = EOK;
    ti_udma_ocb_t * udma_ocb = (ti_udma_ocb_t *)ocb;
    TIUDMA_CmdArgs *cargs = (TIUDMA_CmdArgs *)(_DEVCTL_DATA (msg->i));
    TIUDMA_CmdArgs *output = (TIUDMA_CmdArgs *)(_DEVCTL_DATA (msg->o));
    Udma_DrvHandle  drvHandle = &gUdmaDrvObj[cargs->instId];

    if (udma_ocb->pid != ctp->info.pid) {
        QNX_PR_ERR("%s: ERROR UDMA---> pid=%d not matching ctp->info.pid=%d", __FUNCTION__, udma_ocb->pid, ctp->info.pid);
        return -1;
    }

    if ((status = iofunc_devctl_default(ctp, msg, ocb)) != _RESMGR_DEFAULT) {
        return status;
    }

    pthread_mutex_lock(&g_lock);
    status = nbytes = 0;
    switch(msg->i.dcmd) {
        case DCMD_TIUDMA_ALLOC_BLKCOPYCH:
            output->args.blkcopy.chNum =
                Udma_rmAllocBlkCopyCh(cargs->args.blkcopy.preferredChNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_BLKCOPYCH blkcopy.preferredChNum=%d, blkcopy.ChNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.blkcopy.preferredChNum, output->args.blkcopy.chNum);
            break;

        case DCMD_TIUDMA_FREE_BLKCOPYCH:
            Udma_rmFreeBlkCopyCh(cargs->args.blkcopy.chNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> FREE_BLKCOPYCH blkcopy.ChNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.blkcopy.chNum);
            break;

        case DCMD_TIUDMA_ALLOC_BLKCOPYHCCH:
            output->args.blkcopyhc.chNum =
                Udma_rmAllocBlkCopyHcCh(cargs->args.blkcopyhc.preferredChNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_BLKCOPYHCCH blkcopyhc.preferredChNum=%d, blkcopyhc.ChNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.blkcopyhc.preferredChNum, output->args.blkcopyhc.chNum);
            break;

        case DCMD_TIUDMA_FREE_BLKCOPYHCCH:
            Udma_rmFreeBlkCopyHcCh(cargs->args.blkcopyhc.chNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> FREE_BLKCOPYHCCH blkcopyhc.chNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.blkcopyhc.chNum);
            break;

        case DCMD_TIUDMA_ALLOC_BLKCOPYUHCCH:
            output->args.blkcopyuhc.chNum =
                Udma_rmAllocBlkCopyUhcCh(cargs->args.blkcopyuhc.preferredChNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_BLKCOPYUHCCH blkcopyuhc.preferredChNum=%d, blkcopyuhc.ChNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.blkcopyuhc.preferredChNum, output->args.blkcopyuhc.chNum);
            break;

        case DCMD_TIUDMA_FREE_BLKCOPYUHCCH:
            Udma_rmFreeBlkCopyUhcCh(cargs->args.blkcopyuhc.chNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> FREE_BLKCOPYUHCCH blkcopyuhc.chNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.blkcopyuhc.chNum);
            break;

        case DCMD_TIUDMA_ALLOC_TXCH:
            output->args.tx.chNum =
                Udma_rmAllocTxCh(cargs->args.tx.preferredChNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_TXCH tx.preferredChNum=%d, tx.ChNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.tx.preferredChNum, output->args.tx.chNum);
            break;

        case DCMD_TIUDMA_FREE_TXCH:
            Udma_rmFreeTxCh(cargs->args.tx.chNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> FREE_TXCH tx.chNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.tx.chNum);
            break;

        case DCMD_TIUDMA_ALLOC_RXCH:
            output->args.rx.chNum =
                Udma_rmAllocRxCh(cargs->args.rx.preferredChNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_RXCH rx.preferredChNum=%d, rx.ChNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.rx.preferredChNum, output->args.rx.chNum);
            break;

        case DCMD_TIUDMA_FREE_RXCH:
            Udma_rmFreeRxCh(cargs->args.rx.chNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> FREE_RXCH rx.chNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.rx.chNum);
            break;

        case DCMD_TIUDMA_ALLOC_TXHCCH:
            output->args.txhc.chNum =
                Udma_rmAllocTxHcCh(cargs->args.txhc.preferredChNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_TXHCCH txhc.preferredChNum=%d, txhc.ChNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.txhc.preferredChNum, output->args.txhc.chNum);
            break;

        case DCMD_TIUDMA_FREE_TXHCCH:
            Udma_rmFreeTxHcCh(cargs->args.txhc.chNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> FREE_TXHCCH txhc.chNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.txhc.chNum);
            break;

        case DCMD_TIUDMA_ALLOC_RXHCCH:
            output->args.rxhc.chNum =
                Udma_rmAllocRxHcCh(cargs->args.rxhc.preferredChNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_RXHCCH rxhc.preferredChNum=%d, rxhc.ChNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.rxhc.preferredChNum, output->args.rxhc.chNum);
            break;

        case DCMD_TIUDMA_FREE_RXHCCH:
            Udma_rmFreeRxHcCh(cargs->args.rxhc.chNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> FREE_RXHCCH rxhc.chNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.rxhc.chNum);
            break;

        case DCMD_TIUDMA_ALLOC_TXUHCCH:
            output->args.txuhc.chNum =
                Udma_rmAllocTxUhcCh(cargs->args.txuhc.preferredChNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_TXUHCCH txuhc.preferredChNum=%d, txuhc.ChNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.txuhc.preferredChNum, output->args.txuhc.chNum);
            break;

        case DCMD_TIUDMA_FREE_TXUHCCH:
            Udma_rmFreeTxUhcCh(cargs->args.txuhc.chNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> FREE_TXHCCH rxhc.chNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.txhc.chNum);
            break;

        case DCMD_TIUDMA_ALLOC_RXUHCCH:
            output->args.rxuhc.chNum =
                Udma_rmAllocRxUhcCh(cargs->args.rxuhc.preferredChNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_RXUHCCH rxuhc.preferredChNum=%d, rxuhc.ChNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.rxuhc.preferredChNum, output->args.rxuhc.chNum);
            break;

        case DCMD_TIUDMA_FREE_RXUHCCH:
            Udma_rmFreeRxUhcCh(cargs->args.rxuhc.chNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> FREE_RXUHCCH rxuhc.chNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.rxuhc.chNum);
            break;

#if (UDMA_NUM_MAPPED_TX_GROUP > 0)
        case DCMD_TIUDMA_ALLOC_MAPPED_TX_CH:
            output->args.mappedtx.chNum =
                Udma_rmAllocMappedTxCh(cargs->args.mappedtx.preferredChNum, drvHandle,
                                        cargs->args.mappedtx.mappedChGrp);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_MAPPED_TX_CH mappedtx.preferredChNum=%d, mappedtx.ChNum=%d mappedtx.mappedChGrp=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.mappedtx.preferredChNum, output->args.mappedtx.chNum,
                cargs->args.mappedtx.mappedChGrp);
            break;

        case DCMD_TIUDMA_FREE_MAPPED_TX_CH:
            Udma_rmFreeMappedTxCh(cargs->args.mappedtx.chNum, drvHandle, cargs->args.mappedtx.mappedChGrp);
            QNX_PR_INFO("%s: pid=%d UDMA---> FREE_MAPPED_TX_CH mappedtx.chNum=%d mappedtx.mappedChGrp=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.mappedtx.chNum, cargs->args.mappedtx.mappedChGrp);
            break;
#endif

#if (UDMA_NUM_MAPPED_RX_GROUP > 0)
        case DCMD_TIUDMA_ALLOC_MAPPED_RX_CH:
            output->args.mappedrx.chNum =
                Udma_rmAllocMappedRxCh(cargs->args.mappedrx.preferredChNum, drvHandle, cargs->args.mappedrx.mappedChGrp);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_MAPPED_RX_CH mappedrx.preferredChNum=%d, mappedrx.ChNum=%d mappedrx.mappedChGrp=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.mappedrx.preferredChNum, output->args.mappedrx.chNum,
                cargs->args.mappedrx.mappedChGrp);
            break;

        case DCMD_TIUDMA_FREE_MAPPED_RX_CH:
            Udma_rmFreeMappedRxCh(cargs->args.mappedrx.chNum, drvHandle, cargs->args.mappedrx.mappedChGrp);
            QNX_PR_INFO("%s: pid=%d UDMA---> FREE_MAPPED_RX_CH mappedrx.chNum=%d mappedrx.mappedChGrp=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.mappedrx.chNum, cargs->args.mappedrx.mappedChGrp);
            break;
#endif

#if (UDMA_NUM_UTC_INSTANCE > 0)
        case DCMD_TIUDMA_ALLOC_EXTCH:
            output->args.ext.chNum =
                Udma_rmAllocExtCh(cargs->args.ext.preferredChNum, drvHandle, cargs->args.ext.utcInfo);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_EXTCH ext.preferredChNum=%d, ext.ChNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.ext.preferredChNum, output->args.ext.chNum);
            break;

        case DCMD_TIUDMA_FREE_EXTCH:
            Udma_rmFreeExtCh(cargs->args.ext.chNum, drvHandle, cargs->args.ext.utcInfo);
            QNX_PR_INFO("%s: pid=%d UDMA---> FREE_EXTCH ext.chNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.ext.chNum);
            break;
#endif

#if (UDMA_SOC_CFG_PKTDMA_PRESENT == 1)
        case DCMD_TIUDMA_ALLOC_MAPPEDRING:
            output->args.allocmapring.ringNum =
                Udma_rmAllocMappedRing(drvHandle, cargs->args.allocmapring.mappdRingGrp, cargs->args.allocmapring.mappedChNum);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_MAPPEDRING allocmapring.mappdRingGrp=%d, allocmapring.mappedChNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.allocmapring.mappdRingGrp, cargs->args.allocmapring.mappedChNum);
            break;

        case DCMD_TIUDMA_FREE_MAPPEDRING:
            Udma_rmFreeMappedRing(cargs->args.freemapring.ringNum, drvHandle, cargs->args.freemapring.mappdRingGrp, cargs->args.freemapring.mappedChNum);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_FREEMAPPEDRING freemapring.mappdRingGrp=%d, allocmapring.mappedChNum=%d", __FUNCTION__, ctp->info.pid,
                    cargs->args.freemapring.ringNum, cargs->args.allocmapring.mappdRingGrp, cargs->args.allocmapring.mappedChNum);
            break;
#endif
        case DCMD_TIUDMA_ALLOC_PROXY:
        {
         int32_t idx = ResMgrCheckProxyIdx(drvHandle, ctp->info.pid);
         if (idx != -1)
         {
             output->args.proxy.proxyNum = idx;
         }
         else
         {
             output->args.proxy.proxyNum =
                 Resmgr_Udma_rmAllocProxy(cargs->args.proxy.preferredProxyNum, drvHandle);
             if(output->args.proxy.proxyNum <= MAX_UDMA_PROXIES)
                drvHandle->proxyPids[output->args.proxy.proxyNum] = ctp->info.pid;
         }
         QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_PROXY proxy.preferredProxyNum=%d, proxy.proxyNum=%d", __FUNCTION__, ctp->info.pid,
                     cargs->args.proxy.preferredProxyNum, output->args.proxy.proxyNum);
         break;
        }
        case DCMD_TIUDMA_FREE_PROXY:
            Udma_rmFreeProxy(cargs->args.proxy.proxyNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> FREE_PROXY proxy.proxyNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.proxy.proxyNum);
            break;

        case DCMD_TIUDMA_ALLOC_FREERING:
            output->args.freering.ringNum =
                Udma_rmAllocFreeRing(drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_FREERING freering.ringNum=%d", __FUNCTION__, ctp->info.pid,
                output->args.freering.ringNum);
            break;

        case DCMD_TIUDMA_FREE_FREERING:
            Udma_rmFreeFreeRing(cargs->args.freering.ringNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> FREE_FREERING freering.ringNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.freering.ringNum);
            break;

        case DCMD_TIUDMA_ALLOC_RINGMON:
            output->args.ringmon.ringNum =
                Udma_rmAllocRingMon(drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_RINGMON ringmon.ringNum=%d", __FUNCTION__, ctp->info.pid,
                output->args.ringmon.ringNum);
            break;

        case DCMD_TIUDMA_FREE_RINGMON:
            Udma_rmFreeRingMon(cargs->args.ringmon.ringNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> FREE_RINGMON ringmon.ringNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.ringmon.ringNum);
            break;

        case DCMD_TIUDMA_ALLOC_VINTR:
            output->args.vintr.vintrNum =
                Udma_rmAllocVintr(drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_VINTR vintr.vintrNum=%d", __FUNCTION__, ctp->info.pid,
                output->args.vintr.vintrNum);
            break;

        case DCMD_TIUDMA_FREE_VINTR:
            Udma_rmFreeVintr(cargs->args.vintr.vintrNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> FREE_VINTR vintr.vintrNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.vintr.vintrNum);
            break;

        case DCMD_TIUDMA_ALLOC_IRINTR:
            output->args.irintr.irIntrNum =
                Udma_rmAllocIrIntr(cargs->args.irintr.preferredIrIntrNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_IRINTR rintr.preferredIrIntrNum=%d irintr.irIntrNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.irintr.preferredIrIntrNum, output->args.irintr.irIntrNum);
            break;

        case DCMD_TIUDMA_FREE_IRINTR:
            Udma_rmFreeIrIntr(cargs->args.irintr.irIntrNum, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> FREE_IRINTR irintr.irIntrNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.irintr.irIntrNum);
            break;

        case DCMD_TIUDMA_ALLOC_EVENT:
            output->args.event.globalEvent =
                Udma_rmAllocEvent(drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_EVENT event.globalEvent=%d", __FUNCTION__, ctp->info.pid,
                output->args.event.globalEvent);
            break;

        case DCMD_TIUDMA_FREE_EVENT:
            Udma_rmFreeEvent(cargs->args.event.globalEvent, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> FREE_EVENT event.globalEvent=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.event.globalEvent);
            break;

        case DCMD_TIUDMA_ALLOC_VINTRBIT:
            output->args.vintrbit.vintrBitNum =
                Udma_rmAllocVintrBit(drvHandle->globalEventHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_VINTRBIT vintrbit.vintrBitNum=%d", __FUNCTION__, ctp->info.pid,
                output->args.vintrbit.vintrBitNum);
            break;

        case DCMD_TIUDMA_FREE_VINTRBIT:
            Udma_rmFreeVintrBit(cargs->args.vintrbit.vintrBitNum, drvHandle, drvHandle->globalEventHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> FREE_VINTRBIT vintrbit.vintrBitNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.vintrbit.vintrBitNum);
            break;

        case DCMD_TIUDMA_TRANSLATE_IR_OUTPUT:
            output->args.irOutput.coreIntrNum =
                Udma_rmTranslateIrOutput(drvHandle, cargs->args.irOutput.irIntrNum);
            QNX_PR_INFO("%s: pid=%d UDMA---> TRANSLATE_IR_OUTPUT irOutput.irIntrNum=%d, irOutput.coreIntrNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.irOutput.irIntrNum, output->args.irOutput.coreIntrNum);
            break;

        case DCMD_TIUDMA_TRANSLATE_CORE_INTR_INPUT:
            output->args.coreIntInput.irIntrNum =
                Udma_rmTranslateCoreIntrInput(drvHandle, cargs->args.coreIntInput.coreIntrNum);
            QNX_PR_INFO("%s: pid=%d UDMA---> TRANSLATE_CORE_INTR_INPUT coreIntInput.coreIntrNum=%d, coreIntInput.irIntrNum=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.coreIntInput.coreIntrNum, output->args.coreIntInput.irIntrNum);
            break;

#if (UDMA_SOC_CFG_UDMAP_PRESENT == 1)
        case DCMD_TIUDMA_ALLOC_FLOW:
            output->args.flow.flowStart =
                Udma_rmAllocflow(cargs->args.flow.flowCnt, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> ALLOC_FLOW flow.flowCnt=%d, flow.flowStart=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.flow.flowCnt, output->args.flow.flowStart);
            break;

        case DCMD_TIUDMA_FREE_FLOW:
            Udma_rmFreeflow(cargs->args.flow.flowStart, cargs->args.flow.flowCnt, drvHandle);
            QNX_PR_INFO("%s: pid=%d UDMA---> FREE_FLOW flow.flowStart=%d, flow.flowCnt=%d", __FUNCTION__, ctp->info.pid,
                cargs->args.flow.flowStart, cargs->args.flow.flowCnt);
            break;
#endif
        default:
            err = EINVAL;
    }

    if (err != EOK) {
        QNX_PR_ERR("%s: pid=%d ERROR nbytes/%d err/%d EOK/%d", __func__, ctp->info.pid, nbytes, err, EOK);
        pthread_mutex_unlock(&g_lock);
        return (err);
    }

    msg->o.ret_val = 0;
    pthread_mutex_unlock(&g_lock);

    return (_RESMGR_PTR(ctp, &msg->o, sizeof(msg->o) + sizeof(TIUDMA_CmdArgs)));
}

