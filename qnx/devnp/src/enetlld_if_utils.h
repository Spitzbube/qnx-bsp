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
 *  \file enetlld_if_utils.h
 *
 *  \brief Common ENET LLD utility header file.
 */

#ifndef ENETLLD_IF_UTILS_H_
#define ENETLLD_IF_UTILS_H_

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
#include <sys/neutrino.h>
#include <hw/nicinfo.h>

#include <ti/osal/osal.h>
#include <ti/drv/udma/udma.h>

#include <ti/drv/enet/enet.h>
#include <ti/drv/enet/include/dma/udma/enet_udma.h>
#include <ti/drv/enet/include/core/enet_dma.h>
#include <ti/drv/enet/include/core/enet_rm.h>
#include <ti/drv/enet/include/phy/dp83867.h>
#include <ti/drv/enet/include/mod/cpsw_stats.h>
#include <ti/drv/enet/include/per/cpsw.h>



#if defined (CPSW9G) || defined (CPSW5G)
#include <ethremotecfg/client/include/cpsw_proxy.h>
#include <utils/ethfw_common/include/ethfw_trace.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */
void EnetIf_print(const char *pcString, ...);

extern void error_log(const char *str, const char *fileName, int32_t lineNum);

static inline void EnetIf_assertLocal(bool condition,
                                            const char *str,
                                            const char *fileName,
                                            int32_t lineNum)
{
    if (!(condition))
    {
        error_log(str, fileName, lineNum);
    }

    return;
}


#define EnetIf_assert(cond)                                     \
    (EnetIf_assertLocal((bool) (cond), (const char *) # cond, \
                    (const char *) __FILE__, (int32_t) __LINE__))

#ifndef htons
/** \brief Host to network byte order conversion for short integer */
#define htons(a)                        ((((a) & 0x00FFU) << 8) | \
                                         (((a) & 0xFF00U) >> 8))
#endif

#ifndef ntohs
/** \brief Network to host byte order conversion for long integer */
#define ntohs(a)                        htons(a)
#endif

/** \brief VLAN tag's Tag Protocol Identifier (TPID) */
#define ETHERTYPE_VLAN_TAG              (0x8100U)

/** \brief MAC address length in bytes */
#define ETH_MAC_ADDR_LEN                 (6U)

/** \brief Total bytes in header */
#define ETH_HDR_LEN                     (14U)

/** \brief Max octets in payload */
#define ETH_PAYLOAD_LEN                 (1500U)

/** \brief VLAN tag length in bytes */
#define ETH_VLAN_TAG_LEN                (4U)

/** \brief Test frame's header length in bytes */
#define ETH_TEST_DATA_HDR_LEN           (4U)

/* \brief Octets of payload per console row */
#define OCTETS_PER_ROW                  (16U)
/** \brief Max frame length */
#define ETH_MAX_FRAME_LEN               (1522U)

#define RGMII_ID_DISABLE_MASK           (0x10)

#define MAX_ETH_PACKET_SIZE             (1536U)

/* \brief Support macros for MMR lock/unlock functions */
#define MMR_KICK0_UNLOCK_VAL            (0x68EF3490U)
#define MMR_KICK1_UNLOCK_VAL            (0xD172BC5AU)
#define MMR_KICK_LOCK_VAL               (0x00000000U)

#define CSL_MMR_KICK_UNLOCKED_MASK      (0x00000001U)
#define CSL_MMR_KICK_UNLOCKED_SHIFT     (0x0U)

#define ENETIF_MAX_INTR_COUNT           (5)

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */
typedef enum
{
    GMII = 0,
    RMII,
    RGMII,
    SGMII,
    QSGMII,
    XFI,
    QSGMII_SUB
}emac_mode;

/**
 *  \brief  CPTS Clock Select Mux enum
 */
