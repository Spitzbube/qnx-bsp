/*
 *  Copyright (c) Texas Instruments Incorporated 2018-21
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
 *  \file     cpsw_lld_if.c
 *
 *  \brief    CPSW interface private source file.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <hw/inout.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <hw/nicinfo.h>
#include <io-pkt/iopkt_driver.h>
#include <sys/io-pkt.h>

#include <ti/osal/osal.h>
#include <ti/drv/udma/udma.h>

#include <ti/drv/enet/enet.h>

#if defined (CPSW9G) || defined (CPSW5G)
#include <ti/drv/ipc/ipc.h>
#include <ti/csl/cslr_gtc.h>
#endif /* defined (CPSW9G) || defined (CPSW5G)*/

#include "j7_cpsw.h"
#include "enetlld_if_utils.h"
#include "enetlld_if_memutils.h"
#include "enetlld_if.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#if defined (CPSW9G)  || defined (CPSW5G)
#define CPSW_REMOTE_APP_MASTER_ENDPT          (26)
#define CPSW_REMOTE_APP_GTC_PUSHEVT_BIT_SEL   (28U)
#define CPSW_REMOTE_APP_CPTS_HW_PUSH_NUM      (3U)
/* Heartbeat poll period for CPSW proxy. */
#define CPSW_REMOTE_APP_POLL_PERIOD_MS        (2000U)
/* CPSW proxy command response timeout. */
#define CPSW_REMOTE_APP_CMD_TIMEOUT_MS        (1000U)
#endif /* defined (CPSW9G)  || defined (CPSW5G)*/

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/* CPSW configuration */
EnetIf_Obj gEnetIfObj;

/* SMMU enable flag.. referred by lld driver */
uint32_t gSMMU = 0;
uint32_t gSMMU_virtid = 0;

/* Cache Op flag */
uint32_t gCache_ops = 0;

/* Cache Op stucture */
extern struct cache_ctrl cachectl;

/* devnp driver object */
extern struct cpsw_dev *g_cpsw;

#if defined (CPSW9G) || defined (CPSW5G)
extern int g_cpsw_recovery_connection_id;


uint32_t gRemoteProcArray[] =
{
#if defined (SOC_J721E)
    IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU2_1, IPC_MCU3_0, IPC_MCU3_1, IPC_C66X_1, IPC_C66X_2, IPC_C7X_1
#elif defined (SOC_J7200)
    IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU2_1
#elif defined (SOC_J784S4)
    IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU2_1, IPC_MCU3_0, IPC_MCU3_1, IPC_MCU4_0, IPC_MCU4_1, IPC_C7X_1, IPC_C7X_2, IPC_C7X_3, IPC_C7X_4
#endif
};

static Enet_MacPort gRemoteAppMacPorts[] =
{
#if defined (SOC_J721E) || defined (SOC_J784S4)
    /* configuration based on GESI board */
    ENET_MAC_PORT_1,
    ENET_MAC_PORT_3,
    ENET_MAC_PORT_4,
    ENET_MAC_PORT_8,
    /* configuration based on Quad-Port Eth Expansion board */
#if defined(ENABLE_QSGMII_PORTS)
    ENET_MAC_PORT_2, /* QSGMII main */
    ENET_MAC_PORT_5, /* QSGMII sub */
    ENET_MAC_PORT_6, /* QSGMII sub */
    ENET_MAC_PORT_7, /* QSGMII sub */
#endif
#elif defined (SOC_J7200)
    /* configuration based on Quad-Port Eth Expansion board */
    ENET_MAC_PORT_1,
    ENET_MAC_PORT_2,
    ENET_MAC_PORT_3,
    ENET_MAC_PORT_4,
#endif
};
#endif /* defined (CPSW9G) || defined (CPSW5G)*/

EnetDma_Pkt gPktInfoMem[CPSW_MEM_TX_PKTS_MAX + CPSW_MEM_RX_PKTS_MAX];

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */
#if defined (CPSW9G) || defined (CPSW5G)
void appLogPrintf(const char *format, ...)
{
    char printBuffer[256];
    va_list arguments;

    if (256 < strlen(format))
    {
        assert(false);
    }

    /* Start the varargs processing */
    va_start(arguments, format);
    vsnprintf(printBuffer, sizeof(printBuffer), format, arguments);

    slogf(_SLOGC_NETWORK, _SLOG_INFO, printBuffer);

    /* End the varargs processing */
    va_end(arguments);
}

static void printDevInfo(EthRemoteCfg_DeviceData *ethDevData)
{
    char *tf[] = {"false", "true"};

    EnetIf_print("ETHFW Version:%2d.%2d.%2d\n",
                  ethDevData->fwVer.major,
                  ethDevData->fwVer.minor,
                  ethDevData->fwVer.rev);
    EnetIf_print("ETHFW Build Date (YYYY/MMM/DD):%c%c%c%c/%c%c%c/%c%c\n",
                  ethDevData->fwVer.year[0], ethDevData->fwVer.year[1], ethDevData->fwVer.year[2], ethDevData->fwVer.year[3],
                  ethDevData->fwVer.month[0], ethDevData->fwVer.month[1], ethDevData->fwVer.month[2],
                  ethDevData->fwVer.date[0], ethDevData->fwVer.date[1]);
    EnetIf_print("ETHFW Commit SHA:%c%c%c%c%c%c%c%c\n",
                  ethDevData->fwVer.commitHash[0],
                  ethDevData->fwVer.commitHash[1],
                  ethDevData->fwVer.commitHash[2],
                  ethDevData->fwVer.commitHash[3],
                  ethDevData->fwVer.commitHash[4],
                  ethDevData->fwVer.commitHash[5],
                  ethDevData->fwVer.commitHash[6],
                  ethDevData->fwVer.commitHash[7]);
    EnetIf_print("ETHFW PermissionFlag:0x%x, UART Connected:%s,UART Id:%d",
                  ethDevData->permissionFlags,
                  tf[ethDevData->uartConnected],
                  ethDevData->uartId);
}

#if defined (CPSW9G) || defined (CPSW5G)
static void EnetIf_hbStatus(EthRemoteCfg_ServerStatus serverStatus,
                            void *cbArg)
{
    switch (serverStatus)
    {
        case ETHREMOTECFG_SERVERSTATUS_UNINIT:
            EnetIf_print("Server status: server has not been initialized\r\n");
            break;
        case ETHREMOTECFG_SERVERSTATUS_READY:
            EnetIf_print("Server status: server is running normally\r\n");
            break;
        case ETHREMOTECFG_SERVERSTATUS_RECOVERY:
            EnetIf_print("Server status: Ethernet switch is under recovery\r\n");
            break;
        case ETHREMOTECFG_SERVERSTATUS_BAD:
            EnetIf_print("Server status: Not responding, possibly in bad/crashed state\r\n");
            break;
        default:
            EnetIf_print("Server status: Unexpected status: %d\r\n", (int32_t)serverStatus);
            break;
    }
}
#endif

static uint64_t EnetIf_getLocalTime(uint64_t baseAddress)
{
    uint64_t gtcTimeLo = 0UL, gtcTimeHi = 0UL;
    uint64_t gtcTime = 0UL;

    gtcTimeLo = (uint64_t) CSL_REG32_RD(baseAddress + CSL_GTC_CFG1_CNTCV_LO);
    gtcTimeHi = (uint64_t) CSL_REG32_RD(baseAddress + CSL_GTC_CFG1_CNTCV_HI);
    gtcTime = (gtcTimeHi << 32U) | gtcTimeLo;

    return gtcTime;
}

static uint64_t EnetIf_getSynchronizedTime(EnetIf_SyncTimerObj *hSyncTimerObj)
{
    uint64_t gtcTime = 0UL, synchronizedTime = 0UL;

    /* Get GTC time */
    gtcTime = EnetIf_getLocalTime((uint64_t) hSyncTimerObj->GTC0_GTC_CFG1_Base);

    /* Compute synchronized time from GTC time */
    synchronizedTime = (uint64_t)((hSyncTimerObj->rate * (double)gtcTime) + hSyncTimerObj->offset);

    return synchronizedTime;
}

static void EnetIf_calcSyncTimeParams(uint32_t notifyType, void *notifyArg, void *cbArg)
{
    CpswProxy_HwPushNotifyParams *params = (CpswProxy_HwPushNotifyParams *)notifyArg;
    CpswCpts_HwPush hwPushNum = (CpswCpts_HwPush)(params->hwPushNum);
    uint64_t syncTime = params->timestamp;
    uint64_t gtcTime = 0U;
    EnetIf_SyncTimerObj *hSyncTimerObj = (EnetIf_SyncTimerObj*)&gEnetIfObj.syncTimerObj;

    if (hwPushNum == CPSW_REMOTE_APP_CPTS_HW_PUSH_NUM)
    {
        if(hSyncTimerObj->prevLocalTime == 0U)
        {
            /* Set GTC time */
            gtcTime = EnetIf_getLocalTime((uint64_t) gEnetIfObj.syncTimerObj.GTC0_GTC_CFG1_Base) &
                                            (0xFFFFFFFFFFFFFFFFUL << CPSW_REMOTE_APP_GTC_PUSHEVT_BIT_SEL);
        }
        else
        {
            /* Increment GTC time used for computation based on selected bit for event */
            gtcTime = hSyncTimerObj->prevLocalTime + (1UL << (CPSW_REMOTE_APP_GTC_PUSHEVT_BIT_SEL+1));
        }

        if ((hSyncTimerObj->prevLocalTime != 0U) &&
            (hSyncTimerObj->prevCptsTime != 0U))
        {
            /* Logic:
             *  T1, T2 - Previous & Current CPTS time
             *  t1, t2 - Previous & Current local GTC time
             *  rate = (T2-T1) / (t2-t1)
             *  offset = T2 - rate*t2
             */
            hSyncTimerObj->rate = (double)((double)(syncTime - hSyncTimerObj->prevCptsTime) /
                                          (double)(gtcTime - hSyncTimerObj->prevLocalTime));
            hSyncTimerObj->offset = (double) syncTime - (hSyncTimerObj->rate * (double) gtcTime);
        }

        hSyncTimerObj->prevLocalTime = gtcTime;
        hSyncTimerObj->prevCptsTime = syncTime;
    }
}

#endif /* defined (CPSW9G) || defined (CPSW5G)*/

void EnetIf_InitObj()
{
    /* Calling this so that the EnetSoc lib gets linked */
    EnetSoc_init();

#if defined (CPSW2G)

    memset(&gEnetIfObj, 0, sizeof(EnetIf_Obj));

    gEnetIfObj.enetType = ENET_CPSW_2G;
    gEnetIfObj.instId = 0;
    gEnetIfObj.macPort = ENET_MAC_PORT_1;
    gEnetIfObj.coreId = IPC_MPU1_0;
    gEnetIfObj.macMode = RGMII;
    gEnetIfObj.preferredUdmaChannel = UDMA_DMA_CH_ANY;
    gEnetIfObj.numTxPkts = CPSW_MEM_TX_PKTS_DEFAULT;
    gEnetIfObj.numRxPkts = CPSW_MEM_RX_PKTS_DEFAULT;
    gCache_ops = 0;
    gSMMU = 0;

    EnetIfBoard_cpsw2gMacModeConfig(gEnetIfObj.macPort, gEnetIfObj.macMode);

    EnetIf_selectCptsClock(gEnetIfObj.enetType, ENETIF_CPTS_CLKSEL_MAIN_SYSCLK0);

#elif defined (CPSW2G_MAIN)

    memset(&gEnetIfObj, 0, sizeof(EnetIf_Obj));

    gEnetIfObj.enetType = ENET_CPSW_2G;
    gEnetIfObj.instId = 1;
    gEnetIfObj.macPort = ENET_MAC_PORT_1;
    gEnetIfObj.coreId = IPC_MPU1_0;
    gEnetIfObj.macMode = RGMII;
    gEnetIfObj.preferredUdmaChannel = UDMA_DMA_CH_ANY;
    gEnetIfObj.numTxPkts = CPSW_MEM_TX_PKTS_DEFAULT;
    gEnetIfObj.numRxPkts = CPSW_MEM_RX_PKTS_DEFAULT;
    gCache_ops = 0;
    gSMMU = 0;

    EnetIfBoard_cpsw2gMacModeConfig(gEnetIfObj.macPort, gEnetIfObj.macMode);

    EnetIf_selectCptsClock(gEnetIfObj.enetType, ENETIF_CPTS_CLKSEL_MAIN_SYSCLK0);

    EnetBoard_setupGESI();

#elif defined (CPSW9G) || defined (CPSW5G)

    memset(&gEnetIfObj, 0, sizeof(EnetIf_Obj));

    gEnetIfObj.macPort = ENET_MAC_PORT_INV;
    gEnetIfObj.numMacPorts = ENET_ARRAYSIZE(gRemoteAppMacPorts);
    gEnetIfObj.macPorts  = gRemoteAppMacPorts;
    gEnetIfObj.coreId = IPC_MPU1_0;
#if defined (SOC_J721E) || defined (SOC_J784S4)
    gEnetIfObj.enetType = ENET_CPSW_9G;
#elif defined (SOC_J7200)
    gEnetIfObj.enetType = ENET_CPSW_5G;
#endif
    gEnetIfObj.instId = 0;
    gEnetIfObj.preferredUdmaChannel = UDMA_DMA_CH_ANY;
    gEnetIfObj.numTxPkts = CPSW_MEM_TX_PKTS_DEFAULT;
    gEnetIfObj.numRxPkts = CPSW_MEM_RX_PKTS_DEFAULT;
    gCache_ops= 0;
    gSMMU = 0;

#else
    #error "Unsupported!"
#endif

    return;
}

