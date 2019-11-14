/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

namespace {
    template <typename F> struct Fuzzy {};
    /* Data taken from qglobal.h's implementation of qFuzzyCompare:
     * qFuzzyCompare conflates values with fractional difference up to (and
     * including) the given scale.
     */
    template <> struct Fuzzy<double> { constexpr static double scale = 1e12; };
    template <> struct Fuzzy<float> { constexpr static float scale = 1e5f; };
}

class tst_QNumeric: public QObject
{
    Q_OBJECT

    // Support for floating-point:
    template<typename F> inline void fuzzyCompare_data();
    template<typename F> inline void fuzzyCompare();
    template<typename F> inline void checkNaN(F nan);
    template<typename F> inline void rawNaN_data();
    template<typename F> inline void rawNaN();
#if QT_CONFIG(signaling_nan)
    template<typename F> inline void distinctNaN();
#endif
    template<typename F, typename Whole> inline void generalNaN_data();
    template<typename F, typename Whole> inline void generalNaN();
    template<typename F> inline void infinity();
    template<typename F> inline void classifyfp();
    template<typename F, typename Count> inline void distance_data();
    template<typename F, typename Count> inline void distance();

private slots:
    // Floating-point tests:
    void fuzzyCompareF_data() { fuzzyCompare_data<float>(); }
    void fuzzyCompareF() { fuzzyCompare<float>(); }
    void fuzzyCompareD_data() { fuzzyCompare_data<double>(); }
    void fuzzyCompareD() { fuzzyCompare<double>(); }
    void rawNaNF_data() { rawNaN_data<float>(); }
    void rawNaNF() { rawNaN<float>(); }
    void rawNaND_data() { rawNaN_data<double>(); }
    void rawNaND() { rawNaN<double>(); }
#if QT_CONFIG(signaling_nan)
    void distinctNaNF();
    void distinctNaND() { distinctNaN<double>(); }
#endif
    void generalNaNd_data() { generalNaN_data<double, quint64>(); }
    void generalNaNd() { generalNaN<double, quint64>(); }
    void generalNaNf_data() { generalNaN_data<float, quint32>(); }
    void generalNaNf() { generalNaN<float, quint32>(); }
    void infinityF() { infinity<float>(); }
    void infinityD() { infinity<double>(); }
    void classifyF() { classifyfp<float>(); }
    void classifyD() { classifyfp<double>(); }
    void floatDistance_data() { distance_data<float, quint32>(); }
    void floatDistance() { distance<float, quint32>(); }
    void doubleDistance_data() { distance_data<double, quint64>(); }
    void doubleDistance() { distance<double, quint64>(); }

    // Whole number tests:
    void addOverflow_data();
    void addOverflow();
    void mulOverflow_data();
    void mulOverflow();
    void signedOverflow();
};

// Floating-point tests:

template<typename F>
void tst_QNumeric::fuzzyCompare_data()
{
    QTest::addColumn<F>("val1");
    QTest::addColumn<F>("val2");
    QTest::addColumn<bool>("isEqual");
    const F zero(0), one(1), ten(10);
    const F huge = Fuzzy<F>::scale, tiny = one / huge;
    const F deci(.1), giga(1e9), nano(1e-9), big(1e7), small(1e-10);

    QTest::newRow("zero") << zero << zero << true;
    QTest::newRow("ten") << ten << ten << true;
    QTest::newRow("large") << giga << giga << true;
    QTest::newRow("small") << small << small << true;
    QTest::newRow("10+9*tiny==10") << (ten + 9 * tiny) << ten << true;
    QTest::newRow("huge+.9==huge") << (huge + 9 * deci) << huge << true;
    QTest::newRow("eps2") << (ten + tiny) << (ten + 2 * tiny) << true;
    QTest::newRow("eps9") << (ten + tiny) << (ten + 9 * tiny) << true;

    QTest::newRow("0!=1") << zero << one << false;
    QTest::newRow("0!=big") << zero << big << false;
    QTest::newRow("0!=nano") << zero << nano << false;
    QTest::newRow("giga!=nano") << giga << nano << false;
    QTest::newRow("small!=nano") << small << nano << false;
    QTest::newRow("huge+1.1!=huge") << (huge + 1 + deci) << huge << false;
    QTest::newRow("1+1.1*tiny!=1") << (one + tiny * (one + deci)) << one << false;
}

