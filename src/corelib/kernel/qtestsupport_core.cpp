// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtestsupport_core.h"

#include <thread>

using namespace std::chrono_literals;

QT_BEGIN_NAMESPACE

/*!
    \overload

    Sleeps for \a ms milliseconds, blocking execution of the test.

    Equivalent to calling:
    \code
    QTest::qSleep(std::chrono::milliseconds{ms});
    \endcode
*/
void QTest::qSleep(int ms)
{
    QTest::qSleep(std::chrono::milliseconds{ms});
}

/*!
    \since 6.7

    Sleeps for \a msecs, blocking execution of the test.

    This method will not do any event processing and will leave your test
    unresponsive. Network communication might time out while sleeping.
    Use \l {QTest::qWait()} to do non-blocking sleeping.

    \a msecs must be greater than 0ms.

    \note Starting from Qt 6.7, this function is implemented using
    \c {std::this_thread::sleep_for}, so the accuracy of time spent depends
    on the Standard Library implementation. Before Qt 6.7 this function called
    either \c nanosleep() on Unix or \c Sleep() on Windows, so the accuracy of
    time spent in this function depended on the operating system.

    Example:
    \snippet code/src_qtestlib_qtestcase.cpp 23

    \sa {QTest::qWait()}
*/
void QTest::qSleep(std::chrono::milliseconds msecs)
{
    Q_ASSERT(msecs > 0ms);
    std::this_thread::sleep_for(msecs);
}

/*! \fn template <typename Functor> bool QTest::qWaitFor(Functor predicate, int timeout)

    \since 5.10
    \overload

    Waits for \a timeout milliseconds or until the \a predicate returns true.

    This is equivalent to calling:
    \code
    qWaitFor(predicate, QDeadlineTimer(timeout));
    \endcode
*/

/*! \fn template <typename Functor> bool QTest::qWaitFor(Functor predicate, QDeadlineTimer deadline)
    \since 6.7

    Waits until \a deadline has expired, or until \a predicate returns true, whichever
    happens first.

    Returns \c true if \a predicate returned true at any point, otherwise returns \c false.

    Example:

    \snippet code/src_corelib_kernel_qtestsupport_core.cpp 2

    The code above will wait for the object to become ready, for a
    maximum of three seconds.
*/

/*!
    \overload

    Waits for \a msecs. Equivalent to calling:
    \code
    QTest::qWait(std::chrono::milliseconds{msecs});
    \endcode
*/
Q_CORE_EXPORT void QTest::qWait(int msecs)
{
    qWait(std::chrono::milliseconds{msecs});
}

/*!
    \since 6.7

    Waits for \a msecs. While waiting, events will be processed and
    your test will stay responsive to user interface events or network communication.

    Example:

    \snippet code/src_corelib_kernel_qtestsupport_core.cpp 1

    The code above will wait until the network server is responding for a
    maximum of about 12.5 seconds.

    The \l{QTRY_COMPARE()}{QTRY_*} macros are usually a better choice than
    qWait(). qWait() always pauses for the full timeout, which can leave the
    test idle and slow down execution.

    The \c {QTRY_*} macros poll the condition until it succeeds or the timeout
    expires. Your test therefore continues as soon as possible and stays more
    reliable. If the condition still fails, the macros double the timeout once
    and report the new value so that you can adjust it.

    For example, rewrite the code above as:

    \code
        QTRY_VERIFY_WITH_TIMEOUT(!myNetworkServerNotResponding(), 12.5s);
    \endcode

    \sa QTest::qSleep(), QSignalSpy::wait(), QTRY_VERIFY_WITH_TIMEOUT()
*/
Q_CORE_EXPORT void QTest::qWait(std::chrono::milliseconds msecs)
{
    // Ideally this method would be implemented in terms of qWaitFor(), with a
    // predicate that always returns false, but qWaitFor() uses the 1-arg overload
    // of processEvents(), which doesn't handle events posted in this round of event
    // processing, which, together with the 10ms qSleep() after every processEvents(),
    // lead to a 10x slow-down in some webengine tests.

    Q_ASSERT(QCoreApplication::instance());

    using namespace std::chrono;

    QDeadlineTimer deadline(msecs, Qt::PreciseTimer);

    do {
        QCoreApplication::processEvents(QEventLoop::AllEvents, deadline);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        // If dealine is Forever, processEvents() has already looped forever
        if (deadline.isForever())
            break;

        msecs = ceil<milliseconds>(deadline.remainingTimeAsDuration());
        if (msecs == 0ms)
            break;

        QTest::qSleep(std::min(10ms, msecs));
    } while (!deadline.hasExpired());
}

QT_END_NAMESPACE