int32_t EnetIf_RemoteAttach(uint8_t macAddr[ENET_MAC_ADDR_LEN])
{
    int32_t status = ENET_SOK;

#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    /* Nothing for CPSW2G */
#elif defined (CPSW9G) || defined (CPSW5G)
    uint32_t connectAttempts = 0;

    /* Initialize the multiproc */
    if (IPC_SOK == Ipc_mpSetConfig(IPC_MPU1_0, sizeof(gRemoteProcArray)/sizeof(uint32_t), (uint32_t *)&gRemoteProcArray))
    {
#if defined (SOC_J721E) || defined (SOC_J784S4)
        EnetIf_print("CPSW9G Client (core : %s) .....\r\n", Ipc_mpGetSelfName());
#elif defined (SOC_J7200)
        EnetIf_print("CPSW5G Client (core : %s) .....\r\n", Ipc_mpGetSelfName());
#endif

        if (IPC_SOK == Ipc_init(NULL))
        {
            EthFwTrace_Cfg traceCfg;
            CpswProxy_Config proxyConfig;
            CpswProxy_initParams initParams;

            memset(&traceCfg, 0, sizeof(traceCfg));

            traceCfg.print = appLogPrintf;
            EthFwTrace_init(&traceCfg);
            EthFwTrace_setLevel(ETHFW_TRACE_INFO);

            /* Initialize CPSW Proxy config params */
            CpswProxy_initConfig(&initParams);
            initParams.hbPeriodInMsecs = CPSW_REMOTE_APP_POLL_PERIOD_MS;
            initParams.cmdTimeoutInMsecs = CPSW_REMOTE_APP_CMD_TIMEOUT_MS;
            initParams.hbNotifyCb.cbFxn = EnetIf_hbStatus;

            /* Init CPSW Proxy, needs to be called only once */
            CpswProxy_init(&initParams);

            proxyConfig.virtPort = ETHREMOTECFG_SWITCH_PORT_0;

            if (g_cpsw == NULL)
                EnetIf_print("%s:%d g_cpsw is NULL!!!!", __FUNCTION__, __LINE__);

            while (connectAttempts < g_cpsw->cpsw_connect_attempts)
            {
                status = CpswProxy_connect();

                if (IPC_SOK == status)
                {
                    EnetIf_print("%s:%d Successfully connected to CpswProxy Server after %d attempt(s)!", __FUNCTION__, __LINE__, ++connectAttempts);
                    break;
                }
                else
                {
                    EnetIf_print("%s:%d Failed to connect to CpswProxy Server...attempt %d/%d", __FUNCTION__, __LINE__, ++connectAttempts, g_cpsw->cpsw_connect_attempts);
                    if (connectAttempts < g_cpsw->cpsw_connect_attempts)
                        TaskP_sleepInMsecs(g_cpsw->cpsw_connect_delay_ms);
                }
            }

            EnetIf_print("%s:%d Spent approx. %d ms idle waiting to connect to CpswProxy Server", __FUNCTION__, __LINE__, (connectAttempts-1)*(g_cpsw->cpsw_connect_delay_ms));

            if (IPC_SOK == status)
            {
                /* Start Cpsw Proxy */
                gEnetIfObj.hCpswProxy = CpswProxy_open(&proxyConfig);
                if (NULL != gEnetIfObj.hCpswProxy)
                {
#if defined (SOC_J721E) || defined (SOC_J784S4)
                    EnetIf_print("CPSW9G: Attach to CpswProxy Server.\r\n");
#elif defined (SOC_J7200)
                    EnetIf_print("CPSW5G: Attach to CpswProxy Server.\r\n");
#endif
                    /* TODO: Check for checksum enabled returned by CpswProxy_Attach */
                    CpswProxy_attach(gEnetIfObj.hCpswProxy,
                            ETHREMOTECFG_SWITCH_PORT_0,
                            &gEnetIfObj.rxMtu,
                            gEnetIfObj.txMtu,
                            &gEnetIfObj.numTxCh,
                            &gEnetIfObj.numRxFlow,
                            &gEnetIfObj.features);

                    gEnetIfObj.macPort = ENET_MAC_PORT_1;
                    EnetIf_print("%s:%d RX MTU returned - %d", __FUNCTION__, __LINE__, gEnetIfObj.rxMtu);

                    EnetIf_print("%s:%d MAC:%x:%x:%x:%x:%x:%x", __FUNCTION__, __LINE__,
                            macAddr[0], macAddr[1], macAddr[2],
                            macAddr[3], macAddr[4], macAddr[5]);
                    if (!memcmp(macAddr, "\0\0\0\0\0\0", ETHER_ADDR_LEN)) {
                        EnetIf_print("%s:%d MAC addr is NULL", __FUNCTION__, __LINE__);
                        CpswProxy_allocMac(gEnetIfObj.hCpswProxy,
                                gEnetIfObj.hostMacAddr);
                        gEnetIfObj.allocMacFlag = 1;
                    }
                    else {
                        EnetIf_print("%s:%d MAC addr is NOT NULL", __FUNCTION__, __LINE__);
                        memcpy(gEnetIfObj.hostMacAddr, macAddr, ETH_MAC_ADDR_LEN);
                    }
                }
                else
                {
                    EnetIf_print("%s:%d Error: CpswProxy_init failed", __FUNCTION__, __LINE__);
                    status = ENET_EFAIL;
                }
            }
            else
            {
                EnetIf_print("%s:%d Error: CpswProxy_connect failed...exhausted retry attempts.", __FUNCTION__, __LINE__);
                status = ENET_EFAIL;
            }
        }
        else
        {
            EnetIf_print("%s:%d Error: Ipc_init failed", __FUNCTION__, __LINE__);
            status = ENET_EFAIL;
        }
    }
    else
    {
        EnetIf_print("%s:%d Error: Ipc_mpSetConfig failed", __FUNCTION__, __LINE__);
        status = ENET_EFAIL;
    }
#endif

    return status;
}

void EnetIf_RemoteDetach(void)
{
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    /* Nothing for CPSW2G */
#elif defined (CPSW9G) || defined (CPSW5G)
    if (NULL != gEnetIfObj.hCpswProxy)
    {
        if (gEnetIfObj.ipv4Addr[0] != 0)
        {
            CpswProxy_unregisterIPV4Addr(gEnetIfObj.hCpswProxy,
                                         gEnetIfObj.ipv4Addr);
            memset(gEnetIfObj.ipv4Addr, 0, sizeof(gEnetIfObj.ipv4Addr));
        }

        if (gEnetIfObj.allocMacFlag == 1)
        {
            CpswProxy_freeMac(gEnetIfObj.hCpswProxy,
                              gEnetIfObj.hostMacAddr);
            gEnetIfObj.allocMacFlag = 0;
            memset (gEnetIfObj.hostMacAddr, 0, sizeof(gEnetIfObj.hostMacAddr));
        }

        CpswProxy_detach(gEnetIfObj.hCpswProxy);
        CpswProxy_close(gEnetIfObj.hCpswProxy);

#if defined (SOC_J721E) || defined (SOC_J784S4)
        EnetIf_print("CPSW9G: Detach from CpswProxy Server.\r\n");
#elif defined (SOC_J7200)
        EnetIf_print("CPSW5G: Detach from CpswProxy Server.\r\n");
#endif
        gEnetIfObj.hCpswProxy = NULL;
        gEnetIfObj.hEnet = NULL;
        gEnetIfObj.coreKey = 0;

        CpswProxy_deinit(); // must be called only once

        Ipc_deinit();
    }
#endif
}

void EnetIf_SetPerferredDmaChNum(uint32_t preferred_udma_channel)
{
    gEnetIfObj.preferredUdmaChannel = preferred_udma_channel;
}

void EnetIf_EnableSmmu(uint32_t virt_id)
{
    EnetIf_print("%s:%d SMMU", __FUNCTION__, __LINE__);
    gSMMU = 1;
    gSMMU_virtid = virt_id;
}

void EnetIf_EnableCacheOps()
{
    EnetIf_print("%s:%d SMMU Cache Ops enabled", __FUNCTION__, __LINE__);
    gCache_ops = 1;
}

void EnetIf_SetDescriptorCount(uint32_t tx_descriptors_count,
                               uint32_t rx_descriptors_count)
{
    gEnetIfObj.numTxPkts = tx_descriptors_count;
    gEnetIfObj.numRxPkts = rx_descriptors_count;
}

int32_t EnetIf_GetMacAddr(uint8_t macAddr[][ENET_MAC_ADDR_LEN])
{
#if defined (CPSW2G)
    uint32_t num = 1;
    EnetSoc_getEFusedMacAddrs(macAddr, &num);
#elif defined (CPSW2G_MAIN)
    uint8_t macAddrBuf[ENET_MAC_ADDR_LEN] =  { 0x70U, 0xFFU, 0x76U, 0x1DU, 0x92U, 0xC1U };
    memcpy(macAddr[0], macAddrBuf, ETH_MAC_ADDR_LEN);
#elif defined (CPSW9G) || defined (CPSW5G)
    memcpy(macAddr[0], gEnetIfObj.hostMacAddr, ETH_MAC_ADDR_LEN);
    EnetIf_print("MAC Address: %x:%x:%x:%x:%x:%x\n", macAddr[0][0],macAddr[0][1],
                                               macAddr[0][2],macAddr[0][3],
                                               macAddr[0][4],macAddr[0][5]);
#else
    #error "Unsupported!"
#endif
    return ENET_SOK;
}

int32_t EnetIf_SetMacAddr(uint8_t macAddr[ENET_MAC_ADDR_LEN])
{
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    /* Nothing to do */
#elif defined (CPSW9G) || defined (CPSW5G)
    if (gEnetIfObj.allocMacFlag == 1)
    {
        CpswProxy_freeMac(gEnetIfObj.hCpswProxy,
                          gEnetIfObj.hostMacAddr);
        gEnetIfObj.allocMacFlag = 0;
        memset (gEnetIfObj.hostMacAddr, 0, sizeof(gEnetIfObj.hostMacAddr));
    }
    memcpy(gEnetIfObj.hostMacAddr, macAddr, ETH_MAC_ADDR_LEN);
    EnetIf_print("MAC Address: %x:%x:%x:%x:%x:%x\n", macAddr[0],macAddr[1],
                                               macAddr[2],macAddr[3],
                                               macAddr[4],macAddr[5]);
#endif
    return ENET_SOK;
}

static void CpswCptsEvent_setDefaultPortEventPrms(
    CpswMacPort_TsEventCfg *tsPortEventCfg)
{
    tsPortEventCfg->commonPortIpCfg.ttlNonzeroEn = true;
    tsPortEventCfg->commonPortIpCfg.tsIp107En    = true;
    tsPortEventCfg->commonPortIpCfg.tsIp129En    = true;
    tsPortEventCfg->commonPortIpCfg.tsIp130En    = true;
    tsPortEventCfg->commonPortIpCfg.tsIp131En    = true;
    tsPortEventCfg->commonPortIpCfg.tsIp132En    = true;
    tsPortEventCfg->commonPortIpCfg.tsIp132En    = true;
    tsPortEventCfg->commonPortIpCfg.tsPort320En  = true;
    tsPortEventCfg->commonPortIpCfg.unicastEn    = false;
    tsPortEventCfg->domainOffset                 = 4U;
    tsPortEventCfg->ltype2En                     = false;
    tsPortEventCfg->rxAnnexDEn                   = true;
    tsPortEventCfg->rxAnnexEEn                   = true;
    tsPortEventCfg->rxAnnexFEn                   = true;
    tsPortEventCfg->txAnnexDEn                   = true;
    tsPortEventCfg->txAnnexEEn                   = true;
    tsPortEventCfg->txAnnexFEn                   = true;
    tsPortEventCfg->txHostTsEn                   = true;
    tsPortEventCfg->mcastType                    = 0U;
    tsPortEventCfg->messageType                  = 0xFFFFU;
    tsPortEventCfg->rxVlanType                   = ENET_MACPORT_VLAN_TYPE_SINGLE_TAG;
    tsPortEventCfg->seqIdOffset                  = 30U;
    tsPortEventCfg->txVlanType                   = ENET_MACPORT_VLAN_TYPE_SINGLE_TAG;
    tsPortEventCfg->vlanLType1                   = 0x8100;
    tsPortEventCfg->vlanLType2                   = 0U;
}

int32_t EnetIf_PtpInit()
{
    int32_t status = ENET_SOK;
    Enet_IoctlPrms prms;
    CpswCpts_RegisterStackInArgs registerStackInArgs;
    CpswMacPort_EnableTsEventInArgs enableTsEventInArgs;

    EnetIf_print("%s:%d PTP", __FUNCTION__, __LINE__);

    /* CPTS */
    if (status == ENET_SOK)
    {
        registerStackInArgs.eventNotifyCb = EnetIf_EventNotifyCallback;
        registerStackInArgs.eventNotifyCbArg = &gEnetIfObj;

        ENET_IOCTL_SET_IN_ARGS(&prms, &registerStackInArgs);
        status = Enet_ioctl(gEnetIfObj.hEnet,
                    gEnetIfObj.coreId,
                    CPSW_CPTS_IOCTL_REGISTER_STACK,
                    &prms);
        if (status != ENET_SOK)
        {
            EnetIf_print("%s:%d Error: failed CPSW_CPTS_IOCTL_REGISTER_STACK: %d",
                __FUNCTION__, __LINE__, status);
        }
        else {
            EnetIf_print("%s:%d CPSW_CPTS_IOCTL_REGISTER_STACK is sucessful",
                __FUNCTION__, __LINE__);
            gEnetIfObj.ptpEnabled = 1;
        }
    }

    if (status == ENET_SOK)
    {
        /* Enable PTP over Bare Ethernet - Annex F (IEEE802.3) */
        enableTsEventInArgs.macPort = ENET_MAC_PORT_FIRST;
        CpswCptsEvent_setDefaultPortEventPrms(&enableTsEventInArgs.tsEventCfg);

        ENET_IOCTL_SET_IN_ARGS(&prms, &enableTsEventInArgs);
        status = Enet_ioctl(gEnetIfObj.hEnet,
                            gEnetIfObj.coreId,
                            CPSW_MACPORT_IOCTL_ENABLE_CPTS_EVENT,
                            &prms);
        if (status != ENET_SOK)
        {
            EnetIf_print("%s:%d Error: failed CPSW_MACPORT_IOCTL_ENABLE_CPTS_EVENT: %d",
                __FUNCTION__, __LINE__, status);
        }
    }

    return status;
}

void EnetIf_EventNotifyCallback(void *hEventNotifyCallbackArg,
                                 CpswCpts_Event *eventInfo)
{
    /* Do Nothing */
    if (g_cpsw->cfg.verbose & DEBUG_PTP_APP) {
        EnetIf_print("Entered cpts Callback eventtype=%d, ts=%lld, msgType=%d, seqId=%d ",
            eventInfo->eventType, eventInfo->tsVal, eventInfo->msgType, eventInfo->seqId);
    }
}

int32_t EnetIf_GetTime(uint64_t *ts)
{
    int32_t status = ENET_SOK;
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    Enet_IoctlPrms prms;

    if (gEnetIfObj.hEnet == NULL)
    {
        return ENET_ENOTFOUND;
    }

    if (ts == NULL)
    {
        return ENET_EBADARGS;
    }

    *ts = 0;
    ENET_IOCTL_SET_OUT_ARGS(&prms, ts);
    status = Enet_ioctl(gEnetIfObj.hEnet,
                        gEnetIfObj.coreId,
                        ENET_TIMESYNC_IOCTL_GET_CURRENT_TIMESTAMP,
                        &prms);
    if (status != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: failed ENET_TIMESYNC_IOCTL_GET_CURRENT_TIMESTAMP: %d",
            __FUNCTION__, __LINE__, status);
    }
    else {
        if (g_cpsw->cfg.verbose & DEBUG_PTP_APP) {
            EnetIf_print("%s:%d The timestamp is: %lld", __FUNCTION__, __LINE__, *ts);
        }
    }
#elif defined (CPSW9G) || defined (CPSW5G)
    if (ts == NULL)
    {
        return ENET_EBADARGS;
    }

    *ts = EnetIf_getSynchronizedTime(&gEnetIfObj.syncTimerObj);
#endif
    return status;
}

int32_t EnetIf_SetTime(uint64_t ts)
{
    int32_t status = ENET_SOK;
    Enet_IoctlPrms prms;

    if (gEnetIfObj.hEnet == NULL)
    {
        return ENET_ENOTFOUND;
    }

    if (g_cpsw->cfg.verbose & DEBUG_PTP_APP) {
        EnetIf_print("%s:%d set timestamp is: %lld", __FUNCTION__, __LINE__, ts);
    }

    ENET_IOCTL_SET_IN_ARGS(&prms, &ts);
    status = Enet_ioctl(gEnetIfObj.hEnet,
                        gEnetIfObj.coreId,
                        ENET_TIMESYNC_IOCTL_SET_TIMESTAMP,
                        &prms);
    if (status != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: failed ENET_TIMESYNC_IOCTL_SET_TIMESTAMP: %d",
            __FUNCTION__, __LINE__, status);
    }

    return status;
}

