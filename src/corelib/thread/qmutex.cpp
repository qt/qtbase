// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// Copyright (C) 2012 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "global/qglobal.h"
#include "qplatformdefs.h"
#include "qmutex.h"
#include <qdebug.h>
#include "qatomic.h"
#include "qelapsedtimer.h"
#include "qfutex_p.h"
#include "qthread.h"
#include "qmutex_p.h"

#ifndef QT_ALWAYS_USE_FUTEX
#include "private/qfreelist_p.h"
#endif

QT_BEGIN_NAMESPACE

using namespace QtFutex;
static inline QMutexPrivate *dummyFutexValue()
{
    return reinterpret_cast<QMutexPrivate *>(quintptr(3));
}

/*
    \class QBasicMutex
    \inmodule QtCore
    \brief QMutex POD
    \internal

    \ingroup thread

    - Can be used as global static object.
    - Always non-recursive
    - Do not use tryLock with timeout > 0, else you can have a leak (see the ~QMutex destructor)
*/

/*!
    \class QMutex
    \inmodule QtCore
    \brief The QMutex class provides access serialization between threads.

    \threadsafe

    \ingroup thread

    The purpose of a QMutex is to protect an object, data structure or
    section of code so that only one thread can access it at a time
    (this is similar to the Java \c synchronized keyword). It is
    usually best to use a mutex with a QMutexLocker since this makes
    it easy to ensure that locking and unlocking are performed
    consistently.

    For example, say there is a method that prints a message to the
    user on two lines:

    \snippet code/src_corelib_thread_qmutex.cpp 0

    If these two methods are called in succession, the following happens:

    \snippet code/src_corelib_thread_qmutex.cpp 1

    If these two methods are called simultaneously from two threads then the
    following sequence could result:

    \snippet code/src_corelib_thread_qmutex.cpp 2

    If we add a mutex, we should get the result we want:

    \snippet code/src_corelib_thread_qmutex.cpp 3

    Then only one thread can modify \c number at any given time and
    the result is correct. This is a trivial example, of course, but
    applies to any other case where things need to happen in a
    particular sequence.

    When you call lock() in a thread, other threads that try to call
    lock() in the same place will block until the thread that got the
    lock calls unlock(). A non-blocking alternative to lock() is
    tryLock().

    QMutex is optimized to be fast in the non-contended case. It
    will not allocate memory if there is no contention on that mutex.
    It is constructed and destroyed with almost no overhead,
    which means it is fine to have many mutexes as part of other classes.

    \sa QRecursiveMutex, QMutexLocker, QReadWriteLock, QSemaphore, QWaitCondition
*/

/*!
    \fn QMutex::QMutex()

    Constructs a new mutex. The mutex is created in an unlocked state.
*/

/*! \fn QMutex::~QMutex()

    Destroys the mutex.

    \warning Destroying a locked mutex may result in undefined behavior.
*/
void QBasicMutex::destroyInternal(QMutexPrivate *d)
{
    if (!d)
        return;
    if (!futexAvailable()) {
        if (d != dummyLocked() && d->possiblyUnlocked.loadRelaxed() && tryLock()) {
            unlock();
            return;
        }
    }
    qWarning("QMutex: destroying locked mutex");
}

/*! \fn void QMutex::lock()

    Locks the mutex. If another thread has locked the mutex then this
    call will block until that thread has unlocked it.

    Calling this function multiple times on the same mutex from the
    same thread will cause a \e dead-lock.

    \sa unlock()
*/

/*! \fn bool QMutex::tryLock(int timeout)

    Attempts to lock the mutex. This function returns \c true if the lock
    was obtained; otherwise it returns \c false. If another thread has
    locked the mutex, this function will wait for at most \a timeout
    milliseconds for the mutex to become available.

    Note: Passing a negative number as the \a timeout is equivalent to
    calling lock(), i.e. this function will wait forever until mutex
    can be locked if \a timeout is negative.

    If the lock was obtained, the mutex must be unlocked with unlock()
    before another thread can successfully lock it.

    Calling this function multiple times on the same mutex from the
    same thread will cause a \e dead-lock.

    \sa lock(), unlock()
*/

