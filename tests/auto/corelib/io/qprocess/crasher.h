// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2020 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#if defined(_MSC_VER)
#  include <intrin.h>
#endif
#if __has_include(<signal.h>)
#  include <signal.h>
#endif
#if __has_include(<sys/resource.h>)
#  include <sys/resource.h>
#endif

#ifndef __has_builtin
#  define __has_builtin(x)  0
#endif

namespace tst_QProcessCrash {
struct NoCoreDumps
{
#if defined(RLIMIT_CORE)
    struct rlimit rlim;
    NoCoreDumps()
    {
        if (getrlimit(RLIMIT_CORE, &rlim) == 0 && rlim.rlim_cur != 0) {
            struct rlimit newrlim = rlim;
            newrlim.rlim_cur = 0;
            setrlimit(RLIMIT_CORE, &newrlim);
        }
    }
    ~NoCoreDumps()
    {
        setrlimit(RLIMIT_CORE, &rlim);
    }
#endif // RLIMIT_CORE
};

void crashFallback(volatile int *ptr = nullptr)
{
    *ptr = 0;
}

void crash()
{
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    __ud2();
#elif __has_builtin(__builtin_trap)
    __builtin_trap();
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    asm("ud2");
#elif defined(SIGILL)
    raise(SIGILL);
#endif

    crashFallback();
}
} // namespace tst_QProcessCrash