int32_t EnetIf_GetTimestamp(uint8_t msgType,
                             uint16_t sequenceId,
                             uint8_t tx,
                             uint64_t *ts)
{
    int32_t status = ENET_SOK;
    CpswCpts_Event lookUpEventInArgs;
    CpswCpts_Event lookUpEventOutArgs;
    Enet_IoctlPrms prms;

    if (gEnetIfObj.hEnet == NULL)
    {
        return ENET_ENOTFOUND;
    }

    if (ts == NULL)
    {
        return ENET_EBADARGS;
    }

    *ts = 0;
    if (tx)
    {
        lookUpEventInArgs.eventType = CPSW_CPTS_EVENTTYPE_ETH_TRANSMIT;
    }
    else
    {
        lookUpEventInArgs.eventType = CPSW_CPTS_EVENTTYPE_ETH_RECEIVE;
    }
    lookUpEventInArgs.msgType = msgType;
    lookUpEventInArgs.seqId = sequenceId;
    lookUpEventInArgs.portNum = gEnetIfObj.macPort;
    lookUpEventInArgs.domain = 0;

    if (g_cpsw->cfg.verbose & DEBUG_PTP_APP) {
        EnetIf_print("%s:%d PTP_GET_TX_TIMESTAMP/PTP_GET_RX_TIMESTAMP",
                __FUNCTION__, __LINE__);
        EnetIf_print("%s:%d msg_type:%d, sequence_id:%d, tx:%d",
                __FUNCTION__, __LINE__, msgType, sequenceId, tx);
    }
    ENET_IOCTL_SET_INOUT_ARGS(&prms, &lookUpEventInArgs, &lookUpEventOutArgs);

    status = Enet_ioctl(gEnetIfObj.hEnet,
                        gEnetIfObj.coreId,
                        CPSW_CPTS_IOCTL_LOOKUP_EVENT,
                        &prms);
    if (status != ENET_SOK)
    {
        if (g_cpsw->cfg.verbose & DEBUG_PTP_APP) {
            EnetIf_print("%s:%d Error: failed, no retries, CPSW_CPTS_IOCTL_LOOKUP_EVENT: %d",
                __FUNCTION__, __LINE__, status);
        }
    }
    else
    {
        *ts = lookUpEventOutArgs.tsVal;
        if (g_cpsw->cfg.verbose & DEBUG_PTP_APP) {
            EnetIf_print("%s:%d The CPSW_CPTS_IOCTL_LOOKUP_EVENT timestamp is: %lld",
                __FUNCTION__, __LINE__, *ts);
        }
    }

    return status;
}

int32_t EnetIf_SetCompensation(int32_t nudge)
{
    int32_t status = ENET_SOK;
    Enet_IoctlPrms prms;

    if (gEnetIfObj.hEnet == NULL)
    {
        return ENET_ENOTFOUND;
    }

    if (g_cpsw->cfg.verbose & DEBUG_PTP_APP) {
        EnetIf_print("%s:%d nudge is: %d", __FUNCTION__, __LINE__, nudge);
    }

    if (nudge > 127 || nudge < -128)
    {
        uint64_t ts = 0;
        EnetIf_GetTime(&ts);

        ts -= nudge;

        EnetIf_SetTime(ts);
        return ENET_SOK;
    }

    ENET_IOCTL_SET_IN_ARGS(&prms, &nudge);
    status = Enet_ioctl(gEnetIfObj.hEnet,
                        gEnetIfObj.coreId,
                        CPSW_CPTS_IOCTL_SET_TS_NUDGE,
                        &prms);
    if (status != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: failed CPSW_CPTS_IOCTL_SET_TS_NUDGE: %d",
            __FUNCTION__, __LINE__, status);
    }

    return status;
}


int32_t EnetIf_Init(int mac_to_mac, int speed, uint8_t *currentMacAddr)
{
    int32_t status = ENET_SOK;
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    Enet_IoctlPrms prms;
    CpswAle_SetPortStateInArgs setPortStateInArgs;

#if defined (CPSW2G_MAIN)
    EnetIf_enableClocks(gEnetIfObj.enetType, gEnetIfObj.instId);
#endif
#endif

    /* Open the CPSW */
    status = EnetIf_openCpsw(mac_to_mac, speed, currentMacAddr);
    if (status != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: Failed to open CPSW: %d",
            __FUNCTION__, __LINE__, status);
    }

    /* Open CPSW DMA driver */
    if (status == ENET_SOK)
    {
        status = EnetIf_openDma();
        if (status != ENET_SOK)
        {
            EnetIf_print("%s:%d Error: Failed to open CPSW DMA: %d",
                __FUNCTION__, __LINE__, status);
        }
    }

#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    /* Add ALE entry for broadcast MAC address. Note this is needed as the broadcast
     * is disabled via unknownRegMcastFloodMask and other flags in ALE init config.
     * In EthFw we need broadcast to handle ARP entries for clients */
    if (status == ENET_SOK)
    {
        status = EnetIf_setAleBcastEntry();
    }

    /* Enable host port */
    if (status == ENET_SOK)
    {
        setPortStateInArgs.portNum   = CPSW_ALE_HOST_PORT_NUM;
        setPortStateInArgs.portState = CPSW_ALE_PORTSTATE_FORWARD;
        ENET_IOCTL_SET_IN_ARGS(&prms, &setPortStateInArgs);
        prms.outArgs = NULL;
        status       = Enet_ioctl(gEnetIfObj.hEnet,
                                  gEnetIfObj.coreId,
                                  CPSW_ALE_IOCTL_SET_PORT_STATE,
                                  &prms);
        if (status != ENET_SOK)
        {
            EnetIf_print( "%s:%d Error: Failed CPSW_ALE_IOCTL_SET_PORT_STATE: %d",
                __FUNCTION__, __LINE__, status);
        }

        if (status == ENET_SOK)
        {
            ENET_IOCTL_SET_NO_ARGS(&prms);
            status = Enet_ioctl(gEnetIfObj.hEnet,
                                gEnetIfObj.coreId,
                                ENET_HOSTPORT_IOCTL_ENABLE,
                                &prms);
            if (status != ENET_SOK)
            {
                EnetIf_print("%s:%d Error: Failed to enable host port: %d",
                    __FUNCTION__, __LINE__, status);
            }
        }
    }
#endif
    return status;
}

int32_t EnetIf_InitPhy()
{
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    int32_t status = ENET_SOK;
    Enet_IoctlPrms prms;
    int8_t         i;
    bool           alive;

    /* Show alive PHYs */
    if (status == ENET_SOK)
    {
        for (i = 0U; i < ENET_MDIO_PHY_CNT_MAX; i++)
        {
            ENET_IOCTL_SET_INOUT_ARGS(&prms, &i, &alive);
            status = Enet_ioctl(gEnetIfObj.hEnet,
                                gEnetIfObj.coreId,
                                ENET_MDIO_IOCTL_IS_ALIVE,
                                &prms);
            if (status == ENET_SOK)
            {
                if (alive == true)
                {
                    EnetIf_print("PHY %d is alive", i);
                }
            }
            else
            {
                EnetIf_print("%s:%d Error: Failed to get PHY %d alive status: %d",
                    __FUNCTION__, __LINE__, i, status);
            }
        }
    }
    return status;
#elif defined (CPSW9G) || defined (CPSW5G)
    return ENET_SOK;
#endif
}


int32_t EnetIf_CheckPortLinkUp(uint32_t timeout_in_sec)
{
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    bool linked = false;
    int32_t timeout_cnt = (timeout_in_sec * 1000 / 100);

    do {
        linked = EnetIf_isPortLinkUp(gEnetIfObj.hEnet,
                                     gEnetIfObj.coreId,
                                     gEnetIfObj.macPort);
        if ((linked) || (!timeout_cnt)) {
            break;
        }
        EnetIf_wait(100);
        Enet_periodicTick(gEnetIfObj.hEnet);
        timeout_cnt--;
    } while (timeout_cnt);

    if (!linked) {
        return ENET_EFAIL;
    }
    return ENET_SOK;
#elif defined (CPSW9G) || defined (CPSW5G)
    uint32_t i;
    bool linked = false;
    int32_t timeout_cnt = (timeout_in_sec * 1000 / 100);

    do {
        for (i = 0; i < gEnetIfObj.numMacPorts; i++)
        {
            linked = (linked || CpswProxy_isPhyLinked(gEnetIfObj.hCpswProxy));
        }
        if ((linked) || (!timeout_cnt)) {
                break;
        }
        EnetIf_wait(100);
        timeout_cnt--;
    } while (timeout_cnt);
    if (!linked) {
        return ENET_EFAIL;
    }
    return ENET_SOK;
#endif
}

int32_t EnetIf_PortLinkUpCfg(int32_t *speed,
                             int32_t *duplexity)
{
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    Enet_IoctlPrms prms;
    Enet_MacPort portLinkInArgs;
    EnetMacPort_LinkCfg portLinkCfgOutArg;
    int32_t status = ENET_SOK;

    portLinkInArgs = gEnetIfObj.macPort;
    ENET_IOCTL_SET_INOUT_ARGS(&prms, &portLinkInArgs, &portLinkCfgOutArg);

    status = Enet_ioctl(gEnetIfObj.hEnet, gEnetIfObj.coreId, ENET_PER_IOCTL_GET_PORT_LINK_CFG, &prms);
    if (status != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: failed to get port %u's link cfg: %d",
              __FUNCTION__, __LINE__, gEnetIfObj.macPort, status);
        return status;
    }
    if (speed != NULL) {
        switch (portLinkCfgOutArg.speed) {
            case ENET_SPEED_10MBIT:
                *speed = 10;
                break;
            case ENET_SPEED_100MBIT:
                *speed = 100;
                break;
            case ENET_SPEED_1GBIT:
            default:
                *speed = 1000;
                    break;
        }
    }
    if (duplexity != NULL) {
        switch (portLinkCfgOutArg.duplexity) {
            case ENET_DUPLEX_HALF:
                *duplexity = 0;
                break;
            case ENET_DUPLEX_FULL:
            default:
                *duplexity = 1;
                    break;
        }
    }
    return status;
#elif defined (CPSW9G) || defined (CPSW5G)
    // TODO: need to add api to call EthFW for link cfg
    if (speed != NULL) {
        *speed = 1000;
    }
    if (duplexity != NULL) {
        *duplexity = 1;
    }
    return ENET_SOK;
#endif
}

int32_t EnetIf_Close()
{
    int32_t status = ENET_SOK;
    int i;
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    Enet_IoctlPrms prms;

    /* Retreive any pending packets */
    EnetIf_retrieveFreeTxPkts();

    /* Print CPSW statistics of all ports */
    if (status == ENET_SOK)
    {
        EnetIf_showStats();

        EnetIf_print("rxReadyQ count = %d", EnetQueue_getQCount(&gEnetIfObj.rxReadyQ));
        EnetIf_print("freePktInfoQ count = %d", EnetQueue_getQCount(&gEnetIfObj.freePktInfoQ));
    }

    /* Disable host port */
    if (status == ENET_SOK)
    {
        ENET_IOCTL_SET_NO_ARGS(&prms);
        status = Enet_ioctl(gEnetIfObj.hEnet,
                            gEnetIfObj.coreId,
                            ENET_HOSTPORT_IOCTL_DISABLE,
                            &prms);
        if (status != ENET_SOK)
        {
            EnetIf_print("%s:%d Error: Failed to disable host port: %d",
                __FUNCTION__, __LINE__, status);
        }
    }

    /* De-register ptp */
    if ((status == ENET_SOK) && (gEnetIfObj.ptpEnabled == 1))
    {
        ENET_IOCTL_SET_NO_ARGS(&prms);
        status = Enet_ioctl(gEnetIfObj.hEnet,
                            gEnetIfObj.coreId,
                            CPSW_CPTS_IOCTL_UNREGISTER_STACK,
                            &prms);
        if (status != ENET_SOK)
        {
            EnetIf_print("%s:%d Error: Failed to de-register ptp: %d",
                __FUNCTION__, __LINE__, status);
        }
        gEnetIfObj.ptpEnabled = 0;
    }

    /* Print DMA statistics */
    if (status == ENET_SOK)
    {
        /* Print RX flow DMA statistics */
        EnetIf_showRxFlowStats(gEnetIfObj.hRxFlow);
        /* Print TX channel DMA statistics */
        EnetIf_showTxChStats(gEnetIfObj.hTxCh);
    }
#elif defined (CPSW9G) || defined (CPSW5G)
    /* Nothing for CPSW9G/5G */
#endif

    /* Retreive any pending packets */
    EnetIf_retrieveFreeTxPkts();

    /* Close CPSW DMA driver */
    EnetIf_closeDma();

    /* Close CPSW */
    EnetIf_closeCpsw();

    /* Close UDMA driver */
    EnetIf_udmaclose(gEnetIfObj.hUdmaDrv);

    pthread_mutex_destroy(&gEnetIfObj.cpswMutex);
    memset((void *) &gEnetIfObj.spinlockDma, 0, sizeof(intrspin_t));

    for (i = 0; i < ENETIF_MAX_INTR_COUNT; i++) {
        if (gEnetIfObj.hwi[i].isrThreadRunning) {
            MsgSendPulse(gEnetIfObj.hwi[i].isr_event.sigev_coid, SIGEV_PULSE_PRIO_INHERIT, CPSW_ISR_EXIT_PULSE, 1);
            gEnetIfObj.hwi[i].isrThreadRunning = 0;
        }
    }
    delay (10); // 10 msec

    memset(&gEnetIfObj.hwi, 0, sizeof(EnetIf_hwi_info) * ENETIF_MAX_INTR_COUNT);
    gEnetIfObj.currIntrCount = 0;
    gEnetIfObj.hwiRx = NULL;
    gEnetIfObj.hwiTx = NULL;

#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    gEnetIfObj.hEnet = NULL;
#elif defined (CPSW9G) || defined (CPSW5G)
    /* gEnetIfObj.hEnet is still required for detach, so not set to NULL here */
#endif
    EnetIf_print("cpsw deinit done");

#if defined (CPSW2G_MAIN)
    /* Disable peripheral clocks */
    EnetIf_disableClocks(gEnetIfObj.enetType, gEnetIfObj.instId);
#endif

    /* Deinit Enet driver */
    Enet_deinit();

    return status;
}

uintptr_t EnetIf_disableAllIntr(void)
{
    pthread_mutex_lock(&gEnetIfObj.cpswMutex);
    return 0;
}

void EnetIf_restoreAllIntr(uintptr_t key)
{
    pthread_mutex_unlock(&gEnetIfObj.cpswMutex);
}

