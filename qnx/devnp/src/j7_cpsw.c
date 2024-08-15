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

#include <j7_cpsw.h>
#include <io-pkt/iopkt_driver.h>
#include <sys/io-pkt.h>
#include <sys/syspage.h>
#include <sys/device.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <nw_thread.h>
#include <sys/netmgr.h>
#include <net/ifdrvcom.h>
#include <sys/sockio.h>
#include <device_qnx.h>
#include <net/netbyte.h>
#include <quiesce.h>
#include <sys/cache.h>
#include <net/if_vlanvar.h>
#include <net/netbyte.h>
#include <netdrvr/avb.h>

#include "enetlld_if.h"

#define NBPFILTER 1
#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif


struct _iopkt_drvr_entry IOPKT_DRVR_ENTRY_SYM(cpsw) = IOPKT_DRVR_ENTRY_SYM_INIT(cpsw_entry);

#ifdef VARIANT_a
#include <nw_dl.h>
/* This is what gets specified in the stack's dl.c */
struct nw_dll_syms cpsw_syms[] = {
        {"iopkt_drvr_entry", &IOPKT_DRVR_ENTRY_SYM(cpsw)},
        {NULL, NULL}
};
#endif

int cpsw_attach(struct device *, struct device *, void *);
int cpsw_detach(struct device *, int);

// cpsw recovery functions and variables
#if defined(CPSW9G) || defined(CPSW5G)
static void cpsw_recovery_thread_quiesce(void *arg, int die);
static int cpsw_recovery_thread_init(void *arg);
uint32_t cpsw_recovery_teardown(struct ifnet *ifp,struct cpsw_dev *cpsw);
void *cpsw_recovery_thread(void *arg);


int g_cpswRecoveryTid = -1;
int g_cpsw_recovery_channel_id = -1;
int g_cpsw_recovery_connection_id = -1;
#endif

CFATTACH_DECL(cpsw,
    sizeof(struct cpsw_dev),
    NULL,
    cpsw_attach,
    cpsw_detach,
    NULL);

attach_args_t   attach_args;
int single      = 1; /* single instance of port*/
struct cpsw_dev *g_cpsw = NULL;
struct cache_ctrl   cachectl;

/* Driver options */
static char *cpsw_opts[] = {
    "verbose",
#define CPSWOPT_VERBOSE               0
    "mac-to-mac",
#define CPSWOPT_MACTOMAC              1
    "speed",
#define CPSWOPT_SPEED                 2
    "p0mac",
#define CPSWOPT_P0MAC                 3
    "ptp",
#define CPSWOPT_PTP                   4
    "promiscuous",
#define CPSWOPT_PROMISCUOUS           5
    "typed_mem",
#define CPSWOPT_TYPED_MEM             6
    "udma_chnum",
#define CPSWOPT_UDMA_CHNUM            7
    "tx_freeq_threshold",
#define CPSWOPT_TX_FREEQ_THRESHOLD    8
    "tx_descriptor_cnt",
#define CPSWOPT_TX_DESCRIPTORS        9
    "rx_descriptor_cnt",
#define CPSWOPT_RX_DESCRIPTORS        10
    "run_mask_cpu",
#define CPSWOPT_RUN_MASK_CPU          11
    "poll_phy_ms",
#define CPSWOPT_POLL_MS               12
    "cache_ops",
#define CPSWOPT_CACHE_OPS             13
    "smmu",
#define CPSWOPT_SMMU                  14
    "virt_id",
#define CPSWOPT_VIRT_ID               15
    "hw_csum",
#define CPSWOPT_HW_CSUM               16
    "joinvlan",
#define CPSWOPT_JOINVLAN              17
    "rx_intr_prio",
#define CPSWOPT_RX_INTR_PRIO          18
    "tx_intr_prio",
#define CPSWOPT_TX_INTR_PRIO          19
    "cpts_intr_prio",
#define CPSWOPT_CPTS_INTR_PRIO        20
    "no_stack_thread",
#define CPSWOPT_NO_STACK_THREAD       21
    "rx_pacing",
#define CPSWOPT_RX_PACING_ENABLE      22
    "rx_pacing_msec",
#define CPSWOPT_RX_PACING_MSEC        23
    "cpsw_connect_attempts",
#define CPSWOPT_CPSW_CONNECT_ATTEMPTS 24
    "cpsw_connect_delay_ms",
#define CPSWOPT_CPSW_CONNECT_DELAY_MS 25
    NULL
};

/*****************************************************************************/
/*  RX thread quiesce fuction                                                */
/*****************************************************************************/
void cpsw_rx_thread_quiesce (void *arg, int die)
{
    struct cpsw_dev     *cpsw = arg;

    MsgSendPulse(cpsw->coid, SIGEV_PULSE_PRIO_INHERIT,
        CPSW_QUIESCE_PULSE, die);

    return;
}

/*****************************************************************************/
/*  RX thread init fuction                                                   */
/*****************************************************************************/
static int cpsw_rx_thread_init (void *arg)
{
    struct cpsw_dev     *cpsw = arg;
    struct nw_work_thread   *wtp = WTP;

#if defined (CPSW2G)
    pthread_setname_np(gettid(), "cpsw2g Rx");
#elif defined (CPSW9G) || defined (CPSW5G)
  #if defined (SOC_J721E)
    pthread_setname_np(gettid(), "cpsw9g Rx");
  #elif defined (SOC_J7200)
    pthread_setname_np(gettid(), "cpsw5g Rx");
  #endif
#elif defined (CPSW2G_MAIN)
    pthread_setname_np(gettid(), "cpsw2g_main Rx");
#else
  #error "Unsupported!"
#endif

    wtp->quiesce_callout = cpsw_rx_thread_quiesce;
    wtp->quiesce_arg = cpsw;
    return EOK;
}


/*****************************************************************************/
/* Parse the options                                                         */
/*****************************************************************************/
void cpsw_parse_options(struct cpsw_dev *cpsw, const char *optstring,
                          nic_config_t *cfg)

{
    char    *value;
    int     opt;
    char    *options, *freeptr;
    int     tmp;
    char    *ptr;
    int     count;

    if (optstring == NULL) {
        return;
    }

    /* getsubopt() is destructive */
    freeptr = options = strdup (optstring);

    while (options && *options != '\0') {
    opt = getsubopt (&options, cpsw_opts, &value);

    switch (opt) {
        case CPSWOPT_VERBOSE:
            if (cpsw != NULL) {
                if (value) {
                    cpsw->cfg.verbose = strtoul(value, 0, 0);
                    slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Verbose -->%d",
                        __FUNCTION__, __LINE__, cpsw->cfg.verbose);
                } else {
                    cpsw->cfg.verbose++;
                }
            }
            break;
        case CPSWOPT_PTP:
            if (cpsw != NULL) {
                cpsw->ptp_enable = strtoul(value, 0, 0);
                slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d PTP -->%d",
                    __FUNCTION__, __LINE__, cpsw->ptp_enable);
            }
            break;
        case CPSWOPT_MACTOMAC:
            if (cpsw != NULL) {
                tmp = strtoul(value, 0, 0);
                if ((tmp != 0) && (tmp != 1)) {
                    slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                      "%s:%d Error: Invalid mac-to-mac value %s", __FUNCTION__, __LINE__, value);
                } else {
                    cpsw->mac_to_mac = tmp;
                    slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d mac_to_mac -->%d",
                        __FUNCTION__, __LINE__, cpsw->mac_to_mac);
                }
            }
            break;
        case CPSWOPT_SPEED:
            if (cpsw != NULL) {
                tmp = strtoul(value, 0, 0);
                if ((tmp != 100) && (tmp != 1000)) {
                    slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                      "%s:%d Error: Invalid Port 0 speed %s", __FUNCTION__, __LINE__, value);
                } else {
                    cpsw->speed |= tmp;
                    slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d speed -->%d",
                        __FUNCTION__, __LINE__, cpsw->speed);
                }
            }
            break;

        case CPSWOPT_P0MAC:
            if (cpsw != NULL) {
                if (nic_strtomac(value, cfg->current_address)) {
                    slogf(_SLOGC_NETWORK, _SLOG_WARNING,
                        "%s:%d Error: Invalid mac address %s", __FUNCTION__, __LINE__, value);
                }
            }
            break;
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
        case CPSWOPT_PROMISCUOUS:
            if (cpsw != NULL) {
                cpsw->promiscuous = strtoul(value, 0, 0);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d promiscuous -->%d",
                        __FUNCTION__, __LINE__, cpsw->promiscuous);
            }
            break;
