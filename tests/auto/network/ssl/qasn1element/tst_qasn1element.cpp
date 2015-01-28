/****************************************************************************
**
** Copyright (C) 2014 Jeremy Lainé <jeremy.laine@m4x.org>
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
#include "private/qasn1element_p.h"

class tst_QAsn1Element : public QObject
{
    Q_OBJECT

private slots:
    void emptyConstructor();
    void equals_data();
    void equals();
    void toBool_data();
    void toBool();
    void dateTime_data();
    void dateTime();
    void integer_data();
    void integer();
    void invalid_data();
    void invalid();
    void octetString_data();
    void octetString();
    void objectIdentifier_data();
    void objectIdentifier();
    void string_data();
    void string();
};

void tst_QAsn1Element::emptyConstructor()
{
    QAsn1Element elem;
    QCOMPARE(elem.type(), quint8(0));
    QCOMPARE(elem.value(), QByteArray());
}

Q_DECLARE_METATYPE(QAsn1Element)

void tst_QAsn1Element::equals_data()
{
    QTest::addColumn<QAsn1Element>("a");
    QTest::addColumn<QAsn1Element>("b");
    QTest::addColumn<bool>("equals");

    QTest::newRow("equal")
        << QAsn1Element(QAsn1Element::BooleanType, QByteArray("\0", 1))
        << QAsn1Element(QAsn1Element::BooleanType, QByteArray("\0", 1))
        << true;
    QTest::newRow("different type")
        << QAsn1Element(QAsn1Element::BooleanType, QByteArray("\0", 1))
        << QAsn1Element(QAsn1Element::IntegerType, QByteArray("\0", 1))
        << false;
    QTest::newRow("different value")
        << QAsn1Element(QAsn1Element::BooleanType, QByteArray("\0", 1))
        << QAsn1Element(QAsn1Element::BooleanType, QByteArray("\xff", 1))
        << false;
}

void tst_QAsn1Element::equals()
{
    QFETCH(QAsn1Element, a);
    QFETCH(QAsn1Element, b);
    QFETCH(bool, equals);
    QCOMPARE(a == b, equals);
    QCOMPARE(a != b, !equals);
}

void tst_QAsn1Element::toBool_data()
{
    QTest::addColumn<QByteArray>("encoded");
    QTest::addColumn<bool>("value");
    QTest::addColumn<bool>("valid");

    QTest::newRow("bad type") << QByteArray::fromHex("0201ff") << false << false;
    QTest::newRow("bad value") << QByteArray::fromHex("010102") << false << false;
    QTest::newRow("false") << QByteArray::fromHex("010100") << false << true;
    QTest::newRow("true") << QByteArray::fromHex("0101ff") << true << true;
}

void tst_QAsn1Element::toBool()
{
    QFETCH(QByteArray, encoded);
    QFETCH(bool, value);
    QFETCH(bool, valid);

    bool ok;
    QAsn1Element elem;
    QVERIFY(elem.read(encoded));
    QCOMPARE(elem.toBool(&ok), value);
    QCOMPARE(ok, valid);
}

void tst_QAsn1Element::dateTime_data()
{
    QTest::addColumn<QByteArray>("encoded");
    QTest::addColumn<QDateTime>("value");

    QTest::newRow("bad type")
        << QByteArray::fromHex("020100")
        << QDateTime();
    QTest::newRow("UTCTime - 070417074026Z")
        << QByteArray::fromHex("170d3037303431373037343032365a")
        << QDateTime(QDate(2007, 4, 17), QTime(7, 40, 26), Qt::UTC);
    QTest::newRow("UTCTime - bad length")
        << QByteArray::fromHex("170c30373034313730373430325a")
        << QDateTime();
    QTest::newRow("UTCTime - no trailing Z")
        << QByteArray::fromHex("170d30373034313730373430323659")
        << QDateTime();
    QTest::newRow("GeneralizedTime - 20510829095341Z")
        << QByteArray::fromHex("180f32303531303832393039353334315a")
        << QDateTime(QDate(2051, 8, 29), QTime(9, 53, 41), Qt::UTC);
    QTest::newRow("GeneralizedTime - bad length")
        << QByteArray::fromHex("180e323035313038323930393533345a")
        << QDateTime();
    QTest::newRow("GeneralizedTime - no trailing Z")
        << QByteArray::fromHex("180f323035313038323930393533343159")
        << QDateTime();
}

void tst_QAsn1Element::dateTime()
{
    QFETCH(QByteArray, encoded);
    QFETCH(QDateTime, value);

    QAsn1Element elem;
    QVERIFY(elem.read(encoded));
    QCOMPARE(elem.toDateTime(), value);
}

void tst_QAsn1Element::integer_data()
{
    QTest::addColumn<QByteArray>("encoded");
    QTest::addColumn<int>("value");

    QTest::newRow("0") << QByteArray::fromHex("020100") << 0;
    QTest::newRow("127") << QByteArray::fromHex("02017F") << 127;
    QTest::newRow("128") << QByteArray::fromHex("02020080") << 128;
    QTest::newRow("256") << QByteArray::fromHex("02020100") << 256;
}

void tst_QAsn1Element::integer()
{
    QFETCH(QByteArray, encoded);
    QFETCH(int, value);

    // read
    bool ok;
    QAsn1Element elem;
    QVERIFY(elem.read(encoded));
    QCOMPARE(elem.type(), quint8(QAsn1Element::IntegerType));
    QCOMPARE(elem.toInteger(&ok), value);
    QVERIFY(ok);

    // write
    QByteArray buffer;
    QDataStream stream(&buffer, QIODevice::WriteOnly);
    QAsn1Element::fromInteger(value).write(stream);
    QCOMPARE(buffer, encoded);
}

void tst_QAsn1Element::invalid_data()
{
    QTest::addColumn<QByteArray>("encoded");

    QTest::newRow("empty") << QByteArray();
    QTest::newRow("bad type") << QByteArray::fromHex("000100");
    QTest::newRow("truncated value") << QByteArray::fromHex("0401");
}

void tst_QAsn1Element::invalid()
{
    QFETCH(QByteArray, encoded);

    QAsn1Element elem;
    QVERIFY(!elem.read(encoded));
}

void tst_QAsn1Element::octetString_data()
{
    QTest::addColumn<QByteArray>("encoded");
    QTest::addColumn<QByteArray>("value");

    QTest::newRow("0 byte") << QByteArray::fromHex("0400") << QByteArray();
    QTest::newRow("1 byte") << QByteArray::fromHex("040100") << QByteArray(1, '\0');
    QTest::newRow("127 bytes") << QByteArray::fromHex("047f") + QByteArray(127, '\0') << QByteArray(127, '\0');
    QTest::newRow("128 bytes") << QByteArray::fromHex("048180") + QByteArray(128, '\0') << QByteArray(128, '\0');
}

void tst_QAsn1Element::octetString()
{
    QFETCH(QByteArray, encoded);
    QFETCH(QByteArray, value);

    // read
    QAsn1Element elem;
    QVERIFY(elem.read(encoded));
    QCOMPARE(elem.type(), quint8(QAsn1Element::OctetStringType));
    QCOMPARE(elem.value(), value);

    // write
    QByteArray buffer;
    QDataStream stream(&buffer, QIODevice::WriteOnly);
    elem.write(stream);
    QCOMPARE(buffer, encoded);
}

void tst_QAsn1Element::objectIdentifier_data()
{
    QTest::addColumn<QByteArray>("encoded");
    QTest::addColumn<QByteArray>("oid");
    QTest::addColumn<QByteArray>("name");

    QTest::newRow("1.2.3.4")
        << QByteArray::fromHex("06032a0304")
        << QByteArray("1.2.3.4")
        << QByteArray("1.2.3.4");
    QTest::newRow("favouriteDrink")
        << QByteArray::fromHex("060a0992268993f22c640105")
        << QByteArray("0.9.2342.19200300.100.1.5")
        << QByteArray("favouriteDrink");
}

void tst_QAsn1Element::objectIdentifier()
{
    QFETCH(QByteArray, encoded);
    QFETCH(QByteArray, oid);
    QFETCH(QByteArray, name);

    QAsn1Element elem;
    QVERIFY(elem.read(encoded));
    QCOMPARE(elem.type(), quint8(QAsn1Element::ObjectIdentifierType));
    QCOMPARE(elem.toObjectId(), oid);
    QCOMPARE(QAsn1Element::fromObjectId(oid).toObjectId(), oid);
    QCOMPARE(elem.toObjectName(), name);
}

void tst_QAsn1Element::string_data()
{
    QTest::addColumn<QAsn1Element>("element");
    QTest::addColumn<QString>("value");

    QTest::newRow("printablestring")
        << QAsn1Element(QAsn1Element::PrintableStringType, QByteArray("Hello World"))
        << QStringLiteral("Hello World");
    QTest::newRow("teletextstring")
        << QAsn1Element(QAsn1Element::TeletexStringType, QByteArray("Hello World"))
        << QStringLiteral("Hello World");
    QTest::newRow("utf8string")
        << QAsn1Element(QAsn1Element::Utf8StringType, QByteArray("Hello World"))
        << QStringLiteral("Hello World");
    QTest::newRow("rfc822name")
        << QAsn1Element(QAsn1Element::Rfc822NameType, QByteArray("Hello World"))
        << QStringLiteral("Hello World");
    QTest::newRow("dnsname")
        << QAsn1Element(QAsn1Element::DnsNameType, QByteArray("Hello World"))
        << QStringLiteral("Hello World");
    QTest::newRow("uri")
        << QAsn1Element(QAsn1Element::UniformResourceIdentifierType, QByteArray("Hello World"))
        << QStringLiteral("Hello World");

    // Embedded NULs are not allowed and should be rejected
    QTest::newRow("evil_printablestring")
        << QAsn1Element(QAsn1Element::PrintableStringType, QByteArray("Hello\0World", 11))
        << QString();
    QTest::newRow("evil_teletextstring")
        << QAsn1Element(QAsn1Element::TeletexStringType, QByteArray("Hello\0World", 11))
        << QString();
    QTest::newRow("evil_utf8string")
        << QAsn1Element(QAsn1Element::Utf8StringType, QByteArray("Hello\0World", 11))
        << QString();
    QTest::newRow("evil_rfc822name")
        << QAsn1Element(QAsn1Element::Rfc822NameType, QByteArray("Hello\0World", 11))
        << QString();
    QTest::newRow("evil_dnsname")
        << QAsn1Element(QAsn1Element::DnsNameType, QByteArray("Hello\0World", 11))
        << QString();
    QTest::newRow("evil_uri")
        << QAsn1Element(QAsn1Element::UniformResourceIdentifierType, QByteArray("Hello\0World", 11))
        << QString();
}

void tst_QAsn1Element::string()
{
    QFETCH(QAsn1Element, element);
    QFETCH(QString, value);

    QCOMPARE(element.toString(), value);
}

QTEST_MAIN(tst_QAsn1Element)
#include "tst_qasn1element.moc"
