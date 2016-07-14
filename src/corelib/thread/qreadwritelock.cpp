/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformdefs.h"
#include "qreadwritelock.h"

#ifndef QT_NO_THREAD
#include "qmutex.h"
#include "qthread.h"
#include "qwaitcondition.h"
#include "qreadwritelock_p.h"
#include "qelapsedtimer.h"
#include "private/qfreelist_p.h"

QT_BEGIN_NAMESPACE

/*
 * Implementation details of QReadWriteLock:
 *
 * Depending on the valued of d_ptr, the lock is in the following state:
 *  - when d_ptr == 0x0: Unlocked (no readers, no writers) and non-recursive.
 *  - when d_ptr & 0x1: If the least significant bit is set, we are locked for read.
 *    In that case, d_ptr>>4 represents the number of reading threads minus 1. No writers
 *    are waiting, and the lock is not recursive.
 *  - when d_ptr == 0x2: We are locked for write and nobody is waiting. (no contention)
 *  - In any other case, d_ptr points to an actual QReadWriteLockPrivate.
 */

namespace {
enum {
    StateMask = 0x3,
    StateLockedForRead = 0x1,
    StateLockedForWrite = 0x2,
};
const auto dummyLockedForRead = reinterpret_cast<QReadWriteLockPrivate *>(quintptr(StateLockedForRead));
const auto dummyLockedForWrite = reinterpret_cast<QReadWriteLockPrivate *>(quintptr(StateLockedForWrite));
inline bool isUncontendedLocked(const QReadWriteLockPrivate *d)
{ return quintptr(d) & StateMask; }
}

/*! \class QReadWriteLock
    \inmodule QtCore
    \brief The QReadWriteLock class provides read-write locking.

    \threadsafe

    \ingroup thread

    A read-write lock is a synchronization tool for protecting
    resources that can be accessed for reading and writing. This type
    of lock is useful if you want to allow multiple threads to have
    simultaneous read-only access, but as soon as one thread wants to
    write to the resource, all other threads must be blocked until
    the writing is complete.

    In many cases, QReadWriteLock is a direct competitor to QMutex.
    QReadWriteLock is a good choice if there are many concurrent
    reads and writing occurs infrequently.

    Example:

    \snippet code/src_corelib_thread_qreadwritelock.cpp 0

    To ensure that writers aren't blocked forever by readers, readers
    attempting to obtain a lock will not succeed if there is a blocked
    writer waiting for access, even if the lock is currently only
    accessed by other readers. Also, if the lock is accessed by a
    writer and another writer comes in, that writer will have
    priority over any readers that might also be waiting.

    Like QMutex, a QReadWriteLock can be recursively locked by the
    same thread when constructed with \l{QReadWriteLock::Recursive} as
    \l{QReadWriteLock::RecursionMode}. In such cases,
    unlock() must be called the same number of times lockForWrite() or
    lockForRead() was called. Note that the lock type cannot be
    changed when trying to lock recursively, i.e. it is not possible
    to lock for reading in a thread that already has locked for
    writing (and vice versa).

    \sa QReadLocker, QWriteLocker, QMutex, QSemaphore
*/

/*!
    \enum QReadWriteLock::RecursionMode
    \since 4.4

    \value Recursive In this mode, a thread can lock the same
    QReadWriteLock multiple times. The QReadWriteLock won't be unlocked
    until a corresponding number of unlock() calls have been made.

    \value NonRecursive In this mode, a thread may only lock a
    QReadWriteLock once.

    \sa QReadWriteLock()
*/

/*!
    \since 4.4

    Constructs a QReadWriteLock object in the given \a recursionMode.

    The default recursion mode is NonRecursive.

    \sa lockForRead(), lockForWrite(), RecursionMode
*/
QReadWriteLock::QReadWriteLock(RecursionMode recursionMode)
    : d_ptr(recursionMode == Recursive ? new QReadWriteLockPrivate(true) : nullptr)
{
    Q_ASSERT_X(!(quintptr(d_ptr.load()) & StateMask), "QReadWriteLock::QReadWriteLock", "bad d_ptr alignment");
}

/*!
    Destroys the QReadWriteLock object.

    \warning Destroying a read-write lock that is in use may result
    in undefined behavior.
*/
QReadWriteLock::~QReadWriteLock()
{
    auto d = d_ptr.load();
    if (isUncontendedLocked(d)) {
        qWarning("QReadWriteLock: destroying locked QReadWriteLock");
        return;
    }
    delete d;
}

