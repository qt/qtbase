// Copyright (C) 2023 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFUTEX_FREEBSD_P_H
#define QFUTEX_FREEBSD_P_H

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

#include <private/qcore_unix_p.h>
#include <qdeadlinetimer.h>

// https://man.freebsd.org/cgi/man.cgi?query=_umtx_op
#include <sys/umtx.h>

#define QT_ALWAYS_USE_FUTEX

QT_BEGIN_NAMESPACE

namespace QtFreeBSDFutex {
constexpr inline bool futexAvailable() { return true; }

template <typename Atomic>
inline int do_wait(Atomic &futex, typename Atomic::Type expectedValue, _umtx_time *tmp = nullptr)
{
    // FreeBSD UMTX_OP_WAIT does not apply acquire or release memory barriers,
    // so there are no QtTsan calls here.

    int op = UMTX_OP_WAIT_UINT_PRIVATE;
    if (sizeof(futex) > sizeof(quint32))
        op = UMTX_OP_WAIT;  // no _PRIVATE version

    // The timeout is passed in uaddr2, with its size in uaddr
    void *uaddr = reinterpret_cast<void *>(tmp ? sizeof(*tmp) : 0);
    void *uaddr2 = tmp;
    int ret = _umtx_op(&futex, op, u_long(expectedValue), uaddr, uaddr2);

    return ret;
}

template <typename Atomic>
inline void futexWait(Atomic &futex, typename Atomic::Type expectedValue)
{
    do_wait(futex, expectedValue);
}

template <typename Atomic>
inline bool futexWait(Atomic &futex, typename Atomic::Type expectedValue, QDeadlineTimer timer)
{
    struct _umtx_time tm = {};
    auto deadline = timer.deadline<std::chrono::steady_clock>();
    tm._timeout = durationToTimespec(deadline.time_since_epoch());
    tm._flags = UMTX_ABSTIME;
    tm._clockid = CLOCK_MONOTONIC;
    int r = do_wait(futex, expectedValue, &tm);
    return r == 0 || errno != ETIMEDOUT;
}

template <typename Atomic> inline void futexWakeOne(Atomic &futex)
{
    _umtx_op(&futex, UMTX_OP_WAKE_PRIVATE, 1, nullptr, nullptr);
}

template <typename Atomic> inline void futexWakeAll(Atomic &futex)
{
    _umtx_op(&futex, UMTX_OP_WAKE_PRIVATE, INT_MAX, nullptr, nullptr);
}
} //namespace QtFreeBSDFutex

namespace QtFutex = QtFreeBSDFutex;

QT_END_NAMESPACE

#endif // QFUTEX_FREEBSD_P_H
