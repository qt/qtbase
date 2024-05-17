// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QSemaphore>
#include <qcoreapplication.h>
#include <qreadwritelock.h>
#include <qelapsedtimer.h>
#include <qmutex.h>
#include <qthread.h>
#include <qwaitcondition.h>
#include <private/qemulationdetector_p.h>
#include <private/qvolatile_p.h>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

//on solaris, threads that loop on the release bool variable
//needs to sleep more than 1 usec.
#ifdef Q_OS_SOLARIS
# define RWTESTSLEEP usleep(10);
#else
# define RWTESTSLEEP usleep(1);
#endif

#include <stdio.h>

using namespace std::chrono_literals;

class tst_QReadWriteLock : public QObject
{
    Q_OBJECT

/*
    Singlethreaded tests
*/
private slots:
    void constructDestruct();
    void readLockUnlock();
    void writeLockUnlock();
    void readLockUnlockLoop();
    void writeLockUnlockLoop();
    void readLockLoop();
    void writeLockLoop();
    void readWriteLockUnlockLoop();
    void tryReadLock();
    void tryWriteLock();

/*
    Multithreaded tests
*/
private slots:
    void readLockBlockRelease();
    void writeLockBlockRelease();
    void multipleReadersBlockRelease();
    void multipleReadersLoop();
    void multipleWritersLoop();
    void multipleReadersWritersLoop();
    void countingTest();
    void limitedReaders();
    void deleteOnUnlock();

/*
    Performance tests
*/
private slots:
    void uncontendedLocks();

    // recursive locking tests
    void recursiveReadLock();
    void recursiveWriteLock();
};

void tst_QReadWriteLock::constructDestruct()
{
    {
        QReadWriteLock rwlock;
    }
}

void tst_QReadWriteLock::readLockUnlock()
{
     QReadWriteLock rwlock;
     rwlock.lockForRead();
     rwlock.unlock();
}

void tst_QReadWriteLock::writeLockUnlock()
{
     QReadWriteLock rwlock;
     rwlock.lockForWrite();
     rwlock.unlock();
}

void tst_QReadWriteLock::readLockUnlockLoop()
{
    QReadWriteLock rwlock;
    int runs=10000;
    int i;
    for (i=0; i<runs; ++i) {
        rwlock.lockForRead();
        rwlock.unlock();
    }
}

void tst_QReadWriteLock::writeLockUnlockLoop()
{
    QReadWriteLock rwlock;
    int runs=10000;
    int i;
    for (i=0; i<runs; ++i) {
        rwlock.lockForWrite();
        rwlock.unlock();
    }
}


void tst_QReadWriteLock::readLockLoop()
{
    QReadWriteLock rwlock;
    int runs=10000;
    int i;
    for (i=0; i<runs; ++i) {
        rwlock.lockForRead();
    }
    for (i=0; i<runs; ++i) {
        rwlock.unlock();
    }
}

void tst_QReadWriteLock::writeLockLoop()
{
    /*
        If you include this, the test should print one line
        and then block.
    */
#if 0
    QReadWriteLock rwlock;
    int runs=10000;
    int i;
    for (i=0; i<runs; ++i) {
        rwlock.lockForWrite();
        qDebug("I am going to block now.");
    }
#endif
}

void tst_QReadWriteLock::readWriteLockUnlockLoop()
{
    QReadWriteLock rwlock;
    int runs=10000;
    int i;
    for (i=0; i<runs; ++i) {
        rwlock.lockForRead();
        rwlock.unlock();
        rwlock.lockForWrite();
        rwlock.unlock();
    }

}

static QAtomicInt lockCount(0);
static QReadWriteLock readWriteLock;
static QSemaphore testsTurn;
static QSemaphore threadsTurn;


