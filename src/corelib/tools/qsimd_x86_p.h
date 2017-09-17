// This is a generated file. DO NOT EDIT.
// Please see util/x86simdgen/generate.pl
#ifndef QSIMD_P_H
#  error "Please include <private/qsimd_p.h> instead"
#endif
#ifndef QSIMD_X86_P_H
#define QSIMD_X86_P_H

#include "qsimd_p.h"

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

QT_BEGIN_NAMESPACE

// Macros for QT_FUNCTION_TARGET (for Clang and GCC)
#define QT_FUNCTION_TARGET_STRING_SSE2              "sse2"
#define QT_FUNCTION_TARGET_STRING_SSE3              "sse3"
#define QT_FUNCTION_TARGET_STRING_SSSE3             "ssse3"
#define QT_FUNCTION_TARGET_STRING_FMA               "fma"
#define QT_FUNCTION_TARGET_STRING_SSE4_1            "sse4.1"
#define QT_FUNCTION_TARGET_STRING_SSE4_2            "sse4.2"
#define QT_FUNCTION_TARGET_STRING_MOVBE             "movbe"
#define QT_FUNCTION_TARGET_STRING_POPCNT            "popcnt"
#define QT_FUNCTION_TARGET_STRING_AES               "aes,sse4.2"
#define QT_FUNCTION_TARGET_STRING_AVX               "avx"
#define QT_FUNCTION_TARGET_STRING_F16C              "f16c"
#define QT_FUNCTION_TARGET_STRING_RDRND             "rdrnd"
#define QT_FUNCTION_TARGET_STRING_BMI               "bmi"
#define QT_FUNCTION_TARGET_STRING_HLE               "hle"
#define QT_FUNCTION_TARGET_STRING_AVX2              "avx2"
#define QT_FUNCTION_TARGET_STRING_BMI2              "bmi2"
#define QT_FUNCTION_TARGET_STRING_RTM               "rtm"
#define QT_FUNCTION_TARGET_STRING_AVX512F           "avx512f"
#define QT_FUNCTION_TARGET_STRING_AVX512DQ          "avx512dq"
#define QT_FUNCTION_TARGET_STRING_RDSEED            "rdseed"
#define QT_FUNCTION_TARGET_STRING_AVX512IFMA        "avx512ifma"
#define QT_FUNCTION_TARGET_STRING_AVX512PF          "avx512pf"
#define QT_FUNCTION_TARGET_STRING_AVX512ER          "avx512er"
#define QT_FUNCTION_TARGET_STRING_AVX512CD          "avx512cd"
#define QT_FUNCTION_TARGET_STRING_SHA               "sha"
#define QT_FUNCTION_TARGET_STRING_AVX512BW          "avx512bw"
#define QT_FUNCTION_TARGET_STRING_AVX512VL          "avx512vl"
#define QT_FUNCTION_TARGET_STRING_AVX512VBMI        "avx512vbmi"
#define QT_FUNCTION_TARGET_STRING_AVX512VBMI2       "avx512vbmi2"
#define QT_FUNCTION_TARGET_STRING_GFNI              "gfni"
#define QT_FUNCTION_TARGET_STRING_VAES              "vaes"
#define QT_FUNCTION_TARGET_STRING_AVX512VNNI        "avx512vnni"
#define QT_FUNCTION_TARGET_STRING_AVX512BITALG      "avx512bitalg"
#define QT_FUNCTION_TARGET_STRING_AVX512VPOPCNTDQ   "avx512vpopcntdq"
#define QT_FUNCTION_TARGET_STRING_AVX5124NNIW       "avx5124nniw"
#define QT_FUNCTION_TARGET_STRING_AVX5124FMAPS      "avx5124fmaps"

enum CPUFeatures {
    // in CPUID Leaf 1, EDX:
    CpuFeatureSSE2          = 1,

    // in CPUID Leaf 1, ECX:
    CpuFeatureSSE3          = 2,
    CpuFeatureSSSE3         = 3,
    CpuFeatureFMA           = 4,
    CpuFeatureSSE4_1        = 5,
    CpuFeatureSSE4_2        = 6,
    CpuFeatureMOVBE         = 7,
    CpuFeaturePOPCNT        = 8,
    CpuFeatureAES           = 9,
    CpuFeatureAVX           = 10,
    CpuFeatureF16C          = 11,
    CpuFeatureRDRND         = 12,

    // in CPUID Leaf 7, Sub-leaf 0, EBX:
    CpuFeatureBMI           = 13,
    CpuFeatureHLE           = 14,
    CpuFeatureAVX2          = 15,
    CpuFeatureBMI2          = 16,
    CpuFeatureRTM           = 17,
    CpuFeatureAVX512F       = 18,
    CpuFeatureAVX512DQ      = 19,
    CpuFeatureRDSEED        = 20,
    CpuFeatureAVX512IFMA    = 21,
    CpuFeatureAVX512PF      = 22,
    CpuFeatureAVX512ER      = 23,
    CpuFeatureAVX512CD      = 24,
    CpuFeatureSHA           = 25,
    CpuFeatureAVX512BW      = 26,
    CpuFeatureAVX512VL      = 27,

