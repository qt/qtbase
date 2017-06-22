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


#include <QtTest/QtTest>
#include <QtCore/qendian.h>
#include <QtCore/private/qendian_p.h>


class tst_QtEndian: public QObject
{
    Q_OBJECT

private slots:
    void fromBigEndian();
    void fromLittleEndian();

    void toBigEndian();
    void toLittleEndian();

    void endianIntegers_data();
    void endianIntegers();

    void endianBitfields();
};

struct TestData
{
    quint64 data64;
    quint32 data32;
    quint16 data16;
    quint8 data8;

    quint8 reserved;
};

union RawTestData
{
    uchar rawData[sizeof(TestData)];
    TestData data;
};

static const TestData inNativeEndian = { Q_UINT64_C(0x0123456789abcdef), 0x00c0ffee, 0xcafe, 0xcf, '\0' };
static const RawTestData inBigEndian = { "\x01\x23\x45\x67\x89\xab\xcd\xef" "\x00\xc0\xff\xee" "\xca\xfe" "\xcf" };
static const RawTestData inLittleEndian = { "\xef\xcd\xab\x89\x67\x45\x23\x01" "\xee\xff\xc0\x00" "\xfe\xca" "\xcf" };

#define EXPAND_ENDIAN_TEST(endian)          \
    do {                                    \
        /* Unsigned tests */                \
        ENDIAN_TEST(endian, quint, 64);     \
        ENDIAN_TEST(endian, quint, 32);     \
        ENDIAN_TEST(endian, quint, 16);     \
        ENDIAN_TEST(endian, quint, 8);      \
                                            \
        /* Signed tests */                  \
        ENDIAN_TEST(endian, qint, 64);      \
        ENDIAN_TEST(endian, qint, 32);      \
        ENDIAN_TEST(endian, qint, 16);      \
        ENDIAN_TEST(endian, qint, 8);       \
    } while (false)                         \
    /**/

#define ENDIAN_TEST(endian, type, size)                                                 \
    do {                                                                                \
        QCOMPARE(qFrom ## endian ## Endian(                                             \
                    (type ## size)(in ## endian ## Endian.data.data ## size)),          \
                (type ## size)(inNativeEndian.data ## size));                           \
        QCOMPARE(qFrom ## endian ## Endian<type ## size>(                               \
                    in ## endian ## Endian.rawData + offsetof(TestData, data ## size)), \
                (type ## size)(inNativeEndian.data ## size));                           \
    } while (false)                                                                     \
    /**/

void tst_QtEndian::fromBigEndian()
{
    EXPAND_ENDIAN_TEST(Big);
}

void tst_QtEndian::fromLittleEndian()
{
    EXPAND_ENDIAN_TEST(Little);
}

#undef ENDIAN_TEST


#define ENDIAN_TEST(endian, type, size)                                                 \
    do {                                                                                \
        QCOMPARE(qTo ## endian ## Endian(                                               \
                    (type ## size)(inNativeEndian.data ## size)),                       \
                (type ## size)(in ## endian ## Endian.data.data ## size));              \
                                                                                        \
        RawTestData test;                                                               \
        qTo ## endian ## Endian(                                                        \
                (type ## size)(inNativeEndian.data ## size),                            \
                test.rawData + offsetof(TestData, data ## size));                       \
        QCOMPARE(test.data.data ## size, in ## endian ## Endian.data.data ## size );    \
    } while (false)                                                                     \
    /**/

void tst_QtEndian::toBigEndian()
{
    EXPAND_ENDIAN_TEST(Big);
}

void tst_QtEndian::toLittleEndian()
{
    EXPAND_ENDIAN_TEST(Little);
}

#undef ENDIAN_TEST

void tst_QtEndian::endianIntegers_data()
{
    QTest::addColumn<int>("val");

    QTest::newRow("-30000") << -30000;
    QTest::newRow("-1") << -1;
    QTest::newRow("0") << 0;
    QTest::newRow("1020") << 1020;
    QTest::newRow("16385") << 16385;
}

void tst_QtEndian::endianIntegers()
{
    QFETCH(int, val);

    qint16 vi16 = val;
    qint32 vi32 = val;
    qint64 vi64 = val;
    quint16 vu16 = val;
    quint32 vu32 = val;
    quint64 vu64 = val;

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    QCOMPARE(*reinterpret_cast<qint16_be*>(&vi16), vi16);
    QCOMPARE(*reinterpret_cast<qint32_be*>(&vi32), vi32);
    QCOMPARE(*reinterpret_cast<qint64_be*>(&vi64), vi64);
    QCOMPARE(*reinterpret_cast<qint16_le*>(&vi16), qbswap(vi16));
    QCOMPARE(*reinterpret_cast<qint32_le*>(&vi32), qbswap(vi32));
    QCOMPARE(*reinterpret_cast<qint64_le*>(&vi64), qbswap(vi64));
    QCOMPARE(*reinterpret_cast<quint16_be*>(&vu16), vu16);
    QCOMPARE(*reinterpret_cast<quint32_be*>(&vu32), vu32);
    QCOMPARE(*reinterpret_cast<quint64_be*>(&vu64), vu64);
    QCOMPARE(*reinterpret_cast<quint16_le*>(&vu16), qbswap(vu16));
    QCOMPARE(*reinterpret_cast<quint32_le*>(&vu32), qbswap(vu32));
    QCOMPARE(*reinterpret_cast<quint64_le*>(&vu64), qbswap(vu64));
#else
    QCOMPARE(*reinterpret_cast<qint16_be*>(&vi16), qbswap(vi16));
    QCOMPARE(*reinterpret_cast<qint32_be*>(&vi32), qbswap(vi32));
    QCOMPARE(*reinterpret_cast<qint64_be*>(&vi64), qbswap(vi64));
    QCOMPARE(*reinterpret_cast<qint16_le*>(&vi16), vi16);
    QCOMPARE(*reinterpret_cast<qint32_le*>(&vi32), vi32);
    QCOMPARE(*reinterpret_cast<qint64_le*>(&vi64), vi64);
    QCOMPARE(*reinterpret_cast<quint16_be*>(&vu16), qbswap(vu16));
    QCOMPARE(*reinterpret_cast<quint32_be*>(&vu32), qbswap(vu32));
    QCOMPARE(*reinterpret_cast<quint64_be*>(&vu64), qbswap(vu64));
    QCOMPARE(*reinterpret_cast<quint16_le*>(&vu16), vu16);
    QCOMPARE(*reinterpret_cast<quint32_le*>(&vu32), vu32);
    QCOMPARE(*reinterpret_cast<quint64_le*>(&vu64), vu64);
#endif
}

void tst_QtEndian::endianBitfields()
{
    union {
        quint32_be_bitfield<21, 11> upper;
        quint32_be_bitfield<10, 11> lower;
        qint32_be_bitfield<0, 10> bottom;
    } u;

    u.upper = 200;
    QCOMPARE(u.upper, 200U);
    u.lower = 1000;
    u.bottom = -8;
    QCOMPARE(u.lower, 1000U);
    QCOMPARE(u.upper, 200U);

    u.lower += u.upper;
    QCOMPARE(u.upper, 200U);
    QCOMPARE(u.lower, 1200U);

    u.upper = 65536 + 7;
    u.lower = 65535;
    QCOMPARE(u.lower, 65535U & ((1<<11) - 1));
    QCOMPARE(u.upper, 7U);

    QCOMPARE(u.bottom, -8);
}

QTEST_MAIN(tst_QtEndian)
#include "tst_qtendian.moc"
