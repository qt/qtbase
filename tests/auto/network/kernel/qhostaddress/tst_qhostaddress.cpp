/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include <qcoreapplication.h>
#include <QtTest/QtTest>
#include <qhostaddress.h>
#include <qplatformdefs.h>
#include <qdebug.h>
#include <qhash.h>
#include <qbytearray.h>
#include <qdatastream.h>
#ifdef Q_OS_WIN
#  include <qt_windows.h>
#  if defined(Q_OS_WINRT)
#    include <winsock2.h>
#  endif
#endif

#ifdef Q_OS_ANDROID
#  include <netinet/in.h>
#endif

Q_DECLARE_METATYPE(QHostAddress::SpecialAddress)

class tst_QHostAddress : public QObject
{
    Q_OBJECT

public:
    tst_QHostAddress();

private slots:
    void constructor_QString_data();
    void constructor_QString();
    void setAddress_QString_data();
    void setAddress_QString();
    void specialAddresses_data();
    void specialAddresses();
    void compare_data();
    void compare();
    void isEqual_data();
    void isEqual();
    void assignment();
    void scopeId();
    void hashKey();
    void streaming_data();
    void streaming();
    void parseSubnet_data();
    void parseSubnet();
    void isInSubnet_data();
    void isInSubnet();
    void isLoopback_data();
    void isLoopback();
    void isMulticast_data();
    void isMulticast();
    void convertv4v6_data();
    void convertv4v6();
};

Q_DECLARE_METATYPE(QHostAddress)

tst_QHostAddress::tst_QHostAddress()
{
    qRegisterMetaType<QHostAddress>("QHostAddress");
}

void tst_QHostAddress::constructor_QString_data()
{
    setAddress_QString_data();
}

void tst_QHostAddress::constructor_QString()
{
    QFETCH(QString, address);
    QFETCH(bool, ok);
    QFETCH(int, protocol);

    QHostAddress hostAddr(address);

    if (address == "0.0.0.0" || address == "::") {
        QVERIFY(ok);
    } else {
        QVERIFY(hostAddr.isNull() != ok);
    }

    if (ok)
        QTEST(hostAddr.toString(), "resAddr");

    if ( protocol == 4 ) {
        QVERIFY( hostAddr.protocol() == QAbstractSocket::IPv4Protocol || hostAddr.protocol() == QAbstractSocket::UnknownNetworkLayerProtocol );
        QVERIFY( hostAddr.protocol() != QAbstractSocket::IPv6Protocol );
    } else if ( protocol == 6 ) {
        QVERIFY( hostAddr.protocol() != QAbstractSocket::IPv4Protocol && hostAddr.protocol() != QAbstractSocket::UnknownNetworkLayerProtocol );
        QVERIFY( hostAddr.protocol() == QAbstractSocket::IPv6Protocol );
    } else {
        QVERIFY( hostAddr.isNull() );
        QVERIFY( hostAddr.protocol() == QAbstractSocket::UnknownNetworkLayerProtocol );
    }
}

