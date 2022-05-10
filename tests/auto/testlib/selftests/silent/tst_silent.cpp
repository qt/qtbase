// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QCoreApplication>
#include <QTest>
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
    QTestLog::warn("This is an internal testlib warning that should not appear in silent test output", __FILE__, __LINE__);
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

QTEST_MAIN_WRAPPER(tst_Silent,
    std::vector<const char*> args(argv, argv + argc);
    args.push_back("-silent");
    argc = int(args.size());
    argv = const_cast<char**>(&args[0]);
    QTEST_MAIN_SETUP())

#include "tst_silent.moc"
