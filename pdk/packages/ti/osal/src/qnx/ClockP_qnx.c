/*
 * Copyright (c) 2024, Texas Instruments Incorporated
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
 *  ======== ClockP_qnx.c ========
 */

#include <ti/osal/ClockP.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ti/csl/csl_types.h>
#include <ti/osal/osal.h>
#include <ti/osal/DebugP.h>
#include <ti/osal/soc/osal_soc.h>

#include <time.h>

#include <sys/slog.h>
#include <sys/slogcodes.h>

#include <sys/neutrino.h>
#include <sys/dispatch.h>
#include <sys/netmgr.h>

#define QNX_OSAL_MAX_CLOCK_COUNT           (16U)
#define QNX_OSAL_CLOCK_PULSE_CODE          _PULSE_CODE_MINAVAIL
#define NS_PER_SECOND                      (1000000000UL)

#define _SLOGC_PRIVATE_OSAL (_SLOGC_PRIVATE_START + 100)

typedef struct ClockP_qnx_s
{
    bool                used;
    timer_t             timer_id;
    struct itimerspec   itime;
    ClockP_FxnCallback  callback;
    void                *arg;
} ClockP_qnx;

/* global pool of statically allocated clock pools */
static ClockP_qnx gOsalClockPQnxPool[QNX_OSAL_MAX_CLOCK_COUNT];

/* Dispath Handler */
dispatch_t *dpp = NULL;

int chid = -1;

uint32_t gOsalClockAllocCnt = 0U, gOsalClockPeak = 0U;

/*!
 *  @brief  Configure the interval for a given ClockP_qnx with a params.
 *
 *  @param params [in] Parameters with the requested interval for a timer.
 *  @param pTimer [in/out] ClockP_qnx to be modified with the requested interval.
 *
 *  @return Status if tick conversion was successful (0 for success).
 */
int ClockP_configureIntervalFromParams(const ClockP_Params *params, ClockP_qnx *pTimer)
{
    struct _clockperiod oldPeriod;
    int status = 0;

    status = ClockPeriod(CLOCK_MONOTONIC, NULL, &oldPeriod, sizeof(oldPeriod));
    if(-1 != status)
    {
        pTimer->itime.it_value.tv_sec  = (time_t) (((unsigned long)params->period * (unsigned long)oldPeriod.nsec) / NS_PER_SECOND);
        pTimer->itime.it_value.tv_nsec = (long)   (((unsigned long)params->period * (unsigned long)oldPeriod.nsec) % NS_PER_SECOND);

        if (ClockP_RunMode_CONTINUOUS == params->runMode)
        {
            pTimer->itime.it_interval.tv_sec  = pTimer->itime.it_value.tv_sec;
            pTimer->itime.it_interval.tv_nsec = pTimer->itime.it_value.tv_nsec;
        }
    }
    else
    {
        slogf(_SLOGC_PRIVATE_OSAL, _SLOG_ERROR, "%s:%d: Error retrieving OS tick period", __FUNCTION__, __LINE__);
    }

    return status;
}

/* Calls the registered callback function when a timer expires */
int ClockP_timerPulseHandler(message_context_t * ctp, int code, unsigned flags, void * handle)
{
    ClockP_qnx *pTimer = (ClockP_qnx *)handle;
    int status = -1;

    if( (NULL_PTR != pTimer) && (pTimer->callback) )
    {
        pTimer->callback(pTimer->arg);
        status = 0;
    }

    return status;
}

void ClockP_Params_init(ClockP_Params *params)
{
    if(NULL_PTR != params)
    {
        params->pErrBlk     = NULL_PTR;
        params->startMode   = ClockP_StartMode_USER;
        params->period      = 0U;
        params->runMode     = ClockP_RunMode_ONESHOT;
        params->arg         = NULL_PTR;
    }

    return;
}

/*
 *  ======== ClockP_create ========
 */