void tst_QReadWriteLock::tryReadLock()
{
    QReadWriteLock rwlock;
    QVERIFY(rwlock.tryLockForRead());
    rwlock.unlock();
    QVERIFY(rwlock.tryLockForRead());
    rwlock.unlock();

    rwlock.lockForRead();
    rwlock.lockForRead();
    QVERIFY(rwlock.tryLockForRead());
    rwlock.unlock();
    rwlock.unlock();
    rwlock.unlock();

    rwlock.lockForWrite();
    QVERIFY(!rwlock.tryLockForRead());
    rwlock.unlock();

    // functionality test
    {
        class Thread : public QThread
        {
        public:
            void run() override
            {
                testsTurn.release();

                threadsTurn.acquire();
                QVERIFY(!readWriteLock.tryLockForRead());
                testsTurn.release();

                threadsTurn.acquire();
                QVERIFY(readWriteLock.tryLockForRead());
                lockCount.ref();
                QVERIFY(readWriteLock.tryLockForRead());
                lockCount.ref();
                lockCount.deref();
                readWriteLock.unlock();
                lockCount.deref();
                readWriteLock.unlock();
                testsTurn.release();

                threadsTurn.acquire();
                QElapsedTimer timer;
                timer.start();
                QVERIFY(!readWriteLock.tryLockForRead(1000));
                QVERIFY(timer.elapsed() >= 1000);
                testsTurn.release();

                threadsTurn.acquire();
                timer.start();
                QVERIFY(readWriteLock.tryLockForRead(1000));
                QVERIFY(timer.elapsed() <= 1000);
                lockCount.ref();
                QVERIFY(readWriteLock.tryLockForRead(1000));
                lockCount.ref();
                lockCount.deref();
                readWriteLock.unlock();
                lockCount.deref();
                readWriteLock.unlock();
                testsTurn.release();

                threadsTurn.acquire();
            }
        };

        Thread thread;
        thread.start();

        testsTurn.acquire();
        readWriteLock.lockForWrite();
        QVERIFY(lockCount.testAndSetRelaxed(0, 1));
        threadsTurn.release();

        testsTurn.acquire();
        QVERIFY(lockCount.testAndSetRelaxed(1, 0));
        readWriteLock.unlock();
        threadsTurn.release();

        testsTurn.acquire();
        readWriteLock.lockForWrite();
        QVERIFY(lockCount.testAndSetRelaxed(0, 1));
        threadsTurn.release();

        testsTurn.acquire();
        QVERIFY(lockCount.testAndSetRelaxed(1, 0));
        readWriteLock.unlock();
        threadsTurn.release();

        // stop thread
        testsTurn.acquire();
        threadsTurn.release();
        thread.wait();
    }
}

void tst_QReadWriteLock::tryWriteLock()
{
    {
        QReadWriteLock rwlock;
        QVERIFY(rwlock.tryLockForWrite());
        rwlock.unlock();
        QVERIFY(rwlock.tryLockForWrite());
        rwlock.unlock();

        rwlock.lockForWrite();
        QVERIFY(!rwlock.tryLockForWrite());
        QVERIFY(!rwlock.tryLockForWrite());
        rwlock.unlock();

        rwlock.lockForRead();
        QVERIFY(!rwlock.tryLockForWrite());
        rwlock.unlock();
    }

    {
        QReadWriteLock rwlock(QReadWriteLock::Recursive);
        QVERIFY(rwlock.tryLockForWrite());
        rwlock.unlock();
        QVERIFY(rwlock.tryLockForWrite());
        rwlock.unlock();

        rwlock.lockForWrite();
        QVERIFY(rwlock.tryLockForWrite());
        QVERIFY(rwlock.tryLockForWrite());
        rwlock.unlock();
        rwlock.unlock();
        rwlock.unlock();

        rwlock.lockForRead();
        QVERIFY(!rwlock.tryLockForWrite());
        rwlock.unlock();
    }

    // functionality test
    {
        class Thread : public QThread
        {
        public:
            Thread() : failureCount(0) { }
            void run() override
            {
                testsTurn.release();

                threadsTurn.acquire();
                if (readWriteLock.tryLockForWrite())
                    failureCount++;
                testsTurn.release();

                threadsTurn.acquire();
                if (!readWriteLock.tryLockForWrite())
                    failureCount++;
                if (!lockCount.testAndSetRelaxed(0, 1))
                    failureCount++;
                if (!lockCount.testAndSetRelaxed(1, 0))
                    failureCount++;
                readWriteLock.unlock();
                testsTurn.release();

                threadsTurn.acquire();
                if (readWriteLock.tryLockForWrite(1000))
                    failureCount++;
                testsTurn.release();

                threadsTurn.acquire();
                if (!readWriteLock.tryLockForWrite(1000))
                    failureCount++;
                if (!lockCount.testAndSetRelaxed(0, 1))
                    failureCount++;
                if (!lockCount.testAndSetRelaxed(1, 0))
                    failureCount++;
                readWriteLock.unlock();
                testsTurn.release();

                threadsTurn.acquire();
            }

            int failureCount;
        };

        Thread thread;
        thread.start();

        testsTurn.acquire();
        readWriteLock.lockForRead();
        lockCount.ref();
        threadsTurn.release();

        testsTurn.acquire();
        lockCount.deref();
        readWriteLock.unlock();
        threadsTurn.release();

        testsTurn.acquire();
        readWriteLock.lockForRead();
        lockCount.ref();
        threadsTurn.release();

        testsTurn.acquire();
        lockCount.deref();
        readWriteLock.unlock();
        threadsTurn.release();

        // stop thread
        testsTurn.acquire();
        threadsTurn.release();
        thread.wait();

        QCOMPARE(thread.failureCount, 0);
    }
}

