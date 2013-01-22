/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qmutex.h>
#include <qthread.h>
#include <qwaitcondition.h>

#define COND_WAIT_TIME 1

class tst_QWaitCondition : public QObject
{
    Q_OBJECT
private slots:
    void wait_QMutex();
    void wait_QReadWriteLock();
    void wakeOne();
    void wakeAll();
    void wait_RaceCondition();
};

static const int iterations = 4;
static const int ThreadCount = 4;

class wait_QMutex_Thread_1 : public QThread
{
public:
    QMutex mutex;
    QWaitCondition cond;

    inline wait_QMutex_Thread_1()
    { }

    void run()
    {
        mutex.lock();
        cond.wakeOne();
        cond.wait(&mutex);
        mutex.unlock();
    }
};

class wait_QMutex_Thread_2 : public QThread
{
public:
    QWaitCondition started;

    QMutex *mutex;
    QWaitCondition *cond;

    inline wait_QMutex_Thread_2()
    : mutex(0), cond(0)
    { }

    void run()
    {
        mutex->lock();
        started.wakeOne();
        cond->wait(mutex);
        mutex->unlock();
    }
};

class wait_QReadWriteLock_Thread_1 : public QThread
{
public:
    QReadWriteLock readWriteLock;
    QWaitCondition cond;

    inline wait_QReadWriteLock_Thread_1()
    { }

    void run()
    {
        readWriteLock.lockForWrite();
        cond.wakeOne();
        cond.wait(&readWriteLock);
        readWriteLock.unlock();
    }
};

class wait_QReadWriteLock_Thread_2 : public QThread
{
public:
    QWaitCondition started;

    QReadWriteLock *readWriteLock;
    QWaitCondition *cond;

    inline wait_QReadWriteLock_Thread_2()
    : readWriteLock(0), cond(0)
    { }

    void run()
    {
        readWriteLock->lockForRead();
        started.wakeOne();
        cond->wait(readWriteLock);
        readWriteLock->unlock();
    }
};

void tst_QWaitCondition::wait_QMutex()
{
    int x;
    for (int i = 0; i < iterations; ++i) {
        {
            QMutex mutex;
            QWaitCondition cond;

            mutex.lock();

            cond.wakeOne();
            QVERIFY(!cond.wait(&mutex, 1));

            cond.wakeAll();
            QVERIFY(!cond.wait(&mutex, 1));

            mutex.unlock();
        }

        {
            // test multiple threads waiting on separate wait conditions
            wait_QMutex_Thread_1 thread[ThreadCount];

            for (x = 0; x < ThreadCount; ++x) {
                thread[x].mutex.lock();
                thread[x].start();
                // wait for thread to start
                QVERIFY(thread[x].cond.wait(&thread[x].mutex, 1000));
                thread[x].mutex.unlock();
            }

            for (x = 0; x < ThreadCount; ++x) {
                QVERIFY(thread[x].isRunning());
                QVERIFY(!thread[x].isFinished());
            }

            for (x = 0; x < ThreadCount; ++x) {
                thread[x].mutex.lock();
                thread[x].cond.wakeOne();
                thread[x].mutex.unlock();
            }

            for (x = 0; x < ThreadCount; ++x) {
                QVERIFY(thread[x].wait(1000));
            }
        }

        {
            // test multiple threads waiting on a wait condition
            QMutex mutex;
            QWaitCondition cond1, cond2;
            wait_QMutex_Thread_2 thread[ThreadCount];

            mutex.lock();
            for (x = 0; x < ThreadCount; ++x) {
                thread[x].mutex = &mutex;
                thread[x].cond = (x < ThreadCount / 2) ? &cond1 : &cond2;
                thread[x].start();
                // wait for thread to start
                QVERIFY(thread[x].started.wait(&mutex, 1000));
            }
            mutex.unlock();

            for (x = 0; x < ThreadCount; ++x) {
                QVERIFY(thread[x].isRunning());
                QVERIFY(!thread[x].isFinished());
            }

            mutex.lock();
            cond1.wakeAll();
            cond2.wakeAll();
            mutex.unlock();

            for (x = 0; x < ThreadCount; ++x) {
                QVERIFY(thread[x].wait(1000));
            }
        }
    }
}

