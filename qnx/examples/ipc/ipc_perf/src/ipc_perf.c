/*
 * Copyright (c) 2023, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <unix.h>
#include <time.h>

#include <pthread.h>
#include <semaphore.h>

#include <sys/neutrino.h>

#include <ti/drv/ipc/ipc.h>
#include <ti/osal/TaskP.h>
#include "ipc_perf.h"


#define NANO  1000000000L
#define SERVICE  "ti.ipc4.ping-pong"
#define ENDPT1   13
#define NUMMSGS  10 /* number of message sent per task */
#define IPC_MAX_RPROCS (IPC_MAX_PROCS - 2)


enum {
    RTT_MAX_IDX = 0,
    RTT_MIN_IDX,
    RTT_TOT_IDX,
    RTT_IDX_END,
};

#if defined (SOC_AM65XX)
#define CORE_IN_TEST            2
#elif defined (SOC_J721E)
#define CORE_IN_TEST            9
#elif defined (SOC_J7200)
#define CORE_IN_TEST            4
#elif defined (SOC_J721S2)
#define CORE_IN_TEST            8
#elif defined (SOC_AM62X)
#define CORE_IN_TEST            2
#elif defined (SOC_AM62A)
#define CORE_IN_TEST            3
#elif defined (SOC_J784S4)
#define CORE_IN_TEST            12
#elif defined (SOC_J722S)
#define CORE_IN_TEST            5
#endif

/*
 * In the cfg file of R5F, C66x, default heap is 48K which is not
 * enough for 9 task_stack, so creating task_stack on global.
 */

uint8_t  g_task_stack_buf[(CORE_IN_TEST+2)*IPC_TASK_STACKSIZE];

uint8_t  g_cntrl_buf[RPMSG_DATA_SIZE] __attribute__ ((aligned (8)));
uint8_t  sys_vq_buf[VQ_BUF_SIZE]  __attribute__ ((aligned (8)));
uint8_t  g_send_buf[RPMSG_DATA_SIZE * CORE_IN_TEST]  __attribute__ ((aligned (8)));
uint8_t  g_rsp_buf[RPMSG_DATA_SIZE]  __attribute__ ((aligned (8)));

uint8_t *p_cntrl_buf = g_cntrl_buf;
uint8_t *p_task_buf = g_task_stack_buf;
uint8_t *p_send_task_buf = g_send_buf;
uint8_t *p_recv_task_buf = g_rsp_buf;
uint8_t *p_sys_vq_buf = sys_vq_buf;

uint32_t self_id = IPC_MPU1_0;
uint32_t remoteProc[] =
{
#if defined (SOC_AM65XX)
    IPC_MCU1_0, IPC_MCU1_1
#elif defined (SOC_J721E)
    IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU2_1, IPC_MCU3_0, IPC_MCU3_1, IPC_C66X_1, IPC_C66X_2, IPC_C7X_1
#elif defined (SOC_J7200)
    IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU2_1
#elif defined (SOC_J721S2)
    IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU2_1, IPC_MCU3_0, IPC_MCU3_1, IPC_C7X_1, IPC_C7X_2
#elif defined (SOC_AM62X)
    IPC_M4F_0, IPC_MCU1_0
#elif defined (SOC_AM62A)
    IPC_MCU1_0, IPC_C7X_1, IPC_MCU2_0
#elif defined (SOC_J784S4)
    IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU2_1, IPC_MCU3_0, IPC_MCU3_1, IPC_MCU4_0, IPC_MCU4_1, IPC_C7X_1, IPC_C7X_2, IPC_C7X_3, IPC_C7X_4
#elif defined (SOC_J722S)
    IPC_WKUP_R5F, IPC_MCU1_0, IPC_MAIN_R5F, IPC_C7X_1, IPC_C7X_2
#endif
};

uint32_t *p_rproc_array = remoteProc;
uint32_t  g_num_rproc = sizeof(remoteProc)/sizeof(uint32_t);

typedef struct ipc_pref_stats_s {
    uint32_t rproc_id;
    uint32_t message_sz;
    uint32_t num_msgs;
    uint64_t rtt[RTT_IDX_END];
    double   avg_rtt;
}ipc_perf_stats;

pthread_t *g_rproc_threads[IPC_MAX_RPROCS];
uint32_t g_rproc_num[IPC_MAX_RPROCS];
uint32_t g_rproc_skiplist[IPC_MAX_RPROCS]= {0};
uint32_t g_num_msgs = NUMMSGS;
uint32_t g_msg_sz = RPMSG_DATA_SIZE;
uint32_t rpmsg_data_sz = RPMSG_DATA_SIZE;
uint32_t g_verbose = 0;
uint32_t g_skip_rprocs = 0;
int32_t  g_rproc_ut = -1;