static bool threadDone;
static QAtomicInt release;

/*
    write-lock
    unlock
    set threadone
*/
class WriteLockThread : public QThread
{
public:
    QReadWriteLock &testRwlock;
    inline WriteLockThread(QReadWriteLock &l) : testRwlock(l) { }
    void run() override
    {
        testRwlock.lockForWrite();
        testRwlock.unlock();
        threadDone=true;
    }
};

/*
    read-lock
    unlock
    set threadone
*/
class ReadLockThread : public QThread
{
public:
    QReadWriteLock &testRwlock;
    inline ReadLockThread(QReadWriteLock &l) : testRwlock(l) { }
    void run() override
    {
        testRwlock.lockForRead();
        testRwlock.unlock();
        threadDone=true;
    }
};
/*
    write-lock
    wait for release==true
    unlock
*/
class WriteLockReleasableThread : public QThread
{
public:
    QReadWriteLock &testRwlock;
    inline WriteLockReleasableThread(QReadWriteLock &l) : testRwlock(l) { }
    void run() override
    {
        testRwlock.lockForWrite();
        while (release.loadRelaxed() == false) {
            RWTESTSLEEP
        }
        testRwlock.unlock();
    }
};

/*
    read-lock
    wait for release==true
    unlock
*/
class ReadLockReleasableThread : public QThread
{
public:
    QReadWriteLock &testRwlock;
    inline ReadLockReleasableThread(QReadWriteLock &l) : testRwlock(l) { }
    void run() override
    {
        testRwlock.lockForRead();
        while (release.loadRelaxed() == false) {
            RWTESTSLEEP
        }
        testRwlock.unlock();
    }
};


/*
    for(runTime msecs)
        read-lock
        msleep(holdTime msecs)
        release lock
        msleep(waitTime msecs)
*/
class ReadLockLoopThread : public QThread
{
public:
    QReadWriteLock &testRwlock;
    int runTime;
    std::chrono::milliseconds holdTime;
    std::chrono::milliseconds waitTime;
    bool print;
    QElapsedTimer t;
    inline ReadLockLoopThread(QReadWriteLock &l, int runTime, int holdTime=0, int waitTime=0, bool print=false)
    :testRwlock(l)
    ,runTime(runTime)
    ,holdTime(holdTime)
    ,waitTime(waitTime)
    ,print(print)
    { }
    void run() override
    {
        t.start();
        while (t.elapsed()<runTime)  {
            testRwlock.lockForRead();
            if(print) printf("reading\n");
            if (holdTime > 0ms) sleep(holdTime);
            testRwlock.unlock();
            if (waitTime > 0ms) sleep(waitTime);
        }
    }
};

