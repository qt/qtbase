// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtCore/QCoreApplication>
#include <QTest>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#else
#include <sys/resource.h>
#endif

class tst_Crashes: public QObject
{
    Q_OBJECT

private slots:
    void crash();
};

void tst_Crashes::crash()
{
#if defined(Q_OS_WIN)
   //we avoid the error dialogbox to appear on windows
   SetErrorMode( SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
#elif defined(RLIMIT_CORE)
    // Unix: set our core dump limit to zero to request no dialogs.
    if (struct rlimit rlim; getrlimit(RLIMIT_CORE, &rlim) == 0) {
        rlim.rlim_cur = 0;
        setrlimit(RLIMIT_CORE, &rlim);
    }
#endif
    /*
        We deliberately dereference an invalid but non-zero address;
        it should be non-zero because a few platforms may have special crash behavior
        when dereferencing exactly 0 (e.g. some macs have been observed to generate SIGILL
        rather than SIGSEGV).
    */
    int *i = 0;

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Warray-bounds")
    i[1] = 1;
QT_WARNING_POP
}

QTEST_MAIN(tst_Crashes)

#include "tst_crashes.moc"
