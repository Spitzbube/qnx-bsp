/*
 *  Copyright (c) Texas Instruments Incorporated 2018-2021
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
 *  \file ipc_testsetup.c
 *
 *  \brief IPC  example code
 *
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unix.h>

#include <stdio.h>
#include <sys/neutrino.h>
#include <stdlib.h>

#include <ti/drv/ipc/ipc.h>
#include <ti/osal/TaskP.h>
#include "ipc_setup.h"

#define SERVICE  "ti.ipc4.ping-pong"
#define ENDPT1   13
#define NUMMSGS  10 /* number of message sent per task */
//#define NUMMSGS  1000000   /* number of message sent per task */

struct sendFxnArgs {
    uint32_t *cntPing;
    RPMessage_Handle RPHandle;
    uint16_t dstProc;
    uint32_t myEndPt;
    sem_t *semHandle;
};

extern uint8_t  *pCntrlBuf;
extern uint8_t  *pTaskBuf;
extern uint8_t  *pSendTaskBuf;
extern uint8_t  *pRecvTaskBuf;
extern uint8_t  *pSysVqBuf;

extern uint32_t  selfProcId;
extern uint32_t *pRemoteProcArray;
extern uint32_t  gNumRemoteProc;

uint32_t gNumMsgs = NUMMSGS;
uint32_t gTimeOut = 1000000L;
uint32_t gNegative = 0;
uint32_t gVerbose = 0;
uint32_t gSkipRps = 0;
pthread_t *gRpThreads[IPC_MAX_PROCS];
uint32_t gRemoteProcNum[IPC_MAX_PROCS];
uint32_t gRpSkipList[IPC_MAX_PROCS]= {0};
uint32_t gThreadedSendReceive = 0;

uint32_t gTotalPings = 0;
pthread_mutex_t gMutex;

uint32_t rpmsgDataSize = RPMSG_DATA_SIZE;

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */


/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */


/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */
// #define DEBUG_PRINT

bool g_exitRespTsk = 0;

void* rpmsg_exit_responseTask()
{
    g_exitRespTsk = 1;
    return(0);
}

