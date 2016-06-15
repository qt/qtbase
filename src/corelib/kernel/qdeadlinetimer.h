/****************************************************************************
**
** Copyright (C) 2016 Intel Corporation.
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

#ifndef QDEADLINETIMER_H
#define QDEADLINETIMER_H

#include <QtCore/qelapsedtimer.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qpair.h>

#ifdef max
// un-pollute the namespace. We need std::numeric_limits::max() and std::chrono::duration::max()
#  undef max
#endif

#include <limits>

#if __has_include(<chrono>)
#  include <chrono>
#endif

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QDeadlineTimer
{
public:
    enum ForeverConstant { Forever };

    constexpr QDeadlineTimer(Qt::TimerType type_ = Qt::CoarseTimer) noexcept
        : t1(0), t2(0), type(type_) {}
    constexpr QDeadlineTimer(ForeverConstant, Qt::TimerType type_ = Qt::CoarseTimer) noexcept
        : t1(std::numeric_limits<qint64>::max()), t2(0), type(type_) {}
    explicit QDeadlineTimer(qint64 msecs, Qt::TimerType type = Qt::CoarseTimer) noexcept;

    void swap(QDeadlineTimer &other) noexcept
    { qSwap(t1, other.t1); qSwap(t2, other.t2); qSwap(type, other.type); }

    constexpr bool isForever() const noexcept
    { return t1 == (std::numeric_limits<qint64>::max)(); }
    bool hasExpired() const noexcept;

    Qt::TimerType timerType() const noexcept
    { return Qt::TimerType(type & 0xff); }
    void setTimerType(Qt::TimerType type);

    qint64 remainingTime() const noexcept;
    qint64 remainingTimeNSecs() const noexcept;
    void setRemainingTime(qint64 msecs, Qt::TimerType type = Qt::CoarseTimer) noexcept;
    void setPreciseRemainingTime(qint64 secs, qint64 nsecs = 0,
                                 Qt::TimerType type = Qt::CoarseTimer) noexcept;

    qint64 deadline() const noexcept Q_DECL_PURE_FUNCTION;
    qint64 deadlineNSecs() const noexcept Q_DECL_PURE_FUNCTION;
    void setDeadline(qint64 msecs, Qt::TimerType timerType = Qt::CoarseTimer) noexcept;
    void setPreciseDeadline(qint64 secs, qint64 nsecs = 0,
                            Qt::TimerType type = Qt::CoarseTimer) noexcept;

    static QDeadlineTimer addNSecs(QDeadlineTimer dt, qint64 nsecs) noexcept Q_DECL_PURE_FUNCTION;
    static QDeadlineTimer current(Qt::TimerType timerType = Qt::CoarseTimer) noexcept;

    friend bool operator==(QDeadlineTimer d1, QDeadlineTimer d2) noexcept
    { return d1.t1 == d2.t1 && d1.t2 == d2.t2; }
    friend bool operator!=(QDeadlineTimer d1, QDeadlineTimer d2) noexcept
    { return !(d1 == d2); }
    friend bool operator<(QDeadlineTimer d1, QDeadlineTimer d2) noexcept
    { return d1.t1 < d2.t1 || (d1.t1 == d2.t1 && d1.t2 < d2.t2); }
    friend bool operator<=(QDeadlineTimer d1, QDeadlineTimer d2) noexcept
    { return d1 == d2 || d1 < d2; }
    friend bool operator>(QDeadlineTimer d1, QDeadlineTimer d2) noexcept
    { return d2 < d1; }
    friend bool operator>=(QDeadlineTimer d1, QDeadlineTimer d2) noexcept
    { return !(d1 < d2); }

    friend Q_CORE_EXPORT QDeadlineTimer operator+(QDeadlineTimer dt, qint64 msecs);
    friend QDeadlineTimer operator+(qint64 msecs, QDeadlineTimer dt)
    { return dt + msecs; }
    friend QDeadlineTimer operator-(QDeadlineTimer dt, qint64 msecs)
    { return dt + (-msecs); }
    friend qint64 operator-(QDeadlineTimer dt1, QDeadlineTimer dt2)
    { return (dt1.deadlineNSecs() - dt2.deadlineNSecs()) / (1000 * 1000); }
    QDeadlineTimer &operator+=(qint64 msecs)
    { *this = *this + msecs; return *this; }
    QDeadlineTimer &operator-=(qint64 msecs)
    { *this = *this + (-msecs); return *this; }

#if __has_include(<chrono>) || defined(Q_CLANG_QDOC)
    template <class Clock, class Duration>
    QDeadlineTimer(std::chrono::time_point<Clock, Duration> deadline_,
                   Qt::TimerType type_ = Qt::CoarseTimer) : t2(0)
    { setDeadline(deadline_, type_); }
    template <class Clock, class Duration>
    QDeadlineTimer &operator=(std::chrono::time_point<Clock, Duration> deadline_)
    { setDeadline(deadline_); return *this; }

    template <class Clock, class Duration>
    void setDeadline(std::chrono::time_point<Clock, Duration> deadline_,
                     Qt::TimerType type_ = Qt::CoarseTimer)
    { setRemainingTime(deadline_ == deadline_.max() ? Duration::max() : deadline_ - Clock::now(), type_); }

    template <class Clock, class Duration = typename Clock::duration>
    std::chrono::time_point<Clock, Duration> deadline() const
    {
        auto val = std::chrono::nanoseconds(rawRemainingTimeNSecs()) + Clock::now();
        return std::chrono::time_point_cast<Duration>(val);
    }

    template <class Rep, class Period>
    QDeadlineTimer(std::chrono::duration<Rep, Period> remaining, Qt::TimerType type_ = Qt::CoarseTimer)
        : t2(0)
    { setRemainingTime(remaining, type_); }

    template <class Rep, class Period>
    QDeadlineTimer &operator=(std::chrono::duration<Rep, Period> remaining)
    { setRemainingTime(remaining); return *this; }

    template <class Rep, class Period>
    void setRemainingTime(std::chrono::duration<Rep, Period> remaining, Qt::TimerType type_ = Qt::CoarseTimer)
    {
        if (remaining == remaining.max())
            *this = QDeadlineTimer(Forever, type_);
        else
            setPreciseRemainingTime(0, std::chrono::nanoseconds(remaining).count(), type_);
    }

    std::chrono::nanoseconds remainingTimeAsDuration() const noexcept
    {
        if (isForever())
            return std::chrono::nanoseconds::max();
        qint64 nsecs = rawRemainingTimeNSecs();
        if (nsecs <= 0)
            return std::chrono::nanoseconds::zero();
        return std::chrono::nanoseconds(nsecs);
    }

    template <class Rep, class Period>
    friend QDeadlineTimer operator+(QDeadlineTimer dt, std::chrono::duration<Rep, Period> value)
    { return QDeadlineTimer::addNSecs(dt, std::chrono::duration_cast<std::chrono::nanoseconds>(value).count()); }
    template <class Rep, class Period>
    friend QDeadlineTimer operator+(std::chrono::duration<Rep, Period> value, QDeadlineTimer dt)
    { return dt + value; }
    template <class Rep, class Period>
    friend QDeadlineTimer operator+=(QDeadlineTimer &dt, std::chrono::duration<Rep, Period> value)
    { return dt = dt + value; }
#endif

private:
    qint64 t1;
    unsigned t2;
    unsigned type;

    qint64 rawRemainingTimeNSecs() const noexcept;

public:
    // This is not a public function, it's here only for Qt's internal convenience...
    QPair<qint64, unsigned> _q_data() const { return qMakePair(t1, t2); }
};

#if QT_HAS_INCLUDE(<chrono>) && (defined(Q_OS_DARWIN) || defined(Q_OS_LINUX) || (defined(Q_CC_MSVC) && Q_CC_MSVC >= 1900))
// We know for these OS/compilers that the std::chrono::steady_clock uses the same
// reference time as QDeadlineTimer

template <> inline std::chrono::steady_clock::time_point
QDeadlineTimer::deadline<std::chrono::steady_clock, std::chrono::steady_clock::duration>() const
{
    return std::chrono::steady_clock::time_point(std::chrono::nanoseconds(deadlineNSecs()));
}

template <> inline void
QDeadlineTimer::setDeadline<std::chrono::steady_clock, std::chrono::steady_clock::duration>(std::chrono::steady_clock::time_point tp, Qt::TimerType type_)
{
    using namespace std::chrono;
    if (tp == tp.max()) {
        *this = Forever;
        type = type_;
    } else if (type_ != Qt::PreciseTimer) {
        // if we aren't using PreciseTimer, then we need to convert
        setPreciseRemainingTime(0, duration_cast<nanoseconds>(tp - steady_clock::now()).count(), type_);
    } else {
        setPreciseDeadline(0,
                           duration_cast<nanoseconds>(tp.time_since_epoch()).count(),
                           type_);
    }
}
#endif

Q_DECLARE_SHARED(QDeadlineTimer)

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QDeadlineTimer)

#endif // QDEADLINETIMER_H
