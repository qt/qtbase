// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtCore/QCoreApplication>
#include <QTest>

Q_DECLARE_METATYPE(QTest::TestFailMode)

class tst_ExpectFail: public QObject
{
    Q_OBJECT

private slots:
    void cleanupTestCase() const;
    void init() const;
    void xfailAndContinue() const;
    void xfailAndAbort() const;
    void xfailContinueSkip() const;
    void xfailAbortSkip() const;
    void xfailTwice() const;
    void xfailDataDrivenTwice() const;
    void xfailDataDrivenTwice_data() const { xfailDataDriven_data(false); }
    void xfailWithQString() const;
    void xfailDataDrivenWithQString_data() const { xfailDataDriven_data(false); }
    void xfailDataDrivenWithQString() const;
    void xfailDataDrivenWithQVerify_data() const { xfailDataDriven_data(false); }
    void xfailDataDrivenWithQVerify() const;
    void xfailDataDrivenWithQCompare_data() const { xfailDataDriven_data(false); }
    void xfailDataDrivenWithQCompare() const;
    void xfailOnWrongRow_data() const { xfailDataDriven_data(true); }
    void xfailOnWrongRow() const;
    void xfailOnAnyRow_data() const { xfailDataDriven_data(true); }
    void xfailOnAnyRow() const;
    void xfailWithoutCheck_data() const { xfailDataDriven_data(true); }
    void xfailWithoutCheck() const;
    void xpassAbort() const;
    void xpassAbortSkip() const;
    void xpassAbortXfailContinue() const;
    void xpassContinue() const;
    void xpassContinueSkip() const;
    void xpassContinueXfailAbort() const;
    void xpassAbortDataDrivenWithQVerify_data() const { xpassDataDriven_data(); }
    void xpassAbortDataDrivenWithQVerify() const;
    void xpassContinueDataDrivenWithQVerify_data() const { xpassDataDriven_data(); }
    void xpassContinueDataDrivenWithQVerify() const;
    void xpassAbortDataDrivenWithQCompare_data() const { xpassDataDriven_data(); }
    void xpassAbortDataDrivenWithQCompare() const;
    void xpassContinueDataDrivenWithQCompare_data() const { xpassDataDriven_data(); }
    void xpassContinueDataDrivenWithQCompare() const;

private:
    void xfailDataDriven_data(bool failOnly) const;
    void xpassDataDriven_data() const;
};
static int casesTested = 2;
// What the totals line's numbers *should* be:
static int passed = 2, failed = 0, skipped = 0;
// Total and passed get {init,cleanup}TestCase() in addition to the actual tests.

void tst_ExpectFail::init() const
{
    ++casesTested;
}

void tst_ExpectFail::cleanupTestCase() const
{
    qDebug("Totals should add up to %d: %d passed, %d failed, %d skipped",
           casesTested, passed, failed, skipped);
}

void tst_ExpectFail::xfailAndContinue() const
{
    ++passed;
    qDebug("begin");
    QEXPECT_FAIL("", "This should xfail", Continue);
    QVERIFY(false);
    qDebug("after");
}

void tst_ExpectFail::xfailAndAbort() const
{
    ++passed;
    qDebug("begin");
    QEXPECT_FAIL("", "This should xfail", Abort);
    QVERIFY(false);

    // If we get here the test did not correctly abort on the previous QVERIFY.
    QVERIFY2(false, "This should not be reached");
}

void tst_ExpectFail::xfailContinueSkip() const
{
    ++skipped;
    QEXPECT_FAIL("", "This should xfail then skip", Continue);
    QVERIFY(false);
    QSKIP("This skip should be reported and counted");
}

void tst_ExpectFail::xfailAbortSkip() const
{
    ++passed;
    QEXPECT_FAIL("", "This should xfail", Abort);
    QVERIFY(false);

    // If we get here the test did not correctly abort on the previous QVERIFY.
    QSKIP("This skip should not be reached");
}

void tst_ExpectFail::xfailTwice() const
{
    ++failed;
    QEXPECT_FAIL("", "Calling QEXPECT_FAIL once is fine", Continue);
    QEXPECT_FAIL("", "Calling QEXPECT_FAIL when already expecting a failure is "
                     "an error and should abort this test function", Continue);

    // If we get here the test did not correctly abort on the double call to QEXPECT_FAIL.
    QVERIFY2(false, "This should not be reached");
}

