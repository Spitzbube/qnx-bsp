/*
 * $QNXLicenseC:
 * Copyright 2020, QNX Software Systems.
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
 * Modfications copyright (c) 2020-2022, Texas Instruments Incorporated
 *
 */

/*
 * Define THREAD_POOL_PARAM_T such that we can avoid a compiler
 * warning when we use the dispatch*() functions below
 */
#define THREAD_POOL_PARAM_T dispatch_context_t

#include "tiipc_mgr.h"
#include "tiipc_mgr_private.h"
#include "psdkqnx_proto.h"

#define TIIPC_DEVICE_NAME       "/dev/tiipc"

#define TIIPC_ARG_VAL( _o, _v ) if( (_v) == NULL || *(_v) == '\0' ) { ret = EINVAL; fprintf(stderr, "%s: missing argument for '%s'\n", __func__, _o); break;}

/* log level set to info by default */
int g_log_level = _SLOG_NOTICE;

/* init the module number for the qnx_logger */
int module_num = PSDKQA_SLOGC_TI_IPC_RM;

static char *supported_opts[] = {
    "vring_base",      // VRING base physical address
    "vring_size",      // VRING window size
    NULL
};

#if defined (PSDK_QNX_ENABLE_GCOV)
extern void __gcov_flush();
#endif

static uint32_t  g_vringBaseAddr = VRING_BASE_ADDRESS;
static uint32_t  g_vringBufSize = IPC_VRING_BUFFER_SIZE;
static int ipc_io_devctl(resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *ocb);
static uint32_t remoteProc[] =
{
#if defined (SOC_J721E)
    IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU2_1,
    IPC_MCU3_0, IPC_MCU3_1, IPC_C66X_1, IPC_C66X_2,
    IPC_C7X_1
#elif defined (SOC_J7200)
    IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU2_1
#elif defined (SOC_J721S2)
    IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU2_1,
    IPC_MCU3_0, IPC_MCU3_1, IPC_C7X_1, IPC_C7X_2
#elif defined (SOC_AM62X)
    IPC_M4F_0, IPC_MCU1_0
#elif defined (SOC_AM62A)
    IPC_MCU1_0, IPC_C7X_1, IPC_MCU2_0
#elif defined (SOC_J784S4)
    IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU2_1,
    IPC_MCU3_0, IPC_MCU3_1, IPC_MCU4_0, IPC_MCU4_1,
    IPC_C7X_1, IPC_C7X_2, IPC_C7X_3, IPC_C7X_4
#elif defined (SOC_J722S)
    IPC_WKUP_R5F, IPC_MCU1_0, IPC_MAIN_R5F, IPC_C7X_1, IPC_C7X_2
#endif
};


uint8_t ctrlStack[IPC_TASK_STACKSIZE] __attribute__ ((section("ipc_data_buffer"), aligned(8192)));
uint8_t ctrlBuf[RPMSG_DATA_SIZE]      __attribute__ ((section("ipc_data_buffer"), aligned (8)));
uint8_t sysVqBuf[VQ_BUF_SIZE]         __attribute__ ((section("ipc_data_buffer"), aligned (8)));

/* must be called with lock held */
ipc_rpmsg_handle_entry *ipc_lookup_entry(IpcUtils_QHandle *handle, uint32_t ept)
{
    ipc_rpmsg_handle_entry *entry = NULL;
    IpcUtils_QElem *elem, *head;
    uint8_t found = 0;

    if (FALSE == IpcUtils_QisEmpty(handle))
    {
        elem = head = (IpcUtils_QElem *)IpcUtils_QgetHeadNode(handle);
        do
        {
            entry = (ipc_rpmsg_handle_entry*)elem;
            if( (NULL != entry) &&
                (entry->ept == ept))
            {
                found = 1;
                break;
            }
            elem = (IpcUtils_QElem *) IpcUtils_Qnext(elem);
        } while (elem != head);
    }
    if (found != 1)
    {
        entry = NULL;
    }
    return entry;
}

