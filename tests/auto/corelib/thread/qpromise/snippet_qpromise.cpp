// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

// Note: this file is published under a license that is different from a default
//       test sources license. This is intentional to comply with default
//       snippet license.

#include <QCoreApplication>
#include <QTest>

#include <qfuture.h>
#include <qfuturewatcher.h>
#include <qpromise.h>
#include <qscopedpointer.h>
#include <qsharedpointer.h>

class snippet_QPromise
{
public:
    static void basicExample();
    static void multithreadExample();
    static void suspendExample();
};

void snippet_QPromise::basicExample()
{
#if QT_CONFIG(cxx11_future)
//! [basic]
    QPromise<int> promise;
    QFuture<int> future = promise.future();

    QScopedPointer<QThread> thread(QThread::create([] (QPromise<int> promise) {
        promise.start();   // notifies QFuture that the computation is started
        promise.addResult(42);
        promise.finish();  // notifies QFuture that the computation is finished
    }, std::move(promise)));
    thread->start();

    future.waitForFinished();  // blocks until QPromise::finish is called
    future.result();  // returns 42
//! [basic]

    QCOMPARE(future.result(), 42);
    thread->wait();
#endif
}

void snippet_QPromise::multithreadExample()
{
#if QT_CONFIG(cxx11_future)
//! [multithread_init]
    QSharedPointer<QPromise<int>> sharedPromise(new QPromise<int>());
    QFuture<int> future = sharedPromise->future();

    // ...

    sharedPromise->start();
//! [multithread_init]

//! [multithread_main]
    // here, QPromise is shared between threads via a smart pointer
    QScopedPointer<QThread> threads[] = {
        QScopedPointer<QThread>(QThread::create([] (auto sharedPromise) {
            sharedPromise->addResult(0, 0);  // adds value 0 by index 0
        }, sharedPromise)),
        QScopedPointer<QThread>(QThread::create([] (auto sharedPromise) {
            sharedPromise->addResult(-1, 1);  // adds value -1 by index 1
        }, sharedPromise)),
        QScopedPointer<QThread>(QThread::create([] (auto sharedPromise) {
            sharedPromise->addResult(-2, 2);  // adds value -2 by index 2
        }, sharedPromise)),
        // ...
    };
    // start all threads
    for (auto& t : threads)
        t->start();

    // ...

    future.resultAt(0);  // waits until result at index 0 becomes available. returns value  0
    future.resultAt(1);  // waits until result at index 1 becomes available. returns value -1
    future.resultAt(2);  // waits until result at index 2 becomes available. returns value -2
//! [multithread_main]

    QCOMPARE(future.resultAt(0), 0);
    QCOMPARE(future.resultAt(1), -1);
    QCOMPARE(future.resultAt(2), -2);

    for (auto& t : threads)
        t->wait();
//! [multithread_cleanup]
    sharedPromise->finish();
//! [multithread_cleanup]
#endif
}

void snippet_QPromise::suspendExample()
{
#if QT_CONFIG(cxx11_future)
//! [suspend_start]
    // Create promise and future
    QPromise<int> promise;
    QFuture<int> future = promise.future();

    promise.start();
    // Start a computation thread that supports suspension and cancellation
    QScopedPointer<QThread> thread(QThread::create([] (QPromise<int> promise) {
        for (int i = 0; i < 100; ++i) {
            promise.addResult(i);
            promise.suspendIfRequested();   // support suspension
            if (promise.isCanceled())       // support cancellation
                break;
        }
        promise.finish();
    }, std::move(promise)));
    thread->start();
//! [suspend_start]

//! [suspend_suspend]
    future.suspend();
//! [suspend_suspend]

    // wait in calling thread until future.isSuspended() becomes true or do
    // something meanwhile
    while (!future.isSuspended()) {
        QThread::msleep(50);
    }

//! [suspend_intermediateResults]
    future.resultCount();  // returns some number between 0 and 100
    for (int i = 0; i < future.resultCount(); ++i) {
        // process results available before suspension
    }
//! [suspend_intermediateResults]

    // at least one result is available due to the logic inside a thread
    QVERIFY(future.resultCount() > 0);
    QVERIFY(future.resultCount() <= 100);
    for (int i = 0; i < future.resultCount(); ++i) {
        QCOMPARE(future.resultAt(i), i);
    }

//! [suspend_end]
    future.resume();  // resumes computation, this call will unblock the promise
    // alternatively, call future.cancel() to stop the computation

    future.waitForFinished();
    future.results();  // returns all computation results - array of values from 0 to 99
//! [suspend_end]

    thread->wait();

    QCOMPARE(future.resultCount(), 100);
    QList<int> expected(100);
    std::iota(expected.begin(), expected.end(), 0);
    QCOMPARE(future.results(), expected);
#endif
}
