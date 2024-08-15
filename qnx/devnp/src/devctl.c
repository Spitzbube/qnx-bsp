/*
 * $QNXLicenseC:
 * Copyright 2019, QNX Software Systems.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as
 * contributors under the License or as licensors under other terms.
 * Please review this entire file for other proprietary rights or license
 * notices, as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

/*
 * Modfications copyright (c) 2019-2021, Texas Instruments Incorporated
 *
 */

#include    <j7_cpsw.h>
#include    <net/ifdrvcom.h>
#include    <sys/sockio.h>
#include    <netdrvr/ptp.h>

#include "enetlld_if.h"


pthread_mutex_t ts_mutex = PTHREAD_MUTEX_INITIALIZER;

/*****************************************************************************/
/* cpsw_ptp_ioctl                                                            */
/*****************************************************************************/
static int cpsw_ptp_ioctl (struct cpsw_dev *cpsw, struct ifdrv *ifd)
{
    ptp_time_t      time;
    ptp_comp_t      comp;
    ptp_extts_t     ts;
    uint8_t         tx;
    uint64_t        time_ns;
    int32_t         nudge;
    int32_t         status;

    if (!cpsw->ptp_enable) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d PTP not enabled using driver start", __FUNCTION__, __LINE__);
        return EINVAL;
    }

    if (ifd != NULL) {
        switch(ifd->ifd_cmd) {

        case PTP_GET_TX_TIMESTAMP:
        case PTP_GET_RX_TIMESTAMP:
            if (ifd->ifd_len != sizeof(ts)) {
                return EINVAL;
            }

            if (ISSTACK) {
                if (copyin((((uint8_t *)ifd) + sizeof(*ifd)),
                    &ts, sizeof(ts))) {
                    return EINVAL;
                }
            } else {
                memcpy(&ts, (((uint8_t *)ifd) + sizeof(*ifd)), sizeof(ts));
            }

            if (ifd->ifd_cmd == PTP_GET_TX_TIMESTAMP) {
                tx = 1;
            } else {
                tx = 0;
            }
            if (cpsw->cfg.verbose & DEBUG_PTP) {
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d PTP_GET_TX_TIMESTAMP/PTP_GET_RX_TIMESTAMP",
                        __FUNCTION__, __LINE__);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d msg_type:%d, sequence_id:%d, tx:%d",
                        __FUNCTION__, __LINE__, ts.msg_type, ts.sequence_id, tx);
            }
            status = EnetIf_GetTimestamp(ts.msg_type, ts.sequence_id, tx, &time_ns);
            if (status != EOK)
            {
                ts.ts.sec = 0;
                ts.ts.nsec = 0;
            }
            else
            {
                ts.ts.sec = time_ns / (1000LL * 1000LL * 1000LL);
                ts.ts.nsec = time_ns % (1000LL * 1000LL * 1000LL);
            }

            if (cpsw->cfg.verbose & DEBUG_PTP) {
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d time_ns:%lud, ts.sec:%d, ts.nsec:%d",
                        __FUNCTION__, __LINE__, time_ns, ts.ts.sec, ts.ts.nsec);
            }

            if (ISSTACK) {
                return (copyout(&ts, (((uint8_t *)ifd) + sizeof(*ifd)),
                    sizeof(ts)));
            } else {
                memcpy((((uint8_t *)ifd) + sizeof(*ifd)), &ts, sizeof(ts));
                return EOK;
            }
            break;

        case PTP_GET_TIME:
            if (ifd->ifd_len != sizeof(time)) {
                return EINVAL;
            }
            pthread_mutex_lock(&ts_mutex);
            time_ns = 0;
            status = EnetIf_GetTime(&time_ns);
            if (status != EOK)
            {
                time.sec = 0;
                time.nsec = 0;
            }
            else
            {
                time.sec = (int32_t) (time_ns / (1000LL * 1000LL * 1000LL));
                time.nsec = (int32_t) (time_ns % (1000LL * 1000LL * 1000LL));
            }
            pthread_mutex_unlock(&ts_mutex);

            if (cpsw->cfg.verbose & DEBUG_PTP) {
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d PTP_GET_TIME",
                        __FUNCTION__, __LINE__);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d time_ns:%lud, ts.sec:%d, ts.nsec:%d",
                        __FUNCTION__, __LINE__, time_ns, time.sec, time.nsec);
            }

            if (ISSTACK) {
                return (copyout(&time, (((uint8_t *)ifd) + sizeof(*ifd)),
                    sizeof(time)));
            } else {
                memcpy((((uint8_t *)ifd) + sizeof(*ifd)), &time, sizeof(time));
                return EOK;
            }
            break;

        case PTP_SET_TIME:
            if (ifd->ifd_len != sizeof(time)) {
                return EINVAL;
            }
            if (ISSTACK) {
                if (copyin((((uint8_t *)ifd) + sizeof(*ifd)),
                    &time, sizeof(time))) {
                return EINVAL;
                }
            } else {
                memcpy(&time, (((uint8_t *)ifd) + sizeof(*ifd)), sizeof(time));
            }

            time_ns = (time.sec * 1000LL * 1000LL * 1000LL) + time.nsec;
            if (cpsw->cfg.verbose & DEBUG_PTP) {
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d PTP_SET_TIME",
                        __FUNCTION__, __LINE__);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d time_ns:%lud, ts.sec:%d, ts.nsec:%d",
                        __FUNCTION__, __LINE__, time_ns, time.sec, time.nsec);
            }

            pthread_mutex_lock(&ts_mutex);
            status = EnetIf_SetTime(time_ns);
            pthread_mutex_unlock(&ts_mutex);
            return EOK;
            break;

        case PTP_SET_COMPENSATION:
            if (ifd->ifd_len != sizeof(comp)) {
                return EINVAL;
            }
            if (ISSTACK) {
                if (copyin((((uint8_t *)ifd) + sizeof(*ifd)),
                    &comp, sizeof(comp))) {
                return EINVAL;
                }
            } else {
                memcpy(&comp, (((uint8_t *)ifd) + sizeof(*ifd)), sizeof(comp));
            }

            if (comp.positive)
                nudge = comp.comp;
            else
                nudge = -(comp.comp);

            if (cpsw->cfg.verbose & DEBUG_PTP) {
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d PTP_SET_COMPENSATION",
                        __FUNCTION__, __LINE__);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d positive:%d, comp:%d",
                        __FUNCTION__, __LINE__, comp.positive, comp.comp);
            }
            status = EnetIf_SetCompensation(nudge);
            return EOK;
            break;

        default:
            slogf(_SLOGC_NETWORK, _SLOG_ERROR, "Unknown PTP ioctl 0x%lx", ifd->ifd_cmd);
            break;
        }
    }
    return EINVAL;
}


