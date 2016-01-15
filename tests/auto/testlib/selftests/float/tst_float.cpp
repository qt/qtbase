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

class tst_float: public QObject
{
    Q_OBJECT
private slots:
    void floatComparisons() const;
    void floatComparisons_data() const;
    void compareFloatTests() const;
    void compareFloatTests_data() const;
};

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

    QTest::newRow("should SUCCEED 1")
        << float(0)
        << float(0);

    QTest::newRow("should FAIL 1")
        << float(1.00000)
        << float(3.00000);

    QTest::newRow("should FAIL 2")
        << float(1.00000e-7f)
        << float(3.00000e-7f);

    // QCOMPARE for floats uses qFuzzyCompare(), which succeeds if the numbers
    // differ by no more than 1/100,000th of the smaller value.  Thus
    // QCOMPARE(99998, 99999) should fail, while QCOMPARE(100001, 100002)
    // should pass.

    QTest::newRow("should FAIL 3")
        << float(99998)
        << float(99999);

    QTest::newRow("should SUCCEED 2")
        << float(100001)
        << float(100002);
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
