// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSIMD_P_H
#define QSIMD_P_H

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

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qsimd.h>

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wundef")
QT_WARNING_DISABLE_GCC("-Wundef")
QT_WARNING_DISABLE_INTEL(103)

#define ALIGNMENT_PROLOGUE_16BYTES(ptr, i, length) \
    for (; i < static_cast<int>(qMin(static_cast<quintptr>(length), ((4 - ((reinterpret_cast<quintptr>(ptr) >> 2) & 0x3)) & 0x3))); ++i)

#define ALIGNMENT_PROLOGUE_32BYTES(ptr, i, length) \
    for (; i < static_cast<int>(qMin(static_cast<quintptr>(length), ((8 - ((reinterpret_cast<quintptr>(ptr) >> 2) & 0x7)) & 0x7))); ++i)

#define SIMD_EPILOGUE(i, length, max) \
    for (int _i = 0; _i < max && i < length; ++i, ++_i)

/*
 * Code can use the following constructs to determine compiler support & status:
 * - #ifdef __XXX__      (e.g: #ifdef __AVX__  or #ifdef __ARM_NEON__)
 *   If this test passes, then the compiler is already generating code for that
 *   given sub-architecture. The intrinsics for that sub-architecture are
 *   #included and can be used without restriction or runtime check.
 *
 * - #if QT_COMPILER_SUPPORTS(XXX)
 *   If this test passes, then the compiler is able to generate code for that
 *   given sub-architecture in another translation unit, given the right set of
 *   flags. Use of the intrinsics is not guaranteed. This is useful with
 *   runtime detection (see below).
 *
 * - #if QT_COMPILER_SUPPORTS_HERE(XXX)
 *   If this test passes, then the compiler is able to generate code for that
 *   given sub-architecture in this translation unit, even if it is not doing
 *   that now (it might be). Individual functions may be tagged with
 *   QT_FUNCTION_TARGET(XXX) to cause the compiler to generate code for that
 *   sub-arch. Only inside such functions is the use of the intrisics
 *   guaranteed to work. This is useful with runtime detection (see below).
 *
 * The distinction between QT_COMPILER_SUPPORTS and QT_COMPILER_SUPPORTS_HERE is
 * historical: GCC 4.8 needed the distinction.
 *
 * Runtime detection of a CPU sub-architecture can be done with the
 * qCpuHasFeature(XXX) function. There are two strategies for generating
 * optimized code like that:
 *
 * 1) place the optimized code in a different translation unit (C or assembly
 * sources) and pass the correct flags to the compiler to enable support. Those
 * sources must not include qglobal.h, which means they cannot include this
 * file either. The dispatcher function would look like this:
 *
 *      void foo()
 *      {
 *      #if QT_COMPILER_SUPPORTS(XXX)
 *          if (qCpuHasFeature(XXX)) {
 *              foo_optimized_xxx();
 *              return;
 *          }
 *      #endif
 *          foo_plain();
 *      }
 *
 * 2) place the optimized code in a function tagged with QT_FUNCTION_TARGET and
 * surrounded by #if QT_COMPILER_SUPPORTS_HERE(XXX). That code can freely use
 * other Qt code. The dispatcher function would look like this:
 *
 *      void foo()
 *      {
 *      #if QT_COMPILER_SUPPORTS_HERE(XXX)
 *          if (qCpuHasFeature(XXX)) {
 *              foo_optimized_xxx();
 *              return;
 *          }
 *      #endif
 *          foo_plain();
 *      }
 */

#if defined(__MINGW64_VERSION_MAJOR) || defined(Q_CC_MSVC)
#include <intrin.h>
#endif

#define QT_COMPILER_SUPPORTS(x)     (QT_COMPILER_SUPPORTS_ ## x - 0)

#if defined(Q_PROCESSOR_ARM)
#  define QT_COMPILER_SUPPORTS_HERE(x)    ((__ARM_FEATURE_ ## x) || (__ ## x ## __) || QT_COMPILER_SUPPORTS(x))
#  if defined(Q_CC_GNU)
     /* GCC requires attributes for a function */
