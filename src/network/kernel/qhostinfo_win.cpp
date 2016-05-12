/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <winsock2.h>

#include "qhostinfo_p.h"
#include <ws2tcpip.h>
#include <private/qsystemlibrary_p.h>
#include <qmutex.h>
#include <qbasicatomic.h>
#include <qurl.h>

QT_BEGIN_NAMESPACE

//#define QHOSTINFO_DEBUG

// Older SDKs do not include the addrinfo struct declaration, so we
// include a copy of it here.
struct qt_addrinfo
{
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    size_t ai_addrlen;
    char *ai_canonname;
    sockaddr *ai_addr;
    qt_addrinfo *ai_next;
};

//###
#define QT_SOCKLEN_T int
#ifndef NI_MAXHOST // already defined to 1025 in ws2tcpip.h?
#define NI_MAXHOST 1024
#endif

typedef int (__stdcall *getnameinfoProto)(const sockaddr *, QT_SOCKLEN_T, const char *, DWORD, const char *, DWORD, int);
typedef int (__stdcall *getaddrinfoProto)(const char *, const char *, const qt_addrinfo *, qt_addrinfo **);
typedef int (__stdcall *freeaddrinfoProto)(qt_addrinfo *);
static getnameinfoProto local_getnameinfo = 0;
static getaddrinfoProto local_getaddrinfo = 0;
static freeaddrinfoProto local_freeaddrinfo = 0;

static bool resolveLibraryInternal()
{
    // Attempt to resolve getaddrinfo(); without it we'll have to fall
    // back to gethostbyname(), which has no IPv6 support.
#if defined (Q_OS_WINRT)
    local_getaddrinfo = (getaddrinfoProto) &getaddrinfo;
    local_freeaddrinfo = (freeaddrinfoProto) &freeaddrinfo;
    local_getnameinfo = (getnameinfoProto) getnameinfo;
#else
    local_getaddrinfo = (getaddrinfoProto) QSystemLibrary::resolve(QLatin1String("ws2_32"), "getaddrinfo");
    local_freeaddrinfo = (freeaddrinfoProto) QSystemLibrary::resolve(QLatin1String("ws2_32"), "freeaddrinfo");
    local_getnameinfo = (getnameinfoProto) QSystemLibrary::resolve(QLatin1String("ws2_32"), "getnameinfo");
#endif
    return true;
}
Q_GLOBAL_STATIC_WITH_ARGS(bool, resolveLibrary, (resolveLibraryInternal()))

static void translateWSAError(int error, QHostInfo *results)
{
    switch (error) {
    case WSAHOST_NOT_FOUND: //authoritative not found
    case WSATRY_AGAIN: //non authoritative not found
    case WSANO_DATA: //valid name, no associated address
        results->setError(QHostInfo::HostNotFound);
        results->setErrorString(QHostInfoAgent::tr("Host not found"));
        return;
    default:
        results->setError(QHostInfo::UnknownError);
        results->setErrorString(QHostInfoAgent::tr("Unknown error (%1)").arg(error));
        return;
    }
}