ClockP_Handle ClockP_create(ClockP_FxnCallback clockfxn,
                            const ClockP_Params *params)
{

    ClockP_qnx *pTimer = (ClockP_qnx *)NULL_PTR;
    ClockP_qnx *timerPool;
    ClockP_Handle ret_handle = NULL_PTR;
    uint32_t i;
    uintptr_t key;
    uint32_t maxClocks;
    int status = 0, priority;
    struct sigevent event;
    struct sched_param scheduling_params;
    int coid;
    int flag = 0;

    slogf(_SLOGC_PRIVATE_OSAL, _SLOG_ERROR, "%s:%d: Creating clock for pid %d", __FUNCTION__, __LINE__, getpid());

    /* Pick up the internal static memory block */
    timerPool = (ClockP_qnx *) &gOsalClockPQnxPool[0];
    maxClocks  = QNX_OSAL_MAX_CLOCK_COUNT;

    /* Get our priority. */
    if (SchedGet( 0, 0, &scheduling_params) != -1)
    {
       priority = scheduling_params.sched_priority;
    }
    else
    {
       priority = 10;
    }

    if(0U == gOsalClockAllocCnt)
    {
        (void)memset((void *)gOsalClockPQnxPool,0,sizeof(gOsalClockPQnxPool));
    }

    if(NULL == dpp)
    {
        dpp = dispatch_create();
        if(NULL == dpp)
        {
            status = -1;
        }
    }

    key = HwiP_disable();

    for (i = 0U; i < maxClocks; i++)
    {
        if (BFALSE == timerPool[i].used)
        {
            timerPool[i].used = BTRUE;
            /* Update statistics */
            gOsalClockAllocCnt++;
            if (gOsalClockAllocCnt > gOsalClockPeak)
            {
                gOsalClockPeak = gOsalClockAllocCnt;
            }
            break;
        }
    }
    HwiP_restore(key);

    if (i < maxClocks)
    {
        /* Grab the memory */
        pTimer = (ClockP_qnx *) &timerPool[i];
    }

    if((NULL_PTR == pTimer) || (NULL_PTR == params))
    {
        ret_handle = NULL_PTR;
    }
    else
    {
        pTimer->callback = (ClockP_FxnCallback)clockfxn;
        pTimer->arg = params->arg;

        if(-1 == chid)
        {
            chid = ChannelCreate(0);
            if(-1 == chid)
            {
                status = -1;
                slogf(_SLOGC_PRIVATE_OSAL, _SLOG_ERROR, "%s:%d: Failed to create channel!", __FUNCTION__, __LINE__);
            }
        }

        if(0 == status)
        {
            coid = ConnectAttach(ND_LOCAL_NODE, 0, chid, _NTO_SIDE_CHANNEL, 0);
            if(-1 == coid)
            {
                status = -1;
                slogf(_SLOGC_PRIVATE_OSAL, _SLOG_ERROR, "%s:%d: Error creating connection id!", __FUNCTION__, __LINE__);
            }
        }

        if(0 == status)
        {
            SIGEV_PULSE_INIT(&event, coid, priority, QNX_OSAL_CLOCK_PULSE_CODE + i, 0);

            status = timer_create(CLOCK_MONOTONIC, &event, &pTimer->timer_id);
            if(0 != status)
            {
                slogf(_SLOGC_PRIVATE_OSAL, _SLOG_ERROR, "%s:%d: timer_create() failed!", __FUNCTION__, __LINE__);

                /* If there was an error reset the clock object and return NULL. */
                key = HwiP_disable();
                pTimer->used      = BFALSE;
                /* Found the osal clock object to delete */
                if (0U < gOsalClockAllocCnt)
                {
                    gOsalClockAllocCnt--;
                }
                HwiP_restore(key);
                ret_handle = NULL_PTR;
            }
        }

        if(0 == status)
        {
            status = pulse_attach(dpp, flag, QNX_OSAL_CLOCK_PULSE_CODE + i, &ClockP_timerPulseHandler, &pTimer);
            if(0 != status)
            {
                slogf(_SLOGC_PRIVATE_OSAL, _SLOG_ERROR, "%s:%d: pulse_attach failed! %d", __FUNCTION__, __LINE__, status);
            }
        }

        if(0 == status)
        {
            status = ClockP_configureIntervalFromParams(params, pTimer);

            if (ClockP_StartMode_AUTO == params->startMode)
            {
                timer_settime(pTimer->timer_id, 0, &pTimer->itime, NULL);
            }

            ret_handle = (ClockP_Handle)pTimer;
        }
    }

    if(NULL == ret_handle)
    {
        slogf(_SLOGC_PRIVATE_OSAL, _SLOG_ERROR, "%s:%d: Error creating ClockP instance!", __FUNCTION__, __LINE__);
    }
    return ret_handle;
}

/*
 *  ======== ClockP_delete ========
 */
ClockP_Status ClockP_delete(ClockP_Handle handle)
{
    ClockP_qnx *pTimer = (ClockP_qnx*)handle;
    uintptr_t key;
    ClockP_Status ret = ClockP_OK;

    if ((NULL_PTR != pTimer) && (BTRUE == pTimer->used))
    {
        (void)timer_delete(pTimer->timer_id);

        key = HwiP_disable();
        (void)memset((void *)pTimer,0,sizeof(pTimer));
        if (0U < gOsalClockAllocCnt)
        {
            gOsalClockAllocCnt--;
        }
        HwiP_restore(key);
        ret = ClockP_OK;
    }
    else
    {
        ret = ClockP_FAILURE;
    }

    return ret;
}

ClockP_Status ClockP_start(ClockP_Handle handle)
{
    ClockP_Status ret = ClockP_OK;
    ClockP_qnx *pTimer = (ClockP_qnx*)handle;

    if ((NULL_PTR != pTimer) && (BTRUE == pTimer->used))
    {
        ret = timer_settime(pTimer->timer_id, 0, &pTimer->itime, NULL);
    }
    else
    {
        ret = ClockP_FAILURE;
    }

    return ret;
}

ClockP_Status ClockP_stop(ClockP_Handle handle)
{
    ClockP_qnx *pTimer = (ClockP_qnx*)handle;
    ClockP_Status ret = ClockP_OK;
    struct itimerspec itime;

    if ((NULL_PTR != pTimer) && (BTRUE == pTimer->used))
    {
        itime.it_value.tv_sec = 0;
        itime.it_value.tv_nsec = 0;
        itime.it_interval.tv_sec = 0;
        itime.it_interval.tv_nsec = 0;

        ret = timer_settime(pTimer->timer_id, 0, &itime, NULL);
    }
    else
    {
        ret = ClockP_FAILURE;
    }

    return ret;
}

/* Nothing past this point */
