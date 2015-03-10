/****************************************************************************
**
** Copyright (C) 2016 Intel Corporation.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#include <QNetworkDatagram>
#include <QtTest>
#include <QCoreApplication>

class tst_QNetworkDatagram : public QObject
{
    Q_OBJECT

public:
    tst_QNetworkDatagram();

private Q_SLOTS:
    void getSetCheck();
    void makeReply_data();
    void makeReply();
};

tst_QNetworkDatagram::tst_QNetworkDatagram()
{
}

void tst_QNetworkDatagram::getSetCheck()
{
    QNetworkDatagram dg;

    QVERIFY(dg.isNull());
    QVERIFY(!dg.isValid());
    QCOMPARE(dg.senderAddress(), QHostAddress());
    QCOMPARE(dg.destinationAddress(), QHostAddress());
    QCOMPARE(dg.senderPort(), -1);
    QCOMPARE(dg.destinationPort(), -1);
    QCOMPARE(dg.hopLimit(), -1);
    QCOMPARE(dg.interfaceIndex(), 0U);

    dg.setHopLimit(1);
    QCOMPARE(dg.hopLimit(), 1);
    dg.setHopLimit(255);
    QCOMPARE(dg.hopLimit(), 255);

    dg.setInterfaceIndex(1);
    QCOMPARE(dg.interfaceIndex(), 1U);
    dg.setInterfaceIndex(1234567U);
    QCOMPARE(dg.interfaceIndex(), 1234567U);

    dg.setSender(QHostAddress::Any, 12345);
    QCOMPARE(dg.senderAddress(), QHostAddress(QHostAddress::Any));
    QCOMPARE(dg.senderPort(), 12345);
    dg.setSender(QHostAddress::LocalHost);
    QCOMPARE(dg.senderAddress(), QHostAddress(QHostAddress::LocalHost));
    QCOMPARE(dg.senderPort(), 0);

    dg.setDestination(QHostAddress::LocalHostIPv6, 12345);
    QCOMPARE(dg.destinationAddress(), QHostAddress(QHostAddress::LocalHostIPv6));
    QCOMPARE(dg.destinationPort(), 12345);
    dg.setDestination(QHostAddress::Broadcast, 137);
    QCOMPARE(dg.destinationAddress(), QHostAddress(QHostAddress::Broadcast));
    QCOMPARE(dg.destinationPort(), 137);
}

void tst_QNetworkDatagram::makeReply_data()
{
    qRegisterMetaType<QNetworkDatagram>();
    QTest::addColumn<QNetworkDatagram>("dgram");
    QTest::addColumn<QString>("localAddress");

    QNetworkDatagram dgram("some data", QHostAddress("192.0.2.1"), 10001);
    dgram.setHopLimit(64);
    dgram.setSender(QHostAddress::LocalHost, 12345);
    QTest::newRow("ipv4") << dgram << "192.0.2.1";

    dgram.setDestination(QHostAddress("224.0.0.1"), 10002);
    QTest::newRow("ipv4-multicast") << dgram << QString();

    dgram.setSender(QHostAddress::LocalHostIPv6, 12346);
    dgram.setDestination(QHostAddress("2001:db8::1"), 12347);
    QTest::newRow("ipv6") << dgram << "2001:db8::1";

    dgram.setSender(QHostAddress("fe80::1%1"), 10003);
    dgram.setDestination(QHostAddress("fe80::2%1"), 10004);
    dgram.setInterfaceIndex(1);
    QTest::newRow("ipv6-linklocal") << dgram << "fe80::2%1";

    dgram.setDestination(QHostAddress("ff02::1%1"), 10005);
    QTest::newRow("ipv6-multicast") << dgram << QString();
}

void tst_QNetworkDatagram::makeReply()
{
    QFETCH(QNetworkDatagram, dgram);
    QFETCH(QString, localAddress);

    {
        QNetworkDatagram reply = dgram.makeReply("World");
        QCOMPARE(reply.data(), QByteArray("World"));
        QCOMPARE(reply.senderAddress(), QHostAddress(localAddress));
        QCOMPARE(reply.senderPort(), localAddress.isEmpty() ? -1 : dgram.destinationPort());
        QCOMPARE(reply.destinationAddress(), dgram.senderAddress());
        QCOMPARE(reply.destinationPort(), dgram.senderPort());
        QCOMPARE(reply.interfaceIndex(), dgram.interfaceIndex());
        QCOMPARE(reply.hopLimit(), -1);
    }

    QNetworkDatagram copy = dgram;
    copy.setData(copy.data());
    {
        QNetworkDatagram reply = qMove(copy).makeReply("World");
        QCOMPARE(reply.data(), QByteArray("World"));
        QCOMPARE(reply.senderAddress(), QHostAddress(localAddress));
        QCOMPARE(reply.senderPort(), localAddress.isEmpty() ? -1 : dgram.destinationPort());
        QCOMPARE(reply.destinationAddress(), dgram.senderAddress());
        QCOMPARE(reply.destinationPort(), dgram.senderPort());
        QCOMPARE(reply.interfaceIndex(), dgram.interfaceIndex());
        QCOMPARE(reply.hopLimit(), -1);
    }
}

QTEST_MAIN(tst_QNetworkDatagram)
#include "tst_qnetworkdatagram.moc"
