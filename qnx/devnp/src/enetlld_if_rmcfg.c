/*
 *  Copyright (c) Texas Instruments Incorporated 2019-21
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

/*!
 * \file     cpswapp_rmcfg.c
 *
 * \brief    This file contains the CPSW driver resource management
 *           configuration used to initialize the RM init parameters passed
 *           during driver init.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <ti/drv/enet/enet.h>
#include <ti/drv/enet/include/per/cpsw.h>
#include <ti/drv/ipc/ipc.h>

#include "enetlld_if_utils.h"
#include "enetlld_if_memutils.h"
#include "enetlld_if.h"

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

/*!
 *  \brief CPSW9G default configuration
 *
 *   Note: If user wishes to change the Resource Partition the following
 *   things must be considered:
 *   1. Sum of numTxCh allocated to each core should not exceed 8.
 *   2. Sum of numRxFlows allocated to each core should not exceed 63 (not 64),
 *      as one Rx flow is reserved to the master core.
 *
 */
static EnetRm_ResPrms gEnetIfRmDefCfg_9G =
{
    .coreDmaResInfo =
    {
        [0] =
        {
            .coreId        = IPC_MPU1_0,
            .numTxCh       = 1U,
            .numRxFlows    = 1U,
            .numMacAddress = 1U,
        },
        [1] =
        {
            .coreId        = IPC_MCU2_0,
            .numTxCh       = 4U,
            .numRxFlows    = 5U,
            .numMacAddress = 1U,
        },
        [2] =
        {
            .coreId        = IPC_MCU2_1,
            .numTxCh       = 1U,
            .numRxFlows    = 2U,
            .numMacAddress = 1U,
        },
        [3] =
        {
            .coreId        = IPC_MCU1_0,
            .numTxCh       = 2U,
            .numRxFlows    = 2U,
            .numMacAddress = 1U,
        },
    },
    .numCores = 4U,
};

/*!
 *  \brief CPSW2G default configuration
 *         Note: If user wishes to change the Resource Partition the following
 *         things must be considered:
 *         1. Sum of numTxCh allocated to each core should not exceed 8.
 *         2. Sum of numRxFlows allocated to each core should not exceed 63 (not 64),
 *            as one Rx flow is reserved to the master core.
 *
 */
static EnetRm_ResPrms gEnetIfRmDefCfg_2G =
{
    .coreDmaResInfo =
    {
        [0] =
        {
            .coreId        = IPC_MPU1_0,
            .numTxCh       = 2U,
            .numRxFlows    = 2U,
            .numMacAddress = 1U,
        },
        [1] =
        {
            .coreId        = IPC_MCU2_1,
            .numTxCh       = 2U,
            .numRxFlows    = 2U,
            .numMacAddress = 1U,
        },
        [2] =
        {
            .coreId        = IPC_MCU1_0,
            .numTxCh       = 4U,
            .numRxFlows    = 4U,
            .numMacAddress = 1U,
        },
    },
    .numCores = 3,
};

#if defined (SOC_J721S2) || defined(SOC_J784S4)
/*!
 *  \brief Main CPSW2G default configuration
 *
 *   Note: If user wishes to change the Resource Partition the following
 *   things must be considered:
 *   1. Sum of numTxCh allocated to each core should not exceed 8.
 *   2. Sum of numRxFlows allocated to each core should not exceed 63 (not 64),
 *      as one Rx flow is reserved to the master core.
 *
 */
static EnetRm_ResPrms gEnetIfRmDefCfg_Main2G =
{
    .coreDmaResInfo =
    {
        [0] =
        {
            .coreId        = IPC_MPU1_0,
            .numTxCh       = 1U,
            .numRxFlows    = 2U,
            .numMacAddress = 1U,
        },
        [1] =
        {
            .coreId        = IPC_MCU2_0,
            .numTxCh       = 4U,
            .numRxFlows    = 5U,
            .numMacAddress = 1U,
        },
    },
    .numCores = 2U,
};
#endif

