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

#include "tisci_mgr.h"
#include "psdkqnx_proto.h"

#define TISCI_DEVICE_NAME       "/dev/tisci"

#define SPROXY_RECV_TIMEOUT     1000
#define SCI_MAX_MESSAGE_SIZE    2048

/* log level set to info by default */
int g_log_level = _SLOG_NOTICE;

/* Default operation mode for sciclient (interrupt vs polled) */
#if !defined(SOC_J722S) && (_NTO_VERSION == 710)
uint32_t gSciClientOpMode = SCICLIENT_SERVICE_OPERATION_MODE_INTERRUPT;
#else
uint32_t gSciClientOpMode = SCICLIENT_SERVICE_OPERATION_MODE_POLLED;
#endif

/* init the module number for the qnx_logger */
int module_num = PSDKQA_SLOGC_TI_SCI_RM;

pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static int sci_io_devctl(resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *ocb);
static int ti_sci_msg_xfer(void *ibuf, void *obuf, int *nbytes);

uint64_t Sciclient_qnxVirtToPhyFxn(const void *virtAddr,
                                   void *appData);

void * Sciclient_qnxPhyToVirtFxn(uint64_t phyAddr,
                                 void *appData);

static void getRevision(void)
{
    int32_t status = CSL_PASS;

    /* Fill in version request message */
    struct tisci_msg_version_req request;
    const Sciclient_ReqPrm_t      reqPrm =
    {
        TISCI_MSG_VERSION,
        TISCI_MSG_FLAG_AOP,
        (uint8_t *) &request,
        sizeof(request),
        SCICLIENT_SERVICE_WAIT_FOREVER
    };

    struct tisci_msg_version_resp response;
    Sciclient_RespPrm_t           respPrm =
    {
        0,
        (uint8_t *) &response,
        sizeof (response)
    };

    /* Request version */
    status = Sciclient_service_rsmgr(&reqPrm, &respPrm);
    if (CSL_PASS == status) {
        if (respPrm.flags == TISCI_MSG_FLAG_ACK) {
            status = CSL_PASS;
            QNX_PR_NOTICE(" SYSFW Firmware Version %s", (char *) response.str);
            QNX_PR_NOTICE(" SYSFW Firmware revision 0x%x", response.version);
            QNX_PR_NOTICE(" SYSFW ABI revision %d.%d", response.abi_major, response.abi_minor);
        }
        else {
            QNX_PR_ERR("Error: %s (%d): DMSC Firmware Get Version failed", __func__,__LINE__);
        }
    }
    else {
        QNX_PR_ERR("Error: %s (%d): DMSC Firmware Get Version failed", __func__,__LINE__);
    }
}

