/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include "qsimd_p.h"
#include "qalgorithms.h"
#include <QByteArray>
#include <stdio.h>

#ifdef Q_OS_LINUX
#  include "../testlib/3rdparty/valgrind_p.h"
#endif

#if defined(Q_OS_WIN)
#  if defined(Q_OS_WINCE)
#    include <qt_windows.h>
#    if _WIN32_WCE < 0x800
#      include <cmnintrin.h>
#    endif
#  endif
#  if !defined(Q_CC_GNU)
#    ifndef Q_OS_WINCE
#      include <intrin.h>
#    endif
#  endif
#elif defined(Q_OS_LINUX) && (defined(Q_PROCESSOR_ARM) || defined(Q_PROCESSOR_MIPS_32))
#include "private/qcore_unix_p.h"

// the kernel header definitions for HWCAP_*
// (the ones we need/may need anyway)

// copied from <asm/hwcap.h> (ARM)
#define HWCAP_CRUNCH    1024
#define HWCAP_THUMBEE   2048
#define HWCAP_NEON      4096
#define HWCAP_VFPv3     8192
#define HWCAP_VFPv3D16  16384

// copied from <asm/hwcap.h> (ARM):
#define HWCAP2_CRC32 (1 << 4)

// copied from <asm/hwcap.h> (Aarch64)
#define HWCAP_CRC32             (1 << 7)

// copied from <linux/auxvec.h>
#define AT_HWCAP  16    /* arch dependent hints at CPU capabilities */
#define AT_HWCAP2 26    /* extension of AT_HWCAP */

#elif defined(Q_CC_GHS)
#include <INTEGRITY_types.h>
#endif

QT_BEGIN_NAMESPACE

#if defined (Q_OS_NACL)
static inline uint detectProcessorFeatures()
{
    return 0;
}
#elif defined (Q_OS_WINCE)
static inline quint64 detectProcessorFeatures()
{
    quint64 features = 0;

#if defined (ARM)
#  ifdef PF_ARM_NEON
    if (IsProcessorFeaturePresent(PF_ARM_NEON))
        features |= Q_UINT64_C(1) << CpuFeatureNEON;
#  endif
#elif defined(_X86_)
    if (IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE))
        features |= Q_UINT64_C(1) << CpuFeatureSSE2;
    if (IsProcessorFeaturePresent(PF_SSE3_INSTRUCTIONS_AVAILABLE))
        features |= Q_UINT64_C(1) << CpuFeatureSSE3;
#endif
    return features;
}

#elif defined(Q_PROCESSOR_ARM)
static inline quint64 detectProcessorFeatures()
{
    quint64 features = 0;

#if defined(Q_OS_LINUX)
#  if defined(Q_PROCESSOR_ARM_V8) && defined(Q_PROCESSOR_ARM_64)
    features |= Q_UINT64_C(1) << CpuFeatureNEON; // NEON is always available on ARMv8 64bit.
#  endif
    int auxv = qt_safe_open("/proc/self/auxv", O_RDONLY);
    if (auxv != -1) {
        unsigned long vector[64];
        int nread;
        while (features == 0) {
            nread = qt_safe_read(auxv, (char *)vector, sizeof vector);
            if (nread <= 0) {
                // EOF or error
                break;
            }

            int max = nread / (sizeof vector[0]);
            for (int i = 0; i < max; i += 2) {
                if (vector[i] == AT_HWCAP) {
#  if defined(Q_PROCESSOR_ARM_V8) && defined(Q_PROCESSOR_ARM_64)
                    // For Aarch64:
                    if (vector[i+1] & HWCAP_CRC32)
                        features |= Q_UINT64_C(1) << CpuFeatureCRC32;
#  endif
                    // Aarch32, or ARMv7 or before:
                    if (vector[i+1] & HWCAP_NEON)
                        features |= Q_UINT64_C(1) << CpuFeatureNEON;
                }
#  if defined(Q_PROCESSOR_ARM_32)
                // For Aarch32:
                if (vector[i] == AT_HWCAP2) {
                    if (vector[i+1] & HWCAP2_CRC32)
                        features |= Q_UINT64_C(1) << CpuFeatureCRC32;
                }
#  endif
            }
        }

        qt_safe_close(auxv);
        return features;
    }
    // fall back if /proc/self/auxv wasn't found
#endif

#if defined(__ARM_NEON__)
    features |= Q_UINT64_C(1) << CpuFeatureNEON;
#endif
#if defined(__ARM_FEATURE_CRC32)
    features |= Q_UINT64_C(1) << CpuFeatureCRC32;
#endif

    return features;
}