/*!
    Locks the lock for reading. This function will block the current
    thread if another thread has locked for writing.

    It is not possible to lock for read if the thread already has
    locked for write.

    \sa unlock(), lockForWrite(), tryLockForRead()
*/
void QReadWriteLock::lockForRead()
{
    if (d_ptr.testAndSetAcquire(nullptr, dummyLockedForRead))
        return;
    tryLockForRead(-1);
}

/*!
    Attempts to lock for reading. If the lock was obtained, this
    function returns \c true, otherwise it returns \c false instead of
    waiting for the lock to become available, i.e. it does not block.

    The lock attempt will fail if another thread has locked for
    writing.

    If the lock was obtained, the lock must be unlocked with unlock()
    before another thread can successfully lock it for writing.

    It is not possible to lock for read if the thread already has
    locked for write.

    \sa unlock(), lockForRead()
*/
bool QReadWriteLock::tryLockForRead()
{
    return tryLockForRead(0);
}

/*! \overload

    Attempts to lock for reading. This function returns \c true if the
    lock was obtained; otherwise it returns \c false. If another thread
    has locked for writing, this function will wait for at most \a
    timeout milliseconds for the lock to become available.

    Note: Passing a negative number as the \a timeout is equivalent to
    calling lockForRead(), i.e. this function will wait forever until
    lock can be locked for reading when \a timeout is negative.

    If the lock was obtained, the lock must be unlocked with unlock()
    before another thread can successfully lock it for writing.

    It is not possible to lock for read if the thread already has
    locked for write.

    \sa unlock(), lockForRead()
*/
bool QReadWriteLock::tryLockForRead(int timeout)
{
    // Fast case: non contended:
    QReadWriteLockPrivate *d;
    if (d_ptr.testAndSetAcquire(nullptr, dummyLockedForRead, d))
        return true;

    while (true) {
        if (d == 0) {
            if (!d_ptr.testAndSetAcquire(nullptr, dummyLockedForRead, d))
                continue;
            return true;
        }

        if ((quintptr(d) & StateMask) == StateLockedForRead) {
            // locked for read, increase the counter
            const auto val = reinterpret_cast<QReadWriteLockPrivate *>(quintptr(d) + (1U<<4));
            Q_ASSERT_X(quintptr(val) > (1U<<4), "QReadWriteLock::tryLockForRead()",
                       "Overflow in lock counter");
            if (!d_ptr.testAndSetAcquire(d, val, d))
                continue;
            return true;
        }

        if (d == dummyLockedForWrite) {
            if (!timeout)
                return false;

            // locked for write, assign a d_ptr and wait.
            auto val = QReadWriteLockPrivate::allocate();
            val->writerCount = 1;
            if (!d_ptr.testAndSetOrdered(d, val, d)) {
                val->writerCount = 0;
                val->release();
                continue;
            }
            d = val;
        }
        Q_ASSERT(!isUncontendedLocked(d));
        // d is an actual pointer;

        if (d->recursive)
            return d->recursiveLockForRead(timeout);

        QMutexLocker lock(&d->mutex);
        if (d != d_ptr.load()) {
            // d_ptr has changed: this QReadWriteLock was unlocked before we had
            // time to lock d->mutex.
            // We are holding a lock to a mutex within a QReadWriteLockPrivate
            // that is already released (or even is already re-used). That's ok
            // because the QFreeList never frees them.
            // Just unlock d->mutex (at the end of the scope) and retry.
            d = d_ptr.loadAcquire();
            continue;
        }
        return d->lockForRead(timeout);
    }
}

/*!
    Locks the lock for writing. This function will block the current
    thread if another thread (including the current) has locked for
    reading or writing (unless the lock has been created using the
    \l{QReadWriteLock::Recursive} mode).

    It is not possible to lock for write if the thread already has
    locked for read.

    \sa unlock(), lockForRead(), tryLockForWrite()
*/
void QReadWriteLock::lockForWrite()
{
    tryLockForWrite(-1);
}

/*!
    Attempts to lock for writing. If the lock was obtained, this
    function returns \c true; otherwise, it returns \c false immediately.

    The lock attempt will fail if another thread has locked for
    reading or writing.

    If the lock was obtained, the lock must be unlocked with unlock()
    before another thread can successfully lock it.

    It is not possible to lock for write if the thread already has
    locked for read.

    \sa unlock(), lockForWrite()
*/
bool QReadWriteLock::tryLockForWrite()
{
    return tryLockForWrite(0);
}

