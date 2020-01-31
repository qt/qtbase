/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include <QtTest/QtTest>
#include <private/qtestlog_p.h>

class tst_Blacklisted : public QObject
{
    Q_OBJECT

private slots:
    void pass();
    void skip();
    void fail();
    void xfail();
    void xpass();

    // This test function must be last, as it calls qFatal().
    void messages();
};

// All the tests below except pass() have been blacklisted in blacklisted/BLACKLIST
// Contrast with ../silent/, for the same tests without blacklisting but with -silent

void tst_Blacklisted::pass()
{
    QVERIFY(true);
}

void tst_Blacklisted::skip()
{
    QSKIP("This test should SKIP");
}

void tst_Blacklisted::fail()
{
    QVERIFY2(false, "This test should BFAIL");
}

void tst_Blacklisted::xfail()
{
    QEXPECT_FAIL("", "This test should BXFAIL then BPASS", Abort);
    QVERIFY(false);
}

void tst_Blacklisted::xpass()
{
    QEXPECT_FAIL("", "This test should BXPASS", Abort);
    QVERIFY2(true, "This test should BXPASS");
}

#ifndef Q_OS_WIN
#include <signal.h>
#include <setjmp.h>

static jmp_buf state;
static void abort_handler(int)
{
    longjmp(state, 1);
}
#endif

void tst_Blacklisted::messages()
{
    qWarning("This is a warning that should not appear in silent test output");
    QWARN("This is an internal testlib warning that should not appear in silent test output");
    qDebug("This is a debug message that should not appear in silent test output");
    qCritical("This is a critical message that should not appear in silent test output");
    qInfo("This is an info message that should not appear in silent test output");
    QTestLog::info("This is an internal testlib info message that should not appear in silent test output", __FILE__, __LINE__);

#ifndef Q_OS_WIN
    // We're testing qFatal, but we don't want to actually std::abort() !
    auto prior = signal(SIGABRT, abort_handler);
    if (setjmp(state))
        signal(SIGABRT, prior);
    else
#endif
        qFatal("This is a fatal error message that should still appear in silent test output");
}

QTEST_MAIN(tst_Blacklisted)
#include "tst_blacklisted.moc"