static void IpcPrint(const char *str)
{
    slogf(_SLOG_SETCODE(_SLOGC_PRIVATE_START, module_num), _SLOG_INFO, str);
}

void Ipc_rsmgr_setup(void)
{
    Ipc_VirtIoParams  vqParam;
    RPMessage_Params  cntrlParam;
    uint32_t          selfId  = Ipc_getCoreId();
    uint32_t          numProc = sizeof(remoteProc)/sizeof(uint32_t);
    Ipc_InitPrms      initPrms;

    Ipc_mpSetConfig(selfId, numProc, remoteProc);

    /* Initialize params with defaults */
    IpcInitPrms_init(0U, &initPrms);

    initPrms.printFxn = &IpcPrint;

    Ipc_init(&initPrms);

    vqParam.vqObjBaseAddr = (void*)sysVqBuf;
    vqParam.vqBufSize     = numProc * Ipc_getVqObjMemoryRequiredPerCore();
    vqParam.vringBaseAddr = (void*)(uintptr_t)g_vringBaseAddr;
    vqParam.vringBufSize  = g_vringBufSize;
    vqParam.timeoutCnt    = 100;  /* Wait for counts */
    Ipc_initVirtIO(&vqParam);

    RPMessageParams_init(&cntrlParam);

    /* Set memory for HeapMemory for control task */
    cntrlParam.buf         = ctrlBuf;
    cntrlParam.bufSize     = RPMSG_DATA_SIZE;
    cntrlParam.stackBuffer = ctrlStack;
    cntrlParam.stackSize   = IPC_TASK_STACKSIZE;
    RPMessage_init(&cntrlParam);
}

int
ipc_unblock(resmgr_context_t *ctp, io_pulse_t *msg, RESMGR_OCB_T *ocb)
{
    int status = _RESMGR_NOREPLY;
    struct _msg_info info;
    ti_ipc_ocb_t *ipc_ocb = (ti_ipc_ocb_t *)ocb;
    ipc_rpmsg_handle_entry *rpmsg_entry = NULL;
    ipc_waiting_threads_entry *wt_entry = NULL;
    IpcUtils_QElem *elem, *head;
    uint8_t found = 0;

    if ((status = iofunc_unblock_default(ctp, msg, ocb)) != _RESMGR_DEFAULT) {
        return status;
    }

    if (MsgInfo(ctp->rcvid, &info) == -1 ||
        !(info.flags & _NTO_MI_UNBLOCK_REQ)) {
        return _RESMGR_NOREPLY;
    }

    /* find this rcvid */
    pthread_mutex_lock(&ipc_ocb->lock);
    if (FALSE == IpcUtils_QisEmpty(&ipc_ocb->created_handles))
    {
        elem = head = (IpcUtils_QElem *)IpcUtils_QgetHeadNode(&ipc_ocb->created_handles);
        do
        {
            rpmsg_entry = (ipc_rpmsg_handle_entry*)elem;
            if( (NULL != rpmsg_entry) &&
                (rpmsg_entry->rcvid == ctp->rcvid))
            {
                found = 1;
                RPMessage_unblock(rpmsg_entry->handle);
                break;
            }
            elem = (IpcUtils_QElem *) IpcUtils_Qnext(elem);
        } while (elem != head);
    }

    if (found == 0 && (FALSE == IpcUtils_QisEmpty(&ipc_ocb->waiting_threads)))
    {

        elem = head = (IpcUtils_QElem *)IpcUtils_QgetHeadNode(&ipc_ocb->waiting_threads);
        do
        {
            wt_entry = (ipc_waiting_threads_entry*)elem;
            if( (NULL != wt_entry) &&
                (wt_entry->rcvid == ctp->rcvid))
            {
                RPMessage_unblockGetRemoteEndPt(ctp->rcvid);
                break;
            }
            elem = (IpcUtils_QElem *) IpcUtils_Qnext(elem);
        } while (elem != head);
    }

    pthread_mutex_unlock(&ipc_ocb->lock);
    return _RESMGR_NOREPLY;
}

