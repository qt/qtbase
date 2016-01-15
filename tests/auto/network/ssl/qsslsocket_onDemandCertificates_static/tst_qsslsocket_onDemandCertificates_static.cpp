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
    QVERIFY(QtNetworkSettings::verifyTestNetworkSettings());
}

void tst_QSslSocket_onDemandCertificates_static::init()
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

    // not using any root certs -> should not work
    QSslSocket::setDefaultCaCertificates(QList<QSslCertificate>());
    QSslSocketPtr socket = newSocket();
    this->socket = socket.data();
    socket->connectToHostEncrypted(host, 443);
    QVERIFY(!socket->waitForEncrypted());

    // using system root certs -> should work
    QSslSocket::setDefaultCaCertificates(QSslSocket::systemCaCertificates());
    QSslSocketPtr socket2 = newSocket();
    this->socket = socket2.data();
    socket2->connectToHostEncrypted(host, 443);
    QVERIFY2(socket2->waitForEncrypted(), qPrintable(socket2->errorString()));

    // not using any root certs again -> should not work
    QSslSocket::setDefaultCaCertificates(QList<QSslCertificate>());
    QSslSocketPtr socket3 = newSocket();
    this->socket = socket3.data();
    socket3->connectToHostEncrypted(host, 443);
    QVERIFY(!socket3->waitForEncrypted());

    QSslSocket::setDefaultCaCertificates(QSslSocket::systemCaCertificates());

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
