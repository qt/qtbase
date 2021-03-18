/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2018 Intel Corporation.
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

#include "qsemaphore.h"
#include "qmutex.h"
#include "qfutex_p.h"
#include "qwaitcondition.h"
#include "qdeadlinetimer.h"
#include "qdatetime.h"

QT_BEGIN_NAMESPACE

using namespace QtFutex;

/*!
    \class QSemaphore
    \inmodule QtCore
    \brief The QSemaphore class provides a general counting semaphore.

    \threadsafe

    \ingroup thread

    A semaphore is a generalization of a mutex. While a mutex can
    only be locked once, it's possible to acquire a semaphore
    multiple times. Semaphores are typically used to protect a
    certain number of identical resources.

    Semaphores support two fundamental operations, acquire() and
    release():

    \list
    \li acquire(\e{n}) tries to acquire \e n resources. If there aren't
       that many resources available, the call will block until this
       is the case.
    \li release(\e{n}) releases \e n resources.
    \endlist

    There's also a tryAcquire() function that returns immediately if
    it cannot acquire the resources, and an available() function that
    returns the number of available resources at any time.

    Example:

    \snippet code/src_corelib_thread_qsemaphore.cpp 0

    A typical application of semaphores is for controlling access to
    a circular buffer shared by a producer thread and a consumer
    thread. The \l{Semaphores Example} shows how
    to use QSemaphore to solve that problem.

    A non-computing example of a semaphore would be dining at a
    restaurant. A semaphore is initialized with the number of chairs
    in the restaurant. As people arrive, they want a seat. As seats
    are filled, available() is decremented. As people leave, the
    available() is incremented, allowing more people to enter. If a
    party of 10 people want to be seated, but there are only 9 seats,
    those 10 people will wait, but a party of 4 people would be
    seated (taking the available seats to 5, making the party of 10
    people wait longer).

    \sa QSemaphoreReleaser, QMutex, QWaitCondition, QThread, {Semaphores Example}
*/

/*
    QSemaphore futex operation

    QSemaphore stores a 32-bit integer with the counter of currently available
    tokens (value between 0 and INT_MAX). When a thread attempts to acquire n
    tokens and the counter is larger than that, we perform a compare-and-swap
    with the new count. If that succeeds, the acquisition worked; if not, we
    loop again because the counter changed. If there were not enough tokens,
    we'll perform a futex-wait.

    Before we do, we set the high bit in the futex to indicate that semaphore
    is contended: that is, there's a thread waiting for more tokens. On
    release() for n tokens, we perform a fetch-and-add of n and then check if
    that high bit was set. If it was, then we clear that bit and perform a
    futex-wake on the semaphore to indicate the waiting threads can wake up and
    acquire tokens. Which ones get woken up is unspecified.

    If the system has the ability to wake up a precise number of threads, has
    Linux's FUTEX_WAKE_OP functionality, and is 64-bit, instead of using a
    single bit indicating a contended semaphore, we'll store the number of
    tokens *plus* total number of waiters in the high word. Additionally, all
    multi-token waiters will be waiting on that high word. So when releasing n
    tokens on those systems, we tell the kernel to wake up n single-token
    threads and all of the multi-token ones. Which threads get woken up is
    unspecified, but it's likely single-token threads will get woken up first.
 */

#if defined(FUTEX_OP) && QT_POINTER_SIZE > 4
static Q_CONSTEXPR bool futexHasWaiterCount = true;
#else
static Q_CONSTEXPR bool futexHasWaiterCount = false;
#endif

static const quintptr futexNeedsWakeAllBit =
        Q_UINT64_C(1) << (sizeof(quintptr) * CHAR_BIT - 1);

static int futexAvailCounter(quintptr v)
{
    // the low 31 bits
    if (futexHasWaiterCount) {
        // the high bit of the low word isn't used
        Q_ASSERT((v & 0x80000000U) == 0);

        // so we can be a little faster
        return int(unsigned(v));
    }
    return int(v & 0x7fffffffU);
}

