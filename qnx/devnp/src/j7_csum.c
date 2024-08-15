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

#include "bpfilter.h"


#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <net/if_vlanvar.h>

#include <j7_cpsw.h>

/*****************************************************************************/
/* perform hardware checksum offload setup as instructed by stack            */
/*****************************************************************************/
int cpsw_csum_offload_setup (struct cpsw_dev *cpsw,
                             struct mbuf *m0,
                             int offload_flags,
                             uint32_t *csInsertPos,
                             uint32_t *csStartPos,
                             uint32_t *csNumBytes)
{
    int                     mac_offset=0;   // offset into mbuf, past ethernet mac header
    int                     mac_hdrlen=0;   // length of ethernet mac header
    int                     ip_hdrlen=0;    // length of IP header
    struct ether_header     *eh;
    struct mbuf             *m;
    uint16_t                etype=0;

    //
    // get the IP header length and figure out the minimum IP/TCP header
    // length which will allow us to fudge the TCP header
    //
    if (offload_flags & (M_CSUM_TCPv4 | M_CSUM_UDPv4)) {
        ip_hdrlen  = M_CSUM_DATA_IPv4_IPHL (m0->m_pkthdr.csum_data);
    }
    else if (offload_flags & (M_CSUM_TCPv6 | M_CSUM_UDPv6)) {
        ip_hdrlen = M_CSUM_DATA_IPv6_HL (m0->m_pkthdr.csum_data);
    }
    else {
        slogf (_SLOGC_NETWORK, _SLOG_ERROR, "%s(): unsupported hardware offload flags: 0x%X",
                __FUNCTION__, offload_flags);
        return -1;
    }

    /* Ethernet header will always be in first frag */
    eh = mtod (m0, struct ether_header *);

    /* Get in host order */
    etype = ENDIAN_BE16 (eh->ether_type);

    // how long is the ethernet mac headers?
    switch (etype) {
        case    ETHERTYPE_IP:
        case    ETHERTYPE_IPV6:
            mac_offset = mac_hdrlen = ETHER_HDR_LEN;
            break;

        case    ETHERTYPE_VLAN:
            mac_offset = mac_hdrlen = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
            break;

        default:
            slogf (_SLOGC_NETWORK, _SLOG_ERROR, "%s(): hw csum unsupported etype 0x%X",
                    __FUNCTION__, etype);
            return -1;
            break;
        }

    //
    // skip over the 14 byte ethernet header (18 if vlan)
    //
    for (m=m0; m && (m->m_len <= mac_offset); m=m->m_next) {
        mac_offset -= m->m_len;
    }
    if (!m) {
        // ridiculously short packet - should never happen
        slogf (_SLOGC_NETWORK, _SLOG_ERROR, "%s(): dropped short packet: mac_offset %d  mac_hdrlen %d",
                    __FUNCTION__, mac_offset, mac_hdrlen);
        return -1;
    }

    //
    // TCP/UDP checksum offload setup
    //
    if (offload_flags & (M_CSUM_TCPv4 | M_CSUM_UDPv4)) {
        int hdr_offset = mac_hdrlen + ip_hdrlen;

        /* Start byte for HW to begin CS computation */
        *csStartPos = 1 + hdr_offset;
        /* Byte offset where HW should insert CS */
        *csInsertPos = 1 + hdr_offset + M_CSUM_DATA_IPv4_OFFSET (m0->m_pkthdr.csum_data);
        /* Number of bytes to CS */
        *csNumBytes = m0->m_pkthdr.len - hdr_offset;

    }
    else if (offload_flags & (M_CSUM_TCPv6 | M_CSUM_UDPv6)) {
        int hdr_offset = mac_hdrlen + ip_hdrlen;

        /* Start byte for HW to begin CS computation */
        *csStartPos = 1 + hdr_offset;
        /* Byte offset where HW should insert CS */
        *csInsertPos = 1 + hdr_offset +  M_CSUM_DATA_IPv6_OFFSET (m0->m_pkthdr.csum_data);
        /* Number of bytes to CS */
        *csNumBytes = m0->m_pkthdr.len - hdr_offset;

    }

    return 0;  // worked ok
}