#endif
        case CPSWOPT_TYPED_MEM:
            if (cpsw != NULL) {
                cpsw->use_typed_mem = 1;
                strlcpy(cpsw->typed_mem, value, sizeof(cpsw->typed_mem));
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d typed_mem -->%s",
                        __FUNCTION__, __LINE__, cpsw->typed_mem);
            }
            break;
        case CPSWOPT_UDMA_CHNUM:
            if (cpsw != NULL) {
                cpsw->preferred_udma_channel = strtoul(value, 0, 0);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d perferred udma channel -->%d",
                        __FUNCTION__, __LINE__, cpsw->preferred_udma_channel);
            }
            break;
        case CPSWOPT_TX_FREEQ_THRESHOLD:
            if (cpsw != NULL) {
                cpsw->tx_freeq_threshold = strtoul(value, 0, 0);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d tx_freeq_threshold -->%d",
                        __FUNCTION__, __LINE__, cpsw->tx_freeq_threshold);
            }
            break;
        case CPSWOPT_TX_DESCRIPTORS:
            if (cpsw != NULL) {
                cpsw->tx_descriptors_count = strtoul(value, 0, 0);
                if (cpsw->tx_descriptors_count > CPSW_MEM_TX_PKTS_MAX) {
                    cpsw->tx_descriptors_count = CPSW_MEM_TX_PKTS_MAX;
                }
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d tx_descriptors_count -->%d",
                        __FUNCTION__, __LINE__, cpsw->tx_descriptors_count);
            }
            break;
        case CPSWOPT_RX_DESCRIPTORS:
            if (cpsw != NULL) {
                cpsw->rx_descriptors_count = strtoul(value, 0, 0);
                if (cpsw->rx_descriptors_count > CPSW_MEM_RX_PKTS_MAX) {
                    cpsw->rx_descriptors_count = CPSW_MEM_RX_PKTS_MAX;
                }
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d rx_descriptors_count -->%d",
                        __FUNCTION__, __LINE__, cpsw->rx_descriptors_count);
            }
            break;
        case CPSWOPT_RUN_MASK_CPU:
            if (cpsw != NULL) {
                cpsw->run_mask_core = strtoul(value, 0, 0);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d run_mask_core -->%d",
                        __FUNCTION__, __LINE__, cpsw->run_mask_core);
                if (cpsw->run_mask_core > DEFAULT_CPU_AFFINITY) {
                    // Bad run mask, so defaulting to DEFAULT_CPU_AFFINITY
                    cpsw->run_mask_core = DEFAULT_CPU_AFFINITY;
                }
            }
            break;
        case CPSWOPT_POLL_MS:
            if (cpsw != NULL) {
                cpsw->poll_ms = strtoul(value, 0, 0);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d poll_ms -->%d",
                                       __FUNCTION__, __LINE__, cpsw->poll_ms);
            }
            break;
        case CPSWOPT_CACHE_OPS:
            if (cpsw != NULL) {
                cpsw->cache_ops = strtoul(value, 0, 0);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d cache_ops -->%d",
                        __FUNCTION__, __LINE__, cpsw->cache_ops);
            }
            break;
        case CPSWOPT_SMMU:
            if (cpsw != NULL) {
                cpsw->smmu = strtoul(value, 0, 0);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d smmu -->%d",
                        __FUNCTION__, __LINE__, cpsw->smmu);
            }
            break;
        case CPSWOPT_VIRT_ID:
            if (cpsw != NULL) {
                cpsw->virt_id = strtoul(value, 0, 0);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d virt_id -->%d",
                        __FUNCTION__, __LINE__, cpsw->virt_id);
            }
            break;
        case CPSWOPT_HW_CSUM:
            if (cpsw != NULL) {
                cpsw->hw_csum = strtoul(value, 0, 0);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d hw_csum -->%d",
                        __FUNCTION__, __LINE__, cpsw->hw_csum);
            }
            break;
        case CPSWOPT_JOINVLAN:
            if ((cpsw != NULL) && (cpsw->cfg.device_index == 0)) {
                count = 0;
                ptr = value;
                if (ptr == NULL) {
                    slogf(_SLOGC_NETWORK, _SLOG_WARNING,
                        "Missing joinvlan values");
                    break;
                }
                for (;;) {
                    ptr = strchr(ptr, ';');
                    count++;
                    if (ptr == NULL) {
                    break;
                    } else {
                    ptr++;
                    }
                }
                cpsw->join_vlan = calloc(count + 1, sizeof(*cpsw->join_vlan));
                if (cpsw->join_vlan) {
                    ptr = strtok(value, ";");
                    count = 0;
                    while (ptr != NULL) {
                        tmp = strtoul(ptr, 0, 0);
                        if ((tmp > 0) && (tmp < 4096)) {
                            cpsw->join_vlan[count] = tmp;
                            count++;
                        } else {
                            slogf(_SLOGC_NETWORK, _SLOG_WARNING,
                                "Ignoring invalid join vlan %s", ptr);
                        }
                        ptr = strtok(NULL, ";");
                    }
                }
            }
            break;
        case CPSWOPT_RX_INTR_PRIO:
            if (cpsw != NULL) {
                cpsw->rx_intr_prio = strtoul(value, 0, 0);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d rx_intr_prio -->%d",
                        __FUNCTION__, __LINE__, cpsw->rx_intr_prio);
            }
            break;
        case CPSWOPT_TX_INTR_PRIO:
            if (cpsw != NULL) {
                cpsw->tx_intr_prio = strtoul(value, 0, 0);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d tx_intr_prio -->%d",
                        __FUNCTION__, __LINE__, cpsw->tx_intr_prio);
            }
            break;
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
        case CPSWOPT_CPTS_INTR_PRIO:
            if (cpsw != NULL) {
                cpsw->cpts_intr_prio = strtoul(value, 0, 0);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d cpts_intr_prio -->%d",
                        __FUNCTION__, __LINE__, cpsw->cpts_intr_prio);
            }
            break;
#endif
        case CPSWOPT_NO_STACK_THREAD:
            if (cpsw != NULL) {
                cpsw->no_stack_thread = strtoul(value, 0, 0);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d no_stack_thread -->%d",
                        __FUNCTION__, __LINE__, cpsw->no_stack_thread);
            }
            break;
        case CPSWOPT_RX_PACING_ENABLE:
            if (cpsw != NULL) {
                cpsw->rx_pacing = strtoul(value, 0, 0);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d rx_pacing -->%d",
                                       __FUNCTION__, __LINE__, cpsw->rx_pacing);
            }
            break;
        case CPSWOPT_RX_PACING_MSEC:
            if (cpsw != NULL) {
                cpsw->rx_pacing_msec = strtoul(value, 0, 0);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d rx_pacing_msec -->%d",
                                       __FUNCTION__, __LINE__, cpsw->rx_pacing_msec);
            }
            break;
        case CPSWOPT_CPSW_CONNECT_ATTEMPTS:
            if (cpsw != NULL) {
                cpsw->cpsw_connect_attempts = (uint32_t)strtoul(value, 0, 0);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d CpswProxy_connect attempts --> %d",
                                       __FUNCTION__, __LINE__, cpsw->cpsw_connect_attempts);
            }
            break;
        case CPSWOPT_CPSW_CONNECT_DELAY_MS:
            if (cpsw != NULL) {
                cpsw->cpsw_connect_delay_ms = (uint32_t)strtoul(value, 0, 0);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d CpswProxy_connect attempt delay --> %d ms",
                                       __FUNCTION__, __LINE__, cpsw->cpsw_connect_delay_ms);
            }
            break;
        default:
            slogf(_SLOGC_NETWORK, _SLOG_WARNING,
              "%s:%d Error: Skipping unknown option %s", __FUNCTION__, __LINE__, value);
            break;
        }
    }

    (free)(freeptr);
    return;
}

/*****************************************************************************/
/* alloc_typed_mem                                                           */
/*****************************************************************************/
void *alloc_typed_mem(size_t size, paddr64_t *paddr)
{
    off64_t offset;
    size_t contig_len;
    uint64_t physAddr = 0;

    void *buf = MAP_FAILED;
    int fd    = -1;
    int flags = 0;
    int prot  = PROT_READ | PROT_WRITE;

    if (!g_cpsw->use_typed_mem) {
        return MAP_FAILED;
    }

    /* The caller of the API, may or may not have provided a physical address to be mapped */
    if(paddr != NULL)
    {
        physAddr = *paddr;
    }

    if (g_cpsw->cache_ops) {
        slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d Adding PROT_NOCACHE for typed alloc",
            __FUNCTION__,__LINE__);
        prot |= PROT_NOCACHE;
    }

    /*
     * User did not specify a physical address, then use the memory pool.
     * This is the expected usage.
     */
    if(physAddr == 0)
    {
        fd = g_cpsw->memFd;
        flags = MAP_SHARED;
    }
    /*
     * User did specify a src/dst physical address so create a memory
     * mapping directly to that address.
     */
    else
    {
        fd = NOFD;
        flags = MAP_PHYS | MAP_PRIVATE;
    }

    buf = mmap64(0, size, prot, flags, fd, physAddr);
    if(buf != MAP_FAILED)
    {
        /*
            * If physical address was not provided, figure out which
            * address was mapped.
            */
        if(physAddr == 0)
        {
            int tmp_fd = -1;
            if (posix_mem_offset64(buf, size, &offset, &contig_len, &tmp_fd) != 0)
            {

                slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Error: Could not obtain buffer physical address",
                    __FUNCTION__, __LINE__);
                munmap(buf, size);
                return MAP_FAILED;
            }
            if (paddr != NULL)
            {
                *paddr = (uint32_t) offset;
            }
            physAddr = offset;
        }

        slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d Alloc successful; Virt: 0x%p, Phy: 0x%lx Contig_len: %ld",
            __FUNCTION__,__LINE__, buf, physAddr, contig_len);
    }
    else
    {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d  Alloc failed; Virt: 0x%p, Phys: 0x%lx\n",
            __FUNCTION__, __LINE__,  buf, offset);
    }
    if(paddr != NULL)
    {
        *paddr = physAddr;
    }

    return buf;
}

