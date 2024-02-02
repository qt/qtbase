// Copyright (C) 2013 David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QtCore>

class tst_QThreadPool : public QObject
{
    Q_OBJECT

public:
    tst_QThreadPool();
    ~tst_QThreadPool();

private slots:
    void startRunnables();
    void activeThreadCount();
};

tst_QThreadPool::tst_QThreadPool()
{
}

tst_QThreadPool::~tst_QThreadPool()
{
}

class NoOpRunnable : public QRunnable
{
public:
    void run() override {
    }
};

void tst_QThreadPool::startRunnables()
{
    QThreadPool threadPool;
    threadPool.setMaxThreadCount(10);
    QBENCHMARK {
        threadPool.start(new NoOpRunnable());
    }
}

void tst_QThreadPool::activeThreadCount()
{
    QThreadPool threadPool;
    threadPool.start(new NoOpRunnable());
    QBENCHMARK {
        QVERIFY(threadPool.activeThreadCount() <= 10);
    }
}

QTEST_MAIN(tst_QThreadPool)

#include "tst_bench_qthreadpool.moc"
