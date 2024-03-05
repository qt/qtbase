// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QSemaphore>

#include <qatomic.h>
#include <qcoreapplication.h>
#include <qelapsedtimer.h>
#include <qmutex.h>
#include <qthread.h>
#include <qvarlengtharray.h>
#include <qwaitcondition.h>
#include <private/qvolatile_p.h>

using namespace std::chrono_literals;

class tst_QMutex : public QObject
{
    Q_OBJECT
public:
    enum class TimeUnit {
        Nanoseconds,
        Microseconds,
        Milliseconds,
        Seconds,
    };
    Q_ENUM(TimeUnit)

private slots:
    void tryLock_non_recursive();
    void try_lock_for_non_recursive();
    void try_lock_until_non_recursive();
    void tryLock_recursive();
    void try_lock_for_recursive();
    void try_lock_until_recursive();
    void lock_unlock_locked_tryLock();
    void stressTest();
    void tryLockRace();
    void tryLockDeadlock();
    void tryLockNegative_data();
    void tryLockNegative();
    void moreStress();
};

static const int iterations = 100;

static QAtomicInt lockCount(0);
static QMutex normalMutex;
static QRecursiveMutex recursiveMutex;
static QSemaphore testsTurn;
static QSemaphore threadsTurn;

/*
    Depending on the OS, tryWaits may return early than expected because of the
    resolution of the underlying timer is too coarse. E.g.: on Windows
    WaitForSingleObjectEx does *not* use high resolution multimedia timers, and
    it's actually very coarse, about 16msec by default.
*/
enum {
#ifdef Q_OS_WIN
    systemTimersResolution = 16,
#elif defined(Q_OS_QNX)
    systemTimersResolution = 10,
#else
    systemTimersResolution = 1,
#endif
    waitTime = 100
};

static constexpr std::chrono::milliseconds waitTimeAsDuration(waitTime);

void tst_QMutex::tryLock_non_recursive()
{
    class Thread : public QThread
    {
    public:
        void run() override
        {
            testsTurn.release();

            // TEST 1: thread can't acquire lock
            threadsTurn.acquire();
            QVERIFY(!normalMutex.tryLock());
            testsTurn.release();

            // TEST 2: thread can acquire lock
            threadsTurn.acquire();
            QVERIFY(normalMutex.tryLock());
            QVERIFY(lockCount.testAndSetRelaxed(0, 1));
            QVERIFY(!normalMutex.tryLock());
            QVERIFY(lockCount.testAndSetRelaxed(1, 0));
            normalMutex.unlock();
            testsTurn.release();

            // TEST 3: thread can't acquire lock, timeout = waitTime
            threadsTurn.acquire();
            QElapsedTimer timer;
            timer.start();
            QVERIFY(!normalMutex.tryLock(waitTime));
            QCOMPARE_GE(timer.elapsed(), waitTime - systemTimersResolution);
            testsTurn.release();

            // TEST 4: thread can acquire lock, timeout = waitTime
            threadsTurn.acquire();
            timer.start();
            QVERIFY(normalMutex.tryLock(waitTime));
            QCOMPARE_LE(timer.elapsed(), waitTime + systemTimersResolution);
            QVERIFY(lockCount.testAndSetRelaxed(0, 1));
            timer.start();
            // it's non-recursive, so the following lock needs to fail
            QVERIFY(!normalMutex.tryLock(waitTime));
            QCOMPARE_GE(timer.elapsed(), waitTime - systemTimersResolution);
            QVERIFY(lockCount.testAndSetRelaxed(1, 0));
            normalMutex.unlock();
            testsTurn.release();

            // TEST 5: thread can't acquire lock, timeout = 0
            threadsTurn.acquire();
            QVERIFY(!normalMutex.tryLock(0));
            testsTurn.release();

            // TEST 6: thread can acquire lock, timeout = 0
            threadsTurn.acquire();
            timer.start();
            QVERIFY(normalMutex.tryLock(0));
            QCOMPARE_LT(timer.elapsed(), waitTime + systemTimersResolution);
            QVERIFY(lockCount.testAndSetRelaxed(0, 1));
            QVERIFY(!normalMutex.tryLock(0));
            QVERIFY(lockCount.testAndSetRelaxed(1, 0));
            normalMutex.unlock();
            testsTurn.release();

            // TEST 7 overflow: thread can acquire lock, timeout = 3000 (QTBUG-24795)
            threadsTurn.acquire();
            timer.start();
            QVERIFY(normalMutex.tryLock(3000));
            QCOMPARE_LT(timer.elapsed(), 3000 + systemTimersResolution);
            normalMutex.unlock();
            testsTurn.release();

            threadsTurn.acquire();
        }
    };

    Thread thread;
    thread.start();

    qDebug("TEST 1: thread can't acquire lock");
    testsTurn.acquire();
    normalMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(0, 1));
    threadsTurn.release();

    qDebug("TEST 2: thread can acquire lock");
    testsTurn.acquire();
    QVERIFY(lockCount.testAndSetRelaxed(1, 0));
    normalMutex.unlock();
    threadsTurn.release();

    qDebug("TEST 3: thread can't acquire lock, timeout = waitTime");
    testsTurn.acquire();
    normalMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(0, 1));
    threadsTurn.release();

    qDebug("TEST 4: thread can acquire lock, timeout = waitTime");
    testsTurn.acquire();
    QVERIFY(lockCount.testAndSetRelaxed(1, 0));
    normalMutex.unlock();
    threadsTurn.release();

    qDebug("TEST 5: thread can't acquire lock, timeout = 0");
    testsTurn.acquire();
    normalMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(0, 1));
    threadsTurn.release();

    qDebug("TEST 6: thread can acquire lock, timeout = 0");
    testsTurn.acquire();
    QVERIFY(lockCount.testAndSetRelaxed(1, 0));
    normalMutex.unlock();
    threadsTurn.release();

    qDebug("TEST 7: thread can acquire lock, timeout = 3000   (QTBUG-24795)");
    testsTurn.acquire();
    normalMutex.lock();
    threadsTurn.release();
    QThread::sleep(100ms);
    normalMutex.unlock();

    // wait for thread to finish
    testsTurn.acquire();
    threadsTurn.release();
    thread.wait();
}