typedef enum
{
    ENETIF_CPTS_CLKSEL_MAIN_PLL3_HSDIV1_CLKOUT = 0x0U,
    /**< Main PLL3 HSDIV1 Clockout for CPTS*/
    ENETIF_CPTS_CLKSEL_MAIN_PLL0_HSDIV6_CLKOUT = 0x1U,
    /**< Main PLL0 HSDIV6 Clockout for CPTS*/
    ENETIF_CPTS_CLKSEL_MCU_CPTS_REF_CLK        = 0x2U,
    /**< MCU CPTS Reference Clockout*/
    ENETIF_CPTS_CLKSEL_CPTS_RFT_CLK            = 0x3U,
    /**< CPTS RFT Clock */
    ENETIF_CPTS_CLKSEL_MCU_EXT_REFCLK0         = 0x4U,
    /**< MCU External Reference Clock0*/
    ENETIF_CPTS_CLKSEL_EXT_REFCLK1             = 0x5U,
    /**< External Reference Clock 1 for CPTS*/
    ENETIF_CPTS_CLKSEL_SERDES0_IP2_LN0_TXMCLK  = 0x6U,
    /**< SERDES0 IP2 Lane0 Tx MCLK*/
    ENETIF_CPTS_CLKSEL_SERDES0_IP2_LN1_TXMCLK  = 0x7U,
    /**< SERDES0 IP2 Lane1 Tx MCLK*/
    ENETIF_CPTS_CLKSEL_SERDES1_IP2_LN0_TXMCLK  = 0x8U,
    /**< SERDES1 IP2 Lane0 Tx MCLK*/
    ENETIF_CPTS_CLKSEL_SERDES1_IP2_LN1_TXMCLK  = 0x9U,
    /**< SERDES1 IP2 Lane1 Tx MCLK*/
    ENETIF_CPTS_CLKSEL_SERDES2_IP2_LN0_TXMCLK  = 0xAU,
    /**< SERDES2 IP2 Lane0 Tx MCLK*/
    ENETIF_CPTS_CLKSEL_SERDES2_IP2_LN1_TXMCLK  = 0xBU,
    /**< SERDES2 IP2 Lane1 Tx MCLK*/
    ENETIF_CPTS_CLKSEL_SERDES3_IP2_LN0_TXMCLK  = 0xCU,
    /**< SERDES3 IP2 Lane0 Tx MCLK*/
    ENETIF_CPTS_CLKSEL_SERDES3_IP2_LN1_TXMCLK  = 0xDU,
    /**< SERDES3 IP2 Lane1 Tx MCLK*/
    ENETIF_CPTS_CLKSEL_MCU_PLL2_HSDIV1_CLKOUT  = 0xEU,
    /**< MCU PLL2 HSDIV1 Clockout*/
    ENETIF_CPTS_CLKSEL_MAIN_SYSCLK0            = 0xFU,
    /**< Main System Clock*/
} EnetIf_CptsClkSelMux;

/**
 *  \brief  Lock/unlock enum
 */
typedef enum
{
    ENETIF_LOCK_MMR = 0U,
    /**< lock control MMR */
    ENETIF_UNLOCK_MMR,
    /**< unlock control MMR */
} EnetIf_MmrLockState;

/**
 *  \brief CTRL MMR enum for MCU and Main domain
 */
typedef enum
{
    ENETIF_MMR_LOCK0 = 0U,
    /**< control MMR lock 0*/
    ENETIF_MMR_LOCK1,
    /**< control MMR lock 1*/
    ENETIF_MMR_LOCK2,
    /**< control MMR lock 2*/
    ENETIF_MMR_LOCK3,
    /**< control MMR lock 3*/
    ENETIF_MMR_LOCK4,
    /**< control MMR lock 4*/
    ENETIF_MMR_LOCK5,
    /**< control MMR lock 5*/
    ENETIF_MMR_LOCK6,
    /**< control MMR lock 6*/
    ENETIF_MMR_LOCK7,
    /**< control MMR lock 7*/
} EnetIf_CtrlMmrType;

/**
 *  \brief  Clkout frequency select enum
 */
typedef enum
{
    ENETIF_CLKOUT_FREQ_50MHZ = 0U,
    /**< Select 50MHz output on clkout pin */
    ENETIF_CLKOUT_FREQ_25MHZ,
    /**< Select 25MHz output on clkout pin */
} EnetIf_ClkOutFreqType;

typedef struct
{
    uint8_t dstMac[ETH_MAC_ADDR_LEN];
    uint8_t srcMac[ETH_MAC_ADDR_LEN];
    uint16_t etherType;
} __attribute__ ((packed)) EthFrameHeader;