/*! \fn bool QMutex::tryLock(QDeadlineTimer timer)
    \since 6.6

    Attempts to lock the mutex. This function returns \c true if the lock
    was obtained; otherwise it returns \c false. If another thread has
    locked the mutex, this function will wait until \a timer expires
    for the mutex to become available.

    If the lock was obtained, the mutex must be unlocked with unlock()
    before another thread can successfully lock it.

    Calling this function multiple times on the same mutex from the
    same thread will cause a \e dead-lock.

    \sa lock(), unlock()
*/

/*! \fn bool QMutex::tryLock()
    \overload

    Attempts to lock the mutex. This function returns \c true if the lock
    was obtained; otherwise it returns \c false.

    If the lock was obtained, the mutex must be unlocked with unlock()
    before another thread can successfully lock it.

    Calling this function multiple times on the same mutex from the
    same thread will cause a \e dead-lock.

    \sa lock(), unlock()
*/

/*! \fn bool QMutex::try_lock()
    \since 5.8

    Attempts to lock the mutex. This function returns \c true if the lock
    was obtained; otherwise it returns \c false.

    This function is provided for compatibility with the Standard Library
    concept \c Lockable. It is equivalent to tryLock().
*/

/*! \fn template <class Rep, class Period> bool QMutex::try_lock_for(std::chrono::duration<Rep, Period> duration)
    \since 5.8

    Attempts to lock the mutex. This function returns \c true if the lock
    was obtained; otherwise it returns \c false. If another thread has
    locked the mutex, this function will wait for at least \a duration
    for the mutex to become available.

    Note: Passing a negative duration as the \a duration is equivalent to
    calling try_lock(). This behavior differs from tryLock().

    If the lock was obtained, the mutex must be unlocked with unlock()
    before another thread can successfully lock it.

    Calling this function multiple times on the same mutex from the
    same thread will cause a \e dead-lock.

    \sa lock(), unlock()
*/

/*! \fn template<class Clock, class Duration> bool QMutex::try_lock_until(std::chrono::time_point<Clock, Duration> timePoint)
    \since 5.8

    Attempts to lock the mutex. This function returns \c true if the lock
    was obtained; otherwise it returns \c false. If another thread has
    locked the mutex, this function will wait at least until \a timePoint
    for the mutex to become available.

    Note: Passing a \a timePoint which has already passed is equivalent
    to calling try_lock(). This behavior differs from tryLock().

    If the lock was obtained, the mutex must be unlocked with unlock()
    before another thread can successfully lock it.

    Calling this function multiple times on the same mutex from the
    same thread will cause a \e dead-lock.

    \sa lock(), unlock()
*/

/*! \fn void QMutex::unlock()

    Unlocks the mutex. Attempting to unlock a mutex in a different
    thread to the one that locked it results in an error. Unlocking a
    mutex that is not locked results in undefined behavior.

    \sa lock()
*/

/*!
    \class QRecursiveMutex
    \inmodule QtCore
    \since 5.14
    \brief The QRecursiveMutex class provides access serialization between threads.

    \threadsafe

    \ingroup thread

    The QRecursiveMutex class is a mutex, like QMutex, with which it is
    API-compatible. It differs from QMutex by accepting lock() calls from
    the same thread any number of times. QMutex would deadlock in this situation.

    QRecursiveMutex is much more expensive to construct and operate on, so
    use a plain QMutex whenever you can. Sometimes, one public function,
    however, calls another public function, and they both need to lock the
    same mutex. In this case, you have two options:

    \list
    \li Factor the code that needs mutex protection into private functions,
    which assume that the mutex is held when they are called, and lock a
    plain QMutex in the public functions before you call the private
    implementation ones.
    \li Or use a recursive mutex, so it doesn't matter that the first public
    function has already locked the mutex when the second one wishes to do so.
    \endlist

    \sa QMutex, QMutexLocker, QReadWriteLock, QSemaphore, QWaitCondition
*/

/*! \fn QRecursiveMutex::QRecursiveMutex()

    Constructs a new recursive mutex. The mutex is created in an unlocked state.

    \sa lock(), unlock()
*/

/*!
    Destroys the mutex.

    \warning Destroying a locked mutex may result in undefined behavior.
*/
QRecursiveMutex::~QRecursiveMutex()
{
}

/*! \fn void QRecursiveMutex::lock()

    Locks the mutex. If another thread has locked the mutex then this
    call will block until that thread has unlocked it.

    Calling this function multiple times on the same mutex from the
    same thread is allowed.

    \sa unlock()
*/