void tst_QMutex::try_lock_for_non_recursive()
{
    class Thread : public QThread
    {
    public:
        void run() override
        {
            testsTurn.release();

            // TEST 1: thread can't acquire lock
            threadsTurn.acquire();
            QVERIFY(!normalMutex.try_lock());
            testsTurn.release();

            // TEST 2: thread can acquire lock
            threadsTurn.acquire();
            QVERIFY(normalMutex.try_lock());
            QVERIFY(lockCount.testAndSetRelaxed(0, 1));
            QVERIFY(!normalMutex.try_lock());
            QVERIFY(lockCount.testAndSetRelaxed(1, 0));
            normalMutex.unlock();
            testsTurn.release();

            // TEST 3: thread can't acquire lock, timeout = waitTime
            threadsTurn.acquire();
            QElapsedTimer timer;
            timer.start();
            QVERIFY(!normalMutex.try_lock_for(waitTimeAsDuration));
            QCOMPARE_GE(timer.elapsed(), waitTime - systemTimersResolution);
            testsTurn.release();

            // TEST 4: thread can acquire lock, timeout = waitTime
            threadsTurn.acquire();
            timer.start();
            QVERIFY(normalMutex.try_lock_for(waitTimeAsDuration));
            QCOMPARE_LE(timer.elapsed(), waitTime + systemTimersResolution);
            QVERIFY(lockCount.testAndSetRelaxed(0, 1));
            timer.start();
            // it's non-recursive, so the following lock needs to fail
            QVERIFY(!normalMutex.try_lock_for(waitTimeAsDuration));
            QCOMPARE_GE(timer.elapsed(), waitTime - systemTimersResolution);
            QVERIFY(lockCount.testAndSetRelaxed(1, 0));
            normalMutex.unlock();
            testsTurn.release();

            // TEST 5: thread can't acquire lock, timeout = 0
            threadsTurn.acquire();
            QVERIFY(!normalMutex.try_lock_for(std::chrono::milliseconds::zero()));
            testsTurn.release();

            // TEST 6: thread can acquire lock, timeout = 0
            threadsTurn.acquire();
            timer.start();
            QVERIFY(normalMutex.try_lock_for(std::chrono::milliseconds::zero()));
            QCOMPARE_LT(timer.elapsed(), waitTime + systemTimersResolution);
            QVERIFY(lockCount.testAndSetRelaxed(0, 1));
            QVERIFY(!normalMutex.try_lock_for(std::chrono::milliseconds::zero()));
            QVERIFY(lockCount.testAndSetRelaxed(1, 0));
            normalMutex.unlock();
            testsTurn.release();

            // TEST 7 overflow: thread can acquire lock, timeout = 3000 (QTBUG-24795)
            threadsTurn.acquire();
            timer.start();
            QVERIFY(normalMutex.try_lock_for(std::chrono::milliseconds(3000)));
            QCOMPARE_LT(timer.elapsed(), 3000 + systemTimersResolution);
            normalMutex.unlock();
            testsTurn.release();

            threadsTurn.acquire();
        }
    };

    Thread thread;
    thread.start();

    // TEST 1: thread can't acquire lock
    testsTurn.acquire();
    normalMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(0, 1));
    threadsTurn.release();

    // TEST 2: thread can acquire lock
    testsTurn.acquire();
    QVERIFY(lockCount.testAndSetRelaxed(1, 0));
    normalMutex.unlock();
    threadsTurn.release();

    // TEST 3: thread can't acquire lock, timeout = waitTime
    testsTurn.acquire();
    normalMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(0, 1));
    threadsTurn.release();

    // TEST 4: thread can acquire lock, timeout = waitTime
    testsTurn.acquire();
    QVERIFY(lockCount.testAndSetRelaxed(1, 0));
    normalMutex.unlock();
    threadsTurn.release();

    // TEST 5: thread can't acquire lock, timeout = 0
    testsTurn.acquire();
    normalMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(0, 1));
    threadsTurn.release();

    // TEST 6: thread can acquire lock, timeout = 0
    testsTurn.acquire();
    QVERIFY(lockCount.testAndSetRelaxed(1, 0));
    normalMutex.unlock();
    threadsTurn.release();

    // TEST 7: thread can acquire lock, timeout = 3000   (QTBUG-24795)
    testsTurn.acquire();
    normalMutex.lock();
    threadsTurn.release();
    QThread::sleep(100ms);
    normalMutex.unlock();

    // wait for thread to finish
    testsTurn.acquire();
    threadsTurn.release();
    thread.wait();
}

