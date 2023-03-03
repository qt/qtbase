// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QCoreApplication>
#include <QTest>

class tst_BenchlibCounting : public QObject
{
    Q_OBJECT

private slots:
    void passingBenchmark();
    void skippingBenchmark();
    void failingBenchmark();
};

void tst_BenchlibCounting::passingBenchmark()
{
    QBENCHMARK {
    }
}

void tst_BenchlibCounting::skippingBenchmark()
{
    QBENCHMARK {
        QSKIP("This is a skipping benchmark");
    }
}

void tst_BenchlibCounting::failingBenchmark()
{
    QBENCHMARK {
        QFAIL("This is a failing benchmark");
    };
}

QTEST_MAIN_WRAPPER(tst_BenchlibCounting,
    std::vector<const char*> args(argv, argv + argc);
    args.push_back("-eventcounter");
    argc = int(args.size());
    argv = const_cast<char**>(&args[0]);
    QTEST_MAIN_SETUP())

#include "tst_benchlibcounting.moc"