void tst_QHostAddress::setAddress_QString_data()
{
    QTest::addColumn<QString>("address");
    QTest::addColumn<bool>("ok");
    QTest::addColumn<QString>("resAddr");
    QTest::addColumn<int>("protocol"); // 4: IPv4, 6: IPv6, other: undefined

    //next we fill it with data
    QTest::newRow("ip4_00")  << QString("127.0.0.1") << true << QString("127.0.0.1") << 4;
    QTest::newRow("ip4_01")  << QString("255.3.2.1") << true << QString("255.3.2.1") << 4;
    QTest::newRow("ip4_03")  << QString(" 255.3.2.1") << true << QString("255.3.2.1") << 4;
    QTest::newRow("ip4_04")  << QString("255.3.2.1\r ") << true << QString("255.3.2.1") << 4;
    QTest::newRow("ip4_05")  << QString("0.0.0.0") << true << QString("0.0.0.0") << 4;
    QTest::newRow("ip4_06")  << QString("123.0.0") << true << QString("123.0.0.0") << 4;

    // for the format of IPv6 addresses see also RFC 5952
    // rule 4.1: Leading zeros MUST be suppressed
    // rule 4.2.1: Shorten as Much as Possible
    // rule 4.2.2: The symbol "::" MUST NOT be used to shorten just one 16-bit 0 field.
    // rule 4.2.3: the longest run of consecutive 16-bit 0 fields MUST be shortened
    //             When the length of the consecutive 16-bit 0 fields, the first sequence
    //             of zero bits MUST be shortened
    // rule 4.3: The characters "a", "b", "c", "d", "e", and "f" in an IPv6 address
    //           MUST be represented in lowercase
    QTest::newRow("ip6_00")  << QString("FEDC:BA98:7654:3210:FEDC:BA98:7654:3210") << true << QString("fedc:ba98:7654:3210:fedc:ba98:7654:3210") << 6; // 4.3
    QTest::newRow("ip6_01")  << QString("1080:0000:0000:0000:0008:0800:200C:417A") << true << QString("1080::8:800:200c:417a") << 6; // 4.1, 4.2.1
    QTest::newRow("ip6_02")  << QString("1080:0:0:0:8:800:200C:417A") << true << QString("1080::8:800:200c:417a") << 6;
    QTest::newRow("ip6_03")  << QString("1080::8:800:200C:417A") << true << QString("1080::8:800:200c:417a") << 6;
    QTest::newRow("ip6_04")  << QString("FF01::43") << true << QString("ff01::43") << 6;
    QTest::newRow("ip6_05")  << QString("::1") << true << QString("::1") << 6;
    QTest::newRow("ip6_06")  << QString("1::") << true << QString("1::") << 6;
    QTest::newRow("ip6_07")  << QString("::") << true << QString("::") << 6;
    QTest::newRow("ip6_08")  << QString("0:0:0:0:0:0:13.1.68.3") << true << QString("::13.1.68.3") << 6;
    QTest::newRow("ip6_09")  << QString("::13.1.68.3") << true <<  QString("::13.1.68.3") << 6;
    QTest::newRow("ip6_10")  << QString("0:0:0:0:0:FFFF:129.144.52.38") << true << QString("::ffff:129.144.52.38") << 6;
    QTest::newRow("ip6_11")  << QString("::FFFF:129.144.52.38") << true << QString("::ffff:129.144.52.38") << 6;
    QTest::newRow("ip6_12")  << QString("1::FFFF:129.144.52.38") << true << QString("1::ffff:8190:3426") << 6;
    QTest::newRow("ip6_13")  << QString("A:B::D:E") << true << QString("a:b::d:e") << 6;
    QTest::newRow("ip6_14")  << QString("1080:0:1:0:8:800:200C:417A") << true << QString("1080:0:1:0:8:800:200c:417a") << 6; // 4.2.2
    QTest::newRow("ip6_15")  << QString("1080:0:1:0:8:800:200C:0") << true << QString("1080:0:1:0:8:800:200c:0") << 6;
    QTest::newRow("ip6_16")  << QString("1080:0:1:0:8:800:0:0") << true << QString("1080:0:1:0:8:800::") << 6;
    QTest::newRow("ip6_17a") << QString("1080:0:0:8:800:0:0:0") << true << QString("1080:0:0:8:800::") << 6; // 4.2.3a
    QTest::newRow("ip6_17b") << QString("1080:0:0:0:8:0:0:0") << true << QString("1080::8:0:0:0") << 6; // 4.2.3b
    QTest::newRow("ip6_18")  << QString("0:1:1:1:8:800:0:0") << true << QString("0:1:1:1:8:800::") << 6;
    QTest::newRow("ip6_19")  << QString("0:1:1:1:8:800:0:1") << true << QString("0:1:1:1:8:800:0:1") << 6;

    QTest::newRow("error_00")  << QString("foobarcom") << false << QString() << 0;
    QTest::newRow("error_01")  << QString("foo.bar.com") << false << QString() << 0;
    QTest::newRow("error_02")  << QString("") << false << QString() << 0;
    QTest::newRow("error_03")  << QString() << false << QString() << 0;
    QTest::newRow("error_04")  << QString(" \t\r") << false << QString() << 0;

    QTest::newRow("error_ip4_00")  << QString("256.9.9.9") << false << QString() << 0;
    QTest::newRow("error_ip4_01")  << QString("-1.9.9.9") << false << QString() << 0;
    //QTest::newRow("error_ip4_02")  << QString("123.0.0") << false << QString() << 0; // no longer invalid in Qt5
    QTest::newRow("error_ip4_02")  << QString("123.0.0.") << false << QString() << 0;
    QTest::newRow("error_ip4_03")  << QString("123.0.0.0.0") << false << QString() << 0;
    QTest::newRow("error_ip4_04")  << QString("255.2 3.2.1") << false << QString() << 0;

    QTest::newRow("error_ip6_00")  << QString(":") << false << QString() << 0;
    QTest::newRow("error_ip6_01")  << QString(":::") << false << QString() << 0;
    QTest::newRow("error_ip6_02")  << QString("::AAAA:") << false << QString() << 0;
    QTest::newRow("error_ip6_03")  << QString(":AAAA::") << false << QString() << 0;
    QTest::newRow("error_ip6_04")  << QString("FFFF:::129.144.52.38") << false << QString() << 0;
    QTest::newRow("error_ip6_05")  << QString("FEDC:BA98:7654:3210:FEDC:BA98:7654:3210:1234") << false << QString() << 0;
    QTest::newRow("error_ip6_06")  << QString("129.144.52.38::") << false << QString() << 0;
    QTest::newRow("error_ip6_07")  << QString("::129.144.52.38:129.144.52.38") << false << QString() << 0;
    QTest::newRow("error_ip6_08")  << QString(":::129.144.52.38") << false << QString() << 0;
    QTest::newRow("error_ip6_09")  << QString("1FEDC:BA98:7654:3210:FEDC:BA98:7654:3210") << false << QString() << 0;
    QTest::newRow("error_ip6_10")  << QString("::FFFFFFFF") << false << QString() << 0;
    QTest::newRow("error_ip6_11")  << QString("::EFGH") << false << QString() << 0;
    QTest::newRow("error_ip6_12")  << QString("ABCD:ABCD:ABCD") << false << QString() << 0;
    QTest::newRow("error_ip6_13")  << QString("::ABCD:ABCD::") << false << QString() << 0;
    QTest::newRow("error_ip6_14")  << QString("1::2::3") << false << QString() << 0;
    QTest::newRow("error_ip6_15")  << QString("1:2:::") << false << QString() << 0;
    QTest::newRow("error_ip6_16")  << QString(":::1:2") << false << QString() << 0;
    QTest::newRow("error_ip6_17")  << QString("1:::2") << false << QString() << 0;
    QTest::newRow("error_ip6_18")  << QString("FEDC::7654:3210:FEDC:BA98::3210") << false << QString() << 0;
    QTest::newRow("error_ip6_19")  << QString("ABCD:ABCD:ABCD:1.2.3.4") << false << QString() << 0;
    QTest::newRow("error_ip6_20")  << QString("ABCD::ABCD::ABCD:1.2.3.4") << false << QString() << 0;

}

void tst_QHostAddress::setAddress_QString()
{
    QFETCH(QString, address);
    QFETCH(bool, ok);
    QFETCH(int, protocol);

    QHostAddress hostAddr;
    QCOMPARE(hostAddr.setAddress(address), ok);

    if (ok)
        QTEST(hostAddr.toString(), "resAddr");

    if ( protocol == 4 ) {
        QVERIFY( hostAddr.protocol() == QAbstractSocket::IPv4Protocol || hostAddr.protocol() == QAbstractSocket::UnknownNetworkLayerProtocol );
        QVERIFY( hostAddr.protocol() != QAbstractSocket::IPv6Protocol );
    } else if ( protocol == 6 ) {
        QVERIFY( hostAddr.protocol() != QAbstractSocket::IPv4Protocol && hostAddr.protocol() != QAbstractSocket::UnknownNetworkLayerProtocol );
        QVERIFY( hostAddr.protocol() == QAbstractSocket::IPv6Protocol );
    } else {
        QVERIFY( hostAddr.isNull() );
        QVERIFY( hostAddr.protocol() == QAbstractSocket::UnknownNetworkLayerProtocol );
    }
}

void tst_QHostAddress::specialAddresses_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QHostAddress::SpecialAddress>("address");
    QTest::addColumn<bool>("result");

    QTest::newRow("localhost_1") << QString("127.0.0.1") << QHostAddress::LocalHost << true;
    QTest::newRow("localhost_2") << QString("127.0.0.2") << QHostAddress::LocalHost << false;
    QTest::newRow("localhost_3") << QString("127.0.0.2") << QHostAddress::LocalHostIPv6 << false;

    QTest::newRow("localhost_ipv6_4") << QString("::1") << QHostAddress::LocalHostIPv6 << true;
    QTest::newRow("localhost_ipv6_5") << QString("::2") << QHostAddress::LocalHostIPv6 << false;
    QTest::newRow("localhost_ipv6_6") << QString("::1") << QHostAddress::LocalHost << false;

    QTest::newRow("null_1") << QString("") << QHostAddress::Null << true;
    QTest::newRow("null_2") << QString("bjarne") << QHostAddress::Null << true;

    QTest::newRow("compare_from_null") << QString("") << QHostAddress::Broadcast << false;

    QTest::newRow("broadcast_1") << QString("255.255.255.255") << QHostAddress::Any << false;
    QTest::newRow("broadcast_2") << QString("255.255.255.255") << QHostAddress::Broadcast << true;

    QTest::newRow("any_ipv6") << QString("::") << QHostAddress::AnyIPv6 << true;
    QTest::newRow("any_ipv4") << QString("0.0.0.0") << QHostAddress::AnyIPv4 << true;

    QTest::newRow("dual_not_ipv6") << QString("::") << QHostAddress::Any << false;
    QTest::newRow("dual_not_ipv4") << QString("0.0.0.0") << QHostAddress::Any << false;
}


