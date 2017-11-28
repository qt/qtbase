/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QT_NO_THREAD
#include "qmutex.h"
#include "qwaitcondition.h"
#include "qdeadlinetimer.h"
#include "qdatetime.h"

QT_BEGIN_NAMESPACE

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
    d = new QSemaphorePrivate(n);
}

/*!
    Destroys the semaphore.

    \warning Destroying a semaphore that is in use may result in
    undefined behavior.
*/
QSemaphore::~QSemaphore()
{ delete d; }

/*!
    Tries to acquire \c n resources guarded by the semaphore. If \a n
    > available(), this call will block until enough resources are
    available.

    \sa release(), available(), tryAcquire()
*/
void QSemaphore::acquire(int n)
{
    Q_ASSERT_X(n >= 0, "QSemaphore::acquire", "parameter 'n' must be non-negative");
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

    QDeadlineTimer timer(timeout);
    QMutexLocker locker(&d->mutex);
    qint64 remainingTime = timer.remainingTime();
    while (n > d->avail && remainingTime != 0) {
        if (!d->cond.wait(locker.mutex(), remainingTime))
            return false;
        remainingTime = timer.remainingTime();
    }
    if (n > d->avail)
        return false;
    d->avail -= n;
    return true;


}

/*!
    \class QSemaphoreReleaser
    \brief The QSemaphoreReleaser class provides exception-safe deferral of a QSemaphore::release() call
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

    \code
    // ... do something that may throw or return early
    sem.release();
    \endcode

    If an early return is taken or an exception is thrown before the
    \c{sem.release()} call is reached, the semaphore is not released,
    possibly preventing the thread waiting in the corresponding
    \c{sem.acquire()} call from ever continuing execution.

    When using RAII instead:

    \code
    const QSemaphoreReleaser releaser(sem);
    // ... do something that may throw or early return
    // implicitly calls sem.release() here and at every other return in between
    \endcode

    this can no longer happen, because the compiler will make sure that
    the QSemaphoreReleaser destructor is always called, and therefore
    the semaphore is always released.

    QSemaphoreReleaser is move-enabled and can therefore be returned
    from functions to transfer responsibility for releasing a semaphore
    out of a function or a scope:

    \code
    { // some scope
        QSemaphoreReleaser releaser; // does nothing
        // ...
        if (someCondition) {
            releaser = QSemaphoreReleaser(sem);
            // ...
        }
        // ...
    } // conditionally calls sem.release(), depending on someCondition
    \endcode

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
    or by the last move assignment, if any. Otherwise, returns \c nullptr.
*/

/*!
    \fn QSemaphoreReleaser::cancel()

    Cancels this QSemaphoreReleaser such that the destructor will no longer
    call \c{semaphore()->release()}. Returns the value of semaphore()
    before this call. After this call, semaphore() will return \c nullptr.

    To enable again, assign a new QSemaphoreReleaser:

    \code
    releaser.cancel(); // avoid releasing old semaphore()
    releaser = QSemaphoreReleaser(sem, 42);
    // now will call sem.release(42) when 'releaser' is destroyed
    \endcode
*/


QT_END_NAMESPACE

#endif // QT_NO_THREAD