void* rpmsg_ipcTestFxn(void* arg_array)
{
    RPMessage_Handle    handle;
    RPMessage_Params    params;
    uint32_t            myEndPt = 0;
    uint32_t            remoteEndPt = ENDPT1;
    uint32_t            remoteProcId;
    uint16_t            dstProc;
    uint16_t            sendLen, recvLen;
    int32_t             i;
    int32_t             status = 0;
    char                buf[IPC_RPMESSAGE_MSG_BUFFER_SIZE];
    uint8_t            *buf1;

    uint32_t            cntPing = 0;
    uint32_t            cntPong = 0;
    uint32_t           *arg0;
    uint32_t           *arg1;

    int32_t             retValue = 0;
    uint32_t            recvTimeout = (gSkipRps && (gTimeOut != -1)) ? gTimeOut : IPC_RPMESSAGE_TIMEOUT_FOREVER;

    arg0 = (uint32_t *) *(uint64_t *)arg_array;
    arg1 = (uint32_t *) *(uint64_t *)(arg_array + sizeof(void*));

    buf1 = &pSendTaskBuf[rpmsgDataSize * (*arg1)];
    dstProc = (uint16_t) *arg0;

    remoteProcId = dstProc;
    /* Create the endpoint for receiving. */
    RPMessageParams_init(&params);
    params.numBufs = 2;
    params.buf = buf1;
    params.bufSize = rpmsgDataSize;
    handle = RPMessage_create(&params, &myEndPt);
    if(!handle)
    {
        printf("SendTask %d: Failed to create message endpoint\n",
                dstProc);
        pthread_exit((void*)(intptr_t)0);
    }

    status = RPMessage_getRemoteEndPt(dstProc, SERVICE, &remoteProcId,
            &remoteEndPt, 100000L);

    if(dstProc != remoteProcId)
    {
        printf("SendTask%d: RPMessage_getRemoteEndPt() malfunctioned, status %d\n",
                dstProc, status);
        pthread_exit((void*)(intptr_t)0);
    }

    for (i = 0; i < gNumMsgs; i++)
    {
        /* Send data to remote endPt: */
        sendLen = snprintf(buf, IPC_RPMESSAGE_MSG_BUFFER_SIZE-1, "ping %d", i);
        buf[sendLen++] = '\0';

        if (gVerbose)
        {
            printf("SendTask%d: Sending \"%s\" from %s to %s...\n", dstProc,
                    buf, Ipc_mpGetSelfName(),
                    Ipc_mpGetName(dstProc));
        }

        status = RPMessage_send(handle, dstProc, ENDPT1, myEndPt, (Ptr)buf, sendLen);
        if (status != IPC_SOK)
        {
            printf("SendTask%d: rpmsg_senderFxn: RPMessage_send "
                " failed status %d\n", dstProc, status);
        }

        if (status == IPC_SOK)
        {
            /* Increase the Ping Counter */
            cntPing++;

            /* wait a for a response message: */
            status = RPMessage_recv(handle, (Ptr)buf, &recvLen, &remoteEndPt,
                    &remoteProcId, recvTimeout);

            if(status != IPC_SOK)
            {
                printf("SendTask%d: RPMessage_recv failed with code %d\n",
                        dstProc, status);
            }
            else
            {
                if (gVerbose)
                {
                    printf("SendTask%d: RPMessage_recv returned with code %d\n",
                            dstProc, status);
                }
                cntPong++;
            }

            /* Make it NULL terminated string */
            if(recvLen >= IPC_RPMESSAGE_MSG_BUFFER_SIZE)
            {
                buf[IPC_RPMESSAGE_MSG_BUFFER_SIZE-1] = '\0';
            }
            else
            {
                buf[recvLen] = '\0';
            }

            if (gVerbose)
            {
                printf("SendTask%d: Received \"%s\" len %d from %s endPt %d, status: %d\n",
                        dstProc, buf, recvLen, Ipc_mpGetName(remoteProcId),
                        remoteEndPt, status);
            }

            if (recvLen != sendLen)
            {
                printf("SendTask%d: Recieved mismatched string lengths sent:%d recieved: %d status: %d\n", dstProc, sendLen, recvLen, status);
            }
        }

        if((i+1)%50 == 0)
        {
            printf("%s <--> %s, ping/pong iteration %d ...\n",
                    Ipc_mpGetSelfName(), Ipc_mpGetName(dstProc), i);
        }
    }

    printf("SendTask%d: %s <--> %s, Ping- %d, pong - %d completed\n",
            dstProc, Ipc_mpGetSelfName(),
            Ipc_mpGetName(dstProc),
            cntPing, cntPong);

    /* Delete the RPMesg object now */
    RPMessage_delete(&handle);
    retValue = cntPing == cntPong && cntPing == gNumMsgs;

    if (retValue != 1)
    {
        printf("SendTask%d: %s <--> %s failure\n",
            dstProc,
            Ipc_mpGetSelfName(),
            Ipc_mpGetName(dstProc));
    }

    /* Record the number of pings */
    pthread_mutex_lock(&gMutex);
    gTotalPings += cntPong;
    pthread_mutex_unlock(&gMutex);

    pthread_exit((void*)(intptr_t)retValue);
}

void* rpmsg_senderFxn(void *arg)
{
    uint16_t            len;
    int32_t             i;
    int32_t             status = 0;
    char                buf[IPC_RPMESSAGE_MSG_BUFFER_SIZE];
    int32_t             retValue = 0;
    struct sendFxnArgs *senderArgs = (struct sendFxnArgs *)arg;

    for (i = 0; i < gNumMsgs; i++)
    {
        /* Send data to remote endPt: */
        len = snprintf(buf, IPC_RPMESSAGE_MSG_BUFFER_SIZE-1, "ping %d", i);
        buf[len++] = '\0';

        if (gVerbose)
        {
            printf("SendTask%d: Sending \"%s\" from %s to %s...\n", senderArgs->dstProc,
                    buf, Ipc_mpGetSelfName(),
                    Ipc_mpGetName(senderArgs->dstProc));
        }

        status = RPMessage_send(senderArgs->RPHandle, senderArgs->dstProc, ENDPT1, senderArgs->myEndPt, (Ptr)buf, len);
        if (status != IPC_SOK)
        {
            printf("SendTask%d: rpmsg_senderFxn: RPMessage_send "
                " failed status %d\n", senderArgs->dstProc, status);
        }

        if (status == IPC_SOK)
        {
            /* Increase the Ping Counter */
            (*(senderArgs->cntPing))++;
        }

        if((i+1)%50 == 0)
        {
            printf("%s <--> %s, ping/pong iteration %d ...\n",
                    Ipc_mpGetSelfName(), Ipc_mpGetName(senderArgs->dstProc), i);
        }

        /* Sleep for 1 ms to prevent flooding */
        TaskP_sleepInMsecs(1);
    }

    printf("SendTask%d: %s <--> %s, %d pings completed\n",
            senderArgs->dstProc, Ipc_mpGetSelfName(),
            Ipc_mpGetName(senderArgs->dstProc),
            *senderArgs->cntPing);

    /* Sleep for 1 second before posting semaphore at the end of test to tell recv thread
       to close even if it hasn't received all the messages. */
    TaskP_sleep(1000);

    sem_post(senderArgs->semHandle);

    pthread_exit((void*)(intptr_t)retValue);
}