void tst_QHostAddress::specialAddresses()
{
    QFETCH(QString, text);
    QFETCH(QHostAddress::SpecialAddress, address);
    QFETCH(bool, result);
    QCOMPARE(QHostAddress(text) == address, result);

    //check special address equal to itself (QTBUG-22898), note two overloads of operator==
    QVERIFY(QHostAddress(address) == QHostAddress(address));
    QVERIFY(QHostAddress(address) == address);
    QVERIFY(!(QHostAddress(address) != QHostAddress(address)));
    QVERIFY(!(QHostAddress(address) != address));

    {
        QHostAddress ha;
        ha.setAddress(address);
        QVERIFY(ha == address);
    }

    QHostAddress setter;
    setter.setAddress(text);
    QCOMPARE(setter == address, result);
}


void tst_QHostAddress::compare_data()
{
    QTest::addColumn<QHostAddress>("first");
    QTest::addColumn<QHostAddress>("second");
    QTest::addColumn<bool>("result");

    QTest::newRow("1") << QHostAddress() << QHostAddress() << true;
    QTest::newRow("2") << QHostAddress(QHostAddress::Any) << QHostAddress(QHostAddress::Any) << true;
    QTest::newRow("3") << QHostAddress(QHostAddress::AnyIPv6) << QHostAddress(QHostAddress::AnyIPv6) << true;
    QTest::newRow("4") << QHostAddress(QHostAddress::Broadcast) << QHostAddress(QHostAddress::Broadcast) << true;
    QTest::newRow("5") << QHostAddress(QHostAddress::LocalHost) << QHostAddress(QHostAddress::Broadcast) << false;
    QTest::newRow("6") << QHostAddress(QHostAddress::LocalHost) << QHostAddress(QHostAddress::LocalHostIPv6) << false;
    QTest::newRow("7") << QHostAddress() << QHostAddress(QHostAddress::LocalHostIPv6) << false;
    QTest::newRow("any4-any6") << QHostAddress(QHostAddress::AnyIPv4) << QHostAddress(QHostAddress::AnyIPv6) << false;

    Q_IPV6ADDR localhostv4mapped = { { 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 255, 255,  127, 0, 0, 1 } };
    QTest::newRow("v4-v4mapped") << QHostAddress(QHostAddress::LocalHost) << QHostAddress("::ffff:127.0.0.1") << false;
    QTest::newRow("v4-v4mapped-2") << QHostAddress(QHostAddress::LocalHost) << QHostAddress(localhostv4mapped) << false;
}

void tst_QHostAddress::compare()
{
    QFETCH(QHostAddress, first);
    QFETCH(QHostAddress, second);
    QFETCH(bool, result);

    QCOMPARE(first == second, result);
    QCOMPARE(second == first, result);
    if (result == true)
        QCOMPARE(qHash(first), qHash(second));
}

void tst_QHostAddress::isEqual_data()
{
    QTest::addColumn<QHostAddress>("first");
    QTest::addColumn<QHostAddress>("second");
    QTest::addColumn<int>("flags");
    QTest::addColumn<bool>("result");

    // QHostAddress::StrictConversion is already tested in compare()
    QTest::newRow("localhost4to6-local") << QHostAddress(QHostAddress::LocalHost) << QHostAddress(QHostAddress::LocalHostIPv6) << (int)QHostAddress::ConvertLocalHost << true;
    QTest::newRow("localhost4to6-compat") << QHostAddress(QHostAddress::LocalHost) << QHostAddress(QHostAddress::LocalHostIPv6) << (int)QHostAddress::ConvertV4CompatToIPv4 << false;
    QTest::newRow("localhost4to6-mapped") << QHostAddress(QHostAddress::LocalHost) << QHostAddress(QHostAddress::LocalHostIPv6) << (int)QHostAddress::ConvertV4MappedToIPv4 << false;
    QTest::newRow("localhost4to6-unspec") << QHostAddress(QHostAddress::LocalHost) << QHostAddress(QHostAddress::LocalHostIPv6) << (int)QHostAddress::ConvertUnspecifiedAddress << false;
    QTest::newRow("0.0.0.1-::1-local") << QHostAddress("0.0.0.1") << QHostAddress(QHostAddress::LocalHostIPv6) << (int)QHostAddress::ConvertLocalHost << false;
    QTest::newRow("v4-v4compat-local") << QHostAddress("192.168.1.1") << QHostAddress("::192.168.1.1") << (int)QHostAddress::ConvertLocalHost << false;
    QTest::newRow("v4-v4mapped-local") << QHostAddress("192.168.1.1") << QHostAddress("::ffff:192.168.1.1") << (int)QHostAddress::ConvertLocalHost << false;
    QTest::newRow("0.0.0.1-::1-unspec") << QHostAddress("0.0.0.1") << QHostAddress(QHostAddress::LocalHostIPv6) << (int)QHostAddress::ConvertUnspecifiedAddress << false;
    QTest::newRow("v4-v4compat-unspec") << QHostAddress("192.168.1.1") << QHostAddress("::192.168.1.1") << (int)QHostAddress::ConvertUnspecifiedAddress << false;
    QTest::newRow("v4-v4mapped-unspec") << QHostAddress("192.168.1.1") << QHostAddress("::ffff:192.168.1.1") << (int)QHostAddress::ConvertUnspecifiedAddress << false;
    QTest::newRow("0.0.0.1-::1-compat") << QHostAddress("0.0.0.1") << QHostAddress(QHostAddress::LocalHostIPv6) << (int)QHostAddress::ConvertV4CompatToIPv4 << false;
    QTest::newRow("v4-v4compat-compat") << QHostAddress("192.168.1.1") << QHostAddress("::192.168.1.1") << (int)QHostAddress::ConvertV4CompatToIPv4 << true;
    QTest::newRow("v4-v4mapped-compat") << QHostAddress("192.168.1.1") << QHostAddress("::ffff:192.168.1.1") << (int)QHostAddress::ConvertV4CompatToIPv4 << false;
    QTest::newRow("0.0.0.1-::1-mapped") << QHostAddress("0.0.0.1") << QHostAddress(QHostAddress::LocalHostIPv6) << (int)QHostAddress::ConvertV4MappedToIPv4  << false;
    QTest::newRow("v4-v4compat-mapped") << QHostAddress("192.168.1.1") << QHostAddress("::192.168.1.1") << (int)QHostAddress::ConvertV4MappedToIPv4  << false;
    QTest::newRow("v4-v4mapped-mapped") << QHostAddress("192.168.1.1") << QHostAddress("::FFFF:192.168.1.1") << (int)QHostAddress::ConvertV4MappedToIPv4 << true;
    QTest::newRow("undef-any-local") << QHostAddress() << QHostAddress(QHostAddress::Any) << (int)QHostAddress::ConvertLocalHost << false;
    QTest::newRow("undef-any-unspec") << QHostAddress() << QHostAddress(QHostAddress::Any) << (int)QHostAddress::ConvertUnspecifiedAddress << false;
    QTest::newRow("anyv6-anyv4-compat") << QHostAddress(QHostAddress::AnyIPv6) << QHostAddress(QHostAddress::AnyIPv4) << (int)QHostAddress::ConvertV4CompatToIPv4 << true;
    QTest::newRow("anyv6-anyv4-mapped") << QHostAddress(QHostAddress::AnyIPv6) << QHostAddress(QHostAddress::AnyIPv4) << (int)QHostAddress::ConvertV4MappedToIPv4 << false;
    QTest::newRow("anyv6-anyv4-unspec") << QHostAddress(QHostAddress::AnyIPv6) << QHostAddress(QHostAddress::AnyIPv4) << (int)QHostAddress::ConvertUnspecifiedAddress << true;
    QTest::newRow("any-anyv4-unspec") << QHostAddress(QHostAddress::Any) << QHostAddress(QHostAddress::AnyIPv4) << (int)QHostAddress::ConvertUnspecifiedAddress << true;
    QTest::newRow("any-anyv6-unspec") << QHostAddress(QHostAddress::Any) << QHostAddress(QHostAddress::AnyIPv6) << (int)QHostAddress::ConvertUnspecifiedAddress << true;
    QTest::newRow("anyv6-anyv4-local") << QHostAddress(QHostAddress::AnyIPv6) << QHostAddress(QHostAddress::AnyIPv4) << (int)QHostAddress::ConvertLocalHost << false;
    QTest::newRow("any-anyv4-local") << QHostAddress(QHostAddress::Any) << QHostAddress(QHostAddress::AnyIPv4) << (int)QHostAddress::ConvertLocalHost << false;
    QTest::newRow("any-anyv6-local") << QHostAddress(QHostAddress::Any) << QHostAddress(QHostAddress::AnyIPv6) << (int)QHostAddress::ConvertLocalHost << false;
}

