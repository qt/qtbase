/****************************************************************************
**
** Copyright (C) 2017 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qnetworkinterface.h"
#include "qnetworkinterface_p.h"
#include "qnetworkinterface_unix_p.h"

#ifndef QT_NO_NETWORKINTERFACE

#include <qendian.h>
#include <qobjectdefs.h>
#include <qvarlengtharray.h>

// accordding to rtnetlink(7)
#include <asm/types.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/wireless.h>
#include <sys/socket.h>

/* in case these aren't defined in linux/if_arp.h (added since 2.6.28)  */
#define ARPHRD_PHONET       820     /* v2.6.29: PhoNet media type */
#define ARPHRD_PHONET_PIPE  821     /* v2.6.29: PhoNet pipe header */
#define ARPHRD_IEEE802154   804     /* v2.6.31 */
#define ARPHRD_6LOWPAN      825     /* v3.14: IPv6 over LoWPAN */

QT_BEGIN_NAMESPACE

enum {
    BufferSize = 8192
};

static QNetworkInterface::InterfaceType probeIfType(int socket, struct ifreq *req, short arptype)
{
    switch (ushort(arptype)) {
    case ARPHRD_LOOPBACK:
        return QNetworkInterface::Loopback;

    case ARPHRD_ETHER:
        // check if it's a WiFi interface
        if (qt_safe_ioctl(socket, SIOCGIWMODE, req) >= 0)
            return QNetworkInterface::Wifi;
        return QNetworkInterface::Ethernet;

    case ARPHRD_SLIP:
    case ARPHRD_CSLIP:
    case ARPHRD_SLIP6:
    case ARPHRD_CSLIP6:
        return QNetworkInterface::Slip;

    case ARPHRD_CAN:
        return QNetworkInterface::CanBus;

    case ARPHRD_PPP:
        return QNetworkInterface::Ppp;

    case ARPHRD_FDDI:
        return QNetworkInterface::Fddi;

    case ARPHRD_IEEE80211:
    case ARPHRD_IEEE80211_PRISM:
    case ARPHRD_IEEE80211_RADIOTAP:
        return QNetworkInterface::Ieee80211;

    case ARPHRD_IEEE802154:
        return QNetworkInterface::Ieee802154;

    case ARPHRD_PHONET:
    case ARPHRD_PHONET_PIPE:
        return QNetworkInterface::Phonet;

    case ARPHRD_6LOWPAN:
        return QNetworkInterface::SixLoWPAN;

    case ARPHRD_TUNNEL:
    case ARPHRD_TUNNEL6:
    case ARPHRD_NONE:
    case ARPHRD_VOID:
        return QNetworkInterface::Virtual;
    }
    return QNetworkInterface::Unknown;
}


namespace {
struct NetlinkSocket
{
    int sock;
    NetlinkSocket(int bufferSize)
    {
        sock = qt_safe_socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
        if (Q_UNLIKELY(sock == -1))
            qErrnoWarning("Could not create AF_NETLINK socket");

        // set buffer length
        socklen_t len = sizeof(bufferSize);
        setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufferSize, len);
    }

    ~NetlinkSocket()
    {
        if (sock != -1)
            qt_safe_close(sock);
    }

    operator int() const { return sock; }
};

template <typename Lambda> struct ProcessNetlinkRequest
{
    using FunctionTraits = QtPrivate::FunctionPointer<decltype(&Lambda::operator())>;
    using FirstArgument = typename FunctionTraits::Arguments::Car;

    static int expectedTypeForRequest(int rtype)
    {
        Q_STATIC_ASSERT(RTM_NEWADDR == RTM_GETADDR - 2);
        Q_STATIC_ASSERT(RTM_NEWLINK == RTM_GETLINK - 2);
        Q_ASSERT(rtype == RTM_GETADDR || rtype == RTM_GETLINK);
        return rtype - 2;
    }

    void operator()(int sock, nlmsghdr *hdr, char *buf, size_t bufsize, Lambda &&func)
    {
        // send the request
        if (send(sock, hdr, hdr->nlmsg_len, 0) != ssize_t(hdr->nlmsg_len))
            return;

        // receive and parse the request
        int expectedType = expectedTypeForRequest(hdr->nlmsg_type);
        const bool isDump = hdr->nlmsg_flags & NLM_F_DUMP;
        forever {
            qsizetype len = recv(sock, buf, bufsize, 0);
            hdr = reinterpret_cast<struct nlmsghdr *>(buf);
            if (!NLMSG_OK(hdr, quint32(len)))
                return;

            auto arg = reinterpret_cast<FirstArgument>(NLMSG_DATA(hdr));
            size_t payloadLen = NLMSG_PAYLOAD(hdr, 0);

            // is this a multipart message?
            Q_ASSERT(isDump == !!(hdr->nlmsg_flags & NLM_F_MULTI));
            if (!isDump) {
                // no, single message
                if (hdr->nlmsg_type == expectedType && payloadLen >= sizeof(FirstArgument))
                    return void(func(arg, payloadLen));
            } else {
                // multipart, parse until done
                do {
                    if (hdr->nlmsg_type == NLMSG_DONE)
                        return;
                    if (hdr->nlmsg_type != expectedType || payloadLen < sizeof(FirstArgument))
                        break;
                    func(arg, payloadLen);

                    // NLMSG_NEXT also updates the len variable
                    hdr = NLMSG_NEXT(hdr, len);
                    arg = reinterpret_cast<FirstArgument>(NLMSG_DATA(hdr));
                    payloadLen = NLMSG_PAYLOAD(hdr, 0);
                } while (NLMSG_OK(hdr, quint32(len)));

                if (len == 0)
                    continue;       // get new datagram
            }

#ifndef QT_NO_DEBUG
            if (NLMSG_OK(hdr, quint32(len)))
                qWarning("QNetworkInterface/AF_NETLINK: received unknown packet type (%d) or too short (%u)",
                         hdr->nlmsg_type, hdr->nlmsg_len);
            else
                qWarning("QNetworkInterface/AF_NETLINK: received invalid packet with size %d", int(len));
#endif
            return;
        }
    }
};

