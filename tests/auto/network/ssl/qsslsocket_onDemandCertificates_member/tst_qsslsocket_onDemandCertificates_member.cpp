// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtNetwork/qsslsocket.h>
#include <QtNetwork/qnetworkproxy.h>
#include <QtNetwork/qauthenticator.h>

#ifdef QT_BUILD_INTERNAL
#  include "private/qhostinfo_p.h"
#  ifndef QT_NO_OPENSSL
#    include "private/qsslsocket_p.h"
#  endif // !QT_NO_OPENSSL
#endif // QT_BUILD_INTERNAL
#include "../../../network-settings.h"

#ifndef QT_NO_OPENSSL
typedef QSharedPointer<QSslSocket> QSslSocketPtr;

QT_BEGIN_NAMESPACE
void qt_ForceTlsSecurityLevel();
QT_END_NAMESPACE

#endif

class tst_QSslSocket_onDemandCertificates_member : public QObject
{
    Q_OBJECT

    int proxyAuthCalled;

public:

#ifndef QT_NO_OPENSSL
    tst_QSslSocket_onDemandCertificates_member()
    {
        QT_PREPEND_NAMESPACE(qt_ForceTlsSecurityLevel)();
    }
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
    void onDemandRootCertLoadingMemberMethods();

private:
    QSslSocket *socket = nullptr;
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

void tst_QSslSocket_onDemandCertificates_member::initTestCase_data()
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

void tst_QSslSocket_onDemandCertificates_member::initTestCase()
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

void tst_QSslSocket_onDemandCertificates_member::init()
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

void tst_QSslSocket_onDemandCertificates_member::cleanup()
{
    QNetworkProxy::setApplicationProxy(QNetworkProxy::DefaultProxy);
}

#ifndef QT_NO_OPENSSL
QSslSocketPtr tst_QSslSocket_onDemandCertificates_member::newSocket()
{
    QSslSocket *socket = new QSslSocket;

    proxyAuthCalled = 0;
    connect(socket, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            Qt::DirectConnection);

    return QSslSocketPtr(socket);
}
#endif

void tst_QSslSocket_onDemandCertificates_member::proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *auth)
{
    ++proxyAuthCalled;
    auth->setUser("qsockstest");
    auth->setPassword("password");
}

#ifndef QT_NO_OPENSSL

static bool waitForEncrypted(QSslSocket *socket)
{
    Q_ASSERT(socket);

    QEventLoop eventLoop;

    QTimer connectionTimeoutWatcher;
    connectionTimeoutWatcher.setSingleShot(true);
    connectionTimeoutWatcher.connect(&connectionTimeoutWatcher, &QTimer::timeout,
                                     [&eventLoop]() {
                                        eventLoop.exit();
                                     });

    bool encrypted = false;
    socket->connect(socket, &QSslSocket::encrypted, [&eventLoop, &encrypted](){
                        eventLoop.exit();
                        encrypted = true;
                    });

    socket->connect(socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
                    [&eventLoop](){
                        eventLoop.exit();
                    });

    // Wait for 30 s. maximum - the default timeout in our QSslSocket::waitForEncrypted ...
    connectionTimeoutWatcher.start(30000);
    eventLoop.exec();
    return encrypted;
}

void tst_QSslSocket_onDemandCertificates_member::onDemandRootCertLoadingMemberMethods()
{
#define ERR(socket) socket->errorString().toLatin1()
    const QString host("www.qt.io");

    // not using any root certs -> should not work
    QSslSocketPtr socket2 = newSocket();
    this->socket = socket2.data();
    auto sslConfig = socket2->sslConfiguration();
    sslConfig.setCaCertificates(QList<QSslCertificate>());
    socket2->setSslConfiguration(sslConfig);
    socket2->connectToHostEncrypted(host, 443);
    QVERIFY2(!waitForEncrypted(socket2.data()), ERR(socket2));

    // default: using on demand loading -> should work
    QSslSocketPtr socket = newSocket();
    this->socket = socket.data();
    socket->connectToHostEncrypted(host, 443);
    QVERIFY2(waitForEncrypted(socket.data()), ERR(socket));

    // not using any root certs again -> should not work
    QSslSocketPtr socket3 = newSocket();
    this->socket = socket3.data();
    sslConfig = socket3->sslConfiguration();
    sslConfig.setCaCertificates(QList<QSslCertificate>());
    socket3->setSslConfiguration(sslConfig);
    socket3->connectToHostEncrypted(host, 443);
    QVERIFY2(!waitForEncrypted(socket3.data()), ERR(socket3));

    // setting empty SSL configuration explicitly -> depends on on-demand loading
    QSslSocketPtr socket4 = newSocket();
    this->socket = socket4.data();
    QSslConfiguration conf;
    socket4->setSslConfiguration(conf);
    socket4->connectToHostEncrypted(host, 443);
#ifdef QT_BUILD_INTERNAL
    const bool works = QSslSocketPrivate::rootCertOnDemandLoadingSupported();
#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
    QVERIFY2(works, ERR(socket4));
#elif defined(Q_OS_MAC)
    QVERIFY2(!works, ERR(socket4));
#endif // other platforms: undecided.
    // When we *allow* on-demand loading, we enable it by default; so, on Unix,
    // it will work without setting any certificates.  Otherwise, the configuration
    // contains an empty set of certificates, so on-demand loading shall fail.
    const bool result = waitForEncrypted(socket4.data());
    if (result != works)
        qDebug() << socket4->errorString();
    QCOMPARE(waitForEncrypted(socket4.data()), works);
#endif // QT_BUILD_INTERNAL
}
#undef ERR

#endif // QT_NO_OPENSSL

QTEST_MAIN(tst_QSslSocket_onDemandCertificates_member)
#include "tst_qsslsocket_onDemandCertificates_member.moc"
