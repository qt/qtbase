// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QWAITCONDITION_P_H
#define QWAITCONDITION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience of
// qmutex.cpp and qmutex_unix.cpp. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>
#include <QtCore/QDeadlineTimer>
#include <QtCore/private/qglobal_p.h>

// This header always defines a class called "mutex" and one called
// "condition_variable", so those mustn't be used to mark ELF symbol
// visibility. Don't add more classes to this header!
// ELFVERSION:stop

#include <condition_variable>
#include <mutex>

// There's no feature macro for C++11 std::mutex, so we use the C++14 one
// for shared_mutex to detect it.
// Needed for: MinGW without gthreads, Integrity
#if __has_include(<shared_mutex>)
#  include <shared_mutex>
#endif

QT_BEGIN_NAMESPACE

namespace QtPrivate {

#if !defined(__cpp_lib_shared_timed_mutex)

enum class cv_status { no_timeout, timeout };
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
    cv_status wait_for(std::unique_lock<QtPrivate::mutex> &lock,
                            const std::chrono::duration<Rep, Period> &d)
    {
        return QWaitCondition::wait(lock.mutex(), QDeadlineTimer{d})
                ? cv_status::no_timeout
                : cv_status::timeout;
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
    cv_status wait_until(std::unique_lock<QtPrivate::mutex> &lock,
                              const std::chrono::time_point<Clock, Duration> &t)
    {
        return QWaitCondition::wait(lock.mutex(), QDeadlineTimer{t})
                ? cv_status::no_timeout
                : cv_status::timeout;
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

#else // C++11 threads

using mutex = std::mutex;
using condition_variable = std::condition_variable;

#endif // C++11 threads

} // namespace QtPrivate

QT_END_NAMESPACE

#endif /* QWAITCONDITION_P_H */