/*!
    \fn QRecursiveMutex::tryLock(int timeout)

    Attempts to lock the mutex. This function returns \c true if the lock
    was obtained; otherwise it returns \c false. If another thread has
    locked the mutex, this function will wait for at most \a timeout
    milliseconds for the mutex to become available.

    Note: Passing a negative number as the \a timeout is equivalent to
    calling lock(), i.e. this function will wait forever until mutex
    can be locked if \a timeout is negative.

    If the lock was obtained, the mutex must be unlocked with unlock()
    before another thread can successfully lock it.

    Calling this function multiple times on the same mutex from the
    same thread is allowed.

    \sa lock(), unlock()
*/

/*!
    \since 6.6

    Attempts to lock the mutex. This function returns \c true if the lock
    was obtained; otherwise it returns \c false. If another thread has
    locked the mutex, this function will wait until \a timeout expires
    for the mutex to become available.

    If the lock was obtained, the mutex must be unlocked with unlock()
    before another thread can successfully lock it.

    Calling this function multiple times on the same mutex from the
    same thread is allowed.

    \sa lock(), unlock()
*/
bool QRecursiveMutex::tryLock(QDeadlineTimer timeout) QT_MUTEX_LOCK_NOEXCEPT
{
    unsigned tsanFlags = QtTsan::MutexWriteReentrant | QtTsan::TryLock;
    QtTsan::mutexPreLock(this, tsanFlags);

    Qt::HANDLE self = QThread::currentThreadId();
    if (owner.loadRelaxed() == self) {
        ++count;
        Q_ASSERT_X(count != 0, "QMutex::lock", "Overflow in recursion counter");
        QtTsan::mutexPostLock(this, tsanFlags, 0);
        return true;
    }
    bool success = true;
    if (timeout.isForever()) {
        mutex.lock();
    } else {
        success = mutex.tryLock(timeout);
    }

    if (success)
        owner.storeRelaxed(self);
    else
        tsanFlags |= QtTsan::TryLockFailed;

    QtTsan::mutexPostLock(this, tsanFlags, 0);

    return success;
}

/*! \fn bool QRecursiveMutex::try_lock()
    \since 5.8

    Attempts to lock the mutex. This function returns \c true if the lock
    was obtained; otherwise it returns \c false.

    This function is provided for compatibility with the Standard Library
    concept \c Lockable. It is equivalent to tryLock().
*/

/*! \fn template <class Rep, class Period> bool QRecursiveMutex::try_lock_for(std::chrono::duration<Rep, Period> duration)
    \since 5.8

    Attempts to lock the mutex. This function returns \c true if the lock
    was obtained; otherwise it returns \c false. If another thread has
    locked the mutex, this function will wait for at least \a duration
    for the mutex to become available.

    Note: Passing a negative duration as the \a duration is equivalent to
    calling try_lock(). This behavior differs from tryLock().

    If the lock was obtained, the mutex must be unlocked with unlock()
    before another thread can successfully lock it.

    Calling this function multiple times on the same mutex from the
    same thread is allowed.

    \sa lock(), unlock()
*/

/*! \fn template<class Clock, class Duration> bool QRecursiveMutex::try_lock_until(std::chrono::time_point<Clock, Duration> timePoint)
    \since 5.8

    Attempts to lock the mutex. This function returns \c true if the lock
    was obtained; otherwise it returns \c false. If another thread has
    locked the mutex, this function will wait at least until \a timePoint
    for the mutex to become available.

    Note: Passing a \a timePoint which has already passed is equivalent
    to calling try_lock(). This behavior differs from tryLock().

    If the lock was obtained, the mutex must be unlocked with unlock()
    before another thread can successfully lock it.

    Calling this function multiple times on the same mutex from the
    same thread is allowed.

    \sa lock(), unlock()
*/

/*!
    Unlocks the mutex. Attempting to unlock a mutex in a different
    thread to the one that locked it results in an error. Unlocking a
    mutex that is not locked results in undefined behavior.

    \sa lock()
*/
void QRecursiveMutex::unlock() noexcept
{
    Q_ASSERT(owner.loadRelaxed() == QThread::currentThreadId());
    QtTsan::mutexPreUnlock(this, 0u);

    if (count > 0) {
        count--;
    } else {
        owner.storeRelaxed(nullptr);
        mutex.unlock();
    }

    QtTsan::mutexPostUnlock(this, 0u);
}


