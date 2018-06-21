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

// used only to indicate that the CPU detection was initialized
static const quint64 QSimdInitialized        = Q_UINT64_C(1) << 0;

// in CPUID Leaf 1, EDX:
static const quint64 CpuFeatureSSE2          = Q_UINT64_C(1) << 1;

// in CPUID Leaf 1, ECX:
static const quint64 CpuFeatureSSE3          = Q_UINT64_C(1) << 2;
static const quint64 CpuFeatureSSSE3         = Q_UINT64_C(1) << 3;
static const quint64 CpuFeatureFMA           = Q_UINT64_C(1) << 4;
static const quint64 CpuFeatureSSE4_1        = Q_UINT64_C(1) << 5;
static const quint64 CpuFeatureSSE4_2        = Q_UINT64_C(1) << 6;
static const quint64 CpuFeatureMOVBE         = Q_UINT64_C(1) << 7;
static const quint64 CpuFeaturePOPCNT        = Q_UINT64_C(1) << 8;
static const quint64 CpuFeatureAES           = Q_UINT64_C(1) << 9;
static const quint64 CpuFeatureAVX           = Q_UINT64_C(1) << 10;
static const quint64 CpuFeatureF16C          = Q_UINT64_C(1) << 11;
static const quint64 CpuFeatureRDRND         = Q_UINT64_C(1) << 12;

// in CPUID Leaf 7, Sub-leaf 0, EBX:
static const quint64 CpuFeatureBMI           = Q_UINT64_C(1) << 13;
static const quint64 CpuFeatureHLE           = Q_UINT64_C(1) << 14;
static const quint64 CpuFeatureAVX2          = Q_UINT64_C(1) << 15;
static const quint64 CpuFeatureBMI2          = Q_UINT64_C(1) << 16;
static const quint64 CpuFeatureRTM           = Q_UINT64_C(1) << 17;
static const quint64 CpuFeatureAVX512F       = Q_UINT64_C(1) << 18;
static const quint64 CpuFeatureAVX512DQ      = Q_UINT64_C(1) << 19;
static const quint64 CpuFeatureRDSEED        = Q_UINT64_C(1) << 20;
static const quint64 CpuFeatureAVX512IFMA    = Q_UINT64_C(1) << 21;
static const quint64 CpuFeatureAVX512PF      = Q_UINT64_C(1) << 22;
static const quint64 CpuFeatureAVX512ER      = Q_UINT64_C(1) << 23;
static const quint64 CpuFeatureAVX512CD      = Q_UINT64_C(1) << 24;
static const quint64 CpuFeatureSHA           = Q_UINT64_C(1) << 25;
static const quint64 CpuFeatureAVX512BW      = Q_UINT64_C(1) << 26;
static const quint64 CpuFeatureAVX512VL      = Q_UINT64_C(1) << 27;

// in CPUID Leaf 7, Sub-leaf 0, ECX:
static const quint64 CpuFeatureAVX512VBMI    = Q_UINT64_C(1) << 28;
static const quint64 CpuFeatureAVX512VBMI2   = Q_UINT64_C(1) << 29;
static const quint64 CpuFeatureGFNI          = Q_UINT64_C(1) << 30;
static const quint64 CpuFeatureVAES          = Q_UINT64_C(1) << 31;
static const quint64 CpuFeatureAVX512VNNI    = Q_UINT64_C(1) << 32;
static const quint64 CpuFeatureAVX512BITALG  = Q_UINT64_C(1) << 33;
static const quint64 CpuFeatureAVX512VPOPCNTDQ = Q_UINT64_C(1) << 34;

// in CPUID Leaf 7, Sub-leaf 0, EDX:
static const quint64 CpuFeatureAVX5124NNIW   = Q_UINT64_C(1) << 35;
static const quint64 CpuFeatureAVX5124FMAPS  = Q_UINT64_C(1) << 36;

static const quint64 qCompilerCpuFeatures = 0
#ifdef __SSE2__
         | (Q_UINT64_C(1) << 1)         // CpuFeatureSSE2
#endif
#ifdef __SSE3__
         | (Q_UINT64_C(1) << 2)         // CpuFeatureSSE3
#endif
#ifdef __SSSE3__
         | (Q_UINT64_C(1) << 3)         // CpuFeatureSSSE3
#endif
#ifdef __FMA__
         | (Q_UINT64_C(1) << 4)         // CpuFeatureFMA