template <typename Lambda>
void processNetlinkRequest(int sock, struct nlmsghdr *hdr, char *buf, size_t bufsize, Lambda &&l)
{
    ProcessNetlinkRequest<Lambda>()(sock, hdr, buf, bufsize, std::forward<Lambda>(l));
}
}

uint QNetworkInterfaceManager::interfaceIndexFromName(const QString &name)
{
    uint index = 0;
    if (name.length() >= IFNAMSIZ)
        return index;

    int socket = qt_safe_socket(AF_INET, SOCK_DGRAM, 0);
    if (socket >= 0) {
        struct ifreq req;
        req.ifr_ifindex = 0;
        strcpy(req.ifr_name, name.toLatin1().constData());

        if (qt_safe_ioctl(socket, SIOCGIFINDEX, &req) >= 0)
            index = req.ifr_ifindex;
        qt_safe_close(socket);
    }
    return index;
}

QString QNetworkInterfaceManager::interfaceNameFromIndex(uint index)
{
    int socket = qt_safe_socket(AF_INET, SOCK_DGRAM, 0);
    if (socket >= 0) {
        struct ifreq req;
        req.ifr_ifindex = index;

        if (qt_safe_ioctl(socket, SIOCGIFNAME, &req) >= 0) {
            qt_safe_close(socket);
            return QString::fromLatin1(req.ifr_name);
        }
        qt_safe_close(socket);
    }
    return QString();
}

static QList<QNetworkInterfacePrivate *> getInterfaces(int sock, char *buf)
{
    QList<QNetworkInterfacePrivate *> result;
    struct ifreq req;

    // request all links
    struct {
        struct nlmsghdr req;
        struct ifinfomsg ifi;
    } ifi_req;
    memset(&ifi_req, 0, sizeof(ifi_req));

    ifi_req.req.nlmsg_len = sizeof(ifi_req);
    ifi_req.req.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    ifi_req.req.nlmsg_type = RTM_GETLINK;

    // parse the interfaces
    processNetlinkRequest(sock, &ifi_req.req, buf, BufferSize, [&](ifinfomsg *ifi, size_t len) {
        auto iface = new QNetworkInterfacePrivate;
        iface->index = ifi->ifi_index;
        iface->flags = convertFlags(ifi->ifi_flags);

        // read attributes
        auto rta = reinterpret_cast<struct rtattr *>(ifi + 1);
        len -= sizeof(*ifi);
        for ( ; RTA_OK(rta, len); rta = RTA_NEXT(rta, len)) {
            int payloadLen = RTA_PAYLOAD(rta);
            auto payloadPtr = reinterpret_cast<char *>(RTA_DATA(rta));

            switch (rta->rta_type) {
            case IFLA_ADDRESS:      // link-level address
                iface->hardwareAddress =
                        iface->makeHwAddress(payloadLen, reinterpret_cast<uchar *>(payloadPtr));
                break;

            case IFLA_IFNAME:       // interface name
                Q_ASSERT(payloadLen <= int(sizeof(req.ifr_name)));
                memcpy(req.ifr_name, payloadPtr, payloadLen);   // including terminating NUL
                iface->name = QString::fromLatin1(payloadPtr, payloadLen - 1);
                break;

            case IFLA_MTU:
                Q_ASSERT(payloadLen == sizeof(int));
                iface->mtu = *reinterpret_cast<int *>(payloadPtr);
                break;

            case IFLA_OPERSTATE:    // operational state
                if (*payloadPtr != IF_OPER_UNKNOWN) {
                    // override the flag
                    iface->flags &= ~QNetworkInterface::IsUp;
                    if (*payloadPtr == IF_OPER_UP)
                        iface->flags |= QNetworkInterface::IsUp;
                }
                break;
            }
        }

        if (Q_UNLIKELY(iface->name.isEmpty())) {
            qWarning("QNetworkInterface: found interface %d with no name", iface->index);
            delete iface;
        } else {
            iface->type = probeIfType(sock, &req, ifi->ifi_type);
            result.append(iface);
        }
    });
    return result;
}