/*!
    \class QMutexLocker
    \inmodule QtCore
    \brief The QMutexLocker class is a convenience class that simplifies
    locking and unlocking mutexes.

    \threadsafe

    \ingroup thread

    Locking and unlocking a QMutex or QRecursiveMutex in complex functions and
    statements or in exception handling code is error-prone and
    difficult to debug. QMutexLocker can be used in such situations
    to ensure that the state of the mutex is always well-defined.

    QMutexLocker should be created within a function where a
    QMutex needs to be locked. The mutex is locked when QMutexLocker
    is created. You can unlock and relock the mutex with \c unlock()
    and \c relock(). If locked, the mutex will be unlocked when the
    QMutexLocker is destroyed.

    For example, this complex function locks a QMutex upon entering
    the function and unlocks the mutex at all the exit points:

    \snippet code/src_corelib_thread_qmutex.cpp 4

    This example function will get more complicated as it is
    developed, which increases the likelihood that errors will occur.

    Using QMutexLocker greatly simplifies the code, and makes it more
    readable:

    \snippet code/src_corelib_thread_qmutex.cpp 5

    Now, the mutex will always be unlocked when the QMutexLocker
    object is destroyed (when the function returns since \c locker is
    an auto variable).

    The same principle applies to code that throws and catches
    exceptions. An exception that is not caught in the function that
    has locked the mutex has no way of unlocking the mutex before the
    exception is passed up the stack to the calling function.

    QMutexLocker also provides a \c mutex() member function that returns
    the mutex on which the QMutexLocker is operating. This is useful
    for code that needs access to the mutex, such as
    QWaitCondition::wait(). For example:

    \snippet code/src_corelib_thread_qmutex.cpp 6

    \sa QReadLocker, QWriteLocker, QMutex
*/

/*!
    \fn template <typename Mutex> QMutexLocker<Mutex>::QMutexLocker(Mutex *mutex) noexcept

    Constructs a QMutexLocker and locks \a mutex. The mutex will be
    unlocked when the QMutexLocker is destroyed. If \a mutex is \nullptr,
    QMutexLocker does nothing.

    \sa QMutex::lock()
*/

/*!
    \fn template <typename Mutex> QMutexLocker<Mutex>::QMutexLocker(QMutexLocker &&other) noexcept
    \since 6.4

    Move-constructs a QMutexLocker from \a other. The mutex and the
    state of \a other is transferred to the newly constructed instance.
    After the move, \a other will no longer be managing any mutex.

    \sa QMutex::lock()
*/

/*!
    \fn template <typename Mutex> QMutexLocker<Mutex> &QMutexLocker<Mutex>::operator=(QMutexLocker &&other) noexcept
    \since 6.4

    Move-assigns \a other onto this QMutexLocker. If this QMutexLocker
    was holding a locked mutex before the assignment, the mutex will be
    unlocked. The mutex and the state of \a other is then transferred
    to this QMutexLocker. After the move, \a other will no longer be
    managing any mutex.

    \sa QMutex::lock()
*/

/*!
    \fn template <typename Mutex> void QMutexLocker<Mutex>::swap(QMutexLocker &other) noexcept
    \since 6.4

    Swaps the mutex and the state of this QMutexLocker with \a other.
    This operation is very fast and never fails.

    \sa QMutex::lock()
*/

/*!
    \fn template <typename Mutex> QMutexLocker<Mutex>::~QMutexLocker() noexcept

    Destroys the QMutexLocker and unlocks the mutex that was locked
    in the constructor.

    \sa QMutex::unlock()
*/

/*!
    \fn template <typename Mutex> bool QMutexLocker<Mutex>::isLocked() const noexcept
    \since 6.4

    Returns true if this QMutexLocker is currently locking its associated
    mutex, or false otherwise.
*/

/*!
    \fn template <typename Mutex> void QMutexLocker<Mutex>::unlock() noexcept

    Unlocks this mutex locker. You can use \c relock() to lock
    it again. It does not need to be locked when destroyed.

    \sa relock()
*/

/*!
    \fn template <typename Mutex> void QMutexLocker<Mutex>::relock() noexcept

    Relocks an unlocked mutex locker.

    \sa unlock()
*/

/*!
    \fn template <typename Mutex> QMutex *QMutexLocker<Mutex>::mutex() const

    Returns the mutex on which the QMutexLocker is operating.

*/