typedef struct
{
    EthFrameHeader hdr;
    uint8_t payload[ETH_PAYLOAD_LEN + ETH_VLAN_TAG_LEN];
} __attribute__ ((packed)) EthFrame;

typedef struct
{
    uint8_t dstMac[ETH_MAC_ADDR_LEN];
    uint8_t srcMac[ETH_MAC_ADDR_LEN];
    uint16_t tpid;
    uint16_t tci;
    uint16_t etherType;
} __attribute__ ((packed)) EthVlanFrameHeader;

typedef struct
{
    EthVlanFrameHeader hdr;
    uint8_t payload[ETH_PAYLOAD_LEN];
} __attribute__ ((packed)) EthVlanFrame;

#if defined (CPSW9G) || defined (CPSW5G)
typedef struct EnetIf_SyncTimerObj_s
{
    uint32_t *GTC0_GTC_CFG0_Base;
    uint32_t *GTC0_GTC_CFG1_Base;
    uint32_t *GTC0_GTC_CFG2_Base;
    uint32_t *GTC0_GTC_CFG3_Base;
    uint64_t currLocalTime;
    uint64_t prevLocalTime;
    uint64_t currCptsTime;
    uint64_t prevCptsTime;
    double rate;
    double offset;
} EnetIf_SyncTimerObj;
#endif

typedef struct
{
    uint32_t evtId;
    int chid;
    struct sigevent isr_event;
    uint32_t coreIntrNum;
    uint32_t intrPriority;
    EnetOsal_Isr isrFxn;
    uintptr_t arg;
    int      isrThreadRunning;
} EnetIf_hwi_info;

typedef struct
{
    /* Enet driver */
    Enet_Handle          hEnet;
    Enet_Type            enetType;
    uint32_t             instId;
    uint32_t             coreId;
    uint32_t             coreKey;
    uint32_t             boardId;
    Enet_MacPort         macPort;
    emac_mode            macMode;  /* MAC mode (defined in board library) */
    uint32_t             preferredUdmaChannel;
    uint32_t             numTxPkts;
    uint32_t             numRxPkts;
    pthread_mutex_t      cpswMutex;
    intrspin_t           spinlockDma;
    uint32_t             ptpEnabled;

    /* UDMA driver handle */
    Udma_DrvHandle       hUdmaDrv;
    EnetDma_RxChHandle   hRxFlow;
    EnetDma_PktQ         rxReadyQ;
    EnetDma_TxChHandle   hTxCh;
    EnetDma_PktQ         freePktInfoQ;
    uint32_t             rxFlowIdx;
    uint32_t             rxStartFlowIdx;
    uint32_t             txChNum;
    uint8_t              hostMacAddr[ETH_MAC_ADDR_LEN];

    EnetIf_hwi_info      hwi[ENETIF_MAX_INTR_COUNT];
    EnetIf_hwi_info      *hwiRx;
    EnetIf_hwi_info      *hwiTx;
    uint8_t              currIntrCount;

#if defined (CPSW9G) || defined (CPSW5G)
    /* CPSW Proxy Driver handle */
    CpswProxy_Handle     hCpswProxy;
    /* MTU of rx packet */
    uint32_t             rxMtu;
    /* Max Tx packet size per priority */
    uint32_t             txMtu[ENET_PRI_NUM];
    /* Number of TX channels available to this client */
    uint32_t             numTxCh;
    /* Number of RX flows available to this client */
    uint32_t             numRxFlow;
    /* Enabled features flag repored by ETHFW */
    uint32_t             features;
    /*! DMA handle */
    EnetDma_Handle       dma;
    /*! DMA Config */
    EnetDma_initCfg      dmaDataPathConfig;
    /*! Sync Timer Object */
    EnetIf_SyncTimerObj  syncTimerObj;
    uint8_t              ipv4Addr[ENET_IPv4_ADDR_LEN];
    Enet_MacPort         *macPorts;
    uint32_t             numMacPorts;
    uint32_t             allocMacFlag;
#endif

} EnetIf_Obj;


/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */
#define EnetIf_wait               delay

/* enetlld_if_rmcfg.c */
const EnetRm_ResPrms *EnetIfRm_getResPartInfo(Enet_Type enetType,
                                              uint32_t instId);
