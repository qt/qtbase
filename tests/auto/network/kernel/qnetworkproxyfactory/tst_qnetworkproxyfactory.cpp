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


#include <QtTest/QTest>
#include <QtTest/QTestEventLoop>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qnetworkproxy.h>

#include <QNetworkAccessManager>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QList>
#include <QSysInfo>
#include <QThread>

#include <private/qtnetworkglobal_p.h>

class tst_QNetworkProxyFactory : public QObject {
    Q_OBJECT

public:
    tst_QNetworkProxyFactory();

    class QDebugProxyFactory : public QNetworkProxyFactory
    {
    public:
        virtual QList<QNetworkProxy> queryProxy(const QNetworkProxyQuery &query = QNetworkProxyQuery())
        {
            returnedList = QNetworkProxyFactory::systemProxyForQuery(query);
            requestCounter++;
            return returnedList;
        }
        QList<QNetworkProxy> returnedList;

        static int requestCounter;
    };

private slots:
    void systemProxyForQueryCalledFromThread();
    void systemProxyForQuery_data();
    void systemProxyForQuery() const;
    void systemProxyForQuery_local();
    void genericSystemProxy();
    void genericSystemProxy_data();

private:
    QString formatProxyName(const QNetworkProxy & proxy) const;
    QDebugProxyFactory *factory;
};

int tst_QNetworkProxyFactory::QDebugProxyFactory::requestCounter = 0;

tst_QNetworkProxyFactory::tst_QNetworkProxyFactory()
{
    factory = new QDebugProxyFactory;
    QNetworkProxyFactory::setApplicationProxyFactory(factory);
}

QString tst_QNetworkProxyFactory::formatProxyName(const QNetworkProxy & proxy) const
{
    QString proxyName;
    if (!proxy.user().isNull())
        proxyName.append(QString("%1:%2@").arg(proxy.user(), proxy.password()));
    proxyName.append(QString("%1:%2").arg(proxy.hostName()).arg(proxy.port()));
    proxyName.append(QString(" (type=%1, capabilities=%2)").arg(proxy.type()).arg(proxy.capabilities()));

    return proxyName;
}

void tst_QNetworkProxyFactory::systemProxyForQuery_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<QString>("tag");
    QTest::addColumn<QString>("hostName");
    QTest::addColumn<int>("port");
    QTest::addColumn<int>("requiredCapabilities");

    //URLs
    QTest::newRow("http") << (int)QNetworkProxyQuery::UrlRequest << QUrl("http://qt-project.org") << QString() << QString() << 0 << 0;
    //windows: "intranet" should be bypassed if "bypass proxy server for local addresses" is ticked
    QTest::newRow("intranet") << (int)QNetworkProxyQuery::UrlRequest << QUrl("http://qt-test-server") << QString() << QString() << 0 << 0;
    //windows: "intranet2" should be bypassed if "*.local" is in the exceptions list (advanced settings)
    QTest::newRow("intranet2") << (int)QNetworkProxyQuery::UrlRequest << QUrl("http://qt-test-server.local") << QString() << QString() << 0 << 0;
    QTest::newRow("https") << (int)QNetworkProxyQuery::UrlRequest << QUrl("https://qt-project.org") << QString() << QString() << 0 << (int)QNetworkProxy::TunnelingCapability;
    QTest::newRow("ftp") << (int)QNetworkProxyQuery::UrlRequest << QUrl("ftp://qt-project.org") << QString() << QString() << 0 << 0;

    //TCP
    QTest::newRow("imap") << (int)QNetworkProxyQuery::TcpSocket << QUrl() << QString() << QString("qt-project.org") << 0 << (int)QNetworkProxy::TunnelingCapability;
    QTest::newRow("autobind-server") << (int)QNetworkProxyQuery::TcpServer << QUrl() << QString() << QString() << 0 << (int)QNetworkProxy::ListeningCapability;
    QTest::newRow("web-server") << (int)QNetworkProxyQuery::TcpServer << QUrl() << QString() << QString() << 80 << (int)QNetworkProxy::ListeningCapability;
    //windows: these should be bypassed  if "bypass proxy server for local addresses" is ticked
    foreach (QHostAddress address, QNetworkInterface::allAddresses()) {
        QTest::newRow(qPrintable(address.toString())) << (int)QNetworkProxyQuery::TcpSocket << QUrl() << QString() << address.toString() << 0 << 0;
    }

    //UDP
    QTest::newRow("udp") << (int)QNetworkProxyQuery::UdpSocket << QUrl() << QString() << QString() << 0 << (int)QNetworkProxy::UdpTunnelingCapability;

    //Protocol tags
    QTest::newRow("http-tag") << (int)QNetworkProxyQuery::TcpSocket << QUrl() << QString("http") << QString("qt-project.org") << 80 << (int)QNetworkProxy::TunnelingCapability;
    QTest::newRow("ftp-tag") << (int)QNetworkProxyQuery::TcpSocket << QUrl() << QString("ftp") << QString("qt-project.org") << 21 << (int)QNetworkProxy::TunnelingCapability;
    QTest::newRow("https-tag") << (int)QNetworkProxyQuery::TcpSocket << QUrl() << QString("https") << QString("qt-project.org") << 443 << (int)QNetworkProxy::TunnelingCapability;
