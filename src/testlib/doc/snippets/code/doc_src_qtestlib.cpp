// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QTest>
#include "src_qtestlib_qtestcase.cpp"
//! [0]
class MyFirstTest: public QObject
{
    Q_OBJECT

private:
    bool myCondition()
    {
        return true;
    }

private slots:
    void initTestCase()
    {
        qDebug("Called before everything else.");
    }

    void myFirstTest()
    {
        QVERIFY(true); // check that a condition is satisfied
        QCOMPARE(1, 1); // compare two values
    }

    void mySecondTest()
    {
        QVERIFY(myCondition());
        QVERIFY(1 != 2);
    }

    void cleanupTestCase()
    {
        qDebug("Called after myFirstTest and mySecondTest.");
    }
};
//! [0]


//! [8]
void TestQString::toUpper()
{
    QString str = "Hello";
    QVERIFY(str.toUpper() == "HELLO");
}
//! [8]

void TestQString::Compare()
{
//! [11]
QCOMPARE(QString("hello").toUpper(), QString("HELLO"));
QCOMPARE(QString("Hello").toUpper(), QString("HELLO"));
QCOMPARE(QString("HellO").toUpper(), QString("HELLO"));
QCOMPARE(QString("HELLO").toUpper(), QString("HELLO"));
//! [11]
}

//! [12]
class MyFirstBenchmark: public QObject
{
    Q_OBJECT
private slots:
    void myFirstBenchmark()
    {
        QString string1;
        QString string2;
        QBENCHMARK {
            string1.localeAwareCompare(string2);
        }
    }
};
//! [12]