#    define QT_FUNCTION_TARGET(x)  __attribute__((__target__(QT_FUNCTION_TARGET_STRING_ ## x)))
#  else
#    define QT_FUNCTION_TARGET(x)
#  endif
#elif defined(Q_PROCESSOR_MIPS)
#  define QT_COMPILER_SUPPORTS_HERE(x)    (__ ## x ## __)
#  define QT_FUNCTION_TARGET(x)
#  if !defined(__MIPS_DSP__) && defined(__mips_dsp) && defined(Q_PROCESSOR_MIPS_32)
#    define __MIPS_DSP__
#  endif
#  if !defined(__MIPS_DSPR2__) && defined(__mips_dspr2) && defined(Q_PROCESSOR_MIPS_32)
#    define __MIPS_DSPR2__
#  endif
#elif defined(Q_PROCESSOR_X86)
#  if defined(Q_CC_CLANG) && defined(Q_CC_MSVC)
#    define QT_COMPILER_SUPPORTS_HERE(x)    (__ ## x ## __)
#  else
#    define QT_COMPILER_SUPPORTS_HERE(x)    ((__ ## x ## __) || QT_COMPILER_SUPPORTS(x))
#  endif
#  if defined(Q_CC_GNU)
     /* GCC requires attributes for a function */
#    define QT_FUNCTION_TARGET(x)  __attribute__((__target__(QT_FUNCTION_TARGET_STRING_ ## x)))
#  else
#    define QT_FUNCTION_TARGET(x)
#  endif
#else
#  define QT_COMPILER_SUPPORTS_HERE(x)    (__ ## x ## __)
#  define QT_FUNCTION_TARGET(x)
#endif

#if defined(__SSE2__) && !defined(QT_COMPILER_SUPPORTS_SSE2) && !defined(QT_BOOTSTRAPPED)
// Intrinsic support appears to be missing, so pretend these features don't exist
#  undef __SSE__
#  undef __SSE2__
#  undef __SSE3__
#  undef __SSSE3__
#  undef __SSE4_1__
#  undef __SSE4_2__
#  undef __AES__
#  undef __POPCNT__
#  undef __AVX__
#  undef __F16C__
#  undef __RDRND__
#  undef __AVX2__
#  undef __BMI__
#  undef __BMI2__
#  undef __FMA__
#  undef __MOVBE__
#  undef __RDSEED__
#  undef __AVX512F__
#  undef __AVX512ER__
#  undef __AVX512CD__
#  undef __AVX512PF__
#  undef __AVX512DQ__
#  undef __AVX512BW__
#  undef __AVX512VL__
#  undef __AVX512IFMA__
#  undef __AVX512VBMI__
#  undef __SHA__
#  undef __AVX512VBMI2__
#  undef __AVX512BITALG__
#  undef __AVX512VNNI__
#  undef __AVX512VPOPCNTDQ__
#  undef __GFNI__
#  undef __VAES__
#endif

#ifdef Q_PROCESSOR_X86
/* -- x86 intrinsic support -- */

#  if defined(QT_COMPILER_SUPPORTS_RDSEED) && defined(Q_OS_QNX)
// The compiler for QNX is missing the intrinsic
#    undef QT_COMPILER_SUPPORTS_RDSEED
#  endif
#  if defined(Q_CC_MSVC) && (defined(_M_X64) || _M_IX86_FP >= 2)
// MSVC doesn't define __SSE2__, so do it ourselves
#    define __SSE__                         1
#  endif

#  if defined(Q_OS_WIN) && defined(Q_CC_GNU) && !defined(Q_CC_CLANG)
// 64-bit GCC on Windows does not support AVX, so we hack around it by forcing
// it to emit unaligned loads & stores
// See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=49001
asm(
    ".macro vmovapd args:vararg\n"
    "    vmovupd \\args\n"
    ".endm\n"
    ".macro vmovaps args:vararg\n"
    "    vmovups \\args\n"
    ".endm\n"
    ".macro vmovdqa args:vararg\n"
    "    vmovdqu \\args\n"
    ".endm\n"
    ".macro vmovdqa32 args:vararg\n"
    "    vmovdqu32 \\args\n"
    ".endm\n"
    ".macro vmovdqa64 args:vararg\n"
    "    vmovdqu64 \\args\n"
    ".endm\n"
);
#  endif

#  if defined(Q_CC_GNU) && !defined(Q_OS_WASM)
// GCC 4.4 and Clang 2.8 added a few more intrinsics there
#    include <x86intrin.h>
#  endif
#ifdef Q_OS_WASM
#   include <immintrin.h>
# endif

#  include <QtCore/private/qsimd_x86_p.h>