void tst_QMutex::try_lock_until_non_recursive()
{
    class Thread : public QThread
    {
    public:
        void run() override
        {
            const std::chrono::milliseconds systemTimersResolutionAsDuration(systemTimersResolution);
            testsTurn.release();

            // TEST 1: thread can't acquire lock
            threadsTurn.acquire();
            QVERIFY(!normalMutex.try_lock());
            testsTurn.release();

            // TEST 2: thread can acquire lock
            threadsTurn.acquire();
            QVERIFY(normalMutex.try_lock());
            QVERIFY(lockCount.testAndSetRelaxed(0, 1));
            QVERIFY(!normalMutex.try_lock());
            QVERIFY(lockCount.testAndSetRelaxed(1, 0));
            normalMutex.unlock();
            testsTurn.release();

            // TEST 3: thread can't acquire lock, timeout = waitTime
            threadsTurn.acquire();
            auto endTimePoint = std::chrono::steady_clock::now() + waitTimeAsDuration;
            QVERIFY(!normalMutex.try_lock_until(endTimePoint));
            QCOMPARE_GE(std::chrono::steady_clock::now(), endTimePoint - systemTimersResolutionAsDuration);
            testsTurn.release();

            // TEST 4: thread can acquire lock, timeout = waitTime
            threadsTurn.acquire();
            endTimePoint = std::chrono::steady_clock::now() + waitTimeAsDuration;
            QVERIFY(normalMutex.try_lock_until(endTimePoint));
            QCOMPARE_LE(std::chrono::steady_clock::now(), endTimePoint + systemTimersResolutionAsDuration);
            QVERIFY(lockCount.testAndSetRelaxed(0, 1));
            endTimePoint = std::chrono::steady_clock::now() + waitTimeAsDuration;
            // it's non-recursive, so the following lock needs to fail
            QVERIFY(!normalMutex.try_lock_until(endTimePoint));
            QCOMPARE_GE(std::chrono::steady_clock::now(), endTimePoint - systemTimersResolutionAsDuration);
            QVERIFY(lockCount.testAndSetRelaxed(1, 0));
            normalMutex.unlock();
            testsTurn.release();

            // TEST 5: thread can't acquire lock, timeout = 0
            threadsTurn.acquire();
            QVERIFY(!normalMutex.try_lock_until(std::chrono::steady_clock::now()));
            testsTurn.release();

            // TEST 6: thread can acquire lock, timeout = 0
            threadsTurn.acquire();
            endTimePoint = std::chrono::steady_clock::now() + waitTimeAsDuration;
            QVERIFY(normalMutex.try_lock_until(std::chrono::steady_clock::now()));
            QCOMPARE_LT(std::chrono::steady_clock::now(), endTimePoint + systemTimersResolutionAsDuration);
            QVERIFY(lockCount.testAndSetRelaxed(0, 1));
            QVERIFY(!normalMutex.try_lock_until(std::chrono::steady_clock::now()));
            QVERIFY(lockCount.testAndSetRelaxed(1, 0));
            normalMutex.unlock();
            testsTurn.release();

            // TEST 7 overflow: thread can acquire lock, timeout = 3000 (QTBUG-24795)
            threadsTurn.acquire();
            endTimePoint = std::chrono::steady_clock::now() + std::chrono::milliseconds(3000);
            QVERIFY(normalMutex.try_lock_until(endTimePoint));
            QCOMPARE_LT(std::chrono::steady_clock::now(), endTimePoint + systemTimersResolutionAsDuration);
            normalMutex.unlock();
            testsTurn.release();

            threadsTurn.acquire();
        }
    };

    Thread thread;
    thread.start();

    // TEST 1: thread can't acquire lock
    testsTurn.acquire();
    normalMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(0, 1));
    threadsTurn.release();

    // TEST 2: thread can acquire lock
    testsTurn.acquire();
    QVERIFY(lockCount.testAndSetRelaxed(1, 0));
    normalMutex.unlock();
    threadsTurn.release();

    // TEST 3: thread can't acquire lock, timeout = waitTime
    testsTurn.acquire();
    normalMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(0, 1));
    threadsTurn.release();

    // TEST 4: thread can acquire lock, timeout = waitTime
    testsTurn.acquire();
    QVERIFY(lockCount.testAndSetRelaxed(1, 0));
    normalMutex.unlock();
    threadsTurn.release();

    // TEST 5: thread can't acquire lock, timeout = 0
    testsTurn.acquire();
    normalMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(0, 1));
    threadsTurn.release();

    // TEST 6: thread can acquire lock, timeout = 0
    testsTurn.acquire();
    QVERIFY(lockCount.testAndSetRelaxed(1, 0));
    normalMutex.unlock();
    threadsTurn.release();

    // TEST 7: thread can acquire lock, timeout = 3000   (QTBUG-24795)
    testsTurn.acquire();
    normalMutex.lock();
    threadsTurn.release();
    QThread::sleep(100ms);
    normalMutex.unlock();

    // wait for thread to finish
    testsTurn.acquire();
    threadsTurn.release();
    thread.wait();
}

