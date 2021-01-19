/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QTESTSUPPORT_CORE_H
#define QTESTSUPPORT_CORE_H

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdeadlinetimer.h>

QT_BEGIN_NAMESPACE

namespace QTestPrivate {
Q_CORE_EXPORT void qSleep(int ms);
}

namespace QTest {

template <typename Functor>
Q_REQUIRED_RESULT static bool qWaitFor(Functor predicate, int timeout = 5000)
{
    // We should not spin the event loop in case the predicate is already true,
    // otherwise we might send new events that invalidate the predicate.
    if (predicate())
        return true;

    // qWait() is expected to spin the event loop, even when called with a small
    // timeout like 1ms, so we we can't use a simple while-loop here based on
    // the deadline timer not having timed out. Use do-while instead.

    int remaining = timeout;
    QDeadlineTimer deadline(remaining, Qt::PreciseTimer);

    do {
        // We explicitly do not pass the remaining time to processEvents, as
        // that would keep spinning processEvents for the whole duration if
        // new events were posted as part of processing events, and we need
        // to return back to this function to check the predicate between
        // each pass of processEvents. Our own timer will take care of the
        // timeout.
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        remaining = deadline.remainingTime();
        if (remaining > 0)
            QTestPrivate::qSleep(qMin(10, remaining));

        if (predicate())
            return true;

        remaining = deadline.remainingTime();
    } while (remaining > 0);

    return predicate(); // Last chance
}

Q_CORE_EXPORT void qWait(int ms);

} // namespace QTest

QT_END_NAMESPACE

#endif