/*
    for(runTime msecs)
        write-lock
        msleep(holdTime msecs)
        release lock
        msleep(waitTime msecs)
*/
class WriteLockLoopThread : public QThread
{
public:
    QReadWriteLock &testRwlock;
    int runTime;
    std::chrono::milliseconds holdTime;
    std::chrono::milliseconds waitTime;
    bool print;
    QElapsedTimer t;
    inline WriteLockLoopThread(QReadWriteLock &l, int runTime, int holdTime=0, int waitTime=0, bool print=false)
    :testRwlock(l)
    ,runTime(runTime)
    ,holdTime(holdTime)
    ,waitTime(waitTime)
    ,print(print)
    { }
    void run() override
    {
        t.start();
        while (t.elapsed() < runTime)  {
            testRwlock.lockForWrite();
            if (print) printf(".");
            if (holdTime > 0ms) sleep(holdTime);
            testRwlock.unlock();
            if (waitTime > 0ms) sleep(waitTime);
        }
    }
};

static volatile int count = 0;

/*
    for(runTime msecs)
        write-lock
        count to maxval
        set count to 0
        release lock
        msleep waitTime
*/
class WriteLockCountThread : public QThread
{
public:
    QReadWriteLock &testRwlock;
    int runTime;
    std::chrono::milliseconds waitTime;
    int maxval;
    QElapsedTimer t;
    inline WriteLockCountThread(QReadWriteLock &l, int runTime, int waitTime, int maxval)
    :testRwlock(l)
    ,runTime(runTime)
    ,waitTime(waitTime)
    ,maxval(maxval)
    { }
    void run() override
    {
        t.start();
        while (t.elapsed() < runTime)  {
            testRwlock.lockForWrite();
            if(count)
                qFatal("Non-zero count at start of write! (%d)",count );
//            printf(".");
            for (int i = 0; i < maxval; ++i)
                QtPrivate::volatilePreIncrement(count);
            count=0;
            testRwlock.unlock();
            sleep(waitTime);
        }
    }
};

/*
    for(runTime msecs)
        read-lock
        verify count==0
        release lock
        msleep waitTime
*/
class ReadLockCountThread : public QThread
{
public:
    QReadWriteLock &testRwlock;
    int runTime;
    std::chrono::milliseconds waitTime;
    QElapsedTimer t;
    inline ReadLockCountThread(QReadWriteLock &l, int runTime, int waitTime)
    :testRwlock(l)
    ,runTime(runTime)
    ,waitTime(waitTime)
    { }
    void run() override
    {
        t.start();
        while (t.elapsed() < runTime)  {
            testRwlock.lockForRead();
            if(count)
                qFatal("Non-zero count at Read! (%d)",count );
            testRwlock.unlock();
            sleep(waitTime);
        }
    }
};


/*
    A writer acquires a read-lock, a reader locks
    the writer releases the lock, the reader gets the lock
*/
void tst_QReadWriteLock::readLockBlockRelease()
{
    QReadWriteLock testLock;
    testLock.lockForWrite();
    threadDone=false;
    ReadLockThread rlt(testLock);
    rlt.start();
    QThread::sleep(1s);
    testLock.unlock();
    rlt.wait();
    QVERIFY(threadDone);
}

