/*
 *  Copyright (c) Texas Instruments Incorporated 2020-2021
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
 *  \file ipc_api.c
 *
 *  \brief File containing the IPC driver APIs.
 *
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include <ti/drv/ipc/ipc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "psdkqnx_proto.h"
#include "tiipc_mgr.h"
#include "ipc_priv.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */
#define TIIPC_DEVICE_NAME      "/dev/tiipc"

#define IPC_RPMESSAGE_MSG_BUFFER_SIZE   (IPC_MAX_DATA_PAYLOAD + 32)
#define HEAPALIGNMENT                   8U

struct RPMessage_Object_s;

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */


/* The RPMessage Object */
typedef struct RPMessage_Object_s
{
    uint32_t             handle;
    uint32_t             buf_size;
} RPMessage_Object;


/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */


/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */
/* log level set to info by default */
int g_log_level = _SLOG_NOTICE;

/* init the module number for the qnx_logger */
int module_num = PSDKQA_SLOGC_TI_IPC_RM_LIB;

static int32_t g_IpcFd = -1;

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */


int32_t RPMessageParams_init(RPMessage_Params *params)
{
    int32_t    retVal = IPC_SOK;

    if(params == NULL)
    {
        retVal = IPC_EBADARGS;
    }
    else
    {
        params->requestedEndpt = RPMESSAGE_ANY;
        params->numBufs        = RPMessage_Buffer_Count_Default;
        params->stackBuffer    = NULL;
        params->stackSize      = 0U;
    }

    return (retVal);
}

uint32_t RPMessage_getMessageBufferSize(void)
{
    uint32_t msgBufSize = IPC_RPMESSAGE_MSG_BUFFER_SIZE;
    msgBufSize = ((msgBufSize + HEAPALIGNMENT-1) & ~(HEAPALIGNMENT-1));
    return msgBufSize;
}

uint32_t RPMessage_getObjMemRequired(void)
{
    uint32_t objSize = sizeof(RPMessage_Object);
    objSize = ((objSize + HEAPALIGNMENT-1) & ~(HEAPALIGNMENT-1));
    return objSize;
}


/**
 *  \brief RPMessage_announce : Announces the availabilty of an
 *          endpoint to all processors or only one.
 */
int32_t RPMessage_announce(uint32_t remoteProcId, uint32_t endPt, const char* name)
{
    int32_t status = IPC_SOK;
    TIIPC_CmdArgs cargs;

    if (g_IpcFd >= 0)
    {
        cargs.args.announce.remoteProcId = remoteProcId;
        cargs.args.announce.endPt = endPt;

        memset(cargs.args.announce.name, 0x0, SERVICENAMELEN);

        if (name != NULL)
        {
            strncpy(cargs.args.announce.name,
                    name,
                    SERVICENAMELEN-1);
            cargs.args.announce.name[SERVICENAMELEN-1] = '\0';
        }

        status = devctl(g_IpcFd, DCMD_TIIPC_RPMSG_ANNOUNCE, &cargs,
                        sizeof(TIIPC_CmdArgs), NULL);

        if (status != IPC_SOK)
        {
            QNX_PR_ERR("RPMessage_announce: devctl failure %d\n", status);
            status = IPC_EFAIL;
        }
        else
        {
            status = cargs.status;
        }
    }

    return status;
}


/**
 *  \brief RPMessage_getRemoteEndPt
 */