ipc_perf_stats g_ipc_stats[IPC_MAX_RPROCS];


void save_stats(uint64_t *rtt_times, int32_t rproc_id)
{
    ipc_perf_stats *p_stats = &g_ipc_stats[rproc_id];
    uint64_t *p_rtt = p_stats->rtt;
    p_stats->rproc_id = rproc_id;
    //p_stats->message_sz = g_msg_sz;
    p_stats->num_msgs = g_num_msgs;
    memcpy(&p_stats->rtt, rtt_times, RTT_IDX_END * sizeof(uint64_t));
    p_stats->avg_rtt = ((double)p_rtt[RTT_TOT_IDX]/(double)(g_num_msgs)); 
}

void print_stats(int32_t rproc_id)
{
    ipc_perf_stats *p_stats = &g_ipc_stats[rproc_id];

    if(g_verbose) {
        printf("IPC STATS FOR REMOTEPROC NAME (%s)\t%d\n", Ipc_mpGetName(rproc_id), rproc_id);
        printf("=======================================================\n");
        printf("=======================================================\n");
        printf("MESSAGE SZ:\t%u\n", p_stats->message_sz);
        printf("NUM MESSAGES:\t%u\n", p_stats->num_msgs);
        printf("MAX RTT(ns):\t%lu\n", p_stats->rtt[RTT_MAX_IDX]);
        printf("MIN RTT(ns):\t%lu\n", p_stats->rtt[RTT_MIN_IDX]);
        printf("TOTAL RTT(ns):\t%lu\n", p_stats->rtt[RTT_TOT_IDX]);
        printf("AVG RTT(us):\t%f\n", (p_stats->avg_rtt/(double)1000));
        printf("=======================================================\n");
        printf("=======================================================\n");
    }

    /* Things that can be put into the datasheet md file. Only avg rtt for now.
     * we might have to expand on this later
     */
    printf("%s| %f\n", Ipc_mpGetName(rproc_id), (p_stats->avg_rtt/(double)1000));
}

