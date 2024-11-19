// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtNetwork/qsslsocket.h>
#include <QtNetwork/qnetworkproxy.h>
#include <QtNetwork/qauthenticator.h>

#include "private/qhostinfo_p.h"

#include "../../../network-settings.h"

#ifndef QT_NO_OPENSSL
typedef QSharedPointer<QSslSocket> QSslSocketPtr;
#endif

class tst_QSslSocket_onDemandCertificates_static : public QObject
{
    Q_OBJECT

    int proxyAuthCalled;

public:

#ifndef QT_NO_OPENSSL
    QSslSocketPtr newSocket();
#endif

public slots:
    void initTestCase_data();
    void initTestCase();
    void init();
    void cleanup();
    void proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *auth);

#ifndef QT_NO_OPENSSL
private slots:
    void onDemandRootCertLoadingStaticMethods();

private:
    QSslSocket *socket;
#endif // QT_NO_OPENSSL
};

enum ProxyTests {
    NoProxy = 0x00,
    Socks5Proxy = 0x01,
    HttpProxy = 0x02,
    TypeMask = 0x0f,

    NoAuth = 0x00,
    AuthBasic = 0x10,
    AuthNtlm = 0x20,
    AuthMask = 0xf0
};

void tst_QSslSocket_onDemandCertificates_static::initTestCase_data()
{
    QTest::addColumn<bool>("setProxy");
    QTest::addColumn<int>("proxyType");

    QTest::newRow("WithoutProxy") << false << 0;
    QTest::newRow("WithSocks5Proxy") << true << int(Socks5Proxy);
    QTest::newRow("WithSocks5ProxyAuth") << true << int(Socks5Proxy | AuthBasic);

    QTest::newRow("WithHttpProxy") << true << int(HttpProxy);
    QTest::newRow("WithHttpProxyBasicAuth") << true << int(HttpProxy | AuthBasic);
    // uncomment the line below when NTLM works
//    QTest::newRow("WithHttpProxyNtlmAuth") << true << int(HttpProxy | AuthNtlm);
}

void tst_QSslSocket_onDemandCertificates_static::initTestCase()
{
#ifdef QT_TEST_SERVER
    QVERIFY(QtNetworkSettings::verifyConnection(QtNetworkSettings::socksProxyServerName(), 1080));
    QVERIFY(QtNetworkSettings::verifyConnection(QtNetworkSettings::socksProxyServerName(), 1081));
    QVERIFY(QtNetworkSettings::verifyConnection(QtNetworkSettings::httpProxyServerName(), 3128));
    QVERIFY(QtNetworkSettings::verifyConnection(QtNetworkSettings::httpProxyServerName(), 3129));
    QVERIFY(QtNetworkSettings::verifyConnection(QtNetworkSettings::httpProxyServerName(), 3130));
#else
    if (!QtNetworkSettings::verifyTestNetworkSettings())
        QSKIP("No network test server available");
#endif // QT_TEST_SERVER
}

void tst_QSslSocket_onDemandCertificates_static::init()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
        QFETCH_GLOBAL(int, proxyType);
        const auto socksAddr = QtNetworkSettings::socksProxyServerIp().toString();
        const auto squidAddr = QtNetworkSettings::httpProxyServerIp().toString();

        QNetworkProxy proxy;

        switch (proxyType) {
        case Socks5Proxy:
            proxy = QNetworkProxy(QNetworkProxy::Socks5Proxy, socksAddr, 1080);
            break;

        case Socks5Proxy | AuthBasic:
            proxy = QNetworkProxy(QNetworkProxy::Socks5Proxy, socksAddr, 1081);
            break;

        case HttpProxy | NoAuth:
            proxy = QNetworkProxy(QNetworkProxy::HttpProxy, squidAddr, 3128);
            break;

        case HttpProxy | AuthBasic:
            proxy = QNetworkProxy(QNetworkProxy::HttpProxy, squidAddr, 3129);
            break;

        case HttpProxy | AuthNtlm:
            proxy = QNetworkProxy(QNetworkProxy::HttpProxy, squidAddr, 3130);
            break;
        }
        QNetworkProxy::setApplicationProxy(proxy);
    }

    qt_qhostinfo_clear_cache();
}

void tst_QSslSocket_onDemandCertificates_static::cleanup()
{
    QNetworkProxy::setApplicationProxy(QNetworkProxy::DefaultProxy);
}

#ifndef QT_NO_OPENSSL
QSslSocketPtr tst_QSslSocket_onDemandCertificates_static::newSocket()
{
    QSslSocket *socket = new QSslSocket;

    proxyAuthCalled = 0;
    connect(socket, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            Qt::DirectConnection);

    return QSslSocketPtr(socket);
}
#endif

void tst_QSslSocket_onDemandCertificates_static::proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *auth)
{
    ++proxyAuthCalled;
    auth->setUser("qsockstest");
    auth->setPassword("password");
}

#ifndef QT_NO_OPENSSL

void tst_QSslSocket_onDemandCertificates_static::onDemandRootCertLoadingStaticMethods()
{
    QString host("www.qt.io");

    // setting empty default configuration -> should not work
    QSslConfiguration conf;
    QSslConfiguration originalDefaultConf = QSslConfiguration::defaultConfiguration();
    QSslConfiguration::setDefaultConfiguration(conf);
    QSslSocketPtr socket4 = newSocket();
    this->socket = socket4.data();
    socket4->connectToHostEncrypted(host, 443);
    QVERIFY(!socket4->waitForEncrypted(4000));
    QSslConfiguration::setDefaultConfiguration(originalDefaultConf); // restore old behaviour for run with proxies etc.
}

#endif // QT_NO_OPENSSL

QTEST_MAIN(tst_QSslSocket_onDemandCertificates_static)
#include "tst_qsslsocket_onDemandCertificates_static.moc"