/* Cores IOCTL Privileges */
static const EnetRm_IoctlPermissionTable gEnetIfIoctlPermission_2G =
{
#if defined (SOC_J721E) || defined (SOC_J721S2) || defined(SOC_J784S4)
    .defaultPermittedCoreMask = (
                                 ENET_BIT(IPC_MPU1_0) |
                                 ENET_BIT(IPC_MCU2_0) |
                                 ENET_BIT(IPC_MCU2_1) |
                                 ENET_BIT(IPC_MCU3_0) |
                                 ENET_BIT(IPC_MCU1_0)
                                 ),
#elif defined (SOC_J7200)
    .defaultPermittedCoreMask = (
                                 ENET_BIT(IPC_MPU1_0) |
                                 ENET_BIT(IPC_MCU2_0) |
                                 ENET_BIT(IPC_MCU2_1) |
                                 ENET_BIT(IPC_MCU1_0)
                                 ),
#endif
    .numEntries = 0,
};

static const EnetRm_IoctlPermissionTable gEnetIfIoctlPermission_9G =
{
#if defined (SOC_J721E) || defined(SOC_J784S4)
    .defaultPermittedCoreMask = (
                                 ENET_BIT(IPC_MPU1_0) |
                                 ENET_BIT(IPC_MCU2_0) |
                                 ENET_BIT(IPC_MCU2_1) |
                                 ENET_BIT(IPC_MCU3_0) |
                                 ENET_BIT(IPC_MCU1_0)
                                 ),
#elif defined (SOC_J7200)
    .defaultPermittedCoreMask = (
                                 ENET_BIT(IPC_MPU1_0) |
                                 ENET_BIT(IPC_MCU2_0) |
                                 ENET_BIT(IPC_MCU2_1) |
                                 ENET_BIT(IPC_MCU1_0)
                                 ),
#endif
    .numEntries = 0,
};

#if defined (SOC_J721S2) || defined(SOC_J784S4)
static const EnetRm_IoctlPermissionTable gEnetIfIoctlPermission_Main2G =
{
    .defaultPermittedCoreMask = (ENET_BIT(IPC_MPU1_0) |
                                 ENET_BIT(IPC_MCU2_0) |
                                 ENET_BIT(IPC_MCU2_1) |
                                 ENET_BIT(IPC_MCU3_0) |
                                 ENET_BIT(IPC_MCU1_0)),
    .numEntries = 0,
};
#endif

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

const EnetRm_ResPrms *EnetIfRm_getResPartInfo(Enet_Type enetType,
                                              uint32_t instId)
{
    const EnetRm_ResPrms *rmInitPrms = NULL;

    switch (enetType)
    {
        case ENET_CPSW_2G:
        {
            if (instId == 0)
                rmInitPrms = &gEnetIfRmDefCfg_2G;
#if defined (SOC_J721S2) || defined(SOC_J784S4)
            else if (instId == 1)
                rmInitPrms = &gEnetIfRmDefCfg_Main2G;
#endif
            else
                rmInitPrms = NULL;

            break;
        }

        case ENET_CPSW_9G:
        {
            rmInitPrms = &gEnetIfRmDefCfg_9G;
            break;
        }

        default:
        {
            rmInitPrms = NULL;
            break;
        }
    }

    return(rmInitPrms);
}

const EnetRm_IoctlPermissionTable *EnetIfRm_getIoctlPermissionInfo(Enet_Type enetType,
                                                                   uint32_t instId)
{
    const EnetRm_IoctlPermissionTable *ioctlPerm = NULL;

    switch (enetType)
    {
        case ENET_CPSW_2G:
        {
            if (instId == 0)
                ioctlPerm = &gEnetIfIoctlPermission_2G;
#if defined (SOC_J721S2) || defined(SOC_J784S4)
            else if (instId == 1)
                ioctlPerm = &gEnetIfIoctlPermission_Main2G;
#endif
            else
                ioctlPerm = NULL;

            break;
        }

        case ENET_CPSW_9G:
        {
            ioctlPerm = &gEnetIfIoctlPermission_9G;
            break;
        }

        default:
        {
            ioctlPerm = NULL;
            break;
        }
    }

    return(ioctlPerm);
}