void tst_QWaitCondition::wait_QReadWriteLock()
{
    {
        QReadWriteLock readWriteLock(QReadWriteLock::Recursive);
        QWaitCondition waitCondition;

        // ensure that the lockForRead is correctly restored
        readWriteLock.lockForRead();

        QVERIFY(!waitCondition.wait(&readWriteLock, 1));

        QVERIFY(!readWriteLock.tryLockForWrite());
        QVERIFY(readWriteLock.tryLockForRead());
        readWriteLock.unlock();
        QVERIFY(!readWriteLock.tryLockForWrite());
        readWriteLock.unlock();

        QVERIFY(readWriteLock.tryLockForWrite());
        readWriteLock.unlock();
    }

    {
        QReadWriteLock readWriteLock(QReadWriteLock::Recursive);
        QWaitCondition waitCondition;

        // ensure that the lockForWrite is correctly restored
        readWriteLock.lockForWrite();

        QVERIFY(!waitCondition.wait(&readWriteLock, 1));

        QVERIFY(!readWriteLock.tryLockForRead());
        QVERIFY(readWriteLock.tryLockForWrite());
        readWriteLock.unlock();
        QVERIFY(!readWriteLock.tryLockForRead());
        readWriteLock.unlock();

        QVERIFY(readWriteLock.tryLockForRead());
        readWriteLock.unlock();
    }


    int x;
    for (int i = 0; i < iterations; ++i) {
        {
            QReadWriteLock readWriteLock;
            QWaitCondition waitCondition;

            readWriteLock.lockForRead();

            waitCondition.wakeOne();
            QVERIFY(!waitCondition.wait(&readWriteLock, 1));

            waitCondition.wakeAll();
            QVERIFY(!waitCondition.wait(&readWriteLock, 1));

            readWriteLock.unlock();
        }

        {
            QReadWriteLock readWriteLock;
            QWaitCondition waitCondition;

            readWriteLock.lockForWrite();

            waitCondition.wakeOne();
            QVERIFY(!waitCondition.wait(&readWriteLock, 1));

            waitCondition.wakeAll();
            QVERIFY(!waitCondition.wait(&readWriteLock, 1));

            readWriteLock.unlock();
        }

        {
            // test multiple threads waiting on separate wait conditions
            wait_QReadWriteLock_Thread_1 thread[ThreadCount];

            for (x = 0; x < ThreadCount; ++x) {
                thread[x].readWriteLock.lockForRead();
                thread[x].start();
                // wait for thread to start
                QVERIFY(thread[x].cond.wait(&thread[x].readWriteLock, 1000));
                thread[x].readWriteLock.unlock();
            }

            for (x = 0; x < ThreadCount; ++x) {
                QVERIFY(thread[x].isRunning());
                QVERIFY(!thread[x].isFinished());
            }

            for (x = 0; x < ThreadCount; ++x) {
                thread[x].readWriteLock.lockForRead();
                thread[x].cond.wakeOne();
                thread[x].readWriteLock.unlock();
            }

            for (x = 0; x < ThreadCount; ++x) {
                QVERIFY(thread[x].wait(1000));
            }
        }

        {
            // test multiple threads waiting on a wait condition
            QReadWriteLock readWriteLock;
            QWaitCondition cond1, cond2;
            wait_QReadWriteLock_Thread_2 thread[ThreadCount];

            readWriteLock.lockForWrite();
            for (x = 0; x < ThreadCount; ++x) {
                thread[x].readWriteLock = &readWriteLock;
                thread[x].cond = (x < ThreadCount / 2) ? &cond1 : &cond2;
                thread[x].start();
                // wait for thread to start
                QVERIFY(thread[x].started.wait(&readWriteLock, 1000));
            }
            readWriteLock.unlock();

            for (x = 0; x < ThreadCount; ++x) {
                QVERIFY(thread[x].isRunning());
                QVERIFY(!thread[x].isFinished());
            }

            readWriteLock.lockForWrite();
            cond1.wakeAll();
            cond2.wakeAll();
            readWriteLock.unlock();

            for (x = 0; x < ThreadCount; ++x) {
                QVERIFY(thread[x].wait(1000));
            }
        }
    }
}

class wake_Thread : public QThread
{
public:
    static int count;

    QWaitCondition started;
    QWaitCondition dummy;

    QMutex *mutex;
    QWaitCondition *cond;

    inline wake_Thread()
    : mutex(0), cond(0)
    { }

    static inline void sleep(ulong s)
    { QThread::sleep(s); }