/*****************************************************************************/
void cpsw_add_multicast_vlan_entry(struct cpsw_dev *cpsw, uint16_t vlan, uint8_t *addr)
{
    uint8_t i;
    struct cpsw_vlan_multicast *vlan_multicast_entry = NULL;

    for (i = 0; i < MAC_VLAN_SUPPORTED; i++) {
        if ((cpsw->vlan_multicast[i].vlanId == 0) && (cpsw->vlan_multicast[i].multi_mac_entries == 0)) {
            cpsw->vlan_multicast[i].vlanId = vlan;
            vlan_multicast_entry = &cpsw->vlan_multicast[i];
            break;
        }
        else if (cpsw->vlan_multicast[i].vlanId == vlan) {
            vlan_multicast_entry = &cpsw->vlan_multicast[i];
            break;
        }
    }

    if (vlan_multicast_entry != NULL) {
        for (i = 0; i < MAX_MULITCAST_MAC_ENTRY; i++) {
            if (vlan_multicast_entry->multi_mac_entry[i][0] == 0) {
                EnetIf_AleAddVlanMcast(vlan, addr);
                memcpy(vlan_multicast_entry->multi_mac_entry[i], addr, ETH_MAC_ADDR_LEN);
                vlan_multicast_entry->multi_mac_entries ++;
                break;
            }
        }
    }
}