const EnetRm_IoctlPermissionTable *EnetIfRm_getIoctlPermissionInfo(Enet_Type enetType,
                                                                   uint32_t instId);

/* enetlld_if_utils.c */
uint32_t EnetIf_getNavSSInstanceId(Enet_Type enetType);
Udma_DrvHandle EnetIf_udmaOpen(Enet_Type enetType,
                                Udma_InitPrms *pInitPrms);
void EnetIf_udmaclose(Udma_DrvHandle hUdmaDrv);
void EnetIf_printMacAddr(uint8_t macAddr[]);
void EnetIf_printFrame(EthFrame *frame,
                        uint32_t len);
void EnetIf_printHostPortStats2G(CpswStats_HostPort_2g *st);
void EnetIf_printMacPortStats2G(CpswStats_MacPort_2g *st);
void EnetIf_validatePacketState(EnetDma_PktQ *pQueue,
                                 uint32_t expectedState,
                                 uint32_t newState);
uint64_t EnetIf_virtToPhyFxn(const void *virtAddr,
                              void *appData);
void *EnetIf_phyToVirtFxn(uint64_t phyAddr,
                           void *appData);
uint64_t EnetIf_udmaVirtToPhyFxn(const void *virtAddr,
                                  uint32_t chNum,
                                  void *appData);
EnetIf_MmrLockState EnetIf_mcuMmrCtrl(EnetIf_CtrlMmrType mmrNum,
                                     EnetIf_MmrLockState lock);
EnetIf_MmrLockState EnetIf_mainMmrCtrl(EnetIf_CtrlMmrType mmrNum,
                                      EnetIf_MmrLockState lock);
void EnetIf_selectCptsClock(Enet_Type enetType,
                             EnetIf_CptsClkSelMux clkSelMux);
void *EnetIf_udmaPhyToVirtFxn(uint64_t phyAddr,
                               uint32_t chNum,
                               void *appData);
int32_t EnetIf_allocRxFlow(Enet_Handle hEnet,
                            uint32_t coreKey,
                            uint32_t coreId,
                            uint32_t *rxFlowStartIdx,
                            uint32_t *flowIdx);
int32_t EnetIf_allocMac(Enet_Handle hEnet,
                         uint32_t coreKey,
                         uint32_t coreId,
                         uint8_t *macAddress);
int32_t EnetIf_freeRxFlow(Enet_Handle hEnet,
                           uint32_t coreKey,
                           uint32_t coreId,
                           uint32_t rxFlowIdx);
int32_t EnetIf_freeTxCh(Enet_Handle hEnet,
                         uint32_t coreKey,
                         uint32_t coreId,
                         uint32_t txChNum);
int32_t EnetIf_freeMac(Enet_Handle hEnet,
                        uint32_t coreKey,
                        uint32_t coreId,
                        uint8_t *macAddress);
int32_t EnetIf_registerDefaultRxFlow(Enet_Handle hEnet,
                                      uint32_t coreKey,
                                      uint32_t coreId,
                                      uint32_t rxFlowStartIdx,
                                      uint32_t rxFlowIdx);
int32_t EnetIf_unregisterDefaultRxFlow(Enet_Handle hEnet,
                                        uint32_t coreKey,
                                        uint32_t coreId,
                                        uint32_t rxFlowStartIdx,
                                        uint32_t rxFlowIdx);
int32_t EnetIf_registerDstMacRxFlow(Enet_Handle hEnet,
                                     uint32_t coreKey,
                                     uint32_t coreId,
                                     uint32_t rxFlowStartIdx,
                                     uint32_t rxFlowIdx,
                                     uint8_t macAddress[ENET_MAC_ADDR_LEN]);
int32_t EnetIf_unregisterDstMacRxFlow(Enet_Handle hEnet,
                                       uint32_t coreKey,
                                       uint32_t coreId,
                                       uint32_t rxFlowStartIdx,
                                       uint32_t rxFlowIdx,
                                       uint8_t macAddress[ENET_MAC_ADDR_LEN]);
bool EnetIf_isPortLinkUp(Enet_Handle hEnet,
                          uint32_t coreId,
                          Enet_MacPort portNum);