///////////////////////////////////////////////////////////////////////////////
/* Thread level ISR handler */
void *EnetIf_isr_thread (void *arg)
{
    EnetIf_hwi_info *hwi = (EnetIf_hwi_info *)arg;
    int             rcvid;
    struct _pulse   pulse;
    uintptr_t       interruptNum;
    int             threadExit = 0;

    while (threadExit == 0) {
        rcvid = MsgReceivePulse(hwi->chid, &pulse, sizeof(struct _pulse), NULL);
        if (rcvid != -1) {
            switch (pulse.code) {
                case CPSW_ISR_PULSE:
                    //EnetIf_print("%s: Interrupt %d received\n",__func__, pulse.value.sival_int);
                    interruptNum = pulse.value.sival_int;

                    /* Call the callback function */
                    hwi->isrFxn(hwi->arg);

                    InterruptUnmask(interruptNum, hwi->evtId);
                    break;
                case CPSW_ISR_EXIT_PULSE:
                    threadExit = 1;
                    EnetIf_print("%s: thread exit pulse received\n",__func__);
                    break;
                default:
                    EnetIf_print("%s: Rx Unknown pulse %d received\n",__func__, pulse.code);
                    break;
            }
        }
        else {
#if defined (USE_TRACE_INSTRUMENTATION)
            trace_logi(813, 0, 0);
#endif
            EnetIf_print("%s: MsgReceivePulse failed\n",__func__);
            delay(20);
        }
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
/* Thread level ISR handler for RX path*/
void EnetIf_RxIntr()
{
    gEnetIfObj.hwiRx->isrFxn(gEnetIfObj.hwiRx->arg);
    InterruptUnmask(gEnetIfObj.hwiRx->coreIntrNum, gEnetIfObj.hwiRx->evtId);
}

///////////////////////////////////////////////////////////////////////////////
static void EnetIf_CreateISRThread(EnetIf_hwi_info  *hwi,
                                   EnetOsal_Isr isrFxn,
                                   uint32_t coreIntrNum,
                                   uint32_t intrPriority,
                                   uint32_t triggerType,
                                   void *arg)
{
    pthread_t      tid;
    pthread_attr_t thread_attr;
    struct sched_param  param;
    char threadName[128];

    hwi->chid = ChannelCreate( _NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK);
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    param.sched_priority = intrPriority;
    pthread_attr_setschedparam(&thread_attr, &param);

    if (pthread_create(&tid, &thread_attr, (void *)EnetIf_isr_thread, (void *)hwi) != EOK) {
        EnetIf_print("%s: Unable to create isr thread\n", __FUNCTION__);
        EnetIf_assert(FALSE);
    }
    hwi->isrThreadRunning = 1;
    sprintf(threadName, "Enet_IntrThread_%d",coreIntrNum);
    pthread_setname_np(tid, threadName);

    pthread_setschedprio(tid, intrPriority);
    /* Store pointer to the hwi structure */
    hwi->isrFxn = isrFxn;
    hwi->coreIntrNum = coreIntrNum;
    hwi->intrPriority = intrPriority;
    hwi->arg = (uintptr_t)arg;

    /* Init the pulse for interrupt event */
    hwi->isr_event.sigev_notify = SIGEV_PULSE;
    hwi->isr_event.sigev_code = CPSW_ISR_PULSE;
    hwi->isr_event.sigev_coid = ConnectAttach(0, 0, hwi->chid, _NTO_SIDE_CHANNEL, 0);
    hwi->isr_event.sigev_priority = intrPriority;     /* service interrupts at a higher priority then client requests */
    hwi->isr_event.sigev_value.sival_int = coreIntrNum;

    /*
     * Attach interrupt handler (thread level)
     *
     * _NTO_INTR_FLAGS_TRK_MSK - Track mask/unmask
     * _NTO_INTR_FLAGS_NO_UNMASK - Start with interrupt masked
     */
    hwi->evtId = InterruptAttachEvent (coreIntrNum, &hwi->isr_event,  0 /*_NTO_INTR_FLAGS_NO_UNMASK*/);
    if(hwi->evtId == -1) {
        EnetIf_print("%s: InterruptAttachEvent failed\n",__FUNCTION__);
    }
    else {
        EnetIf_print("%s: InterruptAttachEvent succeed irq/%d coid/%d event/%p\n",__FUNCTION__,
            coreIntrNum, hwi->isr_event.sigev_coid, hwi->evtId);
    }

    return;
}

void *EnetIf_registerIntr(EnetOsal_Isr isrFxn,
                          uint32_t coreIntrNum,
                          uint32_t intrPriority,
                          uint32_t triggerType,
                          void *arg)
{
    EnetIf_hwi_info  *hwi = NULL;

    if (gEnetIfObj.currIntrCount >= ENETIF_MAX_INTR_COUNT) {
        EnetIf_print("%s: MAXed out on the hwi structure\n", __FUNCTION__);
        EnetIf_assert(FALSE);
    }
    hwi = &gEnetIfObj.hwi[gEnetIfObj.currIntrCount];
    memset(hwi, 0, sizeof(EnetIf_hwi_info));
    gEnetIfObj.currIntrCount++;

    if (coreIntrNum == ENET_LLD_INTERRUPT_NUM_CPTS) {
        // ptp event interrupt
        intrPriority = g_cpsw->cpts_intr_prio;
        EnetIf_print("%s: cpsw cpts isr thread priority set to %d\n", __FUNCTION__, intrPriority);

    }
    else if (coreIntrNum == ENET_LLD_INTERRUPT_NUM_STAT) {
        // stat event interrupt
        intrPriority = 21;
        EnetIf_print("%s: cpsw stat isr thread priority set to %d\n", __FUNCTION__, intrPriority);
    }
    else {
        intrPriority = 21;
        EnetIf_print("%s: some unknown cpsw isr thread priority set to %d\n", __FUNCTION__, intrPriority);
    }

    EnetIf_CreateISRThread(hwi, isrFxn, coreIntrNum, intrPriority, triggerType, arg);

    return ((void *)hwi);
}

void *EnetIf_registerIntr2(EnetOsal_Isr isrFxn,
                          uint32_t coreIntrNum,
                          uint32_t intrPriority,
                          void *arg)
{
    /* NOTE: This function gets called only for the TX and RX DMA interrupts */
    EnetIf_hwi_info  *hwi = NULL;

    if (gEnetIfObj.currIntrCount >= ENETIF_MAX_INTR_COUNT) {
        EnetIf_print("%s: MAXed out on the hwi structure\n", __FUNCTION__);
        EnetIf_assert(FALSE);
    }
    hwi = &gEnetIfObj.hwi[gEnetIfObj.currIntrCount];
    memset(hwi, 0, sizeof(EnetIf_hwi_info));
    gEnetIfObj.currIntrCount++;

    // NOTE: The first call to registerIntr would be the TX interrupt followed by RX interrupt
    if (gEnetIfObj.hwiTx == NULL) {
        // tx event interrupt
        gEnetIfObj.hwiTx = hwi;
        intrPriority = g_cpsw->tx_intr_prio;
        EnetIf_print("%s: tx isr %d thread priority set to %d\n", __FUNCTION__, coreIntrNum, intrPriority);

        EnetIf_CreateISRThread(hwi, isrFxn, coreIntrNum, intrPriority, OSAL_ARM_GIC_TRIG_TYPE_LEVEL, arg);
    }
    else if (gEnetIfObj.hwiRx == NULL) {
        if (g_cpsw->rx_pacing) {
            EnetIf_print("%s: Error: Should not come here if rx pacing is enabled!\n", __FUNCTION__);
            EnetIf_assert(FALSE);
        }
        // rx event interrupt
        gEnetIfObj.hwiRx = hwi;
        intrPriority = g_cpsw->rx_intr_prio;
        EnetIf_print("%s: rx isr %d thread priority set to %d\n", __FUNCTION__, coreIntrNum, intrPriority);

        /* Store pointer to the hwi structure */
        hwi->isrFxn = isrFxn;
        hwi->coreIntrNum = coreIntrNum;
        hwi->intrPriority = intrPriority;
        hwi->arg = (uintptr_t)arg;

        /* Init the pulse for interrupt event */
        hwi->isr_event.sigev_notify = SIGEV_PULSE;
        hwi->isr_event.sigev_code = CPSW_RX_PULSE;
        hwi->isr_event.sigev_coid = ConnectAttach(0, 0, g_cpsw->chid, _NTO_SIDE_CHANNEL, 0);
        hwi->isr_event.sigev_priority = intrPriority;     /* service interrupts at a higher priority then client requests */
        hwi->isr_event.sigev_value.sival_int = coreIntrNum;

        /* for RX interrupt, used nw_thread previously create in cpsw_attach */
        hwi->evtId = InterruptAttachEvent (coreIntrNum, &hwi->isr_event,  0 /*_NTO_INTR_FLAGS_NO_UNMASK*/);
        if(hwi->evtId == -1) {
            EnetIf_print("%s: RX InterruptAttachEvent failed\n",__FUNCTION__);
        }
        else {
            EnetIf_print("%s: RX InterruptAttachEvent succeed irq/%d coid/%d event/%p\n",__FUNCTION__,
                coreIntrNum, hwi->isr_event.sigev_coid, hwi->evtId);
        }
    }
    else {
        EnetIf_print("%s: Error: Unknown dma interrupt!\n", __FUNCTION__);
        EnetIf_assert(FALSE);
    }

    return ((void *)hwi);
}

void EnetIf_unRegisterIntr(void *hwiHandle)
{
    EnetIf_hwi_info *hwi = (EnetIf_hwi_info *)hwiHandle;

    InterruptDetach(hwi->evtId);
    ConnectDetach(hwi->isr_event.sigev_coid);

    return;
}

static void EnetIf_delay(uint64_t delayInNsecs)
{
    // NOTE: Current implementation has a limitation if the delayInNsecs value passed
    // is larger than 32 bit value.
    struct timespec cur;
    cur.tv_sec = 0;
    cur.tv_nsec = delayInNsecs;
    nanosleep(&cur, NULL);
    return;
}

static void EnetIf_macMode2MacMii(emac_mode macMode,
                                  EnetMacPort_Interface *mii)
{
    switch (macMode)
    {
        case RMII:
            mii->layerType    = ENET_MAC_LAYER_MII;
            mii->sublayerType = ENET_MAC_SUBLAYER_REDUCED;
            mii->variantType  = ENET_MAC_VARIANT_NONE;
            break;

        case RGMII:
            mii->layerType    = ENET_MAC_LAYER_GMII;
            mii->sublayerType = ENET_MAC_SUBLAYER_REDUCED;
            mii->variantType  = ENET_MAC_VARIANT_FORCED;
            break;

        case GMII:
            mii->layerType    = ENET_MAC_LAYER_GMII;
            mii->sublayerType = ENET_MAC_SUBLAYER_STANDARD;
            mii->variantType  = ENET_MAC_VARIANT_NONE;
            break;

        case SGMII:
            mii->layerType    = ENET_MAC_LAYER_GMII;
            mii->sublayerType = ENET_MAC_SUBLAYER_SERIAL;
            mii->variantType  = ENET_MAC_VARIANT_NONE;
            break;

        case QSGMII:
            mii->layerType    = ENET_MAC_LAYER_GMII;
            mii->sublayerType = ENET_MAC_SUBLAYER_QUAD_SERIAL_MAIN;
            mii->variantType  = ENET_MAC_VARIANT_NONE;
            break;

        case QSGMII_SUB:
            mii->layerType    = ENET_MAC_LAYER_GMII;
            mii->sublayerType = ENET_MAC_SUBLAYER_QUAD_SERIAL_SUB;
            mii->variantType  = ENET_MAC_VARIANT_NONE;
            break;

        case XFI:
            mii->layerType    = ENET_MAC_LAYER_XGMII;
            mii->sublayerType = ENET_MAC_SUBLAYER_STANDARD;
            mii->variantType  = ENET_MAC_VARIANT_NONE;
            break;

        default:
            EnetIf_print("Invalid MAC mode: %u\n", macMode);
            EnetIf_assert(false);
            break;
    }
}

int32_t EnetIf_openCpsw(int mac_to_mac, int speed, uint8_t *currentMacAddr)
{
    Cpsw_Cfg cpswCfg;
    EnetOsal_Cfg osalCfg;
    EnetUtils_Cfg utilsCfg;
    EnetUdma_Cfg dmaCfg;
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    Enet_IoctlPrms prms;
    EnetPer_PortLinkCfg portLinkCfg;
    CpswMacPort_Cfg macCfg;
#endif
    int32_t status = ENET_SOK;

    // clear hwi strucure
    gEnetIfObj.currIntrCount = 0;
    memset(&gEnetIfObj.hwi, 0, sizeof(EnetIf_hwi_info) * ENETIF_MAX_INTR_COUNT);
    gEnetIfObj.hwiTx = NULL;
    gEnetIfObj.hwiRx = NULL;

    // set the mutex with resursive attribute set
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setrecursive(&mattr, PTHREAD_RECURSIVE_ENABLE);
    pthread_mutex_init(&gEnetIfObj.cpswMutex, &mattr);
    pthread_mutexattr_destroy (&mattr);
    // clear the spinlock before use
    memset((void *) &gEnetIfObj.spinlockDma, 0, sizeof(intrspin_t));

    /* Initialize Enet driver (use default OSAL and utils) */
    Enet_initOsalCfg(&osalCfg);
    Enet_initUtilsCfg(&utilsCfg);

    /* Utils Overrides */
    utilsCfg.print     = (Enet_Print)EnetIf_print;
    utilsCfg.physToVirt = (Enet_PhysToVirt)EnetIf_phyToVirtFxn;
    utilsCfg.virtToPhys = (Enet_VirtToPhys)EnetIf_virtToPhyFxn;

    //Note: Override the osal api to disableAllIntr/restoreAllIntr
    osalCfg.disableAllIntr = &EnetIf_disableAllIntr;
    osalCfg.restoreAllIntr = &EnetIf_restoreAllIntr;
    //Note: Over-ride the osal api to registerIntr/restoreAllIntr
    osalCfg.registerIntr = &EnetIf_registerIntr;
    osalCfg.unregisterIntr = &EnetIf_unRegisterIntr;
    //Note: Override the delay
    osalCfg.delay = &EnetIf_delay;


    Enet_init(&osalCfg, &utilsCfg);

    /********************/
    /* Open Enet driver */
    /********************/

    /* Set configuration parameters */

#if defined (CPSW9G) || defined (CPSW5G)
    /* TODO: May be ignored for 9G/5G */
#endif
    /* Set initial config */
    Enet_initCfg(gEnetIfObj.enetType, gEnetIfObj.instId, &cpswCfg, sizeof(cpswCfg));

    /* Peripheral config */
    /* QNX: override based on user need */
    if (g_cpsw->join_vlan != NULL) {
        cpswCfg.vlanCfg.vlanAware = true;
    }
    else {
        cpswCfg.vlanCfg.vlanAware = false;
    }

    /* Host port config */
    cpswCfg.hostPortCfg.removeCrc      = true;
    cpswCfg.hostPortCfg.padShortPacket = true;
    cpswCfg.hostPortCfg.passCrcErrors  = true;
    cpswCfg.hostPortCfg.csumOffloadEn  = true;
    cpswCfg.hostPortCfg.rxMtu          = 1522U;

    /* ALE config */
    cpswCfg.aleCfg.modeFlags                          = CPSW_ALE_CFG_MODULE_EN;
    cpswCfg.aleCfg.agingCfg.autoAgingEn               = true;
    cpswCfg.aleCfg.agingCfg.agingPeriodInMs           = 1000;
    cpswCfg.aleCfg.nwSecCfg.vid0ModeEn                = true;
    cpswCfg.aleCfg.vlanCfg.aleVlanAwareMode           = cpswCfg.vlanCfg.vlanAware;
    cpswCfg.aleCfg.vlanCfg.cpswVlanAwareMode          = cpswCfg.vlanCfg.vlanAware;
    cpswCfg.aleCfg.vlanCfg.unknownUnregMcastFloodMask = 0; // previous: CPSW_ALE_ALL_PORTS_MASK,  see EnetIf_setAleBcastEntry()
    cpswCfg.aleCfg.vlanCfg.unknownRegMcastFloodMask   = 0; // previous: CPSW_ALE_ALL_PORTS_MASK,  see EnetIf_setAleBcastEntry()
    if (g_cpsw->join_vlan != NULL) {
        cpswCfg.aleCfg.vlanCfg.unknownVlanMemberListMask  = 0;
    }
    else {
        cpswCfg.aleCfg.vlanCfg.unknownVlanMemberListMask  = CPSW_ALE_ALL_PORTS_MASK;
    }
    /* QNX: override based on user need */
    if (g_cpsw->promiscuous)
    {
        cpswCfg.aleCfg.modeFlags |= CPSW_ALE_CFG_BYPASS_EN;
    }

#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    /* CPTS config */
    cpswCfg.cptsCfg.hostRxTsEn = false; /* Keep this flag as false to avoid corruption on the RX FIFO MAC port */
    cpswCfg.cptsCfg.cptsRftClkFreq = CPSW_CPTS_RFTCLK_FREQ_500MHZ; /* Setting cpts refclk frequency */

    EnetIf_initResourceConfig(gEnetIfObj.enetType,
                               gEnetIfObj.instId,
                               gEnetIfObj.coreId,
                               &cpswCfg.resCfg,
                               currentMacAddr);
#else
    /* CPTS config */
    cpswCfg.cptsCfg.hostRxTsEn = false;
#endif

#if defined (SOC_J721E) || defined (SOC_J784S4)
    if (gEnetIfObj.enetType == ENET_CPSW_9G)
    {
        EnetIf_print("ENET_CPSW_9G on MAIN NAVSS");
    }
#elif defined (SOC_J7200)
    if (gEnetIfObj.enetType == ENET_CPSW_5G)
    {
        EnetIf_print("ENET_CPSW_5G on MAIN NAVSS");
    }
#endif

    if (gEnetIfObj.enetType == ENET_CPSW_2G)
    {
#if defined (CPSW2G)
        EnetIf_print("ENET_CPSW_2G on MCU NAVSS");
#elif defined (CPSW2G_MAIN)
        EnetIf_print("ENET_CPSW_2G on MAIN NAVSS");
#endif
    }

    dmaCfg.rxChInitPrms.dmaPriority = UDMA_DEFAULT_RX_CH_DMA_PRIORITY;
    // Note: Using preferred channel
    dmaCfg.rxChInitPrms.preferredChNum = gEnetIfObj.preferredUdmaChannel;

    /* App should open UDMA first as UDMA handle is needed to initialize
     * CPSW RX channel */
    gEnetIfObj.hUdmaDrv = EnetIf_udmaOpen(gEnetIfObj.enetType, NULL);
    EnetIf_assert(NULL != gEnetIfObj.hUdmaDrv);

    dmaCfg.hUdmaDrv = gEnetIfObj.hUdmaDrv;
    cpswCfg.dmaCfg = &dmaCfg;

#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    /* Set Enet global runtime log level */
    Enet_setTraceLevel(ENET_TRACE_DEBUG);

    /* Open the Enet driver */
    gEnetIfObj.hEnet = Enet_open(gEnetIfObj.enetType, gEnetIfObj.instId, &cpswCfg, sizeof(cpswCfg));
    if (gEnetIfObj.hEnet == NULL)
    {
        EnetIf_print("%s:%d Error: Failed to open: %d",
            __FUNCTION__, __LINE__, status);
        status = ENET_EFAIL;
    }

    if (status == ENET_SOK)
    {
        EnetMacPort_LinkCfg *linkCfg = &portLinkCfg.linkCfg;
        EnetMacPort_Interface *mii   = &portLinkCfg.mii;
        EnetPhy_Cfg *phyCfg          = &portLinkCfg.phyCfg;

        /* Set port link params */
        portLinkCfg.macPort = gEnetIfObj.macPort;
        portLinkCfg.macCfg = &macCfg;

        CpswMacPort_initCfg(&macCfg);
        EnetIf_macMode2MacMii(gEnetIfObj.macMode, mii);

        if (mac_to_mac == 1) {
            /* mac-to-mac configuration -> no phy */
            phyCfg->phyAddr   = ENETPHY_INVALID_PHYADDR;

            if (speed == 100) {
                linkCfg->speed     = ENET_SPEED_100MBIT;
            }
            else if (speed == 1000) {
                linkCfg->speed     = ENET_SPEED_1GBIT;
            }
            linkCfg->duplexity = ENET_DUPLEX_FULL;
        }
        else
        {
            EnetIfBoard_setPhyConfig(gEnetIfObj.enetType,
                                       portLinkCfg.macPort,
                                       mii,
                                       phyCfg);

            linkCfg->speed     = ENET_SPEED_AUTO;
            linkCfg->duplexity = ENET_DUPLEX_AUTO;
        }

        ENET_IOCTL_SET_IN_ARGS(&prms, &portLinkCfg);
        status = Enet_ioctl(gEnetIfObj.hEnet,
                            gEnetIfObj.coreId,
                            ENET_PER_IOCTL_OPEN_PORT_LINK,
                            &prms);
        if (status != ENET_SOK)
        {
            EnetIf_print("%s:%d Error: failed to open MAC port: %d",
                __FUNCTION__, __LINE__, status);
        }
    }

    if (status == ENET_SOK)
    {
        uint32_t coreId;
        EnetPer_AttachCoreOutArgs attachCoreOutArgs;

        /* Attach the core with RM */
        coreId = gEnetIfObj.coreId;

        ENET_IOCTL_SET_INOUT_ARGS(&prms, &coreId, &attachCoreOutArgs);
        status = Enet_ioctl(gEnetIfObj.hEnet,
                            gEnetIfObj.coreId,
                            ENET_PER_IOCTL_ATTACH_CORE,
                            &prms);
        if (status != ENET_SOK)
        {
            EnetIf_print("%s:%d Error: failed ENET_PER_IOCTL_ATTACH_CORE: %d",
                __FUNCTION__, __LINE__, status);
        }
        else
        {
            gEnetIfObj.coreKey = attachCoreOutArgs.coreKey;
        }
    }
#elif defined (CPSW9G) || defined (CPSW5G)
    /* CPSW9G/5G proxy driver open & attach in cpsw_attach() */
    /* Open UDMA for CPSW NAVSS instance type */
    gEnetIfObj.dmaDataPathConfig.hUdmaDrv = gEnetIfObj.hUdmaDrv;
    gEnetIfObj.dma = EnetUdma_initDataPath(gEnetIfObj.enetType, gEnetIfObj.instId, &gEnetIfObj.dmaDataPathConfig);
    EnetIf_assert(NULL != gEnetIfObj.dma);
#endif

    if (status == ENET_SOK)
    {
        /* appmem open should happen after Cpsw is opened as it uses CpswUtils_Q
         * functions */
        status = EnetIfMem_init();
        EnetIf_assert(ENET_SOK == status);
    }

    return status;
}

void EnetIf_closeCpsw(void)
{
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    Enet_IoctlPrms prms;
    int32_t status;

    /* Close port link */
    ENET_IOCTL_SET_IN_ARGS(&prms, &gEnetIfObj.macPort);

    status = Enet_ioctl(gEnetIfObj.hEnet,
                        gEnetIfObj.coreId,
                        ENET_PER_IOCTL_CLOSE_PORT_LINK,
                        &prms);
    if (status != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: failed to close port link: %d",
            __FUNCTION__, __LINE__, status);
    }

    /* Detach core */
    ENET_IOCTL_SET_IN_ARGS(&prms, &gEnetIfObj.coreKey);

    status = Enet_ioctl(gEnetIfObj.hEnet,
                        gEnetIfObj.coreId,
                        ENET_PER_IOCTL_DETACH_CORE,
                        &prms);
    if (status != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: Failed to detach core key: %d",
            __FUNCTION__, __LINE__, status);
    }

    /* Close Enet driver */
    Enet_close(gEnetIfObj.hEnet);
#elif defined (CPSW9G) || defined (CPSW5G)
    /* Nothing for CPSW9G/5G */
#endif
}