template<typename F>
void tst_QNumeric::fuzzyCompare()
{
    QFETCH(F, val1);
    QFETCH(F, val2);
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

template<typename F>
void tst_QNumeric::checkNaN(F nan)
{
#define CHECKNAN(value) \
    do { \
        const F v = (value); \
        QCOMPARE(qFpClassify(v), FP_NAN); \
        QVERIFY(qIsNaN(v)); \
        QVERIFY(!qIsFinite(v)); \
        QVERIFY(!qIsInf(v)); \
    } while (0)

    QVERIFY(!(0 > nan));
    QVERIFY(!(0 < nan));
    QVERIFY(!(0 == nan));
    QVERIFY(!(nan == nan));

    CHECKNAN(nan);
    CHECKNAN(nan + 1);
    CHECKNAN(nan - 1);
    CHECKNAN(-nan);
    CHECKNAN(nan * 2.0);
    CHECKNAN(nan / 2.0);
    CHECKNAN(1.0 / nan);
    CHECKNAN(0.0 / nan);
    CHECKNAN(0.0 * nan);

    // When any NaN is expected, any NaN will do:
    QCOMPARE(nan, nan);
    QCOMPARE(nan, -nan);
    QCOMPARE(nan, qQNaN());
#undef CHECKNAN
}

template<typename F>
void tst_QNumeric::rawNaN_data()
{
#if defined __FAST_MATH__ && (__GNUC__ * 100 + __GNUC_MINOR__ < 404)
    QSKIP("Non-conformant fast math mode is enabled, cannot run test");
#endif
    QTest::addColumn<F>("nan");

    QTest::newRow("quiet") << F(qQNaN());
#if QT_CONFIG(signaling_nan)
    QTest::newRow("signaling") << F(qSNaN());
#endif
}

template<typename F>
void tst_QNumeric::rawNaN()
{
    QFETCH(F, nan);
#ifdef Q_OS_WASM
#  ifdef __asmjs
    QEXPECT_FAIL("", "Fastcomp conflates quiet and signaling NaNs", Continue);
#  endif // but the modern clang compiler handls it fine.
#endif
    checkNaN(nan);
}

#if QT_CONFIG(signaling_nan)
template<typename F>
void tst_QNumeric::distinctNaN()
{
    const F qnan = qQNaN();
    const F snan = qSNaN();
    QVERIFY(memcmp(&qnan, &snan, sizeof(F)) != 0);
}

void tst_QNumeric::distinctNaNF() {
#ifdef Q_CC_MSVC
    QEXPECT_FAIL("", "MSVC's float conflates quiet and signaling NaNs", Continue);
#endif
    distinctNaN<float>();
}
#endif // signaling_nan

template<typename F, typename Whole>
void tst_QNumeric::generalNaN_data()
{
    Q_STATIC_ASSERT(sizeof(F) == sizeof(Whole));
    QTest::addColumn<Whole>("whole");
    // Every value with every bit of the exponent set is a NaN.
    // Sign and mantissa can be anything without interfering with that.
    using Bounds = std::numeric_limits<F>;
    // Bounds::digits is one more than the number of bits used to encode the mantissa:
    const int mantissaBits = Bounds::digits - 1;
    // One bit for sign, the rest are mantissa and exponent:
    const int exponentBits = sizeof(F) * CHAR_BIT - 1 - mantissaBits;

    const Whole exponent = ((Whole(1) << exponentBits) - 1) << mantissaBits;
    const Whole sign = Whole(1) << (exponentBits + mantissaBits);
    const Whole mantissaTop = Whole(1) << (mantissaBits - 1);

    QTest::newRow("lowload") << (exponent | 1);
    QTest::newRow("sign-lowload") << (sign | exponent | 1);
    QTest::newRow("highload") << (exponent | mantissaTop);
    QTest::newRow("sign-highload") << (sign | exponent | mantissaTop);
}

template<typename F, typename Whole>
void tst_QNumeric::generalNaN()
{
    Q_STATIC_ASSERT(sizeof(F) == sizeof(Whole));
    QFETCH(const Whole, whole);
    F nan;
    memcpy(&nan, &whole, sizeof(F));
    checkNaN(nan);
}

template<typename F>
void tst_QNumeric::infinity()
{
    const F inf = qInf();
    const F zero(0), one(1), two(2);
    QVERIFY(inf > zero);
    QVERIFY(-inf < zero);
    QVERIFY(qIsInf(inf));
    QCOMPARE(inf, inf);
    QCOMPARE(-inf, -inf);
    QVERIFY(qIsInf(-inf));
    QVERIFY(qIsInf(inf + one));
    QVERIFY(qIsInf(inf - one));
    QVERIFY(qIsInf(-inf - one));
    QVERIFY(qIsInf(-inf + one));
    QVERIFY(qIsInf(inf * two));
    QVERIFY(qIsInf(-inf * two));
    QVERIFY(qIsInf(inf / two));
    QVERIFY(qIsInf(-inf / two));
    QVERIFY(qFuzzyCompare(one / inf, zero));
    QCOMPARE(1.0 / inf, 0.0);
    QVERIFY(qFuzzyCompare(one / -inf, zero));
    QCOMPARE(one / -inf, zero);
    QVERIFY(qIsNaN(zero * inf));
    QVERIFY(qIsNaN(zero * -inf));
}

template<typename F>
void tst_QNumeric::classifyfp()
{
    using Bounds = std::numeric_limits<F>;
    const F huge = Bounds::max();
    const F tiny = Bounds::min();
    // NaNs already handled, see checkNaN()'s callers.
    const F one(1), two(2), inf(qInf());

    QCOMPARE(qFpClassify(inf), FP_INFINITE);
    QCOMPARE(qFpClassify(-inf), FP_INFINITE);
    QCOMPARE(qFpClassify(huge * two), FP_INFINITE);
    QCOMPARE(qFpClassify(huge * -two), FP_INFINITE);

    QCOMPARE(qFpClassify(one), FP_NORMAL);
    QCOMPARE(qFpClassify(huge), FP_NORMAL);
    QCOMPARE(qFpClassify(-huge), FP_NORMAL);
    QCOMPARE(qFpClassify(tiny), FP_NORMAL);
    QCOMPARE(qFpClassify(-tiny), FP_NORMAL);
    if (Bounds::has_denorm == std::denorm_present) {
        QCOMPARE(qFpClassify(tiny / two), FP_SUBNORMAL);
        QCOMPARE(qFpClassify(tiny / -two), FP_SUBNORMAL);
    }
}

template<typename F, typename Count>
void tst_QNumeric::distance_data()
{
    using Bounds = std::numeric_limits<F>;
    const F huge = Bounds::max();
    const F tiny = Bounds::min();

    QTest::addColumn<F>("from");
    QTest::addColumn<F>("stop");
    QTest::addColumn<Count>("expectedDistance");

    using Bounds = std::numeric_limits<F>;
    const int mantissaBits = Bounds::digits - 1;
    const int exponentBits = sizeof(F) * CHAR_BIT - 1 - mantissaBits;

    // Set to 1 and 0 if denormals are not included:
    const Count count_0_to_tiny = Count(1) << mantissaBits;
    const Count count_denormals = count_0_to_tiny - 1;

    // We need +1 to include the 0:
    const Count count_0_to_1
        = (Count(1) << mantissaBits) * ((Count(1) << (exponentBits - 1)) - 2)
          + 1 + count_denormals;
    const Count count_1_to_2 = Count(1) << mantissaBits;

    // We don't need +1 because huge has all bits set in the mantissa. (Thus mantissa
    // have not wrapped back to 0, which would be the case for 1 in _0_to_1
    const Count count_0_to_huge
        = (Count(1) << mantissaBits) * ((Count(1) << exponentBits) - 2)
          + count_denormals;

    const F zero(0), half(.5), one(1), sesqui(1.5), two(2);
    const F denormal = tiny / two;

    QTest::newRow("[0,tiny]") << zero << tiny << count_0_to_tiny;
    QTest::newRow("[0,huge]") << zero << huge << count_0_to_huge;
    QTest::newRow("[1,1.5]") << one << sesqui << (Count(1) << (mantissaBits - 1));
    QTest::newRow("[0,1]") << zero << one << count_0_to_1;
    QTest::newRow("[0.5,1]") << half << one << (Count(1) << mantissaBits);
    QTest::newRow("[1,2]") << one << two << count_1_to_2;
    QTest::newRow("[-1,+1]") << -one << +one << 2 * count_0_to_1;
    QTest::newRow("[-1,0]") << -one << zero << count_0_to_1;
    QTest::newRow("[-1,huge]") << -one << huge << count_0_to_1 + count_0_to_huge;
    QTest::newRow("[-2,-1") << -two << -one << count_1_to_2;
    QTest::newRow("[-1,-2") << -one << -two << count_1_to_2;
    QTest::newRow("[tiny,huge]") << tiny << huge << count_0_to_huge - count_0_to_tiny;
    QTest::newRow("[-huge,huge]") << -huge << huge << (2 * count_0_to_huge);
    QTest::newRow("denormal") << zero << denormal << count_0_to_tiny / 2;
}

template<typename F, typename Count>
void tst_QNumeric::distance()
{
    QFETCH(F, from);
    QFETCH(F, stop);
    QFETCH(Count, expectedDistance);
#ifdef Q_OS_QNX
    QEXPECT_FAIL("denormal", "See QTBUG-37094", Continue);
#endif
    QCOMPARE(qFloatDistance(from, stop), expectedDistance);
}

// Whole number tests:

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
    QCOMPARE(mul_overflow(Int(max - max / 2), Int(2), &r), true);
    QCOMPARE(mul_overflow(Int(1ULL << (std::numeric_limits<Int>::digits - 1)), Int(2), &r), true);

    if (min) {
        QCOMPARE(mul_overflow(min, Int(2), &r), true);
        QCOMPARE(mul_overflow(Int(min / 2), Int(3), &r), true);
        QCOMPARE(mul_overflow(Int(min / 2 - 1), Int(2), &r), true);
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
