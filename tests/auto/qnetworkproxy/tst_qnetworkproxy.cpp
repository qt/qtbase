/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qnetworkproxy.h>

//TESTED_CLASS=
//TESTED_FILES=

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
