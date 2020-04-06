/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "qtestsupport_core.h"

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

QT_BEGIN_NAMESPACE

Q_CORE_EXPORT void QTestPrivate::qSleep(int ms)
{
    Q_ASSERT(ms > 0);

#if defined(Q_OS_WINRT)
    WaitForSingleObjectEx(GetCurrentThread(), ms, true);
#elif defined(Q_OS_WIN)
    Sleep(uint(ms));
#else
    struct timespec ts = { time_t(ms / 1000), (ms % 1000) * 1000 * 1000 };
    nanosleep(&ts, nullptr);
#endif
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
    // Ideally this method would be implemented in terms of qWaitFor, with
    // a predicate that always returns false, but due to a compiler bug in
    // GCC 6 we can't do that.

    Q_ASSERT(QCoreApplication::instance());

    QDeadlineTimer timer(ms, Qt::PreciseTimer);
    int remaining = ms;
    do {
        QCoreApplication::processEvents(QEventLoop::AllEvents, remaining);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        remaining = timer.remainingTime();
        if (remaining <= 0)
            break;
        QTestPrivate::qSleep(qMin(10, remaining));
        remaining = timer.remainingTime();
    } while (remaining > 0);
}

QT_END_NAMESPACE
