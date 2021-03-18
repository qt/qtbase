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

#define WIN32_LEAN_AND_MEAN 1

#include "qnetworkinterface.h"
#include "qnetworkinterface_p.h"

#ifndef QT_NO_NETWORKINTERFACE

#include <qhostinfo.h>
#include <qhash.h>
#include <qurl.h>

// Since we need to include winsock2.h, we need to define WIN32_LEAN_AND_MEAN
// (above) so windows.h won't include winsock.h.
// In addition, we need to include winsock2.h before iphlpapi.h and we need
// to include ws2ipdef.h to work around an MinGW-w64 bug
// (http://sourceforge.net/p/mingw-w64/mailman/message/32935366/)
#include <winsock2.h>
#include <ws2ipdef.h>
#include <wincrypt.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>

#include <qt_windows.h>

// In case these aren't defined
#define IF_TYPE_IEEE80216_WMAN  237
#define IF_TYPE_IEEE802154      259

QT_BEGIN_NAMESPACE

static QHostAddress addressFromSockaddr(sockaddr *sa)
{
    QHostAddress address;
    if (!sa)
        return address;

    if (sa->sa_family == AF_INET) {
        address.setAddress(htonl(reinterpret_cast<const sockaddr_in *>(sa)->sin_addr.s_addr));
    } else if (sa->sa_family == AF_INET6) {
        auto sai6 = reinterpret_cast<const sockaddr_in6 *>(sa);
        address.setAddress(sai6->sin6_addr.s6_addr);
        if (sai6->sin6_scope_id)
            address.setScopeId(QNetworkInterfaceManager::interfaceNameFromIndex(sai6->sin6_scope_id));
    } else {
        qWarning("Got unknown socket family %d", sa->sa_family);
    }
    return address;

}

uint QNetworkInterfaceManager::interfaceIndexFromName(const QString &name)
{
    NET_IFINDEX id;
    NET_LUID luid;
    if (ConvertInterfaceNameToLuidW(reinterpret_cast<const wchar_t *>(name.constData()), &luid) == NO_ERROR
            && ConvertInterfaceLuidToIndex(&luid, &id) == NO_ERROR)
        return uint(id);
    return 0;
}

QString QNetworkInterfaceManager::interfaceNameFromIndex(uint index)
{
    NET_LUID luid;
    if (ConvertInterfaceIndexToLuid(index, &luid) == NO_ERROR) {
        WCHAR buf[IF_MAX_STRING_SIZE + 1];
        if (ConvertInterfaceLuidToNameW(&luid, buf, sizeof(buf)/sizeof(buf[0])) == NO_ERROR)
            return QString::fromWCharArray(buf);
    }
    return QString::number(index);
}

