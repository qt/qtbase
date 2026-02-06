// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QCoreApplication>
#include <QTest>
#include <private/qtestlog_p.h>

class tst_Selected : public QObject
{
    Q_OBJECT

private slots:
    void pass_data();
    void pass();
    void fail_data() { pass_data(); }
    void fail();
    void xfail_data() { pass_data(); }
    void xfail();
    void xpass_data() { pass_data(); }
    void xpass();
    void messages();
};

void tst_Selected::pass_data()
{
    QTest::addColumn<int>("mode");
    QTest::newRow("verify") << 0;
    QTest::newRow("compare") << 1;
    QTest::newRow("less") << 2;
    QTest::newRow("less-or-equal") << 3;
    QTest::newRow("greater") << 4;
    QTest::newRow("greater-or-equal") << 5;
    QTest::newRow("differ") << 6;
    QTest::newRow("same") << 7;
}

void tst_Selected::pass()
{
    QFETCH(int, mode);
    switch (mode) {
    case 0: QVERIFY2(true, "This test should PASS"); break;
    case 1: QCOMPARE(0, 0); break;
    case 2: QCOMPARE_LT(0, 1); break;
    case 3: QCOMPARE_LE(1, 1); break;
    case 4: QCOMPARE_GT(1, 0); break;
    case 5: QCOMPARE_GE(0, 0); break;
    case 6: QCOMPARE_NE(1, 0); break;
    case 7: QCOMPARE_EQ(0, 0); break;
    }
}

void tst_Selected::fail()
{
    QFETCH(int, mode);
    switch (mode) {
    case 0: QVERIFY2(false, "This test should FAIL"); break;
    case 1: QCOMPARE(0, 1); break;
    case 2: QCOMPARE_LT(1, 0); break;
    case 3: QCOMPARE_LE(1, 0); break;
    case 4: QCOMPARE_GT(0, 1); break;
    case 5: QCOMPARE_GE(0, 1); break;
    case 6: QCOMPARE_NE(0, 0); break;
    case 7: QCOMPARE_EQ(1, 0); break;
    }
}

void tst_Selected::xfail()
{
    QFETCH(int, mode);
    QEXPECT_FAIL("", "This test should XFAIL", Abort);
    switch (mode) {
    case 0: QVERIFY2(false, "This test should XFAIL"); break;
    case 1: QCOMPARE(0, 1); break;
    case 2: QCOMPARE_LT(1, 0); break;
    case 3: QCOMPARE_LE(1, 0); break;
    case 4: QCOMPARE_GT(0, 1); break;
    case 5: QCOMPARE_GE(0, 1); break;
    case 6: QCOMPARE_NE(0, 0); break;
    case 7: QCOMPARE_EQ(1, 0); break;
    }
}

void tst_Selected::xpass()
{
    QFETCH(int, mode);
    QEXPECT_FAIL("", "This test should XPASS", Abort);
    switch (mode) {
    case 0: QVERIFY2(true, "This test should XPASS"); break;
    case 1: QCOMPARE(0, 0); break;
    case 2: QCOMPARE_LT(0, 1); break;
    case 3: QCOMPARE_LE(1, 1); break;
    case 4: QCOMPARE_GT(1, 0); break;
    case 5: QCOMPARE_GE(0, 0); break;
    case 6: QCOMPARE_NE(1, 0); break;
    case 7: QCOMPARE_EQ(0, 0); break;
    }
}

// Deliberately not exercised:
void tst_Selected::messages()
{
    qWarning("This is a warning");
    QTestLog::warn("This is an internal testlib warning", __FILE__, __LINE__);
    qDebug("This is a debug message");
    qCritical("This is a critical message");
    qInfo("This is an info message");
    QTestLog::info("This is an internal testlib info message", __FILE__, __LINE__);
}

/* Point of test: verify that the handling of arguments requesting non-existent
   tests doesn't mess up the lists of functions and data tags that QtTest parses
   from the command-line.
*/
QTEST_MAIN_WRAPPER(tst_Selected,
    std::vector<const char*> args(argv, argv + argc);
    args.push_back("pass:verify");
    args.push_back("excell"); // No such function
    args.push_back("fail:less");
    args.push_back("fail:seriously"); // No such data tag
    args.push_back("xfail:differ");
    args.push_back("surprise:greater"); // No such function
    args.push_back("xpass:same");
    argc = int(args.size());
    argv = const_cast<char**>(&args[0]);
    QTEST_MAIN_SETUP())

#include "tst_selected.moc"