/*****************************************************************************/
/* free_typed_mem                                                           */
/*****************************************************************************/
void free_typed_mem(void *addr, size_t size)
{
    munmap(addr, size);
}

/*****************************************************************************/
/*  Initial driver entry point.                                              */
/*****************************************************************************/
int
cpsw_entry(void *dll_hdl,  struct _iopkt_self *iopkt, char *options)
{
    int     instance;
    struct device   *dev;
    struct drvcom_config    *dcon;
    struct ifnet    *ifp;
    int err;

    slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d Entry -->", __FUNCTION__, __LINE__);

    /* Check if it is already mounted by doing a "nicinfo" on each interface */
    dcon = (malloc)(sizeof(*dcon));
    if (dcon == NULL) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR,
              "%s:%d Error: malloc failed", __FUNCTION__, __LINE__);
        return ENOMEM;
    }
    IFNET_FOREACH(ifp) {
        dcon->dcom_cmd.ifdc_cmd = DRVCOM_CONFIG;
        dcon->dcom_cmd.ifdc_len = sizeof(dcon->dcom_config);
        err = ifp->if_ioctl(ifp, SIOCGDRVCOM, (caddr_t)dcon);
        if ((err == EOK) && (dcon->dcom_config.num_io_windows > 0) &&
#if defined (SOC_J721E)
            (dcon->dcom_config.io_window_base[0] == J721E_CPSW_BASE)) {
#elif defined (SOC_J7200)
            (dcon->dcom_config.io_window_base[0] == J7200_CPSW_BASE)) {
#elif defined (SOC_J721S2)
            (dcon->dcom_config.io_window_base[0] == J721S2_CPSW_BASE)) {
#elif defined (SOC_J784S4)
            (dcon->dcom_config.io_window_base[0] == J784S4_CPSW_BASE)) {
#endif
            slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                  "%s:%d Error: Driver already loaded for %s",
                  __FUNCTION__, __LINE__, ifp->if_xname);
            return EBUSY;
        }
    }
    (free)(dcon);

    /* initialize to whatever you want to pass to cpsw_attach() */
    memset(&attach_args, 0x00, sizeof(attach_args));

    attach_args.iopkt = iopkt;
    attach_args.options = options;

    /* parse options */

    attach_args.cfg.num_io_windows = 1;
#if defined (SOC_J721E)
    attach_args.cfg.io_window_base[0] = J721E_CPSW_BASE;
    attach_args.cfg.io_window_size[0] = J721E_CPSW_SIZE;
#elif defined (SOC_J7200)
    attach_args.cfg.io_window_base[0] = J7200_CPSW_BASE;
    attach_args.cfg.io_window_size[0] = J7200_CPSW_SIZE;
#elif defined (SOC_J721S2)
    attach_args.cfg.io_window_base[0] = J721S2_CPSW_BASE;
    attach_args.cfg.io_window_size[0] = J721S2_CPSW_SIZE;
#elif defined (SOC_J784S4)
    attach_args.cfg.io_window_base[0] = J784S4_CPSW_BASE;
    attach_args.cfg.io_window_size[0] = J784S4_CPSW_BASE;