int32_t RPMessage_getRemoteEndPt(uint32_t selfProcId, const char* name,
                                 uint32_t *remoteProcId,
                                 uint32_t *remoteEndPt, uint32_t timeout)
{
    int32_t status = IPC_SOK;
    TIIPC_CmdArgs cargs;

    if (g_IpcFd >=0)
    {
        cargs.args.getremote.selfProcId = selfProcId;
        cargs.args.getremote.remoteProcId = *remoteProcId;
        cargs.args.getremote.remoteEndPt = *remoteEndPt;
        cargs.args.getremote.timeout = timeout;

        memset(cargs.args.getremote.name, 0x0, SERVICENAMELEN);

        if (name != NULL)
        {
            strncpy(cargs.args.getremote.name,
                    name,
                    SERVICENAMELEN-1);
            cargs.args.getremote.name[SERVICENAMELEN-1] = '\0';
        }

        status = devctl(g_IpcFd, DCMD_TIIPC_RPMSG_GETREMOTE, &cargs,
                            sizeof(TIIPC_CmdArgs), NULL);

        if (status != IPC_SOK)
        {
            QNX_PR_ERR("RPMessage_getRemoteEndPt: devctl failed %d\n", status);
            status = IPC_EFAIL;
        }
        else
        {
            status = cargs.status;
            *remoteProcId = cargs.args.getremote.remoteProcId;
            *remoteEndPt = cargs.args.getremote.remoteEndPt;
        }
    }

    return status;
}


/**
 *  \brief RPMessage_init : Initializing the framework
 */
int32_t RPMessage_init(RPMessage_Params *params)
{
    uint32_t retVal = IPC_SOK;

    /* stub - this is handled in resmgr */

    return retVal;
}


/*
 *  ======== RPMessage_create ========
 */
RPMessage_Handle RPMessage_create(RPMessage_Params *params, uint32_t *endPt)
{
    int32_t status;
    RPMessage_Object *obj = NULL;
    RPMessage_Params defaultParams;

    TIIPC_CmdArgs cargs;

    if (g_IpcFd >=0)
    {
        if (params == NULL)
        {
            params = &defaultParams;
            RPMessageParams_init(params);
        }
        cargs.args.create.params = *params;
        cargs.args.create.ept = *endPt;
        status = devctl(g_IpcFd, DCMD_TIIPC_RPMSG_CREATE, &cargs,
                        sizeof(TIIPC_CmdArgs), NULL);
        if (status != IPC_SOK)
        {
            QNX_PR_ERR("RPMessage_create: devctl failed %d\n", status);
        }
        else
        {
            *endPt = cargs.args.create.ept;
            if (cargs.args.create.handle != 0)
            {
                obj = malloc(sizeof(RPMessage_Object));
                if (obj) {
                    obj->handle = cargs.args.create.handle;
                    obj->buf_size = params->bufSize;
                }
            }
        }
    }
    return obj;
}

/**
 *  \brief RPMessage_deinit : Tear down the module
 */
void RPMessage_deInit(void)
{
    /* stub - handled in resmgr */

    return;
}

/*
 *  ======== RPMessage_delete ========
 */
int32_t RPMessage_delete(RPMessage_Handle *handlePtr)
{
    int32_t status = IPC_EFAIL;
    RPMessage_Object *obj = (RPMessage_Object *)(*handlePtr);

    TIIPC_CmdArgs cargs;

    if (g_IpcFd >= 0)
    {
        cargs.args.delete.handle = obj->handle;
        status = devctl(g_IpcFd, DCMD_TIIPC_RPMSG_DELETE, &cargs,
                        sizeof(TIIPC_CmdArgs), NULL);

        if (status != IPC_SOK)
        {
            QNX_PR_ERR("RPMessaage_delete: devctl failed %d\n", status);
        }
        else
        {
            status = cargs.status;
            free(*handlePtr);
            *handlePtr = NULL;
        }
    }

    return status;
}

/*
 *  ======== RPMessage_recv ========
 */
int32_t RPMessage_recv(RPMessage_Handle handle, void* data, uint16_t *len,
                   uint32_t *rplyEndPt, uint32_t *rplyProcId, uint32_t timeout)
{
    int32_t status = IPC_SOK;
    RPMessage_Object *obj = (RPMessage_Object *)handle;
    TIIPC_CmdArgs cargs;
    iov_t rpmsg_recv_iov[2];

    if (g_IpcFd >= 0)
    {
        cargs.args.recv.handle = obj->handle;
        cargs.args.recv.data = NULL;
        cargs.args.recv.timeout = timeout;

        SETIOV(&rpmsg_recv_iov[0], &cargs, sizeof(TIIPC_CmdArgs));
        SETIOV(&rpmsg_recv_iov[1], data, IPC_RPMESSAGE_MSG_BUFFER_SIZE);

        status = devctlv(g_IpcFd, DCMD_TIIPC_RPMSG_RECV, 2, 2,
                         rpmsg_recv_iov, rpmsg_recv_iov, NULL);

        if (status != IPC_SOK)
        {
            QNX_PR_ERR("RPMessage_recv: devctl failed %d\n", status);
            status = IPC_EFAIL;
        }
        else
        {
            status = cargs.status;
            *len = cargs.args.recv.len;
            *rplyEndPt = cargs.args.recv.rplyEndPt;
            *rplyProcId = cargs.args.recv.fromProcId;
        }
    }

    return status;
}