void EnetIf_initResourceConfig(Enet_Type enetType,
                                uint32_t instId,
                                uint32_t selfCoreId,
                                EnetRm_ResCfg *rscCfg,
                                uint8_t *currentMacAddr);
void EnetIf_setCommonRxFlowPrms(EnetUdma_OpenRxFlowPrms *pRxFlowPrms, uint32_t numRxPkts);
void EnetIf_addHostPortEntry(Enet_Handle hEnet,
                              uint32_t coreId,
                              uint8_t *macAddr);
void EnetIf_setCommonTxChPrms(EnetUdma_OpenTxChPrms *pTxChPrms, uint32_t numTxPkts);
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
void EnetIf_openTxCh(Enet_Handle hEnet,
                      uint32_t coreKey,
                      uint32_t coreId,
                      uint32_t *pTxChNum,
                      EnetDma_TxChHandle *pTxChHandle,
                      EnetUdma_OpenTxChPrms *pTxChCfg);
void EnetIf_closeTxCh(Enet_Handle hEnet,
                       uint32_t coreKey,
                       uint32_t coreId,
                       EnetDma_PktQ *pFqPktInfoQ,
                       EnetDma_PktQ *pCqPktInfoQ,
                       EnetDma_TxChHandle hTxChHandle,
                       uint32_t txChNum);
void EnetIf_openRxFlow(Enet_Handle hEnet,
                        uint32_t coreKey,
                        uint32_t coreId,
                        bool useDefaultFlow,
                        uint32_t *pRxFlowStartIdx,
                        uint32_t *pRxFlowIdx,
                        uint8_t macAddr[ENET_MAC_ADDR_LEN],
                        EnetDma_RxChHandle *pRxFlowHandle,
                        EnetUdma_OpenRxFlowPrms *pRxFlowPrms);
void EnetIf_closeRxFlow(Enet_Handle hEnet,
                         uint32_t coreKey,
                         uint32_t coreId,
                         bool useDefaultFlow,
                         EnetDma_PktQ *pFqPktInfoQ,
                         EnetDma_PktQ *pCqPktInfoQ,
                         uint32_t rxFlowStartIdx,
                         uint32_t rxFlowIdx,
                         uint8_t macAddr[ENET_MAC_ADDR_LEN],
                         EnetDma_RxChHandle hRxFlow);
#elif defined (CPSW9G) || defined (CPSW5G)
void EnetIf_openTxCh(EnetIf_Obj *obj,
                      EnetUdma_OpenTxChPrms *pTxChCfg);
void EnetIf_closeTxCh(EnetIf_Obj *obj,
                       EnetDma_PktQ *pFqPktInfoQ,
                       EnetDma_PktQ *pCqPktInfoQ);
void EnetIf_openRxFlow(EnetIf_Obj *obj,
                        bool useDefaultFlow,
                        EnetUdma_OpenRxFlowPrms *pRxFlowPrms);
void EnetIf_closeRxFlow(EnetIf_Obj *obj,
                         bool useDefaultFlow,
                         EnetDma_PktQ *pFqPktInfoQ,
                         EnetDma_PktQ *pCqPktInfoQ);
#endif
int32_t EnetIf_showRxFlowStats(EnetDma_RxChHandle hRxFlow);
int32_t EnetIf_showTxChStats(EnetDma_TxChHandle hTxCh);

int32_t EnetIfBoard_cpsw2gMacModeConfig(uint32_t portNum,
                                         uint8_t mode);
uint32_t EnetIfBoard_getPhyAddr(Enet_Type enetType,
                                 Enet_MacPort portNum);
void EnetIfBoard_setPhyConfig(Enet_Type enetType,
                               Enet_MacPort portNum,
                               EnetMacPort_Interface *interface,
                               EnetPhy_Cfg *phyCfg);
#if defined (CPSW2G_MAIN)
void EnetBoard_setupGESI();
void EnetIf_enableClocks(Enet_Type enetType,
                         uint32_t instId);
void EnetIf_disableClocks(Enet_Type enetType,
                          uint32_t instId);
#endif

/* ========================================================================== */
/*                       Static Function Definitions                          */
/* ========================================================================== */

/* None */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef ENETLLD_IF_UTILS_H_ */