void tst_QHostAddress::isEqual()
{
    QFETCH(QHostAddress, first);
    QFETCH(QHostAddress, second);
    QFETCH(int, flags);
    QFETCH(bool, result);

    QCOMPARE(first.isEqual(second, QHostAddress::ConversionModeFlag(flags)), result);
    QCOMPARE(second.isEqual(first, QHostAddress::ConversionModeFlag(flags)), result);
}

QT_WARNING_PUSH
#ifdef QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_DEPRECATED
#endif

void tst_QHostAddress::assignment()
{
    QHostAddress address;
    address = "127.0.0.1";
    QCOMPARE(address, QHostAddress("127.0.0.1"));

    address = "::1";
    QCOMPARE(address, QHostAddress("::1"));

    // WinRT does not support sockaddr_in
#ifndef Q_OS_WINRT
    QHostAddress addr("4.2.2.1");
    sockaddr_in sockAddr;
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = htonl(addr.toIPv4Address());
    address.setAddress((sockaddr *)&sockAddr);
    QCOMPARE(address, addr);
#endif // !Q_OS_WINRT
}

QT_WARNING_POP

void tst_QHostAddress::scopeId()
{
    QHostAddress address("fe80::2e0:4cff:fefb:662a%eth0");
    QCOMPARE(address.scopeId(), QString("eth0"));
    QCOMPARE(address.toString().toLower(), QString("fe80::2e0:4cff:fefb:662a%eth0"));

    QHostAddress address2("fe80::2e0:4cff:fefb:662a");
    QCOMPARE(address2.scopeId(), QString());
    address2.setScopeId(QString("en0"));
    QCOMPARE(address2.toString().toLower(), QString("fe80::2e0:4cff:fefb:662a%en0"));

    address2 = address;
    QCOMPARE(address2.scopeId(), QString("eth0"));
    QCOMPARE(address2.toString().toLower(), QString("fe80::2e0:4cff:fefb:662a%eth0"));
}

void tst_QHostAddress::hashKey()
{
    QHash<QHostAddress, QString> hostHash;
    hostHash.insert(QHostAddress(), "ole");
}

void tst_QHostAddress::streaming_data()
{
    QTest::addColumn<QHostAddress>("address");
    QTest::newRow("1") << QHostAddress();
    QTest::newRow("2") << QHostAddress(0xDEADBEEF);
    QTest::newRow("3") << QHostAddress("127.128.129.130");
    QTest::newRow("4") << QHostAddress("1080:0000:0000:0000:0008:0800:200C:417A");
    QTest::newRow("5") << QHostAddress("fe80::2e0:4cff:fefb:662a%eth0");
    QTest::newRow("6") << QHostAddress(QHostAddress::Null);
    QTest::newRow("7") << QHostAddress(QHostAddress::LocalHost);
    QTest::newRow("8") << QHostAddress(QHostAddress::LocalHostIPv6);
    QTest::newRow("9") << QHostAddress(QHostAddress::Broadcast);
    QTest::newRow("10") << QHostAddress(QHostAddress::Any);
    QTest::newRow("11") << QHostAddress(QHostAddress::AnyIPv4);
    QTest::newRow("12") << QHostAddress(QHostAddress::AnyIPv6);
    QTest::newRow("13") << QHostAddress("foo.bar.com");
}

void tst_QHostAddress::streaming()
{
    QFETCH(QHostAddress, address);
    QByteArray ba;
    QDataStream ds1(&ba, QIODevice::WriteOnly);
    ds1 << address;
    QCOMPARE(ds1.status(), QDataStream::Ok);
    QDataStream ds2(&ba, QIODevice::ReadOnly);
    QHostAddress address2;
    ds2 >> address2;
    QCOMPARE(ds2.status(), QDataStream::Ok);
    QCOMPARE(address, address2);
}

