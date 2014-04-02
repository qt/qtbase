/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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


#include <QtTest/QtTest>
#include <QtGlobal>

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
    double inf = qInf();
    QVERIFY(inf > 0);
    QVERIFY(-inf < 0);
    QVERIFY(qIsInf(inf));
    QVERIFY(qIsInf(-inf));
    QVERIFY(qIsInf(2*inf));
    QCOMPARE(1/inf, 0.0);
#ifdef Q_CC_INTEL
    QEXPECT_FAIL("", "ICC optimizes zero * anything to zero", Continue);
#endif
    QVERIFY(qIsNaN(0*nan));
#ifdef Q_CC_INTEL
    QEXPECT_FAIL("", "ICC optimizes zero * anything to zero", Continue);
#endif
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

QTEST_APPLESS_MAIN(tst_QNumeric)
#include "tst_qnumeric.moc"