void tst_QMutex::tryLock_recursive()
{
    class Thread : public QThread
    {
    public:
        void run() override
        {
            testsTurn.release();

            threadsTurn.acquire();
            QVERIFY(!recursiveMutex.tryLock());
            testsTurn.release();

            threadsTurn.acquire();
            QVERIFY(recursiveMutex.tryLock());
            QVERIFY(lockCount.testAndSetRelaxed(0, 1));
            QVERIFY(recursiveMutex.tryLock());
            QVERIFY(lockCount.testAndSetRelaxed(1, 2));
            QVERIFY(lockCount.testAndSetRelaxed(2, 1));
            recursiveMutex.unlock();
            QVERIFY(lockCount.testAndSetRelaxed(1, 0));
            recursiveMutex.unlock();
            testsTurn.release();

            threadsTurn.acquire();
            QElapsedTimer timer;
            timer.start();
            QVERIFY(!recursiveMutex.tryLock(waitTime));
            QCOMPARE_GE(timer.elapsed(), waitTime - systemTimersResolution);
            QVERIFY(!recursiveMutex.tryLock(0));
            testsTurn.release();

            threadsTurn.acquire();
            timer.start();
            QVERIFY(recursiveMutex.tryLock(waitTime));
            QCOMPARE_LE(timer.elapsed(), waitTime + systemTimersResolution);
            QVERIFY(lockCount.testAndSetRelaxed(0, 1));
            QVERIFY(recursiveMutex.tryLock(waitTime));
            QVERIFY(lockCount.testAndSetRelaxed(1, 2));
            QVERIFY(lockCount.testAndSetRelaxed(2, 1));
            recursiveMutex.unlock();
            QVERIFY(lockCount.testAndSetRelaxed(1, 0));
            recursiveMutex.unlock();
            testsTurn.release();

            threadsTurn.acquire();
            QVERIFY(!recursiveMutex.tryLock(0));
            QVERIFY(!recursiveMutex.tryLock(0));
            testsTurn.release();

            threadsTurn.acquire();
            timer.start();
            QVERIFY(recursiveMutex.tryLock(0));
            QCOMPARE_LT(timer.elapsed(), waitTime + systemTimersResolution);
            QVERIFY(lockCount.testAndSetRelaxed(0, 1));
            QVERIFY(recursiveMutex.tryLock(0));
            QVERIFY(lockCount.testAndSetRelaxed(1, 2));
            QVERIFY(lockCount.testAndSetRelaxed(2, 1));
            recursiveMutex.unlock();
            QVERIFY(lockCount.testAndSetRelaxed(1, 0));
            recursiveMutex.unlock();
            testsTurn.release();

            threadsTurn.acquire();
        }
    };

    Thread thread;
    thread.start();

    // thread can't acquire lock
    testsTurn.acquire();
    recursiveMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(0, 1));
    recursiveMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(1, 2));
    threadsTurn.release();

    // thread can acquire lock
    testsTurn.acquire();
    QVERIFY(lockCount.testAndSetRelaxed(2, 1));
    recursiveMutex.unlock();
    QVERIFY(lockCount.testAndSetRelaxed(1, 0));
    recursiveMutex.unlock();
    threadsTurn.release();

    // thread can't acquire lock, timeout = waitTime
    testsTurn.acquire();
    recursiveMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(0, 1));
    recursiveMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(1, 2));
    threadsTurn.release();

    // thread can acquire lock, timeout = waitTime
    testsTurn.acquire();
    QVERIFY(lockCount.testAndSetRelaxed(2, 1));
    recursiveMutex.unlock();
    QVERIFY(lockCount.testAndSetRelaxed(1, 0));
    recursiveMutex.unlock();
    threadsTurn.release();

    // thread can't acquire lock, timeout = 0
    testsTurn.acquire();
    recursiveMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(0, 1));
    recursiveMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(1, 2));
    threadsTurn.release();

    // thread can acquire lock, timeout = 0
    testsTurn.acquire();
    QVERIFY(lockCount.testAndSetRelaxed(2, 1));
    recursiveMutex.unlock();
    QVERIFY(lockCount.testAndSetRelaxed(1, 0));
    recursiveMutex.unlock();
    threadsTurn.release();

    // stop thread
    testsTurn.acquire();
    threadsTurn.release();
    thread.wait();
}