void tst_ExpectFail::xfailDataDrivenTwice() const
{
    ++failed;
    // Same with data-driven cases (twist semantics of unused shouldPass; we
    // have four combinations to test):
    QEXPECT_FAIL("Pass Abort", "Calling QEXPECT_FAIL once on a test-case is fine", Abort);
    QEXPECT_FAIL("Pass Abort", "Calling QEXPECT_FAIL when already expecting a failure is "
                               "an error and should abort this test function", Continue);
    QEXPECT_FAIL("Fail Abort", "Calling QEXPECT_FAIL once on a test-case is fine", Abort);
    QEXPECT_FAIL("Fail Abort", "Calling QEXPECT_FAIL when already expecting a failure is "
                               "an error and should abort this test function", Abort);
    QEXPECT_FAIL("Pass Continue", "Calling QEXPECT_FAIL once on a test-case is fine", Continue);
    QEXPECT_FAIL("Pass Continue", "Calling QEXPECT_FAIL when already expecting a failure is "
                                  "an error and should abort this test function", Abort);
    QEXPECT_FAIL("Fail Continue", "Calling QEXPECT_FAIL once on a test-case is fine", Continue);
    QEXPECT_FAIL("Fail Continue", "Calling QEXPECT_FAIL when already expecting a failure is "
                                  "an error and should abort this test function", Continue);

    // If we get here the test did not correctly abort on the double call to QEXPECT_FAIL.
    QVERIFY2(false, "This should not be reached");
}

void tst_ExpectFail::xfailWithQString() const
{
    ++passed;
    QEXPECT_FAIL("", QString("A string").toLatin1().constData(), Continue);
    QVERIFY(false);

    int bugNo = 5;
    QLatin1String msg("The message");
    QEXPECT_FAIL("", QString("Bug %1 (%2)").arg(bugNo).arg(msg).toLatin1().constData(), Continue);
    QVERIFY(false);
}

void tst_ExpectFail::xfailDataDrivenWithQString() const
{
    // This test does not (yet) distinguish the two Pass cases.
    QFETCH(bool, shouldPass);
    QFETCH(QTest::TestFailMode, failMode);
    if (shouldPass || failMode == QTest::Continue)
        ++skipped;
    else
        ++passed;

    QEXPECT_FAIL("Fail Abort", QString("A string").toLatin1().constData(), Abort);
    QEXPECT_FAIL("Fail Continue", QString("A string").toLatin1().constData(), Continue);
    // TODO: why aren't QVERIFY2()'s smessages seen ?
    QVERIFY2(shouldPass, "Both Fail cases should XFAIL here");
    // Fail Abort should be gone now.

    int bugNo = 5;
    QLatin1String msg("The message");
    QEXPECT_FAIL("Fail Continue", qPrintable(QString("Bug %1 (%2)").arg(bugNo).arg(msg)), Continue);
    QVERIFY2(shouldPass, "Only Fail Continue should see this");

    // FAIL is a pass, and SKIP trumps pass.
    QSKIP("Each Continue or Pass reports this and increments skip-count");
}

void tst_ExpectFail::xfailDataDrivenWithQVerify() const
{
    // This test does not (yet) distinguish the two Pass cases.
    ++passed;
    QFETCH(bool, shouldPass);
    QFETCH(QTest::TestFailMode, failMode);

    QEXPECT_FAIL("Fail Abort", "This test should xfail", Abort);
    QEXPECT_FAIL("Fail Continue", "This test should xfail", Continue);
    QVERIFY2(shouldPass, "Both Fail cases should XFAIL here");

    // If we get here, either we expected to pass or we expected to
    // fail and the failure mode was Continue.
    if (!shouldPass)
        QCOMPARE(failMode, QTest::Continue);
}

void tst_ExpectFail::xfailDataDriven_data(bool failOnly) const
{
    QTest::addColumn<bool>("shouldPass");
    QTest::addColumn<QTest::TestFailMode>("failMode");

    if (!failOnly) {
        QTest::newRow("Pass Abort")    << true  << QTest::Abort;
        QTest::newRow("Pass Continue") << true  << QTest::Continue;
    }
    QTest::newRow("Fail Abort")    << false << QTest::Abort;
    QTest::newRow("Fail Continue") << false << QTest::Continue;
}

void tst_ExpectFail::xfailDataDrivenWithQCompare() const
{
    // This test does not (yet) distinguish the two Pass cases.
    ++passed;
    QFETCH(bool, shouldPass);
    QFETCH(QTest::TestFailMode, failMode);

    QEXPECT_FAIL("Fail Abort", "This test should xfail", Abort);
    QEXPECT_FAIL("Fail Continue", "This test should xfail", Continue);

    QCOMPARE(1, shouldPass ? 1 : 2);

    // If we get here, either we expected to pass or we expected to
    // fail and the failure mode was Continue.
    if (!shouldPass)
        QCOMPARE(failMode, QTest::Continue);
}

void tst_ExpectFail::xfailOnWrongRow() const
{
    ++passed;
    qDebug("Should pass (*not* xpass), despite test-case name");
    // QEXPECT_FAIL for a row that does not exist should be ignored.
    // (It might be conditional data(), so exist in other circumstances.)
    QFETCH(QTest::TestFailMode, failMode);
    // You can't pass a variable as the last parameter of QEXPECT_FAIL,
    // because the macro adds "QTest::" in front of the last parameter.
    // That is why the following code appears to be a little strange.
    if (failMode == QTest::Abort)
        QEXPECT_FAIL("wrong row", "This xfail should be ignored", Abort);
    else
        QEXPECT_FAIL("wrong row", "This xfail should be ignored", Continue);
    QTEST(false, "shouldPass"); // _data skips the passing tests as pointless
}

