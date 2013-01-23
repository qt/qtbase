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

class tst_QNumeric: public QObject
{
    Q_OBJECT

private slots:
    void fuzzyCompare_data();
    void fuzzyCompare();
    void qNan();
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

QTEST_APPLESS_MAIN(tst_QNumeric)
#include "tst_qnumeric.moc"
