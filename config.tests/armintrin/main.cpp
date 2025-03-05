// Copyright (C) 2024 The Qt Company GmbH.
// SPDX-License-Identifier: BSD-3-Clause

#if defined QT_COMPILER_SUPPORTS_CRYPTO
#include <arm_neon.h>
void aeshash(uint8x16_t &state0, uint8x16_t data)
{
    auto state1 = state0;
    state0 = vaeseq_u8(state0, data);
    state0 = vaesmcq_u8(state0);
    auto state2 = state0;
    state0 = vaeseq_u8(state0, state1);
    state0 = vaesmcq_u8(state0);
    auto state3 = state0;
    state0 = vaeseq_u8(state0, state2);
    state0 = vaesmcq_u8(state0);
    state0 = veorq_u8(state0, state3);
}
#elif defined QT_COMPILER_SUPPORTS_SVE
#include <arm_sve.h>
void qt_memfill64_sve(uint64_t *dest, uint64_t color, int64_t count)
{
    int64_t i = 0;
    const svuint64_t vdup = svdup_n_u64(color);
    svbool_t pg = svwhilelt_b64_s64(i, count);
    do {
        svst1_u64(pg, (uint64_t*)(dest + i), vdup);
        i += svcntd();
        pg = svwhilelt_b64_s64(i, count);
    } while (svptest_any(svptrue_b64(), pg));
}
#endif

int main(int argc, char **argv)
{
    return 0;
}