/*
    writer1 acquires a read-lock, writer2 blocks,
    writer1 releases the lock, writer2 gets the lock
*/
void tst_QReadWriteLock::writeLockBlockRelease()
{
    QReadWriteLock testLock;
    testLock.lockForWrite();
    threadDone=false;
    WriteLockThread wlt(testLock);
    wlt.start();
    QThread::sleep(1s);
    testLock.unlock();
    wlt.wait();
    QVERIFY(threadDone);
}
/*
    Two readers acquire a read-lock, one writer attempts a write block,
    the readers release their locks, the writer gets the lock.
*/
void tst_QReadWriteLock::multipleReadersBlockRelease()
{

    QReadWriteLock testLock;
    release.storeRelaxed(false);
    threadDone=false;
    ReadLockReleasableThread rlt1(testLock);
    ReadLockReleasableThread rlt2(testLock);
    rlt1.start();
    rlt2.start();
    QThread::sleep(1s);
    WriteLockThread wlt(testLock);
    wlt.start();
    QThread::sleep(1s);
    release.storeRelaxed(true);
    wlt.wait();
    rlt1.wait();
    rlt2.wait();
    QVERIFY(threadDone);
}

/*
    Multiple readers locks and unlocks a lock.
*/
void tst_QReadWriteLock::multipleReadersLoop()
{
    if (QTestPrivate::isRunningArmOnX86())
        QSKIP("Flaky on QEMU, QTBUG-96103");

    constexpr int time = 500;
    constexpr int hold = 250;
    constexpr int wait = 0;
#if defined (Q_OS_HPUX)
    constexpr int NumThreads = 50;
#elif defined(Q_OS_VXWORKS)
    constexpr int NumThreads = 40;
#else
    constexpr int NumThreads = 75;
#endif
    QReadWriteLock testLock;
    ReadLockLoopThread *threads[NumThreads];
    for (auto &thread : threads)
        thread = new ReadLockLoopThread(testLock, time, hold, wait);
    for (auto thread : threads)
        thread->start();
    for (auto thread : threads)
        thread->wait();
    for (auto thread : threads)
        delete thread;
}

/*
    Multiple writers locks and unlocks a lock.
*/
void tst_QReadWriteLock::multipleWritersLoop()
{
    constexpr int time = 500;
    constexpr int wait = 0;
    constexpr int hold = 0;
    constexpr int numthreads = 50;
    QReadWriteLock testLock;
    WriteLockLoopThread *threads[numthreads];
    for (auto &thread : threads)
        thread = new WriteLockLoopThread(testLock, time, hold, wait);
    for (auto thread : threads)
        thread->start();
    for (auto thread : threads)
        thread->wait();
    for (auto thread : threads)
        delete thread;
}

/*
    Multiple readers and writers locks and unlocks a lock.
*/
void tst_QReadWriteLock::multipleReadersWritersLoop()
{
    constexpr int time = 10000; // INT_MAX
    constexpr int readerThreads = 20;
    constexpr int readerWait = 0;
    constexpr int readerHold = 1;

    constexpr int writerThreads = 2;
    constexpr int writerWait = 500;
    constexpr int writerHold = 50;

    QReadWriteLock testLock;
    ReadLockLoopThread  *readers[readerThreads];
    WriteLockLoopThread *writers[writerThreads];

    for (auto &thread : readers)
        thread = new ReadLockLoopThread(testLock, time, readerHold, readerWait, false);
    for (auto &thread : writers)
        thread = new WriteLockLoopThread(testLock, time, writerHold, writerWait, false);
    for (auto thread : readers)
        thread->start(QThread::NormalPriority);
    for (auto thread : writers)
        thread->start(QThread::IdlePriority);

    for (auto thread : readers)
        thread->wait();
    for (auto thread : writers)
        thread->wait();
    for (auto thread : readers)
        delete thread;
    for (auto thread : writers)
        delete thread;
}

/*
    Writers increment a variable from 0 to maxval, then reset it to 0.
    Readers verify that the variable remains at 0.
*/
void tst_QReadWriteLock::countingTest()
{
    constexpr int time = 10000; // INT_MAX
    constexpr int readerThreads = 20;
    constexpr int readerWait = 1;

    constexpr int writerThreads = 3;
    constexpr int writerWait = 150;
    constexpr int maxval = 10000;

    QReadWriteLock testLock;
    ReadLockCountThread  *readers[readerThreads];
    WriteLockCountThread *writers[writerThreads];

    for (auto &thread : readers)
        thread = new ReadLockCountThread(testLock, time,  readerWait);
    for (auto &thread : writers)
        thread = new WriteLockCountThread(testLock, time,  writerWait, maxval);
    for (auto thread : readers)
        thread->start(QThread::NormalPriority);
    for (auto thread : writers)
        thread->start(QThread::LowestPriority);

    for (auto thread : readers)
        thread->wait();
    for (auto thread : writers)
        thread->wait();
    for (auto thread : readers)
        delete thread;
    for (auto thread : writers)
        delete thread;
}

