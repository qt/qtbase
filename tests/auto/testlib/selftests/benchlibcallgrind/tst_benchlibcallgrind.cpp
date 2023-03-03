// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QtCore/QCoreApplication>

#if __has_include(<valgrind/valgrind.h>)
#  include <valgrind/valgrind.h>
#  define HAVE_VALGRIND_H
#endif

class tst_BenchlibCallgrind: public QObject
{
    Q_OBJECT

private slots:
    void failInChildProcess();

    void twoHundredMillionInstructions();
};

void tst_BenchlibCallgrind::failInChildProcess()
{
#ifdef HAVE_VALGRIND_H
    [[maybe_unused]] static double f = 1.0;
    QBENCHMARK {
        for (int i = 0; i < 1000000; ++i) {
            f *= 1.1;
            if (RUNNING_ON_VALGRIND) QFAIL("Running under valgrind!");
        }
    }
#else
    QSKIP("Skipping test because I can't see <valgrind/valgrind.h> - is valgrind installed ?");
#endif
}

void tst_BenchlibCallgrind::twoHundredMillionInstructions()
{
#if defined(__GNUC__) && (defined(__i386) || defined(__x86_64))
    QBENCHMARK {
        __asm__ __volatile__(
            "mov $100000000,%%eax   \n"
            "1:               \n"
            "dec %%eax              \n"
            "jnz 1b            \n"
            : /* no output */
            : /* no input */
            : /* clobber */ "eax"
        );
    }
#else
    QSKIP("This test is only implemented for gcc on x86.");
#endif
}

QTEST_MAIN_WRAPPER(tst_BenchlibCallgrind,
    std::vector<const char*> args(argv, argv + argc);
    // Add the -callgrind argument unless (it's there anyway or) we're the
    // recursive invocation with -callgrindchild passed.
    if (std::find_if(args.begin(), args.end(),
                     [](const char *arg) {
                         return qstrcmp(arg, "-callgrindchild") == 0
                             || qstrcmp(arg, "-callgrind") == 0;
                     }) == args.end()) {
        args.push_back("-callgrind");
        argc = int(args.size());
        argv = const_cast<char**>(&args[0]);
    }
    QTEST_MAIN_SETUP())

#undef HAVE_VALGRIND_H
#include "tst_benchlibcallgrind.moc"