static bool futexNeedsWake(quintptr v)
{
    // If we're counting waiters, the number of waiters is stored in the low 31
    // bits of the high word (that is, bits 32-62). If we're not, then we use
    // bit 31 to indicate anyone is waiting. Either way, if any bit 31 or above
    // is set, there are waiters.
    return v >> 31;
}

static QBasicAtomicInteger<quint32> *futexLow32(QBasicAtomicInteger<quintptr> *ptr)
{
    auto result = reinterpret_cast<QBasicAtomicInteger<quint32> *>(ptr);
#if Q_BYTE_ORDER == Q_BIG_ENDIAN && QT_POINTER_SIZE > 4
    ++result;
#endif
    return result;
}

static QBasicAtomicInteger<quint32> *futexHigh32(QBasicAtomicInteger<quintptr> *ptr)
{
    auto result = reinterpret_cast<QBasicAtomicInteger<quint32> *>(ptr);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN && QT_POINTER_SIZE > 4
    ++result;
#endif
    return result;
}

template <bool IsTimed> bool
futexSemaphoreTryAcquire_loop(QBasicAtomicInteger<quintptr> &u, quintptr curValue, quintptr nn, int timeout)
{
    QDeadlineTimer timer(IsTimed ? QDeadlineTimer(timeout) : QDeadlineTimer());
    qint64 remainingTime = timeout * Q_INT64_C(1000) * 1000;
    int n = int(unsigned(nn));

    // we're called after one testAndSet, so start by waiting first
    goto start_wait;

    forever {
        if (futexAvailCounter(curValue) >= n) {
            // try to acquire
            quintptr newValue = curValue - nn;
            if (u.testAndSetOrdered(curValue, newValue, curValue))
                return true;        // succeeded!
            continue;
        }

        // not enough tokens available, put us to wait
        if (remainingTime == 0)
            return false;

        // indicate we're waiting
start_wait:
        auto ptr = futexLow32(&u);
        if (n > 1 || !futexHasWaiterCount) {
            u.fetchAndOrRelaxed(futexNeedsWakeAllBit);
            curValue |= futexNeedsWakeAllBit;
            if (n > 1 && futexHasWaiterCount) {
                ptr = futexHigh32(&u);
                //curValue >>= 32;  // but this is UB in 32-bit, so roundabout:
                curValue = quint64(curValue) >> 32;
            }
        }

        if (IsTimed && remainingTime > 0) {
            bool timedout = !futexWait(*ptr, curValue, remainingTime);
            if (timedout)
                return false;
        } else {
            futexWait(*ptr, curValue);
        }

        curValue = u.loadAcquire();
        if (IsTimed)
            remainingTime = timer.remainingTimeNSecs();
    }
}

template <bool IsTimed> bool futexSemaphoreTryAcquire(QBasicAtomicInteger<quintptr> &u, int n, int timeout)
{
    // Try to acquire without waiting (we still loop because the testAndSet
    // call can fail).
    quintptr nn = unsigned(n);
    if (futexHasWaiterCount)
        nn |= quint64(nn) << 32;    // token count replicated in high word

    quintptr curValue = u.loadAcquire();
    while (futexAvailCounter(curValue) >= n) {
        // try to acquire
        quintptr newValue = curValue - nn;
        if (u.testAndSetOrdered(curValue, newValue, curValue))
            return true;        // succeeded!
    }
    if (timeout == 0)
        return false;

    // we need to wait
    quintptr oneWaiter = quintptr(Q_UINT64_C(1) << 32); // zero on 32-bit
    if (futexHasWaiterCount) {
        // increase the waiter count
        u.fetchAndAddRelaxed(oneWaiter);

        // We don't use the fetched value from above so futexWait() fails if
        // it changed after the testAndSetOrdered above.
        if ((quint64(curValue) >> 32) == 0x7fffffff)
            return false;       // overflow!
        curValue += oneWaiter;

        // Also adjust nn to subtract oneWaiter when we succeed in acquiring.
        nn += oneWaiter;
    }

    if (futexSemaphoreTryAcquire_loop<IsTimed>(u, curValue, nn, timeout))
        return true;

    if (futexHasWaiterCount) {
        // decrement the number of threads waiting
        Q_ASSERT(futexHigh32(&u)->loadRelaxed() & 0x7fffffffU);
        u.fetchAndSubRelaxed(oneWaiter);
    }
    return false;
}

