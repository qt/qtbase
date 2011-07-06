/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore>
#include <QtTest/QtTest>

class tst_Subtest: public QObject
{
    Q_OBJECT
public slots:
    void init();
    void initTestCase();

    void cleanup();
    void cleanupTestCase();

private slots:
    void test1();
    void test2_data();
    void test2();
    void test3_data();
    void test3();
    void floatComparisons() const;
    void floatComparisons_data() const;
    void compareFloatTests() const;
    void compareFloatTests_data() const;
};


void tst_Subtest::initTestCase()
{
    printf("initTestCase %s %s\n",
            QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)",
            QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");
}

void tst_Subtest::cleanupTestCase()
{
    printf("cleanupTestCase %s %s\n",
            QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)",
            QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");
}

void tst_Subtest::init()
{
    printf("init %s %s\n",
            QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)",
            QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");
}

void tst_Subtest::cleanup()
{
    printf("cleanup %s %s\n",
            QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)",
            QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");
}

void tst_Subtest::test1()
{
    printf("test1 %s %s\n",
            QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)",
            QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");
}

void tst_Subtest::test2_data()
{
    printf("test2_data %s %s\n",
            QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)",
            QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");

    QTest::addColumn<QString>("str");

    QTest::newRow("data0") << QString("hello0");
    QTest::newRow("data1") << QString("hello1");
    QTest::newRow("data2") << QString("hello2");

    printf("test2_data end\n");
}

void tst_Subtest::test2()
{
    printf("test2 %s %s\n",
            QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)",
            QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");

    static int count = 0;

    QFETCH(QString, str);
    QCOMPARE(str, QString("hello%1").arg(count++));

    printf("test2 end\n");
}

void tst_Subtest::test3_data()
{
    printf("test3_data %s %s\n",
            QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)",
            QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");

    QTest::addColumn<QString>("str");

    QTest::newRow("data0") << QString("hello0");
    QTest::newRow("data1") << QString("hello1");
    QTest::newRow("data2") << QString("hello2");

    printf("test3_data end\n");
}

void tst_Subtest::test3()
{
    printf("test2 %s %s\n",
            QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)",
            QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");

    QFETCH(QString, str);

    // second and third time we call this it should FAIL
    QCOMPARE(str, QString("hello0"));

    printf("test2 end\n");
}

void tst_Subtest::floatComparisons() const
{
    QFETCH(float, operandLeft);
    QFETCH(float, operandRight);

    QCOMPARE(operandLeft, operandRight);
}

void tst_Subtest::floatComparisons_data() const
{
    QTest::addColumn<float>("operandLeft");
    QTest::addColumn<float>("operandRight");

    QTest::newRow("should SUCCEED")
        << float(0)
        << float(0);

    QTest::newRow("should FAIL")
        << float(1.00000)
        << float(3.00000);

    QTest::newRow("should FAIL")
        << float(1.00000e-7f)
        << float(3.00000e-7f);

    QTest::newRow("should FAIL")
        << float(100001)
        << float(100002);
}

void tst_Subtest::compareFloatTests() const
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

void tst_Subtest::compareFloatTests_data() const
{
    QTest::addColumn<float>("t1");
    QTest::newRow("1e0") << 1e0f;
    QTest::newRow("1e-7") << 1e-7f;
    QTest::newRow("1e+7") << 1e+7f;
}

QTEST_MAIN(tst_Subtest)

#include "tst_subtest.moc"
