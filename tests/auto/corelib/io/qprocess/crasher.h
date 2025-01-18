// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2020 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#if defined(_MSC_VER)
#  include <intrin.h>
#endif
#ifndef __has_builtin
#  define __has_builtin(x)  0
#endif

namespace tst_QProcessCrash {
void crashFallback(volatile int *ptr = nullptr)
{
    *ptr = 0;
}

void crash()
{
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    __ud2();
#elif defined(_MSC_VER) && defined(_M_ARM64)
    __debugbreak();
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