#endif

    attach_args.cfg.num_irqs = 0;

    /* we have single instance */
    single = 1;
    attach_args.cfg.device_index = instance = 0;

    for (instance = 0;;) {
        /* Apply detection criteria */

        /* Found one */
        dev = NULL; /* No Parent */
#if defined (CPSW2G)
        if (dev_attach("am", options, &cpsw_ca, &attach_args,
#elif defined (CPSW9G) || defined (CPSW5G)
        if (dev_attach("an", options, &cpsw_ca, &attach_args,
#elif defined (CPSW2G_MAIN)
        if (dev_attach("ao", options, &cpsw_ca, &attach_args,
#endif
            &single, &dev, NULL) != EOK) {
            break;
        }
        dev->dv_dll_hdl = dll_hdl;
        instance++;


        if (/* done_detection || */ single)
            break;
    }

    if (instance > 0)
        return EOK;

    return ENODEV;
}


int setRunMask(unsigned cpu)
{
    int *rsizep, rsize, size_tot;
    unsigned *rmaskp, *inheritp;
    unsigned buf[8];
    void *freep;
    long cpumask;

    RMSK_SET(cpu, &cpumask);
    if (ThreadCtl(_NTO_TCTL_RUNMASK, (void *)cpumask) == -1) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Error: ThreadCtl failed: %s",
                __FUNCTION__, __LINE__, strerror(errno));
        /* Continue in default affinity  */
    }

    /*
    * struct _thread_runmask is not
    * uniquely sized, so we construct
    * our own.
    */

    rsize = RMSK_SIZE(_syspage_ptr->num_cpu);

    size_tot = sizeof(*rsizep);
    size_tot += sizeof(*rmaskp) * rsize;
    size_tot += sizeof(*inheritp) * rsize;

    if (size_tot <= sizeof(buf)) {
        rsizep = (int *)buf;
        freep = NULL;
    }
    else if ((rsizep = freep = (malloc)(size_tot)) == NULL) {
        perror("malloc");
        return 1;
    }

    memset(rsizep, 0x00, size_tot);
    *rsizep = rsize;
    rmaskp = (unsigned *)(rsizep + 1);
    inheritp = rmaskp + rsize;

    /*
    * Both masks set to 0 means get the current
    * values without alteration.
    */

    if (ThreadCtl(_NTO_TCTL_RUNMASK_GET_AND_SET_INHERIT,
                 rsizep) == -1) {
        perror("_NTO_TCTL_RUNMASK_GET_AND_SET_INHERIT");
        (free)(freep);
        return 1;
    }

    /*
    * Restrict our inherit mask to the last cpu; leave the
    * runmask unaltered.
    */
    memset(rsizep, 0x00, size_tot);
    *rsizep = rsize;
    RMSK_SET(cpu, rmaskp);
    RMSK_SET(cpu, inheritp);

    if (ThreadCtl(_NTO_TCTL_RUNMASK_GET_AND_SET_INHERIT,
                 rsizep) == -1) {
        perror("_NTO_TCTL_RUNMASK_GET_AND_SET_INHERIT");
        (free)(freep);
        return 1;
    }

    (free)(freep);
    return 0;
}
/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int
cpsw_attach(struct device *parent, struct device *self, void *aux)
{
    struct cpsw_dev     *cpsw;
    struct ifnet        *ifp;
    char                *options;
    uint8_t             enaddr[1][ETHER_ADDR_LEN];
    int                 err = EOK;

    slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d Entry -->", __FUNCTION__, __LINE__);

    /* initialization and attach */
    options = attach_args.options;
    cpsw = (struct cpsw_dev *)self;

    // for any local use
    g_cpsw = cpsw;

    ifp = &cpsw->sc_ec.ec_if;
    ifp->if_softc = cpsw;
    cpsw->sc_iopkt = iopkt_selfp;
    cpsw->sc_iid = -1; /* not attached yet */
    strcpy(cpsw->typed_mem, "ram");
    cpsw->use_typed_mem = 0;
    cpsw->preferred_udma_channel = 0;
    cpsw->memFd = -1;
    cpsw->tx_freeq_threshold = CPSW_TX_FREEQ_THRESHOLD;
    cpsw->tx_descriptors_count = CPSW_MEM_TX_PKTS_DEFAULT;
    cpsw->rx_descriptors_count = CPSW_MEM_RX_PKTS_DEFAULT;
    cpsw->run_mask_core = DEFAULT_CPU_AFFINITY;
    cpsw->poll_ms = PHY_CALLOUT_DELAY;
    cpsw->cache_ops = 0;
    cpsw->smmu = 0;
    cpsw->virt_id = 0;
    cpsw->hw_csum = 0;
    cpsw->csum_flag_rx = 0;
    cpsw->csum_flag_tx = 0;
    memset(cpsw->multi_mac_entry, 0, (MAX_MULITCAST_MAC_ENTRY * ETHER_ADDR_LEN));
    cpsw->multi_mac_entries = 0;
    memset(cpsw->vlan_multicast, 0, (MAC_VLAN_SUPPORTED * sizeof(struct cpsw_vlan_multicast)));
    cpsw->rx_intr_prio = RX_INTR_PRIORITY;
    cpsw->tx_intr_prio = TX_INTR_PRIORITY;
    cpsw->cpts_intr_prio = CPTS_INTR_PRIORITY;
    cpsw->no_stack_thread = 0;
    cpsw->rx_pacing = 0;
    cpsw->rx_pacing_msec = RX_PACING_INTERVAL;
    cpsw->cpsw_connect_attempts = 10;
    cpsw->cpsw_connect_delay_ms = 100;
    cpsw->ipaddr[0] = 0x00;
    cpsw->ipaddr[1] = 0x00;
    cpsw->ipaddr[2] = 0x00;
    cpsw->ipaddr[3] = 0x00;

    /* Copy various config parameters initilized in cpsw_entry */
    memcpy(&cpsw->cfg, &attach_args.cfg, sizeof(nic_config_t));

    /* Parse options */
    cpsw_parse_options(cpsw, options, &cpsw->cfg);

    /* Set interface name */
    strcpy (ifp->if_xname, cpsw->sc_dev.dv_xname);
#if defined (CPSW2G)
    strcpy ((char *) cpsw->cfg.uptype, "am");
    strcpy ((char *) cpsw->cfg.device_description, "cpsw2g");
#elif defined (CPSW9G) || defined (CPSW5G)
    strcpy ((char *) cpsw->cfg.uptype, "an");
  #if defined (SOC_J721E) || defined (SOC_J784S4)
    strcpy ((char *) cpsw->cfg.device_description, "cpsw9g");
  #elif defined (SOC_J7200)
    strcpy ((char *) cpsw->cfg.device_description, "cpsw5g");
  #endif
#elif defined (CPSW2G_MAIN)
    strcpy ((char *) cpsw->cfg.uptype, "ao");
  #if defined (SOC_J721S2) || defined (SOC_J784S4)
    strcpy ((char *) cpsw->cfg.device_description, "cpsw2g-main");
  #endif
#endif

    /* Ethernet stats we are interested in */
    cpsw->stats.un.estats.valid_stats =
        NIC_ETHER_STAT_INTERNAL_TX_ERRORS |
        NIC_ETHER_STAT_INTERNAL_RX_ERRORS |
        NIC_ETHER_STAT_NO_CARRIER |
        NIC_ETHER_STAT_XCOLL_ABORTED |
        NIC_ETHER_STAT_SINGLE_COLLISIONS |
        NIC_ETHER_STAT_MULTI_COLLISIONS |
        NIC_ETHER_STAT_LATE_COLLISIONS |
        NIC_ETHER_STAT_TX_DEFERRED |
        NIC_ETHER_STAT_ALIGN_ERRORS |
        NIC_ETHER_STAT_FCS_ERRORS;

    /* Generic networking stats we are interested in */
    cpsw->stats.valid_stats =
        NIC_STAT_TX_FAILED_ALLOCS |
        NIC_STAT_RX_FAILED_ALLOCS |
        NIC_STAT_RXED_MULTICAST |
        NIC_STAT_RXED_BROADCAST |
        NIC_STAT_TXED_BROADCAST |
        NIC_STAT_TXED_MULTICAST;

    cpsw->cfg.priority = IRUPT_PRIO_DEFAULT;
    cpsw->cfg.lan = cpsw->sc_dev.dv_unit;
    cpsw->cfg.media_rate = -1;
    cpsw->cfg.duplex = -1;
    cpsw->force_link = -1;
    cpsw->cfg.flags |= NIC_FLAG_LINK_DOWN;
    cpsw->linkup = 0;
    if  (!cpsw->mac_to_mac) {
        /* default to 1Gbps if not mac-to-mac */
        cpsw->speed = 1000;
    }

    cpsw->cfg.mtu = ETHERMTU;
    cpsw->cfg.mru = ETHERMTU;
    cpsw->cfg.flags |= NIC_FLAG_MULTICAST;
    cpsw->cfg.mac_length = ETHER_ADDR_LEN;
    if (cpsw->promiscuous) {
        cpsw->cfg.flags |= NIC_FLAG_PROMISCUOUS;
    }

    /* set capabilities */
    ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;

    if (cpsw->hw_csum) {
        /* For TX the J721E hardware supports checksum computation based on byte offset and
         * length in descriptor, effectively enabling support for all protocols.
         * Now as hardware supports only one checksum in the full packet. For example TCP header, TCP
         * payload or IP header, we offload TCP and UDP payload as these are most expensive */
        ifp->if_capabilities_tx = IFCAP_CSUM_TCPv4 | IFCAP_CSUM_UDPv4 | IFCAP_CSUM_TCPv6 | IFCAP_CSUM_UDPv6;
        /* For RX the J721E hardware supports UDP and TCP checksum offload */
        ifp->if_capabilities_rx = IFCAP_CSUM_TCPv4 | IFCAP_CSUM_UDPv4 | IFCAP_CSUM_TCPv6 | IFCAP_CSUM_UDPv6;
    }
    cpsw->sc_ec.ec_capabilities |= ETHERCAP_VLAN_MTU;

    /* Set callouts */
    ifp->if_ioctl = cpsw_ioctl;
    ifp->if_start = cpsw_start;
    ifp->if_init = cpsw_init;
    ifp->if_stop = cpsw_stop;
    IFQ_SET_READY(&ifp->if_snd);

    /* hook up so media devctls work */
    bsd_mii_initmedia (cpsw);

    /* Enable IO capability */
    if (ThreadCtl(_NTO_TCTL_IO_PRIV, NULL) == -1) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Error: ThreadCtl failed: %s",
                __FUNCTION__, __LINE__, strerror(errno));
        /* Continue in user mode  */
    }

    ///////////////////////////////////////////////////////////////////////////
    // Setting thread affinity to run on core specified and set the other
    // threads created by this thread to inherit this
    ///////////////////////////////////////////////////////////////////////////
    /* Run only on cpu core specified */
    if (cpsw->run_mask_core != DEFAULT_CPU_AFFINITY)
        setRunMask(cpsw->run_mask_core);
    ///////////////////////////////////////////////////////////////////////////

    /* If media was not specified on cmdline, default to NIC_MEDIA_802_3 */
    if (cpsw->cfg.media == -1) {
        cpsw->cfg.media = NIC_MEDIA_802_3;
        cpsw->stats.media = cpsw->cfg.media;
    }
    else {
        cpsw->stats.media = cpsw->cfg.media;
    }

    if (cpsw->cfg.mtu == 0 || cpsw->cfg.mtu > (ETHER_MAX_LEN - ETHER_CRC_LEN)) {
        cpsw->cfg.mtu = (ETHER_MAX_LEN - ETHER_CRC_LEN);
    }

    if (cpsw->cfg.mru == 0 || cpsw->cfg.mru > (ETHER_MAX_LEN - ETHER_CRC_LEN)) {
        cpsw->cfg.mru = (ETHER_MAX_LEN - ETHER_CRC_LEN);
    }

    callout_init(&cpsw->phy_callout);


    /* Setup interrupt related info */
    cpsw->sc_inter.func = cpsw_process_interrupt;
    cpsw->sc_inter.enable = cpsw_enable_interrupt;
    cpsw->sc_inter.arg = cpsw;
    if ((err = interrupt_entry_init (&cpsw->sc_inter, 0, NULL, IRUPT_PRIO_DEFAULT)) != EOK) {
        return err;
    }

    pthread_mutex_init(&cpsw->rx_mutex, NULL);
    IFQ_SET_MAXLEN(&cpsw->rx_queue, IFQ_MAXLEN);
    cpsw->rx_running = 0;
    cpsw->rx_thread_exit = 0;

    cpsw->chid = ChannelCreate(0);
    cpsw->coid = ConnectAttach(ND_LOCAL_NODE, 0, cpsw->chid, _NTO_SIDE_CHANNEL, 0);

    if (cpsw->rx_pacing) {
        SIGEV_PULSE_INIT(&cpsw->rx_pacing_event, cpsw->coid, SIGEV_PULSE_PRIO_INHERIT, CPSW_RX_PULSE, 0);
        callout_init(&cpsw->rx_pacing_callout);
    }

    if (nw_pthread_create(&cpsw->tid, NULL,
          cpsw_rx_thread, cpsw, 0,
          cpsw_rx_thread_init, cpsw) != EOK) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Error: nw_pthread_create failed for cpsw_rx_thread: %s",
                __FUNCTION__, __LINE__, strerror(errno));
        return ENOMEM;
    }

#if defined(CPSW9G) || defined(CPSW5G)
    if (nw_pthread_create(&g_cpswRecoveryTid, NULL, cpsw_recovery_thread, cpsw, 0, cpsw_recovery_thread_init, cpsw) != EOK) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Error: nw_pthread_create failed for cpsw_recovery_thread: %s", __FUNCTION__, __LINE__, strerror(errno));
        return ENOMEM;
    }
#endif

    if (cpsw->use_typed_mem) {

        cpsw->memFd = posix_typed_mem_open(cpsw->typed_mem, O_RDWR, POSIX_TYPED_MEM_ALLOCATE_CONTIG);
        if(cpsw->memFd == -1)	{
            slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Unable to open memory region - errno-%d",
                __FUNCTION__, __LINE__, errno);
            return ENOMEM;
        }
        slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d Opened typed memory '%s' fd:%d",
            __FUNCTION__, __LINE__, cpsw->typed_mem, cpsw->memFd);
    }

    EnetIf_InitObj();

