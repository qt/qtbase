// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtCore/QCoreApplication>
#include <QTest>

#include <private/cycle_p.h>

class tst_BenchlibTickCounter: public QObject
{
    Q_OBJECT

private slots:
    void threeBillionTicks();
};

void tst_BenchlibTickCounter::threeBillionTicks()
{
#ifndef HAVE_TICK_COUNTER
    QSKIP("Tick counter not available on this platform");
#else
    QBENCHMARK {
        CycleCounterTicks start = getticks();
        double el = 0.;
        double max = el;
        while (el < 3000000000.) {
            /* Verify that elapsed time never decreases */
            QVERIFY2(el >= max, qPrintable(
                QString("Tick counter is not monotonic\nElapsed moved from %1 to %2")
                    .arg(max).arg(el)
            ));
            max = el;
            el = elapsed(getticks(), start);
        }
    }
#endif
}

QTEST_MAIN_WRAPPER(tst_BenchlibTickCounter,
    std::vector<const char*> args(argv, argv + argc);
    args.push_back("-tickcounter");
    argc = int(args.size());
    argv = const_cast<char**>(&args[0]);
    QTEST_MAIN_SETUP())

#include "tst_benchlibtickcounter.moc"
