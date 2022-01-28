/****************************************************************************
**
** Copyright (C) 2022 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

// This is a generated file. DO NOT EDIT.
// Please see 3rdparty/x86simd_generate.pl
#ifndef QSIMD_X86_P_H
#define QSIMD_X86_P_H

#include <stdint.h>

// in CPUID Leaf 1, EDX:
#define cpu_feature_sse2                            (UINT64_C(1) << 0)
#define QT_FUNCTION_TARGET_STRING_SSE2              "sse2"

// in CPUID Leaf 1, ECX:
#define cpu_feature_sse3                            (UINT64_C(1) << 1)
#define QT_FUNCTION_TARGET_STRING_SSE3              "sse3"
#define cpu_feature_ssse3                           (UINT64_C(1) << 2)
#define QT_FUNCTION_TARGET_STRING_SSSE3             "ssse3"
#define cpu_feature_fma                             (UINT64_C(1) << 3)
#define QT_FUNCTION_TARGET_STRING_FMA               "fma"
#define cpu_feature_sse4_1                          (UINT64_C(1) << 4)
#define QT_FUNCTION_TARGET_STRING_SSE4_1            "sse4.1"
#define cpu_feature_sse4_2                          (UINT64_C(1) << 5)
#define QT_FUNCTION_TARGET_STRING_SSE4_2            "sse4.2"
#define cpu_feature_movbe                           (UINT64_C(1) << 6)
#define QT_FUNCTION_TARGET_STRING_MOVBE             "movbe"
#define cpu_feature_popcnt                          (UINT64_C(1) << 7)
#define QT_FUNCTION_TARGET_STRING_POPCNT            "popcnt"
#define cpu_feature_aes                             (UINT64_C(1) << 8)
#define QT_FUNCTION_TARGET_STRING_AES               "aes,sse4.2"
#define cpu_feature_avx                             (UINT64_C(1) << 9)
#define QT_FUNCTION_TARGET_STRING_AVX               "avx"
#define cpu_feature_f16c                            (UINT64_C(1) << 10)
#define QT_FUNCTION_TARGET_STRING_F16C              "f16c,avx"
#define cpu_feature_rdrnd                           (UINT64_C(1) << 11)
#define QT_FUNCTION_TARGET_STRING_RDRND             "rdrnd"

// in CPUID Leaf 7, Sub-leaf 0, EBX:
#define cpu_feature_bmi                             (UINT64_C(1) << 12)
#define QT_FUNCTION_TARGET_STRING_BMI               "bmi"
#define cpu_feature_avx2                            (UINT64_C(1) << 13)
#define QT_FUNCTION_TARGET_STRING_AVX2              "avx2,avx"
#define cpu_feature_bmi2                            (UINT64_C(1) << 14)
#define QT_FUNCTION_TARGET_STRING_BMI2              "bmi2"
#define cpu_feature_avx512f                         (UINT64_C(1) << 15)
#define QT_FUNCTION_TARGET_STRING_AVX512F           "avx512f,avx"
#define cpu_feature_avx512dq                        (UINT64_C(1) << 16)
#define QT_FUNCTION_TARGET_STRING_AVX512DQ          "avx512dq,avx512f"
#define cpu_feature_rdseed                          (UINT64_C(1) << 17)
#define QT_FUNCTION_TARGET_STRING_RDSEED            "rdseed"
#define cpu_feature_avx512ifma                      (UINT64_C(1) << 18)
#define QT_FUNCTION_TARGET_STRING_AVX512IFMA        "avx512ifma,avx512f"
#define cpu_feature_avx512cd                        (UINT64_C(1) << 19)
#define QT_FUNCTION_TARGET_STRING_AVX512CD          "avx512cd,avx512f"
#define cpu_feature_sha                             (UINT64_C(1) << 20)
#define QT_FUNCTION_TARGET_STRING_SHA               "sha"
#define cpu_feature_avx512bw                        (UINT64_C(1) << 21)
#define QT_FUNCTION_TARGET_STRING_AVX512BW          "avx512bw,avx512f"
#define cpu_feature_avx512vl                        (UINT64_C(1) << 22)
#define QT_FUNCTION_TARGET_STRING_AVX512VL          "avx512vl,avx512f"

// in CPUID Leaf 7, Sub-leaf 0, ECX:
#define cpu_feature_avx512vbmi                      (UINT64_C(1) << 23)
#define QT_FUNCTION_TARGET_STRING_AVX512VBMI        "avx512vbmi,avx512f"
#define cpu_feature_avx512vbmi2                     (UINT64_C(1) << 24)
#define QT_FUNCTION_TARGET_STRING_AVX512VBMI2       "avx512vbmi2,avx512f"
#define cpu_feature_shstk                           (UINT64_C(1) << 25)
#define QT_FUNCTION_TARGET_STRING_SHSTK             "shstk"
#define cpu_feature_gfni                            (UINT64_C(1) << 26)
#define QT_FUNCTION_TARGET_STRING_GFNI              "gfni"
#define cpu_feature_vaes                            (UINT64_C(1) << 27)
#define QT_FUNCTION_TARGET_STRING_VAES              "vaes,avx2,avx,aes"
#define cpu_feature_avx512vnni                      (UINT64_C(1) << 28)
#define QT_FUNCTION_TARGET_STRING_AVX512VNNI        "avx512vnni,avx512f"
#define cpu_feature_avx512bitalg                    (UINT64_C(1) << 29)
#define QT_FUNCTION_TARGET_STRING_AVX512BITALG      "avx512bitalg,avx512f"
#define cpu_feature_avx512vpopcntdq                 (UINT64_C(1) << 30)
#define QT_FUNCTION_TARGET_STRING_AVX512VPOPCNTDQ   "avx512vpopcntdq,avx512f"

// in CPUID Leaf 7, Sub-leaf 0, EDX:
#define cpu_feature_hybrid                          (UINT64_C(1) << 31)
#define QT_FUNCTION_TARGET_STRING_HYBRID            "hybrid"
#define cpu_feature_ibt                             (UINT64_C(1) << 32)
#define QT_FUNCTION_TARGET_STRING_IBT               "ibt"
#define cpu_feature_avx512fp16                      (UINT64_C(1) << 33)
#define QT_FUNCTION_TARGET_STRING_AVX512FP16        "avx512fp16,avx512f,f16c"

// CPU architectures
#define cpu_x86_64              (0 \
                                 | cpu_feature_sse2)
#define cpu_core2               (cpu_x86_64 \
                                 | cpu_feature_sse3 \
                                 | cpu_feature_ssse3)
#define cpu_nhm                 (cpu_core2 \
                                 | cpu_feature_sse4_1 \
                                 | cpu_feature_sse4_2 \
                                 | cpu_feature_popcnt)
#define cpu_wsm                 (cpu_nhm)
#define cpu_snb                 (cpu_wsm \
                                 | cpu_feature_avx)
#define cpu_ivb                 (cpu_snb \
                                 | cpu_feature_f16c \
                                 | cpu_feature_rdrnd)
#define cpu_hsw                 (cpu_ivb \
                                 | cpu_feature_avx2 \
                                 | cpu_feature_fma \
                                 | cpu_feature_bmi \
                                 | cpu_feature_bmi2 \
                                 | cpu_feature_movbe)
#define cpu_bdw                 (cpu_hsw \
                                 | cpu_feature_rdseed)
#define cpu_bdx                 (cpu_bdw)
#define cpu_skl                 (cpu_bdw)
#define cpu_adl                 (cpu_skl \
                                 | cpu_feature_gfni \
                                 | cpu_feature_vaes \
                                 | cpu_feature_shstk \
                                 | cpu_feature_ibt)
#define cpu_skx                 (cpu_skl \
                                 | cpu_feature_avx512f \
                                 | cpu_feature_avx512dq \
                                 | cpu_feature_avx512cd \
                                 | cpu_feature_avx512bw \
                                 | cpu_feature_avx512vl)
#define cpu_clx                 (cpu_skx \
                                 | cpu_feature_avx512vnni)
#define cpu_cpx                 (cpu_clx)
#define cpu_cnl                 (cpu_skx \
                                 | cpu_feature_avx512ifma \
                                 | cpu_feature_avx512vbmi)
#define cpu_icl                 (cpu_cnl \
                                 | cpu_feature_avx512vbmi2 \
                                 | cpu_feature_gfni \
                                 | cpu_feature_vaes \
                                 | cpu_feature_avx512vnni \
                                 | cpu_feature_avx512bitalg \
                                 | cpu_feature_avx512vpopcntdq)
#define cpu_icx                 (cpu_icl)
#define cpu_tgl                 (cpu_icl \
                                 | cpu_feature_shstk \
                                 | cpu_feature_ibt)
#define cpu_spr                 (cpu_tgl)
#define cpu_slm                 (cpu_wsm \
                                 | cpu_feature_rdrnd \
                                 | cpu_feature_movbe)
#define cpu_glm                 (cpu_slm \
                                 | cpu_feature_rdseed)
#define cpu_tnt                 (cpu_glm \
                                 | cpu_feature_gfni)
#define cpu_nehalem             (cpu_nhm)
#define cpu_westmere            (cpu_wsm)
#define cpu_sandybridge         (cpu_snb)
#define cpu_ivybridge           (cpu_ivb)
#define cpu_haswell             (cpu_hsw)
#define cpu_broadwell           (cpu_bdw)
#define cpu_skylake             (cpu_skl)
#define cpu_skylake_avx512      (cpu_skx)
#define cpu_cascadelake         (cpu_clx)
#define cpu_cooperlake          (cpu_cpx)
#define cpu_cannonlake          (cpu_cnl)
#define cpu_icelake_client      (cpu_icl)
#define cpu_icelake_server      (cpu_icx)
#define cpu_alderlake           (cpu_adl)
#define cpu_sapphirerapids      (cpu_spr)
#define cpu_tigerlake           (cpu_tgl)
#define cpu_silvermont          (cpu_slm)
#define cpu_goldmont            (cpu_glm)
#define cpu_tremont             (cpu_tnt)

static const uint64_t _compilerCpuFeatures = 0
#ifdef __SSE2__
         | cpu_feature_sse2
#endif
#ifdef __SSE3__
         | cpu_feature_sse3
#endif
#ifdef __SSSE3__
         | cpu_feature_ssse3
#endif
#ifdef __FMA__
         | cpu_feature_fma
#endif
#ifdef __SSE4_1__
         | cpu_feature_sse4_1
#endif
#ifdef __SSE4_2__
         | cpu_feature_sse4_2
#endif
#ifdef __MOVBE__
         | cpu_feature_movbe
#endif
#ifdef __POPCNT__
         | cpu_feature_popcnt
#endif
#ifdef __AES__
         | cpu_feature_aes
#endif
#ifdef __AVX__
         | cpu_feature_avx
#endif
#ifdef __F16C__
         | cpu_feature_f16c
#endif
#ifdef __RDRND__
         | cpu_feature_rdrnd
#endif
#ifdef __BMI__
         | cpu_feature_bmi
#endif
#ifdef __AVX2__
         | cpu_feature_avx2
#endif
#ifdef __BMI2__
         | cpu_feature_bmi2
#endif
#ifdef __AVX512F__
         | cpu_feature_avx512f
#endif
#ifdef __AVX512DQ__
         | cpu_feature_avx512dq
#endif
#ifdef __RDSEED__
         | cpu_feature_rdseed
#endif
#ifdef __AVX512IFMA__
         | cpu_feature_avx512ifma
#endif
#ifdef __AVX512CD__
         | cpu_feature_avx512cd
#endif
#ifdef __SHA__
         | cpu_feature_sha
#endif
#ifdef __AVX512BW__
         | cpu_feature_avx512bw
#endif
#ifdef __AVX512VL__
         | cpu_feature_avx512vl
#endif
#ifdef __AVX512VBMI__
         | cpu_feature_avx512vbmi
#endif
#ifdef __AVX512VBMI2__
         | cpu_feature_avx512vbmi2
#endif
#ifdef __SHSTK__
         | cpu_feature_shstk
#endif
#ifdef __GFNI__
         | cpu_feature_gfni
#endif
#ifdef __VAES__
         | cpu_feature_vaes
#endif
#ifdef __AVX512VNNI__
         | cpu_feature_avx512vnni
#endif
#ifdef __AVX512BITALG__
         | cpu_feature_avx512bitalg
#endif
#ifdef __AVX512VPOPCNTDQ__
         | cpu_feature_avx512vpopcntdq
#endif
#ifdef __HYBRID__
         | cpu_feature_hybrid
#endif
#ifdef __IBT__
         | cpu_feature_ibt
#endif
#ifdef __AVX512FP16__
         | cpu_feature_avx512fp16
#endif
        ;

#if (defined __cplusplus) && __cplusplus >= 201103L
enum X86CpuFeatures : uint64_t {
    CpuFeatureSSE2 = cpu_feature_sse2,                       ///< Streaming SIMD Extensions 2
    CpuFeatureSSE3 = cpu_feature_sse3,                       ///< Streaming SIMD Extensions 3
    CpuFeatureSSSE3 = cpu_feature_ssse3,                     ///< Supplemental Streaming SIMD Extensions 3
    CpuFeatureFMA = cpu_feature_fma,                         ///< Fused Multiply-Add
    CpuFeatureSSE4_1 = cpu_feature_sse4_1,                   ///< Streaming SIMD Extensions 4.1
    CpuFeatureSSE4_2 = cpu_feature_sse4_2,                   ///< Streaming SIMD Extensions 4.2
    CpuFeatureMOVBE = cpu_feature_movbe,                     ///< MOV Big Endian
    CpuFeaturePOPCNT = cpu_feature_popcnt,                   ///< Population count
    CpuFeatureAES = cpu_feature_aes,                         ///< Advenced Encryption Standard
    CpuFeatureAVX = cpu_feature_avx,                         ///< Advanced Vector Extensions
    CpuFeatureF16C = cpu_feature_f16c,                       ///< 16-bit Float Conversion
    CpuFeatureRDRND = cpu_feature_rdrnd,                     ///< Random number generator
    CpuFeatureBMI = cpu_feature_bmi,                         ///< Bit Manipulation Instructions
    CpuFeatureAVX2 = cpu_feature_avx2,                       ///< Advanced Vector Extensions 2
    CpuFeatureBMI2 = cpu_feature_bmi2,                       ///< Bit Manipulation Instructions 2
    CpuFeatureAVX512F = cpu_feature_avx512f,                 ///< AVX512 Foundation
    CpuFeatureAVX512DQ = cpu_feature_avx512dq,               ///< AVX512 Double & Quadword
    CpuFeatureRDSEED = cpu_feature_rdseed,                   ///< Random number generator for seeding
    CpuFeatureAVX512IFMA = cpu_feature_avx512ifma,           ///< AVX512 Integer Fused Multiply-Add
    CpuFeatureAVX512CD = cpu_feature_avx512cd,               ///< AVX512 Conflict Detection
    CpuFeatureSHA = cpu_feature_sha,                         ///< SHA-1 and SHA-256 instructions
    CpuFeatureAVX512BW = cpu_feature_avx512bw,               ///< AVX512 Byte & Word
    CpuFeatureAVX512VL = cpu_feature_avx512vl,               ///< AVX512 Vector Length
    CpuFeatureAVX512VBMI = cpu_feature_avx512vbmi,           ///< AVX512 Vector Byte Manipulation Instructions
    CpuFeatureAVX512VBMI2 = cpu_feature_avx512vbmi2,         ///< AVX512 Vector Byte Manipulation Instructions 2
    CpuFeatureSHSTK = cpu_feature_shstk,                     ///< Control Flow Enforcement Technology Shadow Stack
    CpuFeatureGFNI = cpu_feature_gfni,                       ///< Galois Field new instructions
    CpuFeatureVAES = cpu_feature_vaes,                       ///< 256- and 512-bit AES
    CpuFeatureAVX512VNNI = cpu_feature_avx512vnni,           ///< AVX512 Vector Neural Network Instructions
    CpuFeatureAVX512BITALG = cpu_feature_avx512bitalg,       ///< AVX512 Bit Algorithms
    CpuFeatureAVX512VPOPCNTDQ = cpu_feature_avx512vpopcntdq, ///< AVX512 Population Count
    CpuFeatureHYBRID = cpu_feature_hybrid,                   ///< Hybrid processor
    CpuFeatureIBT = cpu_feature_ibt,                         ///< Control Flow Enforcement Technology Indirect Branch Tracking
    CpuFeatureAVX512FP16 = cpu_feature_avx512fp16,           ///< AVX512 16-bit Floating Point
}; // enum X86CpuFeatures

enum X86CpuArchitectures : uint64_t {
    CpuArchx8664 = cpu_x86_64,
    CpuArchCore2 = cpu_core2,
    CpuArchNHM = cpu_nhm,
    CpuArchWSM = cpu_wsm,
    CpuArchSNB = cpu_snb,
    CpuArchIVB = cpu_ivb,
    CpuArchHSW = cpu_hsw,
    CpuArchBDW = cpu_bdw,
    CpuArchBDX = cpu_bdx,
    CpuArchSKL = cpu_skl,
    CpuArchADL = cpu_adl,
    CpuArchSKX = cpu_skx,
    CpuArchCLX = cpu_clx,
    CpuArchCPX = cpu_cpx,
    CpuArchCNL = cpu_cnl,
    CpuArchICL = cpu_icl,
    CpuArchICX = cpu_icx,
    CpuArchTGL = cpu_tgl,
    CpuArchSPR = cpu_spr,
    CpuArchSLM = cpu_slm,
    CpuArchGLM = cpu_glm,
    CpuArchTNT = cpu_tnt,
    CpuArchNehalem = cpu_nehalem,                            ///< Intel Core i3/i5/i7
    CpuArchWestmere = cpu_westmere,                          ///< Intel Core i3/i5/i7
    CpuArchSandyBridge = cpu_sandybridge,                    ///< Second Generation Intel Core i3/i5/i7
    CpuArchIvyBridge = cpu_ivybridge,                        ///< Third Generation Intel Core i3/i5/i7
    CpuArchHaswell = cpu_haswell,                            ///< Fourth Generation Intel Core i3/i5/i7
    CpuArchBroadwell = cpu_broadwell,                        ///< Fifth Generation Intel Core i3/i5/i7
    CpuArchSkylake = cpu_skylake,                            ///< Sixth Generation Intel Core i3/i5/i7
    CpuArchSkylakeAvx512 = cpu_skylake_avx512,               ///< Intel Xeon Scalable
    CpuArchCascadeLake = cpu_cascadelake,                    ///< Second Generation Intel Xeon Scalable
    CpuArchCooperLake = cpu_cooperlake,                      ///< Third Generation Intel Xeon Scalable
    CpuArchCannonLake = cpu_cannonlake,                      ///< Intel Core i3-8121U
    CpuArchIceLakeClient = cpu_icelake_client,               ///< Tenth Generation Intel Core i3/i5/i7
    CpuArchIceLakeServer = cpu_icelake_server,               ///< Third Generation Intel Xeon Scalable
    CpuArchAlderLake = cpu_alderlake,
    CpuArchSapphireRapids = cpu_sapphirerapids,
    CpuArchTigerLake = cpu_tigerlake,                        ///< Eleventh Generation Intel Core i3/i5/i7
    CpuArchSilvermont = cpu_silvermont,
    CpuArchGoldmont = cpu_goldmont,
    CpuArchTremont = cpu_tremont,
}; // enum X86cpuArchitectures
#endif /* C++11 */

#endif /* QSIMD_X86_P_H */