#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    /* Nothing for CPSW2G */
#elif defined (CPSW9G) || defined (CPSW5G)
    if (EnetIf_RemoteAttach(cpsw->cfg.current_address) != 0) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Error: EnetIf_RemoteAttach failed",
                __FUNCTION__, __LINE__);
        return ENOMEM;
    }
#endif

    if (cpsw->preferred_udma_channel) {
        EnetIf_SetPerferredDmaChNum(cpsw->preferred_udma_channel);
    }

    if (cpsw->cache_ops) {

        /* Cache Init */
        cachectl.fd = NOFD;
        if(cache_init(0, &cachectl, NULL) == -1) {
            slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d cache_init fail\n", __FUNCTION__, __LINE__);
        } else {
            slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d cache_init done\n", __FUNCTION__, __LINE__);
        }
        EnetIf_EnableCacheOps();
    }

    if (cpsw->smmu) {
        EnetIf_EnableSmmu(cpsw->virt_id);
    }

    EnetIf_SetDescriptorCount(cpsw->tx_descriptors_count, cpsw->rx_descriptors_count);
    EnetIf_GetMacAddr(enaddr);
    memcpy(cpsw->cfg.permanent_address, enaddr, ETHER_ADDR_LEN);
    /* Not specified on command line, copy to current */
    if (!memcmp(cpsw->cfg.current_address, "\0\0\0\0\0\0", ETHER_ADDR_LEN)) {
        memcpy(cpsw->cfg.current_address, cpsw->cfg.permanent_address, ETHER_ADDR_LEN);
    }

#if defined (CPSW9G) || defined (CPSW5G)
    if (cpsw->ptp_enable)
    {
        EnetIf_enableSyncTimer();
    }

    EnetIf_RegisterForCPSWRecoveryNotifications();
#endif

    if_attach(ifp);
    ether_ifattach(ifp, cpsw->cfg.current_address);

    if (cpsw->cfg.verbose) {
        nic_dump_config (&cpsw->cfg);
    }

    cpsw->sc_sdhook = shutdownhook_establish(cpsw_shutdown, cpsw);

    return EOK;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int
cpsw_init(struct ifnet *ifp)
{
    struct cpsw_dev *cpsw = ifp->if_softc;
    struct nw_work_thread       *wtp = WTP;
    int32_t linkStatus = EOK;
    uint32_t callout_delay = cpsw->poll_ms;
    int loop = 0;

    if (cpsw->dying == 1)
        return (0);

    if (cpsw->cfg.verbose & DEBUG_TRACE) {
        slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d Entry --> ",
            __FUNCTION__, __LINE__);
    }

    if(memcmp(cpsw->cfg.current_address, LLADDR(ifp->if_sadl), ifp->if_addrlen)) {
        memcpy(cpsw->cfg.current_address, LLADDR(ifp->if_sadl), ifp->if_addrlen);
        /* update the hardware */
#if defined (CPSW9G) || defined (CPSW5G)
        cpsw_stop(ifp, 1);
        EnetIf_SetMacAddr(cpsw->cfg.current_address);
#endif
    }

    if ((ifp->if_flags & IFF_RUNNING) == 0) {

        if ((cpsw->promiscuous) || (ifp->if_flags & IFF_PROMISC)) {
            slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d --> Setting promiscuous", __FUNCTION__, __LINE__);
            cpsw->cfg.flags |= NIC_FLAG_PROMISCUOUS;
        }

        if (ifp->if_capenable_rx & (IFCAP_CSUM_TCPv4 | IFCAP_CSUM_UDPv4 | IFCAP_CSUM_TCPv6 | IFCAP_CSUM_UDPv6)) {
            cpsw->csum_flag_rx = ifp->if_capenable_rx;
            slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d --> RX CSUM enabled - 0x%x", __FUNCTION__, __LINE__, cpsw->csum_flag_rx);

        }

        if (ifp->if_capenable_tx & (IFCAP_CSUM_TCPv4 | IFCAP_CSUM_UDPv4 | IFCAP_CSUM_TCPv6 | IFCAP_CSUM_UDPv6)) {
            cpsw->csum_flag_tx = ifp->if_capenable_tx;
            slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d --> TX CSUM enabled - 0x%x", __FUNCTION__, __LINE__, cpsw->csum_flag_tx);
        }

        EnetIf_Init(cpsw->mac_to_mac, cpsw->speed, cpsw->cfg.current_address);
        if (cpsw->join_vlan != NULL) {
            loop = 0;
            while (cpsw->join_vlan[loop] != 0) {
                EnetIf_AddVlan(cpsw->join_vlan[loop], 1);
                loop++;
            }
        }
#if defined (CPSW2G) || defined (CPSW2G_MAIN)
        EnetIf_InitPhy();
        if (!cpsw->mac_to_mac) {
            linkStatus = EnetIf_CheckPortLinkUp(LINKUP_UP_TIMEOUT);
        }
        if (cpsw->ptp_enable)
        {
            EnetIf_PtpInit();
        }
#elif defined (CPSW9G) || defined (CPSW5G)
        if (!cpsw->mac_to_mac) {
            linkStatus = EnetIf_CheckPortLinkUp(LINKUP_UP_TIMEOUT);
        }
#endif
        if ((linkStatus == EOK) || (cpsw->mac_to_mac)) {
            cpsw->cfg.connector = NIC_CONNECTOR_MII;

            if (cpsw->mac_to_mac) {
                cpsw->cfg.connector = NIC_CONNECTOR_UNKNOWN;
                cpsw->cfg.media_rate = cpsw->speed * 1000L;
                cpsw->cfg.duplex = 1;
                cpsw->cfg.phy_addr = 0;
            }
            else {
                EnetIf_PortLinkUpCfg(&cpsw->speed, &cpsw->cfg.duplex);
                cpsw->cfg.media_rate = cpsw->speed * 1000L;
                cpsw->cfg.phy_addr = 0;
            }
            cpsw->cfg.flags &= ~NIC_FLAG_LINK_DOWN;
            cpsw->linkup = 1;
            if_link_state_change(ifp, LINK_STATE_UP);
            slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d Phy is linked, ifp=0x%p", __FUNCTION__, __LINE__, ifp);
        }
        else {
            cpsw->cfg.connector = NIC_CONNECTOR_MII;
            cpsw->cfg.phy_addr = 0;
            cpsw->cfg.duplex = cpsw->cfg.media_rate = 0;
            cpsw->cfg.flags = NIC_FLAG_LINK_DOWN;
            cpsw->linkup = 0;
            if_link_state_change(ifp, LINK_STATE_DOWN);
            slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d Phy is not linked", __FUNCTION__, __LINE__);
            callout_delay = PHY_CALLOUT_DELAY_LINK_DN;
        }
        callout_msec (&cpsw->phy_callout, callout_delay,
                    cpsw_MonitorPhy, cpsw);

        if (cpsw->rx_pacing) {
            callout_msec (&cpsw->rx_pacing_callout, cpsw->rx_pacing_msec, cpsw_rxPacing, cpsw);
        }
    }
    else {
        if ((cpsw->promiscuous) || (ifp->if_flags & IFF_PROMISC)) {
            slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d --> Setting promiscuous", __FUNCTION__, __LINE__);
            cpsw->cfg.flags |= NIC_FLAG_PROMISCUOUS;
            EnetIf_EnablePromiscuousMode();
        }
    }

    NW_SIGLOCK_P (&ifp->if_snd_ex, cpsw->sc_iopkt, wtp);
    ifp->if_flags_tx |= IFF_RUNNING;
    ifp->if_flags_tx &= ~IFF_OACTIVE;
    NW_SIGUNLOCK_P (&ifp->if_snd_ex, cpsw->sc_iopkt, wtp);

    ifp->if_flags |= IFF_RUNNING;

    if (cpsw->cfg.verbose & DEBUG_TRACE) {
        slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d <--Exit ",
            __FUNCTION__, __LINE__);
    }
    return EOK;
}