#endif
#ifdef __SSE4_1__
         | (Q_UINT64_C(1) << 5)         // CpuFeatureSSE4_1
#endif
#ifdef __SSE4_2__
         | (Q_UINT64_C(1) << 6)         // CpuFeatureSSE4_2
#endif
#ifdef __MOVBE__
         | (Q_UINT64_C(1) << 7)         // CpuFeatureMOVBE
#endif
#ifdef __POPCNT__
         | (Q_UINT64_C(1) << 8)         // CpuFeaturePOPCNT
#endif
#ifdef __AES__
         | (Q_UINT64_C(1) << 9)         // CpuFeatureAES
#endif
#ifdef __AVX__
         | (Q_UINT64_C(1) << 10)        // CpuFeatureAVX
#endif
#ifdef __F16C__
         | (Q_UINT64_C(1) << 11)        // CpuFeatureF16C
#endif
#ifdef __RDRND__
         | (Q_UINT64_C(1) << 12)        // CpuFeatureRDRND
#endif
#ifdef __BMI__
         | (Q_UINT64_C(1) << 13)        // CpuFeatureBMI
#endif
#ifdef __HLE__
         | (Q_UINT64_C(1) << 14)        // CpuFeatureHLE
#endif
#ifdef __AVX2__
         | (Q_UINT64_C(1) << 15)        // CpuFeatureAVX2
#endif
#ifdef __BMI2__
         | (Q_UINT64_C(1) << 16)        // CpuFeatureBMI2
#endif
#ifdef __RTM__
         | (Q_UINT64_C(1) << 17)        // CpuFeatureRTM
#endif
#ifdef __AVX512F__
         | (Q_UINT64_C(1) << 18)        // CpuFeatureAVX512F
#endif
#ifdef __AVX512DQ__
         | (Q_UINT64_C(1) << 19)        // CpuFeatureAVX512DQ
#endif
#ifdef __RDSEED__
         | (Q_UINT64_C(1) << 20)        // CpuFeatureRDSEED
#endif
#ifdef __AVX512IFMA__
         | (Q_UINT64_C(1) << 21)        // CpuFeatureAVX512IFMA
#endif
#ifdef __AVX512PF__
         | (Q_UINT64_C(1) << 22)        // CpuFeatureAVX512PF
#endif
#ifdef __AVX512ER__
         | (Q_UINT64_C(1) << 23)        // CpuFeatureAVX512ER
#endif
#ifdef __AVX512CD__
         | (Q_UINT64_C(1) << 24)        // CpuFeatureAVX512CD
#endif
#ifdef __SHA__
         | (Q_UINT64_C(1) << 25)        // CpuFeatureSHA
#endif
#ifdef __AVX512BW__
         | (Q_UINT64_C(1) << 26)        // CpuFeatureAVX512BW
#endif
#ifdef __AVX512VL__
         | (Q_UINT64_C(1) << 27)        // CpuFeatureAVX512VL
#endif
#ifdef __AVX512VBMI__
         | (Q_UINT64_C(1) << 28)        // CpuFeatureAVX512VBMI
#endif
#ifdef __AVX512VBMI2__
         | (Q_UINT64_C(1) << 29)        // CpuFeatureAVX512VBMI2
#endif
#ifdef __GFNI__
         | (Q_UINT64_C(1) << 30)        // CpuFeatureGFNI
#endif
#ifdef __VAES__
         | (Q_UINT64_C(1) << 31)        // CpuFeatureVAES
#endif
#ifdef __AVX512VNNI__
         | (Q_UINT64_C(1) << 32)        // CpuFeatureAVX512VNNI
#endif
#ifdef __AVX512BITALG__
         | (Q_UINT64_C(1) << 33)        // CpuFeatureAVX512BITALG
#endif
#ifdef __AVX512VPOPCNTDQ__
         | (Q_UINT64_C(1) << 34)        // CpuFeatureAVX512VPOPCNTDQ
#endif
#ifdef __AVX5124NNIW__
         | (Q_UINT64_C(1) << 35)        // CpuFeatureAVX5124NNIW
#endif
#ifdef __AVX5124FMAPS__
         | (Q_UINT64_C(1) << 36)        // CpuFeatureAVX5124FMAPS
#endif
        ;

QT_END_NAMESPACE

#endif // QSIMD_X86_P_H