IOFUNC_OCB_T *
ocb_calloc (resmgr_context_t * ctp, IOFUNC_ATTR_T * device)
{
    ti_ipc_ocb_t *ocb = NULL;

    /* Allocate the OCB */
    ocb = (ti_ipc_ocb_t *) calloc (1, sizeof (ti_ipc_ocb_t));
    if (ocb == NULL)
    {
        errno = ENOMEM;
        return NULL;
    }

    pthread_mutex_init(&ocb->lock, NULL);
    /* Initialize OCB */
    IpcUtils_Qcreate(&ocb->created_handles);
    IpcUtils_Qcreate(&ocb->waiting_threads);

    return (IOFUNC_OCB_T *)(ocb);
}

void
ocb_free (IOFUNC_OCB_T * ocb)
{
    ti_ipc_ocb_t *ipc_ocb = (ti_ipc_ocb_t *)ocb;

    if (ipc_ocb)
    {
        // todo: free all un-released resources
        /* Free/discard all allocated handles if app failed to do so: */
        while (0U == IpcUtils_QisEmpty(&ipc_ocb->created_handles))
        {
            void * buf = NULL;
            ipc_rpmsg_handle_entry *entry = NULL;
            RPMessage_Handle handle = NULL;

            entry = (ipc_rpmsg_handle_entry*)IpcUtils_QgetHead(&ipc_ocb->created_handles);
            if (entry != NULL)
            {
                handle = entry->handle;
                buf = entry->handle;
                if (handle != NULL)
                {
                    RPMessage_delete(&handle);
                }
                if (buf != NULL)
                {
                    free(buf);
                }
                free(entry);
            }
        }
        pthread_mutex_destroy(&ipc_ocb->lock);

#if defined (PSDK_QNX_ENABLE_GCOV)
        // Flush coverage info
        __gcov_flush();
#endif
        free (ipc_ocb);
    }
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
    char                   *input_args;
    char                   *value;
    char                   *freeptr;
    char                   *options;
    int                     opt;
    int                     ret = EOK;

    /* Only allow one instance */
    if (-1 != stat(TIIPC_DEVICE_NAME, &sbuf))
    {
        perror("IPC resmgr already running...");
        return (-1);
    }

    /* Get IO priveleges */
    if (ThreadCtl(_NTO_TCTL_IO, NULL) == -1)
    {
        perror("ThreadCtl(_NTO_TCTL_IO");
        return 1;
    }

    // option has to be the last, no "-"
    input_args = NULL;
    if (strstr(argv[argc - 1], "-") == NULL) {
        input_args = argv[--argc];
    }

    if (input_args != NULL) {

        freeptr = strdup(input_args);

        options = freeptr;

        while (options && *options != '\0')
        {
            opt = getsubopt(&options, supported_opts, &value);
            switch (opt)
            {
                case 0:
                    TIIPC_ARG_VAL(supported_opts[opt], value);
                    g_vringBaseAddr     = strtoul(value, 0, 0);
                    break;
                case 1:
                    TIIPC_ARG_VAL(supported_opts[opt], value);
                    g_vringBufSize     = strtoul(value, 0, 0);
                    break;
                default:
                    QNX_PR_ERR("%s: unknown option: %s\n", argv[0], value);
                    fprintf(stderr, "unknown option: %s\n", value);
                    break;
            }
        }

        free(freeptr);

        if (ret != EOK)
        {
            goto fail0;
        }
    }

    printVersion("TI IPC ResMgr");

    QNX_PR_NOTICE("%s: Starting TI IPC Resmgr", argv[0]);
    QNX_PR_NOTICE("%s: Using VRING base address: 0x%x, size:0x%x", argv[0], g_vringBaseAddr, g_vringBufSize);

    /* Ipc setup for all remote cores */
    Ipc_rsmgr_setup();

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
    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs,
                     _RESMGR_IO_NFUNCS, &io_funcs);
    io_funcs.devctl = ipc_io_devctl;
    io_funcs.unblock = ipc_unblock;
    iofunc_attr_init(&ioattr, S_IFCHR | 0644, NULL, NULL);
    ioattr.mount = &mattr;

    /* Attach the device name */
    id = resmgr_attach(dpp,
                       &rattr,
                       TIIPC_DEVICE_NAME,
                       _FTYPE_ANY,
                       0,
                       &connect_funcs,
                       &io_funcs,
                       &ioattr);
    if (id == -1)
    {
        QNX_PR_ERR("%s: Failed to attach pathname", argv[0]);
        printf("%s: Failed to attach pathname", argv[0]);
        return (errno);
    }

    if ((tpool = thread_pool_create(&tattr, 0)) == NULL)
    {
        QNX_PR_ERR("%s: Thread pool create failed", argv[0]);
        printf("%s: Thread pool create failed\n", argv[0]);
        return (errno);
    }

    /* Allocate a context structure */
    ctp = dispatch_context_alloc(dpp);

    /* Run in the background */
    if (procmgr_daemon(EXIT_SUCCESS,
                       PROCMGR_DAEMON_NOCLOSE | PROCMGR_DAEMON_NODEVNULL ) == -1)
    {
        QNX_PR_ERR("%s: procmgr_daemo failed", argv[0]);
        printf("%s: procmgr_daemon failed", argv[0]);
        goto fail0;
    }

    thread_pool_start(tpool);


    while (1)
    {
        if ((ctp = dispatch_block(ctp)) == NULL)
        {
            QNX_PR_ERR("%s: Block error", argv[0]);
            goto fail0;
        }
        dispatch_handler(ctp);
    }

