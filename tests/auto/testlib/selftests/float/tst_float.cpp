/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QtCore/QCoreApplication>
#include <QtCore/qfloat16.h>
#include <QtTest/QtTest>
#include <QDebug>

#include "emulationdetector.h"

// Test proper handling of floating-point types
class tst_float: public QObject
{
    Q_OBJECT
private slots:
    void doubleComparisons() const;
    void doubleComparisons_data() const;
    void floatComparisons() const;
    void floatComparisons_data() const;
    void float16Comparisons() const;
    void float16Comparisons_data() const;
    void compareFloatTests() const;
    void compareFloatTests_data() const;
};

template<typename F>
static void nonFinite_data(F zero, F one)
{
    using Bounds = std::numeric_limits<F>;

    // QCOMPARE special-cases non-finite values
    if (Bounds::has_quiet_NaN) {
        const F nan = Bounds::quiet_NaN();
        QTest::newRow("should PASS: NaN == NaN") << nan << nan;
        QTest::newRow("should FAIL: NaN != 0") << nan << zero;
        QTest::newRow("should FAIL: 0 != NaN") << zero << nan;
        QTest::newRow("should FAIL: NaN != 1") << nan << one;
        QTest::newRow("should FAIL: 1 != NaN") << one << nan;
    }

    if (Bounds::has_infinity) {
        const F uge = Bounds::infinity();
        QTest::newRow("should PASS: inf == inf") << uge << uge;
        QTest::newRow("should PASS: -inf == -inf") << -uge << -uge;
        QTest::newRow("should FAIL: inf != -inf") << uge << -uge;
        QTest::newRow("should FAIL: -inf != inf") << -uge << uge;
        if (Bounds::has_quiet_NaN) {
            const F nan = Bounds::quiet_NaN();
            QTest::newRow("should FAIL: inf != nan") << uge << nan;
            QTest::newRow("should FAIL: nan != inf") << nan << uge;
            QTest::newRow("should FAIL: -inf != nan") << -uge << nan;
            QTest::newRow("should FAIL: nan != -inf") << nan << -uge;
        }
        QTest::newRow("should FAIL: inf != 0") << uge << zero;
        QTest::newRow("should FAIL: 0 != inf") << zero << uge;
        QTest::newRow("should FAIL: -inf != 0") << -uge << zero;
        QTest::newRow("should FAIL: 0 != -inf") << zero << -uge;
        QTest::newRow("should FAIL: inf != 1") << uge << one;
        QTest::newRow("should FAIL: 1 != inf") << one << uge;
        QTest::newRow("should FAIL: -inf != 1") << -uge << one;
        QTest::newRow("should FAIL: 1 != -inf") << one << -uge;

        const F big = Bounds::max();
        QTest::newRow("should FAIL: inf != max") << uge << big;
        QTest::newRow("should FAIL: inf != -max") << uge << -big;
        QTest::newRow("should FAIL: max != inf") << big << uge;
        QTest::newRow("should FAIL: -max != inf") << -big << uge;
        QTest::newRow("should FAIL: -inf != max") << -uge << big;
        QTest::newRow("should FAIL: -inf != -max") << -uge << -big;
        QTest::newRow("should FAIL: max != -inf") << big << -uge;
        QTest::newRow("should FAIL: -max != -inf") << -big << -uge;
    }
}

void tst_float::doubleComparisons() const
{
    QFETCH(double, operandLeft);
    QFETCH(double, operandRight);

    QCOMPARE(operandLeft, operandRight);
}

void tst_float::doubleComparisons_data() const
{
    QTest::addColumn<double>("operandLeft");
    QTest::addColumn<double>("operandRight");
    double zero(0.), one(1.);

    QTest::newRow("should FAIL 1") << one << 3.;
    QTest::newRow("should PASS 1") << zero << zero;
    QTest::newRow("should FAIL 2") << 1.e-7 << 3.e-7;

    // QCOMPARE() uses qFuzzyCompare(), which succeeds if doubles differ by no
    // more than 1e-12 times the smaller value; but QCOMPARE() also considers
    // values equal if qFuzzyIsNull() is true for both, so all doubles smaller
    // than 1e-12 are equal.  Thus QCOMPARE(1e12-2, 1e12-1) should fail, while
    // QCOMPARE(1e12+1, 1e12+2) should pass, as should QCOMPARE(1e-12-2e-24,
    // 1e-12-1e-24), despite the values differing by more than one part in 1e12.

    QTest::newRow("should PASS 2") << 1e12 + one << 1e12 + 2.;
    QTest::newRow("should FAIL 3") << 1e12 - one << 1e12 - 2.;
    QTest::newRow("should PASS 3") << 1e-12 << -1e-12;
    // ... but rounding makes that a bit unrelaible when scaled close to the bounds.
    QTest::newRow("should FAIL 4") << 1e-12 + 1e-24 << 1e-12 - 1e-24;
    QTest::newRow("should PASS 4") << 1e307 + 1e295 << 1e307 + 2e295;
    QTest::newRow("should FAIL 5") << 1e307 - 1e295 << 1e307 - 3e295;

    nonFinite_data(zero, one);
}