class QSemaphorePrivate {
public:
    inline QSemaphorePrivate(int n) : avail(n) { }

    QMutex mutex;
    QWaitCondition cond;

    int avail;
};

/*!
    Creates a new semaphore and initializes the number of resources
    it guards to \a n (by default, 0).

    \sa release(), available()
*/
QSemaphore::QSemaphore(int n)
{
    Q_ASSERT_X(n >= 0, "QSemaphore", "parameter 'n' must be non-negative");
    if (futexAvailable()) {
        quintptr nn = unsigned(n);
        if (futexHasWaiterCount)
            nn |= quint64(nn) << 32;    // token count replicated in high word
        u.storeRelaxed(nn);
    } else {
        d = new QSemaphorePrivate(n);
    }
}

/*!
    Destroys the semaphore.

    \warning Destroying a semaphore that is in use may result in
    undefined behavior.
*/
QSemaphore::~QSemaphore()
{
    if (!futexAvailable())
        delete d;
}

/*!
    Tries to acquire \c n resources guarded by the semaphore. If \a n
    > available(), this call will block until enough resources are
    available.

    \sa release(), available(), tryAcquire()
*/
void QSemaphore::acquire(int n)
{
    Q_ASSERT_X(n >= 0, "QSemaphore::acquire", "parameter 'n' must be non-negative");

    if (futexAvailable()) {
        futexSemaphoreTryAcquire<false>(u, n, -1);
        return;
    }

    QMutexLocker locker(&d->mutex);
    while (n > d->avail)
        d->cond.wait(locker.mutex());
    d->avail -= n;
}

/*!
    Releases \a n resources guarded by the semaphore.

    This function can be used to "create" resources as well. For
    example:

    \snippet code/src_corelib_thread_qsemaphore.cpp 1

    QSemaphoreReleaser is a \l{http://en.cppreference.com/w/cpp/language/raii}{RAII}
    wrapper around this function.

    \sa acquire(), available(), QSemaphoreReleaser
*/
void QSemaphore::release(int n)
{
    Q_ASSERT_X(n >= 0, "QSemaphore::release", "parameter 'n' must be non-negative");

    if (futexAvailable()) {
        quintptr nn = unsigned(n);
        if (futexHasWaiterCount)
            nn |= quint64(nn) << 32;    // token count replicated in high word
        quintptr prevValue = u.fetchAndAddRelease(nn);
        if (futexNeedsWake(prevValue)) {
#ifdef FUTEX_OP
            if (!futexHasWaiterCount) {
                /*
                   On 32-bit systems, all waiters are waiting on the same address,
                   so we'll wake them all and ask the kernel to clear the high bit.

                   atomic {
                      int oldval = u;
                      u = oldval & ~(1 << 31);
                      futexWake(u, INT_MAX);
                      if (oldval == 0)       // impossible condition
                          futexWake(u, INT_MAX);
                   }
                */
                quint32 op = FUTEX_OP_ANDN | FUTEX_OP_OPARG_SHIFT;
                quint32 oparg = 31;
                quint32 cmp = FUTEX_OP_CMP_EQ;
                quint32 cmparg = 0;
                futexWakeOp(u, INT_MAX, INT_MAX, u, FUTEX_OP(op, oparg, cmp, cmparg));
            } else {
                /*
                   On 64-bit systems, the single-token waiters wait on the low half
                   and the multi-token waiters wait on the upper half. So we ask
                   the kernel to wake up n single-token waiters and all multi-token
                   waiters (if any), then clear the multi-token wait bit.

                   atomic {
                      int oldval = *upper;
                      *upper = oldval & ~(1 << 31);
                      futexWake(lower, n);
                      if (oldval < 0)   // sign bit set
                          futexWake(upper, INT_MAX);
                   }
                */
                quint32 op = FUTEX_OP_ANDN | FUTEX_OP_OPARG_SHIFT;
                quint32 oparg = 31;
                quint32 cmp = FUTEX_OP_CMP_LT;
                quint32 cmparg = 0;
                futexWakeOp(*futexLow32(&u), n, INT_MAX, *futexHigh32(&u), FUTEX_OP(op, oparg, cmp, cmparg));
            }
#else
            // Unset the bit and wake everyone. There are two possibibilies
            // under which a thread can set the bit between the AND and the
            // futexWake:
            // 1) it did see the new counter value, but it wasn't enough for
            //    its acquisition anyway, so it has to wait;
            // 2) it did not see the new counter value, in which case its
            //    futexWait will fail.
            u.fetchAndAndRelease(futexNeedsWakeAllBit - 1);
            futexWakeAll(u);
#endif
        }
        return;
    }

    QMutexLocker locker(&d->mutex);
    d->avail += n;
    d->cond.wakeAll();
}

