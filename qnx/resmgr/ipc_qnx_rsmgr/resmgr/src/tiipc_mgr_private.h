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

#ifndef _TI_IPCMGR_PRIVATE_H_INCLUDED
#define _TI_IPCMGR_PRIVATE_H_INCLUDED

#include <stdint.h>
#include <sys/iofunc.h>
#include <ti/drv/ipc/ipc.h>
#include <ti/osal/osal.h>
#include <ti/drv/ipc/src/ipc_utils.h>

#if defined(SOC_J721E)
#define VRING_BASE_ADDRESS      0xAA000000U
#elif defined(SOC_J7200)
#define VRING_BASE_ADDRESS      0xA4000000U
#elif defined (SOC_J721S2)
#define VRING_BASE_ADDRESS      0xA8000000U
#elif defined(SOC_AM62X)
#define VRING_BASE_ADDRESS      0x9C800000U
#elif defined(SOC_AM62A)
#define VRING_BASE_ADDRESS      0xA0000000U
#elif defined (SOC_J784S4)
#define VRING_BASE_ADDRESS      0xAC000000U
#elif defined (SOC_J722S)
#define VRING_BASE_ADDRESS      0xA5000000U
#endif

/* this should be >= RPMessage_getMessageBufferSize() */
#define IPC_RPMESSAGE_MSG_BUFFER_SIZE  (IPC_MAX_DATA_PAYLOAD + 32)

#define ENDPT1                  13U
#define IPC_RPMESSAGE_OBJ_SIZE  256U
#define RPMSG_DATA_SIZE         (256U*IPC_RPMESSAGE_MSG_BUFFER_SIZE + IPC_RPMESSAGE_OBJ_SIZE)
#define VQ_BUF_SIZE             2048U

/* list element for the created handles list */
typedef struct ipc_rpmsg_handle_entry_s
{
    IpcUtils_QElem   elem;
    uint32_t         ept;
    RPMessage_Handle handle;
    uint32_t         rcvid;
} ipc_rpmsg_handle_entry;

typedef struct ipc_waiting_threads_entry_s
{
    IpcUtils_QElem   elem;
    uint32_t         rcvid;
} ipc_waiting_threads_entry;

typedef struct ti_ipc_ocb {
    iofunc_ocb_t ocb;
    pthread_mutex_t lock;
    IpcUtils_QHandle created_handles;
    IpcUtils_QHandle waiting_threads;
} ti_ipc_ocb_t;

#endif