static void getAddresses(int sock, char *buf, QList<QNetworkInterfacePrivate *> &result)
{
    // request all addresses
    struct {
        struct nlmsghdr req;
        struct ifaddrmsg ifa;
    } ifa_req;
    memset(&ifa_req, 0, sizeof(ifa_req));

    ifa_req.req.nlmsg_len = sizeof(ifa_req);
    ifa_req.req.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    ifa_req.req.nlmsg_type = RTM_GETADDR;
    ifa_req.req.nlmsg_seq = 1;

    // parse the addresses
    processNetlinkRequest(sock, &ifa_req.req, buf, BufferSize, [&](ifaddrmsg *ifa, size_t len) {
        if (Q_UNLIKELY(ifa->ifa_family != AF_INET && ifa->ifa_family != AF_INET6)) {
            // unknown address types
            return;
        }

        // find the interface this is relevant to
        QNetworkInterfacePrivate *iface = nullptr;
        for (auto candidate : qAsConst(result)) {
            if (candidate->index != int(ifa->ifa_index))
                continue;
            iface = candidate;
            break;
        }

        if (Q_UNLIKELY(!iface)) {
            qWarning("QNetworkInterface/AF_NETLINK: found unknown interface with index %d", ifa->ifa_index);
            return;
        }

        QNetworkAddressEntry entry;
        quint32 flags = ifa->ifa_flags;  // may be overwritten by IFA_FLAGS

        auto makeAddress = [=](uchar *ptr, int len) {
            QHostAddress addr;
            if (ifa->ifa_family == AF_INET) {
                Q_ASSERT(len == 4);
                addr.setAddress(qFromBigEndian<quint32>(ptr));
            } else {
                Q_ASSERT(len == 16);
                addr.setAddress(ptr);

                // do we need a scope ID?
                if (addr.isLinkLocal())
                    addr.setScopeId(iface->name);
            }
            return addr;
        };

        // read attributes
        auto rta = reinterpret_cast<struct rtattr *>(ifa + 1);
        len -= sizeof(*ifa);
        for ( ; RTA_OK(rta, len); rta = RTA_NEXT(rta, len)) {
            int payloadLen = RTA_PAYLOAD(rta);
            auto payloadPtr = reinterpret_cast<uchar *>(RTA_DATA(rta));

            switch (rta->rta_type) {
            case IFA_ADDRESS:
                // Local address (all interfaces except for point-to-point)
                if (entry.ip().isNull())
                    entry.setIp(makeAddress(payloadPtr, payloadLen));
                break;

            case IFA_LOCAL:
                // Override the local address (point-to-point interfaces)
                entry.setIp(makeAddress(payloadPtr, payloadLen));
                break;

            case IFA_BROADCAST:
                Q_ASSERT(ifa->ifa_family == AF_INET);
                entry.setBroadcast(makeAddress(payloadPtr, payloadLen));
                break;

            case IFA_CACHEINFO:
                if (size_t(payloadLen) >= sizeof(ifa_cacheinfo)) {
                    auto cacheinfo = reinterpret_cast<ifa_cacheinfo *>(payloadPtr);
                    auto toDeadline = [](quint32 lifetime) -> QDeadlineTimer {
                        if (lifetime == quint32(-1))
                            return QDeadlineTimer::Forever;
                        return QDeadlineTimer(lifetime * 1000);
                    };
                    entry.setAddressLifetime(toDeadline(cacheinfo->ifa_prefered), toDeadline(cacheinfo->ifa_valid));
                }
                break;

            case IFA_FLAGS:
                Q_ASSERT(payloadLen == 4);
                flags = qFromUnaligned<quint32>(payloadPtr);
                break;
            }
        }

        // now handle flags
        QNetworkInterfacePrivate::calculateDnsEligibility(&entry,
                                                          flags & IFA_F_TEMPORARY,
                                                          flags & IFA_F_DEPRECATED);


        if (!entry.ip().isNull()) {
            entry.setPrefixLength(ifa->ifa_prefixlen);
            iface->addressEntries.append(entry);
        }
    });
}

QList<QNetworkInterfacePrivate *> QNetworkInterfaceManager::scan()
{
    // open netlink socket
    QList<QNetworkInterfacePrivate *> result;
    NetlinkSocket sock(BufferSize);
    if (Q_UNLIKELY(sock == -1))
        return result;

    QByteArray buffer(BufferSize, Qt::Uninitialized);
    char *buf = buffer.data();

    result = getInterfaces(sock, buf);
    getAddresses(sock, buf, result);

    return result;
}

QT_END_NAMESPACE

#endif // QT_NO_NETWORKINTERFACE