void tst_float::floatComparisons() const
{
    QFETCH(float, operandLeft);
    QFETCH(float, operandRight);

    QCOMPARE(operandLeft, operandRight);
}

void tst_float::floatComparisons_data() const
{
    QTest::addColumn<float>("operandLeft");
    QTest::addColumn<float>("operandRight");
    float zero(0.f), one(1.f);

    QTest::newRow("should FAIL 1") << one << 3.f;
    QTest::newRow("should PASS 1") << zero << zero;
    QTest::newRow("should FAIL 2") << 1.e-5f << 3.e-5f;

    // QCOMPARE() uses qFuzzyCompare(), which succeeds if the floats differ by
    // no more than 1e-5 times the smaller value; but QCOMPARE() also considers
    // values equal if qFuzzyIsNull is true for both, so all floats smaller than
    // 1e-5 are equal.  Thus QCOMPARE(1e5-2, 1e5-1) should fail, while
    // QCOMPARE(1e5+1, 1e5+2) should pass, as should QCOMPARE(1e-5-2e-10,
    // 1e-5-1e-10), despite the values differing by more than one part in 1e5.

    QTest::newRow("should PASS 2") << 1e5f + one << 1e5f + 2.f;
    QTest::newRow("should FAIL 3") << 1e5f - one << 1e5f - 2.f;
    QTest::newRow("should PASS 3") << 1e-5f << -1e-5f;
    // ... but rounding makes that a bit unrelaible when scaled close to the bounds.
    QTest::newRow("should FAIL 4") << 1e-5f + 1e-10f << 1e-5f - 1e-10f;
    QTest::newRow("should PASS 4") << 1e38f + 1e33f << 1e38f + 2e33f;
    QTest::newRow("should FAIL 5") << 1e38f - 1e33f << 1e38f - 3e33f;

    nonFinite_data(zero, one);
}

void tst_float::float16Comparisons() const
{
    QFETCH(qfloat16, operandLeft);
    QFETCH(qfloat16, operandRight);

    QCOMPARE(operandLeft, operandRight);
}

void tst_float::float16Comparisons_data() const
{
    QTest::addColumn<qfloat16>("operandLeft");
    QTest::addColumn<qfloat16>("operandRight");
    const qfloat16 zero(0), one(1);
    const qfloat16 tiny(EmulationDetector::isRunningArmOnX86() ? 0.00099f : 0.001f);

    QTest::newRow("should FAIL 1") << one << qfloat16(3);
    QTest::newRow("should PASS 1") << zero << zero;
    QTest::newRow("should FAIL 2") << qfloat16(1e-3f) << qfloat16(3e-3f);

    // QCOMPARE for uses qFuzzyCompare(), which ignores differences of one part
    // in 102.5 and considers any two qFuzzyIsNull() values, i.e. values smaller
    // than 1e-3, equal
    QTest::newRow("should PASS 2") << qfloat16(1001) << qfloat16(1002);
    QTest::newRow("should FAIL 3") << qfloat16(98) << qfloat16(99);
    QTest::newRow("should PASS 3") << tiny << -tiny;
    // ... which gets a bit unreliable near to the type's bounds
    QTest::newRow("should FAIL 4") << qfloat16(1.01e-3f) << qfloat16(0.99e-3f);
    QTest::newRow("should PASS 4") << qfloat16(6e4) + qfloat16(700) << qfloat16(6e4) + qfloat16(1200);
    QTest::newRow("should FAIL 5") << qfloat16(6e4) - qfloat16(600) << qfloat16(6e4) - qfloat16(1200);

    nonFinite_data(zero, one);
}

void tst_float::compareFloatTests() const
{
    QFETCH(float, t1);

    // Create two more values
    // t2 differs from t1 by 1 ppm (part per million)
    // t3 differs from t1 by 200%
    // We should consider that t1 == t2 and t1 != t3 (provided at least one is > 1e-5)
    const float t2 = t1 + (t1 / 1e6);
    const float t3 = 3 * t1;

    QCOMPARE(t1, t2);

    /* Should FAIL. */
    QCOMPARE(t1, t3);
}

void tst_float::compareFloatTests_data() const
{
    QTest::addColumn<float>("t1");
    QTest::newRow("1e0") << 1e0f;
    QTest::newRow("1e-5") << 1e-5f;
    QTest::newRow("1e+7") << 1e+7f;
}

QTEST_MAIN(tst_float)

#include "tst_float.moc"
