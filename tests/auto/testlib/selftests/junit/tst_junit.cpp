// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

class tst_JUnit : public QObject
{
    Q_OBJECT

public:
    tst_JUnit();

private slots:
    void testFunc1();
    void testFunc2();
    void testFunc3();
    void testFunc4();
    void testFunc5();
    void testFunc6();
    void testFunc7();
};

tst_JUnit::tst_JUnit()
{
}

void tst_JUnit::testFunc1()
{
    qWarning("just a qWarning() !");
    QCOMPARE(1,1);
}

void tst_JUnit::testFunc2()
{
    qDebug("a qDebug() call with comment-ending stuff -->");
    QCOMPARE(2, 3);
}

void tst_JUnit::testFunc3()
{
    QSKIP("skipping this function!");
}

void tst_JUnit::testFunc4()
{
    QFAIL("a forced failure!");
}

/*
    Note there are two testfunctions which give expected failures.
    This is so we can test that expected failures don't add to failure
    counts and unexpected passes do.  If we had one xfail and one xpass
    testfunction, we couldn't test which one of them adds to the failure
    count.
*/

void tst_JUnit::testFunc5()
{
    QEXPECT_FAIL("", "this failure is expected", Abort);
    QVERIFY(false);
}

void tst_JUnit::testFunc6()
{
    QEXPECT_FAIL("", "this failure is also expected", Abort);
    QFAIL("This is a deliberate failure");
}

void tst_JUnit::testFunc7()
{
    QEXPECT_FAIL("", "this pass is unexpected", Abort);
    QVERIFY(true);
}


QTEST_APPLESS_MAIN(tst_JUnit)
#include "tst_junit.moc"
