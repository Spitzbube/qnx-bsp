/*
 *  Copyright (c) Texas Instruments Incorporated 2018
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
 *  \file udma.c
 *
 *  \brief File containing the UDMA generic driver APIs.
 *
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <ti/drv/udma/src/udma_priv.h>
#ifdef QNX_OS
#include <udma_resmgr.h>
#endif

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

int32_t Udma_init(Udma_DrvHandle drvHandle, const Udma_InitPrms *initPrms)
{
    int32_t                             retVal = UDMA_SOK;
#if (UDMA_SOC_CFG_PROXY_PRESENT == 1)
    struct tisci_msg_rm_proxy_cfg_req   req;
#endif

#ifdef QNX_OS
    /* We need to have IO PRIV to access UDMA Channel/Ring registers */
    if (ThreadCtl(_NTO_TCTL_IO_PRIV, NULL) == -1) {
        retVal = UDMA_EALLOC;
    }
#endif

    if((drvHandle == NULL_PTR) || (NULL_PTR == initPrms))
    {
        retVal = UDMA_EBADARGS;
    }

    if(UDMA_SOK == retVal)
    {
        (void) memset(drvHandle, 0, sizeof(*drvHandle));
        (void) memcpy(&drvHandle->initPrms, initPrms, sizeof(Udma_InitPrms));
        Udma_initDrvHandle(drvHandle);

        Udma_assert(drvHandle, ((Udma_OsalMutexCreateFxn) NULL_PTR != drvHandle->initPrms.osalPrms.createMutex));
#ifdef QNX_OS
        if(1 == drvHandle->initPrms.isQnxRMInstance)
        {
            drvHandle->rmLock = drvHandle->initPrms.osalPrms.createMutex();
            if(NULL_PTR == drvHandle->rmLock)
            {
                retVal = UDMA_EALLOC;
            }
        }
#else
        drvHandle->rmLock = drvHandle->initPrms.osalPrms.createMutex();
        if(NULL_PTR == drvHandle->rmLock)
        {
            retVal = UDMA_EALLOC;
        }
#endif

        if(UDMA_SOK == retVal)
        {
            drvHandle->printLock = drvHandle->initPrms.osalPrms.createMutex();

            /* We can't handle mutex create failure for printf, as all logging
             * stops when lock is not available*/
            if(NULL_PTR == drvHandle->printLock)
            {
                /* We don't assert as Udma_assert uses print function */
                retVal = UDMA_EALLOC;
            }
        }

#ifdef QNX_OS
        if((UDMA_SOK == retVal) && (1 == drvHandle->initPrms.isQnxRMInstance))
        {
            // QNX udma RM only
            retVal = Udma_rmInit(drvHandle);
            if(UDMA_SOK != retVal)
            {
                Udma_printf(drvHandle, "[Error] RM init failed!!!\n");
            }
        }
        else if((UDMA_SOK == retVal) && (0 == drvHandle->initPrms.isQnxRMInstance))
        {
            // QNX udma clients only
            Udma_printf(drvHandle, "Calling Udma_resmgr_open");
            Udma_resmgr_open(drvHandle);
        }
#else
        if(UDMA_SOK == retVal)
        {
            retVal = Udma_rmInit(drvHandle);
            if(UDMA_SOK != retVal)
            {
                Udma_printf(drvHandle, "[Error] RM init failed!!!\n");
            }
        }
#endif

#if (UDMA_SOC_CFG_PROXY_PRESENT == 1)
#ifdef QNX_OS
        // QNX RM only
        if((UDMA_INST_TYPE_NORMAL == drvHandle->instType) && (1 == drvHandle->initPrms.isQnxRMInstance))
#else
        if(UDMA_INST_TYPE_NORMAL == drvHandle->instType)
#endif
        {
            if(UDMA_SOK == retVal)
            {
                /* Setup channelized firewall for default core proxy */
                req.valid_params = 0U;
                req.nav_id       = drvHandle->devIdProxy;
                req.index        = drvHandle->initPrms.rmInitPrms.proxyThreadNum;
                retVal = Sciclient_rmSetProxyCfg(&req, UDMA_SCICLIENT_TIMEOUT);
                if(UDMA_SOK != retVal)
                {
                    Udma_printf(drvHandle, "[Error] SciClient Set proxy config failed!!!\n");
                }
            }
        }
#endif

        if(UDMA_SOK == retVal)
        {
            drvHandle->drvInitDone = UDMA_INIT_DONE;

            if(UFALSE == initPrms->skipGlobalEventReg)
            {
                Udma_EventPrms  eventPrms;

                UdmaEventPrms_init(&eventPrms);
#ifdef QNX_OS
                if ((initPrms->intrPriority == 0U) || (initPrms->intrPriority > 255U))
                    eventPrms.intrPriority = 20U; // set a reasonable priority if outside the range.
                else
                    eventPrms.intrPriority = initPrms->intrPriority;
#endif
                eventPrms.eventType = UDMA_EVENT_TYPE_MASTER;
                eventPrms.eventMode = UDMA_EVENT_MODE_SHARED;
                retVal = Udma_eventRegister(
                             drvHandle, &drvHandle->globalEventObj, &eventPrms);
                if(UDMA_SOK != retVal)
                {
                    Udma_printf(drvHandle,
                        "[Error] Global master event register failed!!!\n");
                }
                else
                {
                    drvHandle->globalEventHandle = &drvHandle->globalEventObj;
                }
            }
        }

        if(UDMA_SOK != retVal)
        {
            /* Free-up allocated resources */
            Udma_assert(drvHandle, ((Udma_OsalMutexDeleteFxn) NULL_PTR != drvHandle->initPrms.osalPrms.deleteMutex));
#ifdef QNX_OS
            // QNX RM only
            if((NULL_PTR != drvHandle->rmLock) && (1 == drvHandle->initPrms.isQnxRMInstance))
#else
            if(NULL_PTR != drvHandle->rmLock)
#endif
            {
                drvHandle->initPrms.osalPrms.deleteMutex(drvHandle->rmLock);
                drvHandle->rmLock = NULL_PTR;
            }
            if(NULL_PTR != drvHandle->printLock)
            {
                drvHandle->initPrms.osalPrms.deleteMutex(drvHandle->printLock);
                drvHandle->printLock = NULL_PTR;
            }
            drvHandle->drvInitDone = UDMA_DEINIT_DONE;
        }
    }

    return (retVal);
}

