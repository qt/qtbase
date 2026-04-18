// Copyright (C) 2023 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFUTEX_MAC_P_H
#define QFUTEX_MAC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qdeadlinetimer.h>
#include <qtsan_impl.h>
#include <private/qglobal_p.h>

#include <os/os_sync_wait_on_address.h>

#define QT_ALWAYS_USE_FUTEX

QT_BEGIN_NAMESPACE

namespace QtDarwinFutex {

constexpr bool futexAvailable() { return true; }

template <typename Atomic>
inline void futexWait(Atomic &futex, typename Atomic::Type expectedValue)
{
    QtTsan::futexRelease(&futex);
    static_assert(sizeof(typename Atomic::Type) == 4 || sizeof(typename Atomic::Type) == 8);
    os_sync_wait_on_address(&futex, quint64(expectedValue), sizeof(typename Atomic::Type),
                            OS_SYNC_WAIT_ON_ADDRESS_NONE);
    QtTsan::futexAcquire(&futex);
}

template <typename Atomic>
inline bool futexWait(Atomic &futex, typename Atomic::Type expectedValue, QDeadlineTimer timer)
{
    QtTsan::futexRelease(&futex);
    static_assert(sizeof(typename Atomic::Type) == 4 || sizeof(typename Atomic::Type) == 8);
    int r = os_sync_wait_on_address_with_timeout(
            &futex, quint64(expectedValue), sizeof(typename Atomic::Type),
            OS_SYNC_WAIT_ON_ADDRESS_NONE, OS_CLOCK_MACH_ABSOLUTE_TIME, timer.remainingTimeNSecs());
    QtTsan::futexAcquire(&futex);
    return r >= 0 || errno != ETIMEDOUT;
}

template <typename Atomic>
inline void futexWakeAll(Atomic &futex)
{
    static_assert(sizeof(typename Atomic::Type) == 4 || sizeof(typename Atomic::Type) == 8);
    os_sync_wake_by_address_all(&futex, sizeof(typename Atomic::Type),
                                OS_SYNC_WAKE_BY_ADDRESS_NONE);
}

template <typename Atomic>
inline void futexWakeOne(Atomic &futex)
{
    static_assert(sizeof(typename Atomic::Type) == 4 || sizeof(typename Atomic::Type) == 8);
    os_sync_wake_by_address_any(&futex, sizeof(typename Atomic::Type),
                                OS_SYNC_WAKE_BY_ADDRESS_NONE);
}
} //namespace QtDarwinFutex

namespace QtFutex = QtDarwinFutex;

QT_END_NAMESPACE

#endif // QFUTEX_MAC_P_H