void tst_QHostAddress::parseSubnet_data()
{
    QTest::addColumn<QString>("subnet");
    QTest::addColumn<QHostAddress>("prefix");
    QTest::addColumn<int>("prefixLength");

    // invalid/error values
    QTest::newRow("empty") << QString() << QHostAddress() << -1;
    QTest::newRow("invalid_01") << "foobar" << QHostAddress() << -1;
    QTest::newRow("invalid_02") << "   " << QHostAddress() << -1;
    QTest::newRow("invalid_03") << "1.2.3.a" << QHostAddress() << -1;
    QTest::newRow("invalid_04") << "1.2.3.4.5" << QHostAddress() << -1;
    QTest::newRow("invalid_05") << "1.2.3.4:80" << QHostAddress() << -1;
    QTest::newRow("invalid_06") << "1.2.3.4/33" << QHostAddress() << -1;
    QTest::newRow("invalid_07") << "1.2.3.4/-1" << QHostAddress() << -1;
    QTest::newRow("invalid_08") << "1.2.3.4/256.0.0.0" << QHostAddress() << -1;
    QTest::newRow("invalid_09") << "1.2.3.4/255.253.0.0" << QHostAddress() << -1;
    QTest::newRow("invalid_10") << "1.2.3.4/255.0.0.255" << QHostAddress() << -1;
    QTest::newRow("invalid_11") << "1.2.3.4." << QHostAddress() << -1;
    QTest::newRow("invalid_20") << "ffff::/-1" << QHostAddress() << -1;
    QTest::newRow("invalid_21") << "ffff::/129" << QHostAddress() << -1;
    QTest::newRow("invalid_22") << "ffff::/255.255.0.0" << QHostAddress() << -1;
    QTest::newRow("invalid_23") << "ffff::/ff00::" << QHostAddress() << -1;

    // correct IPv4 with netmask
    QTest::newRow("netmask_0") << "0.0.0.0/0.0.0.0" << QHostAddress(QHostAddress::AnyIPv4) << 0;
    QTest::newRow("netmask_1") << "0.0.0.0/255.128.0.0" << QHostAddress(QHostAddress::AnyIPv4) << 9;
    QTest::newRow("netmask_2") << "0.0.0.0/255.192.0.0" << QHostAddress(QHostAddress::AnyIPv4) << 10;
    QTest::newRow("netmask_3") << "0.0.0.0/255.224.0.0" << QHostAddress(QHostAddress::AnyIPv4) << 11;
    QTest::newRow("netmask_4") << "0.0.0.0/255.240.0.0" << QHostAddress(QHostAddress::AnyIPv4) << 12;
    QTest::newRow("netmask_5") << "0.0.0.0/255.248.0.0" << QHostAddress(QHostAddress::AnyIPv4) << 13;
    QTest::newRow("netmask_6") << "0.0.0.0/255.252.0.0" << QHostAddress(QHostAddress::AnyIPv4) << 14;
    QTest::newRow("netmask_7") << "0.0.0.0/255.254.0.0" << QHostAddress(QHostAddress::AnyIPv4) << 15;
    QTest::newRow("netmask_8") << "0.0.0.0/255.255.0.0" << QHostAddress(QHostAddress::AnyIPv4) << 16;
    QTest::newRow("netmask_16") << "0.0.0.0/255.255.0.0" << QHostAddress(QHostAddress::AnyIPv4) << 16;
    QTest::newRow("netmask_24") << "0.0.0.0/255.255.255.0" << QHostAddress(QHostAddress::AnyIPv4) << 24;
    QTest::newRow("netmask_31") << "0.0.0.0/255.255.255.254" << QHostAddress(QHostAddress::AnyIPv4) << 31;
    QTest::newRow("netmask_32") << "0.0.0.0/255.255.255.255" << QHostAddress(QHostAddress::AnyIPv4) << 32;

    // correct IPv4 with prefix
    QTest::newRow("prefix_0") << "0.0.0.0/0" << QHostAddress(QHostAddress::AnyIPv4) << 0;
    QTest::newRow("prefix_1") << "0.0.0.0/1" << QHostAddress(QHostAddress::AnyIPv4) << 1;
    QTest::newRow("prefix_9") << "0.0.0.0/9" << QHostAddress(QHostAddress::AnyIPv4) << 9;
    QTest::newRow("prefix_31") << "0.0.0.0/31" << QHostAddress(QHostAddress::AnyIPv4) << 31;
    QTest::newRow("prefix_32") << "0.0.0.0/32" << QHostAddress(QHostAddress::AnyIPv4) << 32;

    // correct IPv4 without prefix or netmask
    QTest::newRow("classA") << "10" << QHostAddress("10.0.0.0") << 8;
    QTest::newRow("classA+dot") << "10." << QHostAddress("10.0.0.0") << 8;
    QTest::newRow("classB") << "172.16" << QHostAddress("172.16.0.0") << 16;
    QTest::newRow("classB+dot") << "172.16." << QHostAddress("172.16.0.0") << 16;
    QTest::newRow("classC") << "192.168.0" << QHostAddress("192.168.0.0") << 24;
    QTest::newRow("classC+dot") << "192.168.0" << QHostAddress("192.168.0.0") << 24;
    QTest::newRow("full-ipv4") << "192.168.0.1" << QHostAddress("192.168.0.1") << 32;

    // correct IPv6 with prefix
    QTest::newRow("ipv6_01") << "::/0" << QHostAddress(QHostAddress::AnyIPv6) << 0;
    QTest::newRow("ipv6_03") << "::/3" << QHostAddress(QHostAddress::AnyIPv6) << 3;
    QTest::newRow("ipv6_16") << "::/16" << QHostAddress(QHostAddress::AnyIPv6) << 16;
    QTest::newRow("ipv6_48") << "::/48" << QHostAddress(QHostAddress::AnyIPv6) << 48;
    QTest::newRow("ipv6_127") << "::/127" << QHostAddress(QHostAddress::AnyIPv6) << 127;
    QTest::newRow("ipv6_128") << "::/128" << QHostAddress(QHostAddress::AnyIPv6) << 128;

    // tail bit clearing:
    QTest::newRow("clear_01") << "255.255.255.255/31" << QHostAddress("255.255.255.254") << 31;
    QTest::newRow("clear_08") << "255.255.255.255/24" << QHostAddress("255.255.255.0") << 24;
    QTest::newRow("clear_09") << "255.255.255.255/23" << QHostAddress("255.255.254.0") << 23;
    QTest::newRow("clear_10") << "255.255.255.255/22" << QHostAddress("255.255.252.0") << 22;
    QTest::newRow("clear_11") << "255.255.255.255/21" << QHostAddress("255.255.248.0") << 21;
    QTest::newRow("clear_12") << "255.255.255.255/20" << QHostAddress("255.255.240.0") << 20;
    QTest::newRow("clear_13") << "255.255.255.255/19" << QHostAddress("255.255.224.0") << 19;
    QTest::newRow("clear_14") << "255.255.255.255/18" << QHostAddress("255.255.192.0") << 18;
    QTest::newRow("clear_15") << "255.255.255.255/17" << QHostAddress("255.255.128.0") << 17;
    QTest::newRow("clear_16") << "255.255.255.255/16" << QHostAddress("255.255.0.0") << 16;
    QTest::newRow("clear_24") << "255.255.255.255/8" << QHostAddress("255.0.0.0") << 8;
    QTest::newRow("clear_31") << "255.255.255.255/1" << QHostAddress("128.0.0.0") << 1;
    QTest::newRow("clear_32") << "255.255.255.255/0" << QHostAddress("0.0.0.0") << 0;

    // same for IPv6:
    QTest::newRow("ipv6_clear_01") << "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/127"
                                   << QHostAddress("ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffe")
                                   << 127;
    QTest::newRow("ipv6_clear_07") << "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/121"
                                   << QHostAddress("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ff80")
                                   << 121;
    QTest::newRow("ipv6_clear_08") << "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/120"
                                   << QHostAddress("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ff00")
                                   << 120;
    QTest::newRow("ipv6_clear_16") << "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/112"
                                   << QHostAddress("ffff:ffff:ffff:ffff:ffff:ffff:ffff:0")
                                   << 112;
    QTest::newRow("ipv6_clear_80") << "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/48"
                                   << QHostAddress("ffff:ffff:ffff::")
                                   << 48;
    QTest::newRow("ipv6_clear_81") << "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/47"
                                   << QHostAddress("ffff:ffff:fffe::")
                                   << 47;
    QTest::newRow("ipv6_clear_82") << "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/46"
                                   << QHostAddress("ffff:ffff:fffc::")
                                   << 46;
    QTest::newRow("ipv6_clear_83") << "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/45"
                                   << QHostAddress("ffff:ffff:fff8::")
                                   << 45;
    QTest::newRow("ipv6_clear_84") << "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/44"
                                   << QHostAddress("ffff:ffff:fff0::")
                                   << 44;
    QTest::newRow("ipv6_clear_85") << "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/43"
                                   << QHostAddress("ffff:ffff:ffe0::")
                                   << 43;
    QTest::newRow("ipv6_clear_86") << "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/42"
                                   << QHostAddress("ffff:ffff:ffc0::")
                                   << 42;
    QTest::newRow("ipv6_clear_87") << "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/41"
                                   << QHostAddress("ffff:ffff:ff80::")
                                   << 41;
    QTest::newRow("ipv6_clear_88") << "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/40"
                                   << QHostAddress("ffff:ffff:ff00::")
                                   << 40;
    QTest::newRow("ipv6_clear_125") << "3fff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/3"
                                    << QHostAddress("2000::")
                                    << 3;
    QTest::newRow("ipv6_clear_127") << "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/1"
                                    << QHostAddress("8000::")
                                    << 1;
    QTest::newRow("ipv6_clear_128") << "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/0"
                                    << QHostAddress(QHostAddress::AnyIPv6)
                                    << 0;
}

