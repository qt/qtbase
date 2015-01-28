/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

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
