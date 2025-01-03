// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QNETWORKINTERFACE_UIKIT_P_H
#define QNETWORKINTERFACE_UIKIT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

// Platform SDK for iOS, tvOS and watchOS is missing those headers:
// net/if_media.h, netinet/in_var.h, netinet6/in6_var.h This header is
// a workaround, it provides missing macros and structs.

// <net/if_media.h>:

/*
 * Ethernet
 */
#define IFM_ETHER 0x00000020
/*
 * FDDI
 */
#define IFM_FDDI 0x00000060
/*
 * IEEE 802.11 Wireless
 */
#define IFM_IEEE80211 0x00000080
/*
 * Masks
 */
#define IFM_NMASK 0x000000e0 /* Network type */
/*
 * Macros to extract various bits of information from the media word.
 */
#define IFM_TYPE(x) ((x) & IFM_NMASK)

// <netinet6/in6_var.h>:

struct in6_addrlifetime {
    time_t ia6t_expire;    /* valid lifetime expiration time */
    time_t ia6t_preferred;    /* preferred lifetime expiration time */
    u_int32_t ia6t_vltime;    /* valid lifetime */
    u_int32_t ia6t_pltime;    /* prefix lifetime */
};

/*
 * IPv6 interface statistics, as defined in RFC2465 Ipv6IfStatsEntry (p12).
 */
struct in6_ifstat {
    u_quad_t ifs6_in_receive;    /* # of total input datagram */
    u_quad_t ifs6_in_hdrerr;    /* # of datagrams with invalid hdr */
    u_quad_t ifs6_in_toobig;    /* # of datagrams exceeded MTU */
    u_quad_t ifs6_in_noroute;    /* # of datagrams with no route */
    u_quad_t ifs6_in_addrerr;    /* # of datagrams with invalid dst */
    u_quad_t ifs6_in_protounknown;    /* # of datagrams with unknown proto */
                    /* NOTE: increment on final dst if */
    u_quad_t ifs6_in_truncated;    /* # of truncated datagrams */
    u_quad_t ifs6_in_discard;    /* # of discarded datagrams */
                    /* NOTE: fragment timeout is not here */
    u_quad_t ifs6_in_deliver;    /* # of datagrams delivered to ULP */
                    /* NOTE: increment on final dst if */
    u_quad_t ifs6_out_forward;    /* # of datagrams forwarded */
                    /* NOTE: increment on outgoing if */
    u_quad_t ifs6_out_request;    /* # of outgoing datagrams from ULP */
                    /* NOTE: does not include forwrads */
    u_quad_t ifs6_out_discard;    /* # of discarded datagrams */
    u_quad_t ifs6_out_fragok;    /* # of datagrams fragmented */
    u_quad_t ifs6_out_fragfail;    /* # of datagrams failed on fragment */
    u_quad_t ifs6_out_fragcreat;    /* # of fragment datagrams */
                    /* NOTE: this is # after fragment */
    u_quad_t ifs6_reass_reqd;    /* # of incoming fragmented packets */
                    /* NOTE: increment on final dst if */
    u_quad_t ifs6_reass_ok;        /* # of reassembled packets */
                    /* NOTE: this is # after reass */
                    /* NOTE: increment on final dst if */
    u_quad_t ifs6_atmfrag_rcvd;    /* # of atomic fragments received */
    u_quad_t ifs6_reass_fail;    /* # of reass failures */
                    /* NOTE: may not be packet count */
                    /* NOTE: increment on final dst if */
    u_quad_t ifs6_in_mcast;        /* # of inbound multicast datagrams */
    u_quad_t ifs6_out_mcast;    /* # of outbound multicast datagrams */

    u_quad_t ifs6_cantfoward_icmp6;    /* # of ICMPv6 packets received for unreachable dest */
    u_quad_t ifs6_addr_expiry_cnt;    /* # of address expiry events (excluding privacy addresses) */
    u_quad_t ifs6_pfx_expiry_cnt;    /* # of prefix expiry events */
    u_quad_t ifs6_defrtr_expiry_cnt;    /* # of default router expiry events */
};

/*
 * ICMPv6 interface statistics, as defined in RFC2466 Ipv6IfIcmpEntry.
 * XXX: I'm not sure if this file is the right place for this structure...
 */