void* rpmsg_sender_fxn(void* arg_array)
{
    RPMessage_Handle    handle;
    RPMessage_Params    params;
    uint32_t            myEndPt = 0;
    uint32_t            remoteEndPt = ENDPT1;
    uint32_t            remoteProcId;
    uint16_t            dstProc;
    uint16_t            len;
    int32_t             i;
    int32_t             status = 0;
    char                buf[IPC_RPMESSAGE_MSG_BUFFER_SIZE];
    uint8_t            *buf1;

    uint32_t            cntPing = 0;
    uint32_t            cntPong = 0;
    uint32_t           *arg0;
    uint32_t           *arg1;

    struct timespec start_ts, end_ts, loop_start_ts, loop_end_ts;
    uint64_t rtt_times[RTT_IDX_END]= {0,0,0}; /* MAX, MIN, AVG */
    uint64_t value = 0;

    arg0 = (uint32_t *) *(uint64_t *)arg_array;
    arg1 = (uint32_t *) *(uint64_t *)(arg_array + sizeof(void*));

    buf1 = &p_send_task_buf[rpmsg_data_sz * (*arg1)];
    dstProc = (uint16_t) *arg0;

    if (g_verbose) {
        if (Ipc_mpGetName(p_rproc_array[dstProc]))
            printf("%s: testing for remoteProc %s\n", __func__, Ipc_mpGetName(p_rproc_array[dstProc]));
        else {
            printf("%s: Error testing for remoteProc id - %d \n", __func__, p_rproc_array[dstProc]);
            return NULL;
        }
    }

    remoteProcId = dstProc;
    /* Create the endpoint for receiving. */
    RPMessageParams_init(&params);
    params.numBufs = 2;
    params.buf = buf1;
    params.bufSize = rpmsg_data_sz;
    handle = RPMessage_create(&params, &myEndPt);
    if(!handle)
    {
        printf("SendTask %d: Failed to create message endpoint\n",
                dstProc);
        return(0);
    }

    status = RPMessage_getRemoteEndPt(dstProc, SERVICE, &remoteProcId,
            &remoteEndPt, 100000L);

    if(dstProc != remoteProcId)
    {
        printf("SendTask%d: RPMessage_getRemoteEndPt() malfunctioned, status %d\n",
                dstProc, status);
        return(0);
    }


    if( clock_gettime( CLOCK_REALTIME, &loop_start_ts) == -1 ) {
        perror( "clock gettime start failed ");
    }

    for (i = 0; i < g_num_msgs; i++)
    {
        /* Send data to remote endPt: */
        len = snprintf(buf, IPC_RPMESSAGE_MSG_BUFFER_SIZE-1, "ping %d", i);
        buf[len++] = '\0';

        if (g_verbose)
        {
            printf("SendTask%d: Sending \"%s\" from %s to %s...\n", dstProc,
                    buf, Ipc_mpGetSelfName(),
                    Ipc_mpGetName(dstProc));
        }
        /* Increase the Ping Counter */
        cntPing++;

        if( clock_gettime( CLOCK_REALTIME, &start_ts) == -1 ) {
            perror( "clock gettime start failed ");
        }


        status = RPMessage_send(handle, dstProc, ENDPT1, myEndPt, (Ptr)buf, len);
        if (status != IPC_SOK)
        {
            printf("SendTask%d: rpmsg_sender_fxn: RPMessage_send "
                " failed status %d\n", dstProc, status);
        }

        /* wait a for a response message: */
        status = RPMessage_recv(handle, (Ptr)buf, &len, &remoteEndPt,
                &remoteProcId, IPC_RPMESSAGE_TIMEOUT_FOREVER);

        if(status != IPC_SOK)
        {
            printf("SendTask%d: RPMessage_recv failed with code %d\n",
                    dstProc, status);
        }

        /* Make it NULL terminated string */
        if(len >= IPC_RPMESSAGE_MSG_BUFFER_SIZE)
        {
            buf[IPC_RPMESSAGE_MSG_BUFFER_SIZE-1] = '\0';
        }
        else
        {
            buf[len] = '\0';
        }
        if (g_verbose)
        {
            printf("SendTask%d: Received \"%s\" len %d from %s endPt %d \n",
                    dstProc, buf, len, Ipc_mpGetName(remoteProcId),
                    remoteEndPt);
        }
        cntPong++;

        if( clock_gettime( CLOCK_REALTIME, &end_ts) == -1 ) {
            perror( "clock gettime end failed" );
        }

        value = (( end_ts.tv_sec - start_ts.tv_sec ) * NANO) + ( end_ts.tv_nsec - start_ts.tv_nsec );

        if(g_verbose) {
            printf("start spec %lld.%.9ld\n", (long long)start_ts.tv_sec, start_ts.tv_nsec);
            printf("end spec %lld.%.9ld\n", (long long)end_ts.tv_sec, end_ts.tv_nsec);
            printf("delta time in nanosecs %ld\n", value);
        }

        if(value > rtt_times[RTT_MAX_IDX])
            rtt_times[RTT_MAX_IDX] = value;
        if (value <= rtt_times[RTT_MIN_IDX])
            rtt_times[RTT_MIN_IDX] = value;

         rtt_times[RTT_TOT_IDX] +=value;

        if (g_verbose) {
            if((i+1)%50 == 0)
            {
                printf("%s <--> %s, ping/pong iteration %d ...\n",
                        Ipc_mpGetSelfName(), Ipc_mpGetName(dstProc), i);
            }
        }
    }

    g_ipc_stats[dstProc].message_sz = len;

    if( clock_gettime( CLOCK_REALTIME, &loop_end_ts) == -1 ) {
        perror( "clock gettime start failed ");
    }

    printf("loop start spec %lld.%.9ld\n", (long long)loop_start_ts.tv_sec, loop_start_ts.tv_nsec);
    printf("loop end spec %lld.%.9ld\n", (long long)loop_end_ts.tv_sec, loop_end_ts.tv_nsec);
    int64_t delta_val = (( loop_end_ts.tv_sec - loop_start_ts.tv_sec ) * NANO) + ( loop_end_ts.tv_nsec - loop_start_ts.tv_nsec );
    printf("time in microsecs for %d iterations =  %f\n", g_num_msgs , ((double)delta_val/(g_num_msgs *1000)));

    save_stats(rtt_times, dstProc);
    print_stats(dstProc);
    if (g_verbose) {
        printf("SendTask%d: %s <--> %s, Ping- %d, pong - %d completed\n",
                dstProc, Ipc_mpGetSelfName(),
                Ipc_mpGetName(dstProc),
                cntPing, cntPong);
    }

    /* Delete the RPMesg object now */
    RPMessage_delete(&handle);
    return(0);
}

int run_perf_for_rproc()
{
    void *senderArgs[2];
    TaskP_Params params;
    void *status;

    TaskP_Params_init(&params);
    params.priority = 3;
    senderArgs[0] = &p_rproc_array[g_rproc_ut];
    senderArgs[1] = (void *) &g_rproc_num[g_rproc_ut];
    params.arg0      = (void *) senderArgs;

    TaskP_Handle send_thread = TaskP_create((TaskP_Fxn)&rpmsg_sender_fxn, &params);

    if(send_thread)
        pthread_join(*(pthread_t *)(send_thread), &status);

    return 0;
}

uint32_t skipRemoteProc(uint32_t remoteProcId)
{
    return (g_skip_rprocs && g_rproc_skiplist[remoteProcId]);
}