void tst_QReadWriteLock::limitedReaders()
{

};

/*
    Test a race-condition that may happen if one thread is in unlock() while
    another thread deletes the rw-lock.

    MainThread              DeleteOnUnlockThread

    write-lock
    unlock
      |                     write-lock
      |                     unlock
      |                     delete lock
    deref d inside unlock
*/
class DeleteOnUnlockThread : public QThread
{
public:
    DeleteOnUnlockThread(QReadWriteLock **lock, QWaitCondition *startup, QMutex *waitMutex)
    :m_lock(lock), m_startup(startup), m_waitMutex(waitMutex) {}
    void run() override
    {
        m_waitMutex->lock();
        m_startup->wakeAll();
        m_waitMutex->unlock();

        // DeleteOnUnlockThread and the main thread will race from this point
        (*m_lock)->lockForWrite();
        (*m_lock)->unlock();
        delete *m_lock;
    }
private:
    QReadWriteLock **m_lock;
    QWaitCondition *m_startup;
    QMutex *m_waitMutex;
};

void tst_QReadWriteLock::deleteOnUnlock()
{
    QReadWriteLock *lock = nullptr;
    QWaitCondition startup;
    QMutex waitMutex;

    DeleteOnUnlockThread thread2(&lock, &startup, &waitMutex);

    QElapsedTimer t;
    t.start();
    while(t.elapsed() < 4000) {
        lock = new QReadWriteLock();
        waitMutex.lock();
        lock->lockForWrite();
        thread2.start();
        startup.wait(&waitMutex);
        waitMutex.unlock();

        // DeleteOnUnlockThread and the main thread will race from this point
        lock->unlock();

        thread2.wait();
    }
}


void tst_QReadWriteLock::uncontendedLocks()
{

    uint read=0;
    uint write=0;
    uint count=0;
    int millisecs=1000;
    {
        QElapsedTimer t;
        t.start();
        while(t.elapsed() <millisecs)
        {
            ++count;
        }
    }
    {
        QReadWriteLock rwlock;
        QElapsedTimer t;
        t.start();
        while(t.elapsed() <millisecs)
        {
            rwlock.lockForRead();
            rwlock.unlock();
            ++read;
        }
    }
    {
        QReadWriteLock rwlock;
        QElapsedTimer t;
        t.start();
        while(t.elapsed() <millisecs)
        {
            rwlock.lockForWrite();
            rwlock.unlock();
            ++write;
        }
    }

    qDebug("during %d millisecs:", millisecs);
    qDebug("counted to %u", count);
    qDebug("%u uncontended read locks/unlocks", read);
    qDebug("%u uncontended write locks/unlocks", write);
}

enum { RecursiveLockCount = 10 };