/*! \overload

    Attempts to lock for writing. This function returns \c true if the
    lock was obtained; otherwise it returns \c false. If another thread
    has locked for reading or writing, this function will wait for at
    most \a timeout milliseconds for the lock to become available.

    Note: Passing a negative number as the \a timeout is equivalent to
    calling lockForWrite(), i.e. this function will wait forever until
    lock can be locked for writing when \a timeout is negative.

    If the lock was obtained, the lock must be unlocked with unlock()
    before another thread can successfully lock it.

    It is not possible to lock for write if the thread already has
    locked for read.

    \sa unlock(), lockForWrite()
*/
bool QReadWriteLock::tryLockForWrite(int timeout)
{
    // Fast case: non contended:
    QReadWriteLockPrivate *d;
    if (d_ptr.testAndSetAcquire(nullptr, dummyLockedForWrite, d))
        return true;

    while (true) {
        if (d == 0) {
            if (!d_ptr.testAndSetAcquire(d, dummyLockedForWrite, d))
                continue;
            return true;
        }

        if (isUncontendedLocked(d)) {
            if (!timeout)
                return false;

            // locked for either read or write, assign a d_ptr and wait.
            auto val = QReadWriteLockPrivate::allocate();
            if (d == dummyLockedForWrite)
                val->writerCount = 1;
            else
                val->readerCount = (quintptr(d) >> 4) + 1;
            if (!d_ptr.testAndSetOrdered(d, val, d)) {
                val->writerCount = val->readerCount = 0;
                val->release();
                continue;
            }
            d = val;
        }
        Q_ASSERT(!isUncontendedLocked(d));
        // d is an actual pointer;

        if (d->recursive)
            return d->recursiveLockForWrite(timeout);

        QMutexLocker lock(&d->mutex);
        if (d != d_ptr.load()) {
            // The mutex was unlocked before we had time to lock the mutex.
            // We are holding to a mutex within a QReadWriteLockPrivate that is already released
            // (or even is already re-used) but that's ok because the QFreeList never frees them.
            d = d_ptr.loadAcquire();
            continue;
        }
        return d->lockForWrite(timeout);
    }
}

/*!
    Unlocks the lock.

    Attempting to unlock a lock that is not locked is an error, and will result
    in program termination.

    \sa lockForRead(), lockForWrite(), tryLockForRead(), tryLockForWrite()
*/
void QReadWriteLock::unlock()
{
    QReadWriteLockPrivate *d = d_ptr.load();
    while (true) {
        Q_ASSERT_X(d, "QReadWriteLock::unlock()", "Cannot unlock an unlocked lock");

        // Fast case: no contention: (no waiters, no other readers)
        if (quintptr(d) <= 2) { // 1 or 2 (StateLockedForRead or StateLockedForWrite)
            if (!d_ptr.testAndSetRelease(d, nullptr, d))
                continue;
            return;
        }

        if ((quintptr(d) & StateMask) == StateLockedForRead) {
            Q_ASSERT(quintptr(d) > (1U<<4)); //otherwise that would be the fast case
            // Just decrease the reader's count.
            auto val = reinterpret_cast<QReadWriteLockPrivate *>(quintptr(d) - (1U<<4));
            if (!d_ptr.testAndSetRelease(d, val, d))
                continue;
            return;
        }

        Q_ASSERT(!isUncontendedLocked(d));

        if (d->recursive) {
            d->recursiveUnlock();
            return;
        }

        QMutexLocker locker(&d->mutex);
        if (d->writerCount) {
            Q_ASSERT(d->writerCount == 1);
            Q_ASSERT(d->readerCount == 0);
            d->writerCount = 0;
        } else {
            Q_ASSERT(d->readerCount > 0);
            d->readerCount--;
            if (d->readerCount > 0)
                return;
        }

        if (d->waitingReaders || d->waitingWriters) {
            d->unlock();
        } else {
            Q_ASSERT(d_ptr.load() == d); // should not change when we still hold the mutex
            d_ptr.storeRelease(nullptr);
            d->release();
        }
        return;
    }
}

