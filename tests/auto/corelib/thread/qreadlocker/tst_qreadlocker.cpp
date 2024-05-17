// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QCoreApplication>
#include <QReadLocker>
#include <QSemaphore>
#include <QThread>

class tst_QReadLockerThread : public QThread
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

class tst_QReadLocker : public QObject
{
    Q_OBJECT

public:
    tst_QReadLockerThread *thread;

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

void tst_QReadLocker::scopeTest()
{
    class ScopeTestThread : public tst_QReadLockerThread
    {
    public:
        void run() override
        {
            waitForTest();

            {
                QReadLocker locker(&lock);
                waitForTest();
            }

            waitForTest();
        }
    };

    thread = new ScopeTestThread;
    thread->start();

    waitForThread();
    // lock should be unlocked before entering the scope that creates the QReadLocker
    QVERIFY(thread->lock.tryLockForWrite());
    thread->lock.unlock();
    releaseThread();

    waitForThread();
    // lock should be locked by the QReadLocker
    QVERIFY(!thread->lock.tryLockForWrite());
    releaseThread();

    waitForThread();
    // lock should be unlocked when the QReadLocker goes out of scope
    QVERIFY(thread->lock.tryLockForWrite());
    thread->lock.unlock();
    releaseThread();

    QVERIFY(thread->wait());

    delete thread;
    thread = nullptr;
}


void tst_QReadLocker::unlockAndRelockTest()
{
    class UnlockAndRelockThread : public tst_QReadLockerThread
    {
    public:
        void run() override
        {
            QReadLocker locker(&lock);

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
    // lock should be locked by the QReadLocker
    QVERIFY(!thread->lock.tryLockForWrite());
    releaseThread();

    waitForThread();
    // lock has been explicitly unlocked via QReadLocker
    QVERIFY(thread->lock.tryLockForWrite());
    thread->lock.unlock();
    releaseThread();

    waitForThread();
    // lock has been explicitly relocked via QReadLocker
    QVERIFY(!thread->lock.tryLockForWrite());
    releaseThread();

    QVERIFY(thread->wait());

    delete thread;
    thread = nullptr;
}

void tst_QReadLocker::lockerStateTest()
{
    class LockerStateThread : public tst_QReadLockerThread
    {
    public:
        void run() override
        {
            {
                QReadLocker locker(&lock);
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
    // even though we relock() after creating the QReadLocker, it shouldn't lock the lock more than once
    QVERIFY(thread->lock.tryLockForWrite());
    thread->lock.unlock();
    releaseThread();

    waitForThread();
    // if we call QReadLocker::unlock(), its destructor should do nothing
    QVERIFY(thread->lock.tryLockForWrite());
    thread->lock.unlock();
    releaseThread();

    QVERIFY(thread->wait());

    delete thread;
    thread = nullptr;
}

QTEST_MAIN(tst_QReadLocker)
#include "tst_qreadlocker.moc"
