/*
 * Copyright (c) 2019 QNX Software Systems. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Modfications copyright (c) 2019-2021, Texas Instruments Incorporated
 *
 */

#ifndef _J7_CPSW_H_
#define _J7_CPSW_H_

#include <io-pkt/iopkt_driver.h>
#include <stdio.h>
#include <errno.h>
#include <atomic.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/siginfo.h>
#include <sys/syspage.h>
#include <sys/neutrino.h>
#include <sys/mbuf.h>
#include <sys/slogcodes.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <sys/io-pkt.h>
#include <sys/cache.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <hw/inout.h>
#include <netdrvr/mdi.h>
#include <netdrvr/nicsupport.h>
#include <hw/nicinfo.h>
#include <sys/device.h>
#define _STDDEF_H_INCLUDED
#include <siglock.h>
#include <dev/mii/miivar.h>
#include <sys/trace.h>
#include <device_qnx.h>
#include <net/netbyte.h>

#ifdef  __cplusplus
 extern "C" {
#endif

/******************************************************************************
 *  Defines.
 *****************************************************************************/
#define DEBUG_TRACE                     0x00000001
#define DEBUG_BUFFER                    0x00000002
#define DEBUG_START_PACKET              0x00000004
#define DEBUG_PTP                       0x00000010
#define DEBUG_PTP_APP                   0x00000020
#define DEBUG_TX_RX_ERRORS              0x00000040
#define DEBUG_MASK                      0x000000FF

#if defined (CPSW2G)
  #if defined (SOC_J721E)
    /* mcu_cpsw_nuss register address definitions */
    #define J721E_CPSW_BASE         0x46000000
    #define J721E_CPSW_SIZE         0x200000
  #elif defined (SOC_J7200)
    /* mcu_cpsw_nuss register address definitions */
    #define J7200_CPSW_BASE         0x46000000
    #define J7200_CPSW_SIZE         0x200000
  #elif defined (SOC_J721S2)
    /* mcu_cpsw_nuss register address definitions */
    #define J721S2_CPSW_BASE        0x46000000
    #define J721S2_CPSW_SIZE        0x200000
  #elif defined (SOC_J784S4)
    /* mcu_cpsw_nuss register address definitions */
    #define J784S4_CPSW_BASE        0x46000000
    #define J784S4_CPSW_SIZE        0x200000
  #else
    #error "Unsupported!"
  #endif
#elif defined (CPSW9G) || defined (CPSW5G)
  #if defined (SOC_J721E)
    /* main cpsw_nuss register address definitions */
    #define J721E_CPSW_BASE         0x0C000000
    #define J721E_CPSW_SIZE         0x200000
  #elif defined (SOC_J7200)
    /* main cpsw_nuss register address definitions */
    #define J7200_CPSW_BASE         0x0C000000
    #define J7200_CPSW_SIZE         0x200000
  #elif defined (SOC_J784S4)
    /* mcu_cpsw_nuss register address definitions */
    #define J784S4_CPSW_BASE        0x0C000000
    #define J784S4_CPSW_SIZE        0x200000
  #else
    #error "Unsupported!"
  #endif
#elif defined (CPSW2G_MAIN)
  #if defined (SOC_J721S2)
    /* main cpsw_nuss register address definitions */
    #define J721S2_CPSW_BASE        0x0C200000
    #define J721S2_CPSW_SIZE        0x200000
  #elif defined (SOC_J784S4)
    /* mcu_cpsw_nuss register address definitions */
    #define J784S4_CPSW_BASE        0x0C200000
    #define J784S4_CPSW_SIZE        0x200000
  #else
    #error "Unsupported!"
  #endif
#else
  #error "Unsupported!"
#endif

#if defined (SOC_J721E) || defined (SOC_J7200) || defined (SOC_J721S2)
  #define DEFAULT_CPU_AFFINITY  0x2
#elif defined (SOC_J784S4)
  #define DEFAULT_CPU_AFFINITY  0xFF
#endif

#define CPSW_DYNAMIC_IP_ADDRESS   1

#define CPSW_QUIESCE_PULSE      _PULSE_CODE_MINAVAIL
#define CPSW_RX_PULSE          (CPSW_QUIESCE_PULSE + 1)

#define PHY_CALLOUT_DELAY_LINK_DN 1000 // in msec
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
  #define PHY_CALLOUT_DELAY       2000 // in msec
#elif defined (CPSW9G) || defined (CPSW5G)
  #define PHY_CALLOUT_DELAY       10000 // in msec
#endif
#define LINKUP_UP_TIMEOUT       2    // in sec

#define MAX_TYPED_MEM_NAME      64

#define CPSW_MEM_TX_PKTS_MAX                (256U)
#define CPSW_MEM_RX_PKTS_MAX                (256U)
#define CPSW_MEM_TX_PKTS_DEFAULT            (128U)
#define CPSW_MEM_RX_PKTS_DEFAULT            (128U)

#define CPSW_TX_FREEQ_THRESHOLD 120

#define MAX_MULITCAST_MAC_ENTRY 64

#define RX_INTR_PRIORITY        21
#define TX_INTR_PRIORITY        21
#define CPTS_INTR_PRIORITY      22
#define DEFAULT_INTR_PRIORITY   21

#define RX_PACING_INTERVAL      1 // RX interrupt pacing internal in msec

/* Interrupt numbers */
#define ENET_LLD_INTERRUPT_NUM_STAT              888
#define ENET_LLD_INTERRUPT_NUM_CPTS              890

/* Custom ioctls */
#define DUMP_PORT_STATS         0x400
#define DUMP_PHY_REG            0x401
#define DUMP_ALE_ENTRIES        0x402 
#define DUMP_POLICER_ENTRIES    0x403

/* Enable if we want to collect tracelog */
//#define USE_TRACE_INSTRUMENTATION

#define MAC_VLAN_SUPPORTED      16

enum cpsw_recovery_message {
  CPSW_RECOVERY_MSG_QUIESCE = 0,
  CPSW_RECOVERY_MSG_TEARDOWN = 1,
  CPSW_RECOVERY_MSG_RECOVERY_COMPLETE = 2
};

struct cpsw_vlan_multicast { 
    uint32_t            vlanId;
    uint32_t            multi_mac_entries;
    uint8_t             multi_mac_entry[MAX_MULITCAST_MAC_ENTRY][ETHER_ADDR_LEN];
};



/******************************************************************************
 *  Structs.
 *****************************************************************************/

struct cpsw_dev {
    struct device       sc_dev; /* common device */
    struct ethercom     sc_ec;  /* common ethernet */
    nic_config_t        cfg;    /* nic information */
    nic_stats_t         stats;  /* stats structure */

    struct callout      phy_callout;

    /* whatever else you need follows */
    struct _iopkt_self  *sc_iopkt;
    int                 sc_iid;
    int                 sc_irq;
    int                 sc_intr_cnt;
    int                 sc_intr_spurious;
    void                *sc_sdhook;

    int                 tid;
    int                 chid;
    int                 coid;
    int                 tx_reaped;

    struct _iopkt_inter sc_inter;
    pthread_mutex_t     rx_mutex;
    struct ifqueue      rx_queue;
    int                 rx_running;
    int                 rx_thread_exit;

    int                 mac_to_mac;
    int                 speed;
    int                 ptp_enable;
    int                 promiscuous;
    int                 use_typed_mem;
    char                typed_mem[MAX_TYPED_MEM_NAME];
    int                 preferred_udma_channel;
    int                 memFd;  /* Memory file descriptor */
    int                 tx_freeq_threshold;
    int                 tx_descriptors_count;
    int                 rx_descriptors_count;
    int                 run_mask_core;
    int                 poll_ms;
    int                 cache_ops;
    int                 smmu;
    int                 virt_id;
    int                 hw_csum;
    int                 rx_intr_prio;
    int                 tx_intr_prio;
    int                 cpts_intr_prio;
    int                 no_stack_thread;
    int                 rx_pacing;    /* RX interrupt pacing */
    int                 rx_pacing_msec;
    struct callout      rx_pacing_callout;
    struct sigevent     rx_pacing_event;
    uint32_t            cpsw_connect_attempts;
    uint32_t            cpsw_connect_delay_ms;

    int                 csum_flag_rx;
    int                 csum_flag_tx;

    int                 dying;
    int                 force_link;
    int                 linkup;
    struct mii_data     bsd_mii;
    uint8_t             ipaddr[4];

    uint32_t            multi_mac_entries;
    uint8_t             multi_mac_entry[MAX_MULITCAST_MAC_ENTRY][ETHER_ADDR_LEN];
    uint16_t            *join_vlan;
    struct cpsw_vlan_multicast  vlan_multicast[MAC_VLAN_SUPPORTED];
};

typedef struct {
    struct _iopkt_self  *iopkt;
    char                *options;
    nic_config_t        cfg;

    uintptr_t           cpsw_base;

    void                *sd_hook;
} attach_args_t;

/******************************************************************************
 *  Functions.
 *****************************************************************************/

/* bsd_media.c */
void bsd_mii_initmedia(struct cpsw_dev *);
void bsd_mii_finimedia(struct cpsw_dev *);
int bsd_mii_mediachange(struct ifnet *ifp);

/* j7_cpsw.c */
int cpsw_entry(void *dll_hdl, struct _iopkt_self *iopkt, char *options);
int cpsw_init(struct ifnet *);
void cpsw_stop(struct ifnet *, int);
void cpsw_start(struct ifnet *);
void cpsw_shutdown(void *);
void cpsw_MonitorPhy (void *arg);
void cpsw_rxPacing(void *arg);
int cpsw_process_rx1(void *arg, struct nw_work_thread   *wtp);
struct mbuf * cpsw_process_rx(void *arg, struct mbuf *);
void *cpsw_rx_thread(void *arg);
void *alloc_typed_mem(size_t size, paddr64_t *paddr);
void free_typed_mem(void *addr, size_t size);
int cpsw_process_interrupt (void *, struct nw_work_thread *);
int cpsw_enable_interrupt (void *);

/* j7_cpsw.c */
int cpsw_csum_offload_setup (struct cpsw_dev *cpsw, struct mbuf *m0, int offload_flags,
                             uint32_t *csInsertPos, uint32_t *csStartPos,uint32_t *csNumBytes);

/* devctl.c */
int cpsw_ioctl(struct ifnet *, unsigned long, caddr_t);
void cpsw_add_multicast_entry(struct cpsw_dev *cpsw, uint16_t vlan, uint8_t *addr);
void cpsw_del_multicast_entry_all(struct cpsw_dev *cpsw, uint16_t vlan);

#ifdef __cplusplus
};
#endif

#endif //_J7_CPSW_H_
