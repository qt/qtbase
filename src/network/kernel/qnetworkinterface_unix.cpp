/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include "qbytearray.h"
#include "qset.h"
#include "qnetworkinterface.h"
#include "qnetworkinterface_p.h"
#include "qnetworkinterface_unix_p.h"
#include "qalgorithms.h"

#ifndef QT_NO_NETWORKINTERFACE

#if defined(QT_NO_CLOCK_MONOTONIC)
#  include "qdatetime.h"
#endif

#if defined(QT_LINUXBASE)
#  define QT_NO_GETIFADDRS
#endif

#ifndef QT_NO_GETIFADDRS
# include <ifaddrs.h>
#endif

#ifdef QT_LINUXBASE
#  include <arpa/inet.h>
#  ifndef SIOCGIFBRDADDR
#    define SIOCGIFBRDADDR 0x8919
#  endif
#endif // QT_LINUXBASE

#include <qplatformdefs.h>

QT_BEGIN_NAMESPACE

static QHostAddress addressFromSockaddr(sockaddr *sa, int ifindex = 0, const QString &ifname = QString())
{
    QHostAddress address;
    if (!sa)
        return address;

    if (sa->sa_family == AF_INET)
        address.setAddress(htonl(((sockaddr_in *)sa)->sin_addr.s_addr));
    else if (sa->sa_family == AF_INET6) {
        address.setAddress(((sockaddr_in6 *)sa)->sin6_addr.s6_addr);
        int scope = ((sockaddr_in6 *)sa)->sin6_scope_id;
        if (scope && scope == ifindex) {
            // this is the most likely scenario:
            // a scope ID in a socket is that of the interface this address came from
            address.setScopeId(ifname);
        } else if (scope) {
            address.setScopeId(QNetworkInterfaceManager::interfaceNameFromIndex(scope));
        }
    }
    return address;

}

uint QNetworkInterfaceManager::interfaceIndexFromName(const QString &name)
{
#ifndef QT_NO_IPV6IFNAME
    return ::if_nametoindex(name.toLatin1());
#elif defined(SIOCGIFINDEX)
    struct ifreq req;
    int socket = qt_safe_socket(AF_INET, SOCK_STREAM, 0);
    if (socket < 0)
        return 0;

    QByteArray name8bit = name.toLatin1();
    memset(&req, 0, sizeof(ifreq));
    memcpy(req.ifr_name, name8bit, qMin<int>(name8bit.length() + 1, sizeof(req.ifr_name) - 1));

    uint id = 0;
    if (qt_safe_ioctl(socket, SIOCGIFINDEX, &req) >= 0)
        id = req.ifr_ifindex;
    qt_safe_close(socket);
    return id;
#else
    return 0;
#endif
}

QString QNetworkInterfaceManager::interfaceNameFromIndex(uint index)
{
#ifndef QT_NO_IPV6IFNAME
    char buf[IF_NAMESIZE];
    if (::if_indextoname(index, buf))
        return QString::fromLatin1(buf);
#elif defined(SIOCGIFNAME)
    struct ifreq req;
    int socket = qt_safe_socket(AF_INET, SOCK_STREAM, 0);
    if (socket >= 0) {
        memset(&req, 0, sizeof(ifreq));
        req.ifr_ifindex = index;

        if (qt_safe_ioctl(socket, SIOCGIFNAME, &req) >= 0) {
            qt_safe_close(socket);
            return QString::fromLatin1(req.ifr_name);
        }
        qt_safe_close(socket);
    }
#endif
    return QString::number(uint(index));
}

static int getMtu(int socket, struct ifreq *req)
{
#ifdef SIOCGIFMTU
    if (qt_safe_ioctl(socket, SIOCGIFMTU, req) == 0)
        return req->ifr_mtu;
#endif
    return 0;
}

#ifdef QT_NO_GETIFADDRS
// getifaddrs not available

