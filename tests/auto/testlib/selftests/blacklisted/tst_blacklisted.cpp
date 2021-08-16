/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    // FIXME QTBUG-95661: skip gets counted
    QSKIP("This skip should be seen but not counted");
}

void tst_Blacklisted::xpassContinueFail()
{
    ++blacklisted;
    QEXPECT_FAIL("", "This test should BXPASS then BFAIL", Continue);
    QVERIFY2(true, "This test should BXPASS then BFAIL");
    // FIXME QTBUG-95661: gets double-counted
    QFAIL("This fail should be seen and not counted (due to prior XPASS)");
}

QTEST_MAIN(tst_Blacklisted)
#include "tst_blacklisted.moc"
