// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QSemaphore>

#include <qelapsedtimer.h>
#include <qrunnable.h>
#include <qthreadpool.h>
#include <qstring.h>
#include <qmutex.h>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

using namespace std::chrono_literals;

typedef void (*FunctionPointer)();

class FunctionPointerTask : public QRunnable
{
public:
    FunctionPointerTask(FunctionPointer function)
    :function(function) {}
    void run() override { function(); }
private:
    FunctionPointer function;
};

QRunnable *createTask(FunctionPointer pointer)
{
    return new FunctionPointerTask(pointer);
}

class tst_QThreadPool : public QObject
{
    Q_OBJECT
public:
    tst_QThreadPool();
    ~tst_QThreadPool();

    static QMutex *functionTestMutex;

private slots:
    void runFunction();
    void runFunction2();
    void runFunction3();
    void createThreadRunFunction();
    void runMultiple();
    void waitcomplete();
    void runTask();
    void singleton();
    void destruction();
    void threadRecycling();
    void threadPriority();
    void expiryTimeout();
    void expiryTimeoutRace();
#ifndef QT_NO_EXCEPTIONS
    void exceptions();
#endif
    void setMaxThreadCount_data();
    void setMaxThreadCount();
    void setMaxThreadCountStartsAndStopsThreads();
    void reserveThread_data();
    void reserveThread();
    void releaseThread_data();
    void releaseThread();
    void reserveAndStart();
    void reserveAndStart2();
    void releaseAndBlock();
    void start();
    void tryStart();
    void tryStartPeakThreadCount();
    void tryStartCount();
    void priorityStart_data();
    void priorityStart();
    void qualityOfService();
    void waitForDone();
    void clear();
    void clearWithAutoDelete();
    void tryTake();
    void waitForDoneTimeout();
    void destroyingWaitsForTasksToFinish();
    void stackSize();
    void stressTest();
    void takeAllAndIncreaseMaxThreadCount();
    void waitForDoneAfterTake();
    void threadReuse();
    void nullFunctions();

private:
    QMutex m_functionTestMutex;
};


QMutex *tst_QThreadPool::functionTestMutex = nullptr;

tst_QThreadPool::tst_QThreadPool()
{
    tst_QThreadPool::functionTestMutex = &m_functionTestMutex;
}

tst_QThreadPool::~tst_QThreadPool()
{
    tst_QThreadPool::functionTestMutex = nullptr;
}

static int testFunctionCount;

void sleepTestFunction()
{
    QTest::qSleep(1000);
    ++testFunctionCount;
}

void emptyFunct()
{

}

void noSleepTestFunction()
{
    ++testFunctionCount;
}

void sleepTestFunctionMutex()
{
    Q_ASSERT(tst_QThreadPool::functionTestMutex);
    QTest::qSleep(1000);
    tst_QThreadPool::functionTestMutex->lock();
    ++testFunctionCount;
    tst_QThreadPool::functionTestMutex->unlock();
}

void noSleepTestFunctionMutex()
{
    Q_ASSERT(tst_QThreadPool::functionTestMutex);
    tst_QThreadPool::functionTestMutex->lock();
    ++testFunctionCount;
    tst_QThreadPool::functionTestMutex->unlock();
}

constexpr int DefaultWaitForDoneTimeout = 1 * 60 * 1000; // 1min
// Using qFatal instead of QVERIFY to force exit if threads are still running after timeout.
// Otherwise, QCoreApplication will still wait for the stale threads and never exit the test.
#define WAIT_FOR_DONE(manager) \
    if ((manager).waitForDone(DefaultWaitForDoneTimeout)) {} else \
        qFatal("waitForDone returned false. Aborting to stop background threads.")

// uses explicit timeout in dtor's waitForDone() to avoid tests hanging overly long
class TestThreadPool : public QThreadPool
{
public:
    using QThreadPool::QThreadPool;
    ~TestThreadPool() { WAIT_FOR_DONE(*this); }
};

void tst_QThreadPool::runFunction()
{
    {
        TestThreadPool manager;
        testFunctionCount = 0;
        manager.start(noSleepTestFunction);
    }
    QCOMPARE(testFunctionCount, 1);
}

void tst_QThreadPool::runFunction2()
{
    int localCount = 0;
    {
        TestThreadPool manager;
        manager.start([&]() { ++localCount; });
    }
    QCOMPARE(localCount, 1);
}

struct DeleteCheck
{
    static bool s_deleted;
    ~DeleteCheck() { s_deleted = true; }
};
bool DeleteCheck::s_deleted = false;

void tst_QThreadPool::runFunction3()
{
    std::unique_ptr<DeleteCheck> ptr(new DeleteCheck);
    {
        TestThreadPool manager;
        manager.start([my_ptr = std::move(ptr)]() { });
    }
    QVERIFY(DeleteCheck::s_deleted);
}

void tst_QThreadPool::createThreadRunFunction()
{
    {
        TestThreadPool manager;
        testFunctionCount = 0;
        manager.start(noSleepTestFunction);
    }

    QCOMPARE(testFunctionCount, 1);
}

void tst_QThreadPool::runMultiple()
{
    const int runs = 10;

    {
        TestThreadPool manager;
        testFunctionCount = 0;
        for (int i = 0; i < runs; ++i) {
            manager.start(sleepTestFunctionMutex);
        }
    }
    QCOMPARE(testFunctionCount, runs);

    {
        TestThreadPool manager;
        testFunctionCount = 0;
        for (int i = 0; i < runs; ++i) {
            manager.start(noSleepTestFunctionMutex);
        }
    }
    QCOMPARE(testFunctionCount, runs);

    {
        TestThreadPool manager;
        for (int i = 0; i < 500; ++i)
            manager.start(emptyFunct);
    }
}

