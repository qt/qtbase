// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#undef QTEST_THROW_ON_FAIL    // fails ### investigate
#undef QTEST_THROW_ON_SKIP    // fails ### investigate

#include <QtCore/QCoreApplication>
#include <QTest>
#include <private/qtestlog_p.h>

class tst_Blacklisted : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanupTestCase();
    void pass();
    void skip();
    void fail();
    void xfail();
    void multiSkip();
    void multiFail();
    void xfailContinueSkip();
    void xfailContinueFail();
    void xpass();
    void xpassContinueSkip();
    void xpassContinueFail();

private:
    int casesTested = 2;
    // What the totals line's numbers *should* be:
    int passed = 2, skipped = 0, blacklisted = 0;
    // Total and passed get {init,cleanup}TestCase() in addition to the actual tests.
};

void tst_Blacklisted::init()
{
    ++casesTested;
}

void tst_Blacklisted::cleanupTestCase()
{
    qDebug("Totals should add up to %d: %d passed, 0 failed, %d skipped, %d blacklisted",
           casesTested, passed, skipped, blacklisted);
}

// All the tests below have been blacklisted in blacklisted/BLACKLIST

void tst_Blacklisted::pass()
{
    ++blacklisted;
    qDebug("This test should BPASS");
    QVERIFY(true);
}

void tst_Blacklisted::skip()
{
    ++skipped;
    QSKIP("This test should SKIP");
}

void tst_Blacklisted::fail()
{
    ++blacklisted;
    QVERIFY2(false, "This test should BFAIL");
}

void tst_Blacklisted::multiFail() // cf. ../subtest/'s similar tests
{
    const QTest::ThrowOnFailDisabler nothrow; // tests repeated QFAILs
    ++blacklisted;
    for (int i = 0; i < 10; ++i)
        []() { QFAIL("This failure message should be repeated ten times"); }();
    QFAIL("But this test should only contribute one to the blacklisted count");
}

void tst_Blacklisted::multiSkip()
{
    const QTest::ThrowOnSkipDisabler nothrow; // tests repeated QSKIPs
    // Similar to multiFail()
    ++skipped;
    for (int i = 0; i < 10; ++i)
        []() { QSKIP("This skip should be repeated ten times"); }();
    QSKIP("But this test should only contribute one to the skip count");
}

void tst_Blacklisted::xfail()
{
    ++blacklisted;
    QEXPECT_FAIL("", "This test should BXFAIL then BPASS", Abort);
    QVERIFY(false);
}

void tst_Blacklisted::xfailContinueSkip()
{
    ++skipped;
    QEXPECT_FAIL("", "This test should BXFAIL then SKIP", Continue);
    QVERIFY(false);
    QSKIP("This skip should be seen and counted");
}

void tst_Blacklisted::xfailContinueFail()
{
    ++blacklisted;
    QEXPECT_FAIL("", "This test should BXFAIL then BFAIL", Continue);
    QVERIFY(false);
    QFAIL("This fail should be seen and counted as blacklisted");
}

void tst_Blacklisted::xpass()
{
    ++blacklisted;
    QEXPECT_FAIL("", "This test should BXPASS", Abort);
    QVERIFY2(true, "This test should BXPASS");
}

void tst_Blacklisted::xpassContinueSkip()
{
    ++blacklisted;
    QEXPECT_FAIL("", "This test should BXPASS then SKIP", Continue);
    QVERIFY2(true, "This test should BXPASS then SKIP");
    QSKIP("This skip should be seen but not counted");
}

void tst_Blacklisted::xpassContinueFail()
{
    ++blacklisted;
    QEXPECT_FAIL("", "This test should BXPASS then BFAIL", Continue);
    QVERIFY2(true, "This test should BXPASS then BFAIL");
    QFAIL("This fail should be seen and not counted (due to prior XPASS)");
}

QTEST_MAIN(tst_Blacklisted)
#include "tst_blacklisted.moc"
