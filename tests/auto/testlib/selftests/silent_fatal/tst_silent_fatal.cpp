// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QCoreApplication>
#include <QTest>
#include <private/qtestlog_p.h>

#if defined(Q_OS_WIN) && defined(Q_CC_MSVC)
#  include <crtdbg.h>
#endif
#ifdef Q_OS_UNIX
#  include <sys/resource.h>
#  include <unistd.h>
#endif

void disableCoreDumps()
{
#if defined(Q_OS_WIN) && defined(Q_CC_MSVC)
    // Windows: Suppress crash notification dialog.
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#elif defined(RLIMIT_CORE)
    // Unix: set our core dump limit to zero to request no dialogs.
    if (struct rlimit rlim; getrlimit(RLIMIT_CORE, &rlim) == 0) {
        rlim.rlim_cur = 0;
        setrlimit(RLIMIT_CORE, &rlim);
    }
#endif
}
Q_CONSTRUCTOR_FUNCTION(disableCoreDumps)

class tst_SilentFatal : public QObject
{
    Q_OBJECT

private slots:
    void fatalmessages();
};
void tst_SilentFatal::fatalmessages()
{
    qFatal("This is a fatal error message that should still appear in silent test output");
}

QTEST_MAIN_WRAPPER(tst_SilentFatal,
    std::vector<const char*> args(argv, argv + argc);
    args.push_back("-silent");
    args.push_back("-nocrashhandler");
    argc = int(args.size());
    argv = const_cast<char**>(&args[0]);
    QTEST_MAIN_SETUP())

#include "tst_silent_fatal.moc"