#elif defined(Q_PROCESSOR_X86)

#ifdef Q_PROCESSOR_X86_32
# define PICreg "%%ebx"
#else
# define PICreg "%%rbx"
#endif

static int maxBasicCpuidSupported()
{
#if defined(Q_CC_GNU)
    qregisterint tmp1;

# if Q_PROCESSOR_X86 < 5
    // check if the CPUID instruction is supported
    long cpuid_supported;
    asm ("pushf\n"
         "pop %0\n"
         "mov %0, %1\n"
         "xor $0x00200000, %0\n"
         "push %0\n"
         "popf\n"
         "pushf\n"
         "pop %0\n"
         "xor %1, %0\n" // %eax is now 0 if CPUID is not supported
         : "=a" (cpuid_supported), "=r" (tmp1)
         );
    if (!cpuid_supported)
        return 0;
# endif

    int result;
    asm ("xchg " PICreg", %1\n"
         "cpuid\n"
         "xchg " PICreg", %1\n"
        : "=&a" (result), "=&r" (tmp1)
        : "0" (0)
        : "ecx", "edx");
    return result;
#elif defined(Q_OS_WIN)
    // Use the __cpuid function; if the CPUID instruction isn't supported, it will return 0
    int info[4];
    __cpuid(info, 0);
    return info[0];
#elif defined(Q_CC_GHS)
    unsigned int info[4];
    __CPUID(0, info);
    return info[0];
#else
    return 0;
#endif
}

static void cpuidFeatures01(uint &ecx, uint &edx)
{
#if defined(Q_CC_GNU)
    qregisterint tmp1;
    asm ("xchg " PICreg", %2\n"
         "cpuid\n"
         "xchg " PICreg", %2\n"
        : "=&c" (ecx), "=&d" (edx), "=&r" (tmp1)
        : "a" (1));
#elif defined(Q_OS_WIN)
    int info[4];
    __cpuid(info, 1);
    ecx = info[2];
    edx = info[3];
#elif defined(Q_CC_GHS)
    unsigned int info[4];
    __CPUID(1, info);
    ecx = info[2];
    edx = info[3];
#endif
}

#ifdef Q_OS_WIN
inline void __cpuidex(int info[4], int, __int64) { memset(info, 0, 4*sizeof(int));}
#endif

static void cpuidFeatures07_00(uint &ebx, uint &ecx)
{
#if defined(Q_CC_GNU)
    qregisteruint rbx; // in case it's 64-bit
    qregisteruint rcx = 0;
    asm ("xchg " PICreg", %0\n"
         "cpuid\n"
         "xchg " PICreg", %0\n"
        : "=&r" (rbx), "+&c" (rcx)
        : "a" (7)
        : "%edx");
    ebx = rbx;
    ecx = rcx;
#elif defined(Q_OS_WIN)
    int info[4];
    __cpuidex(info, 7, 0);
    ebx = info[1];
    ecx = info[2];
#elif defined(Q_CC_GHS)
    unsigned int info[4];
    __CPUIDEX(7, 0, info);
    ebx = info[1];
    ecx = info[2];
#endif
}

