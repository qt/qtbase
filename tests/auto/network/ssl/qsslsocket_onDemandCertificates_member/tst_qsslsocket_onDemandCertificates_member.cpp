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


#include <QtNetwork>
#include <QtTest/QtTest>

#include <QNetworkProxy>
#include <QAuthenticator>

#ifdef QT_BUILD_INTERNAL
#  include "private/qhostinfo_p.h"
#  ifndef QT_NO_OPENSSL
#    include "private/qsslsocket_p.h"
#  endif // !QT_NO_OPENSSL
#endif // QT_BUILD_INTERNAL
#include "../../../network-settings.h"

#ifndef QT_NO_OPENSSL
typedef QSharedPointer<QSslSocket> QSslSocketPtr;
#endif

class tst_QSslSocket_onDemandCertificates_member : public QObject
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
    void onDemandRootCertLoadingMemberMethods();

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
    QVERIFY(QtNetworkSettings::verifyTestNetworkSettings());
}

void tst_QSslSocket_onDemandCertificates_member::init()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
        QFETCH_GLOBAL(int, proxyType);
        QString testServer = QHostInfo::fromName(QtNetworkSettings::serverName()).addresses().first().toString();
        QNetworkProxy proxy;

        switch (proxyType) {
        case Socks5Proxy:
            proxy = QNetworkProxy(QNetworkProxy::Socks5Proxy, testServer, 1080);
            break;

        case Socks5Proxy | AuthBasic:
            proxy = QNetworkProxy(QNetworkProxy::Socks5Proxy, testServer, 1081);
            break;

        case HttpProxy | NoAuth:
            proxy = QNetworkProxy(QNetworkProxy::HttpProxy, testServer, 3128);
            break;

        case HttpProxy | AuthBasic:
            proxy = QNetworkProxy(QNetworkProxy::HttpProxy, testServer, 3129);
            break;

        case HttpProxy | AuthNtlm:
            proxy = QNetworkProxy(QNetworkProxy::HttpProxy, testServer, 3130);
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

void tst_QSslSocket_onDemandCertificates_member::onDemandRootCertLoadingMemberMethods()
{
    QString host("www.qt.io");

    // not using any root certs -> should not work
    QSslSocketPtr socket2 = newSocket();
    this->socket = socket2.data();
    socket2->setCaCertificates(QList<QSslCertificate>());
    socket2->connectToHostEncrypted(host, 443);
    QVERIFY(!socket2->waitForEncrypted());

    // default: using on demand loading -> should work
    QSslSocketPtr socket = newSocket();
    this->socket = socket.data();
    socket->connectToHostEncrypted(host, 443);
    QVERIFY2(socket->waitForEncrypted(), qPrintable(socket->errorString()));

    // not using any root certs again -> should not work
    QSslSocketPtr socket3 = newSocket();
    this->socket = socket3.data();
    socket3->setCaCertificates(QList<QSslCertificate>());
    socket3->connectToHostEncrypted(host, 443);
    QVERIFY(!socket3->waitForEncrypted());

    // setting empty SSL configuration explicitly -> depends on on-demand loading
    QSslSocketPtr socket4 = newSocket();
    this->socket = socket4.data();
    QSslConfiguration conf;
    socket4->setSslConfiguration(conf);
    socket4->connectToHostEncrypted(host, 443);
#ifdef QT_BUILD_INTERNAL
    bool rootCertLoadingAllowed = QSslSocketPrivate::rootCertOnDemandLoadingSupported();
#if defined(Q_OS_LINUX)
    QCOMPARE(rootCertLoadingAllowed, true);
#elif defined(Q_OS_MAC)
    QCOMPARE(rootCertLoadingAllowed, false);
#endif // other platforms: undecided (Windows: depends on the version)
    // when we allow on demand loading, it is enabled by default,
    // so on Unix it will work without setting any certificates. Otherwise,
    // the configuration contains an empty set of certificates
    // and will fail.
    bool works;
#if defined (Q_OS_WIN)
    works = false; // on Windows, this won't work even though we use on demand loading
    Q_UNUSED(rootCertLoadingAllowed)
#else
    works = rootCertLoadingAllowed;
#endif
    QCOMPARE(socket4->waitForEncrypted(), works);
#endif // QT_BUILD_INTERNAL
}

#endif // QT_NO_OPENSSL

QTEST_MAIN(tst_QSslSocket_onDemandCertificates_member)
#include "tst_qsslsocket_onDemandCertificates_member.moc"