void tst_QThreadPool::waitcomplete()
{
    testFunctionCount = 0;
    const int runs = 500;
    for (int i = 0; i < 500; ++i) {
        // TestThreadPool pool; // no, we're checking ~QThreadPool()'s waitForDone()
        QThreadPool pool;
        pool.start(noSleepTestFunction);
    }
    QCOMPARE(testFunctionCount, runs);
}

static QAtomicInt ran; // bool
class TestTask : public QRunnable
{
public:
    void run() override
    {
        ran.storeRelaxed(true);
    }
};

void tst_QThreadPool::runTask()
{
    TestThreadPool manager;
    ran.storeRelaxed(false);
    manager.start(new TestTask());
    QTRY_VERIFY(ran.loadRelaxed());
}

/*
    Test running via QThreadPool::globalInstance()
*/
void tst_QThreadPool::singleton()
{
    ran.storeRelaxed(false);
    QThreadPool::globalInstance()->start(new TestTask());
    QTRY_VERIFY(ran.loadRelaxed());
}

static QAtomicInt *value = nullptr;
class IntAccessor : public QRunnable
{
public:
    void run() override
    {
        for (int i = 0; i < 100; ++i) {
            value->ref();
            QTest::qSleep(1);
        }
    }
};

/*
    Test that the ThreadManager destructor waits until
    all threads have completed.
*/
void tst_QThreadPool::destruction()
{
    value = new QAtomicInt;
    QThreadPool *threadManager = new QThreadPool();
    threadManager->start(new IntAccessor());
    threadManager->start(new IntAccessor());
    delete threadManager;
    delete value;
    value = nullptr;
}

static QSemaphore threadRecyclingSemaphore;
static QThread *recycledThread = nullptr;

class ThreadRecorderTask : public QRunnable
{
public:
    void run() override
    {
        recycledThread = QThread::currentThread();
        threadRecyclingSemaphore.release();
    }
};

/*
    Test that the thread pool really reuses threads.
*/
void tst_QThreadPool::threadRecycling()
{
    TestThreadPool threadPool;

    threadPool.start(new ThreadRecorderTask());
    threadRecyclingSemaphore.acquire();
    QThread *thread1 = recycledThread;

    QTest::qSleep(100);

    threadPool.start(new ThreadRecorderTask());
    threadRecyclingSemaphore.acquire();
    QThread *thread2 = recycledThread;
    QCOMPARE(thread1, thread2);

    QTest::qSleep(100);

    threadPool.start(new ThreadRecorderTask());
    threadRecyclingSemaphore.acquire();
    QThread *thread3 = recycledThread;
    QCOMPARE(thread2, thread3);
}

/*
    Test that the thread priority from the thread created by the pool matches
    the one configured on the pool.
*/
void tst_QThreadPool::threadPriority()
{
    QThread::Priority priority = QThread::HighPriority;
    TestThreadPool threadPool;
    threadPool.setThreadPriority(priority);

    threadPool.start(new ThreadRecorderTask());
    threadRecyclingSemaphore.acquire();
    QThread *thread = recycledThread;

    QTest::qSleep(100);

    QCOMPARE(thread->priority(), priority);
}

class ExpiryTimeoutTask : public QRunnable
{
public:
    QThread *thread;
    QAtomicInt runCount;
    QSemaphore semaphore;

    ExpiryTimeoutTask()
        : thread(nullptr), runCount(0)
    {
        setAutoDelete(false);
    }

    void run() override
    {
        thread = QThread::currentThread();
        runCount.ref();
        semaphore.release();
    }
};