fail0:
    return (-errno);
}

int32_t check_mp_config(uint32_t selfId, uint16_t numProc,
                        uint32_t procArry[IPC_MAX_PROCS])
{
    int32_t retVal = IPC_SOK;
    uint32_t i = 0;

    if (selfId != Ipc_getCoreId() ||
        numProc != (sizeof(remoteProc)/sizeof(uint32_t)))
    {
        retVal = IPC_EINVALID_PARAMS;
        QNX_PR_ERR("%s:%d Error: supplied config doesn't match with existing config", __FUNCTION__, __LINE__);
    }
    else
    {
        for(i = 0; i < numProc; i++)
        {
            if (procArry[i] != remoteProc[i])
            {
                retVal = IPC_EINVALID_PARAMS;
                QNX_PR_ERR("%s:%d Error: supplied config doesn't match with existing config", __FUNCTION__, __LINE__);
                break;
            }
        }
    }

    return retVal;
}

static int ipc_io_devctl(resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *ocb)
{
    int     status, nbytes;
    int     err = EOK;
    ti_ipc_ocb_t * ipc_ocb = (ti_ipc_ocb_t *)ocb;
    TIIPC_CmdArgs *cargs;
    uint32_t msg_len;

    if (ipc_ocb == NULL)
    {
        QNX_PR_ERR("%s:%d Error: ipc_ocb is NULL", __FUNCTION__, __LINE__);
        return EINVAL;
    }

    if (msg == NULL)
    {
        QNX_PR_ERR("%s:%d Error: msg is NULL", __FUNCTION__, __LINE__);
        return EINVAL;
    }
    else
    {
        cargs = (TIIPC_CmdArgs *)(_DEVCTL_DATA (msg->i));
        msg_len = msg->i.nbytes;
    }

    if ((status = iofunc_devctl_default(ctp, msg, ocb)) != _RESMGR_DEFAULT)
    {
        return status;
    }

    status = nbytes = 0;
    switch(msg->i.dcmd)
    {
        case DCMD_TIIPC_MPSETCONFIG:
            cargs->status = check_mp_config(cargs->args.mp_setconfig.selfId,
                                            cargs->args.mp_setconfig.numProc,
                                            cargs->args.mp_setconfig.procArry);
            if (cargs->status == IPC_SOK)
            {
                uint32_t i = 0;
                uint32_t id = 0;
                uint32_t numProc = cargs->args.mp_setconfig.numProc;
                const char *coreName = NULL;

                coreName = Ipc_getCoreName(cargs->args.mp_setconfig.selfId);
                if (coreName != NULL)
                {
                    strncpy(cargs->args.mp_setconfig.selfName,
                            coreName,
                            IPC_MAX_PROC_NAMELEN-1);
                    cargs->args.mp_setconfig.selfName[IPC_MAX_PROC_NAMELEN-1] = '\0';

                    for(i = 0; i < numProc; i++)
                    {
                        id = cargs->args.mp_setconfig.procArry[i];
                        if(id < IPC_MAX_PROCS)
                        {
                            coreName = Ipc_getCoreName(id);
                            if (coreName != NULL)
                            {
                                strncpy(cargs->args.mp_setconfig.names[i],
                                        coreName,
                                        IPC_MAX_PROC_NAMELEN-1);
                                cargs->args.mp_setconfig.names[i][IPC_MAX_PROC_NAMELEN-1] = '\0';
                            }
                            else
                            {
                                QNX_PR_ERR("%s:%d Error: Core name invalid", __FUNCTION__, __LINE__);
                                err = EINVAL;
                            }
                        }
                    }
                }
                else
                {
                    QNX_PR_ERR("%s:%d Error: Core name invalid", __FUNCTION__, __LINE__);
                    err = EINVAL;
                }
            }
            else
            {
                QNX_PR_ERR("%s:%d Error: mp_config invalid", __FUNCTION__, __LINE__);
                err = EINVAL;
            }
            break;

        case DCMD_TIIPC_RPMSG_SEND:
        {
            void *buf = (void *)(cargs+1);
            ipc_rpmsg_handle_entry *entry = NULL;
            RPMessage_Handle handle = NULL;
            uint32_t buf_size = msg_len - sizeof(*cargs);

            pthread_mutex_lock(&ipc_ocb->lock);
            entry = ipc_lookup_entry(&ipc_ocb->created_handles, cargs->args.send.handle);
            pthread_mutex_unlock(&ipc_ocb->lock);

            if (buf_size != cargs->args.send.len)
            {
                QNX_PR_ERR("%s:%d Error: buffer size sent does not match buffer len", __FUNCTION__, __LINE__);
                err = EINVAL;
            }
            else
            {
                if (entry != NULL)
                {
                    handle = entry->handle;
                }

                if (err == EOK)
                {
                    /* Note: NULL handle is allowed in send call */
                    cargs->status = RPMessage_send(handle,
                                                   cargs->args.send.dstProc,
                                                   cargs->args.send.dstEndPt,
                                                   cargs->args.send.srcEndPt,
                                                   buf,
                                                   cargs->args.send.len);
                }
            }
            break;
        }

        case DCMD_TIIPC_RPMSG_RECV:
        {
            void *buf = (void *)(cargs+1);
            ipc_rpmsg_handle_entry *entry = NULL;
            RPMessage_Handle handle = NULL;
            uint32_t buf_size = msg_len - sizeof(*cargs);

            if (buf_size < IPC_RPMESSAGE_MSG_BUFFER_SIZE)
            {
                QNX_PR_ERR("%s:%d Error: buffer is not large enough. Must be %d bytes", __FUNCTION__, __LINE__, IPC_RPMESSAGE_MSG_BUFFER_SIZE);
                err = EINVAL;
            }
            else
            {
                pthread_mutex_lock(&ipc_ocb->lock);
                entry = ipc_lookup_entry(&ipc_ocb->created_handles, cargs->args.recv.handle);
                pthread_mutex_unlock(&ipc_ocb->lock);

                if (entry != NULL)
                {
                    handle = entry->handle;
                }
                if ((handle != NULL) && (err == EOK))
                {
                    if (entry->rcvid == 0)
                    {
                        entry->rcvid = ctp->rcvid;
                        /* this call can block. need to unlock the ocb default */
                        iofunc_unlock_ocb_default(ctp, msg, ocb);

                        cargs->status = RPMessage_recv(handle,
                                                       buf,
                                                       &cargs->args.recv.len,
                                                       &cargs->args.recv.rplyEndPt,
                                                       &cargs->args.recv.fromProcId,
                                                       cargs->args.recv.timeout);

                        iofunc_lock_ocb_default(ctp, msg, ocb);
                        entry->rcvid = 0;
                        SETIOV(&ctp->iov[0], &msg->o, sizeof(msg->o) + sizeof(TIIPC_CmdArgs));
                        if (cargs->status == IPC_SOK && cargs->args.recv.len > 0)
                        {
                            SETIOV(&ctp->iov[1], buf, cargs->args.recv.len);
                            return _RESMGR_NPARTS(2);
                        }
                        else
                        {
                            return _RESMGR_NPARTS(1);
                        }
                    }
                    else
                    {
                        QNX_PR_ERR("%s:%d Warning: already waiting for a message on this handle", __FUNCTION__, __LINE__);
                    }
                }
                else
                {
                    QNX_PR_ERR("%s:%d Error: invalid handle passed for this ocb", __FUNCTION__, __LINE__);
                    err = EINVAL;
                }
            }
            break;
        }

        case DCMD_TIIPC_RPMSG_UNBLOCK:
        {
            ipc_rpmsg_handle_entry *entry = NULL;
            RPMessage_Handle handle = NULL;

            pthread_mutex_lock(&ipc_ocb->lock);
            entry = ipc_lookup_entry(&ipc_ocb->created_handles, cargs->args.unblock.handle);
            pthread_mutex_unlock(&ipc_ocb->lock);

            if (entry != NULL)
            {
                handle = entry->handle;
            }
            if (handle != NULL)
            {
                    RPMessage_unblock(handle);
            }
            else
            {
                QNX_PR_ERR("%s:%d Error: invalid handle passed for this ocb", __FUNCTION__, __LINE__);
                err = EINVAL;
            }
            break;
        }

        case DCMD_TIIPC_RPMSG_CREATE:
        {
            // Saving local copy of user provided buf to pass back later
            void *buf = cargs->args.create.params.buf;
            // Creating local queue buf in resource manager context
            cargs->args.create.params.buf = malloc(cargs->args.create.params.bufSize);
            if (cargs->args.create.params.buf != NULL)
            {
                RPMessage_Handle handle = NULL;

                handle = RPMessage_create(&cargs->args.create.params,
                                          &cargs->args.create.ept);
                if (handle == NULL)
                {
                    QNX_PR_ERR("%s:%d Error: handle is NULL", __FUNCTION__, __LINE__);
                    free(cargs->args.create.params.buf);
                    cargs->args.create.handle = 0;
                    cargs->status = IPC_EFAIL;
                }
                else
                {
                    ipc_rpmsg_handle_entry *entry = NULL;

                    entry = malloc(sizeof(ipc_rpmsg_handle_entry));
                    if (entry != NULL)
                    {
                        entry->ept = cargs->args.create.ept;
                        entry->handle = handle;
                        entry->rcvid = 0;
                        cargs->args.create.handle = entry->ept;

                        pthread_mutex_lock(&ipc_ocb->lock);
                        IpcUtils_Qput(&ipc_ocb->created_handles, &entry->elem);
                        pthread_mutex_unlock(&ipc_ocb->lock);
                    }
                }
            }
            cargs->args.create.params.buf = buf;
            break;
        }

        case DCMD_TIIPC_RPMSG_DELETE:
        {
            void * buf = NULL;
            ipc_rpmsg_handle_entry *entry = NULL;
            RPMessage_Handle handle = NULL;

            pthread_mutex_lock(&ipc_ocb->lock);
            entry = ipc_lookup_entry(&ipc_ocb->created_handles, cargs->args.delete.handle);
            pthread_mutex_unlock(&ipc_ocb->lock);

            if (entry != NULL)
            {
                handle = entry->handle;
                buf = entry->handle;
            }
            if (handle != NULL)
            {
                cargs->status = RPMessage_delete(&handle);
                if (cargs->status == IPC_SOK)
                {
                    free(buf);
                    pthread_mutex_lock(&ipc_ocb->lock);
                    IpcUtils_Qremove((IpcUtils_QElem*)&entry->elem);
                    pthread_mutex_unlock(&ipc_ocb->lock);
                    free(entry);
                }
            }
            else
            {
                QNX_PR_ERR("%s:%d Error: invalid handle passed for this ocb", __FUNCTION__, __LINE__);
                err = EINVAL;
            }
            break;
        }

        case DCMD_TIIPC_RPMSG_GETREMOTE:
        {
            ipc_waiting_threads_entry *entry = NULL;

            entry = malloc(sizeof(ipc_waiting_threads_entry));
            if (entry != NULL)
            {
                entry->rcvid = ctp->rcvid;

                pthread_mutex_lock(&ipc_ocb->lock);
                IpcUtils_Qput(&ipc_ocb->waiting_threads, &entry->elem);
                pthread_mutex_unlock(&ipc_ocb->lock);

                /* this call can block so release ocb lock */
                iofunc_unlock_ocb_default(ctp, msg, ocb);

                cargs->status = RPMessage_getRemoteEndPtToken(cargs->args.getremote.selfProcId,
                                                              cargs->args.getremote.name,
                                                              &cargs->args.getremote.remoteProcId,
                                                              &cargs->args.getremote.remoteEndPt,
                                                              cargs->args.getremote.timeout,
                                                              ctp->rcvid);

                iofunc_lock_ocb_default(ctp, msg, ocb);

                pthread_mutex_lock(&ipc_ocb->lock);
                IpcUtils_Qremove((IpcUtils_QElem*)&entry->elem);
                pthread_mutex_unlock(&ipc_ocb->lock);
                free(entry);
            }
            else
            {
                QNX_PR_ERR("%s:%d Error: failed to allocate needed memory", __FUNCTION__, __LINE__);
                err = EINVAL;
            }

            break;
        }

        case DCMD_TIIPC_RPMSG_ANNOUNCE:
            cargs->status = RPMessage_announce(cargs->args.announce.remoteProcId,
                                               cargs->args.announce.endPt,
                                               cargs->args.announce.name);
            break;

        case DCMD_TIIPC_GETSTATS:
        {
            RPMessage_getStats(&cargs->args.getStats.ipcStats);

            QNX_PR_NOTICE("%s:%d DCMD_TIIPC_GETSTATS\n"
                         "\tTotal messages received: %u\n"
                         "\tReceived messages live transferred: %u\n"
                         "\tReceived messages enqueued: %u\n"
                         "\tReceive buffer allocation failures: %u\n"
                         "\tReceive message endpoint find fails: %u\n",
                         __FUNCTION__, __LINE__,
                         cargs->args.getStats.ipcStats.recvTotalMessages,
                         cargs->args.getStats.ipcStats.recvMessagesLiveTransferred,
                         cargs->args.getStats.ipcStats.recvMessagesQueued,
                         cargs->args.getStats.ipcStats.recvBufferAllocFailures,
                         cargs->args.getStats.ipcStats.recvEndpointFindFails);
            break;
        }

        default:
            err = EINVAL;
    }

    if (err != EOK)
    {
        QNX_PR_ERR("%s:%d ERROR nbytes/%d err/%d EOK/%d\n",__FUNCTION__, __LINE__,nbytes,err,EOK);
        return (err);
    }

    msg->o.ret_val = 0;

    return (_RESMGR_PTR(ctp, &msg->o, sizeof(msg->o) + sizeof(TIIPC_CmdArgs)));
}
