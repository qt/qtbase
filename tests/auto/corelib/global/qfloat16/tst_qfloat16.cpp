/****************************************************************************
**
** Copyright (C) 2016 by Southwest Research Institute (R)
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QFloat16>

#include <math.h>

class tst_qfloat16: public QObject
{
    Q_OBJECT

private slots:
    void fuzzyCompare_data();
    void fuzzyCompare();
    void ltgt_data();
    void ltgt();
    void qNan();
    void float_cast();
    void float_cast_data();
    void promotionTests();
};

void tst_qfloat16::fuzzyCompare_data()
{
    QTest::addColumn<qfloat16>("val1");
    QTest::addColumn<qfloat16>("val2");
    QTest::addColumn<bool>("fuzEqual");
    QTest::addColumn<bool>("isEqual");

    QTest::newRow("zero")  << qfloat16(0.0f)   << qfloat16(0.0f)   << true << true;
    QTest::newRow("ten")   << qfloat16(1e1f)   << qfloat16(1e1f)   << true << true;
    QTest::newRow("large") << qfloat16(1e4f)   << qfloat16(1e4f)   << true << true;
    QTest::newRow("small") << qfloat16(1e-5f)  << qfloat16(1e-5f)  << true << true;
    QTest::newRow("eps")   << qfloat16(10.01f) << qfloat16(10.02f) << true << false;
    QTest::newRow("eps2")  << qfloat16(1024.f) << qfloat16(1033.f) << true << false;

    QTest::newRow("mis1") << qfloat16(0.0f)   << qfloat16(1.0f)   << false << false;
    QTest::newRow("mis2") << qfloat16(0.0f)   << qfloat16(1e7f)   << false << false;
    QTest::newRow("mis3") << qfloat16(0.0f)   << qfloat16(1e-4f)  << false << false;
    QTest::newRow("mis4") << qfloat16(1e8f)   << qfloat16(1e-8f)  << false << false;
    QTest::newRow("mis5") << qfloat16(1e-4f)  << qfloat16(1e-5)   << false << false;
    QTest::newRow("mis6") << qfloat16(1024.f) << qfloat16(1034.f) << false << false;
}

void tst_qfloat16::fuzzyCompare()
{
    QFETCH(qfloat16, val1);
    QFETCH(qfloat16, val2);
    QFETCH(bool, fuzEqual);
    QFETCH(bool, isEqual);

    if (!isEqual && (val1==val2))
        qWarning() << "Identical arguments provided unintentionally!";

    if (fuzEqual) {
        QVERIFY(::qFuzzyCompare(val1, val2));
        QVERIFY(::qFuzzyCompare(val2, val1));
        QVERIFY(::qFuzzyCompare(-val1, -val2));
        QVERIFY(::qFuzzyCompare(-val2, -val1));
    } else {
        QVERIFY(!::qFuzzyCompare(val1, val2));
        QVERIFY(!::qFuzzyCompare(val2, val1));
        QVERIFY(!::qFuzzyCompare(-val1, -val2));
        QVERIFY(!::qFuzzyCompare(-val2, -val1));
    }
}

void tst_qfloat16::ltgt_data()
{
    QTest::addColumn<float>("val1");
    QTest::addColumn<float>("val2");

    QTest::newRow("zero")  << 0.0f << 0.0f;
    QTest::newRow("ten")   << 10.0f << 10.0f;
    QTest::newRow("large") << 100000.0f << 100000.0f;
    QTest::newRow("small") << 0.0000001f << 0.0000001f;
    QTest::newRow("eps")   << 10.000000000000001f << 10.00000000000002f;
    QTest::newRow("eps2")  << 10.000000000000001f << 10.000000000000009f;

    QTest::newRow("mis1") << 0.0f << 1.0f;
    QTest::newRow("mis2") << 0.0f << 10000000.0f;
    QTest::newRow("mis3") << 0.0f << 0.0001f;
    QTest::newRow("mis4") << 100000000.0f << 0.000000001f;
    QTest::newRow("mis5") << 0.0001f << 0.00001f;

    QTest::newRow("45,23") << 45.f << 23.f;
    QTest::newRow("1000,76") << 1000.f << 76.f;
}

void tst_qfloat16::ltgt()
{
    QFETCH(float, val1);
    QFETCH(float, val2);

    QCOMPARE(qfloat16(val1)  == qfloat16(val2),   val1 ==  val2);
    QCOMPARE(qfloat16(val1)  <  qfloat16(val2),   val1 <   val2);
    QCOMPARE(qfloat16(val1)  <= qfloat16(val2),   val1 <=  val2);
    QCOMPARE(qfloat16(val1)  >  qfloat16(val2),   val1 >   val2);
    QCOMPARE(qfloat16(val1)  >= qfloat16(val2),   val1 >=  val2);

    QCOMPARE(qfloat16(val1)  == qfloat16(-val2),  val1 == -val2);
    QCOMPARE(qfloat16(val1)  <  qfloat16(-val2),  val1 <  -val2);
    QCOMPARE(qfloat16(val1)  <= qfloat16(-val2),  val1 <= -val2);
    QCOMPARE(qfloat16(val1)  >  qfloat16(-val2),  val1 >  -val2);
    QCOMPARE(qfloat16(val1)  >= qfloat16(-val2),  val1 >= -val2);

    QCOMPARE(qfloat16(-val1) == qfloat16(val2),  -val1 ==  val2);
    QCOMPARE(qfloat16(-val1) <  qfloat16(val2),  -val1 <   val2);
    QCOMPARE(qfloat16(-val1) <= qfloat16(val2),  -val1 <=  val2);
    QCOMPARE(qfloat16(-val1) >  qfloat16(val2),  -val1 >   val2);
    QCOMPARE(qfloat16(-val1) >= qfloat16(val2),  -val1 >=  val2);

    QCOMPARE(qfloat16(-val1) == qfloat16(-val2), -val1 == -val2);
    QCOMPARE(qfloat16(-val1) <  qfloat16(-val2), -val1 <  -val2);
    QCOMPARE(qfloat16(-val1) <= qfloat16(-val2), -val1 <= -val2);
    QCOMPARE(qfloat16(-val1) >  qfloat16(-val2), -val1 >  -val2);
    QCOMPARE(qfloat16(-val1) >= qfloat16(-val2), -val1 >= -val2);
}

#if defined __FAST_MATH__ && (__GNUC__ * 100 + __GNUC_MINOR__ >= 404)
   // turn -ffast-math off
#  pragma GCC optimize "no-fast-math"
#endif

void tst_qfloat16::qNan()
{
#if defined __FAST_MATH__ && (__GNUC__ * 100 + __GNUC_MINOR__ < 404)
    QSKIP("Non-conformant fast math mode is enabled, cannot run test");
#endif
    qfloat16 nan = qQNaN();
    QVERIFY(!(0. > nan));
    QVERIFY(!(0. < nan));
    QVERIFY(qIsNaN(nan));
    QVERIFY(qIsNaN(nan + 1.f));
    QVERIFY(qIsNaN(-nan));
    qfloat16 inf = qInf();
    QVERIFY(inf > qfloat16(0));
    QVERIFY(-inf < qfloat16(0));
    QVERIFY(qIsInf(inf));
    QVERIFY(qIsInf(-inf));
    QVERIFY(qIsInf(2.f*inf));
    QVERIFY(qIsInf(inf*2.f));
    QCOMPARE(qfloat16(1.f/inf), qfloat16(0.f));
#ifdef Q_CC_INTEL
    QEXPECT_FAIL("", "ICC optimizes zero * anything to zero", Continue);
#endif
    QVERIFY(qIsNaN(nan*0.f));
#ifdef Q_CC_INTEL
    QEXPECT_FAIL("", "ICC optimizes zero * anything to zero", Continue);
#endif
    QVERIFY(qIsNaN(inf*0.f));
    QVERIFY(qFuzzyCompare(qfloat16(1.f/inf), qfloat16(0.0)));
}

void tst_qfloat16::float_cast_data()
{
    QTest::addColumn<float>("val");

    QTest::newRow("zero")  << 0.f;
    QTest::newRow("one")   << 1e0f;
    QTest::newRow("ten")   << 1e1f;
    QTest::newRow("hund")  << 1e2f;
    QTest::newRow("thou")  << 1e3f;
    QTest::newRow("tthou") << 1e4f;
    //QTest::newRow("hthou") << 1e5f;
    //QTest::newRow("mil")   << 1e6f;
    //QTest::newRow("tmil")  << 1e7f;
    //QTest::newRow("hmil")  << 1e8f;
}

void tst_qfloat16::float_cast()
{
    QFETCH(float, val);

    QVERIFY(qFuzzyCompare(float(qfloat16(val)),val));
    QVERIFY(qFuzzyCompare(float(qfloat16(-val)),-val));
}

void tst_qfloat16::promotionTests()
{
    QCOMPARE(sizeof(qfloat16),sizeof(qfloat16(1.f)+qfloat16(1.f)));
    QCOMPARE(sizeof(qfloat16),sizeof(qfloat16(1.f)-qfloat16(1.f)));
    QCOMPARE(sizeof(qfloat16),sizeof(qfloat16(1.f)*qfloat16(1.f)));
    QCOMPARE(sizeof(qfloat16),sizeof(qfloat16(1.f)/qfloat16(1.f)));

    QCOMPARE(sizeof(float),sizeof(1.f+qfloat16(1.f)));
    QCOMPARE(sizeof(float),sizeof(1.f-qfloat16(1.f)));
    QCOMPARE(sizeof(float),sizeof(1.f*qfloat16(1.f)));
    QCOMPARE(sizeof(float),sizeof(1.f/qfloat16(1.f)));

    QCOMPARE(sizeof(float),sizeof(qfloat16(1.f)+1.f));
    QCOMPARE(sizeof(float),sizeof(qfloat16(1.f)-1.f));
    QCOMPARE(sizeof(float),sizeof(qfloat16(1.f)*1.f));
    QCOMPARE(sizeof(float),sizeof(qfloat16(1.f)/1.f));

    QCOMPARE(sizeof(double),sizeof(1.+qfloat16(1.f)));
    QCOMPARE(sizeof(double),sizeof(1.-qfloat16(1.f)));
    QCOMPARE(sizeof(double),sizeof(1.*qfloat16(1.f)));
    QCOMPARE(sizeof(double),sizeof(1./qfloat16(1.f)));

    QCOMPARE(sizeof(double),sizeof(qfloat16(1.f)+1.));
    QCOMPARE(sizeof(double),sizeof(qfloat16(1.f)-1.));
    QCOMPARE(sizeof(double),sizeof(qfloat16(1.f)*1.));
    QCOMPARE(sizeof(double),sizeof(qfloat16(1.f)/1.));

    QCOMPARE(sizeof(long double),sizeof((long double)(1.)+qfloat16(1.f)));
    QCOMPARE(sizeof(long double),sizeof((long double)(1.)-qfloat16(1.f)));
    QCOMPARE(sizeof(long double),sizeof((long double)(1.)*qfloat16(1.f)));
    QCOMPARE(sizeof(long double),sizeof((long double)(1.)/qfloat16(1.f)));

    QCOMPARE(sizeof(long double),sizeof(qfloat16(1.f)+(long double)(1.)));
    QCOMPARE(sizeof(long double),sizeof(qfloat16(1.f)-(long double)(1.)));
    QCOMPARE(sizeof(long double),sizeof(qfloat16(1.f)*(long double)(1.)));
    QCOMPARE(sizeof(long double),sizeof(qfloat16(1.f)/(long double)(1.)));

    QCOMPARE(sizeof(double),sizeof(1+qfloat16(1.f)));
    QCOMPARE(sizeof(double),sizeof(1-qfloat16(1.f)));
    QCOMPARE(sizeof(double),sizeof(1*qfloat16(1.f)));
    QCOMPARE(sizeof(double),sizeof(1/qfloat16(1.f)));

    QCOMPARE(sizeof(double),sizeof(qfloat16(1.f)+1));
    QCOMPARE(sizeof(double),sizeof(qfloat16(1.f)-1));
    QCOMPARE(sizeof(double),sizeof(qfloat16(1.f)*1));
    QCOMPARE(sizeof(double),sizeof(qfloat16(1.f)/1));
}

QTEST_APPLESS_MAIN(tst_qfloat16)
#include "tst_qfloat16.moc"
