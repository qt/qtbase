/****************************************************************************
**
** Copyright (C) 2017 Intel Corporation.
** Copyright (C) 2022 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
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

#ifndef QTSAN_IMPL_H
#define QTSAN_IMPL_H

#include <QtCore/qglobal.h>

#if (__has_feature(thread_sanitizer) || defined(__SANITIZE_THREAD__)) && __has_include(<sanitizer/tsan_interface.h>)
#  define QT_BUILDING_UNDER_TSAN
#  include <sanitizer/tsan_interface.h>
#endif

QT_BEGIN_NAMESPACE

namespace QtTsan {
#ifdef QT_BUILDING_UNDER_TSAN
inline void futexAcquire(void *addr, void *addr2 = nullptr)
{
    // A futex call ensures total ordering on the futex words
    // (in either success or failure of the call). Instruct TSAN accordingly,
    // as TSAN does not understand the futex(2) syscall (or equivalent).
    ::__tsan_acquire(addr);
    if (addr2)
        ::__tsan_acquire(addr2);
}

inline void futexRelease(void *addr, void *addr2 = nullptr)
{
    if (addr2)
        ::__tsan_release(addr2);
    ::__tsan_release(addr);
}

inline void mutexPreLock(void *addr, unsigned flags)
{
    ::__tsan_mutex_pre_lock(addr, flags);
}

inline void mutexPostLock(void *addr, unsigned flags, int recursion)
{
    ::__tsan_mutex_post_lock(addr, flags, recursion);
}

inline void mutexPreUnlock(void *addr, unsigned flags)
{
    ::__tsan_mutex_pre_unlock(addr, flags);
}

inline void mutexPostUnlock(void *addr, unsigned flags)
{
    ::__tsan_mutex_post_unlock(addr, flags);
}

enum : unsigned {
    MutexWriteReentrant = ::__tsan_mutex_write_reentrant,
    TryLock = ::__tsan_mutex_try_lock,
    TryLockFailed = ::__tsan_mutex_try_lock_failed,
};
#else
inline void futexAcquire(void *, void * = nullptr) {}
inline void futexRelease(void *, void * = nullptr) {}

enum : unsigned {
    MutexWriteReentrant,
    TryLock,
    TryLockFailed,
};
inline void mutexPreLock(void *, unsigned) {}
inline void mutexPostLock(void *, unsigned, int) {}
inline void mutexPreUnlock(void *, unsigned) {}
inline void mutexPostUnlock(void *, unsigned) {}
#endif // QT_BUILDING_UNDER_TSAN
} // namespace QtTsan

QT_END_NAMESPACE

#endif // QTSAN_IMPL_H
