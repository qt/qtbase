// Copyright (C) 2023 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFUTEX_WIN_P_H
#define QFUTEX_WIN_P_H

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

#include <private/qglobal_p.h>
#include <qdeadlinetimer.h>
#include <qtsan_impl.h>

#include <qt_windows.h>

#define QT_ALWAYS_USE_FUTEX

QT_BEGIN_NAMESPACE

namespace QtWindowsFutex {
constexpr inline bool futexAvailable() { return true; }

template <typename Atomic>
inline void futexWait(Atomic &futex, typename Atomic::Type expectedValue)
{
    QtTsan::futexRelease(&futex);
    WaitOnAddress(&futex, &expectedValue, sizeof(expectedValue), INFINITE);
    QtTsan::futexAcquire(&futex);
}
template <typename Atomic>
inline bool futexWait(Atomic &futex, typename Atomic::Type expectedValue, QDeadlineTimer deadline)
{
    using namespace std::chrono;
    BOOL r = WaitOnAddress(&futex, &expectedValue, sizeof(expectedValue), DWORD(deadline.remainingTime()));
    return r || GetLastError() != ERROR_TIMEOUT;
}
template <typename Atomic> inline void futexWakeAll(Atomic &futex)
{
    WakeByAddressAll(&futex);
}
template <typename Atomic> inline void futexWakeOne(Atomic &futex)
{
    WakeByAddressSingle(&futex);
}
} // namespace QtWindowsFutex
namespace QtFutex = QtWindowsFutex;

QT_END_NAMESPACE

#endif // QFUTEX_WIN_P_H
