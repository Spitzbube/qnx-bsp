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

#include <j7_cpsw.h>
#include <sys/malloc.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <device_qnx.h>


#include <sys/mman.h>


//
// this is a callback, made by the bsd media code.  We passed
// a pointer to this function during the ifmedia_init() call
// in bsd_mii_initmedia()
//
void
bsd_mii_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
    struct cpsw_dev *cpsw = ifp->if_softc;

    if (cpsw->cfg.verbose & DEBUG_TRACE) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Came here -->", __FUNCTION__, __LINE__);
    }

    cpsw->bsd_mii.mii_media_active = IFM_ETHER;
    cpsw->bsd_mii.mii_media_status = IFM_AVALID;

    if (cpsw->force_link) {

        if (cpsw->linkup) {
            cpsw->bsd_mii.mii_media_status |= IFM_ACTIVE;
        }

        // report back the previously forced values
        switch(cpsw->cfg.media_rate) {
            case 0:
            cpsw->bsd_mii.mii_media_active |= IFM_NONE;
            break;

            case 1000*10:
            cpsw->bsd_mii.mii_media_active |= IFM_10_T;
            break;

            case 1000*100:
            cpsw->bsd_mii.mii_media_active |= IFM_100_TX;
            break;

            case 1000*1000:
            cpsw->bsd_mii.mii_media_active |= IFM_1000_T;
            break;

            default:    // this shouldnt really happen, but ...
            cpsw->bsd_mii.mii_media_active |= IFM_NONE;
            break;
        }
        if (cpsw->cfg.duplex) {
            cpsw->bsd_mii.mii_media_active |= IFM_FDX;
        }

        // we dont set flow ctrl bits in mii_media_active

    } else if (cpsw->linkup) {  // link is auto-detect and up

        cpsw->bsd_mii.mii_media_status |= IFM_ACTIVE;

        switch(cpsw->cfg.media_rate) {
            case 1000*10:
            cpsw->bsd_mii.mii_media_active |= IFM_10_T;
            break;

            case 1000*100:
            cpsw->bsd_mii.mii_media_active |= IFM_100_TX;
            break;

            case 1000*1000:
            cpsw->bsd_mii.mii_media_active |= IFM_1000_T;
            break;

            default:    // this shouldnt really happen, but ...
            cpsw->bsd_mii.mii_media_active |= IFM_NONE;
            break;
        }

        if (cpsw->cfg.duplex) {
            cpsw->bsd_mii.mii_media_active |= IFM_FDX;
        }
#if 0
        // these media state variables are set by the link interrupt
        if (cpsw->pause_xmit || cpsw->pause_receive) {
            cpsw->bsd_mii.mii_media_active |= IFM_FLOW;

            if (cpsw->pause_xmit) {
                cpsw->bsd_mii.mii_media_active |= IFM_ETH_TXPAUSE;
            }

            if (cpsw->pause_receive) {
                cpsw->bsd_mii.mii_media_active |= IFM_ETH_RXPAUSE;
            }
        }
#endif
        // could move this to event.c so there was no lag
        ifmedia_set(&cpsw->bsd_mii.mii_media, IFM_ETHER|IFM_AUTO);

    } else {    // link is auto-detect and down
        cpsw->bsd_mii.mii_media_active |= IFM_NONE;
        cpsw->bsd_mii.mii_media_status = 0;

        // could move this to event.c so there was no lag
        ifmedia_set(&cpsw->bsd_mii.mii_media, IFM_ETHER|IFM_NONE);
    }

    // stuff parameter values with hoked-up bsd values
    ifmr->ifm_status = cpsw->bsd_mii.mii_media_status;
    ifmr->ifm_active = cpsw->bsd_mii.mii_media_active;
}