void tst_QReadWriteLock::recursiveReadLock()
{
    // thread to attempt locking for writing while the test recursively locks for reading
    class RecursiveReadLockThread : public QThread
    {
    public:
        QReadWriteLock *lock;
        bool tryLockForWriteResult;

        void run() override
        {
            testsTurn.release();

            // test is recursively locking for writing
            for (int i = 0; i < RecursiveLockCount; ++i) {
                threadsTurn.acquire();
                tryLockForWriteResult = lock->tryLockForWrite();
                testsTurn.release();
            }

            // test is releasing recursive write lock
            for (int i = 0; i < RecursiveLockCount - 1; ++i) {
                threadsTurn.acquire();
                tryLockForWriteResult = lock->tryLockForWrite();
                testsTurn.release();
            }

            // after final unlock in test, we should get the lock
            threadsTurn.acquire();
            tryLockForWriteResult = lock->tryLockForWrite();
            testsTurn.release();

            // cleanup
            threadsTurn.acquire();
            lock->unlock();
            testsTurn.release();

            // test will lockForRead(), then we will lockForWrite()
            // (and block), purpose is to ensure that the test can
            // recursive lockForRead() even with a waiting writer
            threadsTurn.acquire();
            // testsTurn.release(); // ### do not release here, the test uses tryAcquire()
            lock->lockForWrite();
            lock->unlock();
        }
    };

    // init
    QReadWriteLock lock(QReadWriteLock::Recursive);
    RecursiveReadLockThread thread;
    thread.lock = &lock;
    thread.start();

    testsTurn.acquire();

    // verify that we can get multiple read locks in the same thread
    for (int i = 0; i < RecursiveLockCount; ++i) {
        QVERIFY(lock.tryLockForRead());
        threadsTurn.release();

        testsTurn.acquire();
        QVERIFY(!thread.tryLockForWriteResult);
    }

    // have to unlock the same number of times that we locked
    for (int i = 0;i < RecursiveLockCount - 1; ++i) {
        lock.unlock();
        threadsTurn.release();

        testsTurn.acquire();
        QVERIFY(!thread.tryLockForWriteResult);
    }

    // after the final unlock, we should be able to get the write lock
    lock.unlock();
    threadsTurn.release();

    testsTurn.acquire();
    QVERIFY(thread.tryLockForWriteResult);
    threadsTurn.release();

    // check that recursive read locking works even when we have a waiting writer
    testsTurn.acquire();
    QVERIFY(lock.tryLockForRead());
    threadsTurn.release();

    testsTurn.tryAcquire(1, 1000);
    QVERIFY(lock.tryLockForRead());
    lock.unlock();
    lock.unlock();

    // cleanup
    QVERIFY(thread.wait());
}

void tst_QReadWriteLock::recursiveWriteLock()
{
    // thread to attempt locking for reading while the test recursively locks for writing
    class RecursiveWriteLockThread : public QThread
    {
    public:
        QReadWriteLock *lock;
        bool tryLockForReadResult;

        void run() override
        {
            testsTurn.release();

            // test is recursively locking for writing
            for (int i = 0; i < RecursiveLockCount; ++i) {
                threadsTurn.acquire();
                tryLockForReadResult = lock->tryLockForRead();
                testsTurn.release();
            }

            // test is releasing recursive write lock
            for (int i = 0; i < RecursiveLockCount - 1; ++i) {
                threadsTurn.acquire();
                tryLockForReadResult = lock->tryLockForRead();
                testsTurn.release();
            }

            // after final unlock in test, we should get the lock
            threadsTurn.acquire();
            tryLockForReadResult = lock->tryLockForRead();
            testsTurn.release();

            // cleanup
            lock->unlock();
        }
    };

    // init
    QReadWriteLock lock(QReadWriteLock::Recursive);
    RecursiveWriteLockThread thread;
    thread.lock = &lock;
    thread.start();

    testsTurn.acquire();

    // verify that we can get multiple read locks in the same thread
    for (int i = 0; i < RecursiveLockCount; ++i) {
        QVERIFY(lock.tryLockForWrite());
        threadsTurn.release();

        testsTurn.acquire();
        QVERIFY(!thread.tryLockForReadResult);
    }

    // have to unlock the same number of times that we locked
    for (int i = 0;i < RecursiveLockCount - 1; ++i) {
        lock.unlock();
        threadsTurn.release();

        testsTurn.acquire();
        QVERIFY(!thread.tryLockForReadResult);
    }

    // after the final unlock, thread should be able to get the read lock
    lock.unlock();
    threadsTurn.release();

    testsTurn.acquire();
    QVERIFY(thread.tryLockForReadResult);

    // cleanup
    QVERIFY(thread.wait());
}

QTEST_MAIN(tst_QReadWriteLock)

#include "tst_qreadwritelock.moc"