#ifdef Q_OS_WIN
// fallback overload in case this intrinsic does not exist: unsigned __int64 _xgetbv(unsigned int);
inline quint64 _xgetbv(__int64) { return 0; }
#endif
static void xgetbv(uint in, uint &eax, uint &edx)
{
#if defined(Q_CC_GNU) || defined(Q_CC_GHS)
    asm (".byte 0x0F, 0x01, 0xD0" // xgetbv instruction
        : "=a" (eax), "=d" (edx)
        : "c" (in));
#elif defined(Q_OS_WIN)
    quint64 result = _xgetbv(in);
    eax = result;
    edx = result >> 32;
#endif
}

static quint64 detectProcessorFeatures()
{
    // Flags from the CR0 / XCR0 state register
    enum XCR0Flags {
        X87             = 1 << 0,
        XMM0_15         = 1 << 1,
        YMM0_15Hi128    = 1 << 2,
        BNDRegs         = 1 << 3,
        BNDCSR          = 1 << 4,
        OpMask          = 1 << 5,
        ZMM0_15Hi256    = 1 << 6,
        ZMM16_31        = 1 << 7,

        SSEState        = XMM0_15,
        AVXState        = XMM0_15 | YMM0_15Hi128,
        AVX512State     = AVXState | OpMask | ZMM0_15Hi256 | ZMM16_31
    };
    static const quint64 AllAVX512 = (Q_UINT64_C(1) << CpuFeatureAVX512F) | (Q_UINT64_C(1) << CpuFeatureAVX512CD) |
            (Q_UINT64_C(1) << CpuFeatureAVX512ER) | (Q_UINT64_C(1) << CpuFeatureAVX512PF) |
            (Q_UINT64_C(1) << CpuFeatureAVX512BW) | (Q_UINT64_C(1) << CpuFeatureAVX512DQ) |
            (Q_UINT64_C(1) << CpuFeatureAVX512VL) |
            (Q_UINT64_C(1) << CpuFeatureAVX512IFMA) | (Q_UINT64_C(1) << CpuFeatureAVX512VBMI);
    static const quint64 AllAVX2 = (Q_UINT64_C(1) << CpuFeatureAVX2) | AllAVX512;
    static const quint64 AllAVX = (Q_UINT64_C(1) << CpuFeatureAVX) | AllAVX2;

    quint64 features = 0;
    int cpuidLevel = maxBasicCpuidSupported();
#if Q_PROCESSOR_X86 < 5
    if (cpuidLevel < 1)
        return 0;
#else
    Q_ASSERT(cpuidLevel >= 1);
#endif

    uint cpuid01ECX = 0, cpuid01EDX = 0;
    cpuidFeatures01(cpuid01ECX, cpuid01EDX);

    // the low 32-bits of features is cpuid01ECX
    // note: we need to check OS support for saving the AVX register state
    features = cpuid01ECX;

#if defined(Q_PROCESSOR_X86_32)
    // x86 might not have SSE2 support
    if (cpuid01EDX & (1u << 26))
        features |= Q_UINT64_C(1) << CpuFeatureSSE2;
    else
        features &= ~(Q_UINT64_C(1) << CpuFeatureSSE2);
    // we should verify that the OS enabled saving of the SSE state...
#else
    // x86-64 or x32
    features |= Q_UINT64_C(1) << CpuFeatureSSE2;
#endif

    uint xgetbvA = 0, xgetbvD = 0;
    if (cpuid01ECX & (1u << 27)) {
        // XGETBV enabled
        xgetbv(0, xgetbvA, xgetbvD);
    }

    uint cpuid0700EBX = 0;
    uint cpuid0700ECX = 0;
    if (cpuidLevel >= 7) {
        cpuidFeatures07_00(cpuid0700EBX, cpuid0700ECX);

        // the high 32-bits of features is cpuid0700EBX
        features |= quint64(cpuid0700EBX) << 32;
    }

    if ((xgetbvA & AVXState) != AVXState) {
        // support for YMM registers is disabled, disable all AVX
        features &= ~AllAVX;
    } else if ((xgetbvA & AVX512State) != AVX512State) {
        // support for ZMM registers or mask registers is disabled, disable all AVX512
        features &= ~AllAVX512;
    } else {
        // this feature is out of order
        if (cpuid0700ECX & (1u << 1))
            features |= Q_UINT64_C(1) << CpuFeatureAVX512VBMI;
        else
            features &= ~(Q_UINT64_C(1) << CpuFeatureAVX512VBMI);
    }

    return features;
}

