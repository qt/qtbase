// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QString>
#include <QTest>
#include <QOperatingSystemVersion>
#include <QStringBuilder>
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
    static QHostAddress getServerIpImpl(const QString &serverName)
    {
        const QHostInfo info = QHostInfo::fromName(serverName);
        if (info.error()) {
            QTest::qFail(qPrintable(info.errorString()), __FILE__, __LINE__);
            return QHostAddress();
        }
        return info.addresses().constFirst();
    }

    static QHostAddress serverIP()
    {
        return getServerIpImpl(serverName());
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
        QRegularExpression ftpVersion(QRegularExpression::anchoredPattern(QStringLiteral("220 \\(vsFTPd \\d+\\.\\d+.\\d+\\)\\r\\n221 Goodbye.\\r\\n")));
        return ftpVersion.match(actual).hasMatch();
    }

    static bool hasIPv6()
    {
#if defined(Q_OS_QNX)
        // Qt's support for IPv6 on QNX appears to be broken.
        // This is an unaccepable situation after 2011-01-31.
        return false;
#elif defined(Q_OS_UNIX)
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

    static bool canBindToLowPorts()
    {
#ifdef Q_OS_UNIX
        if (geteuid() == 0)
            return true;
        if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSMojave)
            return true;
        // ### Which versions of iOS, watchOS and such does Apple's opening of
        // all ports apply to?
        return false;
#else
        // Windows
        return true;
#endif
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

    static bool verifyConnection(QString serverName, quint16 port, quint32 retry = 60)
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
#ifdef QT_TEST_SERVER_NAME
        return QString("vsftpd.") % serverDomainName();
#else
        return serverName();
#endif
    }
    static QString ftpProxyServerName()
    {
#ifdef QT_TEST_SERVER_NAME
        return QString("ftp-proxy.") % serverDomainName();
#else
        return serverName();
#endif
    }
    static QString httpServerName()
    {
#ifdef QT_TEST_SERVER_NAME
        return QString("apache2.") % serverDomainName();
#else
        return serverName();
#endif
    }
    static QString httpProxyServerName()
    {
#ifdef QT_TEST_SERVER_NAME
        return QString("squid.") % serverDomainName();
#else
        return serverName();
#endif
    }
    static QString socksProxyServerName()
    {
#ifdef QT_TEST_SERVER_NAME
        return QString("danted.") % serverDomainName();
#else
        return serverName();
#endif
    }
    static QString imapServerName()
    {
#ifdef QT_TEST_SERVER_NAME
        return QString("cyrus.") % serverDomainName();
#else
        return serverName();
#endif
    }

    static QString echoServerName()
    {
#ifdef QT_TEST_SERVER_NAME
        return QString("echo.") % serverDomainName();
#else
        return serverName();
#endif
    }

    static QString firewallServerName()
    {
#ifdef QT_TEST_SERVER_NAME
        return QString("iptables.") % serverDomainName();
#else
        return serverName();
#endif
    }

    static QString hostWithServiceOnPort(int port)
    {
#if !defined(QT_TEST_SERVER)
        Q_UNUSED(port);
        return serverName();
#else
        switch (port) {
        case 13:
        case 22:
        case 139:
            return serverName(); // No such things in docker (yet?)
        case 7:
            return echoServerName();
        case 21:
            return ftpServerName();
        case 80:
        case 443:
            return httpServerName();
        case 143:
            return imapServerName();
        case 3128:
        case 3129:
        case 3130:
            return httpProxyServerName();
        case 1080:
        case 1081:
            return socksProxyServerName();
        case 2121:
            return ftpProxyServerName();
        default:
            return serverName();
        }
#endif // QT_TEST_SERVER
    }

#ifdef QT_NETWORK_LIB
    static QHostAddress imapServerIp()
    {
        return getServerIpImpl(imapServerName());
    }

    static QHostAddress httpServerIp()
    {
        return getServerIpImpl(httpServerName());
    }

    static QHostAddress httpProxyServerIp()
    {
        return getServerIpImpl(httpProxyServerName());
    }

    static QHostAddress socksProxyServerIp()
    {
        return getServerIpImpl(socksProxyServerName());
    }

    static QHostAddress ftpProxyServerIp()
    {
        return getServerIpImpl(ftpProxyServerName());
    }

    static QHostAddress ftpServerIp()
    {
        return getServerIpImpl(ftpServerName());
    }

    static QHostAddress firewallServerIp()
    {
        return getServerIpImpl(firewallServerName());
    }

#endif // QT_NETWORK_LIB
};