/*! \internal  Helper for QWaitCondition::wait */
QReadWriteLock::StateForWaitCondition QReadWriteLock::stateForWaitCondition() const
{
    QReadWriteLockPrivate *d = d_ptr.load();
    switch (quintptr(d) & StateMask) {
    case StateLockedForRead: return LockedForRead;
    case StateLockedForWrite: return LockedForWrite;
    }

    if (!d)
        return Unlocked;
    if (d->writerCount > 1)
        return RecursivelyLocked;
    else if (d->writerCount == 1)
        return LockedForWrite;
    return LockedForRead;

}

bool QReadWriteLockPrivate::lockForRead(int timeout)
{
    Q_ASSERT(!mutex.tryLock()); // mutex must be locked when entering this function

    QElapsedTimer t;
    if (timeout > 0)
        t.start();

    while (waitingWriters || writerCount) {
        if (timeout == 0)
            return false;
        if (timeout > 0) {
            auto elapsed = t.elapsed();
            if (elapsed > timeout)
                return false;
            waitingReaders++;
            readerCond.wait(&mutex, timeout - elapsed);
        } else {
            waitingReaders++;
            readerCond.wait(&mutex);
        }
        waitingReaders--;
    }
    readerCount++;
    Q_ASSERT(writerCount == 0);
    return true;
}

bool QReadWriteLockPrivate::lockForWrite(int timeout)
{
    Q_ASSERT(!mutex.tryLock()); // mutex must be locked when entering this function

    QElapsedTimer t;
    if (timeout > 0)
        t.start();

    while (readerCount || writerCount) {
        if (timeout == 0)
            return false;
        if (timeout > 0) {
            auto elapsed = t.elapsed();
            if (elapsed > timeout) {
                if (waitingReaders && !waitingWriters && !writerCount) {
                    // We timed out and now there is no more writers or waiting writers, but some
                    // readers were queueud (probably because of us). Wake the waiting readers.
                    readerCond.wakeAll();
                }
                return false;
            }
            waitingWriters++;
            writerCond.wait(&mutex, timeout - elapsed);
        } else {
            waitingWriters++;
            writerCond.wait(&mutex);
        }
        waitingWriters--;
    }

    Q_ASSERT(writerCount == 0);
    Q_ASSERT(readerCount == 0);
    writerCount = 1;
    return true;
}

void QReadWriteLockPrivate::unlock()
{
    Q_ASSERT(!mutex.tryLock()); // mutex must be locked when entering this function
    if (waitingWriters)
        writerCond.wakeOne();
    else if (waitingReaders)
        readerCond.wakeAll();
}

bool QReadWriteLockPrivate::recursiveLockForRead(int timeout)
{
    Q_ASSERT(recursive);
    QMutexLocker lock(&mutex);

    Qt::HANDLE self = QThread::currentThreadId();

    auto it = currentReaders.find(self);
    if (it != currentReaders.end()) {
        ++it.value();
        return true;
    }

    if (!lockForRead(timeout))
        return false;

    currentReaders.insert(self, 1);
    return true;
}

bool QReadWriteLockPrivate::recursiveLockForWrite(int timeout)
{
    Q_ASSERT(recursive);
    QMutexLocker lock(&mutex);

    Qt::HANDLE self = QThread::currentThreadId();
    if (currentWriter == self) {
        writerCount++;
        return true;
    }

    if (!lockForWrite(timeout))
        return false;

    currentWriter = self;
    return true;
}

void QReadWriteLockPrivate::recursiveUnlock()
{
    Q_ASSERT(recursive);
    QMutexLocker lock(&mutex);

    Qt::HANDLE self = QThread::currentThreadId();
    if (self == currentWriter) {
        if (--writerCount > 0)
            return;
        currentWriter = 0;
    } else {
        auto it = currentReaders.find(self);
        if (it == currentReaders.end()) {
            qWarning("QReadWriteLock::unlock: unlocking from a thread that did not lock");
            return;
        } else {
            if (--it.value() <= 0) {
                currentReaders.erase(it);
                readerCount--;
            }
            if (readerCount)
                return;
        }
    }

    unlock();
}

// The freelist management
namespace {
struct FreeListConstants : QFreeListDefaultConstants {
    enum { BlockCount = 4, MaxIndex=0xffff };
    static const int Sizes[BlockCount];
};
const int FreeListConstants::Sizes[FreeListConstants::BlockCount] = {
    16,
    128,
    1024,
    FreeListConstants::MaxIndex - (16 + 128 + 1024)
};

typedef QFreeList<QReadWriteLockPrivate, FreeListConstants> FreeList;
Q_GLOBAL_STATIC(FreeList, freelist);
}