/*
  For a rough introduction on how this works, refer to
  http://woboq.com/blog/internals-of-qmutex-in-qt5.html
  which explains a slightly simplified version of it.
  The differences are that here we try to work with timeout (requires the
  possiblyUnlocked flag) and that we only wake one thread when unlocking
  (requires maintaining the waiters count)
  We also support recursive mutexes which always have a valid d_ptr.

  The waiters flag represents the number of threads that are waiting or about
  to wait on the mutex. There are two tricks to keep in mind:
  We don't want to increment waiters after we checked no threads are waiting
  (waiters == 0). That's why we atomically set the BigNumber flag on waiters when
  we check waiters. Similarly, if waiters is decremented right after we checked,
  the mutex would be unlocked (d->wakeUp() has (or will) be called), but there is
  no thread waiting. This is only happening if there was a timeout in tryLock at the
  same time as the mutex is unlocked. So when there was a timeout, we set the
  possiblyUnlocked flag.
*/

/*
 * QBasicMutex implementation with futexes (Linux, Windows 10)
 *
 * QBasicMutex contains one pointer value, which can contain one of four
 * different values:
 *    0x0       unlocked
 *    0x1       locked, no waiters
 *    0x3       locked, at least one waiter
 *
 * LOCKING:
 *
 * A starts in the 0x0 state, indicating that it's unlocked. When the first
 * thread attempts to lock it, it will perform a testAndSetAcquire
 * from 0x0 to 0x1. If that succeeds, the caller concludes that it
 * successfully locked the mutex. That happens in fastTryLock().
 *
 * If that testAndSetAcquire fails, QBasicMutex::lockInternal is called.
 *
 * lockInternal will examine the value of the pointer. Otherwise, it will use
 * futexes to sleep and wait for another thread to unlock. To do that, it needs
 * to set a pointer value of 0x3, which indicates that thread is waiting. It
 * does that by a simple fetchAndStoreAcquire operation.
 *
 * If the pointer value was 0x0, it means we succeeded in acquiring the mutex.
 * For other values, it will then call FUTEX_WAIT and with an expected value of
 * 0x3.
 *
 * If the pointer value changed before futex(2) managed to sleep, it will
 * return -1 / EWOULDBLOCK, in which case we have to start over. And even if we
 * are woken up directly by a FUTEX_WAKE, we need to acquire the mutex, so we
 * start over again.
 *
 * UNLOCKING:
 *
 * To unlock, we need to set a value of 0x0 to indicate it's unlocked. The
 * first attempt is a testAndSetRelease operation from 0x1 to 0x0. If that
 * succeeds, we're done.
 *
 * If it fails, unlockInternal() is called. The only possibility is that the
 * mutex value was 0x3, which indicates some other thread is waiting or was
 * waiting in the past. We then set the mutex to 0x0 and perform a FUTEX_WAKE.
 */

/*!
    \internal helper for lock()
 */
void QBasicMutex::lockInternal() QT_MUTEX_LOCK_NOEXCEPT
{
    if (futexAvailable()) {
        // note we must set to dummyFutexValue because there could be other threads
        // also waiting
        while (d_ptr.fetchAndStoreAcquire(dummyFutexValue()) != nullptr) {
            // successfully set the waiting bit, now sleep
            futexWait(d_ptr, dummyFutexValue());

            // we got woken up, so try to acquire the mutex
        }
        Q_ASSERT(d_ptr.loadRelaxed());
    } else {
        lockInternal(-1);
    }
}

/*!
    \internal helper for lock(int)
 */
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
bool QBasicMutex::lockInternal(int timeout) QT_MUTEX_LOCK_NOEXCEPT
{
    if (timeout == 0)
        return false;

    return lockInternal(QDeadlineTimer(timeout));
}
#endif

/*!
    \internal helper for tryLock(QDeadlineTimer)
 */