#ifdef Q_OS_WIN
    //in Qt 4.8, "socks" would get the socks proxy, but we don't want to enforce that for all platforms
    QTest::newRow("socks-tag") << (int)QNetworkProxyQuery::TcpSocket << QUrl() << QString("socks") << QString("qt-project.org") << 21 <<  (int)(QNetworkProxy::TunnelingCapability | QNetworkProxy::ListeningCapability);
#endif
    //windows: ssh is not a tag provided by the os, but any tunneling proxy is acceptable
    QTest::newRow("ssh-tag") << (int)QNetworkProxyQuery::TcpSocket << QUrl() << QString("ssh") << QString("qt-project.org") << 22 << (int)QNetworkProxy::TunnelingCapability;

    //Server protocol tags (ftp/http proxies are no good, we need socks or nothing)
    QTest::newRow("http-server-tag") << (int)QNetworkProxyQuery::TcpServer << QUrl() << QString("http") << QString() << 80 << (int)QNetworkProxy::ListeningCapability;
    QTest::newRow("ftp-server-tag") << (int)QNetworkProxyQuery::TcpServer << QUrl() << QString("ftp") << QString() << 21 << (int)QNetworkProxy::ListeningCapability;
    QTest::newRow("imap-server-tag") << (int)QNetworkProxyQuery::TcpServer << QUrl() << QString("imap") << QString() << 143 << (int)QNetworkProxy::ListeningCapability;

    //UDP protocol tag
    QTest::newRow("sip-udp-tag") << (int)QNetworkProxyQuery::UdpSocket << QUrl() << QString("sip") << QString("qt-project.org") << 5061 << (int)QNetworkProxy::UdpTunnelingCapability;
}

void tst_QNetworkProxyFactory::systemProxyForQuery() const
{
    QFETCH(int, type);
    QFETCH(QUrl, url);
    QFETCH(QString, tag);
    QFETCH(QString, hostName);
    QFETCH(int, port);
    QFETCH(int, requiredCapabilities);

    QNetworkProxyQuery query;

    switch (type) {
    case QNetworkProxyQuery::UrlRequest:
        query = QNetworkProxyQuery(url);
        break;
    case QNetworkProxyQuery::TcpSocket:
    case QNetworkProxyQuery::UdpSocket:
        query = QNetworkProxyQuery(hostName, port, tag, QNetworkProxyQuery::QueryType(type));
        break;
    case QNetworkProxyQuery::TcpServer:
        query = QNetworkProxyQuery(quint16(port), tag);
        break;
    }

    QElapsedTimer sw;
    sw.start();
    QList<QNetworkProxy> systemProxyList = QNetworkProxyFactory::systemProxyForQuery(query);
    qDebug() << sw.elapsed() << "ms";
    QVERIFY(!systemProxyList.isEmpty());

    // for manual comparison with system
    qDebug() << systemProxyList;

    foreach (const QNetworkProxy &proxy, systemProxyList) {
        QVERIFY((requiredCapabilities == 0) || (proxy.capabilities() & requiredCapabilities));
    }
}