void cpsw_add_multicast_entry(struct cpsw_dev *cpsw, uint16_t vlan, uint8_t *addr)
{
    uint8_t i;

    if (vlan == 0) {
        for (i = 0; i < MAX_MULITCAST_MAC_ENTRY; i++) {
            if (cpsw->multi_mac_entry[i][0] == 0) {
                EnetIf_AleAddVlanMcast(vlan, addr);
                memcpy(cpsw->multi_mac_entry[i], addr, ETH_MAC_ADDR_LEN);
                cpsw->multi_mac_entries ++;
                break;
            }
         }
     }
     else {
        cpsw_add_multicast_vlan_entry(cpsw, vlan, addr);
     }
}

void cpsw_del_multicast_vlan_entry_all(struct cpsw_dev *cpsw, uint16_t vlan)
{
    uint8_t i;
    struct cpsw_vlan_multicast *vlan_multicast_entry = NULL;

    for (i = 0; i < MAC_VLAN_SUPPORTED; i++) {
        if (cpsw->vlan_multicast[i].vlanId == vlan) {
            vlan_multicast_entry = &cpsw->vlan_multicast[i];
            break;
        }
    }

    if (vlan_multicast_entry != NULL) {
        for (i = 0; i < MAX_MULITCAST_MAC_ENTRY; i++)
        {
            if (vlan_multicast_entry->multi_mac_entry[i][0] != 0)
            {
                EnetIf_AleDelVlanMcast(vlan, vlan_multicast_entry->multi_mac_entry[i]);
                memset(vlan_multicast_entry->multi_mac_entry[i], 0, ETH_MAC_ADDR_LEN);
            }
        }
        vlan_multicast_entry->multi_mac_entries = 0;
        vlan_multicast_entry->vlanId = 0;
    }

}

void cpsw_del_multicast_entry_all(struct cpsw_dev *cpsw, uint16_t vlan)
{
    uint8_t i;
    if (vlan == 0) {
        for (i = 0; i < MAX_MULITCAST_MAC_ENTRY; i++)
        {
            if (cpsw->multi_mac_entry[i][0] != 0)
            {
                EnetIf_AleDelVlanMcast(vlan, cpsw->multi_mac_entry[i]);
                memset(cpsw->multi_mac_entry[i], 0, ETH_MAC_ADDR_LEN);
            }
        }
        cpsw->multi_mac_entries = 0;
    }
    else {
        cpsw_del_multicast_vlan_entry_all(cpsw, vlan);
    }
}


/*****************************************************************************/
/* cpsw_filter                                                               */
/*****************************************************************************/
static void cpsw_filter(struct cpsw_dev *cpsw)
{
    struct ethercom         *ec;
    struct ether_multi      *enm;
    struct ether_multistep  step;
    struct ifnet            *ifp;
    uint16_t                vlan;
    int                     loop;

    vlan = 0;

    ec = &cpsw->sc_ec;
    ifp = &ec->ec_if;

    ifp->if_flags &= ~IFF_ALLMULTI;
    slogf (_SLOGC_NETWORK, _SLOG_INFO, "%s: Clear IFF_ALLMULTI", __FUNCTION__);
    EnetIf_AleFloodUnregMcast(vlan, 0);
    cpsw_del_multicast_entry_all(cpsw, vlan);
    if (cpsw->join_vlan != NULL) {
        loop = 0;
        while (cpsw->join_vlan[loop] != 0) {
            EnetIf_AleFloodUnregMcast(cpsw->join_vlan[loop], 0);
            cpsw_del_multicast_entry_all(cpsw, cpsw->join_vlan[loop]);
            loop++;
        }
    }

    ETHER_FIRST_MULTI (step, ec, enm);
    while (enm != NULL) {
        if (memcmp (enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN) != 0) {
            ifp->if_flags |= IFF_ALLMULTI;
            slogf (_SLOGC_NETWORK, _SLOG_INFO, "%s:Set IFF_ALLMULTI", __FUNCTION__);
            /*
             * Change port vlan entry to flood all multicast
             * Del all programmed mcast for this port
             */
            cpsw_del_multicast_entry_all(cpsw, vlan);
            EnetIf_AleFloodUnregMcast(vlan, 1);
            if (cpsw->join_vlan != NULL) {
                loop = 0;
                while (cpsw->join_vlan[loop] != 0) {
                    cpsw_del_multicast_entry_all(cpsw, cpsw->join_vlan[loop]);
                    EnetIf_AleFloodUnregMcast(cpsw->join_vlan[loop], 1);
                    loop++;
                }
            }
            break;
        }

        slogf (_SLOGC_NETWORK, _SLOG_INFO, "%s: Adding Multicast MAC Addr -> %02x:%02x:%02x:%02x:%02x:%02x", __FUNCTION__,
            enm->enm_addrlo[0], enm->enm_addrlo[1], enm->enm_addrlo[2], enm->enm_addrlo[3], enm->enm_addrlo[4], enm->enm_addrlo[5]);
        cpsw_add_multicast_entry(cpsw, vlan, enm->enm_addrlo);
        if (cpsw->join_vlan != NULL) {
            loop = 0;
            while (cpsw->join_vlan[loop] != 0) {
                cpsw_add_multicast_entry(cpsw, cpsw->join_vlan[loop], enm->enm_addrlo);
                loop++;
            }
        }

        ETHER_NEXT_MULTI (step, enm);
    }
}