int32_t Ipc_perf(void)
{
    uint32_t          t;
    /* uint32_t         params[2]; */
    TaskP_Params      params;
    uint32_t          numProc = g_num_rproc;
    Ipc_VirtIoParams  vqParam;
    uint32_t          index = 0;
    void *            senderArgs[numProc][2];
    void *status;


    /* Init skiplist: 0 - for MCU1_0 and 1 - MCU1_1 as defined in the remoteProc (ipc_test.c) array
     * exported as p_rproc_array
     */
    g_rproc_skiplist[0] = 1;
    g_rproc_skiplist[1] = 1;

    /* Step1 : Initialize the multiproc */
    if (IPC_SOK != Ipc_mpSetConfig(self_id, numProc, p_rproc_array))
    {
        printf("IPC_echo_test: Ipc_mpSetConfig failed.\r\n");
        return 0;
    }

    printf("IPC_echo_test running on SoC with %d remote cores - (core : %s) .....\r\n", g_num_rproc,
            Ipc_mpGetSelfName());

    if (IPC_SOK != Ipc_init(NULL))
    {
        printf("IPC_echo_test: Ipc_init failed.\r\n");
        return 0;
    }

    if(g_verbose) {
        for (t = 0; t < g_num_rproc; t++) {
            if (Ipc_mpGetName(p_rproc_array[t]))
                printf("Index = %d Remote Core Name %s Remote core index =%d\n", t, Ipc_mpGetName(p_rproc_array[t]), p_rproc_array[t]);
            else
                printf("Index = %d Remote Core Name NULL Remote core index =%d\n", t, p_rproc_array[t]);
        }
    }

    /* Step2 : Initialize Virtio */
    vqParam.vqObjBaseAddr = (void*)p_sys_vq_buf;
    vqParam.vqBufSize     = numProc * Ipc_getVqObjMemoryRequiredPerCore();
    vqParam.vringBaseAddr = (void*)VRING_BASE_ADDRESS;
    vqParam.vringBufSize  = IPC_VRING_BUFFER_SIZE;
    vqParam.timeoutCnt    = 100;  /* Wait for counts */
    Ipc_initVirtIO(&vqParam);

    if(g_rproc_ut >= 0) { /* This is a test for a single proc */
        if (g_rproc_skiplist[g_rproc_ut] || (g_rproc_ut >= numProc)) {
            printf("ipc_perf not supported for this core %d\n", g_rproc_ut);
            return 1;
        } else {
            printf("ipc_perf testing for remoteProc %s\n", Ipc_mpGetName(p_rproc_array[g_rproc_ut]));
            run_perf_for_rproc();
        }
    } else {

        for(t = 0; t < g_num_rproc; t++, index++)
        {
            if (skipRemoteProc(t)) {
                continue;
            }

            g_rproc_num[t] = t;
            /* send messages to peer(s) on ENDPT1 */
            TaskP_Params_init(&params);
            params.priority  = 3;
            params.stack     = &p_task_buf[index * IPC_TASK_STACKSIZE];
            params.stacksize = IPC_TASK_STACKSIZE;
            senderArgs[t][0] = (void *) &p_rproc_array[t];
            senderArgs[t][1] = (void *) &g_rproc_num[t];
            params.arg0      = (void *) senderArgs[t];
            if (g_verbose)
                printf("creating rpmsg_sender_fxn %d\n", t);
            g_rproc_threads[t] = TaskP_create((TaskP_Fxn)&rpmsg_sender_fxn, &params);
        }

        /* wait for all the threads to join */
        for(t = 0; t < g_num_rproc; t++)
        {
            if (skipRemoteProc(t)) {
                continue;
            }

            if (g_verbose)
                printf("waiting for thread %d\n", t);

            if (*g_rproc_threads[t])
                pthread_join(*(g_rproc_threads[t]), &status);
        }
    }

    printf("Exiting the test successfully!!\n");
    return 0;
}

int main(int argc, char *argv[])
{
    int option;
    extern char *optarg;

    ThreadCtl(_NTO_TCTL_IO, 0);

    while ( (option = getopt(argc, argv, "c:n:m:vs")) != -1)
    {
        switch (option)
        {
            case 'n':
                g_num_msgs = strtoul(optarg, 0, 0);
                break;
            case 'v':
                g_verbose++;
                break;
            case 's':
                g_skip_rprocs = 1;
                break;
            case 'm':
                g_msg_sz = atoi(optarg);
                break;
            case 'c':
                g_rproc_ut = strtoul(optarg, 0, 0);
                break;
            default:
                fprintf(stderr,"Unsupported option '-%c'\n",option);
        }
    }

    Ipc_perf();
    return EXIT_SUCCESS;
}