static int options(int argc, char *const argv[])
{
    int opt;
    int loglevel = _SLOG_NOTICE;
    int err = EOK;

    while ((opt = getopt(argc, argv, "vp")) != -1) {
        switch (opt) {
            case 'v':
                loglevel++;
                break;
            case 'p':
                gSciClientOpMode = SCICLIENT_SERVICE_OPERATION_MODE_POLLED;
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

int main(int argc, char *argv[])
{
    struct stat             sbuf;
    int         id;

    int32_t status;
    resmgr_connect_funcs_t connect_funcs;
    resmgr_io_funcs_t io_funcs;
    dispatch_t *dpp;
    resmgr_attr_t rattr;
    dispatch_context_t *ctp;
    iofunc_attr_t ioattr;
    Sciclient_ConfigPrms_t config;

    /* Only allow one instance */
    if (-1 != stat(TISCI_DEVICE_NAME, &sbuf)) {
        perror("TISCI resmgr already running...");
        return (-1);
    }

    /* Get IO priveleges */
    if (ThreadCtl(_NTO_TCTL_IO, NULL) == -1) {
        perror("ThreadCtl(_NTO_TCTL_IO");
        return (-1);
    }

    if ((options(argc, argv)) != EOK) {
        printf ("Starting TISCI resource manager failed, invalid arguments\n");
        return (-1);
    }

    printVersion("TI SCI ResMgr");

    /* Initialize SCI config */
    Sciclient_configPrmsInit(&config);

    /* Set interupt/polling operation mode for sciclient */
    config.opModeFlag = gSciClientOpMode;
    if (config.opModeFlag == SCICLIENT_SERVICE_OPERATION_MODE_POLLED) {
        printf("Initializing sciclient in polling mode\n");
    }
    else if (config.opModeFlag == SCICLIENT_SERVICE_OPERATION_MODE_INTERRUPT) {
        printf("Initializing sciclient in interupt mode\n");
    }

    /* Initialize SCI */
    status = Sciclient_init(&config);
    if (status != 0) {
        perror("SCI INitialization failed\n");
        QNX_PR_ERR("Error: %s: Failed to intiialize SCI Client", argv[0]);
    }

    getRevision();

    /* Initialize the dispatch interface */
    dpp = dispatch_create();
    if (!dpp) {
        QNX_PR_ERR("Error: %s: Failed to create dispatch interface", argv[0]);
        return (errno);
    }

    /* Initialize the resource manager attributes */
    memset(&rattr, 0, sizeof(rattr));

    /* Initialize the connect functions */
    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs, _RESMGR_IO_NFUNCS, &io_funcs);
    io_funcs.devctl = sci_io_devctl;
    iofunc_attr_init(&ioattr, S_IFCHR | 0644, NULL, NULL);

    /* Attach the device name */
    id = resmgr_attach(dpp, &rattr, TISCI_DEVICE_NAME, _FTYPE_ANY, 0, &connect_funcs, &io_funcs, &ioattr);
    if (id == -1) {
        QNX_PR_ERR("Error: %s: Failed to attach pathname", argv[0]);
        return (errno);
    }

    /* Allocate a context structure */
    ctp = dispatch_context_alloc(dpp);

    /* Run in the background */
    if (procmgr_daemon(EXIT_SUCCESS, PROCMGR_DAEMON_NOCLOSE | PROCMGR_DAEMON_NODEVNULL ) == -1) {
        QNX_PR_ERR("Error: %s: procmgr_daemon failed", argv[0]);
        goto fail0;
    }

    while (1) {
        if ((ctp = dispatch_block(ctp)) == NULL) {
            QNX_PR_ERR("Error: %s: Dispatch block error", argv[0]);
            goto fail0;
        }
        dispatch_handler(ctp);
    }

fail0:
    return (-errno);
}

void hexdumpSciMessage(char *payload, int size)
{
    int pending = size;
    while (pending > 0) {
        char buf[16] = {0};
        int bytes = (pending > 16)? 16:pending;
        memcpy(buf, payload, bytes);
        QNX_PR_DEBUG("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
                buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
        pending -=bytes;
    }
}

static int ti_sci_msg_xfer(void *ibuf, void *obuf, int *nbytes)
{
    int32_t status = CSL_PASS;
    tisci_msg_t *tisci_msg_in = (tisci_msg_t *) ibuf;
    tisci_msg_t *tisci_msg_out = (tisci_msg_t *) obuf;

    Sciclient_ReqPrm_t   *reqFromClient = (Sciclient_ReqPrm_t *) &tisci_msg_in->reqPrm;
    Sciclient_RespPrm_t   *respFromClient = (Sciclient_RespPrm_t *) &tisci_msg_in->respPrm;
    Sciclient_RespPrm_t  *respToClient = (Sciclient_RespPrm_t *) &tisci_msg_out->respPrm;

    /* Change the physical address of the client response to virtual mapping, so we can
     * write directly into the address space provided by user.  This somewhat removes the
     * need for the response portion of the message to the user.  Need to think through
     * if the full response has to go back to user, or just the payload.
     */

    uint8_t *pReqPhysAddrBackup = 0;
    uint8_t *pReqVirtAddr = 0;
    uint8_t *pRespPhysAddrBackup = 0;
    uint8_t *pRespVirtAddr = 0;
    if((reqFromClient->pReqPayload != 0) && (reqFromClient->reqPayloadSize > 0)) {
        pReqVirtAddr = Sciclient_qnxPhyToVirtFxn((uint64_t) reqFromClient->pReqPayload, &reqFromClient->reqPayloadSize);
        if(pReqVirtAddr == 0) {
            QNX_PR_ERR("Error: %s (%d): sciclient service call unable to map memory",__func__,__LINE__);
            status = CSL_EALLOC;
        }
        pReqPhysAddrBackup = (uint8_t *)reqFromClient->pReqPayload;
        reqFromClient->pReqPayload = (uint8_t *) pReqVirtAddr;
    }
    if((respFromClient->pRespPayload != 0) && (respFromClient->respPayloadSize > 0)) {
        pRespVirtAddr = Sciclient_qnxPhyToVirtFxn((uint64_t) respFromClient->pRespPayload, &respFromClient->respPayloadSize);
        if(pRespVirtAddr == 0) {
            QNX_PR_ERR("Error: %s (%d): sciclient service call unable to map memory",__func__,__LINE__);
            status = CSL_EALLOC;
        }
        pRespPhysAddrBackup = (uint8_t *) respFromClient->pRespPayload;
        respFromClient->pRespPayload = (uint8_t *) pRespVirtAddr;
    }

    /* Send request to sciclient library */
    if (CSL_PASS == status) {
        status = Sciclient_service_rsmgr(reqFromClient, respFromClient);
    }

    if (CSL_PASS == status) {
        *nbytes = sizeof(tisci_msg_t);

        QNX_PR_INFO("%s: reqFromClient->messageType = 0x%08x,  Req flags = 0x%08x, Resp flags = 0x%08x",__func__,
        reqFromClient->messageType, reqFromClient->flags, respFromClient->flags);

        if (((reqFromClient->flags & TISCI_MSG_FLAG_AOP) == TISCI_MSG_FLAG_AOP) &&
            ((respFromClient->flags & TISCI_MSG_FLAG_ACK) != TISCI_MSG_FLAG_ACK)) {

            status = CSL_EFAIL;
            QNX_PR_ERR("Error: %s (%d): sciclient service call failed on ACK",__func__,__LINE__);
            QNX_PR_INFO("%s: reqFromClient->flags = 0x%08x",__func__, reqFromClient->flags);
            QNX_PR_INFO("%s: respFromClient->flags = 0x%08x",__func__, respFromClient->flags);
        }

        /* Change the virtual address of the request and response virtual address,
         * to physical */
        if((reqFromClient->pReqPayload != 0) && (reqFromClient->reqPayloadSize > 0)) {
            reqFromClient->pReqPayload = pReqPhysAddrBackup;
        }
        if((respFromClient->pRespPayload != 0) && (respFromClient->respPayloadSize > 0)) {
            respFromClient->pRespPayload = pRespPhysAddrBackup;
        }
    }
    else {
          QNX_PR_ERR("Error: %s (%d): Call to Sciclient_service_rsmgr failed",__func__,__LINE__);
    }

    respToClient->flags = respFromClient->flags;
    respToClient->respPayloadSize = respFromClient->respPayloadSize;
    respToClient->pRespPayload = respFromClient->pRespPayload;
    QNX_PR_DEBUG("%s: respFromClient->flags = 0x%08x",__func__, respFromClient->flags);
    QNX_PR_DEBUG("%s: respToClient->flags = 0x%08x",__func__, respToClient->flags);
    QNX_PR_DEBUG("%s: respToClient->payloadSize = 0x%d",__func__, respToClient->respPayloadSize);

#ifdef DEBUG_MODE
    QNX_PR_DEBUG("%s: REQUEST FROM CLIENT HEXDUMP", __func__);
    hexdumpSciMessage(pRespVirtAddr, respFromClient->respPayloadSize);
    QNX_PR_DEBUG("%s: RESPONSE FROM SERVER HEXDUMP", __func__);
    hexdumpSciMessage(pReqVirtAddr, reqFromClient->reqPayloadSize);
#endif

    if(pReqVirtAddr != 0) {
        if(munmap_device_memory(pReqVirtAddr, reqFromClient->reqPayloadSize) == -1) {
            QNX_PR_ERR("Error: %s (%d): Memory unmap failed",__func__,__LINE__);
        }
    }
    if(pRespVirtAddr != 0) {
        if(munmap_device_memory(pRespVirtAddr, respFromClient->respPayloadSize) == -1) {
            QNX_PR_ERR("Error: %s (%d): Memory unmap failed",__func__,__LINE__);
        }
    }

    return (EOK);
}

static int sci_io_devctl(resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *ocb)
{
    int     status, nbytes;
    int     err = EOK;

    if ((status = iofunc_devctl_default(ctp, msg, ocb)) != _RESMGR_DEFAULT) {
        return status;
    }

    pthread_mutex_lock(&g_lock);
    nbytes = 0;
    if (msg->i.dcmd == DCMD_TISCI_MESSAGE) {
        QNX_PR_DEBUG("%s: DCMD_TISCI_MESSAGE rxd",__func__);
        /* Should also pass msg->o here, to copy in response message, re-use user's buffer */
        err = ti_sci_msg_xfer(_DEVCTL_DATA(msg->i), _DEVCTL_DATA(msg->o), &nbytes);
    } else {
        err = EINVAL;
    }

    if (nbytes == 0 || err != EOK) {
        printf("%s: ERROR nbytes/%d err/%d EOK/%d\n",__func__,nbytes,err,EOK);
        QNX_PR_ERR("Error: %s (%d): nbytes/%d err/%d EOK/%d",__func__,__LINE__,nbytes,err,EOK);
        pthread_mutex_unlock(&g_lock);
        return (err);
    }

    msg->o.ret_val = 0;
    msg->o.nbytes = nbytes;
    QNX_PR_DEBUG("%s: sizeof(msg->o)/%d sizeof(tisci_msg_t)/%d nbytes/%d",__func__, (int)sizeof(msg->o) , (int)sizeof(tisci_msg_t), nbytes);

    pthread_mutex_unlock(&g_lock);
    return (_RESMGR_PTR(ctp, &msg->o, sizeof(msg->o) + sizeof(tisci_msg_t)));
}