void tst_QMutex::try_lock_for_recursive()
{
    class Thread : public QThread
    {
    public:
        void run() override
        {
            testsTurn.release();

            threadsTurn.acquire();
            QVERIFY(!recursiveMutex.try_lock());
            testsTurn.release();

            threadsTurn.acquire();
            QVERIFY(recursiveMutex.try_lock());
            QVERIFY(lockCount.testAndSetRelaxed(0, 1));
            QVERIFY(recursiveMutex.try_lock());
            QVERIFY(lockCount.testAndSetRelaxed(1, 2));
            QVERIFY(lockCount.testAndSetRelaxed(2, 1));
            recursiveMutex.unlock();
            QVERIFY(lockCount.testAndSetRelaxed(1, 0));
            recursiveMutex.unlock();
            testsTurn.release();

            threadsTurn.acquire();
            QElapsedTimer timer;
            timer.start();
            QVERIFY(!recursiveMutex.try_lock_for(waitTimeAsDuration));
            QCOMPARE_GE(timer.elapsed(), waitTime - systemTimersResolution);
            QVERIFY(!recursiveMutex.try_lock_for(std::chrono::milliseconds::zero()));
            testsTurn.release();

            threadsTurn.acquire();
            timer.start();
            QVERIFY(recursiveMutex.try_lock_for(waitTimeAsDuration));
            QCOMPARE_LE(timer.elapsed(), waitTime + systemTimersResolution);
            QVERIFY(lockCount.testAndSetRelaxed(0, 1));
            QVERIFY(recursiveMutex.try_lock_for(waitTimeAsDuration));
            QVERIFY(lockCount.testAndSetRelaxed(1, 2));
            QVERIFY(lockCount.testAndSetRelaxed(2, 1));
            recursiveMutex.unlock();
            QVERIFY(lockCount.testAndSetRelaxed(1, 0));
            recursiveMutex.unlock();
            testsTurn.release();

            threadsTurn.acquire();
            QVERIFY(!recursiveMutex.try_lock_for(std::chrono::milliseconds::zero()));
            QVERIFY(!recursiveMutex.try_lock_for(std::chrono::milliseconds::zero()));
            testsTurn.release();

            threadsTurn.acquire();
            timer.start();
            QVERIFY(recursiveMutex.try_lock_for(std::chrono::milliseconds::zero()));
            QCOMPARE_LT(timer.elapsed(), waitTime + systemTimersResolution);
            QVERIFY(lockCount.testAndSetRelaxed(0, 1));
            QVERIFY(recursiveMutex.try_lock_for(std::chrono::milliseconds::zero()));
            QVERIFY(lockCount.testAndSetRelaxed(1, 2));
            QVERIFY(lockCount.testAndSetRelaxed(2, 1));
            recursiveMutex.unlock();
            QVERIFY(lockCount.testAndSetRelaxed(1, 0));
            recursiveMutex.unlock();
            testsTurn.release();

            threadsTurn.acquire();
        }
    };

    Thread thread;
    thread.start();

    // thread can't acquire lock
    testsTurn.acquire();
    recursiveMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(0, 1));
    recursiveMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(1, 2));
    threadsTurn.release();

    // thread can acquire lock
    testsTurn.acquire();
    QVERIFY(lockCount.testAndSetRelaxed(2, 1));
    recursiveMutex.unlock();
    QVERIFY(lockCount.testAndSetRelaxed(1, 0));
    recursiveMutex.unlock();
    threadsTurn.release();

    // thread can't acquire lock, timeout = waitTime
    testsTurn.acquire();
    recursiveMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(0, 1));
    recursiveMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(1, 2));
    threadsTurn.release();

    // thread can acquire lock, timeout = waitTime
    testsTurn.acquire();
    QVERIFY(lockCount.testAndSetRelaxed(2, 1));
    recursiveMutex.unlock();
    QVERIFY(lockCount.testAndSetRelaxed(1, 0));
    recursiveMutex.unlock();
    threadsTurn.release();

    // thread can't acquire lock, timeout = 0
    testsTurn.acquire();
    recursiveMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(0, 1));
    recursiveMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(1, 2));
    threadsTurn.release();

    // thread can acquire lock, timeout = 0
    testsTurn.acquire();
    QVERIFY(lockCount.testAndSetRelaxed(2, 1));
    recursiveMutex.unlock();
    QVERIFY(lockCount.testAndSetRelaxed(1, 0));
    recursiveMutex.unlock();
    threadsTurn.release();

    // stop thread
    testsTurn.acquire();
    threadsTurn.release();
    thread.wait();
}

