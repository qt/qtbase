// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2020 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