//
// this is a callback, made by the bsd media code.  We passed
// a pointer to this function during the ifmedia_init() call
// in bsd_mii_initmedia().  This function is called when
// someone makes an ioctl into us, we call into the generic
// ifmedia source, and it make this callback to actually
// force the speed and duplex, just as if the user had
// set the cmd line options
//
int
bsd_mii_mediachange(struct ifnet *ifp)
{
    struct cpsw_dev *cpsw           = ifp->if_softc;
    int             old_media_rate  = cpsw->cfg.media_rate;
    int             old_duplex      = cpsw->cfg.duplex;
    int             old_force_link  = cpsw->force_link;
    struct ifmedia  *ifm            = &cpsw->bsd_mii.mii_media;
    int             user_duplex     = ifm->ifm_media & IFM_FDX ? 1 : 0;
    int             user_media      = ifm->ifm_media & IFM_TMASK;

    if (cpsw->cfg.verbose & DEBUG_TRACE) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Came here -->", __FUNCTION__, __LINE__);
    }

    if (!(ifp->if_flags & IFF_UP)) {
        slogf(_SLOGC_NETWORK, _SLOG_WARNING,
          "%s(): cpsw interface isn't up, ioctl ignored", __FUNCTION__);
        return 0;
    }

    if (!(ifm->ifm_media & IFM_ETHER)) {
        slogf(_SLOGC_NETWORK, _SLOG_WARNING,
          "%s(): cpsw interface - bad media: 0x%X",
          __FUNCTION__, ifm->ifm_media);
        return 0;   // should never happen
    }

    switch (user_media) {
        case IFM_AUTO:      // auto-select media
        cpsw->force_link      =  0;
        cpsw->cfg.media_rate    = -1;
        cpsw->cfg.duplex        = -1;
        ifmedia_set(&cpsw->bsd_mii.mii_media, IFM_ETHER|IFM_AUTO);
        break;

        case IFM_NONE:      // disable media
        //
        // forcing the link with a speed of zero means to disable the link
        //
        cpsw->force_link        = 1;
        cpsw->cfg.media_rate    = 0;
        cpsw->cfg.duplex        = 0;
        ifmedia_set(&cpsw->bsd_mii.mii_media, IFM_ETHER|IFM_NONE);
        break;

        case IFM_10_T:      // force 10baseT
        cpsw->force_link        = 1;
        cpsw->cfg.media_rate    = 10 * 1000;
        cpsw->cfg.duplex        = user_duplex;
        ifmedia_set(&cpsw->bsd_mii.mii_media,
          user_duplex ? IFM_ETHER|IFM_10_T|IFM_FDX : IFM_ETHER|IFM_10_T);
        break;

        case IFM_100_TX:    // force 100baseTX
        cpsw->force_link        = 1;
        cpsw->cfg.media_rate    = 100 * 1000;
        cpsw->cfg.duplex        = user_duplex;
        ifmedia_set(&cpsw->bsd_mii.mii_media,
          user_duplex ? IFM_ETHER|IFM_100_TX|IFM_FDX : IFM_ETHER|IFM_100_TX);
        break;

        case IFM_1000_T:    // force 1000baseT
        //
        // N.B.  I have not had good luck, trying to get gige to work half
        // duplex.  Even with different gige switches, I can only force full duplex
        //
        cpsw->force_link        = 1;
        cpsw->cfg.media_rate    = 1000 * 1000;
        cpsw->cfg.duplex        = user_duplex;
        ifmedia_set(&cpsw->bsd_mii.mii_media,
          user_duplex ? IFM_ETHER|IFM_1000_T|IFM_FDX : IFM_ETHER|IFM_1000_T);
        break;

        default:            // should never happen
        slogf(_SLOGC_NETWORK, _SLOG_WARNING,
          "%s(): cpsw interface - unknown media: 0x%X",
          __FUNCTION__, user_media);
        return 0;
        break;
    }

    // does the user want something different than it already is?
    if ((cpsw->cfg.media_rate != old_media_rate)    ||
        (cpsw->cfg.duplex     != old_duplex)        ||
        (cpsw->force_link     != old_force_link)    ||
        (cpsw->cfg.flags      &  NIC_FLAG_LINK_DOWN) ) {

        // re-initialize hardware with new parameters
        ifp->if_init(ifp);

    }



    return 0;
}


//
// called from cpsw_attach() in init.c to hook up
// to the bsd media structure.  Not entirely unlike kissing
// a porcupine, we must do so carefully, because we do not
// want to use the bsd mii management structure, because
// this driver uses link interrupt
//
void
bsd_mii_initmedia(struct cpsw_dev *cpsw)
{

    if (cpsw->cfg.verbose & DEBUG_TRACE) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Came here -->", __FUNCTION__, __LINE__);
    }

    cpsw->bsd_mii.mii_ifp = &cpsw->sc_ec.ec_if;

    ifmedia_init(&cpsw->bsd_mii.mii_media, IFM_IMASK, bsd_mii_mediachange,
      bsd_mii_mediastatus);

    // we do NOT call mii_attach() - we do our own link management

    //
    // must create these entries to make ifconfig media work
    // see lib/socket/public/net/if_media.h for defines
    //

    // ifconfig wm0 none (x22)
    ifmedia_add(&cpsw->bsd_mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);

    // ifconfig wm0 auto (x20)
    ifmedia_add(&cpsw->bsd_mii.mii_media, IFM_ETHER|IFM_AUTO, 0, NULL);

    // ifconfig wm0 10baseT (x23 - half duplex)
    ifmedia_add(&cpsw->bsd_mii.mii_media, IFM_ETHER|IFM_10_T, 0, NULL);

    // ifconfig wm0 10baseT-FDX (x100023)
    ifmedia_add(&cpsw->bsd_mii.mii_media, IFM_ETHER|IFM_10_T|IFM_FDX, 0, NULL);

    // ifconfig wm0 100baseTX (x26 - half duplex)
    ifmedia_add(&cpsw->bsd_mii.mii_media, IFM_ETHER|IFM_100_TX, 0, NULL);

    // ifconfig wm0 100baseTX-FDX (x100026 - full duplex)
    ifmedia_add(&cpsw->bsd_mii.mii_media, IFM_ETHER|IFM_100_TX|IFM_FDX, 0, NULL);

    // ifconfig wm0 1000baseT (x30 - half duplex)
    ifmedia_add(&cpsw->bsd_mii.mii_media, IFM_ETHER|IFM_1000_T, 0, NULL);

    // ifconfig wm0 1000baseT mediaopt fdx (x100030 - full duplex)
    ifmedia_add(&cpsw->bsd_mii.mii_media, IFM_ETHER|IFM_1000_T|IFM_FDX, 0, NULL);

    // add more entries to support flow control via ifconfig media

    // link is initially down
    ifmedia_set(&cpsw->bsd_mii.mii_media, IFM_ETHER|IFM_NONE);
}

