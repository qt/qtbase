// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtestsupport_core.h"

#include <thread>

QT_BEGIN_NAMESPACE

/*!
    Sleeps for \a ms milliseconds, blocking execution of the
    test. qSleep() will not do any event processing and leave your test
    unresponsive. Network communication might time out while
    sleeping. Use \l {QTest::qWait()} to do non-blocking sleeping.

    \a ms must be greater than 0.

    \note Starting from Qt 6.7, this function is implemented using
    \c {std::this_thread::sleep_for}, so the accuracy of time spent depends
    on the Standard Library implementation. Before Qt 6.7 this function called
    either \c nanosleep() on Unix or \c Sleep() on Windows, so the accuracy of
    time spent in this function depended on the operating system.

    Example:
    \snippet code/src_qtestlib_qtestcase.cpp 23

    \sa {QTest::qWait()}
*/
Q_CORE_EXPORT void QTest::qSleep(int ms)
{
    Q_ASSERT(ms > 0);
    std::this_thread::sleep_for(std::chrono::milliseconds{ms});
}

/*! \fn template <typename Functor> bool QTest::qWaitFor(Functor predicate, int timeout)

    Waits for \a timeout milliseconds or until the \a predicate returns true.

    Returns \c true if the \a predicate returned true at any point, otherwise returns \c false.

    Example:

    \snippet code/src_corelib_kernel_qtestsupport_core_snippet.cpp 0

    The code above will wait for the object to become ready, for a
    maximum of three seconds.

    \since 5.10
*/


/*! \fn void QTest::qWait(int ms)

    Waits for \a ms milliseconds. While waiting, events will be processed and
    your test will stay responsive to user interface events or network communication.

    Example:

    \snippet code/src_corelib_kernel_qtestsupport_core.cpp 1

    The code above will wait until the network server is responding for a
    maximum of about 12.5 seconds.

    \sa QTest::qSleep(), QSignalSpy::wait()
*/
Q_CORE_EXPORT void QTest::qWait(int ms)
{
    // Ideally this method would be implemented in terms of qWaitFor(), with a
    // predicate that always returns false, but qWaitFor() uses the 1-arg overload
    // of processEvents(), which doesn't handle events posted in this round of event
    // processing, which, together with the 10ms qSleep() after every processEvents(),
    // lead to a 10x slow-down in some webengine tests.

    Q_ASSERT(QCoreApplication::instance());

    QDeadlineTimer timer(ms, Qt::PreciseTimer);
    int remaining = ms;
    do {
        QCoreApplication::processEvents(QEventLoop::AllEvents, remaining);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        remaining = timer.remainingTime();
        if (remaining <= 0)
            break;
        QTest::qSleep(qMin(10, remaining));
        remaining = timer.remainingTime();
    } while (remaining > 0);
}

QT_END_NAMESPACE