// x86-64 sub-architecture version 3
//
// The Intel Core 4th generation was codenamed "Haswell" and introduced AVX2,
// BMI1, BMI2, FMA, LZCNT, MOVBE, which makes it a good divider for a
// sub-target for us. The first AMD processor with AVX2 support (Zen) has the
// same features, but had already introduced BMI1 in the previous generation.
// This feature set was chosen as the version 3 of the x86-64 ISA (x86-64-v3)
// and is supported by GCC and Clang.
//
// macOS's fat binaries support the "x86_64h" sub-architecture and the GNU libc
// ELF loader also supports a "haswell/" subdir (e.g., /usr/lib/haswell).
#  define ARCH_HASWELL_MACROS       (__AVX2__ + __FMA__)
#  if ARCH_HASWELL_MACROS != 0
#    if ARCH_HASWELL_MACROS != 2
#      error "Please enable all x86-64-v3 extensions; you probably want to use -march=haswell or -march=x86-64-v3 instead of -mavx2"
#    endif
static_assert(ARCH_HASWELL_MACROS, "Undeclared identifiers indicate which features are missing.");
#    define __haswell__       1
#  endif
#  undef ARCH_HASWELL_MACROS

// x86-64 sub-architecture version 4
//
// Similar to the above, x86-64-v4 matches the AVX512 variant of the Intel Core
// 6th generation (codename "Skylake"). AMD Zen4 is the their first processor
// with AVX512 support and it includes all of these too.
//
#  define ARCH_SKX_MACROS           (__AVX512F__ + __AVX512BW__ + __AVX512CD__ + __AVX512DQ__ + __AVX512VL__)
#  if ARCH_SKX_MACROS != 0
#    if ARCH_SKX_MACROS != 5
#      error "Please enable all x86-64-v4 extensions; you probably want to use -march=skylake-avx512 or -march=x86-64-v4 instead of -mavx512f"
#    endif
static_assert(ARCH_SKX_MACROS, "Undeclared identifiers indicate which features are missing.");
#    define __skylake_avx512__  1
#  endif
#  undef ARCH_SKX_MACROS
#endif  /* Q_PROCESSOR_X86 */

// NEON intrinsics
// note: as of GCC 4.9, does not support function targets for ARM
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#if defined(Q_CC_CLANG)
#define QT_FUNCTION_TARGET_STRING_NEON      "neon"
#else
#define QT_FUNCTION_TARGET_STRING_NEON      "+neon" // unused: gcc doesn't support function targets on non-aarch64, and on Aarch64 NEON is always available.
#endif
#ifndef __ARM_NEON__
// __ARM_NEON__ is not defined on AArch64, but we need it in our NEON detection.
#define __ARM_NEON__
#endif

#ifndef Q_PROCESSOR_ARM_64 // vaddv is only available on Aarch64
inline uint16_t vaddvq_u16(uint16x8_t v8)
{
    const uint64x2_t v2 = vpaddlq_u32(vpaddlq_u16(v8));
    const uint64x1_t v1 = vadd_u64(vget_low_u64(v2), vget_high_u64(v2));
    return vget_lane_u16(vreinterpret_u16_u64(v1), 0);
}

inline uint8_t vaddv_u8(uint8x8_t v8)
{
    const uint64x1_t v1 = vpaddl_u32(vpaddl_u16(vpaddl_u8(v8)));
    return vget_lane_u8(vreinterpret_u8_u64(v1), 0);
}
#endif

#endif

#if defined(Q_PROCESSOR_ARM) && defined(__ARM_FEATURE_CRC32)
#  include <arm_acle.h>
#endif

#if defined(Q_PROCESSOR_ARM_64)
#if defined(Q_CC_CLANG)
#define QT_FUNCTION_TARGET_STRING_AES        "crypto"
#define QT_FUNCTION_TARGET_STRING_CRC32      "crc"
#elif defined(Q_CC_GNU)
#define QT_FUNCTION_TARGET_STRING_AES        "+crypto"
#define QT_FUNCTION_TARGET_STRING_CRC32      "+crc"
#endif
#elif defined(Q_PROCESSOR_ARM_32)
#if defined(Q_CC_CLANG)
#define QT_FUNCTION_TARGET_STRING_AES        "armv8-a,crypto"
#define QT_FUNCTION_TARGET_STRING_CRC32      "armv8-a,crc"
#elif defined(Q_CC_GNU)
#define QT_FUNCTION_TARGET_STRING_AES        "arch=armv8-a+crypto"
#define QT_FUNCTION_TARGET_STRING_CRC32      "arch=armv8-a+crc"
#endif
#endif

#ifndef Q_PROCESSOR_X86
enum CPUFeatures {
#if defined(Q_PROCESSOR_ARM)
    CpuFeatureNEON          = 2,
    CpuFeatureARM_NEON      = CpuFeatureNEON,
    CpuFeatureCRC32         = 4,
    CpuFeatureAES           = 8,
    CpuFeatureARM_CRYPTO    = CpuFeatureAES,
#elif defined(Q_PROCESSOR_MIPS)
    CpuFeatureDSP           = 2,
    CpuFeatureDSPR2         = 4,
#endif
};

