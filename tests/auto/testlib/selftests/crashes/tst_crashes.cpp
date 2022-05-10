// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtCore/QCoreApplication>
#include <QTest>

#ifdef Q_OS_WIN
#include <qt_windows.h>
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
#endif
    /*
        We deliberately dereference an invalid but non-zero address;
        it should be non-zero because a few platforms may have special crash behavior
        when dereferencing exactly 0 (e.g. some macs have been observed to generate SIGILL
        rather than SIGSEGV).
    */
    int *i = 0;
    i[1] = 1;
}

QTEST_MAIN(tst_Crashes)

#include "tst_crashes.moc"