bool QBasicMutex::lockInternal(QDeadlineTimer deadlineTimer) QT_MUTEX_LOCK_NOEXCEPT
{
    qint64 remainingTime = deadlineTimer.remainingTimeNSecs();
    if (remainingTime == 0)
        return false;

    if (futexAvailable()) {
        if (Q_UNLIKELY(remainingTime < 0)) {    // deadlineTimer.isForever()
            lockInternal();
            return true;
        }

        // The mutex is already locked, set a bit indicating we're waiting.
        // Note we must set to dummyFutexValue because there could be other threads
        // also waiting.
        if (d_ptr.fetchAndStoreAcquire(dummyFutexValue()) == nullptr)
            return true;

        Q_FOREVER {
            if (!futexWait(d_ptr, dummyFutexValue(), remainingTime))
                return false;

            // We got woken up, so must try to acquire the mutex. We must set
            // to dummyFutexValue() again because there could be other threads
            // waiting.
            if (d_ptr.fetchAndStoreAcquire(dummyFutexValue()) == nullptr)
                return true;

            // calculate the remaining time
            remainingTime = deadlineTimer.remainingTimeNSecs();
            if (remainingTime <= 0)
                return false;
        }
    }

#if !defined(QT_ALWAYS_USE_FUTEX)
    while (!fastTryLock()) {
        QMutexPrivate *copy = d_ptr.loadAcquire();
        if (!copy) // if d is 0, the mutex is unlocked
            continue;

        if (copy == dummyLocked()) {
            if (remainingTime == 0)
                return false;
            // The mutex is locked but does not have a QMutexPrivate yet.
            // we need to allocate a QMutexPrivate
            QMutexPrivate *newD = QMutexPrivate::allocate();
            if (!d_ptr.testAndSetOrdered(dummyLocked(), newD)) {
                //Either the mutex is already unlocked, or another thread already set it.
                newD->deref();
                continue;
            }
            copy = newD;
            //the d->refCount is already 1 the deref will occurs when we unlock
        }

        QMutexPrivate *d = static_cast<QMutexPrivate *>(copy);
        if (remainingTime == 0 && !d->possiblyUnlocked.loadRelaxed())
            return false;

        // At this point we have a pointer to a QMutexPrivate. But the other thread
        // may unlock the mutex at any moment and release the QMutexPrivate to the pool.
        // We will try to reference it to avoid unlock to release it to the pool to make
        // sure it won't be released. But if the refcount is already 0 it has been released.
        if (!d->ref())
            continue; //that QMutexPrivate was already released

        // We now hold a reference to the QMutexPrivate. It won't be released and re-used.
        // But it is still possible that it was already re-used by another QMutex right before
        // we did the ref(). So check if we still hold a pointer to the right mutex.
        if (d != d_ptr.loadAcquire()) {
            //Either the mutex is already unlocked, or relocked with another mutex
            d->deref();
            continue;
        }

        // In this part, we will try to increment the waiters count.
        // We just need to take care of the case in which the old_waiters
        // is set to the BigNumber magic value set in unlockInternal()
        int old_waiters;
        do {
            old_waiters = d->waiters.loadAcquire();
            if (old_waiters == -QMutexPrivate::BigNumber) {
                // we are unlocking, and the thread that unlocks is about to change d to 0
                // we try to acquire the mutex by changing to dummyLocked()
                if (d_ptr.testAndSetAcquire(d, dummyLocked())) {
                    // Mutex acquired
                    d->deref();
                    return true;
                } else {
                    Q_ASSERT(d != d_ptr.loadRelaxed()); //else testAndSetAcquire should have succeeded
                    // Mutex is likely to bo 0, we should continue the outer-loop,
                    //  set old_waiters to the magic value of BigNumber
                    old_waiters = QMutexPrivate::BigNumber;
                    break;
                }
            }
        } while (!d->waiters.testAndSetRelaxed(old_waiters, old_waiters + 1));

        if (d != d_ptr.loadAcquire()) {
            // The mutex was unlocked before we incremented waiters.
            if (old_waiters != QMutexPrivate::BigNumber) {
                //we did not break the previous loop
                Q_ASSERT(d->waiters.loadRelaxed() >= 1);
                d->waiters.deref();
            }
            d->deref();
            continue;
        }

        if (d->wait(deadlineTimer)) {
            // reset the possiblyUnlocked flag if needed (and deref its corresponding reference)
            if (d->possiblyUnlocked.loadRelaxed() && d->possiblyUnlocked.testAndSetRelaxed(true, false))
                d->deref();
            d->derefWaiters(1);
            //we got the lock. (do not deref)
            Q_ASSERT(d == d_ptr.loadRelaxed());
            return true;
        } else {
            Q_ASSERT(remainingTime >= 0);
            // timed out
            d->derefWaiters(1);
            //There may be a race in which the mutex is unlocked right after we timed out,
            // and before we deref the waiters, so maybe the mutex is actually unlocked.
            // Set the possiblyUnlocked flag to indicate this possibility.
            if (!d->possiblyUnlocked.testAndSetRelaxed(false, true)) {
                // We keep a reference when possiblyUnlocked is true.
                // but if possiblyUnlocked was already true, we don't need to keep the reference.
                d->deref();
            }
            return false;
        }
    }
    Q_ASSERT(d_ptr.loadRelaxed() != 0);
    return true;
#else
    Q_UNREACHABLE();
#endif
}