void tst_QNetworkProxyFactory::systemProxyForQuery_local()
{
    QList<QNetworkProxy> list;
    const QString proxyHost("myproxy.test.com");

    // set an arbitrary proxy
    QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::HttpProxy, proxyHost, 80));
    factory = 0;

    // localhost
    list = QNetworkProxyFactory::proxyForQuery(QNetworkProxyQuery(QUrl("http://localhost/")));
    QVERIFY(list.isEmpty() || (list[0].type() == QNetworkProxy::NoProxy));
    list = QNetworkProxyFactory::proxyForQuery(QNetworkProxyQuery(QString("localhost"), 80));
    QVERIFY(list.isEmpty() || (list[0].type() == QNetworkProxy::NoProxy));

    // 127.0.0.1
    list = QNetworkProxyFactory::proxyForQuery(QNetworkProxyQuery(QUrl("http://127.0.0.1/")));
    QVERIFY(list.isEmpty() || (list[0].type() == QNetworkProxy::NoProxy));
    list = QNetworkProxyFactory::proxyForQuery(QNetworkProxyQuery(QString("127.0.0.1"), 80));
    QVERIFY(list.isEmpty() || (list[0].type() == QNetworkProxy::NoProxy));

    // [::1]
    list = QNetworkProxyFactory::proxyForQuery(QNetworkProxyQuery(QUrl("http://[::1]/")));
    QVERIFY(list.isEmpty() || (list[0].type() == QNetworkProxy::NoProxy));
    list = QNetworkProxyFactory::proxyForQuery(QNetworkProxyQuery(QString("[::1]"), 80));
    QVERIFY(list.isEmpty() || (list[0].type() == QNetworkProxy::NoProxy));

    // an arbitrary host
    list = QNetworkProxyFactory::proxyForQuery(QNetworkProxyQuery(QUrl("http://another.host.com/")));
    QVERIFY((!list.isEmpty()) && (list[0].hostName() == proxyHost));
    list = QNetworkProxyFactory::proxyForQuery(QNetworkProxyQuery(QString("another.host.com"), 80));
    QVERIFY((!list.isEmpty()) && (list[0].hostName() == proxyHost));

    // disable proxy
    QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    factory = 0;

    // localhost
    list = QNetworkProxyFactory::proxyForQuery(QNetworkProxyQuery(QUrl("http://localhost/")));
    QVERIFY(list.isEmpty() || (list[0].type() == QNetworkProxy::NoProxy));
    list = QNetworkProxyFactory::proxyForQuery(QNetworkProxyQuery(QString("localhost"), 80));
    QVERIFY(list.isEmpty() || (list[0].type() == QNetworkProxy::NoProxy));

    // 127.0.0.1
    list = QNetworkProxyFactory::proxyForQuery(QNetworkProxyQuery(QUrl("http://127.0.0.1/")));
    QVERIFY(list.isEmpty() || (list[0].type() == QNetworkProxy::NoProxy));
    list = QNetworkProxyFactory::proxyForQuery(QNetworkProxyQuery(QString("127.0.0.1"), 80));
    QVERIFY(list.isEmpty() || (list[0].type() == QNetworkProxy::NoProxy));

    // [::1]
    list = QNetworkProxyFactory::proxyForQuery(QNetworkProxyQuery(QUrl("http://[::1]/")));
    QVERIFY(list.isEmpty() || (list[0].type() == QNetworkProxy::NoProxy));
    list = QNetworkProxyFactory::proxyForQuery(QNetworkProxyQuery(QString("[::1]"), 80));
    QVERIFY(list.isEmpty() || (list[0].type() == QNetworkProxy::NoProxy));

    // an arbitrary host
    list = QNetworkProxyFactory::proxyForQuery(QNetworkProxyQuery(QUrl("http://another.host.com/")));
    QVERIFY(list.isEmpty() || (list[0].type() == QNetworkProxy::NoProxy));
    list = QNetworkProxyFactory::proxyForQuery(QNetworkProxyQuery(QString("another.host.com"), 80));
    QVERIFY(list.isEmpty() || (list[0].type() == QNetworkProxy::NoProxy));
}

