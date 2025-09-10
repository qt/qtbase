// Copyright (C) 2023 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

// The Darwin kernel exposes a set of __ulock_{wait,wait2,wake} APIs in
// https://github.com/apple-oss-distributions/xnu/blob/xnu-8792.81.2/bsd/sys/ulock.h,
// but these APIs are marked as private, so we cannot rely on them being
// stable, nor we can use these APIs in builds of Qt intended for
// the Apple App Store. By wholesale disabling the use of the APIs
// in App Store compliant builds, and runtime checking availability
// of the APIs when we do build them in, we should be safe, unless
// the semantics of the APIs change in ways we haven't accounted for,
// but that's a risk we're willing to take.

#if QT_CONFIG(appstore_compliant)
QT_BEGIN_NAMESPACE
namespace QtFutex = QtDummyFutex;
QT_END_NAMESPACE
#else

extern "C" {
// -------- BEGIN OS Declarations --------
// Source: https://github.com/apple-oss-distributions/xnu/blob/xnu-8792.81.2/bsd/sys/ulock.h
// Modification: added __attribute((__weak__))
// Copyright (c) 2015 Apple Inc. All rights reserved.

__attribute((__weak__))
extern int __ulock_wait2(uint32_t operation, void *addr, uint64_t value,
    uint64_t timeout, uint64_t value2);
__attribute((__weak__))
extern int __ulock_wake(uint32_t operation, void *addr, uint64_t wake_value);

/*
 * operation bits [7, 0] contain the operation code.
 */
#define UL_COMPARE_AND_WAIT             1
#define UL_COMPARE_AND_WAIT_SHARED      3
#define UL_COMPARE_AND_WAIT64           5
#define UL_COMPARE_AND_WAIT64_SHARED    6

/*
 * operation bits [15, 8] contain the flags for __ulock_wake
 */
#define ULF_WAKE_ALL                    0x00000100
#define ULF_WAKE_THREAD                 0x00000200
#define ULF_WAKE_ALLOW_NON_OWNER        0x00000400

/*
 * operation bits [15, 8] contain the flags for __ulock_wake
 */
#define ULF_WAKE_ALL                    0x00000100
#define ULF_WAKE_THREAD                 0x00000200
#define ULF_WAKE_ALLOW_NON_OWNER        0x00000400

/*
 * operation bits [31, 24] contain the generic flags
 */
#define ULF_NO_ERRNO                    0x01000000

// -------- END OS Declarations --------
} // extern "C"

QT_BEGIN_NAMESPACE

namespace QtDarwinFutex {

/*not constexpr*/ inline bool futexAvailable() { return __ulock_wake && __ulock_wait2; }

template <typename Atomic>
inline uint32_t baseOperation(Atomic &)
{
    static_assert(sizeof(Atomic) >= sizeof(quint32), "Can only operate on 32- or 64-bit atomics");

    uint32_t operation = ULF_NO_ERRNO;
    if (sizeof(Atomic) == sizeof(quint32))
        operation |= UL_COMPARE_AND_WAIT;
    else
        operation |= UL_COMPARE_AND_WAIT64;
    return operation;
}

template <typename Atomic> inline int
do_wait(Atomic &futex, typename Atomic::Type expectedValue, QDeadlineTimer timer)
{
    // source code inspection shows __ulock_wait2 uses nanoseconds for timeout
    QtTsan::futexRelease(&futex);
    int ret = __ulock_wait2(baseOperation(futex), &futex, uint64_t(expectedValue),
                            timer.remainingTimeNSecs(), 0);
    QtTsan::futexAcquire(&futex);
    return ret;
}

template <typename Atomic>
inline void futexWait(Atomic &futex, typename Atomic::Type expectedValue)
{
    do_wait(futex, expectedValue, {});
}

template <typename Atomic>
inline bool futexWait(Atomic &futex, typename Atomic::Type expectedValue, QDeadlineTimer timer)
{
    int r = do_wait(futex, expectedValue, timer);
    return r == 0 || r != -ETIMEDOUT;
}

template <typename Atomic> inline void futexWakeAll(Atomic &futex)
{
    __ulock_wake(baseOperation(futex) | ULF_WAKE_ALL, &futex, 0);
}

template <typename Atomic> inline void futexWakeOne(Atomic &futex)
{
    __ulock_wake(baseOperation(futex), &futex, 0);
}
} //namespace QtDarwinMutex

namespace QtFutex = QtDarwinFutex;

QT_END_NAMESPACE

#endif // QT_CONFIG(appstore_compliant)

#endif // QFUTEX_MAC_P_H