void
cpsw_stop(struct ifnet *ifp, int disable)
{
    struct cpsw_dev *cpsw = ifp->if_softc;
    struct nw_work_thread   *wtp = WTP;
    struct _iopkt_self  *iopkt = cpsw->sc_iopkt;

    if (cpsw->cfg.verbose & DEBUG_TRACE) {
        slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d Entry -->",
            __FUNCTION__, __LINE__);
    }

    /* Lock out the transmit side */
    NW_SIGLOCK_P (&ifp->if_snd_ex, iopkt, wtp);

    ifp->if_flags_tx |= IFF_OACTIVE;

    /* Release any queued transmit buffers */

    ifp->if_flags_tx &= ~(IFF_RUNNING | IFF_OACTIVE);
    /* Done with the transmit side */
    NW_SIGUNLOCK_P (&ifp->if_snd_ex, iopkt, wtp);


    /*
     * - Cancel any pending io
     * - Clear any interrupt source registers
     * - Clear any interrupt pending registers
     * - Release any queued transmit buffers.
     */

    if (disable) {
        if (cpsw->linkup) {
            /* Stop monitoring */
            callout_stop(&cpsw->phy_callout);
            if (cpsw->rx_pacing) {
                callout_stop (&cpsw->rx_pacing_callout);
            }
#if defined (CPSW9G) || defined (CPSW5G)
            if (cpsw->ptp_enable)
            {
                EnetIf_disableSyncTimer();
            }
#endif
            cpsw_del_multicast_entry_all(cpsw, 0);
            if (cpsw->join_vlan != NULL) {
                int loop = 0;
                while (cpsw->join_vlan[loop] != 0) {
                    EnetIf_AleFloodUnregMcast(cpsw->join_vlan[loop], 0);
                    cpsw_del_multicast_entry_all(cpsw, cpsw->join_vlan[loop]);
                    EnetIf_AddVlan(cpsw->join_vlan[loop], 0);
                    loop++;
                }
            }
            EnetIf_Close();
            cpsw->cfg.flags |= NIC_FLAG_LINK_DOWN;
            cpsw->linkup = 0;
            if_link_state_change(ifp, LINK_STATE_DOWN);
        }
    }

    ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

    if (cpsw->cfg.verbose & DEBUG_TRACE) {
        slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d <--Exit",
            __FUNCTION__, __LINE__);
    }
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
static void cpsw_reap_pkts(struct cpsw_dev  *cpsw)
{
    cpsw->tx_reaped = 1;
    EnetIf_retrieveFreeTxPkts();
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
static struct mbuf *cpsw_defrag (struct mbuf *m)

{
    struct mbuf *m2;

    MGET (m2, M_DONTWAIT, MT_DATA);
    if (m2 == NULL) {
        m_freem (m);
        return (NULL);
    }

    M_COPY_PKTHDR (m2, m);

    MCLGET (m2, M_DONTWAIT);
    if ((m2->m_flags & M_EXT) == 0) {
        m_freem (m);
        m_freem (m2);
        return (NULL);
    }

    /* Paranoid ? */
    if (m->m_pkthdr.len > m2->m_ext.ext_size) {
        m_freem (m);
        m_freem (m2);
        return (NULL);
    }

    m_copydata (m, 0, m->m_pkthdr.len, mtod(m2, caddr_t));
    m2->m_pkthdr.len = m2->m_len = m->m_pkthdr.len;

    m_freem(m);

    return (m2);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void
cpsw_start(struct ifnet *ifp)
{
    struct cpsw_dev     *cpsw = ifp->if_softc;
    struct mbuf     *m;
    struct mbuf *m2;
    struct nw_work_thread   *wtp = WTP;
    int num_frags;
    uint32_t num_free = 0;

    if (cpsw->cfg.verbose & DEBUG_TRACE) {
        slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d Entry -->", __FUNCTION__, __LINE__);
    }
#if defined (USE_TRACE_INSTRUMENTATION)
    trace_logi(800, 0, 0);
#endif

    if ((ifp->if_flags_tx & IFF_RUNNING) == 0) {
        NW_SIGUNLOCK_P (&ifp->if_snd_ex, cpsw->sc_iopkt, wtp);
        return;
    }

    if (!cpsw->linkup) {
        IFQ_PURGE(&ifp->if_snd);
        NW_SIGUNLOCK_P (&ifp->if_snd_ex, cpsw->sc_iopkt, wtp);
        return;
    }

    ifp->if_flags_tx |= IFF_OACTIVE;

    for (;;) {
        /* Are there buffers available to send this packet */
        num_free = EnetIf_GetTxFreeQCnt();
        if(num_free <= cpsw->tx_freeq_threshold)
        {
            cpsw_reap_pkts(cpsw);
        }

        num_free = EnetIf_GetTxFreeQCnt();
        if(num_free == 0) {
             /* Leave IFF_OACTIVE so the stack doesn't call us again */
             NW_SIGUNLOCK_P (&ifp->if_snd_ex, cpsw->sc_iopkt, wtp);
             return;
        }

        IFQ_DEQUEUE(&ifp->if_snd, m);
        if (m == NULL) {
            break;
        }

        /* m could be multiple buffers, defrag to single */
        if ((m2 = cpsw_defrag(m)) == NULL) {
            cpsw->stats.tx_failed_allocs++;
            m_free(m);
            continue;
        }

        /* Re-adjust the number of fragments and total length */
        m = m2;
        for (num_frags = 0, m2 = m; m2; num_frags++) {
            m2 = m2->m_next;
        }

        m->m_nextpkt = NULL;
        for (m2 = m; m2; m2 = m2->m_next) {
            if (!m2->m_len) {
                num_frags --;
                continue;
            }
        }

        /* Build the descriptor chain if there is more than one fragement */
        if (num_frags > 1) {
            slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Error: we dont support multiple fragments",
            __FUNCTION__, __LINE__);
        }

        if (cpsw->cfg.verbose & DEBUG_START_PACKET) {
            if (m->m_data[12] == 0x08 && m->m_data[13] == 0x00) {
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d packet length %d, EthType is 0x%02x%02x, IPv4 Protocol is 0x%x, txFreeCnt %d",
                __FUNCTION__, __LINE__, m->m_len, m->m_data[12], m->m_data[13], m->m_data[23], num_free);
            }
            else if (m->m_data[12] == 0x86 && m->m_data[13] == 0xdd) {
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d packet length %d, EthType is 0x%02x%02x, IPv6 Protocol is 0x%x, txFreeCnt %d",
                __FUNCTION__, __LINE__, m->m_len, m->m_data[12], m->m_data[13], m->m_data[20], num_free);
            }
            else {
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d packet length %d, EthType is 0x%02x%02x, txFreeCnt %d",
                __FUNCTION__, __LINE__, m->m_len, m->m_data[12], m->m_data[13], num_free);
            }
        }

#if defined (CPSW2G) || defined (CPSW2G_MAIN)
        /* Nothing to do for CPSW2G */
#elif defined (CPSW9G) || defined (CPSW5G)
#ifdef CPSW_DYNAMIC_IP_ADDRESS
        /* Check every packat for ARP request for IPv4 address */
        if ((m->m_data[12] == 0x08) && (m->m_data[13] == 0x06)) {
            if ((m->m_data[16] == 0x08)&&(m->m_data[17] == 0x00)) {
                if ((cpsw->ipaddr[0] != m->m_data[28]) ||
                    (cpsw->ipaddr[1] != m->m_data[29]) ||
                    (cpsw->ipaddr[2] != m->m_data[30]) ||
                    (cpsw->ipaddr[3] != m->m_data[31])) {
                    cpsw->ipaddr[0] = m->m_data[28];
                    cpsw->ipaddr[1] = m->m_data[29];
                    cpsw->ipaddr[2] = m->m_data[30];
                    cpsw->ipaddr[3] = m->m_data[31];
                    slogf(_SLOGC_NETWORK, _SLOG_INFO, "IPv4 address: %d:%d:%d:%d",
                            cpsw->ipaddr[0], cpsw->ipaddr[1], cpsw->ipaddr[2], cpsw->ipaddr[3]);
                    EnetIf_RegisterIPv4Address(cpsw->ipaddr);
                }
            }
        }
#else
        /* Snoop send packet to find ARP request for IPv4 address */
        if(cpsw->ipaddr[0] == 0)
        {
            if ((m->m_data[12] == 0x08)&&(m->m_data[13] == 0x06)&&
                    (m->m_data[16] == 0x08)&&(m->m_data[17] == 0x00)) {
                cpsw->ipaddr[0] = m->m_data[28];
                cpsw->ipaddr[1] = m->m_data[29];
                cpsw->ipaddr[2] = m->m_data[30];
                cpsw->ipaddr[3] = m->m_data[31];
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "IPv4 address: %d:%d:%d:%d",
                        cpsw->ipaddr[0], cpsw->ipaddr[1], cpsw->ipaddr[2], cpsw->ipaddr[3]);
                EnetIf_RegisterIPv4Address(cpsw->ipaddr);
            }
        }
#endif
#endif

        /* You're now committed to transmitting it */
        if (EnetIf_SendPkt(m) != 0) {
            /* error, drop packet and continue */
            m_freem (m);
            ifp->if_oerrors++;
            continue;
        }

#if NBPFILTER > 0
        /* Pass the packet to any BPF listeners */
        if (ifp->if_bpf) {
            bpf_mtap (ifp->if_bpf, m);
        }
#endif

        ifp->if_opackets++;  // for ifconfig -v
        // or if error:  ifp->if_oerrors++;
    }

#if defined (USE_TRACE_INSTRUMENTATION)
    trace_logi(801, (unsigned) ifp->if_opackets, (unsigned) ifp->if_oerrors );
#endif
    ifp->if_flags_tx &= ~IFF_OACTIVE;
    NW_SIGUNLOCK_P(&ifp->if_snd_ex, cpsw->sc_iopkt, wtp);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int