QHostInfo QHostInfoAgent::fromName(const QString &hostName)
{
    QSysInfo::machineHostName();        // this initializes ws2_32.dll

    // Load res_init on demand.
    resolveLibrary();

    QHostInfo results;

#if defined(QHOSTINFO_DEBUG)
    qDebug("QHostInfoAgent::fromName(): looking up \"%s\" (IPv6 support is %s)",
           hostName.toLatin1().constData(),
           (local_getaddrinfo && local_freeaddrinfo) ? "enabled" : "disabled");
#endif

    QHostAddress address;
    if (address.setAddress(hostName)) {
        // Reverse lookup
        if (local_getnameinfo) {
            sockaddr_in sa4;
            sockaddr_in6 sa6;
            sockaddr *sa;
            QT_SOCKLEN_T saSize;
            if (address.protocol() == QAbstractSocket::IPv4Protocol) {
                sa = (sockaddr *)&sa4;
                saSize = sizeof(sa4);
                memset(&sa4, 0, sizeof(sa4));
                sa4.sin_family = AF_INET;
                sa4.sin_addr.s_addr = htonl(address.toIPv4Address());
            } else {
                sa = (sockaddr *)&sa6;
                saSize = sizeof(sa6);
                memset(&sa6, 0, sizeof(sa6));
                sa6.sin6_family = AF_INET6;
                memcpy(&sa6.sin6_addr, address.toIPv6Address().c, sizeof(sa6.sin6_addr));
            }

            char hbuf[NI_MAXHOST];
            if (local_getnameinfo(sa, saSize, hbuf, sizeof(hbuf), 0, 0, 0) == 0)
                results.setHostName(QString::fromLatin1(hbuf));
        } else {
            unsigned long addr = inet_addr(hostName.toLatin1().constData());
            struct hostent *ent = gethostbyaddr((const char*)&addr, sizeof(addr), AF_INET);
            if (ent)
                results.setHostName(QString::fromLatin1(ent->h_name));
        }

        if (results.hostName().isEmpty())
            results.setHostName(address.toString());
        results.setAddresses(QList<QHostAddress>() << address);
        return results;
    }

    // IDN support
    QByteArray aceHostname = QUrl::toAce(hostName);
    results.setHostName(hostName);
    if (aceHostname.isEmpty()) {
        results.setError(QHostInfo::HostNotFound);
        results.setErrorString(hostName.isEmpty() ? tr("No host name given") : tr("Invalid hostname"));
        return results;
    }

    if (local_getaddrinfo && local_freeaddrinfo) {
        // Call getaddrinfo, and place all IPv4 addresses at the start
        // and the IPv6 addresses at the end of the address list in
        // results.
        qt_addrinfo *res;
        int err = local_getaddrinfo(aceHostname.constData(), 0, 0, &res);
        if (err == 0) {
            QList<QHostAddress> addresses;
            for (qt_addrinfo *p = res; p != 0; p = p->ai_next) {
                switch (p->ai_family) {
                case AF_INET: {
                    QHostAddress addr;
                    addr.setAddress(ntohl(((sockaddr_in *) p->ai_addr)->sin_addr.s_addr));
                    if (!addresses.contains(addr))
                        addresses.append(addr);
                }
                    break;
                case AF_INET6: {
                    QHostAddress addr;
                    addr.setAddress(((sockaddr_in6 *) p->ai_addr)->sin6_addr.s6_addr);
                    if (!addresses.contains(addr))
                        addresses.append(addr);
                }
                    break;
                default:
                    results.setError(QHostInfo::UnknownError);
                    results.setErrorString(tr("Unknown address type"));
                }
            }
            results.setAddresses(addresses);
            local_freeaddrinfo(res);
        } else {
            translateWSAError(WSAGetLastError(), &results);
        }
    } else {
        // Fall back to gethostbyname, which only supports IPv4.
        hostent *ent = gethostbyname(aceHostname.constData());
        if (ent) {
            char **p;
            QList<QHostAddress> addresses;
            switch (ent->h_addrtype) {
            case AF_INET:
                for (p = ent->h_addr_list; *p != 0; p++) {
                    long *ip4Addr = (long *) *p;
                    QHostAddress temp;
                    temp.setAddress(ntohl(*ip4Addr));
                    addresses << temp;
                }
                break;
            default:
                results.setError(QHostInfo::UnknownError);
                results.setErrorString(tr("Unknown address type"));
                break;
            }
            results.setAddresses(addresses);
        } else {
            translateWSAError(WSAGetLastError(), &results);
        }
    }

#if defined(QHOSTINFO_DEBUG)
    if (results.error() != QHostInfo::NoError) {
        qDebug("QHostInfoAgent::run(): error (%s)",
               results.errorString().toLatin1().constData());
    } else {
        QString tmp;
        QList<QHostAddress> addresses = results.addresses();
        for (int i = 0; i < addresses.count(); ++i) {
            if (i != 0) tmp += ", ";
            tmp += addresses.at(i).toString();
        }
        qDebug("QHostInfoAgent::run(): found %i entries: {%s}",
               addresses.count(), tmp.toLatin1().constData());
    }
#endif
    return results;
}

// QString QHostInfo::localDomainName() defined in qnetworkinterface_win.cpp

QT_END_NAMESPACE