void tst_ExpectFail::xfailOnAnyRow() const
{
    ++passed;
    // In a data-driven test, passing an empty first parameter to QEXPECT_FAIL
    // should mean that the failure is expected for all data rows.
    QFETCH(QTest::TestFailMode, failMode);
    // You can't pass a variable as the last parameter of QEXPECT_FAIL,
    // because the macro adds "QTest::" in front of the last parameter.
    // That is why the following code appears to be a little strange.
    if (failMode == QTest::Abort)
        QEXPECT_FAIL("", "This test should xfail", Abort);
    else
        QEXPECT_FAIL("", "This test should xfail", Continue);
    QTEST(true, "shouldPass"); // _data skips the passing tests as pointless
}

void tst_ExpectFail::xfailWithoutCheck() const
{
    ++failed;
    qDebug("Should fail (*not* xfail), despite test-case name");
    QTEST(false, "shouldPass"); // _data skips the passing tests as pass/fail is irrelevant
    QEXPECT_FAIL("Fail Abort", "Calling QEXPECT_FAIL without any subsequent check is an error",
                 Abort);
    QEXPECT_FAIL("Fail Continue", "Calling QEXPECT_FAIL without any subsequent check is an error",
                 Continue);
}

void tst_ExpectFail::xpassAbort() const
{
    ++failed;
    QEXPECT_FAIL("", "This test should xpass", Abort);
    QVERIFY(true);

    // If we get here the test did not correctly abort on the previous
    // unexpected pass.
    QVERIFY2(false, "This should not be reached");
}

void tst_ExpectFail::xpassAbortSkip() const
{
    ++failed;
    QEXPECT_FAIL("", "This test should xpass", Abort);
    QVERIFY(true);

    QSKIP("This should not be reached (and not add to skip-count)");
}

void tst_ExpectFail::xpassAbortXfailContinue() const
{
    ++failed;
    QEXPECT_FAIL("", "This test should xpass", Abort);
    QVERIFY(true);

    // If we get here the test did not correctly abort on the previous
    // unexpected pass.
    QEXPECT_FAIL("", "This should not be reached", Continue);
    QVERIFY2(false, "This should not be reached");
}

void tst_ExpectFail::xpassContinue() const
{
    ++failed;
    QEXPECT_FAIL("", "This test should xpass", Continue);
    QVERIFY(true);
    qDebug("This should be reached");
}

void tst_ExpectFail::xpassDataDriven_data() const
{
    QTest::addColumn<bool>("shouldXPass");

    QTest::newRow("XPass")  << true;
    QTest::newRow("Pass")   << false;
}

void tst_ExpectFail::xpassContinueSkip() const
{
    ++failed; // and *not* ++skipped
    QEXPECT_FAIL("", "This test should xpass", Continue);
    QVERIFY(true);
    QSKIP("This should be reached but not increment skip-count");
}

void tst_ExpectFail::xpassContinueXfailAbort() const
{
    ++failed;
    QEXPECT_FAIL("", "This test should xpass", Continue);
    QVERIFY(true);
    QEXPECT_FAIL("", "This test should xfail but not add to totals", Abort);
    QVERIFY(false);
    QVERIFY2(false, "This should not be reached");
}

void tst_ExpectFail::xpassAbortDataDrivenWithQVerify() const
{
    QFETCH(bool, shouldXPass);
    if (shouldXPass)
        ++failed;
    else
        ++passed;

    QEXPECT_FAIL("XPass", "This test-row should xpass", Abort);
    QVERIFY(true);

    // We should only get here if the test wasn't supposed to xpass.
    QVERIFY2(!shouldXPass, "Test failed to Abort on XPASS");
}

void tst_ExpectFail::xpassContinueDataDrivenWithQVerify() const
{
    QFETCH(bool, shouldXPass);
    if (shouldXPass)
        ++failed;
    else
        ++passed;

    QEXPECT_FAIL("XPass", "This test-row should xpass", Continue);
    QVERIFY(true);

    qDebug(shouldXPass ? "Test should Continue past XPASS" : "Test should simply PASS");
}

void tst_ExpectFail::xpassAbortDataDrivenWithQCompare() const
{
    QFETCH(bool, shouldXPass);
    if (shouldXPass)
        ++failed;
    else
        ++passed;

    QEXPECT_FAIL("XPass", "This test should xpass", Abort);
    QCOMPARE(1, 1);

    // We should only get here if the test wasn't supposed to xpass.
    QVERIFY2(!shouldXPass, "Test failed to Abort on XPASS");
}

void tst_ExpectFail::xpassContinueDataDrivenWithQCompare() const
{
    QFETCH(bool, shouldXPass);
    if (shouldXPass)
        ++failed;
    else
        ++passed;

    QEXPECT_FAIL("XPass", "This test should xpass", Continue);
    QCOMPARE(1, 1);

    qDebug(shouldXPass ? "Test should Continue past XPASS" : "Test should simply PASS");
}

QTEST_MAIN(tst_ExpectFail)
#include "tst_expectfail.moc"