static const uint64_t qCompilerCpuFeatures = 0
#if defined __ARM_NEON__
        | CpuFeatureNEON
#endif
#if defined __ARM_FEATURE_CRC32
        | CpuFeatureCRC32
#endif
#if defined __ARM_FEATURE_CRYPTO
        | CpuFeatureAES
#endif
#if defined __mips_dsp
        | CpuFeatureDSP
#endif
#if defined __mips_dspr2
        | CpuFeatureDSPR2
#endif
        ;
#endif

#ifdef __cplusplus
#  include <atomic>
#  define Q_ATOMIC(T)   std::atomic<T>
QT_BEGIN_NAMESPACE
using std::atomic_load_explicit;
static constexpr auto memory_order_relaxed = std::memory_order_relaxed;
extern "C" {
#else
#  include <stdatomic.h>
#  define Q_ATOMIC(T)   _Atomic(T)
#endif

#ifdef Q_PROCESSOR_X86
typedef uint64_t QCpuFeatureType;
static const QCpuFeatureType qCompilerCpuFeatures = _compilerCpuFeatures;
static const QCpuFeatureType CpuFeatureArchHaswell = cpu_haswell;
static const QCpuFeatureType CpuFeatureArchSkylakeAvx512 = cpu_skylake_avx512;
#else
typedef unsigned QCpuFeatureType;
#endif
extern Q_CORE_EXPORT Q_ATOMIC(QCpuFeatureType) QT_MANGLE_NAMESPACE(qt_cpu_features)[1];
Q_CORE_EXPORT uint64_t QT_MANGLE_NAMESPACE(qDetectCpuFeatures)();

static inline uint64_t qCpuFeatures()
{
#ifdef QT_BOOTSTRAPPED
    return qCompilerCpuFeatures;    // no detection
#else
    quint64 features = atomic_load_explicit(QT_MANGLE_NAMESPACE(qt_cpu_features), memory_order_relaxed);
    if (!QT_SUPPORTS_INIT_PRIORITY) {
        if (Q_UNLIKELY(features == 0))
            features = QT_MANGLE_NAMESPACE(qDetectCpuFeatures)();
    }
    return features;
#endif
}

#define qCpuHasFeature(feature)     (((qCompilerCpuFeatures & CpuFeature ## feature) == CpuFeature ## feature) \
                                     || ((qCpuFeatures() & CpuFeature ## feature) == CpuFeature ## feature))

/*
    Small wrapper around x86's PAUSE and ARM's YIELD instructions.

    This is completely different from QThread::yieldCurrentThread(), which is
    an OS-level operation that takes the whole thread off the CPU.

    This is just preventing one SMT thread from filling a core's pipeline with
    speculated further loop iterations (which need to be expensively flushed on
    final success) when it could just give those pipeline slots to a second SMT
    thread that can do something useful with the core, such as unblocking this
    SMT thread :)

    So, instead of

        while (!condition)
            ;

    it's better to use

        while (!condition)
            qYieldCpu();
*/
static inline void qYieldCpu()
{
#if defined(Q_PROCESSOR_X86)
    _mm_pause();
#elif defined(Q_PROCESSOR_ARM) && Q_PROCESSOR_ARM >= 7 /* yield was added in ARMv7 */
#  if __has_builtin(__builtin_arm_yield) /* e.g. Clang */
    __builtin_arm_yield();
#  elif defined(Q_OS_INTEGRITY) || \
        (defined(Q_CC_GNU) && !defined(Q_CC_CLANG))
    /*
       - Integrity is missing the arm_acle.h header
       - GCC doesn't have __yield() in arm_acle.h
         https://stackoverflow.com/a/70076751/134841
         https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105416
    */
    asm volatile("yield"); /* this works everywhere */
#  else
    __yield(); /* this is what should work everywhere */
#  endif
#endif
}

#ifdef __cplusplus
} // extern "C"

#  if defined(Q_PROCESSOR_X86) && QT_COMPILER_SUPPORTS_HERE(RDRND) && !defined(QT_BOOTSTRAPPED)
Q_CORE_EXPORT qsizetype qRandomCpu(void *, qsizetype) noexcept;

static inline bool qHasHwrng()
{
    return qCpuHasFeature(RDRND);
}
#  else
static inline qsizetype qRandomCpu(void *, qsizetype) noexcept
{
    return 0;
}
static inline bool qHasHwrng()
{
    return false;
}
#  endif

QT_END_NAMESPACE

#endif // __cplusplus

QT_WARNING_POP

#endif // QSIMD_P_H
