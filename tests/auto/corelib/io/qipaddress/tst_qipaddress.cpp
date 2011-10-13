/****************************************************************************
**
** Copyright (C) 2012 Intel Corporation.
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
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
};

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

QTEST_APPLESS_MAIN(tst_QIpAddress)

#include "tst_qipaddress.moc"