int32_t Udma_deinit(Udma_DrvHandle drvHandle)
{
    int32_t     retVal = UDMA_SOK;

    /* Error check */
    if((NULL_PTR == drvHandle) || (UDMA_INIT_DONE != drvHandle->drvInitDone))
    {
        retVal = UDMA_EBADARGS;
    }

    if(UDMA_SOK == retVal)
    {
        if(NULL_PTR != drvHandle->globalEventHandle)
        {
            retVal = Udma_eventUnRegister(drvHandle->globalEventHandle);
            if(UDMA_SOK != retVal)
            {
                Udma_printf(drvHandle, "[Error] Global event free failed!!!\n");
            }
            drvHandle->globalEventHandle = (Udma_EventHandle) NULL_PTR;
        }


#ifdef QNX_OS
        if(1 == drvHandle->initPrms.isQnxRMInstance)
        {
            retVal += Udma_rmDeinit(drvHandle);
            if(UDMA_SOK != retVal)
            {
                Udma_printf(drvHandle, "[Error] RM deinit failed!!!\n");
            }
        }
        else
        {
            // QNX clients only
            Udma_printf(drvHandle, "Calling Udma_resmgr_close");
            Udma_resmgr_close(drvHandle);
        }
#else
        retVal += Udma_rmDeinit(drvHandle);
        if(UDMA_SOK != retVal)
        {
            Udma_printf(drvHandle, "[Error] RM deinit failed!!!\n");
        }
#endif
    }

    if(UDMA_SOK == retVal)
    {
        Udma_assert(drvHandle,
            ((Udma_OsalMutexDeleteFxn) NULL_PTR != drvHandle->initPrms.osalPrms.deleteMutex));
        if(NULL_PTR != drvHandle->rmLock)
        {
            drvHandle->initPrms.osalPrms.deleteMutex(drvHandle->rmLock);
            drvHandle->rmLock = NULL_PTR;
        }
        if(NULL_PTR != drvHandle->printLock)
        {
            drvHandle->initPrms.osalPrms.deleteMutex(drvHandle->printLock);
            drvHandle->printLock = NULL_PTR;
        }
        (void) memset(drvHandle, 0, sizeof(*drvHandle));
        drvHandle->drvInitDone = UDMA_DEINIT_DONE;
    }

    return (retVal);
}

static inline int32_t Udma_checkInstId(uint32_t instId)
{
    int32_t retVal = UDMA_SOK;
    if(UDMA_INST_ID_MAX < instId)
    {
        retVal = UDMA_EINVALID_PARAMS;
    }
    return retVal;
}

int32_t UdmaInitPrms_init(uint32_t instId, Udma_InitPrms *initPrms)
{
    int32_t retVal = UDMA_SOK;

    /* Error check */
    if(NULL_PTR == initPrms)
    {
        retVal = UDMA_EBADARGS;
    }

    if(UDMA_SOK == retVal)
    {
        retVal = Udma_checkInstId(instId);
    }

    if(UDMA_SOK == retVal)
    {
        initPrms->instId                = instId;
        retVal = UdmaRmInitPrms_init(instId, &initPrms->rmInitPrms);
        UdmaOsalPrms_init(&initPrms->osalPrms);
        initPrms->skipRmOverlapCheck    = UFALSE;
        initPrms->skipGlobalEventReg    = UFALSE;
        initPrms->virtToPhyFxn          = &Udma_defaultVirtToPhyFxn;
        initPrms->phyToVirtFxn          = &Udma_defaultPhyToVirtFxn;
        initPrms->printFxn              = (Udma_PrintFxn) NULL_PTR;
#ifdef QNX_OS
        initPrms->isQnxRMInstance       = 0;
#endif
    }

    return (retVal);
}