/*!
    Returns the number of resources currently available to the
    semaphore. This number can never be negative.

    \sa acquire(), release()
*/
int QSemaphore::available() const
{
    if (futexAvailable())
        return futexAvailCounter(u.loadRelaxed());

    QMutexLocker locker(&d->mutex);
    return d->avail;
}

/*!
    Tries to acquire \c n resources guarded by the semaphore and
    returns \c true on success. If available() < \a n, this call
    immediately returns \c false without acquiring any resources.

    Example:

    \snippet code/src_corelib_thread_qsemaphore.cpp 2

    \sa acquire()
*/
bool QSemaphore::tryAcquire(int n)
{
    Q_ASSERT_X(n >= 0, "QSemaphore::tryAcquire", "parameter 'n' must be non-negative");

    if (futexAvailable())
        return futexSemaphoreTryAcquire<false>(u, n, 0);

    QMutexLocker locker(&d->mutex);
    if (n > d->avail)
        return false;
    d->avail -= n;
    return true;
}

/*!
    Tries to acquire \c n resources guarded by the semaphore and
    returns \c true on success. If available() < \a n, this call will
    wait for at most \a timeout milliseconds for resources to become
    available.

    Note: Passing a negative number as the \a timeout is equivalent to
    calling acquire(), i.e. this function will wait forever for
    resources to become available if \a timeout is negative.

    Example:

    \snippet code/src_corelib_thread_qsemaphore.cpp 3

    \sa acquire()
*/
bool QSemaphore::tryAcquire(int n, int timeout)
{
    Q_ASSERT_X(n >= 0, "QSemaphore::tryAcquire", "parameter 'n' must be non-negative");

    // We're documented to accept any negative value as "forever"
    // but QDeadlineTimer only accepts -1.
    timeout = qMax(timeout, -1);

    if (futexAvailable())
        return futexSemaphoreTryAcquire<true>(u, n, timeout);

    QDeadlineTimer timer(timeout);
    QMutexLocker locker(&d->mutex);
    while (n > d->avail && !timer.hasExpired()) {
        if (!d->cond.wait(locker.mutex(), timer))
            return false;
    }
    if (n > d->avail)
        return false;
    d->avail -= n;
    return true;


}