cpsw_detach(struct device *dev, int flags)
{
    struct cpsw_dev *cpsw;
    struct ifnet    *ifp;

    cpsw = (struct cpsw_dev *)dev;

    if (cpsw->cfg.verbose & DEBUG_TRACE) {
        slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d Entry -->", __FUNCTION__, __LINE__);
    }

    /*
     * Clean up everything.
     *
     * The interface is going away but io-pkt is staying up.
     */
    ifp = &cpsw->sc_ec.ec_if;

    /* Don't init() while we're dying. */
    cpsw->dying = 1;

    cpsw_stop(ifp, 1);

#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    /* Nothing for CPSW2G */
#elif defined (CPSW9G) || defined (CPSW5G)
    EnetIf_RemoteDetach();
#endif

    IF_PURGE(&cpsw->rx_queue);
    pthread_mutex_destroy(&cpsw->rx_mutex);
    interrupt_entry_remove(&cpsw->sc_inter, NULL);

    ether_ifdetach(ifp);

    if_detach(ifp);

    shutdownhook_disestablish(cpsw->sc_sdhook);

    nw_pthread_reap(cpsw->tid);
#if defined (CPSW9G) || defined (CPSW5G)
    nw_pthread_reap(g_cpswRecoveryTid);
#endif
    ConnectDetach(cpsw->coid);
    ChannelDestroy(cpsw->chid);

    /* close typed mem fd if open */
    if (g_cpsw->memFd)
        close(g_cpsw->memFd);
    g_cpsw->memFd = -1;

    if ((cpsw->cache_ops) && (cachectl.fd != -1))
        cache_fini(&cachectl);

    if (g_cpsw->join_vlan != NULL) {
        (free)(g_cpsw->join_vlan);
        g_cpsw->join_vlan = NULL;
    }

    if (cpsw->cfg.verbose & DEBUG_TRACE) {
        slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d <--Exit", __FUNCTION__, __LINE__);
    }

    return EOK;
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void
cpsw_shutdown(void *arg)
{
    struct cpsw_dev *cpsw;

    /* All of io-pkt is going away.  Just quiet hardware. */

    cpsw = arg;

    if (cpsw->cfg.verbose & DEBUG_TRACE) {
        slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d Entry -->", __FUNCTION__, __LINE__);
    }

    cpsw_stop(&cpsw->sc_ec.ec_if, 1);

#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    /* Nothing for CPSW2G */
#elif defined (CPSW9G) || defined (CPSW5G)
    EnetIf_RemoteDetach();
#endif

    if (cpsw->cfg.verbose & DEBUG_TRACE) {
        slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d <--Exit", __FUNCTION__, __LINE__);
    }
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void cpsw_MonitorPhy (void *arg)
{
	struct cpsw_dev *cpsw = arg;
    struct ifnet        *ifp = &cpsw->sc_ec.ec_if;
    uint32_t callout_delay = cpsw->poll_ms;

    if (!cpsw->tx_reaped) {
        NW_SIGLOCK (&ifp->if_snd_ex, cpsw->sc_iopkt);
        cpsw_reap_pkts(cpsw);
        NW_SIGUNLOCK (&ifp->if_snd_ex, cpsw->sc_iopkt);
    }
    cpsw->tx_reaped = 0;

#if defined (CPSW2G) || defined (CPSW2G_MAIN)
    if (!cpsw->mac_to_mac) {
        int32_t linkStatus = EnetIf_CheckPortLinkUp(0);

        if ((linkStatus == EOK) && (cpsw->linkup == 0)) {
            EnetIf_PortLinkUpCfg(&cpsw->speed, &cpsw->cfg.duplex);
            cpsw->cfg.media_rate = cpsw->speed * 1000L;
            cpsw->cfg.flags &= ~NIC_FLAG_LINK_DOWN;
            cpsw->linkup = 1;
            if_link_state_change(ifp, LINK_STATE_UP);
            slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d Phy is linked, ifp=0x%p", __FUNCTION__, __LINE__, ifp);
        }
        else if ((linkStatus != EOK) && (cpsw->linkup == 1)) {
            cpsw->cfg.duplex = cpsw->cfg.media_rate = 0;
            cpsw->cfg.flags = NIC_FLAG_LINK_DOWN;
            cpsw->linkup = 0;
            if_link_state_change(ifp, LINK_STATE_DOWN);
            slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d Phy is not linked", __FUNCTION__, __LINE__);
            callout_delay = PHY_CALLOUT_DELAY_LINK_DN;
        }

        EnetIf_Tick();
    }
#elif defined (CPSW9G) || defined (CPSW5G)
    if (!cpsw->mac_to_mac) {
        int32_t linkStatus = EnetIf_CheckPortLinkUp(0);

        if ((linkStatus == EOK) && (cpsw->linkup == 0)) {
            EnetIf_PortLinkUpCfg(&cpsw->speed, &cpsw->cfg.duplex);
            cpsw->cfg.media_rate = cpsw->speed * 1000L;
            cpsw->cfg.flags &= ~NIC_FLAG_LINK_DOWN;
            cpsw->linkup = 1;
            if_link_state_change(ifp, LINK_STATE_UP);
            slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d Phy is linked", __FUNCTION__, __LINE__);
        }
        else if ((linkStatus != EOK) && (cpsw->linkup == 1)) {
            cpsw->cfg.duplex = cpsw->cfg.media_rate = 0;
            cpsw->cfg.flags = NIC_FLAG_LINK_DOWN;
            cpsw->linkup = 0;
            if_link_state_change(ifp, LINK_STATE_DOWN);
            slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d Phy is not linked", __FUNCTION__, __LINE__);
            callout_delay = PHY_CALLOUT_DELAY_LINK_DN;
        }
    }
#endif

    callout_msec(&cpsw->phy_callout, callout_delay,
                 cpsw_MonitorPhy, cpsw);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void cpsw_rxPacing (void *arg)
{
    struct cpsw_dev *cpsw = arg;
    MsgSendPulse(g_cpsw->coid, SIGEV_PULSE_PRIO_INHERIT, CPSW_RX_PULSE, 0);
    callout_msec(&cpsw->rx_pacing_callout, cpsw->rx_pacing_msec, cpsw_rxPacing, cpsw);
}

/*****************************************************************************/
/*  Process RX                                                               */
/*****************************************************************************/
struct mbuf * cpsw_process_rx(void *arg, struct mbuf *m)
{
    struct cpsw_dev     *cpsw = arg;
    struct mbuf         *mnew;
    struct ifnet        *ifp = &cpsw->sc_ec.ec_if;
    struct nw_work_thread    *wtp = WTP;
#if 0
    struct ether_vlan_header *vlan_hdr;
#endif
    const struct sigevent    *evp;

    m->m_pkthdr.rcvif = ifp;

#if NBPFILTER > 0
    /* Pass the packet to any BPF listeners */
    if (ifp->if_bpf) {
        bpf_mtap (ifp->if_bpf, m);
    }
#endif

    ifp->if_ipackets++; // for ifconfig -v

#if 0
    vlan_hdr = mtod(m, struct ether_vlan_header*);
    if (((ntohs(vlan_hdr->evl_encap_proto) == ETHERTYPE_VLAN) &&
         (ntohs(vlan_hdr->evl_proto) == ETHERTYPE_1722)) ||
        (cpsw->no_stack_thread)) {
        /* 1722 packet, send it straight up for minimum latency */
#else
    if (cpsw->no_stack_thread) {
#endif
        (*ifp->if_input)(ifp, m);
    }
    else {
        /*
         * Send it up from a stack thread so bridge and
         * fastforward work. Without this we get logs of "no flow"
         */
        pthread_mutex_lock(&cpsw->rx_mutex);
        if (IF_QFULL(&cpsw->rx_queue)) {
            if (cpsw->cfg.verbose & DEBUG_TX_RX_ERRORS) {
                slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Error:rx_queue is full", __FUNCTION__, __LINE__);
            }
            m_freem(m);
            ifp->if_ierrors++;
        }
        else {
            IF_ENQUEUE(&cpsw->rx_queue, m);
            evp = interrupt_queue(cpsw->sc_iopkt, &cpsw->sc_inter);
            if (evp != NULL) {
                MsgSendPulse(evp->sigev_coid, evp->sigev_priority,
                    evp->sigev_code,
                    (uintptr_t)evp->sigev_value.sival_ptr);
            } else {
                cpsw->rx_running = 1;
            }
        }
        pthread_mutex_unlock(&cpsw->rx_mutex);
    }

    /* Get a packet/buffer to replace the one that was filled */
    mnew = m_getcl_wtp(M_DONTWAIT, MT_DATA, M_PKTHDR, wtp);
    if (!mnew) {
        if (cpsw->cfg.verbose & DEBUG_TX_RX_ERRORS) {
            slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Error: m_getcl_wtp failed", __FUNCTION__, __LINE__);
        }
        ifp->if_ierrors++;  // for ifconfig -v
        return NULL;
    }

    return mnew;
}


/*****************************************************************************/
/*  RX thread function                                                       */
/*****************************************************************************/
void *cpsw_rx_thread (void *arg)
{
    struct cpsw_dev     *cpsw = arg;
    int  rcvid;
    struct _pulse pulse;
    iov_t  msg;

    SETIOV(&msg, &pulse, sizeof(pulse));

    while (cpsw->rx_thread_exit == 0) {
        rcvid = MsgReceivev(cpsw->chid, &msg, 1, NULL);
        if (rcvid == 0) {
            switch (pulse.code)
            {
                case CPSW_RX_PULSE:
#if defined (USE_TRACE_INSTRUMENTATION)
                    trace_logi(805, 0, 0);
#endif
                    if (cpsw->rx_pacing) {
                        EnetIf_GetRx(cpsw);
                    }
                    else {
                        EnetIf_RxIntr();
                    }
#if defined (USE_TRACE_INSTRUMENTATION)
                    trace_logi(807, 0, 0);
#endif
                break;
                case CPSW_QUIESCE_PULSE:
                    if (pulse.value.sival_int == 1) {
                        cpsw->rx_thread_exit = 1;
                    }
                    quiesce_block(pulse.value.sival_int);
                break;
                default:
                    if (cpsw->cfg.verbose & DEBUG_TX_RX_ERRORS) {
                        slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                            "%s:%d Error: CPSW Rx Unknown pulse %d received", __FUNCTION__, __LINE__, pulse.code);
                    }
                break;
            }
        }
        else
        {
            if (cpsw->cfg.verbose & DEBUG_TX_RX_ERRORS) {
                slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Error: CPSW Rx MsgReceive error", __FUNCTION__, __LINE__);
            }
        }
    }
    return NULL;
}

/*****************************************************************************/
/* RX io-pkt thread process interrupt                                        */
/*****************************************************************************/

int cpsw_process_interrupt (void *arg, struct nw_work_thread *wtp)

{
    struct cpsw_dev    *cpsw = arg;
    struct ifnet    *ifp;
    struct mbuf     *m;
#if defined (USE_TRACE_INSTRUMENTATION)
    uint   rxCnt = 0;
    uint   txDone = 0;
#endif

    ifp = &cpsw->sc_ec.ec_if;

#if defined (USE_TRACE_INSTRUMENTATION)
    trace_logi(808, 0, 0);
#endif
    while(1) {
        pthread_mutex_lock(&cpsw->rx_mutex);
        IF_DEQUEUE(&cpsw->rx_queue, m);
        if (m != NULL) {
            pthread_mutex_unlock(&cpsw->rx_mutex);
#if defined (USE_TRACE_INSTRUMENTATION)
            rxCnt++;
#endif
            (*ifp->if_input)(ifp, m);
        } else {
            cpsw->rx_running = 0;
            pthread_mutex_unlock(&cpsw->rx_mutex);
            break;
        }
    }
#if defined (USE_TRACE_INSTRUMENTATION)
    trace_logi(809, rxCnt, 0);
#endif

    /* Give TX a chance */
    NW_SIGLOCK_P (&ifp->if_snd_ex, cpsw->sc_iopkt, wtp);
    if (ifp->if_flags_tx & IFF_OACTIVE) {
        cpsw_start(ifp);
#if defined (USE_TRACE_INSTRUMENTATION)
        txDone = 1;
#endif
    } else {
        NW_SIGUNLOCK_P (&ifp->if_snd_ex, cpsw->sc_iopkt, wtp);
    }
#if defined (USE_TRACE_INSTRUMENTATION)
    trace_logi(810, txDone, 0);
#endif

    return 1;
}

/*****************************************************************************/
/* RX io-pkt thread enable interrupt                                         */
/*****************************************************************************/
int cpsw_enable_interrupt (void *arg)
{
    struct cpsw_dev    *cpsw = arg;

    pthread_mutex_lock(&cpsw->rx_mutex);
    if (cpsw->rx_running) {
        cpsw->rx_running = 0;
        interrupt_queue(cpsw->sc_iopkt, &cpsw->sc_inter);
    }
    pthread_mutex_unlock(&cpsw->rx_mutex);

    return 1;
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

#if defined(CPSW9G) || defined(CPSW5G)
static void cpsw_recovery_thread_quiesce(void *arg, int die)
{
    EnetIf_print("%s: Messaging cpsw_recovery_thread to quiesce die=%d", __FUNCTION__, die);

    MsgSendPulse(g_cpsw_recovery_connection_id, SIGEV_PULSE_PRIO_INHERIT,
                 CPSW_RECOVERY_MSG_QUIESCE, die);

    return;
}

static int cpsw_recovery_thread_init(void *arg)
{
    struct cpsw_dev       *cpsw = (struct cpsw_dev *)arg;
    struct nw_work_thread *wtp  = WTP;

    pthread_setname_np(0, "CPSWHwErrorFxn");

    g_cpsw_recovery_channel_id = ChannelCreate(0);
    g_cpsw_recovery_connection_id = ConnectAttach(ND_LOCAL_NODE, 0, g_cpsw_recovery_channel_id,
                                                _NTO_SIDE_CHANNEL, 0);

    wtp->quiesce_callout = cpsw_recovery_thread_quiesce;
    wtp->quiesce_arg = (void *)cpsw;

    return EOK;
}

/* Note: this must be called from a io-pkt tracked thread (created with nw_pthread_create())*/
uint32_t cpsw_recovery_teardown(struct ifnet *ifp, struct cpsw_dev *cpsw)
{
    struct nw_work_thread *wtp  = WTP;
    uint32_t ret;

    /* Lock out transmit */
    NW_SIGLOCK_P (&ifp->if_snd_ex, cpsw->sc_iopkt, wtp);
    ifp->if_flags_tx |= IFF_OACTIVE;

    if(g_cpsw->ipaddr[0] != 0x0 && g_cpsw->ipaddr[1] != 0x0 &&
       g_cpsw->ipaddr[2] != 0x0 && g_cpsw->ipaddr[3] != 0x0)
    {
        EnetIf_UnregisterIPv4Address(g_cpsw->ipaddr);
    }

    /* Clear free packets as this might take a little bit of time */
    ret = EnetIf_retrieveFreeTxPkts();

    /* Close CPSW DMA driver */
    EnetIf_closeDma();

    /* Send teardown completion message */
    slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s: Sending teardown notification", __FUNCTION__);
    ret = EnetIf_sendTeardownCompletion();
    if (ret == ENET_SOK)
    {
        slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s: CPSW teardown notification sent successfully", __FUNCTION__);
    }

    return ret;
}

// Note: this must be called from a io-pkt tracked thread (created with nw_pthread_create())
uint32_t cpsw_recovery_reconnect(struct ifnet *ifp, struct cpsw_dev *cpsw)
{
    struct nw_work_thread *wtp  = WTP;
    int32_t ret;

    // Re-open DMA
    ret = EnetIfMem_init();
    if(ret != ENET_SOK)
    {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Error: Problem with mem_init", __FUNCTION__, __LINE__);
    }

    ret = EnetIf_openDma();
    if(ret != ENET_SOK)
    {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Error: Problem reopening DMA", __FUNCTION__, __LINE__);
    }

    if(g_cpsw->ipaddr[0] != 0x0 && g_cpsw->ipaddr[1] != 0x0 &&
       g_cpsw->ipaddr[2] != 0x0 && g_cpsw->ipaddr[3] != 0x0)
    {
        EnetIf_RegisterIPv4Address(g_cpsw->ipaddr);
    }

    // Release transmit
    ifp->if_flags_tx &= ~IFF_OACTIVE;
    NW_SIGUNLOCK_P (&ifp->if_snd_ex, cpsw->sc_iopkt, wtp);

    return ret;
}

void *cpsw_recovery_thread(void *arg)
{
    struct cpsw_dev       *cpsw = (struct cpsw_dev *)arg;
    struct ifnet          *ifp  = &cpsw->sc_ec.ec_if;

    int32_t ret = EOK;
    struct _pulse pulse;
    int8_t recovery_code;
    int die;

    // TODO:: have an exit crietria for this thread
    while(1)
    {
        ret = MsgReceivePulse(g_cpsw_recovery_channel_id, &pulse, sizeof(pulse), NULL);
        if (ret != EOK)
        {
            slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Error: CPSW Recovery MsgReceivePulse error", __FUNCTION__, __LINE__);
            continue;
        }

        recovery_code = (int32_t)pulse.code;
        switch(recovery_code)
        {
            case CPSW_RECOVERY_MSG_QUIESCE:
                die = pulse.value.sival_int;
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d Recieved quiesce die=%d", __FUNCTION__, __LINE__, die);
                quiesce_block(die);
                break;
            case CPSW_RECOVERY_MSG_TEARDOWN:
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d Recieved start teardown", __FUNCTION__, __LINE__);
                cpsw_recovery_teardown(ifp, cpsw);
                break;
            case CPSW_RECOVERY_MSG_RECOVERY_COMPLETE:
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d Recieved teardown complete", __FUNCTION__, __LINE__);
                cpsw_recovery_reconnect(ifp, cpsw);
                break;
            default:
                slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Error: CPSW Recovery MsgReceivePulse unknown pulse code %d", __FUNCTION__, __LINE__, recovery_code);
                break;
        }
    }

    return NULL;
}
#endif