/*!
    \internal
*/
void QBasicMutex::unlockInternal() noexcept
{
    QMutexPrivate *copy = d_ptr.loadAcquire();
    Q_ASSERT(copy); //we must be locked
    Q_ASSERT(copy != dummyLocked()); // testAndSetRelease(dummyLocked(), 0) failed

    if (futexAvailable()) {
        d_ptr.storeRelease(nullptr);
        return futexWakeOne(d_ptr);
    }

#if !defined(QT_ALWAYS_USE_FUTEX)
    QMutexPrivate *d = reinterpret_cast<QMutexPrivate *>(copy);

    // If no one is waiting for the lock anymore, we should reset d to 0x0.
    // Using fetchAndAdd, we atomically check that waiters was equal to 0, and add a flag
    // to the waiters variable (BigNumber). That way, we avoid the race in which waiters is
    // incremented right after we checked, because we won't increment waiters if is
    // equal to -BigNumber
    if (d->waiters.fetchAndAddRelease(-QMutexPrivate::BigNumber) == 0) {
        //there is no one waiting on this mutex anymore, set the mutex as unlocked (d = 0)
        if (d_ptr.testAndSetRelease(d, 0)) {
            // reset the possiblyUnlocked flag if needed (and deref its corresponding reference)
            if (d->possiblyUnlocked.loadRelaxed() && d->possiblyUnlocked.testAndSetRelaxed(true, false))
                d->deref();
        }
        d->derefWaiters(0);
    } else {
        d->derefWaiters(0);
        //there are thread waiting, transfer the lock.
        d->wakeUp();
    }
    d->deref();
#else
    Q_UNUSED(copy);
#endif
}

#if !defined(QT_ALWAYS_USE_FUTEX)
//The freelist management
namespace {
struct FreeListConstants : QFreeListDefaultConstants {
    enum { BlockCount = 4, MaxIndex=0xffff };
    static const int Sizes[BlockCount];
};
Q_CONSTINIT const int FreeListConstants::Sizes[FreeListConstants::BlockCount] = {
    16,
    128,
    1024,
    FreeListConstants::MaxIndex - (16 + 128 + 1024)
};

typedef QFreeList<QMutexPrivate, FreeListConstants> FreeList;
// We cannot use Q_GLOBAL_STATIC because it uses QMutex
Q_CONSTINIT static FreeList freeList_;
FreeList *freelist()
{
    return &freeList_;
}
}

QMutexPrivate *QMutexPrivate::allocate()
{
    int i = freelist()->next();
    QMutexPrivate *d = &(*freelist())[i];
    d->id = i;
    Q_ASSERT(d->refCount.loadRelaxed() == 0);
    Q_ASSERT(!d->possiblyUnlocked.loadRelaxed());
    Q_ASSERT(d->waiters.loadRelaxed() == 0);
    d->refCount.storeRelaxed(1);
    return d;
}

void QMutexPrivate::release()
{
    Q_ASSERT(refCount.loadRelaxed() == 0);
    Q_ASSERT(!possiblyUnlocked.loadRelaxed());
    Q_ASSERT(waiters.loadRelaxed() == 0);
    freelist()->release(id);
}

// atomically subtract "value" to the waiters, and remove the QMutexPrivate::BigNumber flag
void QMutexPrivate::derefWaiters(int value) noexcept
{
    int old_waiters;
    int new_waiters;
    do {
        old_waiters = waiters.loadRelaxed();
        new_waiters = old_waiters;
        if (new_waiters < 0) {
            new_waiters += QMutexPrivate::BigNumber;
        }
        new_waiters -= value;
    } while (!waiters.testAndSetRelaxed(old_waiters, new_waiters));
}
#endif

QT_END_NAMESPACE

#if defined(QT_ALWAYS_USE_FUTEX)
// nothing
#elif defined(Q_OS_DARWIN)
#  include "qmutex_mac.cpp"
#else
#  include "qmutex_unix.cpp"
#endif
