// Copyright (C) 2017 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFUTEX_P_H
#define QFUTEX_P_H

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
#include <QtCore/qtsan_impl.h>

QT_BEGIN_NAMESPACE

namespace QtDummyFutex {
    constexpr inline bool futexAvailable() { return false; }
    template <typename Atomic>
    inline bool futexWait(Atomic &, typename Atomic::Type, int = 0)
    { Q_UNREACHABLE_RETURN(false); }
    template <typename Atomic> inline void futexWakeOne(Atomic &)
    { Q_UNREACHABLE(); }
    template <typename Atomic> inline void futexWakeAll(Atomic &)
    { Q_UNREACHABLE(); }
}

QT_END_NAMESPACE

#if defined(Q_OS_LINUX) && !defined(QT_LINUXBASE)
// use Linux mutexes everywhere except for LSB builds
#  include <sys/syscall.h>
#  include <errno.h>
#  include <limits.h>
#  include <unistd.h>
#  include <asm/unistd.h>
#  include <linux/futex.h>
#  define QT_ALWAYS_USE_FUTEX

// if not defined in linux/futex.h
#  define FUTEX_PRIVATE_FLAG        128         // added in v2.6.22

// RISC-V does not supply __NR_futex
#  ifndef __NR_futex
#    define __NR_futex __NR_futex_time64
#  endif

QT_BEGIN_NAMESPACE
namespace QtLinuxFutex {
    constexpr inline bool futexAvailable() { return true; }
    inline int _q_futex(int *addr, int op, int val, quintptr val2 = 0,
                        int *addr2 = nullptr, int val3 = 0) noexcept
    {
        QtTsan::futexRelease(addr, addr2);

        // we use __NR_futex because some libcs (like Android's bionic) don't
        // provide SYS_futex etc.
        int result = syscall(__NR_futex, addr, op | FUTEX_PRIVATE_FLAG, val, val2, addr2, val3);

        QtTsan::futexAcquire(addr, addr2);

        return result;
    }
    template <typename T> int *addr(T *ptr)
    {
        int *int_addr = reinterpret_cast<int *>(ptr);
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        if (sizeof(T) > sizeof(int))
            int_addr++; //We want a pointer to the least significant half
#endif
        return int_addr;
    }

    template <typename Atomic>
    inline void futexWait(Atomic &futex, typename Atomic::Type expectedValue)
    {
        _q_futex(addr(&futex), FUTEX_WAIT, qintptr(expectedValue));
    }
    template <typename Atomic>
    inline bool futexWait(Atomic &futex, typename Atomic::Type expectedValue, qint64 nstimeout)
    {
        struct timespec ts;
        ts.tv_sec = nstimeout / 1000 / 1000 / 1000;
        ts.tv_nsec = nstimeout % (1000 * 1000 * 1000);
        int r = _q_futex(addr(&futex), FUTEX_WAIT, qintptr(expectedValue), quintptr(&ts));
        return r == 0 || errno != ETIMEDOUT;
    }
    template <typename Atomic> inline void futexWakeOne(Atomic &futex)
    {
        _q_futex(addr(&futex), FUTEX_WAKE, 1);
    }
    template <typename Atomic> inline void futexWakeAll(Atomic &futex)
    {
        _q_futex(addr(&futex), FUTEX_WAKE, INT_MAX);
    }
    template <typename Atomic> inline
    void futexWakeOp(Atomic &futex1, int wake1, int wake2, Atomic &futex2, quint32 op)
    {
        _q_futex(addr(&futex1), FUTEX_WAKE_OP, wake1, wake2, addr(&futex2), op);
    }
}
namespace QtFutex = QtLinuxFutex;
QT_END_NAMESPACE

#elif defined(Q_OS_WIN)
#  include <qt_windows.h>

QT_BEGIN_NAMESPACE
namespace QtWindowsFutex {
#define QT_ALWAYS_USE_FUTEX
constexpr inline bool futexAvailable() { return true; }

template <typename Atomic>
inline void futexWait(Atomic &futex, typename Atomic::Type expectedValue)
{
    QtTsan::futexRelease(&futex);
    WaitOnAddress(&futex, &expectedValue, sizeof(expectedValue), INFINITE);
    QtTsan::futexAcquire(&futex);
}
template <typename Atomic>
inline bool futexWait(Atomic &futex, typename Atomic::Type expectedValue, qint64 nstimeout)
{
    BOOL r = WaitOnAddress(&futex, &expectedValue, sizeof(expectedValue), DWORD(nstimeout / 1000 / 1000));
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
}
namespace QtFutex = QtWindowsFutex;
QT_END_NAMESPACE
#else

QT_BEGIN_NAMESPACE
namespace QtFutex = QtDummyFutex;
QT_END_NAMESPACE
#endif

#endif // QFUTEX_P_H
