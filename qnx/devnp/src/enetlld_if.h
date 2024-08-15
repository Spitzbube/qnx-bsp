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
 *  \file enetlld_if.h
 *
 *  \brief Enet LLD interface private header file.
 */

#ifndef ENETLLD_IF_H_
#define ENETLLD_IF_H_

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
#include <ti/drv/enet/include/phy/dp83867.h>

#include "enetlld_if_utils.h"
#include "enetlld_if_memutils.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */
#ifndef IPC_MPU1_0
#define IPC_MPU1_0                       0
#endif

#ifndef ETH_MAC_ADDR_LEN
/** \brief MAC address length in bytes */
#define ETH_MAC_ADDR_LEN                 (6U)
#endif

#define CPSW_ISR_EXIT_PULSE      _PULSE_CODE_MINAVAIL
#define CPSW_ISR_PULSE           (CPSW_ISR_EXIT_PULSE + 1)

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

void EnetIf_InitObj();
void EnetIf_SetPerferredDmaChNum(uint32_t preferred_udma_channel);
void EnetIf_EnableSmmu(uint32_t virt_id);
void EnetIf_EnableCacheOps();
void EnetIf_SetDescriptorCount(uint32_t tx_descriptors_count,
                               uint32_t rx_descriptors_count);
int32_t EnetIf_RemoteAttach(uint8_t macAddr[ENET_MAC_ADDR_LEN]);
void EnetIf_RemoteDetach(void);
int32_t EnetIf_GetMacAddr(uint8_t macAddr[][ENET_MAC_ADDR_LEN]);
int32_t EnetIf_SetMacAddr(uint8_t macAddr[ENET_MAC_ADDR_LEN]);

int32_t EnetIf_PtpInit();
void EnetIf_EventNotifyCallback(void *hEventNotifyCallbackArg,
                                 CpswCpts_Event *eventInfo);
int32_t EnetIf_GetTime(uint64_t *ts);
int32_t EnetIf_SetTime(uint64_t ts);
int32_t EnetIf_GetTimestamp(uint8_t msgType,
                             uint16_t sequenceId,
                             uint8_t tx,
                             uint64_t *ts);
int32_t EnetIf_SetCompensation(int32_t nudge);

int32_t EnetIf_Init(int mac_to_mac, int speed, uint8_t *currentMacAddr);
int32_t EnetIf_InitPhy();
int32_t EnetIf_CheckPortLinkUp(uint32_t timeout_in_sec);
int32_t EnetIf_PortLinkUpCfg(int32_t *speed,
                             int32_t *duplexity);
int32_t EnetIf_Close();

uintptr_t EnetIf_disableAllIntr(void);
void EnetIf_restoreAllIntr(uintptr_t key);
void *EnetIf_registerIntr(EnetOsal_Isr isrFxn,
                          uint32_t coreIntrNum,
                          uint32_t intrPriority,
                          uint32_t triggerType,
                          void *arg);
void *EnetIf_registerIntr2(EnetOsal_Isr isrFxn,
                          uint32_t coreIntrNum,
                          uint32_t intrPriority,
                          void *arg);
void EnetIf_unRegisterIntr(void *hwiHandle);
int32_t EnetIf_openCpsw(int mac_to_mac, int speed, uint8_t *currentMacAddr);
void EnetIf_closeCpsw(void);
void EnetIf_showStats(void);
void EnetIf_rxIsrFxn(void *appData);
void EnetIf_txIsrFxn(void *appData);
int32_t EnetIf_initTxFreePktQ(void);
int32_t EnetIf_initRxReadyPktQ();
int32_t EnetIf_openDma(void);
void EnetIf_closeDma(void);
int32_t EnetIf_setAleBcastEntry();
int32_t EnetIf_changeHostAleEntry(uint8_t macAddr[]);
uint32_t EnetIf_retrieveFreeTxPkts(void);
uint32_t EnetIf_receivePkts(void);

void EnetIf_Tick();
void EnetIf_GetStats(nic_stats_t *stats);
int32_t EnetIf_SendPkt(struct mbuf * m);
int32_t EnetIf_GetRx(void *arg);
uint32_t EnetIf_GetTxFreeQCnt(void);
#if defined (CPSW9G) || defined (CPSW5G)
void EnetIf_RegisterIPv4Address(uint8_t *ipv4Addr);
int32_t EnetIf_UnregisterIPv4Address(uint8_t *ipv4Addr);
void EnetIf_enableSyncTimer(void);
void EnetIf_disableSyncTimer(void);
void EnetIf_RegisterForCPSWRecoveryNotifications(void);
int32_t EnetIf_sendTeardownCompletion(void);
#endif
void EnetIf_AddVlan(uint16_t vlanId, uint16_t enable);
void EnetIf_AleFloodUnregMcast(uint16_t vlanId, int flood);
void EnetIf_AleDelVlanMcast(uint16_t vlanId, uint8_t *addr);
void EnetIf_AleAddVlanMcast(uint16_t vlanId, uint8_t *addr);
void EnetIf_AleDumpTable();
void EnetIf_AleDumpPolicer();
void EnetIf_ShowPhyRegs();
void EnetIf_RxIntr();
void EnetIf_EnablePromiscuousMode();

/* ========================================================================== */
/*                       Static Function Definitions                          */
/* ========================================================================== */

/* None */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef ENETLLD_IF_H_ */