void* rpmsg_receiverFxn(void* arg_array)
{
    RPMessage_Handle    handle;
    RPMessage_Params    RPMessageParams;
    uint32_t            myEndPt = 0;
    uint32_t            remoteEndPt = ENDPT1;
    uint32_t            remoteProcId;
    uint16_t            dstProc;
    uint16_t            recvLen;
    int32_t             i, recvPingIdx;
    int32_t             status = 0;
    int32_t             sscanStatus = 0;
    int32_t             testOverStatus;
    char                buf[IPC_RPMESSAGE_MSG_BUFFER_SIZE];
    uint8_t            *buf1;
    sem_t               sendSem;
    TaskP_Handle        sendTaskHandle;

    TaskP_Params        params;
    struct sendFxnArgs *senderArgs;

    uint32_t            cntPing = 0;
    uint32_t            cntPong = 0;
    uint32_t           *arg0;
    uint32_t           *arg1;
    uint64_t            index;

    int32_t             retValue = 0;
    int32_t             printedRecvStr = 0;
    uint32_t            recvTimeout = (gSkipRps && (gTimeOut != -1)) ? gTimeOut : IPC_RPMESSAGE_TIMEOUT_FOREVER;

    arg0 = (uint32_t *) *(uint64_t *)arg_array;
    arg1 = (uint32_t *) *(uint64_t *)(arg_array + sizeof(void*));
    index = (uint64_t) *(uint64_t *)(arg_array + (2 * sizeof(void*)));

    buf1 = &pSendTaskBuf[rpmsgDataSize * (*arg1)];
    dstProc = (uint16_t) *arg0;

    remoteProcId = dstProc;
    /* Create the endpoint for receiving. */
    RPMessageParams_init(&RPMessageParams);
    RPMessageParams.numBufs = 16;
    RPMessageParams.buf = buf1;
    RPMessageParams.bufSize = rpmsgDataSize;
    handle = RPMessage_create(&RPMessageParams, &myEndPt);
    if(!handle)
    {
        printf("RecvTask %d: Failed to create message endpoint\n",
                dstProc);
        pthread_exit((void*)(intptr_t)0);
    }

    status = RPMessage_getRemoteEndPt(dstProc, SERVICE, &remoteProcId,
            &remoteEndPt, 100000L);

    if(dstProc != remoteProcId)
    {
        printf("RecvTask%d: RPMessage_getRemoteEndPt() malfunctioned, status %d\n",
                dstProc, status);
        pthread_exit((void*)(intptr_t)0);
    }

    /* Create semaphore for sender to signal end of messages */
    status = sem_init(&sendSem, 0, 0);
    if(status)
    {
        printf("RecvTask%d: Error creating semaphore\n",
                dstProc);
        pthread_exit((void*)(intptr_t)0);
    }

    /* Create sender thread */
    senderArgs = (struct sendFxnArgs *)malloc(sizeof(struct sendFxnArgs));
    senderArgs->cntPing = &cntPing;
    senderArgs->RPHandle = handle;
    senderArgs->dstProc = dstProc;
    senderArgs->myEndPt = myEndPt;
    senderArgs->semHandle = &sendSem;

    TaskP_Params_init(&params);
    params.priority  = 10;
    params.stack     = &pTaskBuf[(index * 2 + 1) * IPC_TASK_STACKSIZE];
    params.stacksize = IPC_TASK_STACKSIZE;
    params.arg0      = (void *)senderArgs;
    printf("Creating sender thread %d for Core %s\n", dstProc, Ipc_mpGetName(dstProc));
    sendTaskHandle = TaskP_create((TaskP_Fxn)&rpmsg_senderFxn, &params);
    if (NULL == sendTaskHandle)
    {
        printf("RecvTask %d: Failed to create send thread\n", dstProc);
        pthread_exit((void*)(intptr_t)0);
    }

    for (i = 0; i < gNumMsgs; i++)
    {
        status = -1;
        printedRecvStr = 0;

        /* Inevitably some receives will be timeouts in threaded model...keep
        looping while we wait to get them */
        while (IPC_SOK != status)
        {
            /* wait a for a response message: */
            status = RPMessage_recv(handle, (Ptr)buf, &recvLen, &remoteEndPt,
                    &remoteProcId, recvTimeout);

            if(status != IPC_SOK)
            {
                printf("RecvTask%d: RPMessage_recv failed with code %d\n",
                        dstProc, status);
            } else {

                /* Make it NULL terminated string */
                if(recvLen >= IPC_RPMESSAGE_MSG_BUFFER_SIZE)
                {
                    buf[IPC_RPMESSAGE_MSG_BUFFER_SIZE-1] = '\0';
                }
                else
                {
                    buf[recvLen] = '\0';
                }

                /* Increment pong count if it was the expected message */
                sscanStatus = sscanf(buf, "pong %d", &recvPingIdx);
                if (1 == sscanStatus)
                {
                    if (recvPingIdx != i)
                    {
                        printf("RecvTask%d: Recieved \"pong %d\" expected \"pong %d\" from %s endPt %d, status: %d\n",
                                dstProc, recvPingIdx, i, Ipc_mpGetName(remoteProcId), remoteEndPt, status);
                        printedRecvStr = 1;
                        i = recvPingIdx;
                    }
                    else
                    {
                        cntPong++;
                    }
                }

                if (gVerbose && !printedRecvStr)
                {
                    printf("RecvTask%d: Received \"%s\" len %d from %s endPt %d, status: %d\n",
                            dstProc, buf, recvLen, Ipc_mpGetName(remoteProcId),
                            remoteEndPt, status);
                }

                /* Reset status regardless of if it was the right message. */
                status = IPC_SOK;
            }

            memset(buf, 0, sizeof(buf));

            /* See if send thread is exiting. If so then messages were missed. */
            testOverStatus = sem_trywait(&sendSem);
            if (0 == testOverStatus)
            {
                printf("RecvTask%d: Sender thread posted exit semaphore...exiting early while expecting \"pong %d\"\n",
                       dstProc, i);
                sem_destroy(&sendSem);
                pthread_exit(0);
            }
        }

        if((i+1)%50 == 0)
        {
            printf("%s <--> %s, ping/pong iteration %d ...\n",
                    Ipc_mpGetSelfName(), Ipc_mpGetName(dstProc), i);
        }
    }

    printf("RecvTask%d: %s <--> %s, Ping- %d, pong - %d completed\n",
            dstProc, Ipc_mpGetSelfName(),
            Ipc_mpGetName(dstProc),
            cntPing, cntPong);

    /* Delete the RPMesg object now */
    RPMessage_delete(&handle);
    retValue = cntPing == cntPong && cntPing == gNumMsgs;

    if (retValue != 1)
    {
        printf("RecvTask%d: %s <--> %s failure\n",
            dstProc,
            Ipc_mpGetSelfName(),
            Ipc_mpGetName(dstProc));
    }

    /* Record the number of pings */
    pthread_mutex_lock(&gMutex);
    gTotalPings += cntPong;
    pthread_mutex_unlock(&gMutex);

    sem_destroy(&sendSem);

    pthread_exit((void*)(intptr_t)retValue);
}

