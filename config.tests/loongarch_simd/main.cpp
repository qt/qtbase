// Copyright (C) 2017 Intel Corporation.
// SPDX-License-Identifier: BSD-3-Clause

#define T(x)                      (QT_COMPILER_SUPPORTS_ ## x)

#if T(LSX)
#include <lsxintrin.h>
void test_lsx()
{
    __m128i a = __lsx_vldi(0);
    (void) __lsx_vshuf_h(__lsx_vldi(0), a, a);
}
#endif

#if T(LASX)
#include <lsxintrin.h>
#include <lasxintrin.h>
void test_lasx()
{
    __m256i a = __lasx_xvldi(0);
    __m256i b = __lasx_xvadd_b(a, a);
    (void) __lasx_xvadd_b(a, b);
}
#endif

int main()
{
    return 0;
}