void tst_QMutex::try_lock_until_recursive()
{
    class Thread : public QThread
    {
    public:
        void run() override
        {
            const std::chrono::milliseconds systemTimersResolutionAsDuration(systemTimersResolution);
            testsTurn.release();

            threadsTurn.acquire();
            QVERIFY(!recursiveMutex.try_lock());
            testsTurn.release();

            threadsTurn.acquire();
            QVERIFY(recursiveMutex.try_lock());
            QVERIFY(lockCount.testAndSetRelaxed(0, 1));
            QVERIFY(recursiveMutex.try_lock());
            QVERIFY(lockCount.testAndSetRelaxed(1, 2));
            QVERIFY(lockCount.testAndSetRelaxed(2, 1));
            recursiveMutex.unlock();
            QVERIFY(lockCount.testAndSetRelaxed(1, 0));
            recursiveMutex.unlock();
            testsTurn.release();

            threadsTurn.acquire();
            auto endTimePoint = std::chrono::steady_clock::now() + waitTimeAsDuration;
            QVERIFY(!recursiveMutex.try_lock_until(endTimePoint));
            QCOMPARE_GE(std::chrono::steady_clock::now(), endTimePoint - systemTimersResolutionAsDuration);
            QVERIFY(!recursiveMutex.try_lock());
            testsTurn.release();

            threadsTurn.acquire();
            endTimePoint = std::chrono::steady_clock::now() + waitTimeAsDuration;
            QVERIFY(recursiveMutex.try_lock_until(endTimePoint));
            QCOMPARE_LE(std::chrono::steady_clock::now(), endTimePoint + systemTimersResolutionAsDuration);
            QVERIFY(lockCount.testAndSetRelaxed(0, 1));
            endTimePoint = std::chrono::steady_clock::now() + waitTimeAsDuration;
            QVERIFY(recursiveMutex.try_lock_until(endTimePoint));
            QVERIFY(lockCount.testAndSetRelaxed(1, 2));
            QVERIFY(lockCount.testAndSetRelaxed(2, 1));
            recursiveMutex.unlock();
            QVERIFY(lockCount.testAndSetRelaxed(1, 0));
            recursiveMutex.unlock();
            testsTurn.release();

            threadsTurn.acquire();
            QVERIFY(!recursiveMutex.try_lock_until(std::chrono::steady_clock::now()));
            QVERIFY(!recursiveMutex.try_lock_until(std::chrono::steady_clock::now()));
            testsTurn.release();

            threadsTurn.acquire();
            endTimePoint = std::chrono::steady_clock::now() + waitTimeAsDuration;
            QVERIFY(recursiveMutex.try_lock_until(std::chrono::steady_clock::now()));
            QCOMPARE_LE(std::chrono::steady_clock::now(), endTimePoint + systemTimersResolutionAsDuration);
            QVERIFY(lockCount.testAndSetRelaxed(0, 1));
            QVERIFY(recursiveMutex.try_lock_until(std::chrono::steady_clock::now()));
            QVERIFY(lockCount.testAndSetRelaxed(1, 2));
            QVERIFY(lockCount.testAndSetRelaxed(2, 1));
            recursiveMutex.unlock();
            QVERIFY(lockCount.testAndSetRelaxed(1, 0));
            recursiveMutex.unlock();
            testsTurn.release();

            threadsTurn.acquire();
        }
    };

    Thread thread;
    thread.start();

    // thread can't acquire lock
    testsTurn.acquire();
    recursiveMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(0, 1));
    recursiveMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(1, 2));
    threadsTurn.release();

    // thread can acquire lock
    testsTurn.acquire();
    QVERIFY(lockCount.testAndSetRelaxed(2, 1));
    recursiveMutex.unlock();
    QVERIFY(lockCount.testAndSetRelaxed(1, 0));
    recursiveMutex.unlock();
    threadsTurn.release();

    // thread can't acquire lock, timeout = waitTime
    testsTurn.acquire();
    recursiveMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(0, 1));
    recursiveMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(1, 2));
    threadsTurn.release();

    // thread can acquire lock, timeout = waitTime
    testsTurn.acquire();
    QVERIFY(lockCount.testAndSetRelaxed(2, 1));
    recursiveMutex.unlock();
    QVERIFY(lockCount.testAndSetRelaxed(1, 0));
    recursiveMutex.unlock();
    threadsTurn.release();

    // thread can't acquire lock, timeout = 0
    testsTurn.acquire();
    recursiveMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(0, 1));
    recursiveMutex.lock();
    QVERIFY(lockCount.testAndSetRelaxed(1, 2));
    threadsTurn.release();

    // thread can acquire lock, timeout = 0
    testsTurn.acquire();
    QVERIFY(lockCount.testAndSetRelaxed(2, 1));
    recursiveMutex.unlock();
    QVERIFY(lockCount.testAndSetRelaxed(1, 0));
    recursiveMutex.unlock();
    threadsTurn.release();

    // stop thread
    testsTurn.acquire();
    threadsTurn.release();
    thread.wait();
}

class mutex_Thread : public QThread
{
public:
    QMutex mutex;
    QWaitCondition cond;

    QMutex &test_mutex;

    inline mutex_Thread(QMutex &m) : test_mutex(m) { }

    void run() override
    {
        test_mutex.lock();

        mutex.lock();
        for (int i = 0; i < iterations; ++i) {
            cond.wakeOne();
            cond.wait(&mutex);
        }
        mutex.unlock();

        test_mutex.unlock();
    }
};

class rmutex_Thread : public QThread
{
public:
    QMutex mutex;
    QWaitCondition cond;

    QRecursiveMutex &test_mutex;

    inline rmutex_Thread(QRecursiveMutex &m) : test_mutex(m) { }

    void run() override
    {
        test_mutex.lock();
        test_mutex.lock();
        test_mutex.lock();
        test_mutex.lock();

        mutex.lock();
        for (int i = 0; i < iterations; ++i) {
            cond.wakeOne();
            cond.wait(&mutex);
        }
        mutex.unlock();

        test_mutex.unlock();
        test_mutex.unlock();
        test_mutex.unlock();
        test_mutex.unlock();
    }
};