void tst_QThreadPool::expiryTimeout()
{
    ExpiryTimeoutTask task;

    TestThreadPool threadPool;
    threadPool.setMaxThreadCount(1);

    int expiryTimeout = threadPool.expiryTimeout();
    threadPool.setExpiryTimeout(1000);
    QCOMPARE(threadPool.expiryTimeout(), 1000);

    // run the task
    threadPool.start(&task);
    QVERIFY(task.semaphore.tryAcquire(1, 10000));
    QCOMPARE(task.runCount.loadRelaxed(), 1);
    QVERIFY(!task.thread->wait(100));
    // thread should expire
    QThread *firstThread = task.thread;
    QVERIFY(task.thread->wait(10000));

    // run task again, thread should be restarted
    threadPool.start(&task);
    QVERIFY(task.semaphore.tryAcquire(1, 10000));
    QCOMPARE(task.runCount.loadRelaxed(), 2);
    QVERIFY(!task.thread->wait(100));
    // thread should expire again
    QVERIFY(task.thread->wait(10000));

    // thread pool should have reused the expired thread (instead of
    // starting a new one)
    QCOMPARE(firstThread, task.thread);

    threadPool.setExpiryTimeout(expiryTimeout);
    QCOMPARE(threadPool.expiryTimeout(), expiryTimeout);

    threadPool.waitForDone();
    // Negative times should be 'forever'
    threadPool.setExpiryTimeout(-1);
    QCOMPARE(threadPool.expiryTimeout(), -1);

    threadPool.start(&task);
    QVERIFY(task.semaphore.tryAcquire(1, 10'000));
    QCOMPARE(task.runCount.loadRelaxed(), 3);
    firstThread = task.thread;

    QTest::qWait(100); // Let some time elapse after the task finishes...

    // Since the thread never expires it should still be waiting for a new task:
    QVERIFY(firstThread->isRunning());
}

void tst_QThreadPool::expiryTimeoutRace() // QTBUG-3786
{
    ExpiryTimeoutTask task;

    TestThreadPool threadPool;
    threadPool.setMaxThreadCount(1);
    threadPool.setExpiryTimeout(50);
    const int numTasks = 20;
    for (int i = 0; i < numTasks; ++i) {
        threadPool.start(&task);
        QThread::sleep(50ms); // exactly the same as the expiry timeout
    }
    QVERIFY(task.semaphore.tryAcquire(numTasks, 10000));
    QCOMPARE(task.runCount.loadRelaxed(), numTasks);
    QVERIFY(threadPool.waitForDone(2000));
}

#ifndef QT_NO_EXCEPTIONS
class ExceptionTask : public QRunnable
{
public:
    void run() override
    {
        throw new int;
    }
};

void tst_QThreadPool::exceptions()
{
    ExceptionTask task;
    {
        TestThreadPool threadPool;
//  Uncomment this for a nice crash.
//        threadPool.start(&task);
    }
}
#endif

void tst_QThreadPool::setMaxThreadCount_data()
{
    QTest::addColumn<int>("limit");

    QTest::newRow("1") << 1;
    QTest::newRow("-1") << -1;
    QTest::newRow("2") << 2;
    QTest::newRow("-2") << -2;
    QTest::newRow("4") << 4;
    QTest::newRow("-4") << -4;
    QTest::newRow("0") << 0;
    QTest::newRow("12345") << 12345;
    QTest::newRow("-6789") << -6789;
    QTest::newRow("42") << 42;
    QTest::newRow("-666") << -666;
}

void tst_QThreadPool::setMaxThreadCount()
{
    QFETCH(int, limit);
    QThreadPool *threadPool = QThreadPool::globalInstance();
    int savedLimit = threadPool->maxThreadCount();
    auto restoreThreadCount = qScopeGuard([=]{
        threadPool->setMaxThreadCount(savedLimit);
    });

    // maxThreadCount() should always return the previous argument to
    // setMaxThreadCount(), regardless of input
    threadPool->setMaxThreadCount(limit);
    QCOMPARE(threadPool->maxThreadCount(), limit);

    // the value returned from maxThreadCount() should always be valid input for setMaxThreadCount()
    threadPool->setMaxThreadCount(savedLimit);
    QCOMPARE(threadPool->maxThreadCount(), savedLimit);

    // setting the limit on children should have no effect on the parent
    {
        TestThreadPool threadPool2(threadPool);
        savedLimit = threadPool2.maxThreadCount();

        // maxThreadCount() should always return the previous argument to
        // setMaxThreadCount(), regardless of input
        threadPool2.setMaxThreadCount(limit);
        QCOMPARE(threadPool2.maxThreadCount(), limit);

        // the value returned from maxThreadCount() should always be valid input for setMaxThreadCount()
        threadPool2.setMaxThreadCount(savedLimit);
        QCOMPARE(threadPool2.maxThreadCount(), savedLimit);
    }
}

void tst_QThreadPool::setMaxThreadCountStartsAndStopsThreads()
{
    class WaitingTask : public QRunnable
    {
    public:
        QSemaphore waitForStarted, waitToFinish;

        WaitingTask() { setAutoDelete(false); }

        void run() override
        {
            waitForStarted.release();
            waitToFinish.acquire();
        }
    };

    TestThreadPool threadPool;
    threadPool.setMaxThreadCount(-1);   // docs say we'll always start at least one

    WaitingTask task;
    threadPool.start(&task);
    QVERIFY(task.waitForStarted.tryAcquire(1, 1000));

    // thread limit is 1, cannot start more tasks
    threadPool.start(&task);
    QVERIFY(!task.waitForStarted.tryAcquire(1, 1000));

    // increasing the limit by 1 should start the task immediately
    threadPool.setMaxThreadCount(2);
    QVERIFY(task.waitForStarted.tryAcquire(1, 1000));

    // ... but we still cannot start more tasks
    threadPool.start(&task);
    QVERIFY(!task.waitForStarted.tryAcquire(1, 1000));

    // increasing the limit should be able to start more than one at a time
    threadPool.start(&task);
    threadPool.setMaxThreadCount(4);
    QVERIFY(task.waitForStarted.tryAcquire(2, 1000));

    // ... but we still cannot start more tasks
    threadPool.start(&task);
    threadPool.start(&task);
    QVERIFY(!task.waitForStarted.tryAcquire(2, 1000));

    // decreasing the thread limit should cause the active thread count to go down
    threadPool.setMaxThreadCount(2);
    QCOMPARE(threadPool.activeThreadCount(), 4);
    task.waitToFinish.release(2);
    QTest::qWait(1000);
    QCOMPARE(threadPool.activeThreadCount(), 2);

    // ... and we still cannot start more tasks
    threadPool.start(&task);
    threadPool.start(&task);
    QVERIFY(!task.waitForStarted.tryAcquire(2, 1000));

    // start all remaining tasks
    threadPool.start(&task);
    threadPool.start(&task);
    threadPool.start(&task);
    threadPool.start(&task);
    threadPool.setMaxThreadCount(8);
    QVERIFY(task.waitForStarted.tryAcquire(6, 1000));

    task.waitToFinish.release(10);
    threadPool.waitForDone();
}

void tst_QThreadPool::reserveThread_data()
{
    setMaxThreadCount_data();
}

void tst_QThreadPool::reserveThread()
{
    QFETCH(int, limit);
    QThreadPool *threadpool = QThreadPool::globalInstance();
    const int savedLimit = threadpool->maxThreadCount();
    auto restoreThreadCount = qScopeGuard([=]{
        threadpool->setMaxThreadCount(savedLimit);
    });

    threadpool->setMaxThreadCount(limit);

    // reserve up to the limit
    for (int i = 0; i < limit; ++i)
        threadpool->reserveThread();

    // reserveThread() should always reserve a thread, regardless of
    // how many have been previously reserved
    threadpool->reserveThread();
    QCOMPARE(threadpool->activeThreadCount(), (limit > 0 ? limit : 0) + 1);
    threadpool->reserveThread();
    QCOMPARE(threadpool->activeThreadCount(), (limit > 0 ? limit : 0) + 2);

    // cleanup
    threadpool->releaseThread();
    threadpool->releaseThread();
    for (int i = 0; i < limit; ++i)
        threadpool->releaseThread();

    // reserving threads in children should not effect the parent
    {
        TestThreadPool threadpool2(threadpool);
        threadpool2.setMaxThreadCount(limit);

        // reserve up to the limit
        for (int i = 0; i < limit; ++i)
            threadpool2.reserveThread();

        // reserveThread() should always reserve a thread, regardless
        // of how many have been previously reserved
        threadpool2.reserveThread();
        QCOMPARE(threadpool2.activeThreadCount(), (limit > 0 ? limit : 0) + 1);
        threadpool2.reserveThread();
        QCOMPARE(threadpool2.activeThreadCount(), (limit > 0 ? limit : 0) + 2);

        threadpool->reserveThread();
        QCOMPARE(threadpool->activeThreadCount(), 1);
        threadpool->reserveThread();
        QCOMPARE(threadpool->activeThreadCount(), 2);

        // cleanup
        threadpool2.releaseThread();
        threadpool2.releaseThread();
        threadpool->releaseThread();
        threadpool->releaseThread();
        while (threadpool2.activeThreadCount() > 0)
            threadpool2.releaseThread();
    }
}

void tst_QThreadPool::releaseThread_data()
{
    setMaxThreadCount_data();
}

void tst_QThreadPool::releaseThread()
{
    QFETCH(int, limit);
    QThreadPool *threadpool = QThreadPool::globalInstance();
    const int savedLimit = threadpool->maxThreadCount();
    auto restoreThreadCount = qScopeGuard([=]{
        threadpool->setMaxThreadCount(savedLimit);
    });
    threadpool->setMaxThreadCount(limit);

    // reserve up to the limit
    for (int i = 0; i < limit; ++i)
        threadpool->reserveThread();

    // release should decrease the number of reserved threads
    int reserved = threadpool->activeThreadCount();
    while (reserved-- > 0) {
        threadpool->releaseThread();
        QCOMPARE(threadpool->activeThreadCount(), reserved);
    }
    QCOMPARE(threadpool->activeThreadCount(), 0);

    // releaseThread() can release more than have been reserved
    threadpool->releaseThread();
    QCOMPARE(threadpool->activeThreadCount(), -1);
    threadpool->reserveThread();
    QCOMPARE(threadpool->activeThreadCount(), 0);

    // releasing threads in children should not effect the parent
    {
        TestThreadPool threadpool2(threadpool);
        threadpool2.setMaxThreadCount(limit);

        // reserve up to the limit
        for (int i = 0; i < limit; ++i)
            threadpool2.reserveThread();

        // release should decrease the number of reserved threads
        int reserved = threadpool2.activeThreadCount();
        while (reserved-- > 0) {
            threadpool2.releaseThread();
            QCOMPARE(threadpool2.activeThreadCount(), reserved);
            QCOMPARE(threadpool->activeThreadCount(), 0);
        }
        QCOMPARE(threadpool2.activeThreadCount(), 0);
        QCOMPARE(threadpool->activeThreadCount(), 0);

        // releaseThread() can release more than have been reserved
        threadpool2.releaseThread();
        QCOMPARE(threadpool2.activeThreadCount(), -1);
        QCOMPARE(threadpool->activeThreadCount(), 0);
        threadpool2.reserveThread();
        QCOMPARE(threadpool2.activeThreadCount(), 0);
        QCOMPARE(threadpool->activeThreadCount(), 0);
    }
}

void tst_QThreadPool::reserveAndStart() // QTBUG-21051
{
    class WaitingTask : public QRunnable
    {
    public:
        QAtomicInt count;
        QSemaphore waitForStarted;
        QSemaphore waitBeforeDone;

        WaitingTask() { setAutoDelete(false); }

        void run() override
        {
            count.ref();
            waitForStarted.release();
            waitBeforeDone.acquire();
        }
    };

    // Set up
    QThreadPool *threadpool = QThreadPool::globalInstance();
    int savedLimit = threadpool->maxThreadCount();
    auto restoreThreadCount = qScopeGuard([=]{
        threadpool->setMaxThreadCount(savedLimit);
    });

    threadpool->setMaxThreadCount(1);
    QCOMPARE(threadpool->activeThreadCount(), 0);

    // reserve
    threadpool->reserveThread();
    QCOMPARE(threadpool->activeThreadCount(), 1);

    // start a task, to get a running thread, works since one thread is always allowed
    WaitingTask task;
    threadpool->start(&task);
    QCOMPARE(threadpool->activeThreadCount(), 2);
    // tryStart() will fail since activeThreadCount() >= maxThreadCount() and one thread is already running
    QVERIFY(!threadpool->tryStart(&task));
    QTRY_COMPARE(threadpool->activeThreadCount(), 2);
    task.waitForStarted.acquire();
    task.waitBeforeDone.release();
    QTRY_COMPARE(task.count.loadRelaxed(), 1);
    QTRY_COMPARE(threadpool->activeThreadCount(), 1);

    // start() will wake up the waiting thread.
    threadpool->start(&task);
    QTRY_COMPARE(threadpool->activeThreadCount(), 2);
    QTRY_COMPARE(task.count.loadRelaxed(), 2);
    WaitingTask task2;
    // startOnReservedThread() will try to take the reserved task, but end up waiting instead
    threadpool->startOnReservedThread(&task2);
    QTRY_COMPARE(threadpool->activeThreadCount(), 1);
    task.waitForStarted.acquire();
    task.waitBeforeDone.release();
    QTRY_COMPARE(threadpool->activeThreadCount(), 1);
    task2.waitForStarted.acquire();
    task2.waitBeforeDone.release();

    QTRY_COMPARE(threadpool->activeThreadCount(), 0);
}

void tst_QThreadPool::reserveAndStart2()
{
    class WaitingTask : public QRunnable
    {
    public:
        QSemaphore waitBeforeDone;

        WaitingTask() { setAutoDelete(false); }

        void run() override
        {
            waitBeforeDone.acquire();
        }
    };

    // Set up
    QThreadPool *threadpool = QThreadPool::globalInstance();
    int savedLimit = threadpool->maxThreadCount();
    auto restoreThreadCount = qScopeGuard([=]{
        threadpool->setMaxThreadCount(savedLimit);
    });
    threadpool->setMaxThreadCount(2);

    // reserve
    threadpool->reserveThread();

    // start two task, to get a running thread and one queued
    WaitingTask task1, task2, task3;
    threadpool->start(&task1);
    // one running thread, one reserved:
    QCOMPARE(threadpool->activeThreadCount(), 2);
    // task2 starts queued
    threadpool->start(&task2);
    QCOMPARE(threadpool->activeThreadCount(), 2);
    // startOnReservedThread() will take the reserved thread however, bypassing the queue
    threadpool->startOnReservedThread(&task3);
    // two running threads, none reserved:
    QCOMPARE(threadpool->activeThreadCount(), 2);
    task3.waitBeforeDone.release();
    // task3 can finish even if all other tasks are blocking
    // then task2 will use the previously reserved thread
    task2.waitBeforeDone.release();
    QTRY_COMPARE(threadpool->activeThreadCount(), 1);
    task1.waitBeforeDone.release();
    QTRY_COMPARE(threadpool->activeThreadCount(), 0);
}

void tst_QThreadPool::releaseAndBlock()
{
    class WaitingTask : public QRunnable
    {
    public:
        QSemaphore waitBeforeDone;

        WaitingTask() { setAutoDelete(false); }

        void run() override
        {
            waitBeforeDone.acquire();
        }
    };

    // Set up
    QThreadPool *threadpool = QThreadPool::globalInstance();
    const int savedLimit = threadpool->maxThreadCount();
    auto restoreThreadCount = qScopeGuard([=]{
        threadpool->setMaxThreadCount(savedLimit);
    });

    threadpool->setMaxThreadCount(1);
    QCOMPARE(threadpool->activeThreadCount(), 0);

    // start a task, to get a running thread, works since one thread is always allowed
    WaitingTask task1, task2;
    threadpool->start(&task1);
    QCOMPARE(threadpool->activeThreadCount(), 1);

    // tryStart() will fail since activeThreadCount() >= maxThreadCount() and one thread is already running
    QVERIFY(!threadpool->tryStart(&task2));
    QCOMPARE(threadpool->activeThreadCount(), 1);

    // Use release without reserve to account for the blocking thread.
    threadpool->releaseThread();
    QTRY_COMPARE(threadpool->activeThreadCount(), 0);

    // Now we can start task2
    QVERIFY(threadpool->tryStart(&task2));
    QCOMPARE(threadpool->activeThreadCount(), 1);
    task2.waitBeforeDone.release();
    QTRY_COMPARE(threadpool->activeThreadCount(), 0);

    threadpool->reserveThread();
    QCOMPARE(threadpool->activeThreadCount(), 1);
    task1.waitBeforeDone.release();
    QTRY_COMPARE(threadpool->activeThreadCount(), 0);
}

static QAtomicInt count;
class CountingRunnable : public QRunnable
{
public:
    void run() override
    {
        count.ref();
    }
};

void tst_QThreadPool::start()
{
    const int runs = 1000;
    count.storeRelaxed(0);
    {
        TestThreadPool threadPool;
        for (int i = 0; i< runs; ++i) {
            threadPool.start(new CountingRunnable());
        }
    }
    QCOMPARE(count.loadRelaxed(), runs);
}

void tst_QThreadPool::tryStart()
{
    class WaitingTask : public QRunnable
    {
    public:
        QSemaphore semaphore;

        WaitingTask() { setAutoDelete(false); }

        void run() override
        {
            semaphore.acquire();
            count.ref();
        }
    };

    count.storeRelaxed(0);

    WaitingTask task;
    TestThreadPool threadPool;
    for (int i = 0; i < threadPool.maxThreadCount(); ++i) {
        threadPool.start(&task);
    }
    QVERIFY(!threadPool.tryStart(&task));
    task.semaphore.release(threadPool.maxThreadCount());
    WAIT_FOR_DONE(threadPool);
    QCOMPARE(count.loadRelaxed(), threadPool.maxThreadCount());
}

static QMutex mutex;
static QAtomicInt activeThreads;
static QAtomicInt peakActiveThreads;
void tst_QThreadPool::tryStartPeakThreadCount()
{
    class CounterTask : public QRunnable
    {
    public:
        CounterTask() { setAutoDelete(false); }

        void run() override
        {
            {
                QMutexLocker lock(&mutex);
                activeThreads.ref();
                peakActiveThreads.storeRelaxed(qMax(peakActiveThreads.loadRelaxed(), activeThreads.loadRelaxed()));
            }

            QTest::qWait(100);
            {
                QMutexLocker lock(&mutex);
                activeThreads.deref();
            }
        }
    };

    CounterTask task;
    TestThreadPool threadPool;

    for (int i = 0; i < 4*QThread::idealThreadCount(); ++i) {
        if (threadPool.tryStart(&task) == false)
            QTest::qWait(10);
    }
    QCOMPARE(peakActiveThreads.loadRelaxed(), QThread::idealThreadCount());

    for (int i = 0; i < 20; ++i) {
        if (threadPool.tryStart(&task) == false)
            QTest::qWait(10);
    }
    QCOMPARE(peakActiveThreads.loadRelaxed(), QThread::idealThreadCount());
}

void tst_QThreadPool::tryStartCount()
{
    class SleeperTask : public QRunnable
    {
    public:
        SleeperTask() { setAutoDelete(false); }

        void run() override
        {
            QTest::qWait(50);
        }
    };

    SleeperTask task;
    TestThreadPool threadPool;
    const int runs = 5;

    for (int i = 0; i < runs; ++i) {
        int count = 0;
        while (threadPool.tryStart(&task))
            ++count;
        QCOMPARE(count, QThread::idealThreadCount());

        QTRY_COMPARE(threadPool.activeThreadCount(), 0);
    }
}

void tst_QThreadPool::priorityStart_data()
{
    QTest::addColumn<int>("otherCount");
    QTest::newRow("0") << 0;
    QTest::newRow("1") << 1;
    QTest::newRow("2") << 2;
}

void tst_QThreadPool::priorityStart()
{
    class Holder : public QRunnable
    {
    public:
        QSemaphore &sem;
        Holder(QSemaphore &sem) : sem(sem) {}
        void run() override
        {
            sem.acquire();
        }
    };
    class Runner : public QRunnable
    {
    public:
        QAtomicPointer<QRunnable> &ptr;
        Runner(QAtomicPointer<QRunnable> &ptr) : ptr(ptr) {}
        void run() override
        {
            ptr.testAndSetRelaxed(nullptr, this);
        }
    };

    QFETCH(int, otherCount);
    QSemaphore sem;
    QAtomicPointer<QRunnable> firstStarted;
    QRunnable *expected;
    TestThreadPool threadPool;
    threadPool.setMaxThreadCount(1); // start only one thread at a time

    // queue the holder first
    // We need to be sure that all threads are active when we
    // queue the two Runners
    threadPool.start(new Holder(sem));
    while (otherCount--)
        threadPool.start(new Runner(firstStarted), 0); // priority 0
    threadPool.start(expected = new Runner(firstStarted), 1); // priority 1

    sem.release();
    WAIT_FOR_DONE(threadPool);
    QCOMPARE(firstStarted.loadRelaxed(), expected);
}

void tst_QThreadPool::qualityOfService()
{
    QThreadPool p;
    p.setServiceLevel(QThread::QualityOfService::Eco);
    QThread::QualityOfService level = QThread::QualityOfService::Auto;
    p.tryStart([&level](){
        level = QThread::currentThread()->serviceLevel();
    });
    QVERIFY(p.waitForDone(QDeadlineTimer(10s)));
    QCOMPARE(level, QThread::QualityOfService::Eco);
}

void tst_QThreadPool::waitForDone()
{
    QElapsedTimer total, pass;
    total.start();
    pass.start();

    TestThreadPool threadPool;
    while (total.elapsed() < 10000) {
        int runs;
        count.storeRelaxed(runs = 0);
        pass.restart();
        while (pass.elapsed() < 100) {
            threadPool.start(new CountingRunnable());
            ++runs;
        }
        WAIT_FOR_DONE(threadPool);
        QCOMPARE(count.loadRelaxed(), runs);

        count.storeRelaxed(runs = 0);
        pass.restart();
        while (pass.elapsed() < 100) {
            threadPool.start(new CountingRunnable());
            threadPool.start(new CountingRunnable());
            runs += 2;
        }
        WAIT_FOR_DONE(threadPool);
        QCOMPARE(count.loadRelaxed(), runs);
    }
}

void tst_QThreadPool::waitForDoneTimeout()
{
    QMutex mutex;
    class BlockedTask : public QRunnable
    {
    public:
      QMutex &mutex;
      explicit BlockedTask(QMutex &m) : mutex(m) {}

      void run() override
        {
          mutex.lock();
          mutex.unlock();
          QTest::qSleep(50);
        }
    };

    TestThreadPool threadPool;

    mutex.lock();
    threadPool.start(new BlockedTask(mutex));
    QVERIFY(!threadPool.waitForDone(100));
    mutex.unlock();
    QVERIFY(threadPool.waitForDone(400));
}

void tst_QThreadPool::clear()
{
    QSemaphore sem(0);
    class BlockingRunnable : public QRunnable
    {
        public:
            QSemaphore & sem;
            BlockingRunnable(QSemaphore & sem) : sem(sem){}
            void run() override
            {
                sem.acquire();
                count.ref();
            }
    };

    TestThreadPool threadPool;
    threadPool.setMaxThreadCount(10);
    int runs = 2 * threadPool.maxThreadCount();
    count.storeRelaxed(0);
    for (int i = 0; i <= runs; i++) {
        threadPool.start(new BlockingRunnable(sem));
    }
    threadPool.clear();
    sem.release(threadPool.maxThreadCount());
    WAIT_FOR_DONE(threadPool);
    QCOMPARE(count.loadRelaxed(), threadPool.maxThreadCount());
}

void tst_QThreadPool::clearWithAutoDelete()
{
    class MyRunnable : public QRunnable
    {
    public:
        MyRunnable() {}
        void run() override { QThread::sleep(30us); }
    };

    TestThreadPool threadPool;
    threadPool.setMaxThreadCount(4);
    const int loopCount = 20;
    const int batchSize = 500;
    // Should not crash see QTBUG-87092
    for (int i = 0; i < loopCount; i++) {
        threadPool.clear();
        for (int j = 0; j < batchSize; j++) {
            auto *runnable = new MyRunnable();
            runnable->setAutoDelete(true);
            threadPool.start(runnable);
        }
    }
}

void tst_QThreadPool::tryTake()
{
    QSemaphore sem(0);
    QSemaphore startedThreads(0);

    class BlockingRunnable : public QRunnable
    {
    public:
        QSemaphore &sem;
        QSemaphore &startedThreads;
        QAtomicInt &dtorCounter;
        QAtomicInt &runCounter;
        int dummy;

        explicit BlockingRunnable(QSemaphore &s, QSemaphore &started, QAtomicInt &c, QAtomicInt &r)
            : sem(s), startedThreads(started), dtorCounter(c), runCounter(r) {}

        ~BlockingRunnable() override
        {
            dtorCounter.fetchAndAddRelaxed(1);
        }

        void run() override
        {
            startedThreads.release();
            runCounter.fetchAndAddRelaxed(1);
            sem.acquire();
            count.ref();
        }
    };

    enum {
        MaxThreadCount = 3,
        OverProvisioning = 2,
        Runs = MaxThreadCount * OverProvisioning
    };

    TestThreadPool threadPool;
    threadPool.setMaxThreadCount(MaxThreadCount);
    BlockingRunnable *runnables[Runs];

    // ensure that the QThreadPool doesn't deadlock if any of the checks fail
    // and cause an early return:
    const QSemaphoreReleaser semReleaser(sem, Runs);

    count.storeRelaxed(0);
    QAtomicInt dtorCounter = 0;
    QAtomicInt runCounter = 0;
    for (int i = 0; i < Runs; i++) {
        runnables[i] = new BlockingRunnable(sem, startedThreads, dtorCounter, runCounter);
        runnables[i]->setAutoDelete(i != 0 && i != Runs - 1); // one which will run and one which will not
        QVERIFY(!threadPool.tryTake(runnables[i])); // verify NOOP for jobs not in the queue
        threadPool.start(runnables[i]);
    }
    // wait for all worker threads to have started up:
    QVERIFY(startedThreads.tryAcquire(MaxThreadCount, 60*1000 /* 1min */));

    for (int i = 0; i < MaxThreadCount; ++i) {
        // check taking runnables doesn't work once they were started:
        QVERIFY(!threadPool.tryTake(runnables[i]));
    }
    for (int i = MaxThreadCount; i < Runs ; ++i) {
        QVERIFY(threadPool.tryTake(runnables[i]));
        delete runnables[i];
    }

    runnables[0]->dummy = 0; // valgrind will catch this if tryTake() is crazy enough to delete currently running jobs
    QCOMPARE(dtorCounter.loadRelaxed(), int(Runs - MaxThreadCount));
    sem.release(MaxThreadCount);
    WAIT_FOR_DONE(threadPool);
    QCOMPARE(runCounter.loadRelaxed(), int(MaxThreadCount));
    QCOMPARE(count.loadRelaxed(), int(MaxThreadCount));
    QCOMPARE(dtorCounter.loadRelaxed(), int(Runs - 1));
    delete runnables[0]; // if the pool deletes them then we'll get double-free crash
}

void tst_QThreadPool::destroyingWaitsForTasksToFinish()
{
    QElapsedTimer total, pass;
    total.start();
    pass.start();

    while (total.elapsed() < 10000) {
        int runs;
        count.storeRelaxed(runs = 0);
        {
            TestThreadPool threadPool;
            pass.restart();
            while (pass.elapsed() < 100) {
                threadPool.start(new CountingRunnable());
                ++runs;
            }
        }
        QCOMPARE(count.loadRelaxed(), runs);

        count.storeRelaxed(runs = 0);
        {
            TestThreadPool threadPool;
            pass.restart();
            while (pass.elapsed() < 100) {
                threadPool.start(new CountingRunnable());
                threadPool.start(new CountingRunnable());
                runs += 2;
            }
        }
        QCOMPARE(count.loadRelaxed(), runs);
    }
}

// Verify that QThreadPool::stackSize is used when creating
// new threads. Note that this tests the Qt property only
// since QThread::stackSize() does not reflect the actual
// stack size used by the native thread.
void tst_QThreadPool::stackSize()
{
#if defined(Q_OS_UNIX) && !(defined(_POSIX_THREAD_ATTR_STACKSIZE) && (_POSIX_THREAD_ATTR_STACKSIZE-0 > 0))
    QSKIP("Setting stack size is unsupported on this platform.");
#endif

    uint targetStackSize = 512 * 1024;
    uint threadStackSize = 1; // impossible value

    class StackSizeChecker : public QRunnable
    {
        public:
        uint *stackSize;

        StackSizeChecker(uint *stackSize)
        :stackSize(stackSize)
        {

        }

        void run() override
        {
            *stackSize = QThread::currentThread()->stackSize();
        }
    };

    TestThreadPool threadPool;
    threadPool.setStackSize(targetStackSize);
    threadPool.start(new StackSizeChecker(&threadStackSize));
    WAIT_FOR_DONE(threadPool);
    QCOMPARE(threadStackSize, targetStackSize);
}

void tst_QThreadPool::stressTest()
{
    class Task : public QRunnable
    {
        QSemaphore semaphore;
    public:
        Task() { setAutoDelete(false); }

        void start()
        {
            QThreadPool::globalInstance()->start(this);
        }

        void wait()
        {
            semaphore.acquire();
        }

        void run() override
        {
            semaphore.release();
        }
    };

    QElapsedTimer total;
    total.start();
    while (total.elapsed() < 30000) {
        Task t;
        t.start();
        t.wait();
    }
}

void tst_QThreadPool::takeAllAndIncreaseMaxThreadCount() {
    class Task : public QRunnable
    {
    public:
        Task(QSemaphore *mainBarrier, QSemaphore *threadBarrier)
            : m_mainBarrier(mainBarrier)
            , m_threadBarrier(threadBarrier)
        {
            setAutoDelete(false);
        }

        void run() override
        {
            m_mainBarrier->release();
            m_threadBarrier->acquire();
        }
    private:
        QSemaphore *m_mainBarrier;
        QSemaphore *m_threadBarrier;
    };

    QSemaphore mainBarrier;
    QSemaphore taskBarrier;

    TestThreadPool threadPool;
    threadPool.setMaxThreadCount(1);

    Task task1(&mainBarrier, &taskBarrier);
    Task task2(&mainBarrier, &taskBarrier);
    Task task3(&mainBarrier, &taskBarrier);

    threadPool.start(&task1);
    threadPool.start(&task2);
    threadPool.start(&task3);

    mainBarrier.acquire(1);

    QCOMPARE(threadPool.activeThreadCount(), 1);

    QVERIFY(!threadPool.tryTake(&task1));
    QVERIFY(threadPool.tryTake(&task2));
    QVERIFY(threadPool.tryTake(&task3));

    // A bad queue implementation can segfault here because two consecutive items in the queue
    // have been taken
    threadPool.setMaxThreadCount(4);

    // Even though we increase the max thread count, there should only be one job to run
    QCOMPARE(threadPool.activeThreadCount(), 1);

    // Make sure jobs 2 and 3 never started
    QCOMPARE(mainBarrier.available(), 0);

    taskBarrier.release(1);

    WAIT_FOR_DONE(threadPool);

    QCOMPARE(threadPool.activeThreadCount(), 0);
}

void tst_QThreadPool::waitForDoneAfterTake()
{
    class Task : public QRunnable
    {
    public:
        Task(QSemaphore *mainBarrier, QSemaphore *threadBarrier)
            : m_mainBarrier(mainBarrier)
            , m_threadBarrier(threadBarrier)
        {}

        void run() override
        {
            m_mainBarrier->release();
            m_threadBarrier->acquire();
        }

    private:
        QSemaphore *m_mainBarrier = nullptr;
        QSemaphore *m_threadBarrier = nullptr;
    };

    int threadCount = 4;

    // Blocks the main thread from releasing the threadBarrier before all run() functions have started
    QSemaphore mainBarrier;
    // Blocks the tasks from completing their run function
    QSemaphore threadBarrier;

    TestThreadPool manager;
    manager.setMaxThreadCount(threadCount);

    // Fill all the threads with runnables that wait for the threadBarrier
    for (int i = 0; i < threadCount; i++) {
        auto *task = new Task(&mainBarrier, &threadBarrier);
        manager.start(task);
    }

    QVERIFY(manager.activeThreadCount() == manager.maxThreadCount());

    // Add runnables that are immediately removed from the pool queue.
    // This sets the queue elements to nullptr in QThreadPool and we want to test that
    // the threads keep going through the queue after encountering a nullptr.
    for (int i = 0; i < threadCount; i++) {
        QScopedPointer<QRunnable> runnable(createTask(emptyFunct));
        manager.start(runnable.get());
        QVERIFY(manager.tryTake(runnable.get()));
    }

    // Add another runnable that will not be removed
    manager.start(createTask(emptyFunct));

    // Wait for the first runnables to start
    mainBarrier.acquire(threadCount);

    QVERIFY(mainBarrier.available() == 0);
    QVERIFY(threadBarrier.available() == 0);

    // Release runnables that are waiting and expect all runnables to complete
    threadBarrier.release(threadCount);
}

/*
    Try trigger reuse of expired threads and check that all tasks execute.

    This is a regression test for QTBUG-72872.
*/
void tst_QThreadPool::threadReuse()
{
    TestThreadPool manager;
    manager.setExpiryTimeout(-1);
    manager.setMaxThreadCount(1);

    constexpr int repeatCount = 10000;
    constexpr int timeoutMs = 1000;
    QSemaphore sem;

    for (int i = 0; i < repeatCount; i++) {
        manager.start([&sem]() { sem.release(); });
        manager.start([&sem]() { sem.release(); });
        manager.releaseThread();
        QVERIFY(sem.tryAcquire(2, timeoutMs));
        manager.reserveThread();
    }
}

void tst_QThreadPool::nullFunctions()
{
    const auto expectWarning = [] {
            QTest::ignoreMessage(QtMsgType::QtWarningMsg,
                                 "Trying to create null QRunnable. This may stop working.");
        };
    // Note this is not necessarily testing intended behavior, only undocumented behavior.
    // If this is changed it should be noted in Behavioral Changes.
    FunctionPointer nullFunction = nullptr;
    std::function<void()> nullStdFunction(nullptr);
    {
        TestThreadPool manager;
        // should not crash:
        expectWarning();
        manager.start(nullFunction);
        expectWarning();
        manager.start(nullStdFunction);
        // should fail (and not leak):
        expectWarning();
        QVERIFY(!manager.tryStart(nullStdFunction));
        expectWarning();
        QVERIFY(!manager.tryStart(nullFunction));
    }
}

QTEST_MAIN(tst_QThreadPool);
#include "tst_qthreadpool.moc"
