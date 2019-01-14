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

#include <QtCore/QCoreApplication>
#include <QtTest/QtTest>
#include <QDebug>

// Test proper handling of floating-point types
class tst_float: public QObject
{
    Q_OBJECT
private slots:
    void doubleComparisons() const;
    void doubleComparisons_data() const;
    void floatComparisons() const;
    void floatComparisons_data() const;
    void compareFloatTests() const;
    void compareFloatTests_data() const;
};

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

    QTest::newRow("should PASS 1") << 0. << 0.;
    QTest::newRow("should FAIL 1") << 1.00000 << 3.00000;
    QTest::newRow("should FAIL 2") << 1.00000e-7 << 3.00000e-7;

    // QCOMPARE for doubles uses qFuzzyCompare(), which succeeds if the numbers
    // differ by no more than 1e-12 times the smaller value.  Thus
    // QCOMPARE(1e12-2, 1e12-1) should fail, while QCOMPARE(1e12+1, 1e12+2)
    // should pass.

    QTest::newRow("should PASS 2") << 1e12 + 1. << 1e12 + 2.;
    QTest::newRow("should FAIL 3") << 1e12 - 1. << 1e12 - 2.;
    // ... but rounding makes that a bit unrelaible when scaled close to the bounds.
    QTest::newRow("should PASS 3") << 1e-310 + 1e-322 << 1e-310 + 2e-322;
    QTest::newRow("should FAIL 4") << 1e-310 - 1e-322 << 1e-310 - 3e-322;
    QTest::newRow("should PASS 4") << 1e307 + 1e295 << 1e307 + 2e295;
    QTest::newRow("should FAIL 5") << 1e307 - 1e295 << 1e307 - 3e295;

    // QCOMPARE special-cases non-finite values
    if (std::numeric_limits<double>::has_quiet_NaN) {
        const double nan = std::numeric_limits<double>::quiet_NaN();
        QTest::newRow("should PASS: NaN == NaN") << nan << nan;
        QTest::newRow("should FAIL: NaN != 0") << nan << 0.;
        QTest::newRow("should FAIL: 0 != NaN") << 0. << nan;
        QTest::newRow("should FAIL: NaN != 1") << nan << 1.;
        QTest::newRow("should FAIL: 1 != NaN") << 1. << nan;
    }
    if (std::numeric_limits<double>::has_infinity) {
        const double uge = std::numeric_limits<double>::infinity();
        QTest::newRow("should PASS: inf == inf") << uge << uge;
        QTest::newRow("should PASS: -inf == -inf") << -uge << -uge;
        QTest::newRow("should FAIL: inf != -inf") << uge << -uge;
        QTest::newRow("should FAIL: -inf != inf") << -uge << uge;
        if (std::numeric_limits<double>::has_quiet_NaN) {
            const double nan = std::numeric_limits<double>::quiet_NaN();
            QTest::newRow("should FAIL: inf != nan") << uge << nan;
            QTest::newRow("should FAIL: nan != inf") << nan << uge;
            QTest::newRow("should FAIL: -inf != nan") << -uge << nan;
            QTest::newRow("should FAIL: nan != -inf") << nan << -uge;
        }
        QTest::newRow("should FAIL: inf != 0") << uge << 0.;
        QTest::newRow("should FAIL: 0 != inf") << 0. << uge;
        QTest::newRow("should FAIL: -inf != 0") << -uge << 0.;
        QTest::newRow("should FAIL: 0 != -inf") << 0. << -uge;
        QTest::newRow("should FAIL: inf != 1") << uge << 1.;
        QTest::newRow("should FAIL: 1 != inf") << 1. << uge;
        QTest::newRow("should FAIL: -inf != 1") << -uge << 1.;
        QTest::newRow("should FAIL: 1 != -inf") << 1. << -uge;

        const double big = std::numeric_limits<double>::max();
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

    QTest::newRow("should FAIL 1") << 1.00000f << 3.00000f;
    QTest::newRow("should PASS 1") << 0.f << 0.f;
    QTest::newRow("should FAIL 2") << 1.00000e-7f << 3.00000e-7f;

    // QCOMPARE for floats uses qFuzzyCompare(), which succeeds if the numbers
    // differ by no more than 1e-5 times the smaller value.  Thus
    // QCOMPARE(1e5-2, 1e5-1) should fail, while QCOMPARE(1e5+1, 1e5+2)
    // should pass.

    QTest::newRow("should PASS 2") << 1e5f + 1.f << 1e5f + 2.f;
    QTest::newRow("should FAIL 3") << 1e5f - 1.f << 1e5f - 2.f;
    // ... but rounding makes that a bit unrelaible when scaled close to the bounds.
    QTest::newRow("should PASS 3") << 1e-39f + 1e-44f << 1e-39f + 2e-44f;
    QTest::newRow("should FAIL 4") << 1e-39f - 1e-44f << 1e-39f - 3e-44f;
    QTest::newRow("should PASS 4") << 1e38f + 1e33f << 1e38f + 2e33f;
    QTest::newRow("should FAIL 5") << 1e38f - 1e33f << 1e38f - 3e33f;

    // QCOMPARE special-cases non-finite values
    if (std::numeric_limits<float>::has_quiet_NaN) {
        const float nan = std::numeric_limits<float>::quiet_NaN();
        QTest::newRow("should PASS: NaN == NaN") << nan << nan;
        QTest::newRow("should FAIL: NaN != 0") << nan << 0.f;
        QTest::newRow("should FAIL: 0 != NaN") << 0.f << nan;
        QTest::newRow("should FAIL: NaN != 1") << nan << 1.f;
        QTest::newRow("should FAIL: 1 != NaN") << 1.f << nan;
    }
    if (std::numeric_limits<float>::has_infinity) {
        const float uge = std::numeric_limits<float>::infinity();
        QTest::newRow("should PASS: inf == inf") << uge << uge;
        QTest::newRow("should PASS: -inf == -inf") << -uge << -uge;
        QTest::newRow("should FAIL: inf != -inf") << uge << -uge;
        QTest::newRow("should FAIL: -inf != inf") << -uge << uge;
        if (std::numeric_limits<float>::has_quiet_NaN) {
            const float nan = std::numeric_limits<float>::quiet_NaN();
            QTest::newRow("should FAIL: inf != nan") << uge << nan;
            QTest::newRow("should FAIL: nan != inf") << nan << uge;
            QTest::newRow("should FAIL: -inf != nan") << -uge << nan;
            QTest::newRow("should FAIL: nan != -inf") << nan << -uge;
        }
        QTest::newRow("should FAIL: inf != 0") << uge << 0.f;
        QTest::newRow("should FAIL: 0 != inf") << 0.f << uge;
        QTest::newRow("should FAIL: -inf != 0") << -uge << 0.f;
        QTest::newRow("should FAIL: 0 != -inf") << 0.f << -uge;
        QTest::newRow("should FAIL: inf != 1") << uge << 1.f;
        QTest::newRow("should FAIL: 1 != inf") << 1.f << uge;
        QTest::newRow("should FAIL: -inf != 1") << -uge << 1.f;
        QTest::newRow("should FAIL: 1 != -inf") << 1.f << -uge;

        const float big = std::numeric_limits<float>::max();
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

void tst_float::compareFloatTests() const
{
    QFETCH(float, t1);

    // Create two more values
    // t2 differs from t1 by 1 ppm (part per million)
    // t3 differs from t1 by 200%
    // we should consider that t1 == t2 and t1 != t3
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
    QTest::newRow("1e-7") << 1e-7f;
    QTest::newRow("1e+7") << 1e+7f;
}

QTEST_MAIN(tst_float)

#include "tst_float.moc"
