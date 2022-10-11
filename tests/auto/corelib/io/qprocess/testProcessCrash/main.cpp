// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2020 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#if __has_include(<sys/resource.h>)
#  include <sys/resource.h>
#  if defined(RLIMIT_CORE)
static bool disableCoreDumps()
{
    // Unix: set our core dump limit to zero to request no dialogs.
    if (struct rlimit rlim; getrlimit(RLIMIT_CORE, &rlim) == 0) {
        rlim.rlim_cur = 0;
        setrlimit(RLIMIT_CORE, &rlim);
    }
    return true;
}
static bool disabledCoreDumps = disableCoreDumps();
#  endif // RLIMIT_CORE
#endif // <sys/resource.h>

void crashFallback(volatile int *ptr = nullptr)
{
    *ptr = 0;
}

#if defined(_MSC_VER)
#  include <intrin.h>

int main()
{
#  if defined(_M_IX86) || defined(_M_X64)
    __ud2();
#  endif

    crashFallback();
}
#elif defined(__MINGW32__)
int main()
{
    asm("ud2");
    crashFallback();
}
#else
#  include <stdlib.h>

int main()
{
    abort();
}
#endif