static QList<QNetworkInterfacePrivate *> interfaceListing()
{
    QList<QNetworkInterfacePrivate *> interfaces;
    IP_ADAPTER_ADDRESSES staticBuf[2]; // 2 is arbitrary
    PIP_ADAPTER_ADDRESSES pAdapter = staticBuf;
    ULONG bufSize = sizeof staticBuf;

    ULONG flags = GAA_FLAG_INCLUDE_PREFIX |
                  GAA_FLAG_SKIP_DNS_SERVER |
                  GAA_FLAG_SKIP_MULTICAST;
    ULONG retval = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAdapter, &bufSize);
    if (retval == ERROR_BUFFER_OVERFLOW) {
        // need more memory
        pAdapter = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(malloc(bufSize));
        if (!pAdapter)
            return interfaces;
        // try again
        if (GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAdapter, &bufSize) != ERROR_SUCCESS) {
            free(pAdapter);
            return interfaces;
        }
    } else if (retval != ERROR_SUCCESS) {
        // error
        return interfaces;
    }

    // iterate over the list and add the entries to our listing
    for (PIP_ADAPTER_ADDRESSES ptr = pAdapter; ptr; ptr = ptr->Next) {
        // the structure grows over time, so let's make sure the fields
        // introduced in Windows Vista are present (Luid is the furthest
        // field we access from IP_ADAPTER_ADDRESSES_LH)
        Q_ASSERT(ptr->Length >= offsetof(IP_ADAPTER_ADDRESSES, Luid));
        Q_ASSERT(ptr->Length >= offsetof(IP_ADAPTER_ADDRESSES, Ipv6IfIndex));

        QNetworkInterfacePrivate *iface = new QNetworkInterfacePrivate;
        interfaces << iface;

        iface->index = 0;
        if (ptr->Ipv6IfIndex != 0)
            iface->index = ptr->Ipv6IfIndex;
        else if (ptr->IfIndex != 0)
            iface->index = ptr->IfIndex;

        iface->mtu = qMin<qint64>(ptr->Mtu, INT_MAX);
        iface->flags = QNetworkInterface::CanBroadcast;
        if (ptr->OperStatus == IfOperStatusUp)
            iface->flags |= QNetworkInterface::IsUp | QNetworkInterface::IsRunning;
        if ((ptr->Flags & IP_ADAPTER_NO_MULTICAST) == 0)
            iface->flags |= QNetworkInterface::CanMulticast;
        if (ptr->IfType == IF_TYPE_PPP)
            iface->flags |= QNetworkInterface::IsPointToPoint;

        switch (ptr->IfType) {
        case IF_TYPE_ETHERNET_CSMACD:
            iface->type = QNetworkInterface::Ethernet;
            break;

        case IF_TYPE_FDDI:
            iface->type = QNetworkInterface::Fddi;
            break;

        case IF_TYPE_PPP:
            iface->type = QNetworkInterface::Ppp;
            break;

        case IF_TYPE_SLIP:
            iface->type = QNetworkInterface::Slip;
            break;

        case IF_TYPE_SOFTWARE_LOOPBACK:
            iface->type = QNetworkInterface::Loopback;
            iface->flags |= QNetworkInterface::IsLoopBack;
            break;

        case IF_TYPE_IEEE80211:
            iface->type = QNetworkInterface::Ieee80211;
            break;

        case IF_TYPE_IEEE1394:
            iface->type = QNetworkInterface::Ieee1394;
            break;

        case IF_TYPE_IEEE80216_WMAN:
            iface->type = QNetworkInterface::Ieee80216;
            break;

        case IF_TYPE_IEEE802154:
            iface->type = QNetworkInterface::Ieee802154;
            break;
        }

        // use ConvertInterfaceLuidToNameW because that returns a friendlier name, though not
        // as "friendly" as FriendlyName below
        WCHAR buf[IF_MAX_STRING_SIZE + 1];
        if (ConvertInterfaceLuidToNameW(&ptr->Luid, buf, sizeof(buf)/sizeof(buf[0])) == NO_ERROR)
            iface->name = QString::fromWCharArray(buf);
        if (iface->name.isEmpty())
            iface->name = QString::fromLocal8Bit(ptr->AdapterName);

        iface->friendlyName = QString::fromWCharArray(ptr->FriendlyName);
        if (ptr->PhysicalAddressLength)
            iface->hardwareAddress = iface->makeHwAddress(ptr->PhysicalAddressLength,
                                                          ptr->PhysicalAddress);

        // parse the IP (unicast) addresses
        for (PIP_ADAPTER_UNICAST_ADDRESS addr = ptr->FirstUnicastAddress; addr; addr = addr->Next) {
            Q_ASSERT(addr->Length >= offsetof(IP_ADAPTER_UNICAST_ADDRESS, OnLinkPrefixLength));

            // skip addresses in invalid state
            if (addr->DadState == IpDadStateInvalid)
                continue;

            QNetworkAddressEntry entry;
            entry.setIp(addressFromSockaddr(addr->Address.lpSockaddr));
            entry.setPrefixLength(addr->OnLinkPrefixLength);

            auto toDeadline = [](ULONG lifetime) -> QDeadlineTimer {
                if (lifetime == 0xffffffffUL)
                    return QDeadlineTimer::Forever;
                return QDeadlineTimer(lifetime * 1000);
            };
            entry.setAddressLifetime(toDeadline(addr->ValidLifetime), toDeadline(addr->PreferredLifetime));
            entry.setDnsEligibility(addr->Flags & IP_ADAPTER_ADDRESS_DNS_ELIGIBLE ?
                                        QNetworkAddressEntry::DnsEligible :
                                        QNetworkAddressEntry::DnsIneligible);

            iface->addressEntries << entry;
        }
    }

    if (pAdapter != staticBuf)
        free(pAdapter);

    return interfaces;
}

QList<QNetworkInterfacePrivate *> QNetworkInterfaceManager::scan()
{
    return interfaceListing();
}

QString QHostInfo::localDomainName()
{
    FIXED_INFO info, *pinfo;
    ULONG bufSize = sizeof info;
    pinfo = &info;
    if (GetNetworkParams(pinfo, &bufSize) == ERROR_BUFFER_OVERFLOW) {
        pinfo = reinterpret_cast<FIXED_INFO *>(malloc(bufSize));
        if (!pinfo)
            return QString();
        // try again
        if (GetNetworkParams(pinfo, &bufSize) != ERROR_SUCCESS) {
            free(pinfo);
            return QString();   // error
        }
    }

    QString domainName = QUrl::fromAce(pinfo->DomainName);

    if (pinfo != &info)
        free(pinfo);

    return domainName;
}

QT_END_NAMESPACE

#endif // QT_NO_NETWORKINTERFACE
