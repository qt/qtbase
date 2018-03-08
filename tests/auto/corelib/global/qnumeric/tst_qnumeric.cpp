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


#include <QtTest/QtTest>
#include <QtGlobal>
#include "private/qnumeric_p.h"

#include <math.h>
#include <float.h>

class tst_QNumeric: public QObject
{
    Q_OBJECT

private slots:
    void fuzzyCompare_data();
    void fuzzyCompare();
    void qNan();
    void floatDistance_data();
    void floatDistance();
    void floatDistance_double_data();
    void floatDistance_double();
    void addOverflow_data();
    void addOverflow();
    void mulOverflow_data();
    void mulOverflow();
    void signedOverflow();
};

void tst_QNumeric::fuzzyCompare_data()
{
    QTest::addColumn<double>("val1");
    QTest::addColumn<double>("val2");
    QTest::addColumn<bool>("isEqual");

    QTest::newRow("zero") << 0.0 << 0.0 << true;
    QTest::newRow("ten") << 10.0 << 10.0 << true;
    QTest::newRow("large") << 1000000000.0 << 1000000000.0 << true;
    QTest::newRow("small") << 0.00000000001 << 0.00000000001 << true;
    QTest::newRow("eps") << 10.000000000000001 << 10.00000000000002 << true;
    QTest::newRow("eps2") << 10.000000000000001 << 10.000000000000009 << true;

    QTest::newRow("mis1") << 0.0 << 1.0 << false;
    QTest::newRow("mis2") << 0.0 << 10000000.0 << false;
    QTest::newRow("mis3") << 0.0 << 0.000000001 << false;
    QTest::newRow("mis4") << 100000000.0 << 0.000000001 << false;
    QTest::newRow("mis5") << 0.0000000001 << 0.000000001 << false;
}

void tst_QNumeric::fuzzyCompare()
{
    QFETCH(double, val1);
    QFETCH(double, val2);
    QFETCH(bool, isEqual);

    QCOMPARE(::qFuzzyCompare(val1, val2), isEqual);
    QCOMPARE(::qFuzzyCompare(val2, val1), isEqual);
    QCOMPARE(::qFuzzyCompare(-val1, -val2), isEqual);
    QCOMPARE(::qFuzzyCompare(-val2, -val1), isEqual);
}

#if defined __FAST_MATH__ && (__GNUC__ * 100 + __GNUC_MINOR__ >= 404)
   // turn -ffast-math off
#  pragma GCC optimize "no-fast-math"
#endif

