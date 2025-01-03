// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <winsock2.h>

#include "qhostinfo_p.h"
#include <ws2tcpip.h>
#include <private/qsystemlibrary_p.h>
#include <qmutex.h>
#include <qbasicatomic.h>
#include <qurl.h>

QT_BEGIN_NAMESPACE

//#define QHOSTINFO_DEBUG

//###
#ifndef NI_MAXHOST // already defined to 1025 in ws2tcpip.h?
#define NI_MAXHOST 1024
#endif

QHostInfo QHostInfoAgent::fromName(const QString &hostName)
{
    QSysInfo::machineHostName();        // this initializes ws2_32.dll

    QHostInfo results;

#if defined(QHOSTINFO_DEBUG)
    qDebug("QHostInfoAgent::fromName(%s) looking up...",
           hostName.toLatin1().constData());
#endif

    QHostAddress address;
    if (address.setAddress(hostName))
        return reverseLookup(address);

    return lookup(hostName);
}

// QString QHostInfo::localDomainName() defined in qnetworkinterface_win.cpp

QT_END_NAMESPACE
