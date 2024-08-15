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
 *  \file ipc_mp.c
 *
 *  \brief File containing the IPC driver utilities for MultiProc handling.
 *
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ti/drv/ipc/ipc.h>
#include <ti/drv/ipc/include/ipc_mp.h>
#include <ti/drv/ipc/include/ipc_types.h>

#include "psdkqnx_proto.h"
#include "tiipc_mgr.h"
#include "ipc_priv.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */
#define TIIPC_DEVICE_NAME      "/dev/tiipc"

typedef struct Ipc_MpConfig_s
{
    uint32_t    selfProcId;
    /**< own processor id */

    char        name[IPC_MAX_PROC_NAMELEN];
    /**< Name of self processor */

    uint16_t    numProcessors;
    /**< Number of Processors */

    Ipc_ProcInfo procInfo[IPC_MAX_PROCS];
    /**< Array of processors Info */
}Ipc_MpConfig;

static uint8_t g_isIpcMpConfigSetup = FALSE;
static Ipc_MpConfig   g_ipcMpConfig;

static int32_t _Ipc_mpSetConfig(uint32_t selfId, char *selfName, uint16_t numProc,
                                uint32_t *procArry,
                                char names[IPC_MAX_PROCS][IPC_MAX_PROC_NAMELEN]);

int32_t Ipc_mpSetConfig(uint32_t selfId, uint16_t numProc, uint32_t *procArry)
{
    int32_t        retVal = IPC_SOK;
    int32_t        fd = -1;
    uint16_t       i = 0;
    TIIPC_CmdArgs  cargs;

    fd = open(TIIPC_DEVICE_NAME, O_RDWR);
    if (fd >= 0) {
        cargs.args.mp_setconfig.selfId = selfId;
        cargs.args.mp_setconfig.numProc = numProc;
        for (i = 0; i < numProc; i++)
        {
            cargs.args.mp_setconfig.procArry[i] = procArry[i];
        }

        retVal = devctl(fd, DCMD_TIIPC_MPSETCONFIG, &cargs,
                        sizeof(TIIPC_CmdArgs), NULL);

        if (retVal != IPC_SOK)
        {
            QNX_PR_ERR("Ipc_mpSetConfig: devctl failed %d\n", retVal);
        }
        else if (cargs.status != IPC_SOK)
        {
            QNX_PR_ERR("Ipc_mpSetConfig failed: check your config is matching with resgmr\n");
            retVal = cargs.status;
        }
        else
        {
            retVal = _Ipc_mpSetConfig(selfId, cargs.args.mp_setconfig.selfName,
                                      numProc, procArry, cargs.args.mp_setconfig.names);
        }
        close(fd);
    }
    else {
        retVal = IPC_EFAIL;
    }
    return retVal;
}

static int32_t _Ipc_mpSetConfig(uint32_t selfId, char *selfName, uint16_t numProc,
                                uint32_t *procArry,
                                char names[IPC_MAX_PROCS][IPC_MAX_PROC_NAMELEN])
{
    int32_t        retVal = IPC_SOK;
    Ipc_MpConfig  *pMpCfg = &g_ipcMpConfig;
    uint32_t       i = 0;
    uint32_t       id;

    if (g_isIpcMpConfigSetup) {
        if (selfId != pMpCfg->selfProcId || numProc != pMpCfg->numProcessors)
        {
            retVal = IPC_EINVALID_PARAMS;
            QNX_PR_ERR("Ipc_mpSetConfig: supplied config doesn't match with existing config\n");
        }
        else
        {
            for(i = 0; i < numProc; i++)
            {
                if (procArry[i] != pMpCfg->procInfo[i].procId)
                {
                    retVal = IPC_EINVALID_PARAMS;
                    QNX_PR_ERR("Ipc_mpSetConfig: supplied config doesn't match with existing config\n");
                    break;
                }
            }
        }
    }
    else
    {
        if( (selfId >= IPC_MAX_PROCS) || (numProc > IPC_MAX_PROCS) )
        {
            retVal = IPC_EINVALID_PARAMS;
        }
        else
        {
            pMpCfg->selfProcId    = selfId;
            pMpCfg->numProcessors = numProc;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
            strncpy(pMpCfg->name, selfName, IPC_MAX_PROC_NAMELEN-1);
#pragma GCC diagnostic pop
            pMpCfg->name[IPC_MAX_PROC_NAMELEN-1] = '\0';
            for(i = 0; i < numProc; i++)
            {
                id = procArry[i];
                pMpCfg->procInfo[i].procId = id;
                if(id < IPC_MAX_PROCS)
                {
                    strncpy(pMpCfg->procInfo[i].name, names[i], IPC_MAX_PROC_NAMELEN-1);
                    pMpCfg->procInfo[i].name[IPC_MAX_PROC_NAMELEN-1] = '\0';
                }
            }
            g_isIpcMpConfigSetup = TRUE;
        }
    }

    return retVal;
}

uint32_t Ipc_mpGetId(const char* name)
{
    uint32_t       procId = IPC_MP_INVALID_ID;
    uint16_t       i      = 0;
    Ipc_MpConfig  *pMpCfg = &g_ipcMpConfig;

    if( (NULL == name) || (strlen(name) == 0U) )
    {
        /* Invalid parameter */
    }
    else
    {
        for(i = 0; i < pMpCfg->numProcessors; i++)
        {
            if (strncmp(name, pMpCfg->procInfo[i].name, IPC_MAX_PROC_NAMELEN) == 0U)
            {
                procId = pMpCfg->procInfo[i].procId;
            }
        }
    }

    return procId;
}

const char* Ipc_mpGetName(uint32_t id)
{
    char          *name = NULL;
    Ipc_MpConfig  *pMpCfg = &g_ipcMpConfig;
    uint16_t       i;

    if(id >= IPC_MAX_PROCS)
    {
        /* TBD : add failure log */
    }
    else
    {
        if(id ==  pMpCfg->selfProcId)
        {
            name = pMpCfg->name;
        }
        else
        {
            for(i = 0; i < pMpCfg->numProcessors; i++)
            {
                if (pMpCfg->procInfo[i].procId == id)
                {
                    name = (char *)(pMpCfg->procInfo[i].name);
                }
            }
        }
    }

    return (const char*)name;
}

const char* Ipc_mpGetSelfName(void)
{
    Ipc_MpConfig  *pMpCfg = &g_ipcMpConfig;
    return (const char*)pMpCfg->name;
}

uint16_t Ipc_mpGetNumProcessors(void)
{
    return g_ipcMpConfig.numProcessors;
}

uint32_t Ipc_mpGetSelfId(void)
{
    return g_ipcMpConfig.selfProcId;
}

uint32_t Ipc_mpGetRemoteProcId(uint32_t coreIndex)
{
    uint32_t       remoteId = 0xFFU;
    Ipc_MpConfig  *pMpCfg = &g_ipcMpConfig;

    if(coreIndex < g_ipcMpConfig.numProcessors)
    {
        remoteId = pMpCfg->procInfo[coreIndex].procId;
    }

    return remoteId;
}
