/****************************************************************************
**
** Copyright (C) 2012 Intel Corporation.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QString>
#include <QtTest/QtTest>
#include <QtCore/private/qipaddress_p.h>

#ifdef __GLIBC__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

class tst_QIpAddress : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parseIp4_data();
    void parseIp4();
    void invalidParseIp4_data();
    void invalidParseIp4();
    void ip4ToString_data();
    void ip4ToString();

    void parseIp6_data();
    void parseIp6();
    void invalidParseIp6_data();
    void invalidParseIp6();
    void ip6ToString_data();
    void ip6ToString();
};

struct Ip6
{
    QIPAddressUtils::IPv6Address u8;
    Ip6() { *this = Ip6(0,0,0,0, 0,0,0,0); }
    Ip6(quint16 p1, quint16 p2, quint16 p3, quint16 p4,
         quint16 p5, quint16 p6, quint16 p7, quint16 p8)
    {
        u8[0] = p1 >> 8;
        u8[2] = p2 >> 8;
        u8[4] = p3 >> 8;
        u8[6] = p4 >> 8;
        u8[8] = p5 >> 8;
        u8[10] = p6 >> 8;
        u8[12] = p7 >> 8;
        u8[14] = p8 >> 8;

        u8[1] = p1 & 0xff;
        u8[3] = p2 & 0xff;
        u8[5] = p3 & 0xff;
        u8[7] = p4 & 0xff;
        u8[9] = p5 & 0xff;
        u8[11] = p6 & 0xff;
        u8[13] = p7 & 0xff;
        u8[15] = p8 & 0xff;
    }

    bool operator==(const Ip6 &other) const
    { return memcmp(u8, other.u8, sizeof u8) == 0; }
};
Q_DECLARE_METATYPE(Ip6)

QT_BEGIN_NAMESPACE
namespace QTest {
    template<>
    char *toString(const Ip6 &ip6)
    {
        char buf[sizeof "1111:2222:3333:4444:5555:6666:7777:8888" + 2];
        sprintf(buf, "%x:%x:%x:%x:%x:%x:%x:%x",
                ip6.u8[0] << 8 | ip6.u8[1],
                ip6.u8[2] << 8 | ip6.u8[3],
                ip6.u8[4] << 8 | ip6.u8[5],
                ip6.u8[6] << 8 | ip6.u8[7],
                ip6.u8[8] << 8 | ip6.u8[9],
                ip6.u8[10] << 8 | ip6.u8[11],
                ip6.u8[12] << 8 | ip6.u8[13],
                ip6.u8[14] << 8 | ip6.u8[15]);
        return qstrdup(buf);
    }
}
QT_END_NAMESPACE

void tst_QIpAddress::parseIp4_data()
{
    QTest::addColumn<QString>("data");
    QTest::addColumn<QIPAddressUtils::IPv4Address>("ip");

    // valid strings
    QTest::newRow("0.0.0.0") << "0.0.0.0" << 0u;
    QTest::newRow("10.0.0.1") << "10.0.0.1" << 0x0a000001u;
    QTest::newRow("127.0.0.1") << "127.0.0.1" << 0x7f000001u;
    QTest::newRow("172.16.0.1") << "172.16.0.1" << 0xac100001u;
    QTest::newRow("172.16.16.1") << "172.16.16.1" << 0xac101001u;
    QTest::newRow("172.16.16.16") << "172.16.16.16" << 0xac101010u;
    QTest::newRow("192.168.0.1") << "192.168.0.1" << 0xc0a80001u;
    QTest::newRow("192.168.16.1") << "192.168.16.1" << 0xc0a81001u;
    QTest::newRow("192.168.16.16") << "192.168.16.16" << 0xc0a81010u;
    QTest::newRow("192.168.192.1") << "192.168.192.1" << 0xc0a8c001u;
    QTest::newRow("192.168.192.16") << "192.168.192.16" << 0xc0a8c010u;
    QTest::newRow("192.168.192.255") << "192.168.192.255" << 0xc0a8c0ffu;
    QTest::newRow("224.0.0.1") << "224.0.0.1" << 0xe0000001u;
    QTest::newRow("239.255.255.255") << "239.255.255.255" << 0xefffffffu;
    QTest::newRow("255.255.255.255") << "255.255.255.255" << uint(-1);

    // still valid but unusual
    QTest::newRow("000.000.000.000") << "000.000.000.000" << 0u;
    QTest::newRow("000001.000002.000000003.000000000004") << "000001.000002.000000003.000000000004" << 0x01020304u;

    // octals:
    QTest::newRow("012.0250.0377.0377") << "012.0250.0377.0377" << 0x0aa8ffffu;
    QTest::newRow("0000000000012.00000000000250.000000000000377.0000000000000000000000000000000000000377")
            << "0000000000012.00000000000250.000000000000377.0000000000000000000000000000000000000377" << 0x0aa8ffffu;

    // hex:
    QTest::newRow("0xa.0xa.0x7f.0xff") << "0xa.0xa.0x7f.0xff" << 0x0a0a7fffu;

    // dots missing, less than 255:
    QTest::newRow("1.2.3") << "1.2.3" << 0x01020003u;
    QTest::newRow("1.2") << "1.2" << 0x01000002u;
    QTest::newRow("1") << "1" << 1u;

    // dots missing, more than 255, no overwrite
    QTest::newRow("1.2.257") << "1.2.257" << 0x01020101u;
    QTest::newRow("1.0x010101") << "1.0x010101" << 0x01010101u;
    QTest::newRow("2130706433") << "2130706433" << 0x7f000001u;
}

void tst_QIpAddress::parseIp4()
{
    QFETCH(QString, data);
    QFETCH(QIPAddressUtils::IPv4Address, ip);

#ifdef __GLIBC__
    {
        in_addr inet_result;
        int inet_ok = inet_aton(data.toLatin1(), &inet_result);
        QVERIFY(inet_ok);
        QCOMPARE(ntohl(inet_result.s_addr), ip);
    }
#endif

    QIPAddressUtils::IPv4Address result;
    bool ok = QIPAddressUtils::parseIp4(result, data.constBegin(), data.constEnd());
    QVERIFY(ok);
    QCOMPARE(result, ip);
}

void tst_QIpAddress::invalidParseIp4_data()
{
    QTest::addColumn<QString>("data");

    // too many dots
    QTest::newRow(".") << ".";
    QTest::newRow("..") << "..";
    QTest::newRow("...") << "...";
    QTest::newRow("....") << "....";
    QTest::newRow("1.") << "1.";
    QTest::newRow("1.2.") << "1.2.";
    QTest::newRow("1.2.3.") << "1.2.3.";
    QTest::newRow("1.2.3.4.") << "1.2.3.4.";
    QTest::newRow("1.2.3..4") << "1.2.3..4";

    // octet more than 255
    QTest::newRow("2.2.2.257") << "2.2.2.257";
    QTest::newRow("2.2.257.2") << "2.2.257.2";
    QTest::newRow("2.257.2.2") << "2.257.2.2";
    QTest::newRow("257.2.2.2") << "257.2.2.2";

    // number more than field available
    QTest::newRow("2.2.0x01010101") << "2.2.0x01010101";
    QTest::newRow("2.0x01010101") << "2.0x01010101";
    QTest::newRow("4294967296") << "4294967296";

    // bad octals
    QTest::newRow("09") << "09";

    // bad hex
    QTest::newRow("0x1g") << "0x1g";

    // letters
    QTest::newRow("abc") << "abc";
    QTest::newRow("1.2.3a.4") << "1.2.3a.4";
    QTest::newRow("a.2.3.4") << "a.2.3.4";
    QTest::newRow("1.2.3.4a") << "1.2.3.4a";
}

void tst_QIpAddress::invalidParseIp4()
{
    QFETCH(QString, data);

#ifdef __GLIBC__
    {
        in_addr inet_result;
        int inet_ok = inet_aton(data.toLatin1(), &inet_result);
# ifdef Q_OS_DARWIN
        QEXPECT_FAIL("4294967296", "Mac's library does parse this one", Continue);
# endif
        QVERIFY(!inet_ok);
    }
#endif

    QIPAddressUtils::IPv4Address result;
    bool ok = QIPAddressUtils::parseIp4(result, data.constBegin(), data.constEnd());
    QVERIFY(!ok);
}

void tst_QIpAddress::ip4ToString_data()
{
    QTest::addColumn<QIPAddressUtils::IPv4Address>("ip");
    QTest::addColumn<QString>("expected");

    QTest::newRow("0.0.0.0") << 0u << "0.0.0.0";
    QTest::newRow("1.2.3.4") << 0x01020304u << "1.2.3.4";
    QTest::newRow("111.222.33.44") << 0x6fde212cu << "111.222.33.44";
    QTest::newRow("255.255.255.255") << 0xffffffffu << "255.255.255.255";
}

void tst_QIpAddress::ip4ToString()
{
    QFETCH(QIPAddressUtils::IPv4Address, ip);
    QFETCH(QString, expected);

#ifdef __GLIBC__
    in_addr inet_ip;
    inet_ip.s_addr = htonl(ip);
    QCOMPARE(QString(inet_ntoa(inet_ip)), expected);
#endif

    QString result;
    QIPAddressUtils::toString(result, ip);
    QCOMPARE(result, expected);
}

void tst_QIpAddress::parseIp6_data()
{
    qRegisterMetaType<Ip6>();
    QTest::addColumn<QString>("address");
    QTest::addColumn<Ip6>("expected");

    // 7 colons, no ::
    QTest::newRow("0:0:0:0:0:0:0:0") << "0:0:0:0:0:0:0:0" << Ip6(0,0,0,0,0,0,0,0);
    QTest::newRow("0:0:0:0:0:0:0:1") << "0:0:0:0:0:0:0:1" << Ip6(0,0,0,0,0,0,0,1);
    QTest::newRow("0:0:0:0:0:0:1:1") << "0:0:0:0:0:0:1:1" << Ip6(0,0,0,0,0,0,1,1);
    QTest::newRow("0:0:0:0:0:0:0:103") << "0:0:0:0:0:0:0:103" << Ip6(0,0,0,0,0,0,0,0x103);
    QTest::newRow("1:2:3:4:5:6:7:8") << "1:2:3:4:5:6:7:8" << Ip6(1,2,3,4,5,6,7,8);
    QTest::newRow("ffee:ddcc:bbaa:9988:7766:5544:3322:1100")
            << "ffee:ddcc:bbaa:9988:7766:5544:3322:1100"
            << Ip6(0xffee, 0xddcc, 0xbbaa, 0x9988, 0x7766, 0x5544, 0x3322, 0x1100);

    // too many zeroes
    QTest::newRow("0:0:0:0:0:0:0:00103") << "0:0:0:0:0:0:0:00103" << Ip6(0,0,0,0,0,0,0,0x103);

    // double-colon
    QTest::newRow("::1:2:3:4:5:6:7") << "::1:2:3:4:5:6:7" << Ip6(0,1,2,3,4,5,6,7);
    QTest::newRow("1:2:3:4:5:6:7::") << "1:2:3:4:5:6:7::" << Ip6(1,2,3,4,5,6,7,0);

    QTest::newRow("1::2:3:4:5:6:7") << "1::2:3:4:5:6:7" << Ip6(1,0,2,3,4,5,6,7);
    QTest::newRow("1:2::3:4:5:6:7") << "1:2::3:4:5:6:7" << Ip6(1,2,0,3,4,5,6,7);
    QTest::newRow("1:2:3::4:5:6:7") << "1:2:3::4:5:6:7" << Ip6(1,2,3,0,4,5,6,7);
    QTest::newRow("1:2:3:4::5:6:7") << "1:2:3:4::5:6:7" << Ip6(1,2,3,4,0,5,6,7);
    QTest::newRow("1:2:3:4:5::6:7") << "1:2:3:4:5::6:7" << Ip6(1,2,3,4,5,0,6,7);
    QTest::newRow("1:2:3:4:5:6::7") << "1:2:3:4:5:6::7" << Ip6(1,2,3,4,5,6,0,7);

    QTest::newRow("::1:2:3:4:5:6") << "::1:2:3:4:5:6" << Ip6(0,0,1,2,3,4,5,6);
    QTest::newRow("1:2:3:4:5:6::") << "1:2:3:4:5:6::" << Ip6(1,2,3,4,5,6,0,0);

    QTest::newRow("1::2:3:4:5:6") << "1::2:3:4:5:6" << Ip6(1,0,0,2,3,4,5,6);
    QTest::newRow("1:2::3:4:5:6") << "1:2::3:4:5:6" << Ip6(1,2,0,0,3,4,5,6);
    QTest::newRow("1:2:3::4:5:6") << "1:2:3::4:5:6" << Ip6(1,2,3,0,0,4,5,6);
    QTest::newRow("1:2:3:4::5:6") << "1:2:3:4::5:6" << Ip6(1,2,3,4,0,0,5,6);
    QTest::newRow("1:2:3:4:5::6") << "1:2:3:4:5::6" << Ip6(1,2,3,4,5,0,0,6);

    QTest::newRow("::1:2:3:4:5") << "::1:2:3:4:5" << Ip6(0,0,0,1,2,3,4,5);
    QTest::newRow("1:2:3:4:5::") << "1:2:3:4:5::" << Ip6(1,2,3,4,5,0,0,0);

    QTest::newRow("1::2:3:4:5") << "1::2:3:4:5" << Ip6(1,0,0,0,2,3,4,5);
    QTest::newRow("1:2::3:4:5") << "1:2::3:4:5" << Ip6(1,2,0,0,0,3,4,5);
    QTest::newRow("1:2:3::4:5") << "1:2:3::4:5" << Ip6(1,2,3,0,0,0,4,5);
    QTest::newRow("1:2:3:4::5") << "1:2:3:4::5" << Ip6(1,2,3,4,0,0,0,5);

    QTest::newRow("::1:2:3:4") << "::1:2:3:4" << Ip6(0,0,0,0,1,2,3,4);
    QTest::newRow("1:2:3:4::") << "1:2:3:4::" << Ip6(1,2,3,4,0,0,0,0);

    QTest::newRow("1::2:3:4") << "1::2:3:4" << Ip6(1,0,0,0,0,2,3,4);
    QTest::newRow("1:2::3:4") << "1:2::3:4" << Ip6(1,2,0,0,0,0,3,4);
    QTest::newRow("1:2:3::4") << "1:2:3::4" << Ip6(1,2,3,0,0,0,0,4);

    QTest::newRow("::1:2:3") << "::1:2:3" << Ip6(0,0,0,0,0,1,2,3);
    QTest::newRow("1:2:3::") << "1:2:3::" << Ip6(1,2,3,0,0,0,0,0);

    QTest::newRow("1::2:3") << "1::2:3" << Ip6(1,0,0,0,0,0,2,3);
    QTest::newRow("1:2::3") << "1:2::3" << Ip6(1,2,0,0,0,0,0,3);

    QTest::newRow("::1:2") << "::1:2" << Ip6(0,0,0,0,0,0,1,2);
    QTest::newRow("1:2::") << "1:2::" << Ip6(1,2,0,0,0,0,0,0);

    QTest::newRow("1::2") << "1::2" << Ip6(1,0,0,0,0,0,0,2);

    QTest::newRow("::1") << "::1" << Ip6(0,0,0,0,0,0,0,1);
    QTest::newRow("1::") << "1::" << Ip6(1,0,0,0,0,0,0,0);

    QTest::newRow("::") << "::" << Ip6(0,0,0,0,0,0,0,0);

    // embedded IPv4
    QTest::newRow("1:2:3:4:5:6:10.0.16.1") << "1:2:3:4:5:6:10.0.16.1" << Ip6(1,2,3,4,5,6,0xa00,0x1001);
    QTest::newRow("1::10.0.16.1") << "1::10.0.16.1" << Ip6(1,0,0,0,0,0,0xa00,0x1001);
    QTest::newRow("::10.0.16.1") << "::10.0.16.1" << Ip6(0,0,0,0,0,0,0xa00,0x1001);
    QTest::newRow("::0.0.0.0") << "::0.0.0.0" << Ip6(0,0,0,0,0,0,0,0);
}

void tst_QIpAddress::parseIp6()
{
    QFETCH(QString, address);
    QFETCH(Ip6, expected);

#if defined(__GLIBC__) && defined(AF_INET6)
    Ip6 inet_result;
    bool inet_ok = inet_pton(AF_INET6, address.toLatin1(), &inet_result.u8);
    QVERIFY(inet_ok);
    QCOMPARE(inet_result, expected);
#endif

    Ip6 result;
    bool ok = QIPAddressUtils::parseIp6(result.u8, address.constBegin(), address.constEnd());
    QVERIFY(ok);
    QCOMPARE(result, expected);
}

void tst_QIpAddress::invalidParseIp6_data()
{
    QTest::addColumn<QString>("address");

    // too many colons
    QTest::newRow("0:0:0:0::0:0:0:0") << "0:0:0:0::0:0:0:0";
    QTest::newRow("0:::") << "0:::"; QTest::newRow(":::0") << ":::0";
    QTest::newRow("16:::::::::::::::::::::::") << "16:::::::::::::::::::::::";

    // non-hex
    QTest::newRow("a:b:c:d:e:f:g:h") << "a:b:c:d:e:f:g:h";

    // too big number
    QTest::newRow("0:0:0:0:0:0:0:10103") << "0:0:0:0:0:0:0:10103";

    // too short
    QTest::newRow("0:0:0:0:0:0:0:") << "0:0:0:0:0:0:0:";
    QTest::newRow("0:0:0:0:0:0:0") << "0:0:0:0:0:0:0";
    QTest::newRow("0:0:0:0:0:0:") << "0:0:0:0:0:0:";
    QTest::newRow("0:0:0:0:0:0") << "0:0:0:0:0:0";
    QTest::newRow("0:0:0:0:0:") << "0:0:0:0:0:";
    QTest::newRow("0:0:0:0:0") << "0:0:0:0:0";
    QTest::newRow("0:0:0:0:") << "0:0:0:0:";
    QTest::newRow("0:0:0:0") << "0:0:0:0";
    QTest::newRow("0:0:0:") << "0:0:0:";
    QTest::newRow("0:0:0") << "0:0:0";
    QTest::newRow("0:0:") << "0:0:";
    QTest::newRow("0:0") << "0:0";
    QTest::newRow("0:") << "0:";
    QTest::newRow("0") << "0";
    QTest::newRow(":0") << ":0";
    QTest::newRow(":0:0") << ":0:0";
    QTest::newRow(":0:0:0") << ":0:0:0";
    QTest::newRow(":0:0:0:0") << ":0:0:0:0";
    QTest::newRow(":0:0:0:0:0") << ":0:0:0:0:0";
    QTest::newRow(":0:0:0:0:0:0") << ":0:0:0:0:0:0";
    QTest::newRow(":0:0:0:0:0:0:0") << ":0:0:0:0:0:0:0";

    // IPv4
    QTest::newRow("1.2.3.4") << "1.2.3.4";

    // embedded IPv4 in the wrong position
    QTest::newRow("1.2.3.4::") << "1.2.3.4::";
    QTest::newRow("f:1.2.3.4::") << "f:1.2.3.4::";
    QTest::newRow("f:e:d:c:b:1.2.3.4:0") << "f:e:d:c:b:1.2.3.4:0";

    // bad embedded IPv4
    QTest::newRow("::1.2.3") << "::1.2.3";
    QTest::newRow("::1.2.257") << "::1.2.257";
    QTest::newRow("::1.2") << "::1.2";
    QTest::newRow("::0250.0x10101") << "::0250.0x10101";
    QTest::newRow("::1.2.3.0250") << "::1.2.3.0250";
    QTest::newRow("::1.2.3.0xff") << "::1.2.3.0xff";
    QTest::newRow("::1.2.3.07") << "::1.2.3.07";
    QTest::newRow("::1.2.3.010") << "::1.2.3.010";

    // separated by something else
    QTest::newRow("1.2.3.4.5.6.7.8") << "1.2.3.4.5.6.7.8";
    QTest::newRow("1,2,3,4,5,6,7,8") << "1,2,3,4,5,6,7,8";
    QTest::newRow("1..2") << "1..2";
    QTest::newRow("1:.2") << "1:.2";
    QTest::newRow("1.:2") << "1.:2";
}

void tst_QIpAddress::invalidParseIp6()
{
    QFETCH(QString, address);

#if defined(__GLIBC__) && defined(AF_INET6)
    Ip6 inet_result;
    bool inet_ok = inet_pton(AF_INET6, address.toLatin1(), &inet_result.u8);
    QVERIFY(!inet_ok);
#endif

    Ip6 result;
    bool ok = QIPAddressUtils::parseIp6(result.u8, address.constBegin(), address.constEnd());
    QVERIFY(!ok);
}

void tst_QIpAddress::ip6ToString_data()
{
    qRegisterMetaType<Ip6>();
    QTest::addColumn<Ip6>("ip");
    QTest::addColumn<QString>("expected");

    QTest::newRow("1:2:3:4:5:6:7:8") << Ip6(1,2,3,4,5,6,7,8) << "1:2:3:4:5:6:7:8";
    QTest::newRow("1:2:3:4:5:6:7:88") << Ip6(1,2,3,4,5,6,7,0x88) << "1:2:3:4:5:6:7:88";
    QTest::newRow("1:2:3:4:5:6:7:888") << Ip6(1,2,3,4,5,6,7,0x888) << "1:2:3:4:5:6:7:888";
    QTest::newRow("1:2:3:4:5:6:7:8888") << Ip6(1,2,3,4,5,6,7,0x8888) << "1:2:3:4:5:6:7:8888";
    QTest::newRow("1:2:3:4:5:6:7:8880") << Ip6(1,2,3,4,5,6,7,0x8880) << "1:2:3:4:5:6:7:8880";
    QTest::newRow("1:2:3:4:5:6:7:8808") << Ip6(1,2,3,4,5,6,7,0x8808) << "1:2:3:4:5:6:7:8808";
    QTest::newRow("1:2:3:4:5:6:7:8088") << Ip6(1,2,3,4,5,6,7,0x8088) << "1:2:3:4:5:6:7:8088";

    QTest::newRow("1:2:3:4:5:6:7:0") << Ip6(1,2,3,4,5,6,7,0) << "1:2:3:4:5:6:7:0";
    QTest::newRow("0:1:2:3:4:5:6:7") << Ip6(0,1,2,3,4,5,6,7) << "0:1:2:3:4:5:6:7";

    QTest::newRow("1:2:3:4:5:6::") << Ip6(1,2,3,4,5,6,0,0) << "1:2:3:4:5:6::";
    QTest::newRow("::1:2:3:4:5:6") << Ip6(0,0,1,2,3,4,5,6) << "::1:2:3:4:5:6";
    QTest::newRow("1:0:0:2::3") << Ip6(1,0,0,2,0,0,0,3) << "1:0:0:2::3";
    QTest::newRow("1:::2:0:0:3") << Ip6(1,0,0,0,2,0,0,3) << "1::2:0:0:3";
    QTest::newRow("1::2:0:0:0") << Ip6(1,0,0,0,2,0,0,0) << "1::2:0:0:0";
    QTest::newRow("0:0:0:1::") << Ip6(0,0,0,1,0,0,0,0) << "0:0:0:1::";
    QTest::newRow("::1:0:0:0") << Ip6(0,0,0,0,1,0,0,0) << "::1:0:0:0";
    QTest::newRow("ff02::1") << Ip6(0xff02,0,0,0,0,0,0,1) << "ff02::1";
    QTest::newRow("1::1") << Ip6(1,0,0,0,0,0,0,1) << "1::1";
    QTest::newRow("::1") << Ip6(0,0,0,0,0,0,0,1) << "::1";
    QTest::newRow("1::") << Ip6(1,0,0,0,0,0,0,0) << "1::";
    QTest::newRow("::") << Ip6(0,0,0,0,0,0,0,0) << "::";

    QTest::newRow("::1.2.3.4") << Ip6(0,0,0,0,0,0,0x102,0x304) << "::1.2.3.4";
    QTest::newRow("::ffff:1.2.3.4") << Ip6(0,0,0,0,0,0xffff,0x102,0x304) << "::ffff:1.2.3.4";
}

void tst_QIpAddress::ip6ToString()
{
    QFETCH(Ip6, ip);
    QFETCH(QString, expected);

#if defined(__GLIBC__) && defined(AF_INET6)
    {
        char buf[INET6_ADDRSTRLEN];
        bool ok = inet_ntop(AF_INET6, ip.u8, buf, sizeof buf) != 0;
        QVERIFY(ok);
        QCOMPARE(QString(buf), expected);
    }
#endif

    QString result;
    QIPAddressUtils::toString(result, ip.u8);
    QCOMPARE(result, expected);
}

QTEST_APPLESS_MAIN(tst_QIpAddress)

#include "tst_qipaddress.moc"