Q_DECLARE_METATYPE(QNetworkProxy::ProxyType)

void tst_QNetworkProxyFactory::genericSystemProxy()
{
    QFETCH(QByteArray, envVar);
    QFETCH(QByteArray, url);
    QFETCH(QNetworkProxy::ProxyType, proxyType);
    QFETCH(QString, hostName);
    QFETCH(int, port);

// We can only use the generic system proxy where available:
#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS) && !QT_CONFIG(libproxy)
    qputenv(envVar, url);
    const QList<QNetworkProxy> systemProxy = QNetworkProxyFactory::systemProxyForQuery();
    QCOMPARE(systemProxy.size(), 1);
    QCOMPARE(systemProxy.first().type(), proxyType);
    QCOMPARE(systemProxy.first().hostName(), hostName);
    QCOMPARE(systemProxy.first().port(), static_cast<quint16>(port));
    qunsetenv(envVar);
#else
    Q_UNUSED(envVar)
    Q_UNUSED(url)
    Q_UNUSED(proxyType)
    Q_UNUSED(hostName)
    Q_UNUSED(port)
    QSKIP("Generic system proxy not available on this platform.");
#endif
}

void tst_QNetworkProxyFactory::genericSystemProxy_data()
{
    QTest::addColumn<QByteArray>("envVar");
    QTest::addColumn<QByteArray>("url");
    QTest::addColumn<QNetworkProxy::ProxyType>("proxyType");
    QTest::addColumn<QString>("hostName");
    QTest::addColumn<int>("port");

    QTest::newRow("no proxy") << QByteArray("http_proxy") << QByteArray() << QNetworkProxy::NoProxy
                              << QString() << 0;
    QTest::newRow("socks5") << QByteArray("http_proxy") << QByteArray("socks5://127.0.0.1:4242")
                            << QNetworkProxy::Socks5Proxy << QString("127.0.0.1") << 4242;
    QTest::newRow("http") << QByteArray("http_proxy") << QByteArray("http://example.com:666")
                          << QNetworkProxy::HttpProxy << QString("example.com") << 666;
}

class QSPFQThread : public QThread
{
protected:
    virtual void run()
    {
        proxies = QNetworkProxyFactory::systemProxyForQuery(query);
    }
public:
    QNetworkProxyQuery query;
    QList<QNetworkProxy> proxies;
};

//regression test for QTBUG-18799
void tst_QNetworkProxyFactory::systemProxyForQueryCalledFromThread()
{
    if (QSysInfo::productType() == QLatin1String("windows") && QSysInfo::productVersion() == QLatin1String("7sp1")) {
        QSKIP("This test fails by the systemProxyForQuery() call hanging - QTQAINFRA-1200");
    }
    QUrl url(QLatin1String("http://qt-project.org"));
    QNetworkProxyQuery query(url);
    QSPFQThread thread;
    thread.query = query;
    connect(&thread, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));
    thread.start();
    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(thread.isFinished());
    QCOMPARE(thread.proxies, QNetworkProxyFactory::systemProxyForQuery(query));
}

QTEST_MAIN(tst_QNetworkProxyFactory)
#include "tst_qnetworkproxyfactory.moc"
