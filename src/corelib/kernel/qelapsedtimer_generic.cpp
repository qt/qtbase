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

#include "qelapsedtimer.h"
#include "qdatetime.h"

QT_BEGIN_NAMESPACE

/*!
    Returns the clock type that this QElapsedTimer implementation uses.

    \sa isMonotonic()
*/
QElapsedTimer::ClockType QElapsedTimer::clockType() Q_DECL_NOTHROW
{
    return SystemTime;
}

/*!
    Returns \c true if this is a monotonic clock, false otherwise. See the
    information on the different clock types to understand which ones are
    monotonic.

    \sa clockType(), QElapsedTimer::ClockType
*/
bool QElapsedTimer::isMonotonic() Q_DECL_NOTHROW
{
    return false;
}

/*!
    Starts this timer. Once started, a timer value can be checked with elapsed() or msecsSinceReference().

    Normally, a timer is started just before a lengthy operation, such as:
    \snippet qelapsedtimer/main.cpp 0

    Also, starting a timer makes it valid again.

    \sa restart(), invalidate(), elapsed()
*/
void QElapsedTimer::start() Q_DECL_NOTHROW
{
    restart();
}

/*!
    Restarts the timer and returns the time elapsed since the previous start.
    This function is equivalent to obtaining the elapsed time with elapsed()
    and then starting the timer again with start(), but it does so in one
    single operation, avoiding the need to obtain the clock value twice.

    Calling this function on a QElapsedTimer that is invalid
    results in undefined behavior.

    The following example illustrates how to use this function to calibrate a
    parameter to a slow operation (for example, an iteration count) so that
    this operation takes at least 250 milliseconds:

    \snippet qelapsedtimer/main.cpp 3

    \sa start(), invalidate(), elapsed(), isValid()
*/
qint64 QElapsedTimer::restart() Q_DECL_NOTHROW
{
    qint64 old = t1;
    t1 = QDateTime::currentMSecsSinceEpoch();
    t2 = 0;
    return t1 - old;
}

/*! \since 4.8

    Returns the number of nanoseconds since this QElapsedTimer was last
    started.

    Calling this function on a QElapsedTimer that is invalid
    results in undefined behavior.

    On platforms that do not provide nanosecond resolution, the value returned
    will be the best estimate available.

    \sa start(), restart(), hasExpired(), invalidate()
*/
qint64 QElapsedTimer::nsecsElapsed() const Q_DECL_NOTHROW
{
    return elapsed() * 1000000;
}

/*!
    Returns the number of milliseconds since this QElapsedTimer was last
    started.

    Calling this function on a QElapsedTimer that is invalid
    results in undefined behavior.

    \sa start(), restart(), hasExpired(), isValid(), invalidate()
*/
qint64 QElapsedTimer::elapsed() const Q_DECL_NOTHROW
{
    return QDateTime::currentMSecsSinceEpoch() - t1;
}

/*!
    Returns the number of milliseconds between last time this QElapsedTimer
    object was started and its reference clock's start.

    This number is usually arbitrary for all clocks except the
    QElapsedTimer::SystemTime clock. For that clock type, this number is the
    number of milliseconds since January 1st, 1970 at 0:00 UTC (that is, it
    is the Unix time expressed in milliseconds).

    On Linux, Windows and OS X/iOS systems, this value is usually the time
    since the system boot, though it usually does not include the time the
    system has spent in sleep states.

    \sa clockType(), elapsed()
*/
qint64 QElapsedTimer::msecsSinceReference() const Q_DECL_NOTHROW
{
    return t1;
}

/*!
    Returns the number of milliseconds between this QElapsedTimer and \a
    other. If \a other was started before this object, the returned value
    will be negative. If it was started later, the returned value will be
    positive.

    The return value is undefined if this object or \a other were invalidated.

    \sa secsTo(), elapsed()
*/
qint64 QElapsedTimer::msecsTo(const QElapsedTimer &other) const Q_DECL_NOTHROW
{
    qint64 diff = other.t1 - t1;
    return diff;
}

/*!
    Returns the number of seconds between this QElapsedTimer and \a other. If
    \a other was started before this object, the returned value will be
    negative. If it was started later, the returned value will be positive.

    Calling this function on or with a QElapsedTimer that is invalid
    results in undefined behavior.

    \sa msecsTo(), elapsed()
*/
qint64 QElapsedTimer::secsTo(const QElapsedTimer &other) const Q_DECL_NOTHROW
{
    return msecsTo(other) / 1000;
}

/*!
    \relates QElapsedTimer

    Returns \c true if \a v1 was started before \a v2, false otherwise.

    The returned value is undefined if one of the two parameters is invalid
    and the other isn't. However, two invalid timers are equal and thus this
    function will return false.
*/
bool operator<(const QElapsedTimer &v1, const QElapsedTimer &v2) Q_DECL_NOTHROW
{
    return v1.t1 < v2.t1;
}

QT_END_NAMESPACE
