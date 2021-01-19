/****************************************************************************
**
** Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
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
#ifndef QWAITCONDITION_P_H
#define QWAITCONDITION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qmutex.cpp, qmutex_unix.cpp, and qmutex_win.cpp.  This header
// file may change from version to version without notice, or even be
// removed.
//
// We mean it.
//

#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>
#include <QtCore/QDeadlineTimer>

#include <condition_variable>
#include <mutex>

QT_BEGIN_NAMESPACE

namespace QtPrivate
{

#if defined(Q_OS_INTEGRITY)

class condition_variable;

class mutex : private QMutex
{
    friend class QtPrivate::condition_variable;
public:
    // all special member functions are ok!
    // do not expose the (QMutex::Recursive) ctor
    // don't use 'using QMutex::lock;' etc as those have the wrong noexcept

    void lock() { return QMutex::lock(); }
    void unlock() { return QMutex::unlock(); }
    bool try_lock() { return QMutex::tryLock(); }
};

class condition_variable : private QWaitCondition
{
public:
    // all special member functions are ok!

    void notify_one() { QWaitCondition::wakeOne(); }
    void notify_all() { QWaitCondition::wakeAll(); }

    void wait(std::unique_lock<QtPrivate::mutex> &lock) { QWaitCondition::wait(lock.mutex()); }
    template <class Predicate>
    void wait(std::unique_lock<QtPrivate::mutex> &lock, Predicate p)
    {
        while (!p())
            wait(lock);
    }

    template <typename Rep, typename Period>
    std::cv_status wait_for(std::unique_lock<QtPrivate::mutex> &lock,
                            const std::chrono::duration<Rep, Period> &d)
    {
        return QWaitCondition::wait(lock.mutex(), QDeadlineTimer{d})
                ? std::cv_status::no_timeout
                : std::cv_status::timeout;
    }
    template <typename Rep, typename Period, typename Predicate>
    bool wait_for(std::unique_lock<QtPrivate::mutex> &lock,
                  const std::chrono::duration<Rep, Period> &d, Predicate p)
    {
        const auto timer = QDeadlineTimer{d};
        while (!p()) {
            if (!QWaitCondition::wait(lock.mutex(), timer))
                return p();
        }
        return true;
    }

    template <typename Clock, typename Duration>
    std::cv_status wait_until(std::unique_lock<QtPrivate::mutex> &lock,
                              const std::chrono::time_point<Clock, Duration> &t)
    {
        return QWaitCondition::wait(lock.mutex(), QDeadlineTimer{t})
                ? std::cv_status::no_timeout
                : std::cv_status::timeout;
    }

    template <typename Clock, typename Duration, typename Predicate>
    bool wait_until(std::unique_lock<QtPrivate::mutex> &lock,
                    const std::chrono::time_point<Clock, Duration> &t, Predicate p)
    {
        const auto timer = QDeadlineTimer{t};
        while (!p()) {
            if (!QWaitCondition::wait(lock.mutex(), timer))
                return p();
        }
        return true;
    }

};

#else // Integrity

using mutex = std::mutex;
using condition_variable = std::condition_variable;

#endif // Integrity

} // namespace QtPrivate

QT_END_NAMESPACE

#endif /* QWAITCONDITION_P_H */