void EnetIf_showStats(void)
{
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    Enet_IoctlPrms prms;
    CpswStats_PortStats portStats;
    int32_t status = ENET_SOK;

    /* Show host port statistics */
    ENET_IOCTL_SET_OUT_ARGS(&prms, &portStats);
    status = Enet_ioctl(gEnetIfObj.hEnet,
                        gEnetIfObj.coreId,
                        ENET_STATS_IOCTL_GET_HOSTPORT_STATS,
                        &prms);
    if (status == ENET_SOK)
    {
        EnetIf_print("-----------------------------------------");
        EnetIf_print(" Port 0 Statistics");
        EnetIf_print("-----------------------------------------");
        EnetIf_printHostPortStats2G((CpswStats_HostPort_2g *)&portStats);
        EnetIf_print("");
    }
    else
    {
        EnetIf_print("%s:%d Error: failed to get host stats: %d",
            __FUNCTION__, __LINE__, status);
    }

    /* Show MAC port statistics */
    if (status == ENET_SOK)
    {
        ENET_IOCTL_SET_INOUT_ARGS(&prms, &gEnetIfObj.macPort, &portStats);
        status = Enet_ioctl(gEnetIfObj.hEnet,
                            gEnetIfObj.coreId,
                            ENET_STATS_IOCTL_GET_MACPORT_STATS,
                            &prms);
        if (status == ENET_SOK)
        {
            EnetIf_print("-----------------------------------------");
            EnetIf_print(" Port 1 Statistics");
            EnetIf_print("-----------------------------------------");
            EnetIf_printMacPortStats2G((CpswStats_MacPort_2g *)&portStats);
            EnetIf_print("");
        }
        else
        {
            EnetIf_print("%s:%d Error: failed to get MAC stats: %d",
                __FUNCTION__, __LINE__, status);
        }
    }
#elif defined (CPSW9G) || defined (CPSW5G)
    /* TODO: Add CPSW9G/5G Proxy support for statistics */
#endif
}

void EnetIf_rxIsrFxn(void *appData)
{
    EnetIf_GetRx(g_cpsw);
}

void EnetIf_txIsrFxn(void *appData)
{
    // nothing to do here
}

int32_t EnetIf_initRxReadyPktQ(void)
{
    EnetDma_PktQ rxFreeQ;
    EnetDma_Pkt *pPktInfo;
    int32_t status = ENET_SOK;
    int i;
    struct mbuf *m;
    struct nw_work_thread   *wtp = WTP;

    EnetQueue_initQ(&rxFreeQ);
    EnetQueue_initQ(&gEnetIfObj.rxReadyQ);

    for (i = 0U; i < gEnetIfObj.numRxPkts; i++)
    {
        m = m_getcl_wtp (M_DONTWAIT, MT_DATA, M_PKTHDR, wtp);
        if (m == NULL) {
            EnetIf_print("%s:%d Error: m_getcl_wtp failed ",
                __FUNCTION__, __LINE__);
            status = ENET_EALLOC;
            return status;
        }

        pPktInfo = (EnetDma_Pkt *)EnetQueue_deq(&gEnetIfObj.freePktInfoQ);
        EnetIf_assert(pPktInfo != NULL);
        pPktInfo->bufPtr     = (uint8_t *)m->m_data;
        pPktInfo->bufPtrPhys = (uint8_t *)pool_phys(m->m_data, m->m_ext.ext_page);
        pPktInfo->userBufLen = MAX_ETH_PACKET_SIZE;
        pPktInfo->orgBufLen  = MAX_ETH_PACKET_SIZE;
        if (gCache_ops)
        {
            CACHE_INVAL(&cachectl, (void *)pPktInfo->bufPtr, (uint64_t)pPktInfo->bufPtrPhys, m->m_ext.ext_size);
        }
        pPktInfo->appPriv    = m;
        ENET_UTILS_SET_PKT_APP_STATE(&pPktInfo->pktState, ENET_PKTSTATE_APP_WITH_FREEQ);
        EnetQueue_enq(&rxFreeQ, &pPktInfo->node);
    }

    /* Retrieve any CPSW packets which are ready */
    status = EnetDma_retrieveRxPktQ(gEnetIfObj.hRxFlow, &gEnetIfObj.rxReadyQ);
    EnetIf_assert(status == ENET_SOK);
    /* There should not be any packet with DMA during init */
    EnetIf_assert(EnetQueue_getQCount(&gEnetIfObj.rxReadyQ) == 0U);

#if (1U == ENET_CFG_DEV_ERROR)
    EnetIf_validatePacketState(&rxFreeQ,
                                 ENET_PKTSTATE_APP_WITH_FREEQ,
                                 ENET_PKTSTATE_APP_WITH_DRIVER);
#endif

    status = EnetDma_submitRxPktQ(gEnetIfObj.hRxFlow,
                            &rxFreeQ);
    if (status != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: failed to send packet.. ret-%d",
            __FUNCTION__, __LINE__, status);
    }

    /* Assert here as during init no. of DMA descriptors should be equal to
     * no. of free Ethernet buffers available with app */

    EnetIf_assert(0U == EnetQueue_getQCount(&rxFreeQ));

    return status;
}

void EnetIf_initPktInfoQ()
{
    uint32_t i;
    EnetDma_Pkt *pPktInfo;

    /* Initialize all queues */
    EnetQueue_initQ(&gEnetIfObj.freePktInfoQ);

    memset(&gPktInfoMem, 0U,
        sizeof(EnetDma_Pkt) * (CPSW_MEM_TX_PKTS_MAX + CPSW_MEM_RX_PKTS_MAX));

    /* Initialize TX & RX EthPkts and queue them to freePktInfoQ */
    for (i = 0U; i < (gEnetIfObj.numTxPkts + gEnetIfObj.numRxPkts); i++)
    {
        pPktInfo = &gPktInfoMem[i];
        EnetDma_initPktInfo(pPktInfo);
        ENET_UTILS_SET_PKT_APP_STATE(&pPktInfo->pktState, ENET_PKTSTATE_APP_WITH_FREEQ);
        EnetQueue_enq(&gEnetIfObj.freePktInfoQ, &pPktInfo->node);
    }

    EnetIf_print("initQs() freePktInfoQ initialized with %d pkts",
                       EnetQueue_getQCount(&gEnetIfObj.freePktInfoQ));

}

