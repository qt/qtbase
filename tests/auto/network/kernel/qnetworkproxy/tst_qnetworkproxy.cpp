// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qnetworkproxy.h>

class tst_QNetworkProxy : public QObject
{
Q_OBJECT

public:
    tst_QNetworkProxy();
    virtual ~tst_QNetworkProxy();

private slots:
    void getSetCheck();
    void capabilitiesPerType();
};

tst_QNetworkProxy::tst_QNetworkProxy()
{
}

tst_QNetworkProxy::~tst_QNetworkProxy()
{
}

// Testing get/set functions
void tst_QNetworkProxy::getSetCheck()
{
    QNetworkProxy obj1;
    // quint16 QNetworkProxy::port()
    // void QNetworkProxy::setPort(quint16)
    obj1.setPort(quint16(0));
    QCOMPARE(quint16(0), obj1.port());
    obj1.setPort(quint16(0xffff));
    QCOMPARE(quint16(0xffff), obj1.port());

    obj1.setType(QNetworkProxy::DefaultProxy);
    QCOMPARE(obj1.type(), QNetworkProxy::DefaultProxy);
    obj1.setType(QNetworkProxy::HttpProxy);
    QCOMPARE(obj1.type(), QNetworkProxy::HttpProxy);
    obj1.setType(QNetworkProxy::Socks5Proxy);
    QCOMPARE(obj1.type(), QNetworkProxy::Socks5Proxy);
}

void tst_QNetworkProxy::capabilitiesPerType()
{
    QNetworkProxy proxy(QNetworkProxy::Socks5Proxy);
    QVERIFY(proxy.capabilities() & QNetworkProxy::TunnelingCapability);
    QVERIFY(proxy.capabilities() & QNetworkProxy::HostNameLookupCapability);
    QVERIFY(proxy.capabilities() & QNetworkProxy::UdpTunnelingCapability);

    proxy.setType(QNetworkProxy::NoProxy);
    // verify that the capabilities changed
    QVERIFY(!(proxy.capabilities() & QNetworkProxy::HostNameLookupCapability));
    QVERIFY(proxy.capabilities() & QNetworkProxy::UdpTunnelingCapability);

    proxy.setType(QNetworkProxy::HttpProxy);
    QVERIFY(proxy.capabilities() & QNetworkProxy::HostNameLookupCapability);
    QVERIFY(!(proxy.capabilities() & QNetworkProxy::UdpTunnelingCapability));

    // now set the capabilities on stone:
    proxy.setCapabilities(QNetworkProxy::TunnelingCapability | QNetworkProxy::UdpTunnelingCapability);
    QCOMPARE(proxy.capabilities(), QNetworkProxy::TunnelingCapability | QNetworkProxy::UdpTunnelingCapability);

    // changing the type shouldn't change the capabilities any more
    proxy.setType(QNetworkProxy::Socks5Proxy);
    QCOMPARE(proxy.capabilities(), QNetworkProxy::TunnelingCapability | QNetworkProxy::UdpTunnelingCapability);
}

QTEST_MAIN(tst_QNetworkProxy)
#include "tst_qnetworkproxy.moc"