    void run()
    {
        mutex->lock();
        ++count;
        dummy.wakeOne(); // this wakeup should be lost
        started.wakeOne();
        dummy.wakeAll(); // this one too
        cond->wait(mutex);
        --count;
        mutex->unlock();
    }
};

int wake_Thread::count = 0;

class wake_Thread_2 : public QThread
{
public:
    static int count;

    QWaitCondition started;
    QWaitCondition dummy;

    QReadWriteLock *readWriteLock;
    QWaitCondition *cond;

    inline wake_Thread_2()
    : readWriteLock(0), cond(0)
    { }

    static inline void sleep(ulong s)
    { QThread::sleep(s); }

    void run()
    {
        readWriteLock->lockForWrite();
        ++count;
        dummy.wakeOne(); // this wakeup should be lost
        started.wakeOne();
        dummy.wakeAll(); // this one too
        cond->wait(readWriteLock);
        --count;
        readWriteLock->unlock();
    }
};

int wake_Thread_2::count = 0;

void tst_QWaitCondition::wakeOne()
{
    int x;
    // wake up threads, one at a time
    for (int i = 0; i < iterations; ++i) {
        QMutex mutex;
        QWaitCondition cond;

        // QMutex
        wake_Thread thread[ThreadCount];
        bool thread_exited[ThreadCount];

        mutex.lock();
        for (x = 0; x < ThreadCount; ++x) {
            thread[x].mutex = &mutex;
            thread[x].cond = &cond;
            thread_exited[x] = false;
            thread[x].start();
            // wait for thread to start
            QVERIFY(thread[x].started.wait(&mutex, 1000));
            // make sure wakeups are not queued... if nothing is
            // waiting at the time of the wakeup, nothing happens
            QVERIFY(!thread[x].dummy.wait(&mutex, 1));
        }
        mutex.unlock();

        QCOMPARE(wake_Thread::count, ThreadCount);

        // wake up threads one at a time
        for (x = 0; x < ThreadCount; ++x) {
            mutex.lock();
            cond.wakeOne();
            QVERIFY(!cond.wait(&mutex, COND_WAIT_TIME));
            QVERIFY(!thread[x].dummy.wait(&mutex, 1));
            mutex.unlock();

            int exited = 0;
            for (int y = 0; y < ThreadCount; ++y) {
                if (thread_exited[y])
                    continue;
                if (thread[y].wait(exited > 0 ? 10 : 1000)) {
                    thread_exited[y] = true;
                    ++exited;
                }
            }

            QCOMPARE(exited, 1);
            QCOMPARE(wake_Thread::count, ThreadCount - (x + 1));
        }

        QCOMPARE(wake_Thread::count, 0);

        // QReadWriteLock
        QReadWriteLock readWriteLock;
        wake_Thread_2 rwthread[ThreadCount];

        readWriteLock.lockForWrite();
        for (x = 0; x < ThreadCount; ++x) {
            rwthread[x].readWriteLock = &readWriteLock;
            rwthread[x].cond = &cond;
            thread_exited[x] = false;
            rwthread[x].start();
            // wait for thread to start
            QVERIFY(rwthread[x].started.wait(&readWriteLock, 1000));
            // make sure wakeups are not queued... if nothing is
            // waiting at the time of the wakeup, nothing happens
            QVERIFY(!rwthread[x].dummy.wait(&readWriteLock, 1));
        }
        readWriteLock.unlock();

        QCOMPARE(wake_Thread_2::count, ThreadCount);

        // wake up threads one at a time
        for (x = 0; x < ThreadCount; ++x) {
            readWriteLock.lockForWrite();
            cond.wakeOne();
            QVERIFY(!cond.wait(&readWriteLock, COND_WAIT_TIME));
            QVERIFY(!rwthread[x].dummy.wait(&readWriteLock, 1));
            readWriteLock.unlock();

            int exited = 0;
            for (int y = 0; y < ThreadCount; ++y) {
                if (thread_exited[y])
                    continue;
                if (rwthread[y].wait(exited > 0 ? 10 : 1000)) {
                    thread_exited[y] = true;
                    ++exited;
                }
            }

            QCOMPARE(exited, 1);
            QCOMPARE(wake_Thread_2::count, ThreadCount - (x + 1));
        }

        QCOMPARE(wake_Thread_2::count, 0);
    }

    // wake up threads, two at a time
    for (int i = 0; i < iterations; ++i) {
        QMutex mutex;
        QWaitCondition cond;

        // QMutex
        wake_Thread thread[ThreadCount];
        bool thread_exited[ThreadCount];

        mutex.lock();
        for (x = 0; x < ThreadCount; ++x) {
            thread[x].mutex = &mutex;
            thread[x].cond = &cond;
            thread_exited[x] = false;
            thread[x].start();
            // wait for thread to start
            QVERIFY(thread[x].started.wait(&mutex, 1000));
            // make sure wakeups are not queued... if nothing is
            // waiting at the time of the wakeup, nothing happens
            QVERIFY(!thread[x].dummy.wait(&mutex, 1));
        }
        mutex.unlock();

        QCOMPARE(wake_Thread::count, ThreadCount);

        // wake up threads one at a time
        for (x = 0; x < ThreadCount; x += 2) {
            mutex.lock();
            cond.wakeOne();
            cond.wakeOne();
            QVERIFY(!cond.wait(&mutex, COND_WAIT_TIME));
            QVERIFY(!thread[x].dummy.wait(&mutex, 1));
            QVERIFY(!thread[x + 1].dummy.wait(&mutex, 1));
            mutex.unlock();

            int exited = 0;
            for (int y = 0; y < ThreadCount; ++y) {
                if (thread_exited[y])
                    continue;
                if (thread[y].wait(exited > 0 ? 10 : 1000)) {
                    thread_exited[y] = true;
                    ++exited;
                }
            }

            QCOMPARE(exited, 2);
            QCOMPARE(wake_Thread::count, ThreadCount - (x + 2));
        }

        QCOMPARE(wake_Thread::count, 0);

        // QReadWriteLock
        QReadWriteLock readWriteLock;
        wake_Thread_2 rwthread[ThreadCount];

        readWriteLock.lockForWrite();
        for (x = 0; x < ThreadCount; ++x) {
            rwthread[x].readWriteLock = &readWriteLock;
            rwthread[x].cond = &cond;
            thread_exited[x] = false;
            rwthread[x].start();
            // wait for thread to start
            QVERIFY(rwthread[x].started.wait(&readWriteLock, 1000));
            // make sure wakeups are not queued... if nothing is
            // waiting at the time of the wakeup, nothing happens
            QVERIFY(!rwthread[x].dummy.wait(&readWriteLock, 1));
        }
        readWriteLock.unlock();

        QCOMPARE(wake_Thread_2::count, ThreadCount);

        // wake up threads one at a time
        for (x = 0; x < ThreadCount; x += 2) {
            readWriteLock.lockForWrite();
            cond.wakeOne();
            cond.wakeOne();
            QVERIFY(!cond.wait(&readWriteLock, COND_WAIT_TIME));
            QVERIFY(!rwthread[x].dummy.wait(&readWriteLock, 1));
            QVERIFY(!rwthread[x + 1].dummy.wait(&readWriteLock, 1));
            readWriteLock.unlock();

            int exited = 0;
            for (int y = 0; y < ThreadCount; ++y) {
                if (thread_exited[y])
                    continue;
                if (rwthread[y].wait(exited > 0 ? 10 : 1000)) {
                    thread_exited[y] = true;
                    ++exited;
                }
            }

            QCOMPARE(exited, 2);
            QCOMPARE(wake_Thread_2::count, ThreadCount - (x + 2));
        }

        QCOMPARE(wake_Thread_2::count, 0);
    }
}