void tst_QHostAddress::parseSubnet()
{
    QFETCH(QString, subnet);
    QFETCH(QHostAddress, prefix);
    QFETCH(int, prefixLength);

    QPair<QHostAddress, int> result = QHostAddress::parseSubnet(subnet);
    QCOMPARE(result.first, prefix);
    QCOMPARE(result.second, prefixLength);
}

void tst_QHostAddress::isInSubnet_data()
{
    QTest::addColumn<QHostAddress>("address");
    QTest::addColumn<QHostAddress>("prefix");
    QTest::addColumn<int>("prefixLength");
    QTest::addColumn<bool>("result");

    // invalid QHostAddresses are never in any subnets
    QTest::newRow("invalid_01") << QHostAddress() << QHostAddress() << 32 << false;
    QTest::newRow("invalid_02") << QHostAddress() << QHostAddress(QHostAddress::AnyIPv4) << 32 << false;
    QTest::newRow("invalid_03") << QHostAddress() << QHostAddress(QHostAddress::AnyIPv4) << 8 << false;
    QTest::newRow("invalid_04") << QHostAddress() << QHostAddress(QHostAddress::AnyIPv4) << 0 << false;
    QTest::newRow("invalid_05") << QHostAddress() << QHostAddress("255.255.255.0") << 24 << false;
    QTest::newRow("invalid_06") << QHostAddress() << QHostAddress(QHostAddress::AnyIPv6) << 0 << false;
    QTest::newRow("invalid_07") << QHostAddress() << QHostAddress(QHostAddress::AnyIPv6) << 32 << false;
    QTest::newRow("invalid_08") << QHostAddress() << QHostAddress(QHostAddress::AnyIPv6) << 128<< false;

    // and no host address can be in a subnet whose prefix is invalid
    QTest::newRow("invalid_20") << QHostAddress(QHostAddress::AnyIPv4) << QHostAddress() << 16 << false;
    QTest::newRow("invalid_21") << QHostAddress(QHostAddress::AnyIPv6) << QHostAddress() << 16 << false;
    QTest::newRow("invalid_22") << QHostAddress(QHostAddress::LocalHost) << QHostAddress() << 16 << false;
    QTest::newRow("invalid_23") << QHostAddress(QHostAddress::LocalHostIPv6) << QHostAddress() << 16 << false;

    // negative netmasks don't make sense:
    QTest::newRow("invalid_30") << QHostAddress(QHostAddress::AnyIPv4) << QHostAddress(QHostAddress::Any) << -1 << false;
    QTest::newRow("invalid_31") << QHostAddress(QHostAddress::AnyIPv6) << QHostAddress(QHostAddress::AnyIPv6) << -1 << false;

    // we don't support IPv4 belonging in an IPv6 netmask and vice-versa
    QTest::newRow("v4-in-v6") << QHostAddress(QHostAddress::LocalHost) << QHostAddress(QHostAddress::AnyIPv6) << 0 << false;
    QTest::newRow("v6-in-v4") << QHostAddress(QHostAddress::LocalHostIPv6) << QHostAddress(QHostAddress::Any) << 0 << false;
    QTest::newRow("v4-in-v6mapped") << QHostAddress(QHostAddress::LocalHost) << QHostAddress("ffff:ffff:ffff:ffff:ffff:ffff:255.0.0.0") << 113 << false;
    QTest::newRow("v4-in-v6mapped2") << QHostAddress(QHostAddress::LocalHost) << QHostAddress("::ffff:255.0.0.0") << 113 << false;

    // IPv4 correct ones
    QTest::newRow("netmask_0") << QHostAddress(QHostAddress::LocalHost) << QHostAddress(QHostAddress::AnyIPv4) << 0 << true;
    QTest::newRow("netmask_0bis") << QHostAddress(QHostAddress::LocalHost) << QHostAddress("255.255.0.0") << 0 << true;
    QTest::newRow("netmask_0ter") << QHostAddress(QHostAddress::LocalHost) << QHostAddress("1.2.3.4") << 0 << true;
    QTest::newRow("netmask_1") << QHostAddress(QHostAddress::LocalHost) << QHostAddress(QHostAddress::AnyIPv4) << 1 << true;
    QTest::newRow("~netmask_1") << QHostAddress(QHostAddress::LocalHost) << QHostAddress("128.0.0.0") << 1 << false;
    QTest::newRow("netmask_1bis") << QHostAddress("224.0.0.1") << QHostAddress("128.0.0.0") << 1 << true;
    QTest::newRow("~netmask_1bis") << QHostAddress("224.0.0.1") << QHostAddress("0.0.0.0") << 1 << false;
    QTest::newRow("netmask_8") << QHostAddress(QHostAddress::LocalHost) << QHostAddress("127.0.0.0") << 8 << true;
    QTest::newRow("~netmask_8") << QHostAddress(QHostAddress::LocalHost) << QHostAddress("126.0.0.0") << 8 << false;
    QTest::newRow("netmask_15") << QHostAddress("10.0.1.255") << QHostAddress("10.0.0.0") << 15 << true;
    QTest::newRow("netmask_16") << QHostAddress("172.16.0.1") << QHostAddress("172.16.0.0") << 16 << true;

    // the address is always in the subnet containing its address, regardless of length:
    QTest::newRow("same_01") << QHostAddress(QHostAddress::LocalHost) << QHostAddress(QHostAddress::LocalHost) << 1 << true;
    QTest::newRow("same_07") << QHostAddress(QHostAddress::LocalHost) << QHostAddress(QHostAddress::LocalHost) << 7 << true;
    QTest::newRow("same_8") << QHostAddress(QHostAddress::LocalHost) << QHostAddress(QHostAddress::LocalHost) << 8 << true;
    QTest::newRow("same_24") << QHostAddress(QHostAddress::LocalHost) << QHostAddress(QHostAddress::LocalHost) << 23 << true;
    QTest::newRow("same_31") << QHostAddress(QHostAddress::LocalHost) << QHostAddress(QHostAddress::LocalHost) << 31 << true;
    QTest::newRow("same_32") << QHostAddress(QHostAddress::LocalHost) << QHostAddress(QHostAddress::LocalHost) << 32 << true;

    // IPv6 correct ones:
    QTest::newRow("ipv6_netmask_0") << QHostAddress(QHostAddress::LocalHostIPv6) << QHostAddress(QHostAddress::AnyIPv6) << 0 << true;
    QTest::newRow("ipv6_netmask_0bis") << QHostAddress(QHostAddress::LocalHostIPv6) << QHostAddress(QHostAddress::LocalHostIPv6) << 0 << true;
    QTest::newRow("ipv6_netmask_0ter") << QHostAddress(QHostAddress::LocalHostIPv6) << QHostAddress("ffff::") << 0 << true;
    QTest::newRow("ipv6_netmask_1") << QHostAddress(QHostAddress::LocalHostIPv6) << QHostAddress(QHostAddress::AnyIPv6) << 1 << true;
    QTest::newRow("ipv6_netmask_1bis") << QHostAddress("fec0::1") << QHostAddress("8000::") << 1 << true;
    QTest::newRow("~ipv6_netmask_1") << QHostAddress(QHostAddress::LocalHostIPv6) << QHostAddress("8000::") << 1 << false;
    QTest::newRow("~ipv6_netmask_1bis") << QHostAddress("fec0::1") << QHostAddress("::") << 1 << false;
    QTest::newRow("ipv6_netmask_47") << QHostAddress("2:3:5::1") << QHostAddress("2:3:4::") << 47 << true;
    QTest::newRow("ipv6_netmask_48") << QHostAddress("2:3:4::1") << QHostAddress("2:3:4::") << 48 << true;
    QTest::newRow("~ipv6_netmask_48") << QHostAddress("2:3:5::1") << QHostAddress("2:3:4::") << 48 << false;
    QTest::newRow("ipv6_netmask_127") << QHostAddress("2:3:4:5::1") << QHostAddress("2:3:4:5::") << 127 << true;
    QTest::newRow("ipv6_netmask_128") << QHostAddress("2:3:4:5::1") << QHostAddress("2:3:4:5::1") << 128 << true;
    QTest::newRow("~ipv6_netmask_128") << QHostAddress("2:3:4:5::1") << QHostAddress("2:3:4:5::0") << 128 << false;
}