QReadWriteLockPrivate *QReadWriteLockPrivate::allocate()
{
    int i = freelist->next();
    QReadWriteLockPrivate *d = &(*freelist)[i];
    d->id = i;
    Q_ASSERT(!d->recursive);
    Q_ASSERT(!d->waitingReaders && !d->waitingReaders && !d->readerCount && !d->writerCount);
    return d;
}

void QReadWriteLockPrivate::release()
{
    Q_ASSERT(!recursive);
    Q_ASSERT(!waitingReaders && !waitingReaders && !readerCount && !writerCount);
    freelist->release(id);
}

/*!
    \class QReadLocker
    \inmodule QtCore
    \brief The QReadLocker class is a convenience class that
    simplifies locking and unlocking read-write locks for read access.

    \threadsafe

    \ingroup thread

    The purpose of QReadLocker (and QWriteLocker) is to simplify
    QReadWriteLock locking and unlocking. Locking and unlocking
    statements or in exception handling code is error-prone and
    difficult to debug. QReadLocker can be used in such situations
    to ensure that the state of the lock is always well-defined.

    Here's an example that uses QReadLocker to lock and unlock a
    read-write lock for reading:

    \snippet code/src_corelib_thread_qreadwritelock.cpp 1

    It is equivalent to the following code:

    \snippet code/src_corelib_thread_qreadwritelock.cpp 2

    The QMutexLocker documentation shows examples where the use of a
    locker object greatly simplifies programming.

    \sa QWriteLocker, QReadWriteLock
*/

/*!
    \fn QReadLocker::QReadLocker(QReadWriteLock *lock)

    Constructs a QReadLocker and locks \a lock for reading. The lock
    will be unlocked when the QReadLocker is destroyed. If \c lock is
    zero, QReadLocker does nothing.

    \sa QReadWriteLock::lockForRead()
*/

/*!
    \fn QReadLocker::~QReadLocker()

    Destroys the QReadLocker and unlocks the lock that was passed to
    the constructor.

    \sa QReadWriteLock::unlock()
*/

/*!
    \fn void QReadLocker::unlock()

    Unlocks the lock associated with this locker.

    \sa QReadWriteLock::unlock()
*/

/*!
    \fn void QReadLocker::relock()

    Relocks an unlocked lock.

    \sa unlock()
*/

/*!
    \fn QReadWriteLock *QReadLocker::readWriteLock() const

    Returns a pointer to the read-write lock that was passed
    to the constructor.
*/

/*!
    \class QWriteLocker
    \inmodule QtCore
    \brief The QWriteLocker class is a convenience class that
    simplifies locking and unlocking read-write locks for write access.

    \threadsafe

    \ingroup thread

    The purpose of QWriteLocker (and QReadLocker) is to simplify
    QReadWriteLock locking and unlocking. Locking and unlocking
    statements or in exception handling code is error-prone and
    difficult to debug. QWriteLocker can be used in such situations
    to ensure that the state of the lock is always well-defined.

    Here's an example that uses QWriteLocker to lock and unlock a
    read-write lock for writing:

    \snippet code/src_corelib_thread_qreadwritelock.cpp 3

    It is equivalent to the following code:

    \snippet code/src_corelib_thread_qreadwritelock.cpp 4

    The QMutexLocker documentation shows examples where the use of a
    locker object greatly simplifies programming.

    \sa QReadLocker, QReadWriteLock
*/

/*!
    \fn QWriteLocker::QWriteLocker(QReadWriteLock *lock)

    Constructs a QWriteLocker and locks \a lock for writing. The lock
    will be unlocked when the QWriteLocker is destroyed. If \c lock is
    zero, QWriteLocker does nothing.

    \sa QReadWriteLock::lockForWrite()
*/

/*!
    \fn QWriteLocker::~QWriteLocker()

    Destroys the QWriteLocker and unlocks the lock that was passed to
    the constructor.

    \sa QReadWriteLock::unlock()
*/

/*!
    \fn void QWriteLocker::unlock()

    Unlocks the lock associated with this locker.

    \sa QReadWriteLock::unlock()
*/

/*!
    \fn void QWriteLocker::relock()

    Relocks an unlocked lock.

    \sa unlock()
*/

/*!
    \fn QReadWriteLock *QWriteLocker::readWriteLock() const

    Returns a pointer to the read-write lock that was passed
    to the constructor.
*/

QT_END_NAMESPACE

#endif // QT_NO_THREAD