void tst_QWaitCondition::wakeAll()
{
    int x;
    for (int i = 0; i < iterations; ++i) {
        QMutex mutex;
        QWaitCondition cond;

        // QMutex
        wake_Thread thread[ThreadCount];

        mutex.lock();
        for (x = 0; x < ThreadCount; ++x) {
            thread[x].mutex = &mutex;
            thread[x].cond = &cond;
            thread[x].start();
            // wait for thread to start
            QVERIFY(thread[x].started.wait(&mutex, 1000));
        }
        mutex.unlock();

        QCOMPARE(wake_Thread::count, ThreadCount);

        // wake up all threads at once
        mutex.lock();
        cond.wakeAll();
        QVERIFY(!cond.wait(&mutex, COND_WAIT_TIME));
        mutex.unlock();

        int exited = 0;
        for (x = 0; x < ThreadCount; ++x) {
            if (thread[x].wait(1000))
            ++exited;
        }

        QCOMPARE(exited, ThreadCount);
        QCOMPARE(wake_Thread::count, 0);

        // QReadWriteLock
        QReadWriteLock readWriteLock;
        wake_Thread_2 rwthread[ThreadCount];

        readWriteLock.lockForWrite();
        for (x = 0; x < ThreadCount; ++x) {
            rwthread[x].readWriteLock = &readWriteLock;
            rwthread[x].cond = &cond;
            rwthread[x].start();
            // wait for thread to start
            QVERIFY(rwthread[x].started.wait(&readWriteLock, 1000));
        }
        readWriteLock.unlock();

        QCOMPARE(wake_Thread_2::count, ThreadCount);

        // wake up all threads at once
        readWriteLock.lockForWrite();
        cond.wakeAll();
        QVERIFY(!cond.wait(&readWriteLock, COND_WAIT_TIME));
        readWriteLock.unlock();

        exited = 0;
        for (x = 0; x < ThreadCount; ++x) {
            if (rwthread[x].wait(1000))
            ++exited;
        }

        QCOMPARE(exited, ThreadCount);
        QCOMPARE(wake_Thread_2::count, 0);
    }
}

