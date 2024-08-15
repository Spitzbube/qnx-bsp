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

#ifndef _TI_IPCMGR_H_INCLUDED
#define _TI_IPCMGR_H_INCLUDED

#include <stdint.h>
#include "ti/drv/ipc/ipc.h"
#include "ti/drv/ipc/src/ipc_priv.h"

/*
 * The following devctls are used by a client application to access IPC
 */
#include <devctl.h>

#define SERVICENAMELEN         32U

typedef struct TIIPC_CmdArgs {
    union {
        struct {
            RPMessage_Params params;
            uint32_t ept;
            uint32_t handle;
        } create;
        struct {
            uint32_t handle;
        } delete;
        struct {
            uint32_t handle;
            uint32_t dstProc;
            uint32_t dstEndPt;
            uint32_t srcEndPt;
            void     *data;
            uint16_t len;
        } send;
        struct {
            uint32_t handle;
            void *data;
            uint16_t len;
            uint32_t rplyEndPt;
            uint32_t fromProcId;
            uint32_t timeout;
        } recv;
        struct {
            uint32_t handle;
        } unblock;
        struct {
            uint32_t selfProcId;
            char name[SERVICENAMELEN];
            uint32_t remoteProcId;
            uint32_t remoteEndPt;
            uint32_t timeout;
        } getremote;
        struct {
            uint32_t remoteProcId;
            uint32_t endPt;
            char name[SERVICENAMELEN];
        } announce;
        struct {
            uint32_t selfId;
            char     selfName[IPC_MAX_PROC_NAMELEN];
            uint16_t numProc;
            uint32_t procArry[IPC_MAX_PROCS];
            char     names[IPC_MAX_PROCS][IPC_MAX_PROC_NAMELEN];
        } mp_setconfig;
        struct {
            RPMessage_Stats ipcStats;
        } getStats;
    } args;
    uint32_t status;
} TIIPC_CmdArgs;

#define DCMD_TIIPC_RPMSG_SEND              __DIOTF(_DCMD_MISC, 0, TIIPC_CmdArgs)
#define DCMD_TIIPC_RPMSG_RECV              __DIOTF(_DCMD_MISC, 1, TIIPC_CmdArgs)
#define DCMD_TIIPC_RPMSG_CREATE            __DIOTF(_DCMD_MISC, 2, TIIPC_CmdArgs)
#define DCMD_TIIPC_RPMSG_DELETE            __DIOTF(_DCMD_MISC, 3, TIIPC_CmdArgs)
#define DCMD_TIIPC_RPMSG_UNBLOCK           __DIOTF(_DCMD_MISC, 4, TIIPC_CmdArgs)
#define DCMD_TIIPC_RPMSG_GETREMOTE         __DIOTF(_DCMD_MISC, 5, TIIPC_CmdArgs)
#define DCMD_TIIPC_RPMSG_ANNOUNCE          __DIOTF(_DCMD_MISC, 6, TIIPC_CmdArgs)
#define DCMD_TIIPC_RPMSG_GETREMOTEEPT      __DIOTF(_DCMD_MISC, 7, TIIPC_CmdArgs)
#define DCMD_TIIPC_MPSETCONFIG             __DIOTF(_DCMD_MISC, 8, TIIPC_CmdArgs)
#define DCMD_TIIPC_GETSTATS                __DIOTF(_DCMD_MISC, 9, TIIPC_CmdArgs)

#endif