#elif defined(Q_PROCESSOR_MIPS_32)

#if defined(Q_OS_LINUX)
//
// Do not use QByteArray: it could use SIMD instructions itself at
// some point, thus creating a recursive dependency. Instead, use a
// QSimpleBuffer, which has the bare minimum needed to use memory
// dynamically and read lines from /proc/cpuinfo of arbitrary sizes.
//
struct QSimpleBuffer {
    static const int chunk_size = 256;
    char *data;
    unsigned alloc;
    unsigned size;

    QSimpleBuffer(): data(0), alloc(0), size(0) {}
    ~QSimpleBuffer() { ::free(data); }

    void resize(unsigned newsize) {
        if (newsize > alloc) {
            unsigned newalloc = chunk_size * ((newsize / chunk_size) + 1);
            if (newalloc < newsize) newalloc = newsize;
            if (newalloc != alloc) {
                data = static_cast<char*>(::realloc(data, newalloc));
                alloc = newalloc;
            }
        }
        size = newsize;
    }
    void append(const QSimpleBuffer &other, unsigned appendsize) {
        unsigned oldsize = size;
        resize(oldsize + appendsize);
        ::memcpy(data + oldsize, other.data, appendsize);
    }
    void popleft(unsigned amount) {
        if (amount >= size) return resize(0);
        size -= amount;
        ::memmove(data, data + amount, size);
    }
    char* cString() {
        if (!alloc) resize(1);
        return (data[size] = '\0', data);
    }
};

//
// Uses a scratch "buffer" (which must be used for all reads done in the
// same file descriptor) to read chunks of data from a file, to read
// one line at a time. Lines include the trailing newline character ('\n').
// On EOF, line.size is zero.
//
static void bufReadLine(int fd, QSimpleBuffer &line, QSimpleBuffer &buffer)
{
    for (;;) {
        char *newline = static_cast<char*>(::memchr(buffer.data, '\n', buffer.size));
        if (newline) {
            unsigned piece_size = newline - buffer.data + 1;
            line.append(buffer, piece_size);
            buffer.popleft(piece_size);
            line.resize(line.size - 1);
            return;
        }
        if (buffer.size + QSimpleBuffer::chunk_size > buffer.alloc) {
            int oldsize = buffer.size;
            buffer.resize(buffer.size + QSimpleBuffer::chunk_size);
            buffer.size = oldsize;
        }
        ssize_t read_bytes = ::qt_safe_read(fd, buffer.data + buffer.size, QSimpleBuffer::chunk_size);
        if (read_bytes > 0) buffer.size += read_bytes;
        else return;
    }
}

//
// Checks if any line with a given prefix from /proc/cpuinfo contains
// a certain string, surrounded by spaces.
//
static bool procCpuinfoContains(const char *prefix, const char *string)
{
    int cpuinfo_fd = ::qt_safe_open("/proc/cpuinfo", O_RDONLY);
    if (cpuinfo_fd == -1)
        return false;

    unsigned string_len = ::strlen(string);
    unsigned prefix_len = ::strlen(prefix);
    QSimpleBuffer line, buffer;
    bool present = false;
    do {
        line.resize(0);
        bufReadLine(cpuinfo_fd, line, buffer);
        char *colon = static_cast<char*>(::memchr(line.data, ':', line.size));
        if (colon && line.size > prefix_len + string_len) {
            if (!::strncmp(prefix, line.data, prefix_len)) {
                // prefix matches, next character must be ':' or space
                if (line.data[prefix_len] == ':' || ::isspace(line.data[prefix_len])) {
                    // Does it contain the string?
                    char *found = ::strstr(line.cString(), string);
                    if (found && ::isspace(found[-1]) &&
                            (::isspace(found[string_len]) || found[string_len] == '\0')) {
                        present = true;
                        break;
                    }
                }
            }
        }
    } while (line.size);

    ::qt_safe_close(cpuinfo_fd);
    return present;
}
#endif