void tst_QMutex::lock_unlock_locked_tryLock()
{
    // normal mutex
    QMutex mutex;
    mutex_Thread thread(mutex);

    QRecursiveMutex rmutex;
    rmutex_Thread rthread(rmutex);

    for (int i = 0; i < iterations; ++i) {
        // normal mutex
        QVERIFY(mutex.tryLock());
        mutex.unlock();

        thread.mutex.lock();
        thread.start();

        for (int j = 0; j < iterations; ++j) {
            QVERIFY(thread.cond.wait(&thread.mutex, 10000));
            QVERIFY(!mutex.tryLock());

            thread.cond.wakeOne();
        }

        thread.mutex.unlock();

        QVERIFY(thread.wait(10000));
        QVERIFY(mutex.tryLock());

        mutex.unlock();

        // recursive mutex
        QVERIFY(rmutex.tryLock());
        QVERIFY(rmutex.tryLock());
        QVERIFY(rmutex.tryLock());
        QVERIFY(rmutex.tryLock());

        rmutex.unlock();
        rmutex.unlock();
        rmutex.unlock();
        rmutex.unlock();

        rthread.mutex.lock();
        rthread.start();

        for (int k = 0; k < iterations; ++k) {
            QVERIFY(rthread.cond.wait(&rthread.mutex, 10000));
            QVERIFY(!rmutex.tryLock());

            rthread.cond.wakeOne();
        }

        rthread.mutex.unlock();

        QVERIFY(rthread.wait(10000));
        QVERIFY(rmutex.tryLock());
        QVERIFY(rmutex.tryLock());
        QVERIFY(rmutex.tryLock());
        QVERIFY(rmutex.tryLock());

        rmutex.unlock();
        rmutex.unlock();
        rmutex.unlock();
        rmutex.unlock();
    }
}

constexpr int one_minute = 6 * 1000; // not really one minute, but else it is too long.
constexpr int threadCount = 10;

class StressTestThread : public QThread
{
    QElapsedTimer t;
public:
    static QBasicAtomicInt lockCount;
    static QBasicAtomicInt sentinel;
    static QMutex mutex;
    static int errorCount;
    void start()
    {
        t.start();
        QThread::start();
    }
    void run() override
    {
        while (t.elapsed() < one_minute) {
            mutex.lock();
            if (sentinel.ref()) ++errorCount;
            if (!sentinel.deref()) ++errorCount;
            lockCount.ref();
            mutex.unlock();
            if (mutex.tryLock()) {
                if (sentinel.ref()) ++errorCount;
                if (!sentinel.deref()) ++errorCount;
                lockCount.ref();
                mutex.unlock();
            }
        }
    }
};
QMutex StressTestThread::mutex;
QBasicAtomicInt StressTestThread::lockCount = Q_BASIC_ATOMIC_INITIALIZER(0);
QBasicAtomicInt StressTestThread::sentinel = Q_BASIC_ATOMIC_INITIALIZER(-1);
int StressTestThread::errorCount = 0;

void tst_QMutex::stressTest()
{
    StressTestThread threads[threadCount];
    for (int i = 0; i < threadCount; ++i)
        threads[i].start();
    QVERIFY(threads[0].wait(one_minute + 10000));
    for (int i = 1; i < threadCount; ++i)
        QVERIFY(threads[i].wait(10000));
    QCOMPARE(StressTestThread::errorCount, 0);
    qDebug("locked %d times", int(StressTestThread::lockCount.loadRelaxed()));
}

class TryLockRaceThread : public QThread
{
public:
    static QMutex mutex;

    void run() override
    {
        QElapsedTimer t;
        t.start();
        do {
            if (mutex.tryLock())
                mutex.unlock();
        } while (t.elapsed() < one_minute/2);
    }
};
QMutex TryLockRaceThread::mutex;

void tst_QMutex::tryLockRace()
{
    // mutex not in use, should be able to lock it
    QVERIFY(TryLockRaceThread::mutex.tryLock());
    TryLockRaceThread::mutex.unlock();

    // try to break tryLock
    TryLockRaceThread thread[threadCount];
    for (int i = 0; i < threadCount; ++i)
        thread[i].start();
    for (int i = 0; i < threadCount; ++i)
        QVERIFY(thread[i].wait());

    // mutex not in use, should be able to lock it
    QVERIFY(TryLockRaceThread::mutex.tryLock());
    TryLockRaceThread::mutex.unlock();
}

// The following is a regression test for QTBUG-16115, where QMutex could
// deadlock after calling tryLock repeatedly.

// Variable that will be protected by the mutex. Volatile so that the
// the optimiser doesn't mess with it based on the increment-then-decrement
// usage pattern.
static volatile int tryLockDeadlockCounter;
// Counter for how many times the protected variable has an incorrect value.
static int tryLockDeadlockFailureCount = 0;

void tst_QMutex::tryLockDeadlock()
{
    //Used to deadlock on unix
    struct TrylockThread : QThread {
        TrylockThread(QMutex &mut) : mut(mut) {}
        QMutex &mut;
        void run() override
        {
            for (int i = 0; i < 100000; ++i) {
                if (mut.tryLock(0)) {
                    if (QtPrivate::volatilePreIncrement(tryLockDeadlockCounter) != 1)
                        ++tryLockDeadlockFailureCount;
                    if (QtPrivate::volatilePreDecrement(tryLockDeadlockCounter) != 0)
                        ++tryLockDeadlockFailureCount;
                    mut.unlock();
                }
            }
        }
    };
    QMutex mut;
    TrylockThread t1(mut);
    TrylockThread t2(mut);
    TrylockThread t3(mut);
    t1.start();
    t2.start();
    t3.start();

    for (int i = 0; i < 100000; ++i) {
        mut.lock();
        if (QtPrivate::volatilePreIncrement(tryLockDeadlockCounter) != 1)
            ++tryLockDeadlockFailureCount;
        if (QtPrivate::volatilePreDecrement(tryLockDeadlockCounter) != 0)
            ++tryLockDeadlockFailureCount;
        mut.unlock();
    }
    t1.wait();
    t2.wait();
    t3.wait();
    QCOMPARE(tryLockDeadlockFailureCount, 0);
}