uint32_t skipRemoteProc(uint32_t remoteProcId)
{
    return (gSkipRps && gRpSkipList[remoteProcId]);
}

int32_t Ipc_echo_test(void)
{
    uint32_t            t;
    /* uint32_t         params[2]; */
    TaskP_Params        params;
    uint32_t            numProc = gNumRemoteProc;
    Ipc_VirtIoParams    vqParam;
    uint64_t            index = 0;
    void *              senderArgs[numProc][3];
    int32_t             status;
    int32_t             passedTests = 1;
    pthread_mutexattr_t mattr;
    RPMessage_Stats     ipcStats;


    pthread_mutexattr_init(&mattr);
    pthread_mutex_init(&gMutex, &mattr);
    pthread_mutexattr_destroy (&mattr);

    /* Init skiplist: 0 - for MCU1_0 and 1 - MCU1_1 as defined in the remoteProc (ipc_test.c) array
     * exported as pRemoteProcArray
     */
    gRpSkipList[0] = 1;
#if ! defined(SOC_J722S)
    gRpSkipList[1] = 1;
#endif



    /* Step1 : Initialize the multiproc */
    if (IPC_SOK != Ipc_mpSetConfig(selfProcId, numProc, pRemoteProcArray))
    {
        printf("%s:%d: Ipc_mpSetConfig failed.\r\n", __FUNCTION__, __LINE__);
        return -1;
    }

    printf("IPC_echo_test (core : %s) .....\r\n",
            Ipc_mpGetSelfName());

    if (IPC_SOK != Ipc_init(NULL))
    {
        printf("IPC_echo_test: Ipc_init failed.\r\n");
        return -1;
    }


    /* Step2 : Initialize Virtio */
    vqParam.vqObjBaseAddr = (void*)pSysVqBuf;
    vqParam.vqBufSize     = numProc * Ipc_getVqObjMemoryRequiredPerCore();
    vqParam.vringBaseAddr = (void*)VRING_BASE_ADDRESS;
    vqParam.vringBufSize  = IPC_VRING_BUFFER_SIZE;
    vqParam.timeoutCnt    = 100;  /* Wait for counts */
    Ipc_initVirtIO(&vqParam);

    if (gVerbose)
    {
        printf("Sending RPMessage_printStats\n");
        RPMessage_getStats(&ipcStats);
    }

    for(t = 0; t < gNumRemoteProc; t++, index++)
    {
        if (skipRemoteProc(t)) {
            continue;
        }

        gRemoteProcNum[t] = t;
        /* send messages to peer(s) on ENDPT1 */
        TaskP_Params_init(&params);
        params.priority  = 10;
        params.stack     = &pTaskBuf[(index * 2) * IPC_TASK_STACKSIZE];
        params.stacksize = IPC_TASK_STACKSIZE;
        senderArgs[t][0] = (void *) &pRemoteProcArray[t];
        senderArgs[t][1] = (void *) &gRemoteProcNum[t];
        senderArgs[t][2] = (void *) index;
        params.arg0      = (void *) senderArgs[t];

        if (0 == gThreadedSendReceive)
        {
            printf("Creating thread %d for Core %s\n", t, Ipc_mpGetName(pRemoteProcArray[t]));
            gRpThreads[t] = TaskP_create((TaskP_Fxn)&rpmsg_ipcTestFxn, &params);
        }
        else if (1 == gThreadedSendReceive)
        {
            printf("Creating receiver thread %d for Core %s\n", t, Ipc_mpGetName(pRemoteProcArray[t]));
            gRpThreads[t] = TaskP_create((TaskP_Fxn)&rpmsg_receiverFxn, &params);
        }
    }

    /* wait for all the threads to join */
    for(t = 0; t < gNumRemoteProc; t++)
    {
        if (skipRemoteProc(t)) {
            continue;
        }

        printf("waiting for thread %d\n", t);
        if (*gRpThreads[t])
            pthread_join(*(gRpThreads[t]), (void**)&status);
        passedTests &= status;
    }

    if (gVerbose)
    {
        printf("Sending RPMessage_printStats\n");
        RPMessage_getStats(&ipcStats);
    }

    if (passedTests)
    {
        printf("Exiting the test successfully!!\n");
        return 0;
    }
    else
    {
        if (gNegative && gTotalPings == 0) {
            printf("Exiting the test successfully!!\n");
        }
        else
        {
            printf("!!!Exiting the test with failures!!!\n");
        }
        return 1;
    }

    pthread_mutex_destroy(&gMutex);
}
