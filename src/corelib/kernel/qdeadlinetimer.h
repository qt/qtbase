/****************************************************************************
**
** Copyright (C) 2016 Intel Corporation.
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

    Q_DECL_CONSTEXPR QDeadlineTimer(Qt::TimerType type_ = Qt::CoarseTimer) noexcept
        : t1(0), t2(0), type(type_) {}
    Q_DECL_CONSTEXPR QDeadlineTimer(ForeverConstant, Qt::TimerType type_ = Qt::CoarseTimer) noexcept
        : t1(std::numeric_limits<qint64>::max()), t2(0), type(type_) {}
    explicit QDeadlineTimer(qint64 msecs, Qt::TimerType type = Qt::CoarseTimer) noexcept;

    void swap(QDeadlineTimer &other) noexcept
    { qSwap(t1, other.t1); qSwap(t2, other.t2); qSwap(type, other.type); }

    Q_DECL_CONSTEXPR bool isForever() const noexcept
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

Q_DECLARE_SHARED(QDeadlineTimer)

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QDeadlineTimer)

#endif // QDEADLINETIMER_H
