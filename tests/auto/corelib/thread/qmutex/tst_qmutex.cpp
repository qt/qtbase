/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include <qatomic.h>
#include <qcoreapplication.h>
#include <qdatetime.h>
#include <qmutex.h>
#include <qthread.h>
#include <qwaitcondition.h>

class tst_QMutex : public QObject
{
    Q_OBJECT
private slots:
    void tryLock();
    void lock_unlock_locked_tryLock();
    void stressTest();
    void tryLockRace();
    void tryLockDeadlock();
    void tryLockNegative_data();
    void tryLockNegative();
    void moreStress();
};

static const int iterations = 100;

QAtomicInt lockCount(0);
QMutex normalMutex, recursiveMutex(QMutex::Recursive);
QSemaphore testsTurn;
QSemaphore threadsTurn;

enum { waitTime = 100 };

void tst_QMutex::tryLock()
{
    // test non-recursive mutex
    {
        class Thread : public QThread
        {
        public:
            void run()
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
                QTime timer;
                timer.start();
                QVERIFY(!normalMutex.tryLock(waitTime));
                QVERIFY(timer.elapsed() >= waitTime);
                testsTurn.release();

                // TEST 4: thread can acquire lock, timeout = waitTime
                threadsTurn.acquire();
                timer.start();
                QVERIFY(normalMutex.tryLock(waitTime));
                QVERIFY(timer.elapsed() <= waitTime);
                QVERIFY(lockCount.testAndSetRelaxed(0, 1));
                timer.start();
                // it's non-recursive, so the following lock needs to fail
                QVERIFY(!normalMutex.tryLock(waitTime));
                QVERIFY(timer.elapsed() >= waitTime);
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
                QVERIFY(timer.elapsed() < waitTime);
                QVERIFY(lockCount.testAndSetRelaxed(0, 1));
                QVERIFY(!normalMutex.tryLock(0));
                QVERIFY(lockCount.testAndSetRelaxed(1, 0));
                normalMutex.unlock();
                testsTurn.release();

                // TEST 7 overflow: thread can acquire lock, timeout = 3000 (QTBUG-24795)
                threadsTurn.acquire();
                timer.start();
                QVERIFY(normalMutex.tryLock(3000));
                QVERIFY(timer.elapsed() < 3000);
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
        QThread::msleep(100);
        normalMutex.unlock();

        // wait for thread to finish
        testsTurn.acquire();
        threadsTurn.release();
        thread.wait();
    }

    // test recursive mutex
    {
        class Thread : public QThread
        {
        public:
            void run()
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
                QTime timer;
                timer.start();
                QVERIFY(!recursiveMutex.tryLock(waitTime));
                QVERIFY(timer.elapsed() >= waitTime);
                QVERIFY(!recursiveMutex.tryLock(0));
                testsTurn.release();

                threadsTurn.acquire();
                timer.start();
                QVERIFY(recursiveMutex.tryLock(waitTime));
                QVERIFY(timer.elapsed() <= waitTime);
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
                QVERIFY(timer.elapsed() < waitTime);
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
}

class mutex_Thread : public QThread
{
public:
    QMutex mutex;
    QWaitCondition cond;

    QMutex &test_mutex;

    inline mutex_Thread(QMutex &m) : test_mutex(m) { }

    void run()
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

    QMutex &test_mutex;

    inline rmutex_Thread(QMutex &m) : test_mutex(m) { }

    void run()
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

    QMutex rmutex(QMutex::Recursive);
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

enum { one_minute = 6 * 1000, //not really one minute, but else it is too long.
       threadCount = 10 };

class StressTestThread : public QThread
{
    QTime t;
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
    void run()
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
    qDebug("locked %d times", int(StressTestThread::lockCount.load()));
}

class TryLockRaceThread : public QThread
{
public:
    static QMutex mutex;

    void run()
    {
        QTime t;
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
        void run() {
            for (int i = 0; i < 100000; ++i) {
                if (mut.tryLock(0)) {
                    if ((++tryLockDeadlockCounter) != 1)
                        ++tryLockDeadlockFailureCount;
                    if ((--tryLockDeadlockCounter) != 0)
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
        if ((++tryLockDeadlockCounter) != 1)
            ++tryLockDeadlockFailureCount;
        if ((--tryLockDeadlockCounter) != 0)
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
        void run() {
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
    QTime t;
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
    void run()
    {
        quint64 i = 0;
        while (t.elapsed() < one_minute) {
            i++;
            uint nb = (i * 9 + lockCount.load() * 13) % threadCount;
            QMutexLocker locker(&mutex[nb]);
            if (sentinel[nb].load()) errorCount.ref();
            if (sentinel[nb].fetchAndAddRelaxed(5)) errorCount.ref();
            if (!sentinel[nb].testAndSetRelaxed(5, 0)) errorCount.ref();
            if (sentinel[nb].load()) errorCount.ref();
            lockCount.ref();
            nb = (nb * 17 + i * 5 + lockCount.load() * 3) % threadCount;
            if (mutex[nb].tryLock()) {
                if (sentinel[nb].load()) errorCount.ref();
                if (sentinel[nb].fetchAndAddRelaxed(16)) errorCount.ref();
                if (!sentinel[nb].testAndSetRelaxed(16, 0)) errorCount.ref();
                if (sentinel[nb].load()) errorCount.ref();
                lockCount.ref();
                mutex[nb].unlock();
            }
            nb = (nb * 15 + i * 47 + lockCount.load() * 31) % threadCount;
            if (mutex[nb].tryLock(2)) {
                if (sentinel[nb].load()) errorCount.ref();
                if (sentinel[nb].fetchAndAddRelaxed(53)) errorCount.ref();
                if (!sentinel[nb].testAndSetRelaxed(53, 0)) errorCount.ref();
                if (sentinel[nb].load()) errorCount.ref();
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
    MoreStressTestThread threads[threadCount];
    for (int i = 0; i < threadCount; ++i)
        threads[i].start();
    QVERIFY(threads[0].wait(one_minute + 10000));
    for (int i = 1; i < threadCount; ++i)
        QVERIFY(threads[i].wait(10000));
    qDebug("locked %d times", MoreStressTestThread::lockCount.load());
    QCOMPARE(MoreStressTestThread::errorCount.load(), 0);
}


QTEST_MAIN(tst_QMutex)
#include "tst_qmutex.moc"