void tst_QHostAddress::isInSubnet()
{
    QFETCH(QHostAddress, address);
    QFETCH(QHostAddress, prefix);
    QFETCH(int, prefixLength);

    QTEST(address.isInSubnet(prefix, prefixLength), "result");
}

void tst_QHostAddress::isLoopback_data()
{
    QTest::addColumn<QHostAddress>("address");
    QTest::addColumn<bool>("result");

    QTest::newRow("default") << QHostAddress() << false;
    QTest::newRow("invalid") << QHostAddress("&&&") << false;

    QTest::newRow("ipv6_loop") << QHostAddress(QHostAddress::LocalHostIPv6) << true;
    QTest::newRow("::1") << QHostAddress("::1") << true;

    QTest::newRow("ipv4_loop") << QHostAddress(QHostAddress::LocalHost) << true;
    QTest::newRow("127.0.0.1") << QHostAddress("127.0.0.1") << true;
    QTest::newRow("127.0.0.2") << QHostAddress("127.0.0.2") << true;
    QTest::newRow("127.3.2.1") << QHostAddress("127.3.2.1") << true;

    QTest::newRow("1.2.3.4") << QHostAddress("1.2.3.4") << false;
    QTest::newRow("10.0.0.4") << QHostAddress("10.0.0.4") << false;
    QTest::newRow("192.168.3.4") << QHostAddress("192.168.3.4") << false;

    QTest::newRow("::") << QHostAddress("::") << false;
    QTest::newRow("Any") << QHostAddress(QHostAddress::Any) << false;
    QTest::newRow("AnyIPv4") << QHostAddress(QHostAddress::AnyIPv4) << false;
    QTest::newRow("AnyIPv6") << QHostAddress(QHostAddress::AnyIPv6) << false;
    QTest::newRow("Broadcast") << QHostAddress(QHostAddress::Broadcast) << false;
    QTest::newRow("Null") << QHostAddress(QHostAddress::Null) << false;
    QTest::newRow("ipv6-all-ffff") << QHostAddress("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff") << false;

    QTest::newRow("::ffff:127.0.0.1") << QHostAddress("::ffff:127.0.0.1") << true;
    QTest::newRow("::ffff:127.0.0.2") << QHostAddress("::ffff:127.0.0.2") << true;
    QTest::newRow("::ffff:127.3.2.1") << QHostAddress("::ffff:127.3.2.1") << true;

}