int32_t EnetIf_openDma(void)
{
    int32_t status = ENET_SOK;
    EnetUdma_OpenRxFlowPrms cpswRxFlowCfg;
    EnetUdma_OpenTxChPrms cpswTxChCfg;

    EnetIf_initPktInfoQ();

    /* Open the CPSW TX channel  */
    if (status == ENET_SOK)
    {
        EnetDma_initTxChParams(&cpswTxChCfg);

        cpswTxChCfg.hUdmaDrv = gEnetIfObj.hUdmaDrv;
        cpswTxChCfg.cbArg   = &gEnetIfObj;
        // Note: we do not register callback for TX channel
        cpswTxChCfg.notifyCb = NULL;
        // Note: Using preferred channel
        cpswTxChCfg.preferredChNum = gEnetIfObj.preferredUdmaChannel;

        EnetIf_setCommonTxChPrms(&cpswTxChCfg, gEnetIfObj.numTxPkts);

#if defined (CPSW2G) || defined (CPSW2G_MAIN)
        EnetIf_openTxCh(gEnetIfObj.hEnet,
                              gEnetIfObj.coreKey,
                              gEnetIfObj.coreId,
                              &gEnetIfObj.txChNum,
                              &gEnetIfObj.hTxCh,
                              &cpswTxChCfg);
#elif defined (CPSW9G) || defined (CPSW5G)
        EnetIf_openTxCh(&gEnetIfObj,
                         &cpswTxChCfg);
#endif
        // Note: We do not call EnetDma_enableTxEvent call for
        // registering the TX event

        if (NULL == gEnetIfObj.hTxCh)
        {
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
            /* Free the Ch Num if open Tx Ch failed */
            EnetIf_freeTxCh(gEnetIfObj.hEnet,
                             gEnetIfObj.coreKey,
                             gEnetIfObj.coreId,
                             gEnetIfObj.txChNum);
#elif defined (CPSW9G) || defined (CPSW5G)
            CpswProxy_freeTxCh(gEnetIfObj.hCpswProxy,
                               gEnetIfObj.txChNum);
#endif
            EnetIf_print("%s:%d Error: EnetDma_openTxCh failed to open: %d",
                __FUNCTION__, __LINE__, status);
            status = ENET_EFAIL;
        }
    }

    /* Open the CPSW RX flow  */
    if (status == ENET_SOK)
    {
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
        EnetDma_initRxChParams(&cpswRxFlowCfg);
        if (g_cpsw->rx_pacing) {
            cpswRxFlowCfg.notifyCb = NULL;
        }
        else {
            cpswRxFlowCfg.notifyCb = EnetIf_rxIsrFxn;
        }
        cpswRxFlowCfg.hUdmaDrv = gEnetIfObj.hUdmaDrv;
        cpswRxFlowCfg.cbArg   = &gEnetIfObj;

        EnetIf_setCommonRxFlowPrms(&cpswRxFlowCfg, gEnetIfObj.numRxPkts);

        EnetIf_openRxFlow(gEnetIfObj.hEnet,
                           gEnetIfObj.coreKey,
                           gEnetIfObj.coreId,
                           true,
                           &gEnetIfObj.rxStartFlowIdx,
                           &gEnetIfObj.rxFlowIdx,
                           &gEnetIfObj.hostMacAddr[0U],
                           &gEnetIfObj.hRxFlow,
                           &cpswRxFlowCfg);
#elif defined (CPSW9G) || defined (CPSW5G)
        EnetDma_initRxChParams(&cpswRxFlowCfg);
        EnetIf_setCommonRxFlowPrms(&cpswRxFlowCfg, gEnetIfObj.numRxPkts);
        if (g_cpsw->rx_pacing) {
            cpswRxFlowCfg.notifyCb = NULL;
        }
        else {
            cpswRxFlowCfg.notifyCb = EnetIf_rxIsrFxn;
        }
        cpswRxFlowCfg.hUdmaDrv = gEnetIfObj.hUdmaDrv;
        cpswRxFlowCfg.cbArg   = &gEnetIfObj;
        cpswRxFlowCfg.rxFlowMtu = gEnetIfObj.rxMtu;

        EnetIf_openRxFlow(&gEnetIfObj,
                           false,
                           &cpswRxFlowCfg);
#endif

        if (NULL == gEnetIfObj.hRxFlow)
        {
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
            EnetIf_freeRxFlow(gEnetIfObj.hEnet,
                               gEnetIfObj.coreKey,
                               gEnetIfObj.coreId,
                               gEnetIfObj.rxFlowIdx);
#elif defined (CPSW9G) || defined (CPSW5G)
            CpswProxy_freeRxFlow(gEnetIfObj.hCpswProxy,
                                 gEnetIfObj.rxStartFlowIdx,
                                 gEnetIfObj.rxFlowIdx);
#endif
            EnetIf_print("%s:%d Error: EnetDma_openRxCh failed to open: %d",
                __FUNCTION__, __LINE__, status);
            EnetIf_assert(NULL != gEnetIfObj.hRxFlow);
        }
        else
        {
            EnetIf_print("Host MAC address: ");
            EnetIf_printMacAddr(gEnetIfObj.hostMacAddr);
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
            /* For MAC loopback disable secure flag for the host port entry */
            EnetIf_changeHostAleEntry(&gEnetIfObj.hostMacAddr[0U]);
#elif defined (CPSW9G) || defined (CPSW5G)
            /* TODO: Is this needed for 9G/5G */
#endif

            /* Submit all ready RX buffers to DMA.*/
            status = EnetIf_initRxReadyPktQ();
        }
    }

    return status;
}

void EnetIf_freePktInfoQ(EnetDma_PktQ *pPktInfoQ)
{
    EnetDma_Pkt *pktInfo;
    uint32_t pktCnt, i;
    struct mbuf *m = NULL;

    pktCnt = EnetQueue_getQCount(pPktInfoQ);
    /* Free all retrieved packets from DMA */
    for (i = 0U; i < pktCnt; i++)
    {
        pktInfo = (EnetDma_Pkt *)EnetQueue_deq(pPktInfoQ);
        if (pktInfo->appPriv != NULL) {
            m = pktInfo->appPriv;
            m_freem (m);
            pktInfo->appPriv = NULL;
        }
    }
}

void EnetIf_closeDma(void)
{
    EnetDma_PktQ fqPktInfoQ;
    EnetDma_PktQ cqPktInfoQ;

    EnetQueue_initQ(&fqPktInfoQ);
    EnetQueue_initQ(&cqPktInfoQ);

    /* There should not be any ready packet */
    EnetIf_assert(0U == EnetQueue_getQCount(&gEnetIfObj.rxReadyQ));

#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    /* Close RX Flow */
    EnetIf_closeRxFlow(gEnetIfObj.hEnet,
                        gEnetIfObj.coreKey,
                        gEnetIfObj.coreId,
                        true,
                        &fqPktInfoQ,
                        &cqPktInfoQ,
                        gEnetIfObj.rxStartFlowIdx,
                        gEnetIfObj.rxFlowIdx,
                        gEnetIfObj.hostMacAddr,
                        gEnetIfObj.hRxFlow);
#elif defined (CPSW9G) || defined (CPSW5G)
    /* Close RX Flow */
    EnetIf_closeRxFlow(&gEnetIfObj,
                        false,
                        &fqPktInfoQ,
                        &cqPktInfoQ);
#endif

    EnetIf_freePktInfoQ(&fqPktInfoQ);
    EnetIf_freePktInfoQ(&cqPktInfoQ);
    /* Close TX channel */
    EnetQueue_initQ(&fqPktInfoQ);
    EnetQueue_initQ(&cqPktInfoQ);

#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    EnetIf_closeTxCh(gEnetIfObj.hEnet,
                      gEnetIfObj.coreKey,
                      gEnetIfObj.coreId,
                      &fqPktInfoQ,
                      &cqPktInfoQ,
                      gEnetIfObj.hTxCh,
                      gEnetIfObj.txChNum);
#elif defined (CPSW9G) || defined (CPSW5G)
    EnetIf_closeTxCh(&gEnetIfObj,
                      &fqPktInfoQ,
                      &cqPktInfoQ);
#endif
    EnetIf_freePktInfoQ(&fqPktInfoQ);
    EnetIf_freePktInfoQ(&cqPktInfoQ);
    EnetIf_freePktInfoQ(&gEnetIfObj.rxReadyQ);
    EnetIf_freePktInfoQ(&gEnetIfObj.freePktInfoQ);

    EnetIfMem_deInit();

    //TODO:: Figure out why only Rx should be set to NULL
    gEnetIfObj.hwiRx = NULL;
    //gEnetIfObj.hwiTx = NULL;

    // terminate the isr threads
    for (int i = 0; i < ENETIF_MAX_INTR_COUNT; i++) {
        if (gEnetIfObj.hwi[i].isrThreadRunning) {
            MsgSendPulse(gEnetIfObj.hwi[i].isr_event.sigev_coid, SIGEV_PULSE_PRIO_INHERIT, CPSW_ISR_EXIT_PULSE, 1);
            gEnetIfObj.hwi[i].isrThreadRunning = 0;
        }
    }
}

int32_t EnetIf_setAleBcastEntry()
{
    Enet_IoctlPrms prms;
    uint32_t setMcastOutArgs;
    CpswAle_SetMcastEntryInArgs setMcastInArgs;
    uint8_t bCastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    int32_t status;

    memcpy(&setMcastInArgs.addr.addr[0], &bCastAddr[0U], sizeof(setMcastInArgs.addr.addr));
    setMcastInArgs.addr.vlanId = 0U;
    setMcastInArgs.info.super  = false;
    setMcastInArgs.info.fwdState   = CPSW_ALE_FWDSTLVL_FWD;
    setMcastInArgs.info.portMask   = CPSW_ALE_ALL_PORTS_MASK;
    setMcastInArgs.info.numIgnBits = 0U;

    ENET_IOCTL_SET_INOUT_ARGS(&prms, &setMcastInArgs, &setMcastOutArgs);

    status = Enet_ioctl(gEnetIfObj.hEnet,
                        gEnetIfObj.coreId,
                        CPSW_ALE_IOCTL_ADD_MCAST,
                        &prms);
    if (status != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: ADD_MULTICAST ioctl failed: %d",
            __FUNCTION__, __LINE__, status);
    }

    return status;
}

int32_t EnetIf_changeHostAleEntry(uint8_t macAddr[])
{
    int32_t status;
    Enet_IoctlPrms prms;
    uint32_t setUcastOutArgs;
    CpswAle_SetUcastEntryInArgs setUcastInArgs;

    memcpy(&setUcastInArgs.addr.addr[0U], macAddr, sizeof(setUcastInArgs.addr.addr));
    setUcastInArgs.addr.vlanId  = 0U;
    setUcastInArgs.info.portNum = 0U;
    setUcastInArgs.info.trunk   = false;
    setUcastInArgs.info.blocked = false;
    setUcastInArgs.info.secure  = false;
    setUcastInArgs.info.super   = 0U;
    setUcastInArgs.info.ageable = false;

    ENET_IOCTL_SET_INOUT_ARGS(&prms, &setUcastInArgs, &setUcastOutArgs);

    status = Enet_ioctl(gEnetIfObj.hEnet,
                        gEnetIfObj.coreId,
                        CPSW_ALE_IOCTL_ADD_UCAST,
                        &prms);
    if (status != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: failed CPSW_ALE_IOCTL_SET_PORT_STATE: %d",
            __FUNCTION__, __LINE__, status);
    }

    return status;
}

uint32_t EnetIf_retrieveFreeTxPkts(void)
{
    EnetDma_PktQ txFreeQ;
    EnetDma_Pkt *pktInfo;
    int32_t status;
    uint32_t txFreeQCnt = 0U;
    struct mbuf *m = NULL;

    EnetQueue_initQ(&txFreeQ);

    /* Retrieve any CPSW packets that may be free now */
    status = EnetDma_retrieveTxPktQ(gEnetIfObj.hTxCh, &txFreeQ);
    if (status == ENET_SOK)
    {
        txFreeQCnt = EnetQueue_getQCount(&txFreeQ);

        pktInfo = (EnetDma_Pkt *)EnetQueue_deq(&txFreeQ);
        while (NULL != pktInfo)
        {
            // start
            m = pktInfo->appPriv;
            EnetIf_assert(m != NULL);
            m_freem (m);
            pktInfo->appPriv = NULL;

#if (1U == ENET_CFG_DEV_ERROR)
            EnetDma_checkPktState(&pktInfo->pktState,
                                    ENET_PKTSTATE_MODULE_APP,
                                    ENET_PKTSTATE_APP_WITH_DRIVER,
                                    ENET_PKTSTATE_APP_WITH_FREEQ);
#endif

            EnetQueue_enq(&gEnetIfObj.freePktInfoQ, &pktInfo->node);
            pktInfo = (EnetDma_Pkt *)EnetQueue_deq(&txFreeQ);
        }
    }
    else
    {
        EnetIf_print("%s:%d Error: failed to retrieve pkts: %d",
            __FUNCTION__, __LINE__, status);
    }

    return txFreeQCnt;
}

uint32_t EnetIf_receivePkts(void)
{
    int32_t status;
    uint32_t rxReadyCnt = 0U;

    EnetQueue_initQ(&gEnetIfObj.rxReadyQ);

    /* Retrieve any CPSW packets which are ready */
    status = EnetDma_retrieveRxPktQ(gEnetIfObj.hRxFlow, &gEnetIfObj.rxReadyQ);
    if (status == ENET_SOK)
    {
        rxReadyCnt = EnetQueue_getQCount(&gEnetIfObj.rxReadyQ);
    }
    else
    {
        EnetIf_print("%s:%d Error: failed to retrieve pkts: %d",
            __FUNCTION__, __LINE__, status);
    }

    return rxReadyCnt;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void EnetIf_Tick()
{
    Enet_periodicTick(gEnetIfObj.hEnet);
}

void EnetIf_GetStats(nic_stats_t *stats)
{
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    Enet_IoctlPrms prms;
    CpswStats_PortStats portStats;
    int32_t status = ENET_SOK;
    nic_ethernet_stats_t    *estats = &stats->un.estats;
    CpswStats_MacPort_2g *st = (CpswStats_MacPort_2g *)&portStats;

    if (gEnetIfObj.hEnet == NULL)
    {
        return;
    }

    ENET_IOCTL_SET_INOUT_ARGS(&prms, &gEnetIfObj.macPort, &portStats);
    status = Enet_ioctl(gEnetIfObj.hEnet,
                        gEnetIfObj.coreId,
                        ENET_STATS_IOCTL_GET_MACPORT_STATS,
                        &prms);
    if (status == ENET_SOK)
    {
        stats->rxed_ok = st->rxGoodFrames;
        stats->rxed_broadcast = st->rxBcastFrames;
        stats->rxed_multicast = st->rxMcastFrames;
        stats->octets_rxed_ok = st->rxOctets;

        stats->txed_ok = st->txGoodFrames;
        stats->txed_broadcast = st->txBcastFrames;
        stats->txed_multicast = st->txMcastFrames;
        stats->octets_txed_ok = st->txOctets;

        estats->align_errors = st->rxAlignCodeErrors;
        estats->single_collisions = st->txSingleCollFrames;
        estats->multi_collisions = st->txMultipleCollFrames;
        estats->fcs_errors = st->rxCrcErrors;
        estats->tx_deferred = st->txDeferredFrames;
        estats->late_collisions = st->txLateCollFrames;
        estats->xcoll_aborted = st->txExcessiveCollFrames;
        estats->no_carrier = st->txCarrierSenseErrors;
        estats->oversized_packets = st->rxOversizedFrames;
        estats->jabber_detected = st->rxJabberFrames;
        estats->short_packets = st->rxUndersizedFrames;
        estats->total_collision_frames = st->txCollisionFrames;
    }
    else
    {
        EnetIf_print("%s:%d Error: failed to get MAC stats: %d",
            __FUNCTION__, __LINE__, status);
    }

#elif defined (CPSW9G) || defined (CPSW5G)
    /* TODO: Add CPSW9G/5G Proxy support for statistics */
#endif
    /* Print port stats */
    EnetIf_showStats();

    /* Print CPSW PHY regs */
    EnetIf_ShowPhyRegs();

    /* Show ALE table entries */
    EnetIf_AleDumpTable();

    /* Show ALE Policer entries */
    EnetIf_AleDumpPolicer();

    /* Misc info */
    EnetIf_print("-----------------------------------------");
    EnetIf_print("Queue info");
    EnetIf_print("-----------------------------------------");
    EnetIf_print("rxReadyQ count = %d", EnetQueue_getQCount(&gEnetIfObj.rxReadyQ));
    EnetIf_print("freePktInfoQ count = %d", EnetQueue_getQCount(&gEnetIfObj.freePktInfoQ));
}


int32_t EnetIf_SendPkt(struct mbuf* m)
{
    int32_t retVal;
    EnetDma_Pkt *pktInfo;
    EnetDma_PktQ txSubmitQ;
    int offload_flags;

    EnetIf_assert(m != NULL);

    /* Transmit a single packet */
    EnetQueue_initQ(&txSubmitQ);

    /* Dequeue one free TX Eth packet */
    pktInfo = (EnetDma_Pkt *) EnetQueue_deq(&gEnetIfObj.freePktInfoQ);
    if (pktInfo == NULL)
    {
        return ENET_EFAIL;
    }

    // start
    if (!ENET_UTILS_IS_ALIGNED(m->m_data, UDMA_CACHELINE_ALIGNMENT))
    {
        EnetIf_print("%s:%d: Error: Send m buffer addr=0x%lx is not aligned",
            __FUNCTION__, __LINE__, m->m_data);
    }

    if (g_cpsw->cfg.verbose & DEBUG_BUFFER) {
        EnetIf_print("TX Buffer ------> size:%d", m->m_len);
        EnetIf_printFrame((EthFrame *)m->m_data, m->m_len - sizeof(EthFrameHeader));
    }

    /* hardware checksumming stuff */
    if (g_cpsw->csum_flag_tx)
    {
        uint32_t csInsertPos = 0;
        uint32_t csStartPos = 0;
        uint32_t csNumBytes = 0;

        offload_flags = m->m_pkthdr.csum_flags &  (M_CSUM_TCPv4 | M_CSUM_UDPv4 | M_CSUM_TCPv6 | M_CSUM_UDPv6);
        if (offload_flags) {
            if (cpsw_csum_offload_setup (g_cpsw, m, offload_flags, &csInsertPos, &csStartPos, &csNumBytes) == -1)
            {
                EnetIf_print("%s:%d: csum_offload_setup() failed, tx packet dropped!", __FUNCTION__, __LINE__);
                return 1;
            }
            ENETUDMA_CPPIPSI_SET_CHKSUM_RES (pktInfo->chkSumInfo, csInsertPos);
            ENETUDMA_CPPIPSI_SET_CHKSUM_STARTBYTE (pktInfo->chkSumInfo, csStartPos);
            ENETUDMA_CPPIPSI_SET_CHKSUM_INV_FLAG (pktInfo->chkSumInfo, true);
            ENETUDMA_CPPIPSI_SET_CHKSUM_BYTECNT (pktInfo->chkSumInfo, csNumBytes);
        }
    }

    pktInfo->bufPtr = (uint8_t *)m->m_data;
    pktInfo->bufPtrPhys = (uint8_t *)pool_phys(m->m_data, m->m_ext.ext_page);
    pktInfo->orgBufLen = m->m_ext.ext_size;
    pktInfo->userBufLen = m->m_len;
    pktInfo->txPortNum = ENET_MAC_PORT_INV;
    pktInfo->appPriv = m;
    if (gCache_ops)
    {
        CACHE_FLUSH(&cachectl, (void *)pktInfo->bufPtr, (uint64_t)pktInfo->bufPtrPhys, pktInfo->userBufLen);
    }
    // end

#if (1U == ENET_CFG_DEV_ERROR)
    EnetDma_checkPktState(&pktInfo->pktState,
                            ENET_PKTSTATE_MODULE_APP,
                            ENET_PKTSTATE_APP_WITH_FREEQ,
                            ENET_PKTSTATE_APP_WITH_DRIVER);
#endif

    /* Enqueue the packet for later transmission */
    EnetQueue_enq(&txSubmitQ, &pktInfo->node);

    retVal = EnetDma_submitTxPktQ(gEnetIfObj.hTxCh,
                                  &txSubmitQ);
    if (retVal != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: failed to send packet.. ret-%d",
            __FUNCTION__, __LINE__, retVal);
    }

    return retVal;
}