static QSet<QByteArray> interfaceNames(int socket)
{
    QSet<QByteArray> result;
#ifdef QT_NO_IPV6IFNAME
    QByteArray storageBuffer;
    struct ifconf interfaceList;
    static const int STORAGEBUFFER_GROWTH = 256;

    forever {
        // grow the storage buffer
        storageBuffer.resize(storageBuffer.size() + STORAGEBUFFER_GROWTH);
        interfaceList.ifc_buf = storageBuffer.data();
        interfaceList.ifc_len = storageBuffer.size();

        // get the interface list
        if (qt_safe_ioctl(socket, SIOCGIFCONF, &interfaceList) >= 0) {
            if (int(interfaceList.ifc_len + sizeof(ifreq) + 64) < storageBuffer.size()) {
                // if the buffer was big enough, break
                storageBuffer.resize(interfaceList.ifc_len);
                break;
            }
        } else {
            // internal error
            return result;
        }
        if (storageBuffer.size() > 100000) {
            // out of space
            return result;
        }
    }

    int interfaceCount = interfaceList.ifc_len / sizeof(ifreq);
    for (int i = 0; i < interfaceCount; ++i) {
        QByteArray name = QByteArray(interfaceList.ifc_req[i].ifr_name);
        if (!name.isEmpty())
            result << name;
    }

    return result;
#else
    Q_UNUSED(socket);

    // use if_nameindex
    struct if_nameindex *interfaceList = ::if_nameindex();
    for (struct if_nameindex *ptr = interfaceList; ptr && ptr->if_name; ++ptr)
        result << ptr->if_name;

    if_freenameindex(interfaceList);
    return result;
#endif
}

static QNetworkInterfacePrivate *findInterface(int socket, QList<QNetworkInterfacePrivate *> &interfaces,
                                               struct ifreq &req)
{
    QNetworkInterfacePrivate *iface = 0;
    int ifindex = 0;

#if !defined(QT_NO_IPV6IFNAME) || defined(SIOCGIFINDEX)
    // Get the interface index
#  ifdef SIOCGIFINDEX
    if (qt_safe_ioctl(socket, SIOCGIFINDEX, &req) >= 0)
#    if defined(Q_OS_HAIKU)
        ifindex = req.ifr_index;
#    else
        ifindex = req.ifr_ifindex;
#    endif
#  else
    ifindex = if_nametoindex(req.ifr_name);
#  endif

    // find the interface data
    QList<QNetworkInterfacePrivate *>::Iterator if_it = interfaces.begin();
    for ( ; if_it != interfaces.end(); ++if_it)
        if ((*if_it)->index == ifindex) {
            // existing interface
            iface = *if_it;
            break;
        }
#else
    // Search by name
    QList<QNetworkInterfacePrivate *>::Iterator if_it = interfaces.begin();
    for ( ; if_it != interfaces.end(); ++if_it)
        if ((*if_it)->name == QLatin1String(req.ifr_name)) {
            // existing interface
            iface = *if_it;
            break;
        }
#endif

    if (!iface) {
        // new interface, create data:
        iface = new QNetworkInterfacePrivate;
        iface->index = ifindex;
        interfaces << iface;
    }

    return iface;
}

