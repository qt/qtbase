/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QString>
#include <QtTest/QtTest>
#ifdef QT_NETWORK_LIB
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QTcpSocket>
#endif

#ifdef Q_OS_UNIX
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
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
#ifdef QT_TEST_SERVER_DOMAIN
        return QString(QT_TEST_SERVER_DOMAIN); // Defined in testserver feature
#else
        return QString("qt-test-net");
#endif
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
        const QHostInfo info = QHostInfo::fromName(serverName());
        if (info.error()) {
            QTest::qFail(qPrintable(info.errorString()), __FILE__, __LINE__);
            return QHostAddress();
        }
        return info.addresses().constFirst();
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
        // output would be e.g. "220 (vsFTPd 2.3.5)\r\n221 Goodbye.\r\n"
        QRegExp ftpVersion(QStringLiteral("220 \\(vsFTPd \\d+\\.\\d+.\\d+\\)\\r\\n221 Goodbye.\\r\\n"));
        return ftpVersion.exactMatch(actual);
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
        ::close(s);
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

    static bool verifyConnection(QString serverName, quint16 port, quint32 retry = 10)
    {
        QTcpSocket socket;
        for (quint32 i = 1; i < retry; i++) {
            socket.connectToHost(serverName, port);
            if (socket.waitForConnected(1000))
                return true;
            // Wait for service to start up
            QTest::qWait(1000);
        }
        socket.connectToHost(serverName, port);
        return socket.waitForConnected(1000);
    }

    // Helper function for usage with QVERIFY2 on sockets.
    static QByteArray msgSocketError(const QAbstractSocket &s)
    {
        QString result;
        QDebug debug(&result);
        debug.nospace();
        debug.noquote();
        if (!s.localAddress().isNull())
            debug << "local=" << s.localAddress().toString() << ':' << s.localPort();
        if (!s.peerAddress().isNull())
            debug << ", peer=" << s.peerAddress().toString() << ':' << s.peerPort();
        debug << ", type=" << s.socketType() << ", state=" << s.state()
            << ", error=" << s.error() << ": " << s.errorString();
       return result.toLocal8Bit();
    }
#endif // QT_NETWORK_LIB

    static QString ftpServerName()
    {
#ifdef QT_TEST_SERVER
        return QString("vsftpd.") % serverDomainName();
#else
        return serverName();
#endif
    }
    static QString ftpProxyServerName()
    {
#ifdef QT_TEST_SERVER
        return QString("ftp-proxy.") % serverDomainName();
#else
        return serverName();
#endif
    }
    static QString httpServerName()
    {
#ifdef QT_TEST_SERVER
        return QString("apache2.") % serverDomainName();
#else
        return serverName();
#endif
    }
    static QString httpProxyServerName()
    {
#ifdef QT_TEST_SERVER
        return QString("squid.") % serverDomainName();
#else
        return serverName();
#endif
    }
    static QString socksProxyServerName()
    {
#ifdef QT_TEST_SERVER
        return QString("danted.") % serverDomainName();
#else
        return serverName();
#endif
    }
};