void tst_QNumeric::qNan()
{
#if defined __FAST_MATH__ && (__GNUC__ * 100 + __GNUC_MINOR__ < 404)
    QSKIP("Non-conformant fast math mode is enabled, cannot run test");
#endif
    double nan = qQNaN();
    QVERIFY(!(0 > nan));
    QVERIFY(!(0 < nan));
    QVERIFY(qIsNaN(nan));
    QVERIFY(qIsNaN(nan + 1));
    QVERIFY(qIsNaN(-nan));

    Q_STATIC_ASSERT(sizeof(double) == 8);
#ifdef Q_LITTLE_ENDIAN
    const uchar bytes[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x7f };
#else
    const uchar bytes[] = { 0x7f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
#endif
    memcpy(&nan, bytes, 8);
    QVERIFY(!qIsFinite(nan));
    QVERIFY(!qIsInf(nan));
    QVERIFY(qIsNaN(nan));

    double inf = qInf();
    QVERIFY(inf > 0);
    QVERIFY(-inf < 0);
    QVERIFY(qIsInf(inf));
    QVERIFY(qIsInf(-inf));
    QVERIFY(qIsInf(2*inf));
    QCOMPARE(1/inf, 0.0);
    QVERIFY(qIsNaN(0*nan));
    QVERIFY(qIsNaN(0*inf));
    QVERIFY(qFuzzyCompare(1/inf, 0.0));
}

void tst_QNumeric::floatDistance_data()
{
    QTest::addColumn<float>("val1");
    QTest::addColumn<float>("val2");
    QTest::addColumn<quint32>("expectedDistance");

    // exponent: 8 bits
    // mantissa: 23 bits
    const quint32 number_of_denormals = (1 << 23) - 1;  // Set to 0 if denormals are not included

    quint32 _0_to_1 = quint32((1 << 23) * 126 + 1 + number_of_denormals); // We need +1 to include the 0
    quint32 _1_to_2 = quint32(1 << 23);

    // We don't need +1 because FLT_MAX has all bits set in the mantissa. (Thus mantissa
    // have not wrapped back to 0, which would be the case for 1 in _0_to_1
    quint32 _0_to_FLT_MAX = quint32((1 << 23) * 254) + number_of_denormals;

    quint32 _0_to_FLT_MIN = 1 + number_of_denormals;
    QTest::newRow("[0,FLT_MIN]") << 0.F << FLT_MIN << _0_to_FLT_MIN;
    QTest::newRow("[0,FLT_MAX]") << 0.F << FLT_MAX << _0_to_FLT_MAX;
    QTest::newRow("[1,1.5]") << 1.0F << 1.5F << quint32(1 << 22);
    QTest::newRow("[0,1]") << 0.F << 1.0F << _0_to_1;
    QTest::newRow("[0.5,1]") << 0.5F << 1.0F << quint32(1 << 23);
    QTest::newRow("[1,2]") << 1.F << 2.0F << _1_to_2;
    QTest::newRow("[-1,+1]") << -1.F << +1.0F << 2 * _0_to_1;
    QTest::newRow("[-1,0]") << -1.F << 0.0F << _0_to_1;
    QTest::newRow("[-1,FLT_MAX]") << -1.F << FLT_MAX << _0_to_1 + _0_to_FLT_MAX;
    QTest::newRow("[-2,-1") << -2.F << -1.F << _1_to_2;
    QTest::newRow("[-1,-2") << -1.F << -2.F << _1_to_2;
    QTest::newRow("[FLT_MIN,FLT_MAX]") << FLT_MIN << FLT_MAX << _0_to_FLT_MAX - _0_to_FLT_MIN;
    QTest::newRow("[-FLT_MAX,FLT_MAX]") << -FLT_MAX << FLT_MAX << (2*_0_to_FLT_MAX);
    float denormal = FLT_MIN;
    denormal/=2.0F;
    QTest::newRow("denormal") << 0.F << denormal << _0_to_FLT_MIN/2;
}

void tst_QNumeric::floatDistance()
{
    QFETCH(float, val1);
    QFETCH(float, val2);
    QFETCH(quint32, expectedDistance);
#ifdef Q_OS_QNX
    QEXPECT_FAIL("denormal", "See QTBUG-37094", Continue);
#endif
    QCOMPARE(qFloatDistance(val1, val2), expectedDistance);
}

void tst_QNumeric::floatDistance_double_data()
{
    QTest::addColumn<double>("val1");
    QTest::addColumn<double>("val2");
    QTest::addColumn<quint64>("expectedDistance");

    // exponent: 11 bits
    // mantissa: 52 bits
    const quint64 number_of_denormals = (Q_UINT64_C(1) << 52) - 1;  // Set to 0 if denormals are not included

    quint64 _0_to_1 = (Q_UINT64_C(1) << 52) * ((1 << (11-1)) - 2) + 1 + number_of_denormals; // We need +1 to include the 0
    quint64 _1_to_2 = Q_UINT64_C(1) << 52;

    // We don't need +1 because DBL_MAX has all bits set in the mantissa. (Thus mantissa
    // have not wrapped back to 0, which would be the case for 1 in _0_to_1
    quint64 _0_to_DBL_MAX = quint64((Q_UINT64_C(1) << 52) * ((1 << 11) - 2)) + number_of_denormals;

    quint64 _0_to_DBL_MIN = 1 + number_of_denormals;
    QTest::newRow("[0,DBL_MIN]") << 0.0 << DBL_MIN << _0_to_DBL_MIN;
    QTest::newRow("[0,DBL_MAX]") << 0.0 << DBL_MAX << _0_to_DBL_MAX;
    QTest::newRow("[1,1.5]") << 1.0 << 1.5 << (Q_UINT64_C(1) << 51);
    QTest::newRow("[0,1]") << 0.0 << 1.0 << _0_to_1;
    QTest::newRow("[0.5,1]") << 0.5 << 1.0 << (Q_UINT64_C(1) << 52);
    QTest::newRow("[1,2]") << 1.0 << 2.0 << _1_to_2;
    QTest::newRow("[-1,+1]") << -1.0 << +1.0 << 2 * _0_to_1;
    QTest::newRow("[-1,0]") << -1.0 << 0.0 << _0_to_1;
    QTest::newRow("[-1,DBL_MAX]") << -1.0 << DBL_MAX << _0_to_1 + _0_to_DBL_MAX;
    QTest::newRow("[-2,-1") << -2.0 << -1.0 << _1_to_2;
    QTest::newRow("[-1,-2") << -1.0 << -2.0 << _1_to_2;
    QTest::newRow("[DBL_MIN,DBL_MAX]") << DBL_MIN << DBL_MAX << _0_to_DBL_MAX - _0_to_DBL_MIN;
    QTest::newRow("[-DBL_MAX,DBL_MAX]") << -DBL_MAX << DBL_MAX << (2*_0_to_DBL_MAX);
    double denormal = DBL_MIN;
    denormal/=2.0;
    QTest::newRow("denormal") << 0.0 << denormal << _0_to_DBL_MIN/2;
}

void tst_QNumeric::floatDistance_double()
{
    QFETCH(double, val1);
    QFETCH(double, val2);
    QFETCH(quint64, expectedDistance);
#ifdef Q_OS_QNX
    QEXPECT_FAIL("denormal", "See QTBUG-37094", Continue);
#endif
    QCOMPARE(qFloatDistance(val1, val2), expectedDistance);
}

void tst_QNumeric::addOverflow_data()
{
    QTest::addColumn<int>("size");

    // for unsigned, all sizes are supported
    QTest::newRow("quint8") << 8;
    QTest::newRow("quint16") << 16;
    QTest::newRow("quint32") << 32;
    QTest::newRow("quint64") << 64;
    QTest::newRow("ulong") << 48;   // it's either 32- or 64-bit, so on average it's 48 :-)

    // for signed, we can't guarantee 64-bit
    QTest::newRow("qint8") << -8;
    QTest::newRow("qint16") << -16;
    QTest::newRow("qint32") << -32;
    if (sizeof(void *) == sizeof(qint64))
        QTest::newRow("qint64") << -64;
}

// Note: in release mode, all the tests may be statically determined and only the calls
// to QTest::toString and QTest::qCompare will remain.
template <typename Int> static void addOverflow_template()
{
#if defined(Q_CC_MSVC) && Q_CC_MSVC < 2000
    QSKIP("Test disabled, this test generates an Internal Compiler Error compiling in release mode");
#else
    const Int max = std::numeric_limits<Int>::max();
    const Int min = std::numeric_limits<Int>::min();
    Int r;

    // basic values
    QCOMPARE(add_overflow(Int(0), Int(0), &r), false);
    QCOMPARE(r, Int(0));
    QCOMPARE(add_overflow(Int(1), Int(0), &r), false);
    QCOMPARE(r, Int(1));
    QCOMPARE(add_overflow(Int(0), Int(1), &r), false);
    QCOMPARE(r, Int(1));

    QCOMPARE(sub_overflow(Int(0), Int(0), &r), false);
    QCOMPARE(r, Int(0));
    QCOMPARE(sub_overflow(Int(1), Int(0), &r), false);
    QCOMPARE(r, Int(1));
    QCOMPARE(sub_overflow(Int(1), Int(1), &r), false);
    QCOMPARE(r, Int(0));
    QCOMPARE(sub_overflow(Int(0), Int(1), &r), !min);
    if (min)
        QCOMPARE(r, Int(-1));

    // half-way through max
    QCOMPARE(add_overflow(Int(max/2), Int(max/2), &r), false);
    QCOMPARE(r, Int(max / 2 * 2));
    QCOMPARE(sub_overflow(Int(max/2), Int(max/2), &r), false);
    QCOMPARE(r, Int(0));
    QCOMPARE(add_overflow(Int(max/2 - 1), Int(max/2 + 1), &r), false);
    QCOMPARE(r, Int(max / 2 * 2));
    QCOMPARE(sub_overflow(Int(max/2 - 1), Int(max/2 + 1), &r), !min);
    if (min)
        QCOMPARE(r, Int(-2));
    QCOMPARE(add_overflow(Int(max/2 + 1), Int(max/2), &r), false);
    QCOMPARE(r, max);
    QCOMPARE(sub_overflow(Int(max/2 + 1), Int(max/2), &r), false);
    QCOMPARE(r, Int(1));
    QCOMPARE(add_overflow(Int(max/2), Int(max/2 + 1), &r), false);
    QCOMPARE(r, max);
    QCOMPARE(sub_overflow(Int(max/2), Int(max/2 + 1), &r), !min);
    if (min)
        QCOMPARE(r, Int(-1));

    QCOMPARE(add_overflow(Int(min/2), Int(min/2), &r), false);
    QCOMPARE(r, Int(min / 2 * 2));
    QCOMPARE(sub_overflow(Int(min/2), Int(min/2), &r), false);
    QCOMPARE(r, Int(0));
    QCOMPARE(add_overflow(Int(min/2 - 1), Int(min/2 + 1), &r), !min);
    if (min)
        QCOMPARE(r, Int(min / 2 * 2));
    QCOMPARE(sub_overflow(Int(min/2 - 1), Int(min/2 + 1), &r), false);
    QCOMPARE(r, Int(-2));
    QCOMPARE(sub_overflow(Int(min/2 + 1), Int(min/2), &r), false);
    QCOMPARE(r, Int(1));
    QCOMPARE(sub_overflow(Int(min/2), Int(min/2 + 1), &r), !min);
    if (min)
        QCOMPARE(r, Int(-1));

    // more than half
    QCOMPARE(add_overflow(Int(max/4 * 3), Int(max/4), &r), false);
    QCOMPARE(r, Int(max / 4 * 4));

    // max
    QCOMPARE(add_overflow(max, Int(0), &r), false);
    QCOMPARE(r, max);
    QCOMPARE(sub_overflow(max, Int(0), &r), false);
    QCOMPARE(r, max);
    QCOMPARE(add_overflow(Int(0), max, &r), false);
    QCOMPARE(r, max);
    QCOMPARE(sub_overflow(Int(0), max, &r), !min);
    if (min)
        QCOMPARE(r, Int(-max));

    QCOMPARE(add_overflow(min, Int(0), &r), false);
    QCOMPARE(r, min);
    QCOMPARE(sub_overflow(min, Int(0), &r), false);
    QCOMPARE(r, min);
    QCOMPARE(add_overflow(Int(0), min, &r), false);
    QCOMPARE(r, min);
    QCOMPARE(sub_overflow(Int(0), Int(min+1), &r), !min);
    if (min)
        QCOMPARE(r, Int(-(min+1)));

    // 64-bit issues
    if (max > std::numeric_limits<uint>::max()) {
        QCOMPARE(add_overflow(Int(std::numeric_limits<uint>::max()), Int(std::numeric_limits<uint>::max()), &r), false);
        QCOMPARE(r, Int(2 * Int(std::numeric_limits<uint>::max())));
        QCOMPARE(sub_overflow(Int(std::numeric_limits<uint>::max()), Int(std::numeric_limits<uint>::max()), &r), false);
        QCOMPARE(r, Int(0));
    }
    if (min && min < -Int(std::numeric_limits<uint>::max())) {
        QCOMPARE(add_overflow(Int(-Int(std::numeric_limits<uint>::max())), Int(-Int(std::numeric_limits<uint>::max())), &r), false);
        QCOMPARE(r, Int(-2 * Int(std::numeric_limits<uint>::max())));
        QCOMPARE(sub_overflow(Int(-Int(std::numeric_limits<uint>::max())), Int(-Int(std::numeric_limits<uint>::max())), &r), false);
        QCOMPARE(r, Int(0));
    }

    // overflows past max
    QCOMPARE(add_overflow(max, Int(1), &r), true);
    QCOMPARE(add_overflow(Int(1), max, &r), true);
    QCOMPARE(add_overflow(Int(max/2 + 1), Int(max/2 + 1), &r), true);
    if (!min) {
        QCOMPARE(sub_overflow(Int(-max), Int(-2), &r), true);
        QCOMPARE(sub_overflow(Int(max/2 - 1), Int(max/2 + 1), &r), true);
    }

    // overflows past min (in case of min == 0, repeats some tests above)
    if (min) {
        QCOMPARE(sub_overflow(min, Int(1), &r), true);
        QCOMPARE(sub_overflow(Int(1), min, &r), true);
        QCOMPARE(sub_overflow(Int(min/2 - 1), Int(-Int(min/2)), &r), true);
        QCOMPARE(add_overflow(min, Int(-1), &r), true);
        QCOMPARE(add_overflow(Int(-1), min, &r), true);
    }
#endif
}

void tst_QNumeric::addOverflow()
{
    QFETCH(int, size);
    if (size == 8)
        addOverflow_template<quint8>();
    if (size == 16)
        addOverflow_template<quint16>();
    if (size == 32)
        addOverflow_template<quint32>();
    if (size == 48)
        addOverflow_template<ulong>();  // not really 48-bit
    if (size == 64)
        addOverflow_template<quint64>();

    if (size == -8)
        addOverflow_template<qint8>();
    if (size == -16)
        addOverflow_template<qint16>();
    if (size == -32)
        addOverflow_template<qint32>();
    if (size == -64)
        addOverflow_template<qint64>();
}

void tst_QNumeric::mulOverflow_data()
{
    addOverflow_data();
}

// Note: in release mode, all the tests may be statically determined and only the calls
// to QTest::toString and QTest::qCompare will remain.
template <typename Int> static void mulOverflow_template()
{
#if defined(Q_CC_MSVC) && Q_CC_MSVC < 1900
    QSKIP("Test disabled, this test generates an Internal Compiler Error compiling");
#else
    const Int max = std::numeric_limits<Int>::max();
    const Int min = std::numeric_limits<Int>::min();

    //  for unsigned (even number of significant bits):  mid2 = mid1 - 1
    //  for signed (odd number of significant bits):     mid2 = mid1 / 2 - 1
    const Int mid1 = Int(Int(1) << sizeof(Int) * CHAR_BIT / 2);
    const Int mid2 = (std::numeric_limits<Int>::digits % 2 ? mid1 / 2 : mid1) - 1;

    Int r;

    // basic multiplications
    QCOMPARE(mul_overflow(Int(0), Int(0), &r), false);
    QCOMPARE(r, Int(0));
    QCOMPARE(mul_overflow(Int(1), Int(0), &r), false);
    QCOMPARE(r, Int(0));
    QCOMPARE(mul_overflow(Int(0), Int(1), &r), false);
    QCOMPARE(r, Int(0));
    QCOMPARE(mul_overflow(max, Int(0), &r), false);
    QCOMPARE(r, Int(0));
    QCOMPARE(mul_overflow(Int(0), max, &r), false);
    QCOMPARE(r, Int(0));
    QCOMPARE(mul_overflow(min, Int(0), &r), false);
    QCOMPARE(r, Int(0));
    QCOMPARE(mul_overflow(Int(0), min, &r), false);
    QCOMPARE(r, Int(0));

    QCOMPARE(mul_overflow(Int(1), Int(1), &r), false);
    QCOMPARE(r, Int(1));
    QCOMPARE(mul_overflow(Int(1), max, &r), false);
    QCOMPARE(r, max);
    QCOMPARE(mul_overflow(max, Int(1), &r), false);
    QCOMPARE(r, max);
    QCOMPARE(mul_overflow(Int(1), min, &r), false);
    QCOMPARE(r, min);
    QCOMPARE(mul_overflow(min, Int(1), &r), false);
    QCOMPARE(r, min);

    // almost max
    QCOMPARE(mul_overflow(mid1, mid2, &r), false);
    QCOMPARE(r, Int(max - mid1 + 1));
    QCOMPARE(mul_overflow(Int(max / 2), Int(2), &r), false);
    QCOMPARE(r, Int(max & ~Int(1)));
    QCOMPARE(mul_overflow(Int(max / 4), Int(4), &r), false);
    QCOMPARE(r, Int(max & ~Int(3)));
    if (min) {
        QCOMPARE(mul_overflow(Int(-mid1), mid2, &r), false);
        QCOMPARE(r, Int(-max + mid1 - 1));
        QCOMPARE(mul_overflow(Int(-max / 2), Int(2), &r), false);
        QCOMPARE(r, Int(-max + 1));
        QCOMPARE(mul_overflow(Int(-max / 4), Int(4), &r), false);
        QCOMPARE(r, Int(-max + 3));

        QCOMPARE(mul_overflow(Int(-mid1), Int(mid2 + 1), &r), false);
        QCOMPARE(r, min);
        QCOMPARE(mul_overflow(mid1, Int(-mid2 - 1), &r), false);
        QCOMPARE(r, min);
    }

    // overflows
    QCOMPARE(mul_overflow(max, Int(2), &r), true);
    QCOMPARE(mul_overflow(Int(max / 2), Int(3), &r), true);
    QCOMPARE(mul_overflow(mid1, Int(mid2 + 1), &r), true);
    QCOMPARE(mul_overflow(Int(max / 2 + 2), Int(2), &r), true);
    QCOMPARE(mul_overflow(Int(1ULL << (std::numeric_limits<Int>::digits - 1)), Int(2), &r), true);

    if (min) {
        QCOMPARE(mul_overflow(min, Int(2), &r), true);
        QCOMPARE(mul_overflow(Int(min / 2), Int(3), &r), true);
        QCOMPARE(mul_overflow(Int(min / 2 - 1), Int(2), &r), true);
        QCOMPARE(mul_overflow(Int(min + min/2), Int(2), &r), true);
    }
#endif
}

template <typename Int, bool enabled = sizeof(Int) <= sizeof(void*)> struct MulOverflowDispatch;
template <typename Int> struct MulOverflowDispatch<Int, true>
{
    void operator()() { mulOverflow_template<Int>(); }
};
template <typename Int> struct MulOverflowDispatch<Int, false>
{
    void operator()() { QSKIP("This type is too big for this architecture"); }
};

void tst_QNumeric::mulOverflow()
{
    QFETCH(int, size);
    if (size == 8)
        MulOverflowDispatch<quint8>()();
    if (size == 16)
        MulOverflowDispatch<quint16>()();
    if (size == 32)
        MulOverflowDispatch<quint32>()();
    if (size == 48)
        MulOverflowDispatch<ulong>()();     // not really 48-bit
    if (size == 64)
        MulOverflowDispatch<quint64>()();

    if (size == -8)
        MulOverflowDispatch<qint8>()();
    if (size == -16)
        MulOverflowDispatch<qint16>()();
    if (size == -32)
        MulOverflowDispatch<qint32>()();
    if (size == -64) {
#if QT_POINTER_SIZE == 8
        MulOverflowDispatch<qint64>()();
#else
        QFAIL("128-bit multiplication not supported on this platform");
#endif
    }
}

void tst_QNumeric::signedOverflow()
{
    const int minInt = std::numeric_limits<int>::min();
    const int maxInt = std::numeric_limits<int>::max();
    int r;

    QCOMPARE(add_overflow(minInt + 1, int(-1), &r), false);
    QCOMPARE(add_overflow(minInt, int(-1), &r), true);
    QCOMPARE(add_overflow(minInt, minInt, &r), true);
    QCOMPARE(add_overflow(maxInt - 1, int(1), &r), false);
    QCOMPARE(add_overflow(maxInt, int(1), &r), true);
    QCOMPARE(add_overflow(maxInt, maxInt, &r), true);

    QCOMPARE(sub_overflow(minInt + 1, int(1), &r), false);
    QCOMPARE(sub_overflow(minInt, int(1), &r), true);
    QCOMPARE(sub_overflow(minInt, maxInt, &r), true);
    QCOMPARE(sub_overflow(maxInt - 1, int(-1), &r), false);
    QCOMPARE(sub_overflow(maxInt, int(-1), &r), true);
    QCOMPARE(sub_overflow(maxInt, minInt, &r), true);

    QCOMPARE(mul_overflow(minInt, int(1), &r), false);
    QCOMPARE(mul_overflow(minInt, int(-1), &r), true);
    QCOMPARE(mul_overflow(minInt, int(2), &r), true);
    QCOMPARE(mul_overflow(minInt, minInt, &r), true);
    QCOMPARE(mul_overflow(maxInt, int(1), &r), false);
    QCOMPARE(mul_overflow(maxInt, int(-1), &r), false);
    QCOMPARE(mul_overflow(maxInt, int(2), &r), true);
    QCOMPARE(mul_overflow(maxInt, maxInt, &r), true);
}

QTEST_APPLESS_MAIN(tst_QNumeric)
#include "tst_qnumeric.moc"
