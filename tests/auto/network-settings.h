/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QString>
#ifdef QT_NETWORK_LIB
#include <QtNetwork/QHostInfo>
#endif

#ifdef Q_OS_UNIX
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

class QtNetworkSettings
{
public:

    static QString serverLocalName()
    {
        return QString("qt-test-server");
    }
    static QString serverDomainName()
    {
        return QString("qt-test-net");
    }
    static QString serverName()
    {
        return serverLocalName() + "." + serverDomainName();
    }
    static QString winServerName()
    {
        return serverName();
    }
    static QString wildcardServerName()
    {
        return "qt-test-server.wildcard.dev." + serverDomainName();
    }

#ifdef QT_NETWORK_LIB
    static QHostAddress serverIP()
    {
        return QHostInfo::fromName(serverName()).addresses().first();
    }
#endif

    static bool compareReplyIMAP(QByteArray const& actual)
    {
        // Server greeting may contain capability, version and server name
        // But spec only requires "* OK" and "\r\n"
        // Match against a prefix and postfix that covers all Cyrus versions
        if (actual.startsWith("* OK ")
            && actual.endsWith("server ready\r\n")) {
            return true;
        }

        return false;
    }

    static bool compareReplyIMAPSSL(QByteArray const& actual)
    {
        return compareReplyIMAP(actual);
    }

    static bool compareReplyFtp(QByteArray const& actual)
    {
        QList<QByteArray> expected;

        // A few different vsFTPd versions.
        // Feel free to add more as needed
        expected << QByteArray( "220 (vsFTPd 2.0.5)\r\n221 Goodbye.\r\n" );
        expected << QByteArray( "220 (vsFTPd 2.2.2)\r\n221 Goodbye.\r\n" );

        Q_FOREACH (QByteArray const& ba, expected) {
            if (ba == actual) {
                return true;
            }
        }

        return false;
    }

    static bool hasIPv6()
    {
#ifdef Q_OS_UNIX
        int s = ::socket(AF_INET6, SOCK_DGRAM, 0);
        if (s == -1)
            return false;
        else {
            struct sockaddr_in6 addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin6_family = AF_INET6;
            memcpy(&addr.sin6_addr, &in6addr_loopback, sizeof(in6_addr));
            if (-1 == ::bind(s, (sockaddr*)&addr, sizeof(addr))) {
                ::close(s);
                return false;
            }
        }
#endif
        return true;
    }


#ifdef QT_NETWORK_LIB
    static bool verifyTestNetworkSettings()
    {
        QHostInfo testServerResult = QHostInfo::fromName(QtNetworkSettings::serverName());
        if (testServerResult.error() != QHostInfo::NoError) {
            qWarning() << "Could not lookup" << QtNetworkSettings::serverName();
            qWarning() << "Please configure the test environment!";
            qWarning() << "See /etc/hosts or network-settings.h";
            return false;
        }
        return true;
    }
#endif
};