struct icmp6_ifstat {
    /*
     * Input statistics
     */
    /* ipv6IfIcmpInMsgs, total # of input messages */
    u_quad_t ifs6_in_msg;
    /* ipv6IfIcmpInErrors, # of input error messages */
    u_quad_t ifs6_in_error;
    /* ipv6IfIcmpInDestUnreachs, # of input dest unreach errors */
    u_quad_t ifs6_in_dstunreach;
    /* ipv6IfIcmpInAdminProhibs, # of input admin. prohibited errs */
    u_quad_t ifs6_in_adminprohib;
    /* ipv6IfIcmpInTimeExcds, # of input time exceeded errors */
    u_quad_t ifs6_in_timeexceed;
    /* ipv6IfIcmpInParmProblems, # of input parameter problem errors */
    u_quad_t ifs6_in_paramprob;
    /* ipv6IfIcmpInPktTooBigs, # of input packet too big errors */
    u_quad_t ifs6_in_pkttoobig;
    /* ipv6IfIcmpInEchos, # of input echo requests */
    u_quad_t ifs6_in_echo;
    /* ipv6IfIcmpInEchoReplies, # of input echo replies */
    u_quad_t ifs6_in_echoreply;
    /* ipv6IfIcmpInRouterSolicits, # of input router solicitations */
    u_quad_t ifs6_in_routersolicit;
    /* ipv6IfIcmpInRouterAdvertisements, # of input router advertisements */
    u_quad_t ifs6_in_routeradvert;
    /* ipv6IfIcmpInNeighborSolicits, # of input neighbor solicitations */
    u_quad_t ifs6_in_neighborsolicit;
    /* ipv6IfIcmpInNeighborAdvertisements, # of input neighbor advs. */
    u_quad_t ifs6_in_neighboradvert;
    /* ipv6IfIcmpInRedirects, # of input redirects */
    u_quad_t ifs6_in_redirect;
    /* ipv6IfIcmpInGroupMembQueries, # of input MLD queries */
    u_quad_t ifs6_in_mldquery;
    /* ipv6IfIcmpInGroupMembResponses, # of input MLD reports */
    u_quad_t ifs6_in_mldreport;
    /* ipv6IfIcmpInGroupMembReductions, # of input MLD done */
    u_quad_t ifs6_in_mlddone;

    /*
     * Output statistics. We should solve unresolved routing problem...
     */
    /* ipv6IfIcmpOutMsgs, total # of output messages */
    u_quad_t ifs6_out_msg;
    /* ipv6IfIcmpOutErrors, # of output error messages */
    u_quad_t ifs6_out_error;
    /* ipv6IfIcmpOutDestUnreachs, # of output dest unreach errors */
    u_quad_t ifs6_out_dstunreach;
    /* ipv6IfIcmpOutAdminProhibs, # of output admin. prohibited errs */
    u_quad_t ifs6_out_adminprohib;
    /* ipv6IfIcmpOutTimeExcds, # of output time exceeded errors */
    u_quad_t ifs6_out_timeexceed;
    /* ipv6IfIcmpOutParmProblems, # of output parameter problem errors */
    u_quad_t ifs6_out_paramprob;
    /* ipv6IfIcmpOutPktTooBigs, # of output packet too big errors */
    u_quad_t ifs6_out_pkttoobig;
    /* ipv6IfIcmpOutEchos, # of output echo requests */
    u_quad_t ifs6_out_echo;
    /* ipv6IfIcmpOutEchoReplies, # of output echo replies */
    u_quad_t ifs6_out_echoreply;
    /* ipv6IfIcmpOutRouterSolicits, # of output router solicitations */
    u_quad_t ifs6_out_routersolicit;
    /* ipv6IfIcmpOutRouterAdvertisements, # of output router advs. */
    u_quad_t ifs6_out_routeradvert;
    /* ipv6IfIcmpOutNeighborSolicits, # of output neighbor solicitations */
    u_quad_t ifs6_out_neighborsolicit;
    /* ipv6IfIcmpOutNeighborAdvertisements, # of output neighbor advs. */
    u_quad_t ifs6_out_neighboradvert;
    /* ipv6IfIcmpOutRedirects, # of output redirects */
    u_quad_t ifs6_out_redirect;
    /* ipv6IfIcmpOutGroupMembQueries, # of output MLD queries */
    u_quad_t ifs6_out_mldquery;
    /* ipv6IfIcmpOutGroupMembResponses, # of output MLD reports */
    u_quad_t ifs6_out_mldreport;
    /* ipv6IfIcmpOutGroupMembReductions, # of output MLD done */
    u_quad_t ifs6_out_mlddone;
};

#define SCOPE6_ID_MAX 16

struct in6_ifreq {
    char    ifr_name[IFNAMSIZ];
    union {
        struct    sockaddr_in6 ifru_addr;
        struct    sockaddr_in6 ifru_dstaddr;
        int    ifru_flags;
        int    ifru_flags6;
        int    ifru_metric;
        int    ifru_intval;
        caddr_t    ifru_data;
        struct in6_addrlifetime ifru_lifetime;
        struct in6_ifstat ifru_stat;
        struct icmp6_ifstat ifru_icmp6stat;
        u_int32_t ifru_scope_id[SCOPE6_ID_MAX];
    } ifr_ifru;
};

#define IN6_IFF_TEMPORARY 0x0080 /* temporary (anonymous) address. */
#define IN6_IFF_DEPRECATED 0x0010 /* deprecated address */

#define SIOCGIFAFLAG_IN6 _IOWR('i', 73, struct in6_ifreq)
#define SIOCGIFALIFETIME_IN6 _IOWR('i', 81, struct in6_ifreq)

#endif // QNETWORKINTERFACE_UIKIT_P_H