extern struct mbuf * cpsw_process_rx(void *arg, struct mbuf *);

int32_t EnetIf_GetRx(void *arg)
{
    int32_t retVal = ENET_SOK;
    EnetDma_PktQ rxFreeQ;
    EnetDma_Pkt *pktInfo;
    uint32_t rxReadyCnt;
    struct mbuf *m, *mNew;
    uint32_t csumInfo;
    bool chkSumErr = false;
#if defined (USE_TRACE_INSTRUMENTATION)
    uint pktCnt = 0;
#endif

    /* Get the packets received so far */
    /* It may be long time operation, try to read all packets out, just in case miss the pulse */
    while ((rxReadyCnt = EnetIf_receivePkts()) > 0U)
    {
#if defined (USE_TRACE_INSTRUMENTATION)
        pktCnt++;
#endif
        EnetQueue_initQ(&rxFreeQ);

        /* Consume the received packets and release them */
        pktInfo = (EnetDma_Pkt *) EnetQueue_deq( &gEnetIfObj.rxReadyQ);
        while (NULL != pktInfo)
        {
#if (1U == ENET_CFG_DEV_ERROR)
            EnetDma_checkPktState(&pktInfo->pktState,
                                    ENET_PKTSTATE_MODULE_APP,
                                    ENET_PKTSTATE_APP_WITH_DRIVER,
                                    ENET_PKTSTATE_APP_WITH_READYQ);
#endif
            m = pktInfo->appPriv;
            m->m_pkthdr.len = m->m_len = pktInfo->userBufLen;
            if (gCache_ops)
            {
                CACHE_INVAL(&cachectl, (void *)pktInfo->bufPtr, (uint64_t)pktInfo->bufPtrPhys, m->m_ext.ext_size);
            }

            if (g_cpsw->csum_flag_rx)
            {
                /* We don't check if HW checksum offload is enabled while checking for checksum error
                 * as default value of this field when offload not enabled is false */
                csumInfo = pktInfo->chkSumInfo;
                if ( ENETUDMA_CPPIPSI_GET_IPV4_FLAG(csumInfo)||
                     ENETUDMA_CPPIPSI_GET_IPV6_FLAG(csumInfo) )
                {
                    chkSumErr = ENETUDMA_CPPIPSI_GET_CHKSUM_ERR_FLAG(csumInfo);
                }

                if (ENETUDMA_CPPIPSI_GET_IPV4_FLAG(csumInfo))
                {
                    m->m_pkthdr.csum_flags |= M_CSUM_TCPv4 | M_CSUM_UDPv4;
                }
                else if (ENETUDMA_CPPIPSI_GET_IPV6_FLAG(csumInfo))
                {
                    m->m_pkthdr.csum_flags |= M_CSUM_TCPv6 | M_CSUM_UDPv6;
                }
                if (chkSumErr)
                {
                    m->m_pkthdr.csum_flags |= M_CSUM_TCP_UDP_BAD;
                }
            }

            if (g_cpsw->cfg.verbose & DEBUG_BUFFER) {
                EnetIf_print("RX Buffer ------> size:%d", m->m_len);
                EnetIf_printFrame((EthFrame *)m->m_data, m->m_len - sizeof(EthFrameHeader));
            }

            /* Get a packet/buffer to replace the one that was filled */
            /* Consume the packet by sending to upper layer */
            mNew = cpsw_process_rx(arg, m);
            if (mNew == NULL) {
                EnetIf_print("%s:%d: Error: cpsw_process_rx returned NULL",
                    __FUNCTION__, __LINE__);
                EnetIf_assert(false);
            }
            else {
                EnetIf_assert(ENET_UTILS_IS_ALIGNED(mNew->m_data, UDMA_CACHELINE_ALIGNMENT));

                pktInfo->bufPtr     = (uint8_t *)mNew->m_data;
                pktInfo->bufPtrPhys = (uint8_t *)pool_phys(mNew->m_data, mNew->m_ext.ext_page);
                pktInfo->orgBufLen  = MAX_ETH_PACKET_SIZE;
                pktInfo->userBufLen = MAX_ETH_PACKET_SIZE;
                if (gCache_ops)
                {
                    CACHE_INVAL(&cachectl, (void *)pktInfo->bufPtr, (uint64_t)pktInfo->bufPtrPhys, m->m_ext.ext_size);
                }
                pktInfo->appPriv    = mNew;
            }

#if (1U == ENET_CFG_DEV_ERROR)
            EnetDma_checkPktState(&pktInfo->pktState,
                                    ENET_PKTSTATE_MODULE_APP,
                                    ENET_PKTSTATE_APP_WITH_READYQ,
                                    ENET_PKTSTATE_APP_WITH_FREEQ);
#endif

            EnetQueue_enq(&rxFreeQ, &pktInfo->node);
            pktInfo = (EnetDma_Pkt *) EnetQueue_deq( &gEnetIfObj.rxReadyQ);
        }

        /*Submit now processed buffers */
        {
#if (1U == ENET_CFG_DEV_ERROR)
            EnetIf_validatePacketState(&rxFreeQ,
                                             ENET_PKTSTATE_APP_WITH_FREEQ,
                                             ENET_PKTSTATE_APP_WITH_DRIVER);
#endif

            retVal = EnetDma_submitRxPktQ(gEnetIfObj.hRxFlow,
                                    &rxFreeQ);
            if (retVal != ENET_SOK)
            {
                EnetIf_print("%s:%d Error: failed to send packet.. ret-%d",
                    __FUNCTION__, __LINE__, retVal);
            }
        }
    }
#if defined (USE_TRACE_INSTRUMENTATION)
    trace_logi(806, pktCnt, 0);
#endif

    return retVal;
}


uint32_t EnetIf_GetTxFreeQCnt() {
    return EnetQueue_getQCount(&gEnetIfObj.freePktInfoQ);
}

#if defined (CPSW9G) || defined (CPSW5G)
void EnetIf_RegisterIPv4Address(uint8_t *ipv4Addr)
{
    if (gEnetIfObj.ipv4Addr[0] != 0)
    {
        CpswProxy_unregisterIPV4Addr(gEnetIfObj.hCpswProxy,
                                     gEnetIfObj.ipv4Addr);
        memset(gEnetIfObj.ipv4Addr, 0, sizeof(gEnetIfObj.ipv4Addr));
    }
    CpswProxy_registerIPV4Addr(gEnetIfObj.hCpswProxy,
                               gEnetIfObj.hostMacAddr,
                               ipv4Addr);
    memcpy(gEnetIfObj.ipv4Addr, ipv4Addr, ENET_IPv4_ADDR_LEN);
}

int32_t EnetIf_UnregisterIPv4Address(uint8_t *ipv4Addr)
{
    int32_t ret = CpswProxy_unregisterIPV4Addr(gEnetIfObj.hCpswProxy, ipv4Addr);

    if(ret == CPSWPROXY_SOK)
    {
        memset(gEnetIfObj.ipv4Addr, 0, sizeof(gEnetIfObj.ipv4Addr));
    }
    else
    {
        EnetIf_print("%s:%d Error: Failed to unregister IPv4 Address...ret:%d", __FUNCTION__, __LINE__);
    }

    return ret;
}

void EnetIf_enableSyncTimer(void)
{
    int32_t status;
    uint64_t baseaddr;

    memset(&gEnetIfObj.syncTimerObj, 0, sizeof(EnetIf_SyncTimerObj));

    gEnetIfObj.syncTimerObj.GTC0_GTC_CFG0_Base = (uint32_t *) mmap_device_memory(0,
            CSL_GTC0_GTC_CFG0_SIZE, PROT_READ|PROT_WRITE|PROT_NOCACHE, 0, CSL_GTC0_GTC_CFG0_BASE);
    EnetIf_assert(MAP_FAILED != gEnetIfObj.syncTimerObj.GTC0_GTC_CFG0_Base);

    gEnetIfObj.syncTimerObj.GTC0_GTC_CFG1_Base = (uint32_t *) mmap_device_memory(0,
            CSL_GTC0_GTC_CFG1_SIZE, PROT_READ|PROT_WRITE|PROT_NOCACHE, 0, CSL_GTC0_GTC_CFG1_BASE);
    EnetIf_assert(MAP_FAILED != gEnetIfObj.syncTimerObj.GTC0_GTC_CFG1_Base);

    gEnetIfObj.syncTimerObj.GTC0_GTC_CFG2_Base = (uint32_t *) mmap_device_memory(0,
            CSL_GTC0_GTC_CFG2_SIZE, PROT_READ|PROT_WRITE|PROT_NOCACHE, 0, CSL_GTC0_GTC_CFG2_BASE);
    EnetIf_assert(MAP_FAILED != gEnetIfObj.syncTimerObj.GTC0_GTC_CFG2_Base);

    gEnetIfObj.syncTimerObj.GTC0_GTC_CFG3_Base = (uint32_t *) mmap_device_memory(0,
            CSL_GTC0_GTC_CFG3_SIZE, PROT_READ|PROT_WRITE|PROT_NOCACHE, 0, CSL_GTC0_GTC_CFG3_BASE);
    EnetIf_assert(MAP_FAILED != gEnetIfObj.syncTimerObj.GTC0_GTC_CFG3_Base);

    EnetIf_print("EnetIf_InitSyncTimer(): Set GTC0 Push event from BIT %d\n", CPSW_REMOTE_APP_GTC_PUSHEVT_BIT_SEL);
    baseaddr = (uint64_t) gEnetIfObj.syncTimerObj.GTC0_GTC_CFG0_Base;
    baseaddr += CSL_GTC_CFG0_PUSHEVT;
    /* Configure GTC push event */
    CSL_REG32_WR(baseaddr, CPSW_REMOTE_APP_GTC_PUSHEVT_BIT_SEL);

    /* Register callback */
    status = CpswProxy_registerNotifyCb(gEnetIfObj.hCpswProxy,
                                        ETHREMOTECFG_NOTIFY_HWPUSH,
                                        EnetIf_calcSyncTimeParams,
                                         (void *)ETHREMOTECFG_SWITCH_PORT_0);
    if (status == ENET_EALREADYOPEN)
    {
        EnetIf_print("CpswProxy_registerNotifyCb(): Callback is registered already\n");
    }
    else
    {
        EnetIf_print("CpswProxy_registerRemoteTimer() for GTC0 Push Event\n");
        /* Send request to Ethfw to configure TSR */
        CpswProxy_registerRemoteTimer(gEnetIfObj.hCpswProxy,
                                      CSLR_TIMESYNC_INTRTR0_IN_GTC0_GTC_PUSH_EVENT_0,
                                      CPSW_REMOTE_APP_CPTS_HW_PUSH_NUM);
    }

    /* Enable Timer */
    baseaddr = (uint64_t) gEnetIfObj.syncTimerObj.GTC0_GTC_CFG1_Base;
    baseaddr += CSL_GTC_CFG1_CNTCR;
    if ((CSL_REG32_RD(baseaddr) & 0x1U) == 0U)
    {
        EnetIf_print("EnetIf_InitSyncTimer(): GTC0 is not enabled. Enable it now.\n");
        CSL_REG32_WR(baseaddr, 0x1U);
    }

    return;
}

void EnetIf_disableSyncTimer(void)
{
    CpswProxy_unregisterRemoteTimer(gEnetIfObj.hCpswProxy,
                                    CPSW_REMOTE_APP_CPTS_HW_PUSH_NUM);
    CpswProxy_unregisterNotifyCb(gEnetIfObj.hCpswProxy, ETHREMOTECFG_NOTIFY_HWPUSH);
}
#endif