static inline quint64 detectProcessorFeatures()
{
    // NOTE: MIPS 74K cores are the only ones supporting DSPr2.
    quint64 flags = 0;

#if defined __mips_dsp
    flags |= Q_UINT64_C(1) << CpuFeatureDSP;
#  if defined __mips_dsp_rev && __mips_dsp_rev >= 2
    flags |= Q_UINT64_C(1) << CpuFeatureDSPR2;
#  elif defined(Q_OS_LINUX)
    if (procCpuinfoContains("cpu model", "MIPS 74Kc") || procCpuinfoContains("cpu model", "MIPS 74Kf"))
        flags |= Q_UINT64_C(1) << CpuFeatureDSPR2;
#  endif
#elif defined(Q_OS_LINUX)
    if (procCpuinfoContains("ASEs implemented", "dsp")) {
        flags |= Q_UINT64_C(1) << CpuFeatureDSP;
        if (procCpuinfoContains("cpu model", "MIPS 74Kc") || procCpuinfoContains("cpu model", "MIPS 74Kf"))
            flags |= Q_UINT64_C(1) << CpuFeatureDSPR2;
    }
#endif

    return flags;
}

#else
static inline uint detectProcessorFeatures()
{
    return 0;
}
#endif

/*
 * Use kdesdk/scripts/generate_string_table.pl to update the table below. Note
 * that the x86 version has a lot of blanks that must be kept and that the
 * offset table's type is changed to make the table smaller. We also remove the
 * terminating -1 that the script adds.
 */

// begin generated
#if defined(Q_PROCESSOR_ARM)
/* Data:
 neon
 crc32
 */
static const char features_string[] =
        " neon\0"
        " crc32\0"
        "\0";
static const int features_indices[] = { 0, 6 };
#elif defined(Q_PROCESSOR_MIPS)
/* Data:
 dsp
 dspr2
*/
static const char features_string[] =
    " dsp\0"
    " dspr2\0"
    "\0";

static const int features_indices[] = {
       0,    5
};
#elif defined(Q_PROCESSOR_X86)
/* Data:
 sse3
 sse2
 avx512vbmi






 ssse3


 fma
 cmpxchg16b





 sse4.1
 sse4.2

 movbe
 popcnt

 aes


 avx
 f16c
 rdrand




 bmi
 hle
 avx2


 bmi2


 rtm




 avx512f
 avx512dq
 rdseed


 avx512ifma




 avx512pf
 avx512er
 avx512cd
 sha
 avx512bw
 avx512vl
 */
static const char features_string[] =
    " sse3\0"
    " sse2\0"
    " avx512vbmi\0"
    " ssse3\0"
    " fma\0"
    " cmpxchg16b\0"
    " sse4.1\0"
    " sse4.2\0"
    " movbe\0"
    " popcnt\0"
    " aes\0"
    " avx\0"
    " f16c\0"
    " rdrand\0"
    " bmi\0"
    " hle\0"
    " avx2\0"
    " bmi2\0"
    " rtm\0"
    " avx512f\0"
    " avx512dq\0"
    " rdseed\0"
    " avx512ifma\0"
    " avx512pf\0"
    " avx512er\0"
    " avx512cd\0"
    " sha\0"
    " avx512bw\0"
    " avx512vl\0"
    "\0";

static const quint8 features_indices[] = {
    0,    6,   12,    5,    5,    5,    5,    5,
    5,   24,    5,    5,   31,   36,    5,    5,
    5,    5,    5,   48,   56,    5,   64,   71,
    5,   79,    5,    5,   84,   89,   95,    5,
    5,    5,    5,  103,  108,  113,    5,    5,
  119,    5,    5,  125,    5,    5,    5,    5,
  130,  139,  149,    5,    5,  157,    5,    5,
    5,    5,  169,  179,  189,  199,  204,  214
};
#else
static const char features_string[] = "";
static const int features_indices[] = { };
#endif
// end generated