    // in CPUID Leaf 7, Sub-leaf 0, ECX:
    CpuFeatureAVX512VBMI    = 28,
    CpuFeatureAVX512VBMI2   = 29,
    CpuFeatureGFNI          = 30,
    CpuFeatureVAES          = 31,
    CpuFeatureAVX512VNNI    = 32,
    CpuFeatureAVX512BITALG  = 33,
    CpuFeatureAVX512VPOPCNTDQ = 34,

    // in CPUID Leaf 7, Sub-leaf 0, EDX:
    CpuFeatureAVX5124NNIW   = 35,
    CpuFeatureAVX5124FMAPS  = 36,

    // used only to indicate that the CPU detection was initialized
    QSimdInitialized = 1
};

static const quint64 qCompilerCpuFeatures = 0
#ifdef __SSE2__
         | (Q_UINT64_C(1) << CpuFeatureSSE2)
#endif
#ifdef __SSE3__
         | (Q_UINT64_C(1) << CpuFeatureSSE3)
#endif
#ifdef __SSSE3__
         | (Q_UINT64_C(1) << CpuFeatureSSSE3)
#endif
#ifdef __FMA__
         | (Q_UINT64_C(1) << CpuFeatureFMA)
#endif
#ifdef __SSE4_1__
         | (Q_UINT64_C(1) << CpuFeatureSSE4_1)
#endif
#ifdef __SSE4_2__
         | (Q_UINT64_C(1) << CpuFeatureSSE4_2)
#endif
#ifdef __MOVBE__
         | (Q_UINT64_C(1) << CpuFeatureMOVBE)
#endif
#ifdef __POPCNT__
         | (Q_UINT64_C(1) << CpuFeaturePOPCNT)
#endif
#ifdef __AES__
         | (Q_UINT64_C(1) << CpuFeatureAES)
#endif
#ifdef __AVX__
         | (Q_UINT64_C(1) << CpuFeatureAVX)
#endif
#ifdef __F16C__
         | (Q_UINT64_C(1) << CpuFeatureF16C)
#endif
#ifdef __RDRND__
         | (Q_UINT64_C(1) << CpuFeatureRDRND)
#endif
#ifdef __BMI__
         | (Q_UINT64_C(1) << CpuFeatureBMI)
#endif
#ifdef __HLE__
         | (Q_UINT64_C(1) << CpuFeatureHLE)
#endif
#ifdef __AVX2__
         | (Q_UINT64_C(1) << CpuFeatureAVX2)
#endif
#ifdef __BMI2__
         | (Q_UINT64_C(1) << CpuFeatureBMI2)
#endif
#ifdef __RTM__
         | (Q_UINT64_C(1) << CpuFeatureRTM)
#endif
#ifdef __AVX512F__
         | (Q_UINT64_C(1) << CpuFeatureAVX512F)
#endif
#ifdef __AVX512DQ__
         | (Q_UINT64_C(1) << CpuFeatureAVX512DQ)
#endif
#ifdef __RDSEED__
         | (Q_UINT64_C(1) << CpuFeatureRDSEED)
#endif
#ifdef __AVX512IFMA__
         | (Q_UINT64_C(1) << CpuFeatureAVX512IFMA)
#endif
#ifdef __AVX512PF__
         | (Q_UINT64_C(1) << CpuFeatureAVX512PF)
#endif
#ifdef __AVX512ER__
         | (Q_UINT64_C(1) << CpuFeatureAVX512ER)
#endif
#ifdef __AVX512CD__
         | (Q_UINT64_C(1) << CpuFeatureAVX512CD)
#endif
#ifdef __SHA__
         | (Q_UINT64_C(1) << CpuFeatureSHA)
#endif
#ifdef __AVX512BW__
         | (Q_UINT64_C(1) << CpuFeatureAVX512BW)
#endif
#ifdef __AVX512VL__
         | (Q_UINT64_C(1) << CpuFeatureAVX512VL)
#endif
#ifdef __AVX512VBMI__
         | (Q_UINT64_C(1) << CpuFeatureAVX512VBMI)
#endif
#ifdef __AVX512VBMI2__
         | (Q_UINT64_C(1) << CpuFeatureAVX512VBMI2)
#endif
#ifdef __GFNI__
         | (Q_UINT64_C(1) << CpuFeatureGFNI)
#endif
#ifdef __VAES__
         | (Q_UINT64_C(1) << CpuFeatureVAES)
#endif
#ifdef __AVX512VNNI__
         | (Q_UINT64_C(1) << CpuFeatureAVX512VNNI)
#endif
#ifdef __AVX512BITALG__
         | (Q_UINT64_C(1) << CpuFeatureAVX512BITALG)
#endif
#ifdef __AVX512VPOPCNTDQ__
         | (Q_UINT64_C(1) << CpuFeatureAVX512VPOPCNTDQ)
#endif
#ifdef __AVX5124NNIW__
         | (Q_UINT64_C(1) << CpuFeatureAVX5124NNIW)
#endif
#ifdef __AVX5124FMAPS__
         | (Q_UINT64_C(1) << CpuFeatureAVX5124FMAPS)
#endif
        ;

QT_END_NAMESPACE

#endif // QSIMD_X86_P_H