/*****************************************************************************/
/* cpsw_ioctl                                                                */
/*****************************************************************************/
int     cpsw_ioctl (struct ifnet * ifp, unsigned long cmd, caddr_t data)

{
    int                     error = 0;
    struct cpsw_dev         *cpsw = ifp->if_softc;
    struct drvcom_config    *dcfgp;
    struct drvcom_stats     *dstp;
    struct ifdrv_com        *ifdc;
    struct ifdrv            *ifd;

    if (cpsw->cfg.verbose & DEBUG_TRACE) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Came here -->cmd-0x%x", __FUNCTION__, __LINE__, (unsigned)cmd);
    }

    switch (cmd) {
        case    SIOCGDRVCOM:
            ifdc = (struct ifdrv_com *)data;
            switch (ifdc->ifdc_cmd) {
                case    DRVCOM_CONFIG:
                    dcfgp = (struct drvcom_config *)ifdc;

                    if (ifdc->ifdc_len != sizeof(nic_config_t)) {
                        error = EINVAL;
                        break;
                    }
                    memcpy(&dcfgp->dcom_config, &cpsw->cfg, sizeof(cpsw->cfg));
                    break;

                case    DRVCOM_STATS:
                    dstp = (struct drvcom_stats *)ifdc;

                    if (ifdc->ifdc_len != sizeof(nic_stats_t)) {
                        error = EINVAL;
                        break;
                    }
                    EnetIf_GetStats(&cpsw->stats);
                    memcpy(&dstp->dcom_stats, &cpsw->stats, sizeof(cpsw->stats));
                    break;

                default:
                    error = ENOTTY;
                }
            break;


        case    SIOCSIFMEDIA:
        case    SIOCGIFMEDIA: {
            struct ifreq *ifr = (struct ifreq *)data;

            error = ifmedia_ioctl(ifp, ifr, &cpsw->bsd_mii.mii_media, cmd);
            break;
            }

        case SIOCSDRVSPEC:
        case SIOCGDRVSPEC:
            ifd = (struct ifdrv *)data;
            switch (ifd->ifd_cmd) {
                case PTP_GET_TX_TIMESTAMP:
                case PTP_GET_RX_TIMESTAMP:
                case PTP_GET_TIME:
                case PTP_SET_TIME:
                case PTP_SET_COMPENSATION:
                    error = cpsw_ptp_ioctl(cpsw, ifd);
                    break;
                case DUMP_PORT_STATS:
                    EnetIf_showStats();
                    break;
                case DUMP_PHY_REG:
                    EnetIf_ShowPhyRegs();
                    break;
                case DUMP_ALE_ENTRIES:
                    EnetIf_AleDumpTable();
                    break;
                case DUMP_POLICER_ENTRIES:
                    EnetIf_AleDumpPolicer();
                    break;
            }
            break;

        default:
            error = ether_ioctl(ifp, cmd, data);
            if (error == ENETRESET) {
                /*
                 * Multicast list has changed; set the
                 * hardware filter accordingly.
                 */
               if ((ifp->if_flags_tx & IFF_RUNNING) == 0) {
                    /* Interface is currently down */
                } else {
                    cpsw_filter(cpsw);
                }
                error = 0;
            }
            break;
        }

    if (cpsw->cfg.verbose & DEBUG_TRACE) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d <--", __FUNCTION__, __LINE__);
    }

    return error;
}