class wait_RaceConditionThread : public QThread
{
public:
    wait_RaceConditionThread(QMutex *mutex, QWaitCondition *startup, QWaitCondition *waitCondition,
                             ulong timeout = ULONG_MAX)
        : timeout(timeout), returnValue(false), ready(false),
          mutex(mutex), startup(startup), waitCondition(waitCondition) {}

    unsigned long timeout;
    bool returnValue;

    bool ready;

    QMutex *mutex;
    QWaitCondition *startup;
    QWaitCondition *waitCondition;

    void run() {
        mutex->lock();

        ready = true;
        startup->wakeOne();

        returnValue = waitCondition->wait(mutex, timeout);

        mutex->unlock();
    }
};

class wait_RaceConditionThread_2 : public QThread
{
public:
    wait_RaceConditionThread_2(QReadWriteLock *readWriteLock,
                               QWaitCondition *startup,
                               QWaitCondition *waitCondition,
                               ulong timeout = ULONG_MAX)
        : timeout(timeout), returnValue(false), ready(false),
          readWriteLock(readWriteLock), startup(startup), waitCondition(waitCondition)
    { }

    unsigned long timeout;
    bool returnValue;

    bool ready;

    QReadWriteLock *readWriteLock;
    QWaitCondition *startup;
    QWaitCondition *waitCondition;

    void run() {
        readWriteLock->lockForWrite();

        ready = true;
        startup->wakeOne();

        returnValue = waitCondition->wait(readWriteLock, timeout);

        readWriteLock->unlock();
    }
};

void tst_QWaitCondition::wait_RaceCondition()
{
    {
        QMutex mutex;
        QWaitCondition startup;
        QWaitCondition waitCondition;

        wait_RaceConditionThread timeoutThread(&mutex, &startup, &waitCondition, 1000),
            waitingThread1(&mutex, &startup, &waitCondition);

        timeoutThread.start();
        waitingThread1.start();
        mutex.lock();

        // wait for the threads to start up
        while (!timeoutThread.ready
               || !waitingThread1.ready) {
            startup.wait(&mutex);
        }

        QTest::qWait(2000);

        waitCondition.wakeOne();

        mutex.unlock();

        QVERIFY(timeoutThread.wait(5000));
        QVERIFY(!timeoutThread.returnValue);
        QVERIFY(waitingThread1.wait(5000));
        QVERIFY(waitingThread1.returnValue);
    }

    {
        QReadWriteLock readWriteLock;
        QWaitCondition startup;
        QWaitCondition waitCondition;

        wait_RaceConditionThread_2 timeoutThread(&readWriteLock, &startup, &waitCondition, 1000),
            waitingThread1(&readWriteLock, &startup, &waitCondition);

        timeoutThread.start();
        waitingThread1.start();
        readWriteLock.lockForRead();

        // wait for the threads to start up
        while (!timeoutThread.ready
               || !waitingThread1.ready) {
            startup.wait(&readWriteLock);
        }

        QTest::qWait(2000);

        waitCondition.wakeOne();

        readWriteLock.unlock();

        QVERIFY(timeoutThread.wait(5000));
        QVERIFY(!timeoutThread.returnValue);
        QVERIFY(waitingThread1.wait(5000));
        QVERIFY(waitingThread1.returnValue);
    }
}

QTEST_MAIN(tst_QWaitCondition)
#include "tst_qwaitcondition.moc"