static const int features_count = (sizeof features_indices) / (sizeof features_indices[0]);

// record what CPU features were enabled by default in this Qt build
static const quint64 minFeature = qCompilerCpuFeatures;

#ifdef Q_ATOMIC_INT64_IS_SUPPORTED
Q_CORE_EXPORT QBasicAtomicInteger<quint64> qt_cpu_features[1] = { Q_BASIC_ATOMIC_INITIALIZER(0) };
#else
Q_CORE_EXPORT QBasicAtomicInteger<unsigned> qt_cpu_features[2] = { Q_BASIC_ATOMIC_INITIALIZER(0), Q_BASIC_ATOMIC_INITIALIZER(0) };
#endif

void qDetectCpuFeatures()
{
#if defined(Q_CC_GNU) && !defined(Q_CC_CLANG) && !defined(Q_CC_INTEL)
# if Q_CC_GNU < 403
    // GCC 4.2 (at least the one that comes with Apple's XCode, on Mac) is
    // known to be broken beyond repair in dealing with the inline assembly
    // above. It will generate bad code that could corrupt important registers
    // like the PIC register. The behaviour of code after this function would
    // be totally unpredictable.
    //
    // For that reason, simply forego the CPUID check at all and return the set
    // of features that we found at compile time, through the #defines from the
    // compiler. This should at least allow code to execute, even if none of
    // the specialized code found in Qt GUI and elsewhere will ever be enabled
    // (it's the user's fault for using a broken compiler).
    //
    // This also disables the runtime checking that the processor actually
    // contains all the features that the code required. Qt 4 ran for years
    // like that, so it shouldn't be a problem.

    qt_cpu_features[0].store(minFeature | quint32(QSimdInitialized));
#ifndef Q_ATOMIC_INT64_IS_SUPPORTED
    qt_cpu_features[1].store(minFeature >> 32);
#endif

    return;
# endif
#endif
    quint64 f = detectProcessorFeatures();
    QByteArray disable = qgetenv("QT_NO_CPU_FEATURE");
    if (!disable.isEmpty()) {
        disable.prepend(' ');
        for (int i = 0; i < features_count; ++i) {
            if (disable.contains(features_string + features_indices[i]))
                f &= ~(Q_UINT64_C(1) << i);
        }
    }

#ifdef RUNNING_ON_VALGRIND
    bool runningOnValgrind = RUNNING_ON_VALGRIND;
#else
    bool runningOnValgrind = false;
#endif
    if (Q_UNLIKELY(!runningOnValgrind && minFeature != 0 && (f & minFeature) != minFeature)) {
        quint64 missing = minFeature & ~f;
        fprintf(stderr, "Incompatible processor. This Qt build requires the following features:\n   ");
        for (int i = 0; i < features_count; ++i) {
            if (missing & (Q_UINT64_C(1) << i))
                fprintf(stderr, "%s", features_string + features_indices[i]);
        }
        fprintf(stderr, "\n");
        fflush(stderr);
        qFatal("Aborted. Incompatible processor: missing feature 0x%llx -%s.", missing,
               features_string + features_indices[qCountTrailingZeroBits(missing)]);
    }

    qt_cpu_features[0].store(f | quint32(QSimdInitialized));
#ifndef Q_ATOMIC_INT64_IS_SUPPORTED
    qt_cpu_features[1].store(f >> 32);
#endif
}

void qDumpCPUFeatures()
{
    quint64 features = qCpuFeatures() & ~quint64(QSimdInitialized);
    printf("Processor features: ");
    for (int i = 0; i < features_count; ++i) {
        if (features & (Q_UINT64_C(1) << i))
            printf("%s%s", features_string + features_indices[i],
                   minFeature & (Q_UINT64_C(1) << i) ? "[required]" : "");
    }
    puts("");
}

QT_END_NAMESPACE