void tst_QMutex::tryLockNegative_data()
{
    QTest::addColumn<int>("timeout");
    QTest::newRow("-1") << -1;
    QTest::newRow("-2") << -2;
    QTest::newRow("INT_MIN/2") << INT_MIN/2;
    QTest::newRow("INT_MIN") << INT_MIN;
}

void tst_QMutex::tryLockNegative()
{
    // the documentation says tryLock() with a negative number is the same as lock()
    struct TrylockThread : QThread {
        TrylockThread(QMutex &mut, int timeout)
            : mut(mut), timeout(timeout), tryLockResult(-1)
        {}
        QMutex &mut;
        int timeout;
        int tryLockResult;
        void run() override
        {
            tryLockResult = mut.tryLock(timeout);
            mut.unlock();
        }
    };

    QFETCH(int, timeout);

    QMutex mutex;
    TrylockThread thr(mutex, timeout);
    mutex.lock();
    thr.start();

    // the thread should have stopped in tryLock(), waiting for us to unlock
    // the mutex. The following test can be falsely positive due to timing:
    // tryLock may still fail but hasn't failed yet. But it certainly cannot be
    // a false negative: if wait() returns true, tryLock failed.
    QVERIFY(!thr.wait(200));

    // after we unlock the mutex, the thread should succeed in locking, then
    // unlock and exit. Do this before more tests to avoid deadlocking due to
    // ~QThread waiting forever on a thread that won't exit.
    mutex.unlock();

    QVERIFY(thr.wait());
    QCOMPARE(thr.tryLockResult, 1);
}


class MoreStressTestThread : public QThread
{
    QElapsedTimer t;
public:
    static QAtomicInt lockCount;
    static QAtomicInt sentinel[threadCount];
    static QMutex mutex[threadCount];
    static QAtomicInt errorCount;
    void start()
    {
        t.start();
        QThread::start();
    }
    void run() override
    {
        quint64 i = 0;
        while (t.elapsed() < one_minute) {
            i++;
            uint nb = (i * 9 + uint(lockCount.loadRelaxed()) * 13) % threadCount;
            QMutexLocker locker(&mutex[nb]);
            if (sentinel[nb].loadRelaxed()) errorCount.ref();
            if (sentinel[nb].fetchAndAddRelaxed(5)) errorCount.ref();
            if (!sentinel[nb].testAndSetRelaxed(5, 0)) errorCount.ref();
            if (sentinel[nb].loadRelaxed()) errorCount.ref();
            lockCount.ref();
            nb = (nb * 17 + i * 5 + uint(lockCount.loadRelaxed()) * 3) % threadCount;
            if (mutex[nb].tryLock()) {
                if (sentinel[nb].loadRelaxed()) errorCount.ref();
                if (sentinel[nb].fetchAndAddRelaxed(16)) errorCount.ref();
                if (!sentinel[nb].testAndSetRelaxed(16, 0)) errorCount.ref();
                if (sentinel[nb].loadRelaxed()) errorCount.ref();
                lockCount.ref();
                mutex[nb].unlock();
            }
            nb = (nb * 15 + i * 47 + uint(lockCount.loadRelaxed()) * 31) % threadCount;
            if (mutex[nb].tryLock(2)) {
                if (sentinel[nb].loadRelaxed()) errorCount.ref();
                if (sentinel[nb].fetchAndAddRelaxed(53)) errorCount.ref();
                if (!sentinel[nb].testAndSetRelaxed(53, 0)) errorCount.ref();
                if (sentinel[nb].loadRelaxed()) errorCount.ref();
                lockCount.ref();
                mutex[nb].unlock();
            }
        }
    }
};
QMutex MoreStressTestThread::mutex[threadCount];
QAtomicInt MoreStressTestThread::lockCount;
QAtomicInt MoreStressTestThread::sentinel[threadCount];
QAtomicInt MoreStressTestThread::errorCount = 0;

void tst_QMutex::moreStress()
{
    QVarLengthArray<MoreStressTestThread, threadCount> threads(qMin(QThread::idealThreadCount(),
                                                                    int(threadCount)));
    for (auto &thread : threads)
        thread.start();
    QVERIFY(threads[0].wait(one_minute + 10000));
    for (auto &thread : threads)
        QVERIFY(thread.wait(10000));
    qDebug("locked %d times", MoreStressTestThread::lockCount.loadRelaxed());
    QCOMPARE(MoreStressTestThread::errorCount.loadRelaxed(), 0);
}


QTEST_MAIN(tst_QMutex)
#include "tst_qmutex.moc"
