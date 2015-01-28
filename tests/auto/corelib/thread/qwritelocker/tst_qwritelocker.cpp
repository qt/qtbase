/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

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
        void run()
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
    thread = 0;
}


void tst_QWriteLocker::unlockAndRelockTest()
{
    class UnlockAndRelockThread : public tst_QWriteLockerThread
    {
    public:
        void run()
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
    thread = 0;
}

void tst_QWriteLocker::lockerStateTest()
{
    class LockerStateThread : public tst_QWriteLockerThread
    {
    public:
        void run()
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
    thread = 0;
}

QTEST_MAIN(tst_QWriteLocker)
#include "tst_qwritelocker.moc"
