// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QTest>

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

    void multiFail();
    void multiSkip();
private:
    void logNames(const char *caller);
    void table_data();
};

void tst_Subtest::logNames(const char *caller)
{
    auto orNull = [](const char *s) { return s ? s : "(null)"; };
    qDebug("%s %s %s", caller, orNull(QTest::currentTestFunction()),
           orNull(QTest::currentDataTag()));
}

void tst_Subtest::initTestCase()
{
    logNames("initTestCase");
}

void tst_Subtest::cleanupTestCase()
{
    logNames("cleanupTestCase");
}

void tst_Subtest::init()
{
    logNames("init");
}

void tst_Subtest::cleanup()
{
    logNames("cleanup");
}

void tst_Subtest::test1()
{
    logNames("test1");
}

void tst_Subtest::table_data()
{
    QTest::addColumn<QString>("str");

    QTest::newRow("data0") << QString("hello0");
    QTest::newRow("data1") << QString("hello1");
    QTest::newRow("data2") << QString("hello2");
}

void tst_Subtest::test2_data()
{
    logNames("test2_data");
    table_data();
    qDebug() << "test2_data end";
}

void tst_Subtest::test2()
{
    logNames("test2");

    static int count = 0;

    QFETCH(QString, str);
    QCOMPARE(str, QString("hello%1").arg(count++));

    qDebug() << "test2 end";
}

void tst_Subtest::test3_data()
{
    logNames("test3_data");
    table_data();
    qDebug() << "test3_data end";
}

void tst_Subtest::test3()
{
    logNames("test3");

    QFETCH(QString, str);

    // second and third time we call this it should FAIL
    QCOMPARE(str, QString("hello0"));

    qDebug() << "test3 end";
}

void tst_Subtest::multiFail()
{
    // Simulates tests which call a shared function that does common checks, or
    // that do checks in code run asynchronously from a message loop.
    for (int i = 0; i < 10; ++i)
        []() { QFAIL("This failure message should be repeated ten times"); }();
    QFAIL("But this test should only contribute one to the failure count");
}

void tst_Subtest::multiSkip()
{
    // Similar to multiFail()
    for (int i = 0; i < 10; ++i)
        []() { QSKIP("This skip should be repeated ten times"); }();
    QSKIP("But this test should only contribute one to the skip count");
}

QTEST_MAIN(tst_Subtest)

#include "tst_subtest.moc"
