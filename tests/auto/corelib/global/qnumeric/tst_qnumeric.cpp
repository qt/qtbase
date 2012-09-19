/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

void tst_QNumeric::qNan()
{
    double nan = qQNaN();
#if defined( __INTEL_COMPILER)
    QCOMPARE((0 > nan), false);
    QCOMPARE((0 < nan), false);
    QSKIP("This fails due to a bug in the Intel Compiler");
#else
    if (0 > nan)
        QFAIL("compiler thinks 0 > nan");

#  if defined(Q_CC_DIAB)
    QWARN("!(0 < nan) would fail due to a bug in dcc");
#  else
    if (0 < nan)
        QFAIL("compiler thinks 0 < nan");
#  endif
#endif
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
    QVERIFY(qIsNaN(0*nan));
    QVERIFY(qIsNaN(0*inf));
    QVERIFY(qFuzzyCompare(1/inf, 0.0));
}

QTEST_APPLESS_MAIN(tst_QNumeric)
#include "tst_qnumeric.moc"
