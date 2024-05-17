// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QCoreApplication>
#include <QTest>

class tst_Counting : public QObject
{
    Q_OBJECT

private slots:
    // The following test functions exercise each possible combination of test
    // results for two data rows.
    void testPassPass_data();
    void testPassPass();

    void testPassSkip_data();
    void testPassSkip();

    void testPassFail_data();
    void testPassFail();

    void testSkipPass_data();
    void testSkipPass();

    void testSkipSkip_data();
    void testSkipSkip();

    void testSkipFail_data();
    void testSkipFail();

    void testFailPass_data();
    void testFailPass();

    void testFailSkip_data();
    void testFailSkip();

    void testFailFail_data();
    void testFailFail();

    // The following test functions test skips and fails in the special
    // init() and cleanup() slots.
    void init();
    void cleanup();
    void testFailInInit_data();
    void testFailInInit();
    void testFailInCleanup_data();
    void testFailInCleanup();
    void testSkipInInit_data();
    void testSkipInInit();
    void testSkipInCleanup_data();
    void testSkipInCleanup();

private:
    void helper();
};

enum TestResult
{
    Pass,
    Fail,
    Skip
};

Q_DECLARE_METATYPE(TestResult);

void tst_Counting::helper()
{
    QFETCH(TestResult, result);

    switch (result) {
        case Pass:
            QVERIFY(true);
            QCOMPARE(2 + 1, 3);
            break;
        case Fail:
            QVERIFY(false);
            break;
        case Skip:
            QSKIP("Skipping");
            break;
    }
}

void tst_Counting::testPassPass_data()
{
    QTest::addColumn<TestResult>("result");
    QTest::newRow("row 1") << Pass;
    QTest::newRow("row 2") << Pass;
}

void tst_Counting::testPassPass()
{
    helper();
}

void tst_Counting::testPassSkip_data()
{
    QTest::addColumn<TestResult>("result");
    QTest::newRow("row 1") << Pass;
    QTest::newRow("row 2") << Skip;
}

void tst_Counting::testPassSkip()
{
    helper();
}

void tst_Counting::testPassFail_data()
{
    QTest::addColumn<TestResult>("result");
    QTest::newRow("row 1") << Pass;
    QTest::newRow("row 2") << Fail;
}

void tst_Counting::testPassFail()
{
    helper();
}

void tst_Counting::testSkipPass_data()
{
    QTest::addColumn<TestResult>("result");
    QTest::newRow("row 1") << Skip;
    QTest::newRow("row 2") << Pass;
}

void tst_Counting::testSkipPass()
{
    helper();
}

void tst_Counting::testSkipSkip_data()
{
    QTest::addColumn<TestResult>("result");
    QTest::newRow("row 1") << Skip;
    QTest::newRow("row 2") << Skip;
}

void tst_Counting::testSkipSkip()
{
    helper();
}

void tst_Counting::testSkipFail_data()
{
    QTest::addColumn<TestResult>("result");
    QTest::newRow("row 1") << Skip;
    QTest::newRow("row 2") << Fail;
}

void tst_Counting::testSkipFail()
{
    helper();
}

void tst_Counting::testFailPass_data()
{
    QTest::addColumn<TestResult>("result");
    QTest::newRow("row 1") << Fail;
    QTest::newRow("row 2") << Pass;
}

void tst_Counting::testFailPass()
{
    helper();
}

void tst_Counting::testFailSkip_data()
{
    QTest::addColumn<TestResult>("result");
    QTest::newRow("row 1") << Fail;
    QTest::newRow("row 2") << Skip;
}

void tst_Counting::testFailSkip()
{
    helper();
}

void tst_Counting::testFailFail_data()
{
    QTest::addColumn<TestResult>("result");
    QTest::newRow("row 1") << Fail;
    QTest::newRow("row 2") << Fail;
}

void tst_Counting::testFailFail()
{
    helper();
}

void tst_Counting::init()
{
    if (strcmp(QTest::currentTestFunction(), "testFailInInit") == 0
        && strcmp(QTest::currentDataTag(), "fail") == 0) {
        QFAIL("Fail in init()");
    } else if (strcmp(QTest::currentTestFunction(), "testSkipInInit") == 0
               && strcmp(QTest::currentDataTag(), "skip") == 0) {
        QSKIP("Skip in init()");
    }
}

void tst_Counting::cleanup()
{
    if (strcmp(QTest::currentTestFunction(), "testFailInCleanup") == 0 && strcmp(QTest::currentDataTag(), "fail") == 0)
        QFAIL("Fail in cleanup()");
    else if (strcmp(QTest::currentTestFunction(), "testSkipInCleanup") == 0 && strcmp(QTest::currentDataTag(), "skip") == 0)
        QSKIP("Skip in cleanup()");
}

void tst_Counting::testFailInInit_data()
{
    QTest::addColumn<bool>("dummy");
    QTest::newRow("before") << true;
    QTest::newRow("fail") << true;
    QTest::newRow("after") << true;
}

void tst_Counting::testFailInInit()
{
    if (strcmp(QTest::currentDataTag(), "fail") == 0)
        QFAIL("This test function should have been skipped due to QFAIL in init()");
}

void tst_Counting::testFailInCleanup_data()
{
    QTest::addColumn<bool>("dummy");
    QTest::newRow("before") << true;
    QTest::newRow("fail") << true;
    QTest::newRow("after") << true;
}

void tst_Counting::testFailInCleanup()
{
    if (strcmp(QTest::currentDataTag(), "fail") == 0)
        qDebug() << "This test function should execute and then QFAIL in cleanup()";
}

void tst_Counting::testSkipInInit_data()
{
    QTest::addColumn<bool>("dummy");
    QTest::newRow("before") << true;
    QTest::newRow("skip") << true;
    QTest::newRow("after") << true;
}

void tst_Counting::testSkipInInit()
{
    if (strcmp(QTest::currentDataTag(), "skip") == 0)
        QFAIL("This test function should have been skipped due to QSKIP in init()");
}

void tst_Counting::testSkipInCleanup_data()
{
    QTest::addColumn<bool>("dummy");
    QTest::newRow("before") << true;
    QTest::newRow("skip") << true;
    QTest::newRow("after") << true;
}

void tst_Counting::testSkipInCleanup()
{
    if (strcmp(QTest::currentDataTag(), "skip") == 0)
        qDebug() << "This test function should execute and then QSKIP in cleanup()";
}

#ifdef TESTLIB_VERBOSITY_ARG
#define SETUP() \
    std::vector<const char*> args(argv, argv + argc); \
    args.push_back(QT_STRINGIFY(TESTLIB_VERBOSITY_ARG)); \
    argc = int(args.size()); \
    argv = const_cast<char**>(&args[0]);
#else
#define SETUP()
#endif

QTEST_MAIN_WRAPPER(tst_Counting,
    SETUP()
    QTEST_MAIN_SETUP())

#include "tst_counting.moc"