static QList<QNetworkInterfacePrivate *> interfaceListing()
{
    QList<QNetworkInterfacePrivate *> interfaces;

    int socket;
    if ((socket = qt_safe_socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == -1)
        return interfaces;      // error

    QSet<QByteArray> names = interfaceNames(socket);
    QSet<QByteArray>::ConstIterator it = names.constBegin();
    for ( ; it != names.constEnd(); ++it) {
        ifreq req;
        memset(&req, 0, sizeof(ifreq));
        memcpy(req.ifr_name, *it, qMin<int>(it->length() + 1, sizeof(req.ifr_name) - 1));

        QNetworkInterfacePrivate *iface = findInterface(socket, interfaces, req);

#ifdef SIOCGIFNAME
        // Get the canonical name
        QByteArray oldName = req.ifr_name;
        if (qt_safe_ioctl(socket, SIOCGIFNAME, &req) >= 0) {
            iface->name = QString::fromLatin1(req.ifr_name);

            // reset the name:
            memcpy(req.ifr_name, oldName, qMin<int>(oldName.length() + 1, sizeof(req.ifr_name) - 1));
        } else
#endif
        {
            // use this name anyways
            iface->name = QString::fromLatin1(req.ifr_name);
        }

        // Get interface flags
        if (qt_safe_ioctl(socket, SIOCGIFFLAGS, &req) >= 0) {
            iface->flags = convertFlags(req.ifr_flags);
        }
        iface->mtu = getMtu(socket, &req);

#ifdef SIOCGIFHWADDR
        // Get the HW address
        if (qt_safe_ioctl(socket, SIOCGIFHWADDR, &req) >= 0) {
            uchar *addr = (uchar *)req.ifr_addr.sa_data;
            iface->hardwareAddress = iface->makeHwAddress(6, addr);
        }
#endif

        // Get the address of the interface
        QNetworkAddressEntry entry;
        if (qt_safe_ioctl(socket, SIOCGIFADDR, &req) >= 0) {
            sockaddr *sa = &req.ifr_addr;
            entry.setIp(addressFromSockaddr(sa));

            // Get the interface broadcast address
            if (iface->flags & QNetworkInterface::CanBroadcast) {
                if (qt_safe_ioctl(socket, SIOCGIFBRDADDR, &req) >= 0) {
                    sockaddr *sa = &req.ifr_addr;
                    if (sa->sa_family == AF_INET)
                        entry.setBroadcast(addressFromSockaddr(sa));
                }
            }

            // Get the interface netmask
            if (qt_safe_ioctl(socket, SIOCGIFNETMASK, &req) >= 0) {
                sockaddr *sa = &req.ifr_addr;
                entry.setNetmask(addressFromSockaddr(sa));
            }

            iface->addressEntries << entry;
        }
    }

    ::close(socket);
    return interfaces;
}

#else
// use getifaddrs

// platform-specific defs:
# ifdef Q_OS_LINUX
QT_BEGIN_INCLUDE_NAMESPACE
#  include <features.h>
QT_END_INCLUDE_NAMESPACE
# endif

# if defined(Q_OS_LINUX) &&  __GLIBC__ - 0 >= 2 && __GLIBC_MINOR__ - 0 >= 1 && !defined(QT_LINUXBASE)
#  include <netpacket/packet.h>

static QList<QNetworkInterfacePrivate *> createInterfaces(ifaddrs *rawList)
{
    Q_UNUSED(getMtu)
    QList<QNetworkInterfacePrivate *> interfaces;
    QSet<QString> seenInterfaces;
    QVarLengthArray<int, 16> seenIndexes;   // faster than QSet<int>

    // On Linux, glibc, uClibc and MUSL obtain the address listing via two
    // netlink calls: first an RTM_GETLINK to obtain the interface listing,
    // then one RTM_GETADDR to get all the addresses (uClibc implementation is
    // copied from glibc; Bionic currently doesn't support getifaddrs). They
    // synthesize AF_PACKET addresses from the RTM_GETLINK responses, which
    // means by construction they currently show up first in the interface
    // listing.
    for (ifaddrs *ptr = rawList; ptr; ptr = ptr->ifa_next) {
        if (ptr->ifa_addr && ptr->ifa_addr->sa_family == AF_PACKET) {
            sockaddr_ll *sll = (sockaddr_ll *)ptr->ifa_addr;
            QNetworkInterfacePrivate *iface = new QNetworkInterfacePrivate;
            interfaces << iface;
            iface->index = sll->sll_ifindex;
            iface->name = QString::fromLatin1(ptr->ifa_name);
            iface->flags = convertFlags(ptr->ifa_flags);
            iface->hardwareAddress = iface->makeHwAddress(sll->sll_halen, (uchar*)sll->sll_addr);

            Q_ASSERT(!seenIndexes.contains(iface->index));
            seenIndexes.append(iface->index);
            seenInterfaces.insert(iface->name);
        }
    }

    // see if we missed anything:
    // - virtual interfaces with no HW address have no AF_PACKET
    // - interface labels have no AF_PACKET, but shouldn't be shown as a new interface
    for (ifaddrs *ptr = rawList; ptr; ptr = ptr->ifa_next) {
        if (!ptr->ifa_addr || ptr->ifa_addr->sa_family != AF_PACKET) {
            QString name = QString::fromLatin1(ptr->ifa_name);
            if (seenInterfaces.contains(name))
                continue;

            int ifindex = if_nametoindex(ptr->ifa_name);
            if (seenIndexes.contains(ifindex))
                continue;

            seenInterfaces.insert(name);
            seenIndexes.append(ifindex);

            QNetworkInterfacePrivate *iface = new QNetworkInterfacePrivate;
            interfaces << iface;
            iface->name = name;
            iface->flags = convertFlags(ptr->ifa_flags);
            iface->index = ifindex;
        }
    }

    return interfaces;
}

static void getAddressExtraInfo(QNetworkAddressEntry *entry, struct sockaddr *sa, const char *ifname)
{
    Q_UNUSED(entry);
    Q_UNUSED(sa);
    Q_UNUSED(ifname)
}

# elif defined(Q_OS_BSD4)
QT_BEGIN_INCLUDE_NAMESPACE
#  include <net/if_dl.h>
#if defined(QT_PLATFORM_UIKIT)
#  include "qnetworkinterface_uikit_p.h"
#if !defined(QT_WATCHOS_OUTDATED_SDK_WORKAROUND)
// TODO: remove it as soon as SDK is updated on CI!!!
#  include <net/if_types.h>
#endif
#else
#  include <net/if_media.h>
#  include <net/if_types.h>
#  include <netinet/in_var.h>
#endif // QT_PLATFORM_UIKIT
QT_END_INCLUDE_NAMESPACE

static int openSocket(int &socket)
{
    if (socket == -1)
        socket = qt_safe_socket(AF_INET, SOCK_DGRAM, 0);
    return socket;
}

static QNetworkInterface::InterfaceType probeIfType(int socket, int iftype, struct ifmediareq *req)
{
    // Determine the interface type.

    // On Darwin, these are #defines, but on FreeBSD they're just an
    // enum, so we can't #ifdef them. Use the authoritative list from
    // https://www.iana.org/assignments/smi-numbers/smi-numbers.xhtml#smi-numbers-5
    switch (iftype) {
    case IFT_PPP:
        return QNetworkInterface::Ppp;

    case IFT_LOOP:
        return QNetworkInterface::Loopback;

    case IFT_SLIP:
        return QNetworkInterface::Slip;

    case 0x47:      // IFT_IEEE80211
        return QNetworkInterface::Ieee80211;

    case IFT_IEEE1394:
        return QNetworkInterface::Ieee1394;

    case IFT_GIF:
    case IFT_STF:
        return QNetworkInterface::Virtual;
    }

    // For the remainder (including Ethernet), let's try SIOGIFMEDIA
    req->ifm_count = 0;
    if (qt_safe_ioctl(socket, SIOCGIFMEDIA, req) == 0) {
        // see https://man.openbsd.org/ifmedia.4

        switch (IFM_TYPE(req->ifm_current)) {
        case IFM_ETHER:
            return QNetworkInterface::Ethernet;

        case IFM_FDDI:
            return QNetworkInterface::Fddi;

        case IFM_IEEE80211:
            return QNetworkInterface::Ieee80211;
        }
    }

    return QNetworkInterface::Unknown;
}

static QList<QNetworkInterfacePrivate *> createInterfaces(ifaddrs *rawList)
{
    QList<QNetworkInterfacePrivate *> interfaces;
    union {
        struct ifmediareq mediareq;
        struct ifreq req;
    };
    int socket = -1;

    // ensure both structs start with the name field, of size IFNAMESIZ
    Q_STATIC_ASSERT(sizeof(mediareq.ifm_name) == sizeof(req.ifr_name));
    Q_ASSERT(&mediareq.ifm_name == &req.ifr_name);

    // on NetBSD we use AF_LINK and sockaddr_dl
    // scan the list for that family
    for (ifaddrs *ptr = rawList; ptr; ptr = ptr->ifa_next)
        if (ptr->ifa_addr && ptr->ifa_addr->sa_family == AF_LINK) {
            QNetworkInterfacePrivate *iface = new QNetworkInterfacePrivate;
            interfaces << iface;

            sockaddr_dl *sdl = (sockaddr_dl *)ptr->ifa_addr;
            iface->index = sdl->sdl_index;
            iface->name = QString::fromLatin1(ptr->ifa_name);
            iface->flags = convertFlags(ptr->ifa_flags);
            iface->hardwareAddress = iface->makeHwAddress(sdl->sdl_alen, (uchar*)LLADDR(sdl));

            qstrncpy(mediareq.ifm_name, ptr->ifa_name, sizeof(mediareq.ifm_name));
            iface->type = probeIfType(openSocket(socket), sdl->sdl_type, &mediareq);
            iface->mtu = getMtu(socket, &req);
        }

    if (socket != -1)
        qt_safe_close(socket);
    return interfaces;
}

static void getAddressExtraInfo(QNetworkAddressEntry *entry, struct sockaddr *sa, const char *ifname)
{
    // get IPv6 address lifetimes
    if (sa->sa_family != AF_INET6)
        return;

    struct in6_ifreq ifr;

    int s6 = qt_safe_socket(AF_INET6, SOCK_DGRAM, 0);
    if (Q_UNLIKELY(s6 < 0)) {
        qErrnoWarning("QNetworkInterface: could not create IPv6 socket");
        return;
    }

    qstrncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

    // get flags
    ifr.ifr_addr = *reinterpret_cast<struct sockaddr_in6 *>(sa);
    if (qt_safe_ioctl(s6, SIOCGIFAFLAG_IN6, &ifr) < 0) {
        qt_safe_close(s6);
        return;
    }
    int flags = ifr.ifr_ifru.ifru_flags6;
    QNetworkInterfacePrivate::calculateDnsEligibility(entry,
                                                      flags & IN6_IFF_TEMPORARY,
                                                      flags & IN6_IFF_DEPRECATED);

    // get lifetimes
    ifr.ifr_addr = *reinterpret_cast<struct sockaddr_in6 *>(sa);
    if (qt_safe_ioctl(s6, SIOCGIFALIFETIME_IN6, &ifr) < 0) {
        qt_safe_close(s6);
        return;
    }
    qt_safe_close(s6);

    auto toDeadline = [](time_t when) {
        QDeadlineTimer deadline = QDeadlineTimer::Forever;
        if (when) {
#if defined(QT_NO_CLOCK_MONOTONIC)
            // no monotonic clock
            deadline.setPreciseRemainingTime(when - QDateTime::currentSecsSinceEpoch());
#else
            deadline.setPreciseDeadline(when);
#endif
        }
        return deadline;
    };
    entry->setAddressLifetime(toDeadline(ifr.ifr_ifru.ifru_lifetime.ia6t_preferred),
                              toDeadline(ifr.ifr_ifru.ifru_lifetime.ia6t_expire));
}

# else  // Generic version

static QList<QNetworkInterfacePrivate *> createInterfaces(ifaddrs *rawList)
{
    Q_UNUSED(getMtu)
    QList<QNetworkInterfacePrivate *> interfaces;

    // make sure there's one entry for each interface
    for (ifaddrs *ptr = rawList; ptr; ptr = ptr->ifa_next) {
        // Get the interface index
        int ifindex = if_nametoindex(ptr->ifa_name);

        QList<QNetworkInterfacePrivate *>::Iterator if_it = interfaces.begin();
        for ( ; if_it != interfaces.end(); ++if_it)
            if ((*if_it)->index == ifindex)
                // this one has been added already
                break;

        if (if_it == interfaces.end()) {
            // none found, create
            QNetworkInterfacePrivate *iface = new QNetworkInterfacePrivate;
            interfaces << iface;

            iface->index = ifindex;
            iface->name = QString::fromLatin1(ptr->ifa_name);
            iface->flags = convertFlags(ptr->ifa_flags);
        }
    }

    return interfaces;
}

static void getAddressExtraInfo(QNetworkAddressEntry *entry, struct sockaddr *sa, const char *ifname)
{
    Q_UNUSED(entry);
    Q_UNUSED(sa);
    Q_UNUSED(ifname)
}
# endif

static QList<QNetworkInterfacePrivate *> interfaceListing()
{
    QList<QNetworkInterfacePrivate *> interfaces;

    ifaddrs *interfaceListing;
    if (getifaddrs(&interfaceListing) == -1) {
        // error
        return interfaces;
    }

    interfaces = createInterfaces(interfaceListing);
    for (ifaddrs *ptr = interfaceListing; ptr; ptr = ptr->ifa_next) {
        // Find the interface
        QLatin1String name(ptr->ifa_name);
        QNetworkInterfacePrivate *iface = 0;
        QList<QNetworkInterfacePrivate *>::Iterator if_it = interfaces.begin();
        for ( ; if_it != interfaces.end(); ++if_it)
            if ((*if_it)->name == name) {
                // found this interface already
                iface = *if_it;
                break;
            }

        if (!iface) {
            // it may be an interface label, search by interface index
            int ifindex = if_nametoindex(ptr->ifa_name);
            for (if_it = interfaces.begin(); if_it != interfaces.end(); ++if_it)
                if ((*if_it)->index == ifindex) {
                    // found this interface already
                    iface = *if_it;
                    break;
                }
        }

        if (!iface) {
            // skip all non-IP interfaces
            continue;
        }

        QNetworkAddressEntry entry;
        entry.setIp(addressFromSockaddr(ptr->ifa_addr, iface->index, iface->name));
        if (entry.ip().isNull())
            // could not parse the address
            continue;

        entry.setNetmask(addressFromSockaddr(ptr->ifa_netmask, iface->index, iface->name));
        if (iface->flags & QNetworkInterface::CanBroadcast)
            entry.setBroadcast(addressFromSockaddr(ptr->ifa_broadaddr, iface->index, iface->name));
        getAddressExtraInfo(&entry, ptr->ifa_addr, name.latin1());

        iface->addressEntries << entry;
    }

    freeifaddrs(interfaceListing);
    return interfaces;
}
#endif

QList<QNetworkInterfacePrivate *> QNetworkInterfaceManager::scan()
{
    return interfaceListing();
}

QT_END_NAMESPACE

#endif // QT_NO_NETWORKINTERFACE
