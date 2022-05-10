// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>

#include <QCoreApplication>
#include <QWriteLocker>
#include <QSemaphore>
#include <QThread>

class tst_QWriteLockerThread : public QThread
{
public:
    QReadWriteLock lock;
    QSemaphore semaphore, testSemaphore;

    void waitForTest()
    {
        semaphore.release();
        testSemaphore.acquire();
    }
};

class tst_QWriteLocker : public QObject
{
    Q_OBJECT

public:
    tst_QWriteLockerThread *thread;

    void waitForThread()
    {
        thread->semaphore.acquire();
    }
    void releaseThread()
    {
        thread->testSemaphore.release();
    }

private slots:
    void scopeTest();
    void unlockAndRelockTest();
    void lockerStateTest();
};

void tst_QWriteLocker::scopeTest()
{
    class ScopeTestThread : public tst_QWriteLockerThread
    {
    public:
        void run() override
        {
            waitForTest();

            {
                QWriteLocker locker(&lock);
                waitForTest();
            }

            waitForTest();
        }
    };

    thread = new ScopeTestThread;
    thread->start();

    waitForThread();
    // lock should be unlocked before entering the scope that creates the QWriteLocker
    QVERIFY(thread->lock.tryLockForWrite());
    thread->lock.unlock();
    releaseThread();

    waitForThread();
    // lock should be locked by the QWriteLocker
    QVERIFY(!thread->lock.tryLockForWrite());
    releaseThread();

    waitForThread();
    // lock should be unlocked when the QWriteLocker goes out of scope
    QVERIFY(thread->lock.tryLockForWrite());
    thread->lock.unlock();
    releaseThread();

    QVERIFY(thread->wait());

    delete thread;
    thread = nullptr;
}


void tst_QWriteLocker::unlockAndRelockTest()
{
    class UnlockAndRelockThread : public tst_QWriteLockerThread
    {
    public:
        void run() override
        {
            QWriteLocker locker(&lock);

            waitForTest();

            locker.unlock();

            waitForTest();

            locker.relock();

            waitForTest();
        }
    };

    thread = new UnlockAndRelockThread;
    thread->start();

    waitForThread();
    // lock should be locked by the QWriteLocker
    QVERIFY(!thread->lock.tryLockForWrite());
    releaseThread();

    waitForThread();
    // lock has been explicitly unlocked via QWriteLocker
    QVERIFY(thread->lock.tryLockForWrite());
    thread->lock.unlock();
    releaseThread();

    waitForThread();
    // lock has been explicitly relocked via QWriteLocker
    QVERIFY(!thread->lock.tryLockForWrite());
    releaseThread();

    QVERIFY(thread->wait());

    delete thread;
    thread = nullptr;
}

void tst_QWriteLocker::lockerStateTest()
{
    class LockerStateThread : public tst_QWriteLockerThread
    {
    public:
        void run() override
        {
            {
                QWriteLocker locker(&lock);
                locker.relock();
                locker.unlock();

                waitForTest();
            }

            waitForTest();
        }
    };

    thread = new LockerStateThread;
    thread->start();

    waitForThread();
    // even though we relock() after creating the QWriteLocker, it shouldn't lock the lock more than once
    QVERIFY(thread->lock.tryLockForWrite());
    thread->lock.unlock();
    releaseThread();

    waitForThread();
    // if we call QWriteLocker::unlock(), its destructor should do nothing
    QVERIFY(thread->lock.tryLockForWrite());
    thread->lock.unlock();
    releaseThread();

    QVERIFY(thread->wait());

    delete thread;
    thread = nullptr;
}

QTEST_MAIN(tst_QWriteLocker)
#include "tst_qwritelocker.moc"
