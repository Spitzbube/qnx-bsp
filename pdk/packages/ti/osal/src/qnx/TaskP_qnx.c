/*
 * Copyright (c) 2019-2021, Texas Instruments Incorporated
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
/*
 *  ======== TaskP_qnx.c ========
 */

#include <ti/osal/TaskP.h>

#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <stdlib.h>

#include <ti/osal/osal.h>
/*
 *  ======== TaskP_create ========
 */
TaskP_Handle TaskP_create(TaskP_Fxn taskfxn, const TaskP_Params *params)
{
    pthread_attr_t attr;
    int            status;
    TaskP_Handle retVal = (TaskP_Handle)NULL;
    pthread_t *tid = (pthread_t *)calloc(1, sizeof(pthread_t));

    pthread_attr_init(&attr);

    if((TaskP_Params *)NULL != params)
    {
        pthread_attr_setstacksize(&attr, params->stacksize);
        if(NULL != params->stack)
        {
            pthread_attr_setstackaddr(&attr, params->stack);
        }
        if (0 != params->priority)
        {
            struct sched_param  param;
            param.sched_priority = params->priority;
            pthread_attr_setschedparam(&attr, &param);
            if (params->priority < 10U)
            {
                DebugP_log1("Thread Priority set < 10, the value is %d!", params->priority);
            }
        }
        else
        {
            DebugP_log0("Thread Priority passed is 0!");
            OSAL_Assert(1);
        }
    }

    status = pthread_create(tid, &attr, (void *)taskfxn, (void *)(params->arg0));

    if(EOK == status)
    {
        retVal = ((TaskP_Handle) tid);
    }

    pthread_attr_destroy(&attr);
    return retVal;
}

/*
 *  ======== TaskP_delete ========
 */
TaskP_Status TaskP_delete(TaskP_Handle *handle)
{
    struct timespec timeout;
    TaskP_Status return_status = TaskP_OK;
    void *value_ptr;
    int status;

    timeout.tv_sec = 0;
    timeout.tv_nsec = 100000000; //100 ms

    if (NULL != *handle)
    {
        pthread_t *tid = (pthread_t *)(*handle);
        if (tid != NULL) {
            pthread_cancel(*tid);
            status = pthread_timedjoin(*tid, &value_ptr, &timeout);
            if(EOK != status) {
                return_status = TaskP_FAILURE;
                DebugP_log3("TaskP_delete on pid %d tid %d failed...join status=%d", getpid(), *tid, status);
            }
            else {
                free(tid);
                *handle = NULL;
            }
        }
    }
    return return_status;
}

/*
 *  ======== TaskP_Params_init ========
 */
void TaskP_Params_init(TaskP_Params *params)
{
    params->name = (const char *) NULL;
    params->pErrBlk = (void *) NULL;
    params->priority = 10U; /* Set default priority as 10 for QNX */
    params->stacksize = 16U * 1024U; /* Set default stacksize as 16K for QNX  */
    params->arg0 = (void *) NULL;
    params->arg1 = (void *) NULL;
    params->stack = (void *) NULL;
    params->userData = NULL;
    params->taskPrivilege = 0U; /* Not Used */
}

/*
 *  ======== TaskP_sleep ========
 */
void TaskP_sleep(uint32_t timeout)
{
    delay(timeout);
}

/*
 *  ======== TaskP_sleepInMsecs ========
 */
void TaskP_sleepInMsecs(uint32_t timeoutInMsecs)
{
    uint32_t ticks;
    uint32_t Clock_tickPeriod = 1000U;

    /* Clock_tickPeriod is in units of usecs */
    ticks = ((uint64_t)timeoutInMsecs * 1000U) / Clock_tickPeriod;

    TaskP_sleep(ticks);
}

/*
 *  ======== TaskP_setPrio ========
 */
void TaskP_setPrio(TaskP_Handle handle, uint32_t priority)
{
    return;
}

/*
 *  ======== TaskP_self ========
 */
TaskP_Handle TaskP_self()
{
    pthread_t taskHandle;
    pthread_t *handle = (pthread_t *)calloc(1, sizeof(pthread_t));

    taskHandle = pthread_self();
    *handle = taskHandle;

    return ((TaskP_Handle) handle);
}

/*
 *  ======== TaskP_yield ========
 */
void TaskP_yield(void) {
    sched_yield();
}

/*
 *  ======== TaskP_isTerminated ========
 */
uint32_t TaskP_isTerminated(TaskP_Handle handle)
{
    /* Not supported by QNX OSAL as of PSDK QNX 10.0 */
    return 0;
}

/* Nothing past this point */