/*
 *  ======== RPMessage_send ========
 */
int32_t RPMessage_send(RPMessage_Handle handle, uint32_t procId, uint32_t dstEndPt,
     uint32_t srcEndPt, void* data, uint16_t len)
{
    int32_t status = IPC_SOK;
    RPMessage_Object *obj = (RPMessage_Object *)handle;
    TIIPC_CmdArgs cargs;
    iov_t rpmsg_send_iov[2];

    if (g_IpcFd >= 0)
    {
        if (obj != NULL)
        {
            cargs.args.send.handle = obj->handle;
        }
        else
        {
            cargs.args.send.handle = NULL;
        }
        cargs.args.send.dstProc = procId;
        cargs.args.send.dstEndPt = dstEndPt;
        cargs.args.send.srcEndPt = srcEndPt;
        cargs.args.send.data = NULL;
        cargs.args.send.len = len;

        SETIOV(&rpmsg_send_iov[0], &cargs, sizeof(TIIPC_CmdArgs));
        SETIOV(&rpmsg_send_iov[1], data, len);

        status = devctlv(g_IpcFd, DCMD_TIIPC_RPMSG_SEND, 2, 1,
                         rpmsg_send_iov, rpmsg_send_iov, NULL);

        if (status != IPC_SOK)
        {
            QNX_PR_ERR("RPMessage_send: devctlv failed %d\n", status);
            status = IPC_EFAIL;
        }
        else
        {
            status = cargs.status;
        }
    }

    return status;
}


/*
 *  ======== RPMessage_unblock ========
 */
void RPMessage_unblock(RPMessage_Handle handle)
{
    int32_t status = IPC_EFAIL;
    RPMessage_Object *obj = (RPMessage_Object *)handle;

    TIIPC_CmdArgs cargs;

    if (g_IpcFd >= 0)
    {
        cargs.args.unblock.handle = obj->handle;
        status = devctl(g_IpcFd, DCMD_TIIPC_RPMSG_UNBLOCK, &cargs,
                        sizeof(TIIPC_CmdArgs), NULL);

        if (status != IPC_SOK)
        {
            QNX_PR_ERR("RPMessage_unblock: devctl failed %d\n", status);
        }
    }
}

/*
 *  ======== RPMessage_printStats ========
 */
void RPMessage_getStats(RPMessage_Stats *ipcStats)
{
    int32_t status = IPC_EFAIL;
    TIIPC_CmdArgs cargs;

    status = devctl(g_IpcFd, DCMD_TIIPC_GETSTATS, &cargs,
                    sizeof(TIIPC_CmdArgs), NULL);
    if (status != IPC_SOK)
    {
        QNX_PR_ERR("RPMessage_printStats: devctl failed %d\n", status);
    }

    if (NULL != ipcStats)
    {
        memcpy((void *)ipcStats, (void *)&cargs.args.getStats.ipcStats, sizeof(RPMessage_Stats));
    }
}

int32_t Ipc_init(const Ipc_InitPrms *cfg)
{
    int32_t retVal = IPC_SOK;

    if (g_IpcFd < 0)
    {
        g_IpcFd = open(TIIPC_DEVICE_NAME, O_RDWR);
        if (g_IpcFd < 0)
        {
            retVal = IPC_EFAIL;
        }
    }

    return retVal;
}

int32_t Ipc_deinit(void)
{
    int32_t  retVal = IPC_SOK;

    if (g_IpcFd > 0)
    {
        close(g_IpcFd);
        g_IpcFd = -1;
    }

    return (retVal);
}