/*****************************************************************************/
void EnetIf_AddVlan(uint16_t vlanId, uint16_t enable)
{
    int32_t status;
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    Enet_IoctlPrms prms;
    CpswAle_VlanEntryInfo inArgs;
    uint32_t aleEntryIdx;
    CpswAle_VlanIdInfo getVlanInArgs;
    CpswAle_GetVlanEntryOutArgs getVlanOutArgs;

    if (enable)
    {
        inArgs.disallowIPFrag           = false;
        inArgs.forceUntaggedEgressMask  = 0U;
        inArgs.limitIPNxtHdr            = false;
        inArgs.noLearnMask              = 0U;
        inArgs.regMcastFloodMask        = (CPSW_ALE_HOST_PORT_MASK |
                                          CPSW_ALE_MACPORT_TO_PORTMASK(ENET_MAC_PORT_1));
        inArgs.unregMcastFloodMask      = 0U;
        inArgs.vidIngressCheck          = false;
        inArgs.vlanMemberList           = (CPSW_ALE_HOST_PORT_MASK |
                                          CPSW_ALE_MACPORT_TO_PORTMASK(ENET_MAC_PORT_1));
        inArgs.vlanIdInfo.vlanId        = vlanId;
        inArgs.vlanIdInfo.tagType       = ENET_VLAN_TAG_TYPE_INNER;
        ENET_IOCTL_SET_INOUT_ARGS(&prms, &inArgs, &aleEntryIdx);

        status = Enet_ioctl(gEnetIfObj.hEnet,
                            gEnetIfObj.coreId,
                            CPSW_ALE_IOCTL_ADD_VLAN,
                            &prms);
        if (status != ENET_SOK)
        {
            getVlanInArgs.vlanId        = vlanId;
            getVlanInArgs.tagType       = ENET_VLAN_TAG_TYPE_INNER;
            ENET_IOCTL_SET_INOUT_ARGS(&prms, &getVlanInArgs, &getVlanOutArgs);

            status = Enet_ioctl(gEnetIfObj.hEnet,
                                gEnetIfObj.coreId,
                                CPSW_ALE_IOCTL_LOOKUP_VLAN,
                                &prms);
            if (status != ENET_SOK)
            {
                EnetIf_print("%s:%d Error: Failed to add VLAN : %d",
                    __FUNCTION__, __LINE__, status);
            }
            else
            {
                EnetIf_print("%s:%d VLAN %d already exists- Ale index is: %d",
                    __FUNCTION__, __LINE__, vlanId, getVlanOutArgs.aleEntryIdx);
            }
        }
        else
        {
            EnetIf_print("%s:%d VLAN %d added- Ale index is: %d",
                    __FUNCTION__, __LINE__, vlanId, getVlanOutArgs.aleEntryIdx);
        }
    }
    else
    {
        getVlanInArgs.vlanId        = vlanId;
        getVlanInArgs.tagType       = ENET_VLAN_TAG_TYPE_INNER;
        ENET_IOCTL_SET_INOUT_ARGS(&prms, &getVlanInArgs, &getVlanOutArgs);

        status = Enet_ioctl(gEnetIfObj.hEnet,
                            gEnetIfObj.coreId,
                            CPSW_ALE_IOCTL_LOOKUP_VLAN,
                            &prms);
        if (status != ENET_SOK)
        {
            EnetIf_print("%s:%d Error: VLAN %d not found : %d",
                    __FUNCTION__, __LINE__, vlanId, status);
        }
        else
        {
            ENET_IOCTL_SET_IN_ARGS(&prms, &getVlanInArgs);
            status = Enet_ioctl(gEnetIfObj.hEnet,
                                gEnetIfObj.coreId,
                                CPSW_ALE_IOCTL_REMOVE_VLAN,
                                &prms);
            if (status != ENET_SOK)
            {
                EnetIf_print("%s:%d Error: VLAN %d not removed : %d",
                    __FUNCTION__, __LINE__, vlanId, status);
            }
            else
            {
                EnetIf_print("%s:%d VLAN %d removed : %d",
                    __FUNCTION__, __LINE__, vlanId);
            }
        }
    }
#elif defined (CPSW9G) || defined (CPSW5G)
    if (enable)
    {
        status = CpswProxy_joinVlan(gEnetIfObj.hCpswProxy,
                           gEnetIfObj.rxStartFlowIdx,
                           gEnetIfObj.rxFlowIdx,
                           gEnetIfObj.hostMacAddr,
                           vlanId);
        if (status != ENET_SOK)
        {
            EnetIf_print("%s:%d Error: VLAN %d not removed : %d",
                    __FUNCTION__, __LINE__, vlanId, status);
        }
        else
        {
            EnetIf_print("%s:%d VLAN %d removed : %d",
                    __FUNCTION__, __LINE__, vlanId);
        }
    }
    else
    {
        status = CpswProxy_leaveVlan(gEnetIfObj.hCpswProxy,
                           gEnetIfObj.rxStartFlowIdx,
                           gEnetIfObj.rxFlowIdx,
                           gEnetIfObj.hostMacAddr,
                           vlanId);
        if (status != ENET_SOK)
        {
            EnetIf_print("%s:%d Error: VLAN %d not removed : %d",
                    __FUNCTION__, __LINE__, vlanId, status);
        }
        else
        {
            EnetIf_print("%s:%d VLAN %d removed : %d",
                    __FUNCTION__, __LINE__, vlanId);
        }
    }
#endif
}

void EnetIf_AleFloodUnregMcast(uint16_t vlanId, int flood)
{
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    CpswAle_RxFilter setRxFilter;
    Enet_IoctlPrms prms;
    int32_t status;

    if (flood == 0)
    {
        setRxFilter = CPSW_ALE_RXFILTER_MCAST;
        ENET_IOCTL_SET_IN_ARGS(&prms, &setRxFilter);
        status = Enet_ioctl(gEnetIfObj.hEnet,
                            gEnetIfObj.coreId,
                            CPSW_ALE_IOCTL_SET_RX_FILTER,
                            &prms);
        if (status != ENET_SOK)
        {
            EnetIf_print("%s:%d Error: Failed to set CPSW_ALE_RXFILTER_MCAST : %d",
                __FUNCTION__, __LINE__, status);
        }
    }
    else if (flood == 1)
    {
        setRxFilter = CPSW_ALE_RXFILTER_ALLMCAST;
        ENET_IOCTL_SET_IN_ARGS(&prms, &setRxFilter);
        status = Enet_ioctl(gEnetIfObj.hEnet,
                            gEnetIfObj.coreId,
                            CPSW_ALE_IOCTL_SET_RX_FILTER,
                            &prms);
        if (status != ENET_SOK)
        {
            EnetIf_print("%s:%d Error: Failed to set CPSW_ALE_RXFILTER_ALLMCAST : %d",
                __FUNCTION__, __LINE__, status);
        }
    }
    return;
#elif defined (CPSW9G) || defined (CPSW5G)
    /* Not doing anything for cpsw9g/5g */
#endif
}

void EnetIf_AleDelVlanMcast(uint16_t vlanId, uint8_t *mcastAddr)
{
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    CpswAle_MacAddrInfo removeInArgs;
    Enet_IoctlPrms prms;
    int32_t status;

    /* Delete the mcast mac entry */
    memset(&removeInArgs, 0, sizeof(CpswAle_MacAddrInfo));
    EnetUtils_copyMacAddr(&removeInArgs.addr[0U], mcastAddr);
    removeInArgs.vlanId = vlanId;

    ENET_IOCTL_SET_IN_ARGS(&prms, &removeInArgs);

    status = Enet_ioctl(gEnetIfObj.hEnet,
                        gEnetIfObj.coreId,
                        CPSW_ALE_IOCTL_REMOVE_ADDR,
                        &prms);
    if (status != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: Failed to delete MAC address: %d",
            __FUNCTION__, __LINE__, status);
    }
#elif defined (CPSW9G) || defined (CPSW5G)
    CpswProxy_filterDelMac(gEnetIfObj.hCpswProxy,
                           mcastAddr, vlanId);
#endif
}

/*****************************************************************************/
void EnetIf_AleAddVlanMcast(uint16_t vlanId, uint8_t *mcastAddr)
{
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    CpswAle_SetMcastEntryInArgs mcastInArgs;
    Enet_IoctlPrms prms;
    uint32_t entry;
    int32_t status;

    /* Add multicast entry with port mask: host port & MAC port 1 */
    memset(&mcastInArgs, 0, sizeof(CpswAle_SetMcastEntryInArgs));
    mcastInArgs.addr.vlanId = vlanId;
    EnetUtils_copyMacAddr(&mcastInArgs.addr.addr[0], &mcastAddr[0]);
    mcastInArgs.info.super    = false;
    mcastInArgs.info.fwdState = CPSW_ALE_FWDSTLVL_FWD;
    mcastInArgs.info.portMask = (CPSW_ALE_HOST_PORT_MASK |
                                 CPSW_ALE_MACPORT_TO_PORTMASK(ENET_MAC_PORT_1));
    mcastInArgs.info.numIgnBits = 0U;

    ENET_IOCTL_SET_INOUT_ARGS(&prms, &mcastInArgs, &entry);

    status = Enet_ioctl(gEnetIfObj.hEnet,
                        gEnetIfObj.coreId,
                        CPSW_ALE_IOCTL_ADD_MCAST,
                        &prms);
    if (status != ENET_SOK)
    {
        EnetIf_print("%s:%d Error: Failed to add mcast ports : %d",
            __FUNCTION__, __LINE__, status);
    }

    return;
#elif defined (CPSW9G) || defined (CPSW5G)
    CpswProxy_filterAddMac(gEnetIfObj.hCpswProxy,
            gEnetIfObj.rxStartFlowIdx,
            gEnetIfObj.rxFlowIdx,
            mcastAddr,
            vlanId);
#endif
}

/*****************************************************************************/
void EnetIf_AleDumpTable()
{
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    int32_t status;
    Enet_IoctlPrms prms;

    EnetIf_print("-----------------------------------------");
    EnetIf_print("ALE Table");
    EnetIf_print("-----------------------------------------");

    ENET_IOCTL_SET_NO_ARGS(&prms);

    status = Enet_ioctl(gEnetIfObj.hEnet,
                        gEnetIfObj.coreId,
                        CPSW_ALE_IOCTL_DUMP_TABLE,
                        &prms);
    EnetIf_assert(status == ENET_SOK);

#elif defined (CPSW9G) || defined (CPSW5G)
    CpswProxy_dumpStats(gEnetIfObj.hCpswProxy);
#endif
}

/*****************************************************************************/
void EnetIf_AleDumpPolicer()
{
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    int32_t status;
    Enet_IoctlPrms prms;

    EnetIf_print("-----------------------------------------");
    EnetIf_print("POLICER_Entries");
    EnetIf_print("-----------------------------------------");

    ENET_IOCTL_SET_NO_ARGS(&prms);

    status = Enet_ioctl(gEnetIfObj.hEnet,
                        gEnetIfObj.coreId,
                        CPSW_ALE_IOCTL_DUMP_POLICER_ENTRIES,
                        &prms);
    EnetIf_assert(status == ENET_SOK);
#endif
}

/*****************************************************************************/
void EnetIf_ShowPhyRegs()
{
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    int32_t status;
    Enet_IoctlPrms prms;

    EnetIf_print("-----------------------------------------");
    EnetIf_print("CPSW PHY regs");
    EnetIf_print("-----------------------------------------");

    ENET_IOCTL_SET_IN_ARGS(&prms, &gEnetIfObj.macPort);

    status = Enet_ioctl(gEnetIfObj.hEnet,
                        gEnetIfObj.coreId,
                        ENET_PHY_IOCTL_PRINT_REGS,
                        &prms);
    EnetIf_assert(status == ENET_SOK);

#elif defined (CPSW9G) || defined (CPSW5G)
    /* Not supported */
#endif
}

/*****************************************************************************/
void EnetIf_EnablePromiscuousMode()
{
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    int32_t status;
    CpswAle_RxFilter setRxFilter;
    Enet_IoctlPrms prms;

    setRxFilter = CPSW_ALE_RXFILTER_ALL;
    ENET_IOCTL_SET_IN_ARGS(&prms, &setRxFilter);

    status = Enet_ioctl(gEnetIfObj.hEnet,
                        gEnetIfObj.coreId,
                        CPSW_ALE_IOCTL_SET_RX_FILTER,
                        &prms);
    EnetIf_assert(status == ENET_SOK);
#elif defined (CPSW9G) || defined (CPSW5G)
    /* Not supported */
#endif
}

/*****************************************************************************/
void EnetIf_CpswRecoveryNotificationCb(uint32_t notifyType,
                                       void *notifyArg,
                                       void *cbArg)
{
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    /* Not supported */
#elif defined (CPSW9G) || defined (CPSW5G)
    int cpswRecoveryMessage = -1;
    int ret;

    if(notifyType == ETHREMOTECFG_NOTIFY_HWERROR)
    {
        EnetIf_print("%s:%d: Recieved ETHREMOTECFG_NOTIFY_HWERROR notification", __FUNCTION__, __LINE__, strerror(errno));
        cpswRecoveryMessage = CPSW_RECOVERY_MSG_TEARDOWN;
    }
    else if(notifyType == ETHREMOTECFG_NOTIFY_HWRECOVERY_COMPLETE)
    {
        EnetIf_print("%s:%d: Recieved ETHREMOTECFG_NOTIFY_HWRECOVERY_COMPLETE notification", __FUNCTION__, __LINE__, strerror(errno));
        cpswRecoveryMessage = CPSW_RECOVERY_MSG_RECOVERY_COMPLETE;
    }

    if(cpswRecoveryMessage != -1)
    {
        EnetIf_assert(g_cpsw_recovery_connection_id != -1);
        ret = MsgSendPulse(g_cpsw_recovery_connection_id, -1, cpswRecoveryMessage, -1);
        if(ret == -1)
        {
            EnetIf_print("%s:%d: Error sending message pulse to cpsw_recovery_thread: %s", __FUNCTION__, __LINE__, strerror(errno));
        }
    }
    else
    {
        EnetIf_print("%s:%d: Error: Unsupported cpsw notification recieved! notifyType=%d", __FUNCTION__,__LINE__, notifyType);
    }
#endif
}

/*****************************************************************************/
void EnetIf_RegisterForCPSWRecoveryNotifications()
{
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    /* Not supported */
#elif defined (CPSW9G) || defined (CPSW5G)
    int32_t ret = ENET_SOK;

    CpswProxy_NotifyCbFxn hwErrorCallback = &EnetIf_CpswRecoveryNotificationCb;
    ret = CpswProxy_registerNotifyCb(gEnetIfObj.hCpswProxy, ETHREMOTECFG_NOTIFY_HWERROR, hwErrorCallback, NULL);

    if(ret != ENET_SOK)
    {
        EnetIf_print("%s: Registration of ETHREMOTECFG_NOTIFY_HWERROR notification failed! ret=%d\n", __FUNCTION__, ret);
    } else {
        EnetIf_print("%s: Registration of ETHREMOTECFG_NOTIFY_HWERROR notification succeded!\n", __FUNCTION__);
    }

    CpswProxy_NotifyCbFxn hwRecoveryCallback = &EnetIf_CpswRecoveryNotificationCb;
    ret = CpswProxy_registerNotifyCb(gEnetIfObj.hCpswProxy, ETHREMOTECFG_NOTIFY_HWRECOVERY_COMPLETE, hwRecoveryCallback, NULL);
    if(ret != ENET_SOK)
    {
        EnetIf_print("%s: Registration of ETHREMOTECFG_NOTIFY_HWRECOVERY_COMPLETE notification failed! ret=%d\n", __FUNCTION__, ret);
    } else {
        EnetIf_print("%s: Registration of ETHREMOTECFG_NOTIFY_HWRECOVERY_COMPLETE notification succeded!\n", __FUNCTION__);
    }
#endif
}

#if defined (CPSW9G) || defined (CPSW5G)
int32_t EnetIf_sendTeardownCompletion()
{
    return CpswProxy_teardownCompletion(gEnetIfObj.hCpswProxy);
}
#endif


/* end of file */