/*!
    \class QSemaphoreReleaser
    \brief The QSemaphoreReleaser class provides exception-safe deferral of a QSemaphore::release() call.
    \since 5.10
    \ingroup thread
    \inmodule QtCore

    \reentrant

    QSemaphoreReleaser can be used wherever you would otherwise use
    QSemaphore::release(). Constructing a QSemaphoreReleaser defers the
    release() call on the semaphore until the QSemaphoreReleaser is
    destroyed (see
    \l{http://en.cppreference.com/w/cpp/language/raii}{RAII pattern}).

    You can use this to reliably release a semaphore to avoid dead-lock
    in the face of exceptions or early returns:

    \snippet code/src_corelib_thread_qsemaphore.cpp 4

    If an early return is taken or an exception is thrown before the
    \c{sem.release()} call is reached, the semaphore is not released,
    possibly preventing the thread waiting in the corresponding
    \c{sem.acquire()} call from ever continuing execution.

    When using RAII instead:

    \snippet code/src_corelib_thread_qsemaphore.cpp 5

    this can no longer happen, because the compiler will make sure that
    the QSemaphoreReleaser destructor is always called, and therefore
    the semaphore is always released.

    QSemaphoreReleaser is move-enabled and can therefore be returned
    from functions to transfer responsibility for releasing a semaphore
    out of a function or a scope:

    \snippet code/src_corelib_thread_qsemaphore.cpp 6

    A QSemaphoreReleaser can be canceled by a call to cancel(). A canceled
    semaphore releaser will no longer call QSemaphore::release() in its
    destructor.

    \sa QMutexLocker
*/

/*!
    \fn QSemaphoreReleaser::QSemaphoreReleaser()

    Default constructor. Creates a QSemaphoreReleaser that does nothing.
*/

/*!
    \fn QSemaphoreReleaser::QSemaphoreReleaser(QSemaphore &sem, int n)

    Constructor. Stores the arguments and calls \a{sem}.release(\a{n})
    in the destructor.
*/

/*!
    \fn QSemaphoreReleaser::QSemaphoreReleaser(QSemaphore *sem, int n)

    Constructor. Stores the arguments and calls \a{sem}->release(\a{n})
    in the destructor.
*/

/*!
    \fn QSemaphoreReleaser::QSemaphoreReleaser(QSemaphoreReleaser &&other)

    Move constructor. Takes over responsibility to call QSemaphore::release()
    from \a other, which in turn is canceled.

    \sa cancel()
*/

/*!
    \fn QSemaphoreReleaser::operator=(QSemaphoreReleaser &&other)

    Move assignment operator. Takes over responsibility to call QSemaphore::release()
    from \a other, which in turn is canceled.

    If this semaphore releaser had the responsibility to call some QSemaphore::release()
    itself, it performs the call before taking over from \a other.

    \sa cancel()
*/

/*!
    \fn QSemaphoreReleaser::~QSemaphoreReleaser()

    Unless canceled, calls QSemaphore::release() with the arguments provided
    to the constructor, or by the last move assignment.
*/

/*!
    \fn QSemaphoreReleaser::swap(QSemaphoreReleaser &other)

    Exchanges the responsibilites of \c{*this} and \a other.

    Unlike move assignment, neither of the two objects ever releases its
    semaphore, if any, as a consequence of swapping.

    Therefore this function is very fast and never fails.
*/

/*!
    \fn QSemaphoreReleaser::semaphore() const

    Returns a pointer to the QSemaphore object provided to the constructor,
    or by the last move assignment, if any. Otherwise, returns \nullptr.
*/

/*!
    \fn QSemaphoreReleaser::cancel()

    Cancels this QSemaphoreReleaser such that the destructor will no longer
    call \c{semaphore()->release()}. Returns the value of semaphore()
    before this call. After this call, semaphore() will return \nullptr.

    To enable again, assign a new QSemaphoreReleaser:

    \snippet code/src_corelib_thread_qsemaphore.cpp 7
*/


QT_END_NAMESPACE
