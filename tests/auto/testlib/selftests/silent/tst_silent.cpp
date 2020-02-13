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

class tst_Silent : public QObject
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

void tst_Silent::pass()
{
    QVERIFY(true);
}

void tst_Silent::skip()
{
    QSKIP("This test should skip");
}

void tst_Silent::fail()
{
    QVERIFY2(false, "This test should fail");
}

void tst_Silent::xfail()
{
    QEXPECT_FAIL("", "This test should XFAIL", Abort);
    QVERIFY(false);
}

void tst_Silent::xpass()
{
    QEXPECT_FAIL("", "This test should XPASS", Abort);
    QVERIFY2(true, "This test should XPASS");
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

void tst_Silent::messages()
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

int main(int argc, char *argv[])
{
    std::vector<const char*> args(argv, argv + argc);
    args.push_back("-silent");
    argc = args.size();
    argv = const_cast<char**>(&args[0]);

    QTEST_MAIN_IMPL(tst_Silent)
}

#include "tst_silent.moc"
