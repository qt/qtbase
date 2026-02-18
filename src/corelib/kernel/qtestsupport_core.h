// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTESTSUPPORT_CORE_H
#define QTESTSUPPORT_CORE_H

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdeadlinetimer.h>

#include <chrono>

QT_BEGIN_NAMESPACE

namespace QTest {

Q_CORE_EXPORT void qSleep(int ms);
Q_CORE_EXPORT void qSleep(std::chrono::milliseconds msecs);

extern Q_CORE_EXPORT std::atomic<std::chrono::milliseconds> defaultTryTimeout;

namespace Internal {
enum class WaitForResult {
    Failed = -1,
    NotYet = 0,
    Done = 1,
};

inline bool waitForMore(bool) { return true; }
inline bool waitForMore(WaitForResult value) { return value == WaitForResult::NotYet; }

inline bool waitForSucceeded(bool value) { return value; }
inline bool waitForSucceeded(WaitForResult value) { return value >= WaitForResult::Done; }
}

template <typename Functor>
[[nodiscard]] bool
qWaitFor(Functor predicate, QDeadlineTimer deadline = QDeadlineTimer(
    defaultTryTimeout.load(std::memory_order_relaxed)))
{
    using Internal::waitForMore; // customization point
    using Internal::waitForSucceeded; // customization point

    // We should not spin the event loop in case the predicate is already true,
    // otherwise we might send new events that invalidate the predicate.
    if (waitForSucceeded(predicate()))
        return true;

    // qWait() is expected to spin the event loop at least once, even when
    // called with a small timeout like 1ns.

    do {
        // We explicitly do not pass the remaining time to processEvents, as
        // that would keep spinning processEvents for the whole duration if
        // new events were posted as part of processing events, and we need
        // to return back to this function to check the predicate between
        // each pass of processEvents. Our own timer will take care of the
        // timeout.
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        if (auto predresult = predicate(); waitForSucceeded(predresult))
            return true;
        else if (!waitForMore(predresult))
            return false;

        using namespace std::chrono;

        if (const auto remaining = deadline.remainingTimeAsDuration(); remaining > 0ns)
            qSleep((std::min)(10ms, ceil<milliseconds>(remaining)));

    } while (!deadline.hasExpired());

    return waitForSucceeded(predicate()); // Last chance
}

template <typename Functor>
[[nodiscard]] bool qWaitFor(Functor predicate, int timeout)
{
    return qWaitFor(predicate, QDeadlineTimer{timeout, Qt::PreciseTimer});
}

Q_CORE_EXPORT void qWait(int ms);

Q_CORE_EXPORT void qWait(std::chrono::milliseconds msecs);

} // namespace QTest

QT_END_NAMESPACE

#endif