void tst_QHostAddress::isLoopback()
{
    QFETCH(QHostAddress, address);
    QFETCH(bool, result);

    QCOMPARE(address.isLoopback(), result);
}

void tst_QHostAddress::isMulticast_data()
{
    QTest::addColumn<QHostAddress>("address");
    QTest::addColumn<bool>("result");

    QTest::newRow("default") << QHostAddress() << false;
    QTest::newRow("invalid") << QHostAddress("&&&") << false;

    QTest::newRow("ipv6_loop") << QHostAddress(QHostAddress::LocalHostIPv6) << false;
    QTest::newRow("::1") << QHostAddress("::1") << false;
    QTest::newRow("ipv4_loop") << QHostAddress(QHostAddress::LocalHost) << false;
    QTest::newRow("127.0.0.1") << QHostAddress("127.0.0.1") << false;
    QTest::newRow("::") << QHostAddress("::") << false;
    QTest::newRow("Any") << QHostAddress(QHostAddress::Any) << false;
    QTest::newRow("AnyIPv4") << QHostAddress(QHostAddress::AnyIPv4) << false;
    QTest::newRow("AnyIPv6") << QHostAddress(QHostAddress::AnyIPv6) << false;
    QTest::newRow("Broadcast") << QHostAddress(QHostAddress::Broadcast) << false;
    QTest::newRow("Null") << QHostAddress(QHostAddress::Null) << false;

    QTest::newRow("223.255.255.255") << QHostAddress("223.255.255.255") << false;
    QTest::newRow("224.0.0.0") << QHostAddress("224.0.0.0") << true;
    QTest::newRow("239.255.255.255") << QHostAddress("239.255.255.255") << true;
    QTest::newRow("240.0.0.0") << QHostAddress("240.0.0.0") << false;

    QTest::newRow("::ffff:223.255.255.255") << QHostAddress("::ffff:223.255.255.255") << false;
    QTest::newRow("::ffff:224.0.0.0") << QHostAddress("::ffff:224.0.0.0") << true;
    QTest::newRow("::ffff:239.255.255.255") << QHostAddress("::ffff:239.255.255.255") << true;
    QTest::newRow("::ffff:240.0.0.0") << QHostAddress("::ffff:240.0.0.0") << false;

    QTest::newRow("fc00::") << QHostAddress("fc00::") << false;
    QTest::newRow("fe80::") << QHostAddress("fe80::") << false;
    QTest::newRow("fec0::") << QHostAddress("fec0::") << false;
    QTest::newRow("ff00::") << QHostAddress("ff00::") << true;
    QTest::newRow("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff") << QHostAddress("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff") << true;
}

void tst_QHostAddress::isMulticast()
{
    QFETCH(QHostAddress, address);
    QFETCH(bool, result);

    QCOMPARE(address.isMulticast(), result);
}

void tst_QHostAddress::convertv4v6_data()
{
    QTest::addColumn<QHostAddress>("source");
    QTest::addColumn<int>("protocol");
    QTest::addColumn<QHostAddress>("result");

    QTest::newRow("any-to-v4") << QHostAddress(QHostAddress::Any) << 4 << QHostAddress(QHostAddress::AnyIPv4);
    QTest::newRow("any-to-v6") << QHostAddress(QHostAddress::Any) << 6 << QHostAddress(QHostAddress::AnyIPv6);
    QTest::newRow("anyv4-to-v6") << QHostAddress(QHostAddress::AnyIPv4) << 6 << QHostAddress(QHostAddress::AnyIPv6);
    QTest::newRow("anyv6-to-v4") << QHostAddress(QHostAddress::AnyIPv6) << 4 << QHostAddress(QHostAddress::AnyIPv4);

    QTest::newRow("v4mapped-to-v4") << QHostAddress("::ffff:192.0.2.1") << 4 << QHostAddress("192.0.2.1");
    QTest::newRow("v4-to-v4mapped") << QHostAddress("192.0.2.1") << 6 << QHostAddress("::ffff:192.0.2.1");

    // we won't convert 127.0.0.1 to ::1 or vice-versa:
    // you can connect to a v4 server socket with ::ffff:127.0.0.1, but not with ::1
    QTest::newRow("localhost-to-v4mapped") << QHostAddress(QHostAddress::LocalHost) << 6 << QHostAddress("::ffff:127.0.0.1");
    QTest::newRow("v4mapped-to-localhost") << QHostAddress("::ffff:127.0.0.1") << 4 << QHostAddress(QHostAddress::LocalHost);

    // in turn, that means localhost6 doesn't convert to v4
    QTest::newRow("localhost6-to-v4") << QHostAddress(QHostAddress::LocalHostIPv6) << 4 << QHostAddress();

    // some other v6 addresses that won't convert to v4
    QTest::newRow("v4compat-to-v4") << QHostAddress("::192.0.2.1") << 4 << QHostAddress();
    QTest::newRow("localhostv4compat-to-v4") << QHostAddress("::127.0.0.1") << 4 << QHostAddress();
    QTest::newRow("v6global-to-v4") << QHostAddress("2001:db8::1") << 4 << QHostAddress();
    QTest::newRow("v6multicast-to-v4") << QHostAddress("ff02::1") << 4 << QHostAddress();
}

void tst_QHostAddress::convertv4v6()
{
    QFETCH(QHostAddress, source);
    QFETCH(int, protocol);
    QFETCH(QHostAddress, result);

    if (protocol == 4) {
        bool ok;
        quint32 v4 = source.toIPv4Address(&ok);
        QCOMPARE(ok, result.protocol() == QAbstractSocket::IPv4Protocol);
        if (ok)
            QCOMPARE(QHostAddress(v4), result);
    } else if (protocol == 6) {
        QCOMPARE(QHostAddress(source.toIPv6Address()), result);
    }
}

QTEST_MAIN(tst_QHostAddress)
#include "tst_qhostaddress.moc"
