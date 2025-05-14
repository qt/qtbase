// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2022 Intel Corporation.
// Copyright (C) 2019 Mail.ru Group.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qstringlist.h"
#if QT_CONFIG(regularexpression)
#include "qregularexpression.h"
#endif
#include "qunicodetables_p.h"
#include <private/qstringconverter_p.h>
#include <private/qtools_p.h>
#include "qlocale_tools_p.h"
#include "private/qsimd_p.h"
#include <qnumeric.h>
#include <qdatastream.h>
#include <qlist.h>
#include "qlocale.h"
#include "qlocale_p.h"
#include "qspan.h"
#include "qstringbuilder.h"
#include "qstringmatcher.h"
#include "qvarlengtharray.h"
#include "qdebug.h"
#include "qendian.h"
#include "qcollator.h"
#include "qttypetraits.h"

#ifdef Q_OS_DARWIN
#include <private/qcore_mac_p.h>
#endif

#include <private/qfunctions_p.h>

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>

#include "qchar.cpp"
#include "qlatin1stringmatcher.h"
#include "qstringmatcher.cpp"
#include "qstringiterator_p.h"
#include "qstringalgorithms_p.h"
#include "qthreadstorage.h"

#include <algorithm>
#include <functional>

#ifdef Q_OS_WIN
#  include <qt_windows.h>
#  if !defined(QT_BOOTSTRAPPED) && (defined(QT_NO_CAST_FROM_ASCII) || defined(QT_NO_CAST_TO_ASCII))
// MSVC requires this, but let's apply it to MinGW compilers too, just in case
#    error "This file cannot be compiled with QT_NO_CAST_{TO,FROM}_ASCII, " \
           "otherwise some QString functions will not get exported."
#  endif
#endif

#ifdef truncate
#  undef truncate
#endif

#define REHASH(a) \
    if (sl_minus_1 < sizeof(sl_minus_1) * CHAR_BIT)  \
        hashHaystack -= decltype(hashHaystack)(a) << sl_minus_1; \
    hashHaystack <<= 1

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QtMiscUtils;

const char16_t QString::_empty = 0;

// in qstringmatcher.cpp
qsizetype qFindStringBoyerMoore(QStringView haystack, qsizetype from, QStringView needle, Qt::CaseSensitivity cs);

namespace {
enum StringComparisonMode {
    CompareStringsForEquality,
    CompareStringsForOrdering
};

template <typename Pointer>
char32_t foldCaseHelper(Pointer ch, Pointer start) = delete;

template <>
char32_t foldCaseHelper<const QChar*>(const QChar* ch, const QChar* start)
{
    return foldCase(reinterpret_cast<const char16_t*>(ch),
                    reinterpret_cast<const char16_t*>(start));
}

template <>
char32_t foldCaseHelper<const char*>(const char* ch, const char*)
{
    return foldCase(char16_t(uchar(*ch)));
}

template <typename T>
char16_t valueTypeToUtf16(T t) = delete;

template <>
char16_t valueTypeToUtf16<QChar>(QChar t)
{
    return t.unicode();
}

template <>
char16_t valueTypeToUtf16<char>(char t)
{
    return char16_t{uchar(t)};
}

template <typename T>
static inline bool foldAndCompare(const T a, const T b)
{
    return foldCase(a) == b;
}

/*!
    \internal

    Returns the index position of the first occurrence of the
    character \a ch in the string given by \a str and \a len,
    searching forward from index
    position \a from. Returns -1 if \a ch could not be found.
*/
template <typename Haystack>
static inline qsizetype qLastIndexOf(Haystack haystack, QChar needle,
                                     qsizetype from, Qt::CaseSensitivity cs) noexcept
{
    if (haystack.size() == 0)
        return -1;
    if (from < 0)
        from += haystack.size();
    else if (std::size_t(from) > std::size_t(haystack.size()))
        from = haystack.size() - 1;
    if (from >= 0) {
        char16_t c = needle.unicode();
        const auto b = haystack.data();
        auto n = b + from;
        if (cs == Qt::CaseSensitive) {
            for (; n >= b; --n)
                if (valueTypeToUtf16(*n) == c)
                    return n - b;
        } else {
            c = foldCase(c);
            for (; n >= b; --n)
                if (foldCase(valueTypeToUtf16(*n)) == c)
                    return n - b;
        }
    }
    return -1;
}
template <> qsizetype
qLastIndexOf(QString, QChar, qsizetype, Qt::CaseSensitivity) noexcept = delete; // unwanted, would detach

template<typename Haystack, typename Needle>
static qsizetype qLastIndexOf(Haystack haystack0, qsizetype from,
                              Needle needle0, Qt::CaseSensitivity cs) noexcept
{
    const qsizetype sl = needle0.size();
    if (sl == 1)
        return qLastIndexOf(haystack0, needle0.front(), from, cs);

    const qsizetype l = haystack0.size();
    if (from < 0)
        from += l;
    if (from == l && sl == 0)
        return from;
    const qsizetype delta = l - sl;
    if (std::size_t(from) > std::size_t(l) || delta < 0)
        return -1;
    if (from > delta)
        from = delta;

    auto sv = [sl](const typename Haystack::value_type *v) { return Haystack(v, sl); };

    auto haystack = haystack0.data();
    const auto needle = needle0.data();
    const auto *end = haystack;
    haystack += from;
    const qregisteruint sl_minus_1 = sl ? sl - 1 : 0;
    const auto *n = needle + sl_minus_1;
    const auto *h = haystack + sl_minus_1;
    qregisteruint hashNeedle = 0, hashHaystack = 0;

    if (cs == Qt::CaseSensitive) {
        for (qsizetype idx = 0; idx < sl; ++idx) {
            hashNeedle = (hashNeedle << 1) + valueTypeToUtf16(*(n - idx));
            hashHaystack = (hashHaystack << 1) + valueTypeToUtf16(*(h - idx));
        }
        hashHaystack -= valueTypeToUtf16(*haystack);

        while (haystack >= end) {
            hashHaystack += valueTypeToUtf16(*haystack);
            if (hashHaystack == hashNeedle
                 && QtPrivate::compareStrings(needle0, sv(haystack), Qt::CaseSensitive) == 0)
                return haystack - end;
            --haystack;
            REHASH(valueTypeToUtf16(haystack[sl]));
        }
    } else {
        for (qsizetype idx = 0; idx < sl; ++idx) {
            hashNeedle = (hashNeedle << 1) + foldCaseHelper(n - idx, needle);
            hashHaystack = (hashHaystack << 1) + foldCaseHelper(h - idx, end);
        }
        hashHaystack -= foldCaseHelper(haystack, end);

        while (haystack >= end) {
            hashHaystack += foldCaseHelper(haystack, end);
            if (hashHaystack == hashNeedle
                 && QtPrivate::compareStrings(sv(haystack), needle0, Qt::CaseInsensitive) == 0)
                return haystack - end;
            --haystack;
            REHASH(foldCaseHelper(haystack + sl, end));
        }
    }
    return -1;
}

template <typename Haystack, typename Needle>
bool qt_starts_with_impl(Haystack haystack, Needle needle, Qt::CaseSensitivity cs) noexcept
{
    if (haystack.isNull())
        return needle.isNull();
    const auto haystackLen = haystack.size();
    const auto needleLen = needle.size();
    if (haystackLen == 0)
        return needleLen == 0;
    if (needleLen > haystackLen)
        return false;

    return QtPrivate::compareStrings(haystack.first(needleLen), needle, cs) == 0;
}

template <typename Haystack, typename Needle>
bool qt_ends_with_impl(Haystack haystack, Needle needle, Qt::CaseSensitivity cs) noexcept
{
    if (haystack.isNull())
        return needle.isNull();
    const auto haystackLen = haystack.size();
    const auto needleLen = needle.size();
    if (haystackLen == 0)
        return needleLen == 0;
    if (haystackLen < needleLen)
        return false;

    return QtPrivate::compareStrings(haystack.last(needleLen), needle, cs) == 0;
}

template <typename T>
static void append_helper(QString &self, T view)
{
    const auto strData = view.data();
    const qsizetype strSize = view.size();
    auto &d = self.data_ptr();
    if (strData && strSize > 0) {
        // the number of UTF-8 code units is always at a minimum equal to the number
        // of equivalent UTF-16 code units
        d.detachAndGrow(QArrayData::GrowsAtEnd, strSize, nullptr, nullptr);
        Q_CHECK_PTR(d.data());
        Q_ASSERT(strSize <= d.freeSpaceAtEnd());

        auto dst = std::next(d.data(), d.size);
        if constexpr (std::is_same_v<T, QUtf8StringView>) {
            dst = QUtf8::convertToUnicode(dst, view);
        } else if constexpr (std::is_same_v<T, QLatin1StringView>) {
            QLatin1::convertToUnicode(dst, view);
            dst += strSize;
        } else {
            static_assert(QtPrivate::type_dependent_false<T>(),
                          "Can only operate on UTF-8 and Latin-1");
        }
        self.resize(std::distance(d.begin(), dst));
    } else if (d.isNull() && !view.isNull()) { // special case
        self = QLatin1StringView("");
    }
}

template <uint MaxCount> struct UnrollTailLoop
{
    template <typename RetType, typename Functor1, typename Functor2, typename Number>
    static inline RetType exec(Number count, RetType returnIfExited, Functor1 loopCheck, Functor2 returnIfFailed, Number i = 0)
    {
        /* equivalent to:
         *   while (count--) {
         *       if (loopCheck(i))
         *           return returnIfFailed(i);
         *   }
         *   return returnIfExited;
         */

        if (!count)
            return returnIfExited;

        bool check = loopCheck(i);
        if (check)
            return returnIfFailed(i);

        return UnrollTailLoop<MaxCount - 1>::exec(count - 1, returnIfExited, loopCheck, returnIfFailed, i + 1);
    }

    template <typename Functor, typename Number>
    static inline void exec(Number count, Functor code)
    {
        /* equivalent to:
         *   for (Number i = 0; i < count; ++i)
         *       code(i);
         */
        exec(count, 0, [=](Number i) -> bool { code(i); return false; }, [](Number) { return 0; });
    }
};
template <> template <typename RetType, typename Functor1, typename Functor2, typename Number>
inline RetType UnrollTailLoop<0>::exec(Number, RetType returnIfExited, Functor1, Functor2, Number)
{
    return returnIfExited;
}
} // unnamed namespace

/*
 * Note on the use of SIMD in qstring.cpp:
 *
 * Several operations with strings are improved with the use of SIMD code,
 * since they are repetitive. For MIPS, we have hand-written assembly code
 * outside of qstring.cpp targeting MIPS DSP and MIPS DSPr2. For ARM and for
 * x86, we can only use intrinsics and therefore everything is contained in
 * qstring.cpp. We need to use intrinsics only for those platforms due to the
 * different compilers and toolchains used, which have different syntax for
 * assembly sources.
 *
 * ** SSE notes: **
 *
 * Whenever multiple alternatives are equivalent or near so, we prefer the one
 * using instructions from SSE2, since SSE2 is guaranteed to be enabled for all
 * 64-bit builds and we enable it for 32-bit builds by default. Use of higher
 * SSE versions should be done when there is a clear performance benefit and
 * requires fallback code to SSE2, if it exists.
 *
 * Performance measurement in the past shows that most strings are short in
 * size and, therefore, do not benefit from alignment prologues. That is,
 * trying to find a 16-byte-aligned boundary to operate on is often more
 * expensive than executing the unaligned operation directly. In addition, note
 * that the QString private data is designed so that the data is stored on
 * 16-byte boundaries if the system malloc() returns 16-byte aligned pointers
 * on its own (64-bit glibc on Linux does; 32-bit glibc on Linux returns them
 * 50% of the time), so skipping the alignment prologue is actually optimizing
 * for the common case.
 */

#if defined(__mips_dsp)
// From qstring_mips_dsp_asm.S
extern "C" void qt_fromlatin1_mips_asm_unroll4 (char16_t*, const char*, uint);
extern "C" void qt_fromlatin1_mips_asm_unroll8 (char16_t*, const char*, uint);
extern "C" void qt_toLatin1_mips_dsp_asm(uchar *dst, const char16_t *src, int length);
#endif

#if defined(__SSE2__) && defined(Q_CC_GNU)
// We may overrun the buffer, but that's a false positive:
// this won't crash nor produce incorrect results
#  define ATTRIBUTE_NO_SANITIZE       __attribute__((__no_sanitize_address__, __no_sanitize_thread__))
#else
#  define ATTRIBUTE_NO_SANITIZE
#endif

#ifdef __SSE2__
static constexpr bool UseSse4_1 = bool(qCompilerCpuFeatures & CpuFeatureSSE4_1);
static constexpr bool UseAvx2 = UseSse4_1 &&
        (qCompilerCpuFeatures & CpuFeatureArchHaswell) == CpuFeatureArchHaswell;

[[maybe_unused]]
Q_ALWAYS_INLINE static __m128i mm_load8_zero_extend(const void *ptr)
{
    const __m128i *dataptr = static_cast<const __m128i *>(ptr);
    if constexpr (UseSse4_1) {
        // use a MOVQ followed by PMOVZXBW
        // if AVX2 is present, these should combine into a single VPMOVZXBW instruction
        __m128i data = _mm_loadl_epi64(dataptr);
        return _mm_cvtepu8_epi16(data);
    }

    // use MOVQ followed by PUNPCKLBW
    __m128i data = _mm_loadl_epi64(dataptr);
    return _mm_unpacklo_epi8(data, _mm_setzero_si128());
}

[[maybe_unused]] ATTRIBUTE_NO_SANITIZE
static qsizetype qustrlen_sse2(const char16_t *str) noexcept
{
    // find the 16-byte alignment immediately prior or equal to str
    quintptr misalignment = quintptr(str) & 0xf;
    Q_ASSERT((misalignment & 1) == 0);
    const char16_t *ptr = str - (misalignment / 2);

    // load 16 bytes and see if we have a null
    // (aligned loads can never segfault)
    const __m128i zeroes = _mm_setzero_si128();
    __m128i data = _mm_load_si128(reinterpret_cast<const __m128i *>(ptr));
    __m128i comparison = _mm_cmpeq_epi16(data, zeroes);
    uint mask = _mm_movemask_epi8(comparison);

    // ignore the result prior to the beginning of str
    mask >>= misalignment;

    // Have we found something in the first block? Need to handle it now
    // because of the left shift above.
    if (mask)
        return qCountTrailingZeroBits(mask) / sizeof(char16_t);

    constexpr qsizetype Step = sizeof(__m128i) / sizeof(char16_t);
    qsizetype size = Step - misalignment / sizeof(char16_t);

    size -= Step;
    do {
        size += Step;
        data = _mm_load_si128(reinterpret_cast<const __m128i *>(str + size));

        comparison = _mm_cmpeq_epi16(data, zeroes);
        mask = _mm_movemask_epi8(comparison);
    } while (mask == 0);

    // found a null
    return size + qCountTrailingZeroBits(mask) / sizeof(char16_t);
}

// Scans from \a ptr to \a end until \a maskval is non-zero. Returns true if
// the no non-zero was found. Returns false and updates \a ptr to point to the
// first 16-bit word that has any bit set (note: if the input is 8-bit, \a ptr
// may be updated to one byte short).
static bool simdTestMask(const char *&ptr, const char *end, quint32 maskval)
{
    auto updatePtr = [&](uint result) {
        // found a character matching the mask
        uint idx = qCountTrailingZeroBits(~result);
        ptr += idx;
        return false;
    };

    if constexpr (UseSse4_1) {
#  ifndef Q_OS_QNX              // compiler fails in the code below
        __m128i mask;
        auto updatePtrSimd = [&](__m128i data) -> bool {
            __m128i masked = _mm_and_si128(mask, data);
            __m128i comparison = _mm_cmpeq_epi16(masked, _mm_setzero_si128());
            uint result = _mm_movemask_epi8(comparison);
            return updatePtr(result);
        };

        if constexpr (UseAvx2) {
            // AVX2 implementation: test 32 bytes at a time
            const __m256i mask256 = _mm256_broadcastd_epi32(_mm_cvtsi32_si128(maskval));
            while (ptr + 32 <= end) {
                __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(ptr));
                if (!_mm256_testz_si256(mask256, data)) {
                    // found a character matching the mask
                    __m256i masked256 = _mm256_and_si256(mask256, data);
                    __m256i comparison256 = _mm256_cmpeq_epi16(masked256, _mm256_setzero_si256());
                    return updatePtr(_mm256_movemask_epi8(comparison256));
                }
                ptr += 32;
            }

            mask = _mm256_castsi256_si128(mask256);
        } else {
            // SSE 4.1 implementation: test 32 bytes at a time (two 16-byte
            // comparisons, unrolled)
            mask = _mm_set1_epi32(maskval);
            while (ptr + 32 <= end) {
                __m128i data1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr));
                __m128i data2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 16));
                if (!_mm_testz_si128(mask, data1))
                    return updatePtrSimd(data1);

                ptr += 16;
                if (!_mm_testz_si128(mask, data2))
                    return updatePtrSimd(data2);
                ptr += 16;
            }
        }

        // AVX2 and SSE4.1: final 16-byte comparison
        if (ptr + 16 <= end) {
            __m128i data1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr));
            if (!_mm_testz_si128(mask, data1))
                return updatePtrSimd(data1);
            ptr += 16;
        }

        // and final 8-byte comparison
        if (ptr + 8 <= end) {
            __m128i data1 = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(ptr));
            if (!_mm_testz_si128(mask, data1))
                return updatePtrSimd(data1);
            ptr += 8;
        }

        return true;
#  endif // QNX
    }

    // SSE2 implementation: test 16 bytes at a time.
    const __m128i mask = _mm_set1_epi32(maskval);
    while (ptr + 16 <= end) {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr));
        __m128i masked = _mm_and_si128(mask, data);
        __m128i comparison = _mm_cmpeq_epi16(masked, _mm_setzero_si128());
        quint16 result = _mm_movemask_epi8(comparison);
        if (result != 0xffff)
            return updatePtr(result);
        ptr += 16;
    }

    // and one 8-byte comparison
    if (ptr + 8 <= end) {
        __m128i data = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(ptr));
        __m128i masked = _mm_and_si128(mask, data);
        __m128i comparison = _mm_cmpeq_epi16(masked, _mm_setzero_si128());
        quint8 result = _mm_movemask_epi8(comparison);
        if (result != 0xff)
            return updatePtr(result);
        ptr += 8;
    }

    return true;
}

template <StringComparisonMode Mode, typename Char> [[maybe_unused]]
static int ucstrncmp_sse2(const char16_t *a, const Char *b, size_t l)
{
    static_assert(std::is_unsigned_v<Char>);

    // Using the PMOVMSKB instruction, we get two bits for each UTF-16 character
    // we compare. This lambda helps extract the code unit.
    static const auto codeUnitAt = [](const auto *n, qptrdiff idx) -> int {
        constexpr int Stride = 2;
        // this is the same as:
        //    return n[idx / Stride];
        // but using pointer arithmetic to avoid the compiler dividing by two
        // and multiplying by two in the case of char16_t (we know idx is even,
        // but the compiler does not). This is not UB.

        auto ptr = reinterpret_cast<const uchar *>(n);
        ptr += idx / (Stride / sizeof(*n));
        return *reinterpret_cast<decltype(n)>(ptr);
    };
    auto difference = [a, b](uint mask, qptrdiff offset) {
        if (Mode == CompareStringsForEquality)
            return 1;
        uint idx = qCountTrailingZeroBits(mask);
        return codeUnitAt(a + offset, idx) - codeUnitAt(b + offset, idx);
    };

    static const auto load8Chars = [](const auto *ptr) {
        if (sizeof(*ptr) == 2)
            return _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr));
        __m128i chunk = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(ptr));
        return _mm_unpacklo_epi8(chunk, _mm_setzero_si128());
    };
    static const auto load4Chars = [](const auto *ptr) {
        if (sizeof(*ptr) == 2)
            return _mm_loadl_epi64(reinterpret_cast<const __m128i *>(ptr));
        __m128i chunk = _mm_cvtsi32_si128(qFromUnaligned<quint32>(ptr));
        return _mm_unpacklo_epi8(chunk, _mm_setzero_si128());
    };

    // we're going to read a[0..15] and b[0..15] (32 bytes)
    auto processChunk16Chars = [a, b](qptrdiff offset) -> uint {
        if constexpr (UseAvx2) {
            __m256i a_data = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(a + offset));
            __m256i b_data;
            if (sizeof(Char) == 1) {
                // expand to UTF-16 via zero-extension
                __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(b + offset));
                b_data = _mm256_cvtepu8_epi16(chunk);
            } else {
                b_data = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(b + offset));
            }
            __m256i result = _mm256_cmpeq_epi16(a_data, b_data);
            return _mm256_movemask_epi8(result);
        }

        __m128i a_data1 = load8Chars(a + offset);
        __m128i a_data2 = load8Chars(a + offset + 8);
        __m128i b_data1, b_data2;
        if (sizeof(Char) == 1) {
            // expand to UTF-16 via unpacking
            __m128i b_data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(b + offset));
            b_data1 = _mm_unpacklo_epi8(b_data, _mm_setzero_si128());
            b_data2 = _mm_unpackhi_epi8(b_data, _mm_setzero_si128());
        } else {
            b_data1 = load8Chars(b + offset);
            b_data2 = load8Chars(b + offset + 8);
        }
        __m128i result1 = _mm_cmpeq_epi16(a_data1, b_data1);
        __m128i result2 = _mm_cmpeq_epi16(a_data2, b_data2);
        return _mm_movemask_epi8(result1) | _mm_movemask_epi8(result2) << 16;
    };

    if (l >= sizeof(__m256i) / sizeof(char16_t)) {
        qptrdiff offset = 0;
        for ( ; l >= offset + sizeof(__m256i) / sizeof(char16_t); offset += sizeof(__m256i) / sizeof(char16_t)) {
            uint mask = ~processChunk16Chars(offset);
            if (mask)
                return difference(mask, offset);
        }

        // maybe overlap the last 32 bytes
        if (size_t(offset) < l) {
            offset = l - sizeof(__m256i) / sizeof(char16_t);
            uint mask = ~processChunk16Chars(offset);
            return mask ? difference(mask, offset) : 0;
        }
    } else if (l >= 4) {
        __m128i a_data1, b_data1;
        __m128i a_data2, b_data2;
        int width;
        if (l >= 8) {
            width = 8;
            a_data1 = load8Chars(a);
            b_data1 = load8Chars(b);
            a_data2 = load8Chars(a + l - width);
            b_data2 = load8Chars(b + l - width);
        } else {
            // we're going to read a[0..3] and b[0..3] (8 bytes)
            width = 4;
            a_data1 = load4Chars(a);
            b_data1 = load4Chars(b);
            a_data2 = load4Chars(a + l - width);
            b_data2 = load4Chars(b + l - width);
        }

        __m128i result = _mm_cmpeq_epi16(a_data1, b_data1);
        ushort mask = ~_mm_movemask_epi8(result);
        if (mask)
            return difference(mask, 0);

        result = _mm_cmpeq_epi16(a_data2, b_data2);
        mask = ~_mm_movemask_epi8(result);
        if (mask)
            return difference(mask, l - width);
    } else {
        // reset l
        l &= 3;

        const auto lambda = [=](size_t i) -> int {
            return a[i] - b[i];
        };
        return UnrollTailLoop<3>::exec(l, 0, lambda, lambda);
    }
    return 0;
}
#endif

Q_NEVER_INLINE
qsizetype QtPrivate::qustrlen(const char16_t *str) noexcept
{
#if defined(__SSE2__) && !(defined(__SANITIZE_ADDRESS__) || __has_feature(address_sanitizer)) && !(defined(__SANITIZE_THREAD__) || __has_feature(thread_sanitizer))
    return qustrlen_sse2(str);
#endif

    if (sizeof(wchar_t) == sizeof(char16_t))
        return wcslen(reinterpret_cast<const wchar_t *>(str));

    qsizetype result = 0;
    while (*str++)
        ++result;
    return result;
}

qsizetype QtPrivate::qustrnlen(const char16_t *str, qsizetype maxlen) noexcept
{
    return qustrchr({ str, maxlen }, u'\0') - str;
}

/*!
 * \internal
 *
 * Searches for character \a c in the string \a str and returns a pointer to
 * it. Unlike strchr() and wcschr() (but like glibc's strchrnul()), if the
 * character is not found, this function returns a pointer to the end of the
 * string -- that is, \c{str.end()}.
 */
Q_NEVER_INLINE
const char16_t *QtPrivate::qustrchr(QStringView str, char16_t c) noexcept
{
    const char16_t *n = str.utf16();
    const char16_t *e = n + str.size();

#ifdef __SSE2__
    bool loops = true;
    // Using the PMOVMSKB instruction, we get two bits for each character
    // we compare.
    __m128i mch;
    if constexpr (UseAvx2) {
        // we're going to read n[0..15] (32 bytes)
        __m256i mch256 = _mm256_set1_epi32(c | (c << 16));
        for (const char16_t *next = n + 16; next <= e; n = next, next += 16) {
            __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(n));
            __m256i result = _mm256_cmpeq_epi16(data, mch256);
            uint mask = uint(_mm256_movemask_epi8(result));
            if (mask) {
                uint idx = qCountTrailingZeroBits(mask);
                return n + idx / 2;
            }
        }
        loops = false;
        mch = _mm256_castsi256_si128(mch256);
    } else {
        mch = _mm_set1_epi32(c | (c << 16));
    }

    auto hasMatch = [mch, &n](__m128i data, ushort validityMask) {
        __m128i result = _mm_cmpeq_epi16(data, mch);
        uint mask = uint(_mm_movemask_epi8(result));
        if ((mask & validityMask) == 0)
            return false;
        uint idx = qCountTrailingZeroBits(mask);
        n += idx / 2;
        return true;
    };

    // we're going to read n[0..7] (16 bytes)
    for (const char16_t *next = n + 8; next <= e; n = next, next += 8) {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(n));
        if (hasMatch(data, 0xffff))
            return n;

        if (!loops) {
            n += 8;
            break;
        }
    }

#  if !defined(__OPTIMIZE_SIZE__)
    // we're going to read n[0..3] (8 bytes)
    if (e - n > 3) {
        __m128i data = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(n));
        if (hasMatch(data, 0xff))
            return n;

        n += 4;
    }

    return UnrollTailLoop<3>::exec(e - n, e,
                                   [=](qsizetype i) { return n[i] == c; },
                                   [=](qsizetype i) { return n + i; });
#  endif
#elif defined(__ARM_NEON__)
    const uint16x8_t vmask = qvsetq_n_u16(1, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7);
    const uint16x8_t ch_vec = vdupq_n_u16(c);
    for (const char16_t *next = n + 8; next <= e; n = next, next += 8) {
        uint16x8_t data = vld1q_u16(reinterpret_cast<const uint16_t *>(n));
        uint mask = vaddvq_u16(vandq_u16(vceqq_u16(data, ch_vec), vmask));
        if (ushort(mask)) {
            // found a match
            return n + qCountTrailingZeroBits(mask);
        }
    }
#endif // aarch64

    return std::find(n, e, c);
}

/*!
 * \internal
 *
 * Searches case-insensitively for character \a c in the string \a str and
 * returns a pointer to it. Iif the character is not found, this function
 * returns a pointer to the end of the string -- that is, \c{str.end()}.
 */
Q_NEVER_INLINE
const char16_t *QtPrivate::qustrcasechr(QStringView str, char16_t c) noexcept
{
    const QChar *n = str.begin();
    const QChar *e = str.end();
    c = foldCase(c);
    auto it = std::find_if(n, e, [c](auto ch) { return foldAndCompare(ch, QChar(c)); });
    return reinterpret_cast<const char16_t *>(it);
}

// Note: ptr on output may be off by one and point to a preceding US-ASCII
// character. Usually harmless.
bool qt_is_ascii(const char *&ptr, const char *end) noexcept
{
#if defined(__SSE2__)
    // Testing for the high bit can be done efficiently with just PMOVMSKB
    bool loops = true;
    if constexpr (UseAvx2) {
        while (ptr + 32 <= end) {
            __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(ptr));
            quint32 mask = _mm256_movemask_epi8(data);
            if (mask) {
                uint idx = qCountTrailingZeroBits(mask);
                ptr += idx;
                return false;
            }
            ptr += 32;
        }
        loops = false;
    }

    while (ptr + 16 <= end) {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr));
        quint32 mask = _mm_movemask_epi8(data);
        if (mask) {
            uint idx = qCountTrailingZeroBits(mask);
            ptr += idx;
            return false;
        }
        ptr += 16;

        if (!loops)
            break;
    }
    if (ptr + 8 <= end) {
        __m128i data = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(ptr));
        quint8 mask = _mm_movemask_epi8(data);
        if (mask) {
            uint idx = qCountTrailingZeroBits(mask);
            ptr += idx;
            return false;
        }
        ptr += 8;
    }
#endif

    while (ptr + 4 <= end) {
        quint32 data = qFromUnaligned<quint32>(ptr);
        if (data &= 0x80808080U) {
            uint idx = QSysInfo::ByteOrder == QSysInfo::BigEndian
                    ? qCountLeadingZeroBits(data)
                    : qCountTrailingZeroBits(data);
            ptr += idx / 8;
            return false;
        }
        ptr += 4;
    }

    while (ptr != end) {
        if (quint8(*ptr) & 0x80)
            return false;
        ++ptr;
    }
    return true;
}

bool QtPrivate::isAscii(QLatin1StringView s) noexcept
{
    const char *ptr = s.begin();
    const char *end = s.end();

    return qt_is_ascii(ptr, end);
}

static bool isAscii_helper(const char16_t *&ptr, const char16_t *end)
{
#ifdef __SSE2__
    const char *ptr8 = reinterpret_cast<const char *>(ptr);
    const char *end8 = reinterpret_cast<const char *>(end);
    bool ok = simdTestMask(ptr8, end8, 0xff80ff80);
    ptr = reinterpret_cast<const char16_t *>(ptr8);
    if (!ok)
        return false;
#endif

    while (ptr != end) {
        if (*ptr & 0xff80)
            return false;
        ++ptr;
    }
    return true;
}

bool QtPrivate::isAscii(QStringView s) noexcept
{
    const char16_t *ptr = s.utf16();
    const char16_t *end = ptr + s.size();

    return isAscii_helper(ptr, end);
}

bool QtPrivate::isLatin1(QStringView s) noexcept
{
    const char16_t *ptr = s.utf16();
    const char16_t *end = ptr + s.size();

#ifdef __SSE2__
    const char *ptr8 = reinterpret_cast<const char *>(ptr);
    const char *end8 = reinterpret_cast<const char *>(end);
    if (!simdTestMask(ptr8, end8, 0xff00ff00))
        return false;
    ptr = reinterpret_cast<const char16_t *>(ptr8);
#endif

    while (ptr != end) {
        if (*ptr++ > 0xff)
            return false;
    }
    return true;
}

bool QtPrivate::isValidUtf16(QStringView s) noexcept
{
    constexpr char32_t InvalidCodePoint = UINT_MAX;

    QStringIterator i(s);
    while (i.hasNext()) {
        const char32_t c = i.next(InvalidCodePoint);
        if (c == InvalidCodePoint)
            return false;
    }

    return true;
}

// conversion between Latin 1 and UTF-16
Q_CORE_EXPORT void qt_from_latin1(char16_t *dst, const char *str, size_t size) noexcept
{
    /* SIMD:
     * Unpacking with SSE has been shown to improve performance on recent CPUs
     * The same method gives no improvement with NEON. On Aarch64, clang will do the vectorization
     * itself in exactly the same way as one would do it with intrinsics.
     */
#if defined(__SSE2__)
    // we're going to read str[offset..offset+15] (16 bytes)
    const __m128i nullMask = _mm_setzero_si128();
    auto processOneChunk = [=](qptrdiff offset) {
        const __m128i chunk = _mm_loadu_si128((const __m128i*)(str + offset)); // load
        if constexpr (UseAvx2) {
            // zero extend to an YMM register
            const __m256i extended = _mm256_cvtepu8_epi16(chunk);

            // store
            _mm256_storeu_si256((__m256i*)(dst + offset), extended);
        } else {
            // unpack the first 8 bytes, padding with zeros
            const __m128i firstHalf = _mm_unpacklo_epi8(chunk, nullMask);
            _mm_storeu_si128((__m128i*)(dst + offset), firstHalf); // store

            // unpack the last 8 bytes, padding with zeros
            const __m128i secondHalf = _mm_unpackhi_epi8 (chunk, nullMask);
            _mm_storeu_si128((__m128i*)(dst + offset + 8), secondHalf); // store
        }
    };

    const char *e = str + size;
    if (size >= sizeof(__m128i)) {
        qptrdiff offset = 0;
        for ( ; str + offset + sizeof(__m128i) <= e; offset += sizeof(__m128i))
            processOneChunk(offset);
        if (str + offset < e)
            processOneChunk(size - sizeof(__m128i));
        return;
    }

#  if !defined(__OPTIMIZE_SIZE__)
    if (size >= 4) {
        // two overlapped loads & stores, of either 64-bit or of 32-bit
        if (size >= 8) {
            const __m128i unpacked1 = mm_load8_zero_extend(str);
            const __m128i unpacked2 = mm_load8_zero_extend(str + size - 8);
            _mm_storeu_si128(reinterpret_cast<__m128i *>(dst), unpacked1);
            _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + size -  8), unpacked2);
        } else {
            const __m128i chunk1 = _mm_cvtsi32_si128(qFromUnaligned<quint32>(str));
            const __m128i chunk2 = _mm_cvtsi32_si128(qFromUnaligned<quint32>(str + size - 4));
            const __m128i unpacked1 = _mm_unpacklo_epi8(chunk1, nullMask);
            const __m128i unpacked2 = _mm_unpacklo_epi8(chunk2, nullMask);
            _mm_storel_epi64(reinterpret_cast<__m128i *>(dst), unpacked1);
            _mm_storel_epi64(reinterpret_cast<__m128i *>(dst + size - 4), unpacked2);
        }
        return;
    } else {
        size = size % 4;
        return UnrollTailLoop<3>::exec(qsizetype(size), [=](qsizetype i) { dst[i] = uchar(str[i]); });
    }
#  endif
#endif
#if defined(__mips_dsp)
    static_assert(sizeof(qsizetype) == sizeof(int),
                  "oops, the assembler implementation needs to be called in a loop");
    if (size > 20)
        qt_fromlatin1_mips_asm_unroll8(dst, str, size);
    else
        qt_fromlatin1_mips_asm_unroll4(dst, str, size);
#else
    while (size--)
        *dst++ = (uchar)*str++;
#endif
}

static QVarLengthArray<char16_t> qt_from_latin1_to_qvla(QLatin1StringView str)
{
    const qsizetype len = str.size();
    QVarLengthArray<char16_t> arr(len);
    qt_from_latin1(arr.data(), str.data(), len);
    return arr;
}

template <bool Checked>
static void qt_to_latin1_internal(uchar *dst, const char16_t *src, qsizetype length)
{
#if defined(__SSE2__)
    auto questionMark256 = []() {
        if constexpr (UseAvx2)
            return _mm256_broadcastw_epi16(_mm_cvtsi32_si128('?'));
        else
            return 0;
    }();
    auto outOfRange256 = []() {
        if constexpr (UseAvx2)
            return _mm256_broadcastw_epi16(_mm_cvtsi32_si128(0x100));
        else
            return 0;
    }();
    __m128i questionMark, outOfRange;
    if constexpr (UseAvx2) {
        questionMark = _mm256_castsi256_si128(questionMark256);
        outOfRange = _mm256_castsi256_si128(outOfRange256);
    } else {
        questionMark = _mm_set1_epi16('?');
        outOfRange = _mm_set1_epi16(0x100);
    }

    auto mergeQuestionMarks = [=](__m128i chunk) {
        if (!Checked)
            return chunk;

        // SSE has no compare instruction for unsigned comparison.
        if constexpr (UseSse4_1) {
            // We use an unsigned uc = qMin(uc, 0x100) and then compare for equality.
            chunk = _mm_min_epu16(chunk, outOfRange);
            const __m128i offLimitMask = _mm_cmpeq_epi16(chunk, outOfRange);
            chunk = _mm_blendv_epi8(chunk, questionMark, offLimitMask);
            return chunk;
        }
        // The variables must be shiffted + 0x8000 to be compared
        const __m128i signedBitOffset = _mm_set1_epi16(short(0x8000));
        const __m128i thresholdMask = _mm_set1_epi16(short(0xff + 0x8000));

        const __m128i signedChunk = _mm_add_epi16(chunk, signedBitOffset);
        const __m128i offLimitMask = _mm_cmpgt_epi16(signedChunk, thresholdMask);

        // offLimitQuestionMark contains '?' for each 16 bits that was off-limit
        // the 16 bits that were correct contains zeros
        const __m128i offLimitQuestionMark = _mm_and_si128(offLimitMask, questionMark);

        // correctBytes contains the bytes that were in limit
        // the 16 bits that were off limits contains zeros
        const __m128i correctBytes = _mm_andnot_si128(offLimitMask, chunk);

        // merge offLimitQuestionMark and correctBytes to have the result
        chunk = _mm_or_si128(correctBytes, offLimitQuestionMark);

        Q_UNUSED(outOfRange);
        return chunk;
    };

    // we're going to read to src[offset..offset+15] (16 bytes)
    auto loadChunkAt = [=](qptrdiff offset) {
        __m128i chunk1, chunk2;
        if constexpr (UseAvx2) {
            __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + offset));
            if (Checked) {
                // See mergeQuestionMarks lambda above for details
                chunk = _mm256_min_epu16(chunk, outOfRange256);
                const __m256i offLimitMask = _mm256_cmpeq_epi16(chunk, outOfRange256);
                chunk = _mm256_blendv_epi8(chunk, questionMark256, offLimitMask);
            }

            chunk2 = _mm256_extracti128_si256(chunk, 1);
            chunk1 = _mm256_castsi256_si128(chunk);
        } else {
            chunk1 = _mm_loadu_si128((const __m128i*)(src + offset)); // load
            chunk1 = mergeQuestionMarks(chunk1);

            chunk2 = _mm_loadu_si128((const __m128i*)(src + offset + 8)); // load
            chunk2 = mergeQuestionMarks(chunk2);
        }

        // pack the two vector to 16 x 8bits elements
        return _mm_packus_epi16(chunk1, chunk2);
    };

    if (size_t(length) >= sizeof(__m128i)) {
        // because of possible overlapping, we won't process the last chunk in the loop
        qptrdiff offset = 0;
        for ( ; offset + 2 * sizeof(__m128i) < size_t(length); offset += sizeof(__m128i))
            _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + offset), loadChunkAt(offset));

        // overlapped conversion of the last full chunk and the tail
        __m128i last1 = loadChunkAt(offset);
        __m128i last2 = loadChunkAt(length - sizeof(__m128i));
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + offset), last1);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + length - sizeof(__m128i)), last2);
        return;
    }

#  if !defined(__OPTIMIZE_SIZE__)
    if (length >= 4) {
        // this code is fine even for in-place conversion because we load both
        // before any store
        if (length >= 8) {
            __m128i chunk1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));
            __m128i chunk2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + length - 8));
            chunk1 = mergeQuestionMarks(chunk1);
            chunk2 = mergeQuestionMarks(chunk2);

            // pack, where the upper half is ignored
            const __m128i result1 = _mm_packus_epi16(chunk1, chunk1);
            const __m128i result2 = _mm_packus_epi16(chunk2, chunk2);
            _mm_storel_epi64(reinterpret_cast<__m128i *>(dst), result1);
            _mm_storel_epi64(reinterpret_cast<__m128i *>(dst + length - 8), result2);
        } else {
            __m128i chunk1 = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(src));
            __m128i chunk2 = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(src + length - 4));
            chunk1 = mergeQuestionMarks(chunk1);
            chunk2 = mergeQuestionMarks(chunk2);

            // pack, we'll zero the upper three quarters
            const __m128i result1 = _mm_packus_epi16(chunk1, chunk1);
            const __m128i result2 = _mm_packus_epi16(chunk2, chunk2);
            qToUnaligned(_mm_cvtsi128_si32(result1), dst);
            qToUnaligned(_mm_cvtsi128_si32(result2), dst + length - 4);
        }
        return;
    }

    length = length % 4;
    return UnrollTailLoop<3>::exec(length, [=](qsizetype i) {
        if (Checked)
            dst[i] = (src[i]>0xff) ? '?' : (uchar) src[i];
        else
            dst[i] = src[i];
    });
#  else
    length = length % 16;
#  endif // optimize size
#elif defined(__ARM_NEON__)
    // Refer to the documentation of the SSE2 implementation.
    // This uses exactly the same method as for SSE except:
    // 1) neon has unsigned comparison
    // 2) packing is done to 64 bits (8 x 8bits component).
    if (length >= 16) {
        const qsizetype chunkCount = length >> 3; // divided by 8
        const uint16x8_t questionMark = vdupq_n_u16('?'); // set
        const uint16x8_t thresholdMask = vdupq_n_u16(0xff); // set
        for (qsizetype i = 0; i < chunkCount; ++i) {
            uint16x8_t chunk = vld1q_u16((uint16_t *)src); // load
            src += 8;

            if (Checked) {
                const uint16x8_t offLimitMask = vcgtq_u16(chunk, thresholdMask); // chunk > thresholdMask
                const uint16x8_t offLimitQuestionMark = vandq_u16(offLimitMask, questionMark); // offLimitMask & questionMark
                const uint16x8_t correctBytes = vbicq_u16(chunk, offLimitMask); // !offLimitMask & chunk
                chunk = vorrq_u16(correctBytes, offLimitQuestionMark); // correctBytes | offLimitQuestionMark
            }
            const uint8x8_t result = vmovn_u16(chunk); // narrowing move->packing
            vst1_u8(dst, result); // store
            dst += 8;
        }
        length = length % 8;
    }
#endif
#if defined(__mips_dsp)
    static_assert(sizeof(qsizetype) == sizeof(int),
                  "oops, the assembler implementation needs to be called in a loop");
    qt_toLatin1_mips_dsp_asm(dst, src, length);
#else
    while (length--) {
        if (Checked)
            *dst++ = (*src>0xff) ? '?' : (uchar) *src;
        else
            *dst++ = *src;
        ++src;
    }
#endif
}

void qt_to_latin1(uchar *dst, const char16_t *src, qsizetype length)
{
    qt_to_latin1_internal<true>(dst, src, length);
}

void qt_to_latin1_unchecked(uchar *dst, const char16_t *src, qsizetype length)
{
    qt_to_latin1_internal<false>(dst, src, length);
}

// Unicode case-insensitive comparison (argument order matches QStringView)
Q_NEVER_INLINE static int ucstricmp(qsizetype alen, const char16_t *a, qsizetype blen, const char16_t *b)
{
    if (a == b)
        return qt_lencmp(alen, blen);

    char32_t alast = 0;
    char32_t blast = 0;
    qsizetype l = qMin(alen, blen);
    qsizetype i;
    for (i = 0; i < l; ++i) {
//         qDebug() << Qt::hex << alast << blast;
//         qDebug() << Qt::hex << "*a=" << *a << "alast=" << alast << "folded=" << foldCase (*a, alast);
//         qDebug() << Qt::hex << "*b=" << *b << "blast=" << blast << "folded=" << foldCase (*b, blast);
        int diff = foldCase(a[i], alast) - foldCase(b[i], blast);
        if ((diff))
            return diff;
    }
    if (i == alen) {
        if (i == blen)
            return 0;
        return -1;
    }
    return 1;
}

// Case-insensitive comparison between a QStringView and a QLatin1StringView
// (argument order matches those types)
Q_NEVER_INLINE static int ucstricmp(qsizetype alen, const char16_t *a, qsizetype blen, const char *b)
{
    qsizetype l = qMin(alen, blen);
    qsizetype i;
    for (i = 0; i < l; ++i) {
        int diff = foldCase(a[i]) - foldCase(char16_t{uchar(b[i])});
        if ((diff))
            return diff;
    }
    if (i == alen) {
        if (i == blen)
            return 0;
        return -1;
    }
    return 1;
}

// Case-insensitive comparison between a Unicode string and a UTF-8 string
Q_NEVER_INLINE static int ucstricmp8(const char *utf8, const char *utf8end, const QChar *utf16, const QChar *utf16end)
{
    auto src1 = reinterpret_cast<const qchar8_t *>(utf8);
    auto end1 = reinterpret_cast<const qchar8_t *>(utf8end);
    QStringIterator src2(utf16, utf16end);

    while (src1 < end1 && src2.hasNext()) {
        char32_t uc1 = QChar::toCaseFolded(QUtf8Functions::nextUcs4FromUtf8(src1, end1));
        char32_t uc2 = QChar::toCaseFolded(src2.next());
        int diff = uc1 - uc2;   // can't underflow
        if (diff)
            return diff;
    }

    // the shorter string sorts first
    return (end1 > src1) - int(src2.hasNext());
}

#if defined(__mips_dsp)
// From qstring_mips_dsp_asm.S
extern "C" int qt_ucstrncmp_mips_dsp_asm(const char16_t *a,
                                         const char16_t *b,
                                         unsigned len);
#endif

// Unicode case-sensitive compare two same-sized strings
template <StringComparisonMode Mode>
static int ucstrncmp(const char16_t *a, const char16_t *b, size_t l)
{
    // This function isn't memcmp() because that can return the wrong sorting
    // result in little-endian architectures: 0x00ff must sort before 0x0100,
    // but the bytes in memory are FF 00 and 00 01.

#ifndef __OPTIMIZE_SIZE__
#  if defined(__mips_dsp)
    static_assert(sizeof(uint) == sizeof(size_t));
    if (l >= 8) {
        return qt_ucstrncmp_mips_dsp_asm(a, b, l);
    }
#  elif defined(__SSE2__)
    return ucstrncmp_sse2<Mode>(a, b, l);
#  elif defined(__ARM_NEON__)
    if (l >= 8) {
        const char16_t *end = a + l;
        const uint16x8_t mask = qvsetq_n_u16( 1, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7 );
        while (end - a > 7) {
            uint16x8_t da = vld1q_u16(reinterpret_cast<const uint16_t *>(a));
            uint16x8_t db = vld1q_u16(reinterpret_cast<const uint16_t *>(b));

            uint8_t r = ~(uint8_t)vaddvq_u16(vandq_u16(vceqq_u16(da, db), mask));
            if (r) {
                // found a different QChar
                if (Mode == CompareStringsForEquality)
                    return 1;
                uint idx = qCountTrailingZeroBits(r);
                return a[idx] - b[idx];
            }
            a += 8;
            b += 8;
        }
        l &= 7;
    }
    const auto lambda = [=](size_t i) -> int {
        return a[i] - b[i];
    };
    return UnrollTailLoop<7>::exec(l, 0, lambda, lambda);
#  endif // MIPS DSP or __SSE2__ or __ARM_NEON__
#endif // __OPTIMIZE_SIZE__

    if (Mode == CompareStringsForEquality || QSysInfo::ByteOrder == QSysInfo::BigEndian)
        return memcmp(a, b, l * sizeof(char16_t));

    for (size_t i = 0; i < l; ++i) {
        if (int diff = a[i] - b[i])
            return diff;
    }
    return 0;
}

template <StringComparisonMode Mode>
static int ucstrncmp(const char16_t *a, const char *b, size_t l)
{
    const uchar *c = reinterpret_cast<const uchar *>(b);
    const char16_t *uc = a;
    const char16_t *e = uc + l;

#if defined(__SSE2__) && !defined(__OPTIMIZE_SIZE__)
    return ucstrncmp_sse2<Mode>(uc, c, l);
#endif

    while (uc < e) {
        int diff = *uc - *c;
        if (diff)
            return diff;
        uc++, c++;
    }

    return 0;
}

// Unicode case-sensitive equality
template <typename Char2>
static bool ucstreq(const char16_t *a, size_t alen, const Char2 *b)
{
    return ucstrncmp<CompareStringsForEquality>(a, b, alen) == 0;
}

// Unicode case-sensitive comparison
template <typename Char2>
static int ucstrcmp(const char16_t *a, size_t alen, const Char2 *b, size_t blen)
{
    const size_t l = qMin(alen, blen);
    int cmp = ucstrncmp<CompareStringsForOrdering>(a, b, l);
    return cmp ? cmp : qt_lencmp(alen, blen);
}

using CaseInsensitiveL1 = QtPrivate::QCaseInsensitiveLatin1Hash;

static int latin1nicmp(const char *lhsChar, qsizetype lSize, const char *rhsChar, qsizetype rSize)
{
    // We're called with QLatin1StringView's .data() and .size():
    Q_ASSERT(lSize >= 0 && rSize >= 0);
    if (!lSize)
        return rSize ? -1 : 0;
    if (!rSize)
        return 1;
    const qsizetype size = std::min(lSize, rSize);

    Q_ASSERT(lhsChar && rhsChar); // since both lSize and rSize are positive
    for (qsizetype i = 0; i < size; i++) {
        if (int res = CaseInsensitiveL1::difference(lhsChar[i], rhsChar[i]))
            return res;
    }
    return qt_lencmp(lSize, rSize);
}

bool QtPrivate::equalStrings(QStringView lhs, QStringView rhs) noexcept
{
    Q_ASSERT(lhs.size() == rhs.size());
    return ucstreq(lhs.utf16(), lhs.size(), rhs.utf16());
}

bool QtPrivate::equalStrings(QStringView lhs, QLatin1StringView rhs) noexcept
{
    Q_ASSERT(lhs.size() == rhs.size());
    return ucstreq(lhs.utf16(), lhs.size(), rhs.latin1());
}

bool QtPrivate::equalStrings(QLatin1StringView lhs, QStringView rhs) noexcept
{
    return QtPrivate::equalStrings(rhs, lhs);
}

bool QtPrivate::equalStrings(QLatin1StringView lhs, QLatin1StringView rhs) noexcept
{
    Q_ASSERT(lhs.size() == rhs.size());
    return (!lhs.size() || memcmp(lhs.data(), rhs.data(), lhs.size()) == 0);
}

bool QtPrivate::equalStrings(QBasicUtf8StringView<false> lhs, QStringView rhs) noexcept
{
    return QUtf8::compareUtf8(lhs, rhs) == 0;
}

bool QtPrivate::equalStrings(QStringView lhs, QBasicUtf8StringView<false> rhs) noexcept
{
    return QtPrivate::equalStrings(rhs, lhs);
}

bool QtPrivate::equalStrings(QLatin1StringView lhs, QBasicUtf8StringView<false> rhs) noexcept
{
    return QUtf8::compareUtf8(QByteArrayView(rhs), lhs) == 0;
}

bool QtPrivate::equalStrings(QBasicUtf8StringView<false> lhs, QLatin1StringView rhs) noexcept
{
    return QtPrivate::equalStrings(rhs, lhs);
}

bool QtPrivate::equalStrings(QBasicUtf8StringView<false> lhs, QBasicUtf8StringView<false> rhs) noexcept
{
#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0) || defined(QT_BOOTSTRAPPED) || defined(QT_STATIC)
    Q_ASSERT(lhs.size() == rhs.size());
#else
    // operator== didn't enforce size prior to Qt 6.2
    if (lhs.size() != rhs.size())
        return false;
#endif
    return (!lhs.size() || memcmp(lhs.data(), rhs.data(), lhs.size()) == 0);
}

bool QAnyStringView::equal(QAnyStringView lhs, QAnyStringView rhs) noexcept
{
    if (lhs.size() != rhs.size() && lhs.isUtf8() == rhs.isUtf8())
        return false;
    return lhs.visit([rhs](auto lhs) {
        return rhs.visit([lhs](auto rhs) {
            return QtPrivate::equalStrings(lhs, rhs);
        });
    });
}

/*!
    \relates QStringView
    \internal
    \since 5.10

    Returns an integer that compares to 0 as \a lhs compares to \a rhs.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {comparison}

    Case-sensitive comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would expect.
    Consider sorting user-visible strings with QString::localeAwareCompare().

    \sa {Comparing Strings}
*/
int QtPrivate::compareStrings(QStringView lhs, QStringView rhs, Qt::CaseSensitivity cs) noexcept
{
    if (cs == Qt::CaseSensitive)
        return ucstrcmp(lhs.utf16(), lhs.size(), rhs.utf16(), rhs.size());
    return ucstricmp(lhs.size(), lhs.utf16(), rhs.size(), rhs.utf16());
}

/*!
    \relates QStringView
    \internal
    \since 5.10
    \overload

    Returns an integer that compares to 0 as \a lhs compares to \a rhs.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {comparison}

    Case-sensitive comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would expect.
    Consider sorting user-visible strings with QString::localeAwareCompare().

    \sa {Comparing Strings}
*/
int QtPrivate::compareStrings(QStringView lhs, QLatin1StringView rhs, Qt::CaseSensitivity cs) noexcept
{
    if (cs == Qt::CaseSensitive)
        return ucstrcmp(lhs.utf16(), lhs.size(), rhs.latin1(), rhs.size());
    return ucstricmp(lhs.size(), lhs.utf16(), rhs.size(), rhs.latin1());
}

/*!
    \relates QStringView
    \internal
    \since 6.0
    \overload
*/
int QtPrivate::compareStrings(QStringView lhs, QBasicUtf8StringView<false> rhs, Qt::CaseSensitivity cs) noexcept
{
    return -compareStrings(rhs, lhs, cs);
}

/*!
    \relates QStringView
    \internal
    \since 5.10
    \overload
*/
int QtPrivate::compareStrings(QLatin1StringView lhs, QStringView rhs, Qt::CaseSensitivity cs) noexcept
{
    return -compareStrings(rhs, lhs, cs);
}

/*!
    \relates QStringView
    \internal
    \since 5.10
    \overload

    Returns an integer that compares to 0 as \a lhs compares to \a rhs.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {comparison}

    Case-sensitive comparison is based exclusively on the numeric Latin-1 values
    of the characters and is very fast, but is not what a human would expect.
    Consider sorting user-visible strings with QString::localeAwareCompare().

    \sa {Comparing Strings}
*/
int QtPrivate::compareStrings(QLatin1StringView lhs, QLatin1StringView rhs, Qt::CaseSensitivity cs) noexcept
{
    if (lhs.isEmpty())
        return qt_lencmp(qsizetype(0), rhs.size());
    if (rhs.isEmpty())
        return qt_lencmp(lhs.size(), qsizetype(0));
    if (cs == Qt::CaseInsensitive)
        return latin1nicmp(lhs.data(), lhs.size(), rhs.data(), rhs.size());
    const auto l = std::min(lhs.size(), rhs.size());
    int r = memcmp(lhs.data(), rhs.data(), l);
    return r ? r : qt_lencmp(lhs.size(), rhs.size());
}

/*!
    \relates QStringView
    \internal
    \since 6.0
    \overload
*/
int QtPrivate::compareStrings(QLatin1StringView lhs, QBasicUtf8StringView<false> rhs, Qt::CaseSensitivity cs) noexcept
{
    return -QUtf8::compareUtf8(QByteArrayView(rhs), lhs, cs);
}

/*!
    \relates QStringView
    \internal
    \since 6.0
    \overload
*/
int QtPrivate::compareStrings(QBasicUtf8StringView<false> lhs, QStringView rhs, Qt::CaseSensitivity cs) noexcept
{
    if (cs == Qt::CaseSensitive)
        return QUtf8::compareUtf8(lhs, rhs);
    return ucstricmp8(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

/*!
    \relates QStringView
    \internal
    \since 6.0
    \overload
*/
int QtPrivate::compareStrings(QBasicUtf8StringView<false> lhs, QLatin1StringView rhs, Qt::CaseSensitivity cs) noexcept
{
    return -compareStrings(rhs, lhs, cs);
}

/*!
    \relates QStringView
    \internal
    \since 6.0
    \overload
*/
int QtPrivate::compareStrings(QBasicUtf8StringView<false> lhs, QBasicUtf8StringView<false> rhs, Qt::CaseSensitivity cs) noexcept
{
    return QUtf8::compareUtf8(QByteArrayView(lhs), QByteArrayView(rhs), cs);
}

int QAnyStringView::compare(QAnyStringView lhs, QAnyStringView rhs, Qt::CaseSensitivity cs) noexcept
{
    return lhs.visit([rhs, cs](auto lhs) {
        return rhs.visit([lhs, cs](auto rhs) {
            return QtPrivate::compareStrings(lhs, rhs, cs);
        });
    });
}

// ### Qt 7: do not allow anything but ASCII digits
// in arg()'s replacements.
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) && !defined(QT_BOOTSTRAPPED)
static bool supportUnicodeDigitValuesInArg()
{
    static const bool result = []() {
        static const char supportUnicodeDigitValuesEnvVar[]
                = "QT_USE_UNICODE_DIGIT_VALUES_IN_STRING_ARG";

        if (qEnvironmentVariableIsSet(supportUnicodeDigitValuesEnvVar))
            return qEnvironmentVariableIntValue(supportUnicodeDigitValuesEnvVar) != 0;

#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0) // keep it in sync with the test
        return true;
#else
        return false;
#endif
    }();

    return result;
}
#endif

static int qArgDigitValue(QChar ch) noexcept
{
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) && !defined(QT_BOOTSTRAPPED)
    if (supportUnicodeDigitValuesInArg())
        return ch.digitValue();
#endif
    if (ch >= u'0' && ch <= u'9')
        return int(ch.unicode() - u'0');
    return -1;
}

#if QT_CONFIG(regularexpression)
Q_DECL_COLD_FUNCTION
static void qtWarnAboutInvalidRegularExpression(const QRegularExpression &re, const char *cls, const char *method)
{
    extern void qtWarnAboutInvalidRegularExpression(const QString &pattern, const char *cls, const char *method);
    qtWarnAboutInvalidRegularExpression(re.pattern(), cls, method);
}
#endif

/*!
  \macro QT_RESTRICTED_CAST_FROM_ASCII
  \relates QString

  Disables most automatic conversions from source literals and 8-bit data
  to unicode QStrings, but allows the use of
  the \c{QChar(char)} and \c{QString(const char (&ch)[N]} constructors,
  and the \c{QString::operator=(const char (&ch)[N])} assignment operator.
  This gives most of the type-safety benefits of \l QT_NO_CAST_FROM_ASCII
  but does not require user code to wrap character and string literals
  with QLatin1Char, QLatin1StringView or similar.

  Using this macro together with source strings outside the 7-bit range,
  non-literals, or literals with embedded NUL characters is undefined.

  \sa QT_NO_CAST_FROM_ASCII, QT_NO_CAST_TO_ASCII
*/

/*!
  \macro QT_NO_CAST_FROM_ASCII
  \relates QString
  \relates QChar

  Disables automatic conversions from 8-bit strings (\c{char *}) to Unicode
  QStrings, as well as from 8-bit \c{char} types (\c{char} and
  \c{unsigned char}) to QChar.

  \sa QT_NO_CAST_TO_ASCII, QT_RESTRICTED_CAST_FROM_ASCII,
      QT_NO_CAST_FROM_BYTEARRAY
*/

/*!
  \macro QT_NO_CAST_TO_ASCII
  \relates QString

  Disables automatic conversion from QString to 8-bit strings (\c{char *}).

  \sa QT_NO_CAST_FROM_ASCII, QT_RESTRICTED_CAST_FROM_ASCII,
      QT_NO_CAST_FROM_BYTEARRAY
*/

/*!
  \macro QT_ASCII_CAST_WARNINGS
  \internal
  \relates QString

  This macro can be defined to force a warning whenever a function is
  called that automatically converts between unicode and 8-bit encodings.

  Note: This only works for compilers that support warnings for
  deprecated API.

  \sa QT_NO_CAST_TO_ASCII, QT_NO_CAST_FROM_ASCII, QT_RESTRICTED_CAST_FROM_ASCII
*/

/*!
    \class QString
    \inmodule QtCore
    \reentrant

    \brief The QString class provides a Unicode character string.

    \ingroup tools
    \ingroup shared
    \ingroup string-processing

    \compares strong
    \compareswith strong QChar QLatin1StringView {const char16_t *} \
                  QStringView QUtf8StringView
    \endcompareswith
    \compareswith strong QByteArray QByteArrayView {const char *}
    When comparing with byte arrays, their content is interpreted as UTF-8.
    \endcompareswith

    QString stores a string of 16-bit \l{QChar}s, where each QChar
    corresponds to one UTF-16 code unit. (Unicode characters
    with code values above 65535 are stored using surrogate pairs,
    that is, two consecutive \l{QChar}s.)

    \l{Unicode} is an international standard that supports most of the
    writing systems in use today. It is a superset of US-ASCII (ANSI
    X3.4-1986) and Latin-1 (ISO 8859-1), and all the US-ASCII/Latin-1
    characters are available at the same code positions.

    Behind the scenes, QString uses \l{implicit sharing}
    (copy-on-write) to reduce memory usage and to avoid the needless
    copying of data. This also helps reduce the inherent overhead of
    storing 16-bit characters instead of 8-bit characters.

    In addition to QString, Qt also provides the QByteArray class to
    store raw bytes and traditional 8-bit '\\0'-terminated strings.
    For most purposes, QString is the class you want to use. It is
    used throughout the Qt API, and the Unicode support ensures that
    your applications are easy to translate if you want to expand
    your application's market at some point. Two prominent cases
    where QByteArray is appropriate are when you need to store raw
    binary data, and when memory conservation is critical (like in
    embedded systems).

    \section1 Initializing a string

    One way to initialize a QString is to pass a \c{const char
    *} to its constructor. For example, the following code creates a
    QString of size 5 containing the data "Hello":

    \snippet qstring/main.cpp 0

    QString converts the \c{const char *} data into Unicode using the
    fromUtf8() function.

    In all of the QString functions that take \c{const char *}
    parameters, the \c{const char *} is interpreted as a classic
    C-style \c{'\\0'}-terminated string. Except where the function's
    name overtly indicates some other encoding, such \c{const char *}
    parameters are assumed to be encoded in UTF-8.

    You can also provide string data as an array of \l{QChar}s:

    \snippet qstring/main.cpp 1

    QString makes a deep copy of the QChar data, so you can modify it
    later without experiencing side effects. You can avoid taking a
    deep copy of the character data by using QStringView or
    QString::fromRawData() instead.

    Another approach is to set the size of the string using resize()
    and to initialize the data character per character. QString uses
    0-based indexes, just like C++ arrays. To access the character at
    a particular index position, you can use \l operator[](). On
    non-\c{const} strings, \l operator[]() returns a reference to a
    character that can be used on the left side of an assignment. For
    example:

    \snippet qstring/main.cpp 2

    For read-only access, an alternative syntax is to use the at()
    function:

    \snippet qstring/main.cpp 3

    The at() function can be faster than \l operator[]() because it
    never causes a \l{deep copy} to occur. Alternatively, use the
    first(), last(), or sliced() functions to extract several characters
    at a time.

    A QString can embed '\\0' characters (QChar::Null). The size()
    function always returns the size of the whole string, including
    embedded '\\0' characters.

    After a call to the resize() function, newly allocated characters
    have undefined values. To set all the characters in the string to
    a particular value, use the fill() function.

    QString provides dozens of overloads designed to simplify string
    usage. For example, if you want to compare a QString with a string
    literal, you can write code like this and it will work as expected:

    \snippet qstring/main.cpp 4

    You can also pass string literals to functions that take QStrings
    as arguments, invoking the QString(const char *)
    constructor. Similarly, you can pass a QString to a function that
    takes a \c{const char *} argument using the \l qPrintable() macro,
    which returns the given QString as a \c{const char *}. This is
    equivalent to calling toLocal8Bit().\l{QByteArray::}{constData()}
    on the QString.

    \section1 Manipulating string data

    QString provides the following basic functions for modifying the
    character data: append(), prepend(), insert(), replace(), and
    remove(). For example:

    \snippet qstring/main.cpp 5

    In the above example, the replace() function's first two arguments are the
    position from which to start replacing and the number of characters that
    should be replaced.

    When data-modifying functions increase the size of the string,
    QString may reallocate the memory in which it holds its data. When
    this happens, QString expands by more than it immediately needs so as
    to have space for further expansion without reallocation until the size
    of the string has significantly increased.

    The insert(), remove(), and, when replacing a sub-string with one of
    different size, replace() functions can be slow (\l{linear time}) for
    large strings because they require moving many characters in the string
    by at least one position in memory.

    If you are building a QString gradually and know in advance
    approximately how many characters the QString will contain, you
    can call reserve(), asking QString to preallocate a certain amount
    of memory. You can also call capacity() to find out how much
    memory the QString actually has allocated.

    QString provides \l{STL-style iterators} (QString::const_iterator and
    QString::iterator). In practice, iterators are handy when working with
    generic algorithms provided by the C++ standard library.

    \note Iterators over a QString, and references to individual characters
    within one, cannot be relied on to remain valid when any non-\c{const}
    method of the QString is called. Accessing such an iterator or reference
    after the call to a non-\c{const} method leads to undefined behavior. When
    stability for iterator-like functionality is required, you should use
    indexes instead of iterators, as they are not tied to QString's internal
    state and thus do not get invalidated.

    \note Due to \l{implicit sharing}, the first non-\c{const} operator or
    function used on a given QString may cause it to internally perform a deep
    copy of its data. This invalidates all iterators over the string and
    references to individual characters within it. Do not call non-const
    functions while keeping iterators. Accessing an iterator or reference
    after it has been invalidated leads to undefined behavior. See the
    \l{Implicit sharing iterator problem} section for more information.

    A frequent requirement is to remove or simplify the spacing between
    visible characters in a string. The characters that make up that spacing
    are those for which \l {QChar::}{isSpace()} returns \c true, such as
    the simple space \c{' '}, the horizontal tab \c{'\\t'} and the newline \c{'\\n'}.
    To obtain a copy of a string leaving out any spacing from its start and end,
    use \l trimmed(). To also replace each sequence of spacing characters within
    the string with a simple space, \c{' '}, use \l simplified().

    If you want to find all occurrences of a particular character or
    substring in a QString, use the indexOf() or lastIndexOf()
    functions.The former searches forward, the latter searches backward.
    Either can be told an index position from which to start their search.
    Each returns the index position of the character or substring if they
    find it; otherwise, they return -1.  For example, here is a typical loop
    that finds all occurrences of a particular substring:

    \snippet qstring/main.cpp 6

    QString provides many functions for converting numbers into
    strings and strings into numbers. See the arg() functions, the
    setNum() functions, the number() static functions, and the
    toInt(), toDouble(), and similar functions.

    To get an uppercase or lowercase version of a string, use toUpper() or
    toLower().

    Lists of strings are handled by the QStringList class. You can
    split a string into a list of strings using the split() function,
    and join a list of strings into a single string with an optional
    separator using QStringList::join(). You can obtain a filtered list
    from a string list by selecting the entries in it that contain a
    particular substring or match a particular QRegularExpression.
    See QStringList::filter() for details.

    \section1 Querying string data

    To see if a QString starts or ends with a particular substring, use
    startsWith() or endsWith(). To check whether a QString contains a
    specific character or substring, use the contains() function. To
    find out how many times a particular character or substring occurs
    in a string, use count().

    To obtain a pointer to the actual character data, call data() or
    constData(). These functions return a pointer to the beginning of
    the QChar data. The pointer is guaranteed to remain valid until a
    non-\c{const} function is called on the QString.

    \section2 Comparing strings

    QStrings can be compared using overloaded operators such as \l
    operator<(), \l operator<=(), \l operator==(), \l operator>=(),
    and so on. The comparison is based exclusively on the lexicographical
    order of the two strings, seen as sequences of UTF-16 code units.
    It is very fast but is not what a human would expect; the
    QString::localeAwareCompare() function is usually a better choice for
    sorting user-interface strings, when such a comparison is available.

    When Qt is linked with the ICU library (which it usually is), its
    locale-aware sorting is used. Otherwise, platform-specific solutions
    are used:
    \list
        \li On Windows, localeAwareCompare() uses the current user locale,
            as set in the \uicontrol{regional} and \uicontrol{language}
            options portion of \uicontrol{Control Panel}.
        \li On \macos and iOS, \l localeAwareCompare() compares according
            to the \uicontrol{Order for sorted lists} setting in the
            \uicontrol{International preferences} panel.
        \li On other Unix-like systems, the comparison falls back to the
            system library's \c strcoll().
    \endlist

    \section1 Converting between encoded string data and QString

    QString provides the following functions that return a
    \c{const char *} version of the string as QByteArray: toUtf8(),
    toLatin1(), and toLocal8Bit().

    \list
    \li toLatin1() returns a Latin-1 (ISO 8859-1) encoded 8-bit string.
    \li toUtf8() returns a UTF-8 encoded 8-bit string. UTF-8 is a
       superset of US-ASCII (ANSI X3.4-1986) that supports the entire
       Unicode character set through multibyte sequences.
    \li toLocal8Bit() returns an 8-bit string using the system's local
       encoding. This is the same as toUtf8() on Unix systems.
    \endlist

    To convert from one of these encodings, QString provides
    fromLatin1(), fromUtf8(), and fromLocal8Bit(). Other
    encodings are supported through the QStringEncoder and QStringDecoder
    classes.

    As mentioned above, QString provides a lot of functions and
    operators that make it easy to interoperate with \c{const char *}
    strings. But this functionality is a double-edged sword: It makes
    QString more convenient to use if all strings are US-ASCII or
    Latin-1, but there is always the risk that an implicit conversion
    from or to \c{const char *} is done using the wrong 8-bit
    encoding. To minimize these risks, you can turn off these implicit
    conversions by defining some of the following preprocessor symbols:

    \list
    \li \l QT_NO_CAST_FROM_ASCII disables automatic conversions from
       C string literals and pointers to Unicode.
    \li \l QT_RESTRICTED_CAST_FROM_ASCII allows automatic conversions
       from C characters and character arrays but disables automatic
       conversions from character pointers to Unicode.
    \li \l QT_NO_CAST_TO_ASCII disables automatic conversion from QString
       to C strings.
    \endlist

    You then need to explicitly call fromUtf8(), fromLatin1(),
    or fromLocal8Bit() to construct a QString from an
    8-bit string, or use the lightweight QLatin1StringView class. For
    example:

    \snippet code/src_corelib_text_qstring.cpp 1

    Similarly, you must call toLatin1(), toUtf8(), or
    toLocal8Bit() explicitly to convert the QString to an 8-bit
    string.

    \table 100 %
    \header
    \li Note for C Programmers

    \row
    \li
    Due to C++'s type system and the fact that QString is
    \l{implicitly shared}, QStrings may be treated like \c{int}s or
    other basic types. For example:

    \snippet qstring/main.cpp 7

    The \c result variable is a normal variable allocated on the
    stack. When \c return is called, and because we're returning by
    value, the copy constructor is called and a copy of the string is
    returned. No actual copying takes place thanks to the implicit
    sharing.

    \endtable

    \section1 Distinction between null and empty strings

    For historical reasons, QString distinguishes between null
    and empty strings. A \e null string is a string that is
    initialized using QString's default constructor or by passing
    \nullptr to the constructor. An \e empty string is any
    string with size 0. A null string is always empty, but an empty
    string isn't necessarily null:

    \snippet qstring/main.cpp 8

    All functions except isNull() treat null strings the same as empty
    strings. For example, toUtf8().\l{QByteArray::}{constData()} returns a valid pointer
    (not \nullptr) to a '\\0' character for a null string. We
    recommend that you always use the isEmpty() function and avoid isNull().

    \section1 Number formats

    When a QString::arg() \c{'%'} format specifier includes the \c{'L'} locale
    qualifier, and the base is ten (its default), the default locale is
    used. This can be set using \l{QLocale::setDefault()}. For more refined
    control of localized string representations of numbers, see
    QLocale::toString(). All other number formatting done by QString follows the
    C locale's representation of numbers.

    When QString::arg() applies left-padding to numbers, the fill character
    \c{'0'} is treated specially. If the number is negative, its minus sign
    appears before the zero-padding. If the field is localized, the
    locale-appropriate zero character is used in place of \c{'0'}. For
    floating-point numbers, this special treatment only applies if the number is
    finite.

    \section2 Floating-point formats

    In member functions (for example, arg() and number()) that format floating-point
    numbers (\c float or \c double) as strings, the representation used can be
    controlled by a choice of \e format and \e precision, whose meanings are as
    for \l {QLocale::toString(double, char, int)}.

    If the selected \e format includes an exponent, localized forms follow the
    locale's convention on digits in the exponent. For non-localized formatting,
    the exponent shows its sign and includes at least two digits, left-padding
    with zero if needed.

    \section1 More efficient string construction

    Many strings are known at compile time. The QString constructor from
    C++ string literals will copy the contents of the string,
    treating the contents as UTF-8. This requires memory allocation and
    re-encoding string data, operations that will happen at runtime.
    If the string data is known at compile time, you can use the QStringLiteral
    macro or similarly \c{operator""_s} to create QString's payload at compile
    time instead.

    Using the QString \c{'+'} operator, it is easy to construct a
    complex string from multiple substrings. You will often write code
    like this:

    \snippet qstring/stringbuilder.cpp 0

    There is nothing wrong with either of these string constructions,
    but there are a few hidden inefficiencies:

    First, repeated use of the \c{'+'} operator may lead to
    multiple memory allocations. When concatenating \e{n} substrings,
    where \e{n > 2}, there can be as many as \e{n - 1} calls to the
    memory allocator.

    These allocations can be optimized by an internal class
    \c{QStringBuilder}. This class is marked
    internal and does not appear in the documentation, because you
    aren't meant to instantiate it in your code. Its use will be
    automatic, as described below.

    \c{QStringBuilder} uses expression templates and reimplements the
    \c{'%'} operator so that when you use \c{'%'} for string
    concatenation instead of \c{'+'}, multiple substring
    concatenations will be postponed until the final result is about
    to be assigned to a QString. At this point, the amount of memory
    required for the final result is known. The memory allocator is
    then called \e{once} to get the required space, and the substrings
    are copied into it one by one.

    Additional efficiency is gained by inlining and reducing reference
    counting (the QString created from a \c{QStringBuilder}
    has a ref count of 1, whereas QString::append() needs an extra
    test).

    There are two ways you can access this improved method of string
    construction. The straightforward way is to include
    \c{QStringBuilder} wherever you want to use it and use the
    \c{'%'} operator instead of \c{'+'} when concatenating strings:

    \snippet qstring/stringbuilder.cpp 5

    A more global approach, which is more convenient but not entirely
    source-compatible, is to define \c QT_USE_QSTRINGBUILDER (by adding
    it to the compiler flags) at build time. This will make concatenating
    strings with \c{'+'} work the same way as \c{QStringBuilder's} \c{'%'}.

    \note Using automatic type deduction (for example, by using the \c
    auto keyword) with the result of string concatenation when QStringBuilder
    is enabled will show that the concatenation is indeed an object of a
    QStringBuilder specialization:

    \snippet qstring/stringbuilder.cpp 6

    This does not cause any harm, as QStringBuilder will implicitly convert to
    QString when required. If this is undesirable, then one should specify
    the necessary types instead of having the compiler deduce them:

    \snippet qstring/stringbuilder.cpp 7

    \section1 Maximum size and out-of-memory conditions

    The maximum size of QString depends on the architecture. Most 64-bit
    systems can allocate more than 2 GB of memory, with a typical limit
    of 2^63 bytes. The actual value also depends on the overhead required for
    managing the data block. As a result, you can expect a maximum size
    of 2 GB minus overhead on 32-bit platforms and 2^63 bytes minus overhead
    on 64-bit platforms. The number of elements that can be stored in a
    QString is this maximum size divided by the size of QChar.

    When memory allocation fails, QString throws a \c std::bad_alloc
    exception if the application was compiled with exception support.
    Out-of-memory conditions in Qt containers are the only cases where Qt
    will throw exceptions. If exceptions are disabled, then running out of
    memory is undefined behavior.

    \note Target operating systems may impose limits on how much memory an
    application can allocate, in total, or on the size of individual allocations.
    This may further restrict the size of string a QString can hold.
    Mitigating or controlling the behavior these limits cause is beyond the
    scope of the Qt API.

    \sa {Which string class to use?}, fromRawData(), QChar, QStringView,
        QLatin1StringView, QByteArray
*/

/*! \typedef QString::ConstIterator

    Qt-style synonym for QString::const_iterator.
*/

/*! \typedef QString::Iterator

    Qt-style synonym for QString::iterator.
*/

/*! \typedef QString::const_iterator

    \sa QString::iterator
*/

/*! \typedef QString::iterator

    \sa QString::const_iterator
*/

/*! \typedef QString::const_reverse_iterator
    \since 5.6

    \sa QString::reverse_iterator, QString::const_iterator
*/

/*! \typedef QString::reverse_iterator
    \since 5.6

    \sa QString::const_reverse_iterator, QString::iterator
*/

/*!
    \typedef QString::size_type
*/

/*!
    \typedef QString::difference_type
*/

/*!
    \typedef QString::const_reference
*/
/*!
    \typedef QString::reference
*/

/*!
    \typedef QString::const_pointer

    The QString::const_pointer typedef provides an STL-style
    const pointer to a QString element (QChar).
*/
/*!
    \typedef QString::pointer

    The QString::pointer typedef provides an STL-style
    pointer to a QString element (QChar).
*/

/*!
    \typedef QString::value_type
*/

/*! \fn QString::iterator QString::begin()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the
    first character in the string.

//! [iterator-invalidation-func-desc]
    \warning The returned iterator is invalidated on detachment or when the
    QString is modified.
//! [iterator-invalidation-func-desc]

    \sa constBegin(), end()
*/

/*! \fn QString::const_iterator QString::begin() const

    \overload begin()
*/

/*! \fn QString::const_iterator QString::cbegin() const
    \since 5.0

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the
    first character in the string.

    \include qstring.cpp iterator-invalidation-func-desc

    \sa begin(), cend()
*/

/*! \fn QString::const_iterator QString::constBegin() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the
    first character in the string.

    \include qstring.cpp iterator-invalidation-func-desc

    \sa begin(), constEnd()
*/

/*! \fn QString::iterator QString::end()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing just after
    the last character in the string.

    \include qstring.cpp iterator-invalidation-func-desc

    \sa begin(), constEnd()
*/

/*! \fn QString::const_iterator QString::end() const

    \overload end()
*/

/*! \fn QString::const_iterator QString::cend() const
    \since 5.0

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing just
    after the last character in the string.

    \include qstring.cpp iterator-invalidation-func-desc

    \sa cbegin(), end()
*/

/*! \fn QString::const_iterator QString::constEnd() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing just
    after the last character in the string.

    \include qstring.cpp iterator-invalidation-func-desc

    \sa constBegin(), end()
*/

/*! \fn QString::reverse_iterator QString::rbegin()
    \since 5.6

    Returns a \l{STL-style iterators}{STL-style} reverse iterator pointing to
    the first character in the string, in reverse order.

    \include qstring.cpp iterator-invalidation-func-desc

    \sa begin(), crbegin(), rend()
*/

/*! \fn QString::const_reverse_iterator QString::rbegin() const
    \since 5.6
    \overload
*/

/*! \fn QString::const_reverse_iterator QString::crbegin() const
    \since 5.6

    Returns a const \l{STL-style iterators}{STL-style} reverse iterator
    pointing to the first character in the string, in reverse order.

    \include qstring.cpp iterator-invalidation-func-desc

    \sa begin(), rbegin(), rend()
*/

/*! \fn QString::reverse_iterator QString::rend()
    \since 5.6

    Returns a \l{STL-style iterators}{STL-style} reverse iterator pointing just
    after the last character in the string, in reverse order.

    \include qstring.cpp iterator-invalidation-func-desc

    \sa end(), crend(), rbegin()
*/

/*! \fn QString::const_reverse_iterator QString::rend() const
    \since 5.6
    \overload
*/

/*! \fn QString::const_reverse_iterator QString::crend() const
    \since 5.6

    Returns a const \l{STL-style iterators}{STL-style} reverse iterator
    pointing just after the last character in the string, in reverse order.

    \include qstring.cpp iterator-invalidation-func-desc

    \sa end(), rend(), rbegin()
*/

/*!
    \fn QString::QString()

    Constructs a null string. Null strings are also considered empty.

    \sa isEmpty(), isNull(), {Distinction Between Null and Empty Strings}
*/

/*!
    \fn QString::QString(QString &&other)

    Move-constructs a QString instance, making it point at the same
    object that \a other was pointing to.

    \since 5.2
*/

/*! \fn QString::QString(const char *str)

    Constructs a string initialized with the 8-bit string \a str. The
    given const char pointer is converted to Unicode using the
    fromUtf8() function.

    You can disable this constructor by defining
    \l QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.

    \note Defining \l QT_RESTRICTED_CAST_FROM_ASCII also disables
    this constructor, but enables a \c{QString(const char (&ch)[N])}
    constructor instead. Using non-literal input, or input with
    embedded NUL characters, or non-7-bit characters is undefined
    in this case.

    \sa fromLatin1(), fromLocal8Bit(), fromUtf8()
*/

/*! \fn QString::QString(const char8_t *str)

    Constructs a string initialized with the UTF-8 string \a str. The
    given const char8_t pointer is converted to Unicode using the
    fromUtf8() function.

    \since 6.1
    \sa fromLatin1(), fromLocal8Bit(), fromUtf8()
*/

/*!
    \fn QString::QString(QStringView sv)

    Constructs a string initialized with the string view's data.

    The QString will be null if and only if \a sv is null.

    \since 6.8

    \sa fromUtf16()
*/

/*
//! [from-std-string]
Returns a copy of the \a str string. The given string is assumed to be
encoded in \1, and is converted to QString using the \2 function.
//! [from-std-string]
*/

/*! \fn QString QString::fromStdString(const std::string &str)

    \include qstring.cpp {from-std-string} {UTF-8} {fromUtf8()}

    \sa fromLatin1(), fromLocal8Bit(), fromUtf8(), QByteArray::fromStdString()
*/

/*! \fn QString QString::fromStdWString(const std::wstring &str)

    Returns a copy of the \a str string. The given string is assumed
    to be encoded in utf16 if the size of wchar_t is 2 bytes (e.g. on
    windows) and ucs4 if the size of wchar_t is 4 bytes (most Unix
    systems).

    \sa fromUtf16(), fromLatin1(), fromLocal8Bit(), fromUtf8(), fromUcs4(),
        fromStdU16String(), fromStdU32String()
*/

/*! \fn QString QString::fromWCharArray(const wchar_t *string, qsizetype size)
    \since 4.2

    Reads the first \a size code units of the \c wchar_t array to whose start
    \a string points, converting them to Unicode and returning the result as
    a QString. The encoding used by \c wchar_t is assumed to be UTF-32 if the
    type's size is four bytes or UTF-16 if its size is two bytes.

    If \a size is -1 (default), the \a string must be '\\0'-terminated.

    \sa fromUtf16(), fromLatin1(), fromLocal8Bit(), fromUtf8(), fromUcs4(),
        fromStdWString()
*/

/*! \fn std::wstring QString::toStdWString() const

    Returns a std::wstring object with the data contained in this
    QString. The std::wstring is encoded in UTF-16 on platforms where
    wchar_t is 2 bytes wide (for example, Windows) and in UTF-32 on platforms
    where wchar_t is 4 bytes wide (most Unix systems).

    This method is mostly useful to pass a QString to a function
    that accepts a std::wstring object.

    \sa utf16(), toLatin1(), toUtf8(), toLocal8Bit(), toStdU16String(),
        toStdU32String()
*/

qsizetype QString::toUcs4_helper(const char16_t *uc, qsizetype length, char32_t *out)
{
    qsizetype count = 0;

    QStringIterator i(QStringView(uc, length));
    while (i.hasNext())
        out[count++] = i.next();

    return count;
}

/*! \fn qsizetype QString::toWCharArray(wchar_t *array) const
  \since 4.2

  Fills the \a array with the data contained in this QString object.
  The array is encoded in UTF-16 on platforms where
  wchar_t is 2 bytes wide (e.g. windows) and in UTF-32 on platforms
  where wchar_t is 4 bytes wide (most Unix systems).

  \a array has to be allocated by the caller and contain enough space to
  hold the complete string (allocating the array with the same length as the
  string is always sufficient).

  This function returns the actual length of the string in \a array.

  \note This function does not append a null character to the array.

  \sa utf16(), toUcs4(), toLatin1(), toUtf8(), toLocal8Bit(), toStdWString(),
      QStringView::toWCharArray()
*/

/*! \fn QString::QString(const QString &other)

    Constructs a copy of \a other.

    This operation takes \l{constant time}, because QString is
    \l{implicitly shared}. This makes returning a QString from a
    function very fast. If a shared instance is modified, it will be
    copied (copy-on-write), and that takes \l{linear time}.

    \sa operator=()
*/

/*!
    Constructs a string initialized with the first \a size characters
    of the QChar array \a unicode.

    If \a unicode is 0, a null string is constructed.

    If \a size is negative, \a unicode is assumed to point to a '\\0'-terminated
    array and its length is determined dynamically. The terminating
    null character is not considered part of the string.

    QString makes a deep copy of the string data. The unicode data is copied as
    is and the Byte Order Mark is preserved if present.

    \sa fromRawData()
*/
QString::QString(const QChar *unicode, qsizetype size)
{
    if (!unicode) {
        d.clear();
    } else {
        if (size < 0)
            size = QtPrivate::qustrlen(reinterpret_cast<const char16_t *>(unicode));
        if (!size) {
            d = DataPointer::fromRawData(&_empty, 0);
        } else {
            d = DataPointer(size, size);
            Q_CHECK_PTR(d.data());
            memcpy(d.data(), unicode, size * sizeof(QChar));
            d.data()[size] = '\0';
        }
    }
}

/*!
    Constructs a string of the given \a size with every character set
    to \a ch.

    \sa fill()
*/
QString::QString(qsizetype size, QChar ch)
{
    if (size <= 0) {
        d = DataPointer::fromRawData(&_empty, 0);
    } else {
        d = DataPointer(size, size);
        Q_CHECK_PTR(d.data());
        d.data()[size] = '\0';
        char16_t *b = d.data();
        char16_t *e = d.data() + size;
        const char16_t value = ch.unicode();
        std::fill(b, e, value);
    }
}

/*! \fn QString::QString(qsizetype size, Qt::Initialization)
  \internal

  Constructs a string of the given \a size without initializing the
  characters. This is only used in \c QStringBuilder::toString().
*/
QString::QString(qsizetype size, Qt::Initialization)
{
    if (size <= 0) {
        d = DataPointer::fromRawData(&_empty, 0);
    } else {
        d = DataPointer(size, size);
        Q_CHECK_PTR(d.data());
        d.data()[size] = '\0';
    }
}

/*! \fn QString::QString(QLatin1StringView str)

    Constructs a copy of the Latin-1 string viewed by \a str.

    \sa fromLatin1()
*/

/*!
    Constructs a string of size 1 containing the character \a ch.
*/
QString::QString(QChar ch)
{
    d = DataPointer(1, 1);
    Q_CHECK_PTR(d.data());
    d.data()[0] = ch.unicode();
    d.data()[1] = '\0';
}

/*! \fn QString::QString(const QByteArray &ba)

    Constructs a string initialized with the byte array \a ba. The
    given byte array is converted to Unicode using fromUtf8().

    You can disable this constructor by defining
    \l QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.

    \note Any null ('\\0') bytes in the byte array will be included in this
    string, converted to Unicode null characters (U+0000). This behavior is
    different from Qt 5.x.

    \sa fromLatin1(), fromLocal8Bit(), fromUtf8()
*/

/*! \fn QString::QString(const Null &)
    \internal
*/

/*! \fn QString::QString(QStringPrivate)
    \internal
*/

/*! \fn QString &QString::operator=(const QString::Null &)
    \internal
*/

/*!
  \fn QString::~QString()

    Destroys the string.
*/


/*! \fn void QString::swap(QString &other)
    \since 4.8
    \memberswap{string}
*/

/*! \fn void QString::detach()

    \internal
*/

/*! \fn bool QString::isDetached() const

    \internal
*/

/*! \fn bool QString::isSharedWith(const QString &other) const

    \internal
*/

/*! \fn QString::operator std::u16string_view() const
    \since 6.7

    Converts this QString object to a \c{std::u16string_view} object.
*/

static bool needsReallocate(const QString &str, qsizetype newSize)
{
    const auto capacityAtEnd = str.capacity() - str.data_ptr().freeSpaceAtBegin();
    return newSize > capacityAtEnd;
}

/*!
    Sets the size of the string to \a size characters.

    If \a size is greater than the current size, the string is
    extended to make it \a size characters long with the extra
    characters added to the end. The new characters are uninitialized.

    If \a size is less than the current size, characters beyond position
    \a size are excluded from the string.

    \note While resize() will grow the capacity if needed, it never shrinks
    capacity. To shed excess capacity, use squeeze().

    Example:

    \snippet qstring/main.cpp 45

    If you want to append a certain number of identical characters to
    the string, use the \l {QString::}{resize(qsizetype, QChar)} overload.

    If you want to expand the string so that it reaches a certain
    width and fill the new positions with a particular character, use
    the leftJustified() function:

    If \a size is negative, it is equivalent to passing zero.

    \snippet qstring/main.cpp 47

    \sa truncate(), reserve(), squeeze()
*/

void QString::resize(qsizetype size)
{
    if (size < 0)
        size = 0;

    if (d->needsDetach() || needsReallocate(*this, size))
        reallocData(size, QArrayData::Grow);
    d.size = size;
    if (d->allocatedCapacity())
        d.data()[size] = u'\0';
}

/*!
    \overload
    \since 5.7

    Unlike \l {QString::}{resize(qsizetype)}, this overload
    initializes the new characters to \a fillChar:

    \snippet qstring/main.cpp 46
*/

void QString::resize(qsizetype newSize, QChar fillChar)
{
    const qsizetype oldSize = size();
    resize(newSize);
    const qsizetype difference = size() - oldSize;
    if (difference > 0)
        std::fill_n(d.data() + oldSize, difference, fillChar.unicode());
}


/*!
    \since 6.8

    Sets the size of the string to \a size characters. If the size of
    the string grows, the new characters are uninitialized.

    The behavior is identical to \c{resize(size)}.

    \sa resize()
*/

void QString::resizeForOverwrite(qsizetype size)
{
    resize(size);
}


/*! \fn qsizetype QString::capacity() const

    Returns the maximum number of characters that can be stored in
    the string without forcing a reallocation.

    The sole purpose of this function is to provide a means of fine
    tuning QString's memory usage. In general, you will rarely ever
    need to call this function. If you want to know how many
    characters are in the string, call size().

    \note a statically allocated string will report a capacity of 0,
    even if it's not empty.

    \note The free space position in the allocated memory block is undefined. In
    other words, one should not assume that the free memory is always located
    after the initialized elements.

    \sa reserve(), squeeze()
*/

/*!
    \fn void QString::reserve(qsizetype size)

    Ensures the string has space for at least \a size characters.

    If you know in advance how large a string will be, you can call this
    function to save repeated reallocation while building it.
    This can improve performance when building a string incrementally.
    A long sequence of operations that add to a string may trigger several
    reallocations, the last of which may leave you with significantly more
    space than you need. This is less efficient than doing a single
    allocation of the right size at the start.

    If in doubt about how much space shall be needed, it is usually better to
    use an upper bound as \a size, or a high estimate of the most likely size,
    if a strict upper bound would be much bigger than this. If \a size is an
    underestimate, the string will grow as needed once the reserved size is
    exceeded, which may lead to a larger allocation than your best
    overestimate would have and will slow the operation that triggers it.

    \warning reserve() reserves memory but does not change the size of the
    string. Accessing data beyond the end of the string is undefined behavior.
    If you need to access memory beyond the current end of the string,
    use resize().

    This function is useful for code that needs to build up a long
    string and wants to avoid repeated reallocation. In this example,
    we want to add to the string until some condition is \c true, and
    we're fairly sure that size is large enough to make a call to
    reserve() worthwhile:

    \snippet qstring/main.cpp 44

    \sa squeeze(), capacity(), resize()
*/

/*!
    \fn void QString::squeeze()

    Releases any memory not required to store the character data.

    The sole purpose of this function is to provide a means of fine
    tuning QString's memory usage. In general, you will rarely ever
    need to call this function.

    \sa reserve(), capacity()
*/

void QString::reallocData(qsizetype alloc, QArrayData::AllocationOption option)
{
    if (!alloc) {
        d = DataPointer::fromRawData(&_empty, 0);
        return;
    }

    // don't use reallocate path when reducing capacity and there's free space
    // at the beginning: might shift data pointer outside of allocated space
    const bool cannotUseReallocate = d.freeSpaceAtBegin() > 0;

    if (d->needsDetach() || cannotUseReallocate) {
        DataPointer dd(alloc, qMin(alloc, d.size), option);
        Q_CHECK_PTR(dd.data());
        if (dd.size > 0)
            ::memcpy(dd.data(), d.data(), dd.size * sizeof(QChar));
        dd.data()[dd.size] = 0;
        d.swap(dd);
    } else {
        d->reallocate(alloc, option);
    }
}

void QString::reallocGrowData(qsizetype n)
{
    if (!n)  // expected to always allocate
        n = 1;

    if (d->needsDetach()) {
        DataPointer dd(DataPointer::allocateGrow(d, n, QArrayData::GrowsAtEnd));
        Q_CHECK_PTR(dd.data());
        dd->copyAppend(d.data(), d.data() + d.size);
        dd.data()[dd.size] = 0;
        d.swap(dd);
    } else {
        d->reallocate(d.constAllocatedCapacity() + n, QArrayData::Grow);
    }
}

/*! \fn void QString::clear()

    Clears the contents of the string and makes it null.

    \sa resize(), isNull()
*/

/*! \fn QString &QString::operator=(const QString &other)

    Assigns \a other to this string and returns a reference to this
    string.
*/

QString &QString::operator=(const QString &other) noexcept
{
    d = other.d;
    return *this;
}

/*!
    \fn QString &QString::operator=(QString &&other)

    Move-assigns \a other to this QString instance.

    \since 5.2
*/

/*! \fn QString &QString::operator=(QLatin1StringView str)

    \overload operator=()

    Assigns the Latin-1 string viewed by \a str to this string.
*/
QString &QString::operator=(QLatin1StringView other)
{
    const qsizetype capacityAtEnd = capacity() - d.freeSpaceAtBegin();
    if (isDetached() && other.size() <= capacityAtEnd) { // assumes d->alloc == 0 -> !isDetached() (sharedNull)
        d.size = other.size();
        d.data()[other.size()] = 0;
        qt_from_latin1(d.data(), other.latin1(), other.size());
    } else {
        *this = fromLatin1(other.latin1(), other.size());
    }
    return *this;
}

/*! \fn QString &QString::operator=(const QByteArray &ba)

    \overload operator=()

    Assigns \a ba to this string. The byte array is converted to Unicode
    using the fromUtf8() function.

    You can disable this operator by defining
    \l QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn QString &QString::operator=(const char *str)

    \overload operator=()

    Assigns \a str to this string. The const char pointer is converted
    to Unicode using the fromUtf8() function.

    You can disable this operator by defining \l QT_NO_CAST_FROM_ASCII
    or \l QT_RESTRICTED_CAST_FROM_ASCII when you compile your applications.
    This can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*!
    \overload operator=()

    Sets the string to contain the single character \a ch.
*/
QString &QString::operator=(QChar ch)
{
    return assign(1, ch);
}

/*!
     \fn QString& QString::insert(qsizetype position, const QString &str)

    Inserts the string \a str at the given index \a position and
    returns a reference to this string.

    Example:

    \snippet qstring/main.cpp 26

//! [string-grow-at-insertion]
    This string grows to accommodate the insertion. If \a position is beyond
    the end of the string, space characters are appended to the string to reach
    this \a position, followed by \a str.
//! [string-grow-at-insertion]

    \sa append(), prepend(), replace(), remove()
*/

/*!
    \fn QString& QString::insert(qsizetype position, QStringView str)
    \since 6.0
    \overload insert()

    Inserts the string view \a str at the given index \a position and
    returns a reference to this string.

    \include qstring.cpp string-grow-at-insertion
*/


/*!
    \fn QString& QString::insert(qsizetype position, const char *str)
    \since 5.5
    \overload insert()

    Inserts the C string \a str at the given index \a position and
    returns a reference to this string.

    \include qstring.cpp string-grow-at-insertion

    This function is not available when \l QT_NO_CAST_FROM_ASCII is
    defined.
*/

/*!
    \fn QString& QString::insert(qsizetype position, const QByteArray &str)
    \since 5.5
    \overload insert()

    Interprets the contents of \a str as UTF-8, inserts the Unicode string
    it encodes at the given index \a position and returns a reference to
    this string.

    \include qstring.cpp string-grow-at-insertion

    This function is not available when \l QT_NO_CAST_FROM_ASCII is
    defined.
*/

/*! \internal
    T is a view or a container on/of QChar, char16_t, or char
*/
template <typename T>
static void insert_helper(QString &str, qsizetype i, const T &toInsert)
{
    auto &str_d = str.data_ptr();
    qsizetype difference = 0;
    if (Q_UNLIKELY(i > str_d.size))
        difference = i - str_d.size;
    const qsizetype oldSize = str_d.size;
    const qsizetype insert_size = toInsert.size();
    const qsizetype newSize = str_d.size + difference + insert_size;
    const auto side = i == 0 ? QArrayData::GrowsAtBeginning : QArrayData::GrowsAtEnd;

    if (str_d.needsDetach() || needsReallocate(str, newSize)) {
        const auto cbegin = str.cbegin();
        const auto cend = str.cend();
        const auto insert_start = difference == 0 ? std::next(cbegin, i) : cend;
        QString other;
        // Using detachAndGrow() so that prepend optimization works and QStringBuilder
        // unittests pass
        other.data_ptr().detachAndGrow(side, newSize, nullptr, nullptr);
        other.append(QStringView(cbegin, insert_start));
        other.resize(i, u' ');
        other.append(toInsert);
        other.append(QStringView(insert_start, cend));
        str.swap(other);
        return;
    }

    str_d.detachAndGrow(side, difference + insert_size, nullptr, nullptr);
    Q_CHECK_PTR(str_d.data());
    str.resize(newSize);

    auto begin = str_d.begin();
    auto old_end = std::next(begin, oldSize);
    std::fill_n(old_end, difference, u' ');
    auto insert_start = std::next(begin, i);
    if (difference == 0)
        std::move_backward(insert_start, old_end, str_d.end());

    using Char = std::remove_cv_t<typename T::value_type>;
    if constexpr(std::is_same_v<Char, QChar>)
        std::copy_n(reinterpret_cast<const char16_t *>(toInsert.data()), insert_size, insert_start);
    else if constexpr (std::is_same_v<Char, char16_t>)
        std::copy_n(toInsert.data(), insert_size, insert_start);
    else if constexpr (std::is_same_v<Char, char>)
        qt_from_latin1(insert_start, toInsert.data(), insert_size);
}

/*!
    \fn QString &QString::insert(qsizetype position, QLatin1StringView str)
    \overload insert()

    Inserts the Latin-1 string viewed by \a str at the given index \a position.

    \include qstring.cpp string-grow-at-insertion
*/
QString &QString::insert(qsizetype i, QLatin1StringView str)
{
    const char *s = str.latin1();
    if (i < 0 || !s || !(*s))
        return *this;

    insert_helper(*this, i, str);
    return *this;
}

/*!
    \fn QString &QString::insert(qsizetype position, QUtf8StringView str)
    \overload insert()
    \since 6.5

    Inserts the UTF-8 string view \a str at the given index \a position.

    \note Inserting variable-width UTF-8-encoded string data is conceptually slower
    than inserting fixed-width string data such as UTF-16 (QStringView) or Latin-1
    (QLatin1StringView) and should thus be used sparingly.

    \include qstring.cpp string-grow-at-insertion
*/
QString &QString::insert(qsizetype i, QUtf8StringView s)
{
    auto insert_size = s.size();
    if (i < 0 || insert_size <= 0)
        return *this;

    qsizetype difference = 0;
    if (Q_UNLIKELY(i > d.size))
        difference = i - d.size;

    const qsizetype newSize = d.size + difference + insert_size;

    if (d.needsDetach() || needsReallocate(*this, newSize)) {
        const auto cbegin = this->cbegin();
        const auto insert_start = difference == 0 ? std::next(cbegin, i) : cend();
        QString other;
        other.reserve(newSize);
        other.append(QStringView(cbegin, insert_start));
        if (difference > 0)
            other.resize(i, u' ');
        other.append(s);
        other.append(QStringView(insert_start, cend()));
        swap(other);
        return *this;
    }

    if (i >= d.size) {
        d.detachAndGrow(QArrayData::GrowsAtEnd, difference + insert_size, nullptr, nullptr);
        Q_CHECK_PTR(d.data());

        if (difference > 0)
            resize(i, u' ');
        append(s);
    } else {
        // Optimal insertion of Utf8 data is at the end, anywhere else could
        // potentially lead to moving characters twice if Utf8 data size
        // (variable-width) is less than the equivalent Utf16 data size
        QVarLengthArray<char16_t> buffer(insert_size); // ### optimize (QTBUG-108546)
        char16_t *b = QUtf8::convertToUnicode(buffer.data(), s);
        insert_helper(*this, i, QStringView(buffer.data(), b));
    }

    return *this;
}

/*!
    \fn QString& QString::insert(qsizetype position, const QChar *unicode, qsizetype size)
    \overload insert()

    Inserts the first \a size characters of the QChar array \a unicode
    at the given index \a position in the string.

    This string grows to accommodate the insertion. If \a position is beyond
    the end of the string, space characters are appended to the string to reach
    this \a position, followed by \a size characters of the QChar array
    \a unicode.
*/
QString& QString::insert(qsizetype i, const QChar *unicode, qsizetype size)
{
    if (i < 0 || size <= 0)
        return *this;

    // In case when data points into "this"
    if (!d->needsDetach() && QtPrivate::q_points_into_range(unicode, *this)) {
        QVarLengthArray copy(unicode, unicode + size);
        insert(i, copy.data(), size);
    } else {
        insert_helper(*this, i, QStringView(unicode, size));
    }

    return *this;
}

/*!
    \fn QString& QString::insert(qsizetype position, QChar ch)
    \overload insert()

    Inserts \a ch at the given index \a position in the string.

    This string grows to accommodate the insertion. If \a position is beyond
    the end of the string, space characters are appended to the string to reach
    this \a position, followed by \a ch.
*/

QString& QString::insert(qsizetype i, QChar ch)
{
    if (i < 0)
        i += d.size;
    return insert(i, &ch, 1);
}

/*!
    Appends the string \a str onto the end of this string.

    Example:

    \snippet qstring/main.cpp 9

    This is the same as using the insert() function:

    \snippet qstring/main.cpp 10

    The append() function is typically very fast (\l{constant time}),
    because QString preallocates extra space at the end of the string
    data so it can grow without reallocating the entire string each
    time.

    \sa operator+=(), prepend(), insert()
*/
QString &QString::append(const QString &str)
{
    if (!str.isNull()) {
        if (isNull()) {
            if (Q_UNLIKELY(!str.d.isMutable()))
                assign(str); // fromRawData, so we do a deep copy
            else
                operator=(str);
        } else if (str.size()) {
            append(str.constData(), str.size());
        }
    }
    return *this;
}

/*!
    \fn QString &QString::append(QStringView v)
    \overload append()
    \since 6.0

    Appends the given string view \a v to this string and returns the result.
*/

/*!
  \overload append()
  \since 5.0

  Appends \a len characters from the QChar array \a str to this string.
*/
QString &QString::append(const QChar *str, qsizetype len)
{
    if (str && len > 0) {
        static_assert(sizeof(QChar) == sizeof(char16_t), "Unexpected difference in sizes");
        // the following should be safe as QChar uses char16_t as underlying data
        const char16_t *char16String = reinterpret_cast<const char16_t *>(str);
        d->growAppend(char16String, char16String + len);
        d.data()[d.size] = u'\0';
    }
    return *this;
}

/*!
  \overload append()

  Appends the Latin-1 string viewed by \a str to this string.
*/
QString &QString::append(QLatin1StringView str)
{
    append_helper(*this, str);
    return *this;
}

/*!
  \overload append()
  \since 6.5

  Appends the UTF-8 string view \a str to this string.
*/
QString &QString::append(QUtf8StringView str)
{
    append_helper(*this, str);
    return *this;
}

/*! \fn QString &QString::append(const QByteArray &ba)

    \overload append()

    Appends the byte array \a ba to this string. The given byte array
    is converted to Unicode using the fromUtf8() function.

    You can disable this function by defining \l QT_NO_CAST_FROM_ASCII
    when you compile your applications. This can be useful if you want
    to ensure that all user-visible strings go through QObject::tr(),
    for example.
*/

/*! \fn QString &QString::append(const char *str)

    \overload append()

    Appends the string \a str to this string. The given const char
    pointer is converted to Unicode using the fromUtf8() function.

    You can disable this function by defining \l QT_NO_CAST_FROM_ASCII
    when you compile your applications. This can be useful if you want
    to ensure that all user-visible strings go through QObject::tr(),
    for example.
*/

/*!
    \overload append()

    Appends the character \a ch to this string.
*/
QString &QString::append(QChar ch)
{
    d.detachAndGrow(QArrayData::GrowsAtEnd, 1, nullptr, nullptr);
    d->copyAppend(1, ch.unicode());
    d.data()[d.size] = '\0';
    return *this;
}

/*! \fn QString &QString::prepend(const QString &str)

    Prepends the string \a str to the beginning of this string and
    returns a reference to this string.

    This operation is typically very fast (\l{constant time}), because
    QString preallocates extra space at the beginning of the string data,
    so it can grow without reallocating the entire string each time.

    Example:

    \snippet qstring/main.cpp 36

    \sa append(), insert()
*/

/*! \fn QString &QString::prepend(QLatin1StringView str)

    \overload prepend()

    Prepends the Latin-1 string viewed by \a str to this string.
*/

/*! \fn QString &QString::prepend(QUtf8StringView str)
    \since 6.5
    \overload prepend()

    Prepends the UTF-8 string view \a str to this string.
*/

/*! \fn QString &QString::prepend(const QChar *str, qsizetype len)
    \since 5.5
    \overload prepend()

    Prepends \a len characters from the QChar array \a str to this string and
    returns a reference to this string.
*/

/*! \fn QString &QString::prepend(QStringView str)
    \since 6.0
    \overload prepend()

    Prepends the string view \a str to the beginning of this string and
    returns a reference to this string.
*/

/*! \fn QString &QString::prepend(const QByteArray &ba)

    \overload prepend()

    Prepends the byte array \a ba to this string. The byte array is
    converted to Unicode using the fromUtf8() function.

    You can disable this function by defining
    \l QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn QString &QString::prepend(const char *str)

    \overload prepend()

    Prepends the string \a str to this string. The const char pointer
    is converted to Unicode using the fromUtf8() function.

    You can disable this function by defining
    \l QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn QString &QString::prepend(QChar ch)

    \overload prepend()

    Prepends the character \a ch to this string.
*/

/*!
    \fn QString &QString::assign(QAnyStringView v)
    \since 6.6

    Replaces the contents of this string with a copy of \a v and returns a
    reference to this string.

    The size of this string will be equal to the size of \a v, converted to
    UTF-16 as if by \c{v.toString()}. Unlike QAnyStringView::toString(), however,
    this function only allocates memory if the estimated size exceeds the capacity
    of this string or this string is shared.

    \sa QAnyStringView::toString()
*/

/*!
    \fn QString &QString::assign(qsizetype n, QChar c)
    \since 6.6

    Replaces the contents of this string with \a n copies of \a c and
    returns a reference to this string.

    The size of this string will be equal to \a n, which has to be non-negative.

    This function will only allocate memory if \a n exceeds the capacity of this
    string or this string is shared.

    \sa fill()
*/

/*!
    \fn template <typename InputIterator, QString::if_compatible_iterator<InputIterator>> QString &QString::assign(InputIterator first, InputIterator last)
    \since 6.6

    Replaces the contents of this string with a copy of the elements in the
    iterator range [\a first, \a last) and returns a reference to this string.

    The size of this string will be equal to the decoded length of the elements
    in the range [\a first, \a last), which need not be the same as the length of
    the range itself, because this function transparently recodes the input
    character set to UTF-16.

    This function will only allocate memory if the number of elements in the
    range, or, for non-UTF-16-encoded input, the maximum possible size of the
    resulting string, exceeds the capacity of this string, or if this string is
    shared.

    \note The behavior is undefined if either argument is an iterator into *this or
    [\a first, \a last) is not a valid range.

    \constraints
    \c InputIterator meets the requirements of a
    \l {https://en.cppreference.com/w/cpp/named_req/InputIterator} {LegacyInputIterator}
    and the \c{value_type} of \c InputIterator is one of the following character types:
    \list
    \li QChar
    \li QLatin1Char
    \li \c {char}
    \li \c {unsigned char}
    \li \c {signed char}
    \li \c {char8_t}
    \li \c char16_t
    \li (on platforms, such as Windows, where it is a 16-bit type) \c wchar_t
    \li \c char32_t
    \endlist
*/

QString &QString::assign(QAnyStringView s)
{
    if (s.size() <= capacity() && isDetached()) {
        const auto offset = d.freeSpaceAtBegin();
        if (offset)
            d.setBegin(d.begin() - offset);
        resize(0);
        s.visit([this](auto input) {
            this->append(input);
        });
    } else {
        *this = s.toString();
    }
    return *this;
}

#ifndef QT_BOOTSTRAPPED
QString &QString::assign_helper(const char32_t *data, qsizetype len)
{
    // worst case: each char32_t requires a surrogate pair, so
    const auto requiredCapacity = len * 2;
    if (requiredCapacity <= capacity() && isDetached()) {
        const auto offset = d.freeSpaceAtBegin();
        if (offset)
            d.setBegin(d.begin() - offset);
        auto begin = reinterpret_cast<QChar *>(d.begin());
        auto ba = QByteArrayView(reinterpret_cast<const std::byte*>(data), len * sizeof(char32_t));
        QStringConverter::State state;
        const auto end = QUtf32::convertToUnicode(begin, ba, &state, DetectEndianness);
        d.size = end - begin;
        d.data()[d.size] = u'\0';
    } else {
        *this = QString::fromUcs4(data, len);
    }
    return *this;
}
#endif

/*!
  \fn QString &QString::remove(qsizetype position, qsizetype n)

  Removes \a n characters from the string, starting at the given \a
  position index, and returns a reference to the string.

  If the specified \a position index is within the string, but \a
  position + \a n is beyond the end of the string, the string is
  truncated at the specified \a position.

  If \a n is <= 0 nothing is changed.

  \snippet qstring/main.cpp 37

//! [shrinking-erase]
  Element removal will preserve the string's capacity and not reduce the
  amount of allocated memory. To shed extra capacity and free as much memory
  as possible, call squeeze() after the last change to the string's size.
//! [shrinking-erase]

  \sa insert(), replace()
*/
QString &QString::remove(qsizetype pos, qsizetype len)
{
    if (pos < 0)  // count from end of string
        pos += size();

    if (size_t(pos) >= size_t(size()) || len <= 0)
        return *this;

    len = std::min(len, size() - pos);

    if (!d->isShared()) {
        d->erase(d.begin() + pos, len);
        d.data()[d.size] = u'\0';
    } else {
        // TODO: either reserve "size()", which is bigger than needed, or
        // modify the shrinking-erase docs of this method (since the size
        // of "copy" won't have any extra capacity any more)
        const qsizetype sz = size() - len;
        QString copy{sz, Qt::Uninitialized};
        auto begin = d.begin();
        auto toRemove_start = d.begin() + pos;
        copy.d->copyRanges({{begin, toRemove_start},
                           {toRemove_start + len, d.end()}});
        swap(copy);
    }
    return *this;
}

template<typename T>
static void removeStringImpl(QString &s, const T &needle, Qt::CaseSensitivity cs)
{
    const auto needleSize = needle.size();
    if (!needleSize)
        return;

    // avoid detach if nothing to do:
    qsizetype i = s.indexOf(needle, 0, cs);
    if (i < 0)
        return;

    QString::DataPointer &dptr = s.data_ptr();
    auto begin = dptr.begin();
    auto end = dptr.end();

    auto copyFunc = [&](auto &dst) {
        auto src = begin + i + needleSize;
        while (src < end) {
            i = s.indexOf(needle, std::distance(begin, src), cs);
            auto hit = i == -1 ? end : begin + i;
            dst = std::copy(src, hit, dst);
            src = hit + needleSize;
        }
        return dst;
    };

    if (!dptr->needsDetach()) {
        auto dst = begin + i;
        dst = copyFunc(dst);
        s.truncate(std::distance(begin, dst));
    } else {
        QString copy{s.size(), Qt::Uninitialized};
        auto copy_begin = copy.begin();
        auto dst = std::copy(begin, begin + i, copy_begin); // Chunk before the first hit
        dst = copyFunc(dst);
        copy.resize(std::distance(copy_begin, dst));
        s.swap(copy);
    }
}

/*!
  Removes every occurrence of the given \a str string in this
  string, and returns a reference to this string.

  \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

  This is the same as \c replace(str, "", cs).

  \include qstring.cpp shrinking-erase

  \sa replace()
*/
QString &QString::remove(const QString &str, Qt::CaseSensitivity cs)
{
    const auto s = str.d.data();
    if (QtPrivate::q_points_into_range(s, d))
        removeStringImpl(*this, QStringView{QVarLengthArray(s, s + str.size())}, cs);
    else
        removeStringImpl(*this, qToStringViewIgnoringNull(str), cs);
    return *this;
}

/*!
  \since 5.11
  \overload

  Removes every occurrence of the given Latin-1 string viewed by \a str
  from this string, and returns a reference to this string.

  \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

  This is the same as \c replace(str, "", cs).

  \include qstring.cpp shrinking-erase

  \sa replace()
*/
QString &QString::remove(QLatin1StringView str, Qt::CaseSensitivity cs)
{
    removeStringImpl(*this, str, cs);
    return *this;
}

/*!
  \fn QString &QString::removeAt(qsizetype pos)

  \since 6.5

  Removes the character at index \a pos. If \a pos is out of bounds
  (i.e. \a pos >= size()), this function does nothing.

  \sa remove()
*/

/*!
  \fn QString &QString::removeFirst()

  \since 6.5

  Removes the first character in this string. If the string is empty,
  this function does nothing.

  \sa remove()
*/

/*!
  \fn QString &QString::removeLast()

  \since 6.5

  Removes the last character in this string. If the string is empty,
  this function does nothing.

  \sa remove()
*/

/*!
  Removes every occurrence of the character \a ch in this string, and
  returns a reference to this string.

  \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

  Example:

  \snippet qstring/main.cpp 38

  This is the same as \c replace(ch, "", cs).

  \include qstring.cpp shrinking-erase

  \sa replace()
*/
QString &QString::remove(QChar ch, Qt::CaseSensitivity cs)
{
    const qsizetype idx = indexOf(ch, 0, cs);
    if (idx == -1)
        return *this;

    const bool isCase = cs == Qt::CaseSensitive;
    ch = isCase ? ch : ch.toCaseFolded();
    auto match = [ch, isCase](QChar x) {
        return ch == (isCase ? x : x.toCaseFolded());
    };


    auto begin = d.begin();
    auto first_match = begin + idx;
    auto end = d.end();
    if (!d->isShared()) {
        auto it = std::remove_if(first_match, end, match);
        d->erase(it, std::distance(it, end));
        d.data()[d.size] = u'\0';
    } else {
        // Instead of detaching, create a new string and copy all characters except for
        // the ones we're removing
        // TODO: size() is more than the needed since "copy" would be shorter
        QString copy{size(), Qt::Uninitialized};
        auto dst = copy.d.begin();
        auto it = std::copy(begin, first_match, dst); // Chunk before idx
        it = std::remove_copy_if(first_match + 1, end, it, match);
        copy.d.size = std::distance(dst, it);
        copy.d.data()[copy.d.size] = u'\0';
        *this = std::move(copy);
    }
    return *this;
}

/*!
  \fn QString &QString::remove(const QRegularExpression &re)
  \since 5.0

  Removes every occurrence of the regular expression \a re in the
  string, and returns a reference to the string. For example:

  \snippet qstring/main.cpp 96

  \include qstring.cpp shrinking-erase

  \sa indexOf(), lastIndexOf(), replace()
*/

/*!
  \fn template <typename Predicate> QString &QString::removeIf(Predicate pred)
  \since 6.1

  Removes all elements for which the predicate \a pred returns true
  from the string. Returns a reference to the string.

  \sa remove()
*/


/*! \internal
  Instead of detaching, or reallocating if "before" is shorter than "after"
  and there isn't enough capacity, create a new string, copy characters to it
  as needed, then swap it with "str".
*/
static void replace_with_copy(QString &str, QSpan<size_t> indices, qsizetype blen,
                              QStringView after)
{
    const qsizetype alen = after.size();
    const char16_t *after_b = after.utf16();

    const QString::DataPointer &str_d = str.data_ptr();
    auto src_start = str_d.begin();
    const qsizetype newSize = str_d.size + indices.size() * (alen - blen);
    QString copy{ newSize, Qt::Uninitialized };
    QString::DataPointer &copy_d = copy.data_ptr();
    auto dst = copy_d.begin();
    for (size_t index : indices) {
        auto hit = str_d.begin() + index;
        dst = std::copy(src_start, hit, dst);
        dst = std::copy_n(after_b, alen, dst);
        src_start = hit + blen;
    }
    dst = std::copy(src_start, str_d.end(), dst);
    str.swap(copy);
}

// No detaching or reallocation is needed
static void replace_in_place(QString &str, QSpan<size_t> indices,
                             qsizetype blen, QStringView after)
{
    const qsizetype alen = after.size();
    const char16_t *after_b = after.utf16();
    const char16_t *after_e = after.utf16() + after.size();

    if (blen == alen) { // Replace in place
        for (size_t index : indices)
            std::copy_n(after_b, alen, str.data_ptr().begin() + index);
    } else if (blen > alen) { // Replace from front
        char16_t *begin = str.data_ptr().begin();
        char16_t *hit = begin + indices.front();
        char16_t *to = hit;
        to = std::copy_n(after_b, alen, to);
        char16_t *movestart = hit + blen;
        for (size_t index : indices.sliced(1)) {
            hit = begin + index;
            to = std::move(movestart, hit, to);
            to = std::copy_n(after_b, alen, to);
            movestart = hit + blen;
        }
        to = std::move(movestart, str.data_ptr().end(), to);
        str.resize(std::distance(begin, to));
    } else { // blen < alen, Replace from back
        const qsizetype oldSize = str.data_ptr().size;
        const qsizetype adjust = indices.size() * (alen - blen);
        const qsizetype newSize = oldSize + adjust;

        str.resize(newSize);
        char16_t *begin = str.data_ptr().begin();
        char16_t *moveend = begin + oldSize;
        char16_t *to = str.data_ptr().end();

        for (auto it = indices.rbegin(), end = indices.rend(); it != end; ++it) {
            char16_t *hit = begin + *it;
            char16_t *movestart = hit + blen;
            to = std::move_backward(movestart, moveend, to);
            to = std::copy_backward(after_b, after_e, to);
            moveend = hit;
        }
    }
}

static void replace_helper(QString &str, QSpan<size_t> indices, qsizetype blen, QStringView after)
{
    const qsizetype oldSize = str.data_ptr().size;
    const qsizetype adjust = indices.size() * (after.size() - blen);
    const qsizetype newSize = oldSize + adjust;
    if (str.data_ptr().needsDetach() || needsReallocate(str, newSize)) {
        replace_with_copy(str, indices, blen, after);
        return;
    }

    if (QtPrivate::q_points_into_range(after.begin(), str))
        // Copy after if it lies inside our own d.b area (which we could
        // possibly invalidate via a realloc or modify by replacement)
        replace_in_place(str, indices, blen, QVarLengthArray(after.begin(), after.end()));
    else
        replace_in_place(str, indices, blen, after);
}

/*!
  \fn QString &QString::replace(qsizetype position, qsizetype n, const QString &after)

  Replaces \a n characters beginning at index \a position with
  the string \a after and returns a reference to this string.

  \note If the specified \a position index is within the string,
  but \a position + \a n goes outside the strings range,
  then \a n will be adjusted to stop at the end of the string.

  Example:

  \snippet qstring/main.cpp 40

  \sa insert(), remove()
*/
QString &QString::replace(qsizetype pos, qsizetype len, const QString &after)
{
    return replace(pos, len, after.constData(), after.size());
}

/*!
  \fn QString &QString::replace(qsizetype position, qsizetype n, const QChar *after, qsizetype alen)
  \overload replace()
  Replaces \a n characters beginning at index \a position with the
  first \a alen characters of the QChar array \a after and returns a
  reference to this string.

  \a n must not be negative.
*/
QString &QString::replace(qsizetype pos, qsizetype len, const QChar *after, qsizetype alen)
{
    Q_PRE(len >= 0);

    if (size_t(pos) > size_t(this->size()))
        return *this;
    if (len > this->size() - pos)
        len = this->size() - pos;

    size_t index = pos;
    replace_helper(*this, QSpan(&index, 1), len, QStringView{after, alen});
    return *this;
}

/*!
  \fn QString &QString::replace(qsizetype position, qsizetype n, QChar after)
  \overload replace()

  Replaces \a n characters beginning at index \a position with the
  character \a after and returns a reference to this string.
*/
QString &QString::replace(qsizetype pos, qsizetype len, QChar after)
{
    return replace(pos, len, &after, 1);
}

/*!
  \overload replace()
  Replaces every occurrence of the string \a before with the string \a
  after and returns a reference to this string.

  \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

  Example:

  \snippet qstring/main.cpp 41

  \note The replacement text is not rescanned after it is inserted.

  Example:

  \snippet qstring/main.cpp 86

//! [empty-before-arg-in-replace]
  \note If you use an empty \a before argument, the \a after argument will be
  inserted \e {before and after} each character of the string.
//! [empty-before-arg-in-replace]

*/
QString &QString::replace(const QString &before, const QString &after, Qt::CaseSensitivity cs)
{
    return replace(before.constData(), before.size(), after.constData(), after.size(), cs);
}

/*!
  \since 4.5
  \overload replace()

  Replaces each occurrence in this string of the first \a blen
  characters of \a before with the first \a alen characters of \a
  after and returns a reference to this string.

  \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

  \note If \a before points to an \e empty string (that is, \a blen == 0),
  the string pointed to by \a after will be inserted \e {before and after}
  each character in this string.
*/
QString &QString::replace(const QChar *before, qsizetype blen,
                          const QChar *after, qsizetype alen,
                          Qt::CaseSensitivity cs)
{
    if (isEmpty()) {
        if (blen)
            return *this;
    } else {
        if (cs == Qt::CaseSensitive && before == after && blen == alen)
            return *this;
    }
    if (alen == 0 && blen == 0)
        return *this;
    if (alen == 1 && blen == 1)
        return replace(*before, *after, cs);

    QStringMatcher matcher(before, blen, cs);

    qsizetype index = 0;

    QVarLengthArray<size_t> indices;
    while ((index = matcher.indexIn(*this, index)) != -1) {
        indices.push_back(index);
        if (blen) // Step over before:
            index += blen;
        else // Only count one instance of empty between any two characters:
            index++;
    }
    if (indices.isEmpty())
        return *this;

    replace_helper(*this, indices, blen, QStringView{after, alen});
    return *this;
}

/*!
  \overload replace()
  Replaces every occurrence of the character \a ch in the string with
  \a after and returns a reference to this string.

  \include qstring.qdocinc {search-comparison-case-sensitivity} {search}
*/
QString& QString::replace(QChar ch, const QString &after, Qt::CaseSensitivity cs)
{
    if (after.size() == 0)
        return remove(ch, cs);

    if (after.size() == 1)
        return replace(ch, after.front(), cs);

    if (size() == 0)
        return *this;

    const char16_t cc = (cs == Qt::CaseSensitive ? ch.unicode() : ch.toCaseFolded().unicode());

    QVarLengthArray<size_t> indices;
    if (cs == Qt::CaseSensitive) {
        const char16_t *begin = d.begin();
        const char16_t *end = d.end();
        QStringView view(begin, end);
        const char16_t *hit = nullptr;
        while ((hit = QtPrivate::qustrchr(view, cc)) != end) {
            indices.push_back(std::distance(begin, hit));
            view = QStringView(std::next(hit), end);
        }
    } else {
        for (qsizetype i = 0; i < d.size; ++i)
            if (QChar::toCaseFolded(d.data()[i]) == cc)
                indices.push_back(i);
    }
    if (indices.isEmpty())
        return *this;

    replace_helper(*this, indices, 1, after);
    return *this;
}

/*!
  \overload replace()
  Replaces every occurrence of the character \a before with the
  character \a after and returns a reference to this string.

  \include qstring.qdocinc {search-comparison-case-sensitivity} {search}
*/
QString& QString::replace(QChar before, QChar after, Qt::CaseSensitivity cs)
{
    const qsizetype idx = indexOf(before, 0, cs);
    if (idx == -1)
        return *this;

    const char16_t achar = after.unicode();
    char16_t bchar = before.unicode();

    auto matchesCIS = [](char16_t beforeChar) {
        return [beforeChar](char16_t ch) { return foldAndCompare(ch, beforeChar); };
    };

    auto hit = d.begin() + idx;
    if (!d.needsDetach()) {
        *hit++ = achar;
        if (cs == Qt::CaseSensitive) {
            std::replace(hit, d.end(), bchar, achar);
        } else {
            bchar = foldCase(bchar);
            std::replace_if(hit, d.end(), matchesCIS(bchar), achar);
        }
    } else {
        QString other{ d.size, Qt::Uninitialized };
        auto dest = std::copy(d.begin(), hit, other.d.begin());
        *dest++ = achar;
        ++hit;
        if (cs == Qt::CaseSensitive) {
            std::replace_copy(hit, d.end(), dest, bchar, achar);
        } else {
            bchar = foldCase(bchar);
            std::replace_copy_if(hit, d.end(), dest, matchesCIS(bchar), achar);
        }

        swap(other);
    }
    return *this;
}

/*!
  \since 4.5
  \overload replace()

  Replaces every occurrence in this string of the Latin-1 string viewed
  by \a before with the Latin-1 string viewed by \a after, and returns a
  reference to this string.

  \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

  \note The text is not rescanned after a replacement.

  \include qstring.cpp empty-before-arg-in-replace
*/
QString &QString::replace(QLatin1StringView before, QLatin1StringView after, Qt::CaseSensitivity cs)
{
    const qsizetype alen = after.size();
    const qsizetype blen = before.size();
    if (blen == 1 && alen == 1)
        return replace(before.front(), after.front(), cs);

    QVarLengthArray<char16_t> a = qt_from_latin1_to_qvla(after);
    QVarLengthArray<char16_t> b = qt_from_latin1_to_qvla(before);
    return replace((const QChar *)b.data(), blen, (const QChar *)a.data(), alen, cs);
}

/*!
  \since 4.5
  \overload replace()

  Replaces every occurrence in this string of the Latin-1 string viewed
  by \a before with the string \a after, and returns a reference to this
  string.

  \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

  \note The text is not rescanned after a replacement.

  \include qstring.cpp empty-before-arg-in-replace
*/
QString &QString::replace(QLatin1StringView before, const QString &after, Qt::CaseSensitivity cs)
{
    const qsizetype blen = before.size();
    if (blen == 1 && after.size() == 1)
        return replace(before.front(), after.front(), cs);

    QVarLengthArray<char16_t> b = qt_from_latin1_to_qvla(before);
    return replace((const QChar *)b.data(), blen, after.constData(), after.d.size, cs);
}

/*!
  \since 4.5
  \overload replace()

  Replaces every occurrence of the string \a before with the string \a
  after and returns a reference to this string.

  \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

  \note The text is not rescanned after a replacement.

  \include qstring.cpp empty-before-arg-in-replace
*/
QString &QString::replace(const QString &before, QLatin1StringView after, Qt::CaseSensitivity cs)
{
    const qsizetype alen = after.size();
    if (before.size() == 1 && alen == 1)
        return replace(before.front(), after.front(), cs);

    QVarLengthArray<char16_t> a = qt_from_latin1_to_qvla(after);
    return replace(before.constData(), before.d.size, (const QChar *)a.data(), alen, cs);
}

/*!
  \since 4.5
  \overload replace()

  Replaces every occurrence of the character \a c with the string \a
  after and returns a reference to this string.

  \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

  \note The text is not rescanned after a replacement.
*/
QString &QString::replace(QChar c, QLatin1StringView after, Qt::CaseSensitivity cs)
{
    const qsizetype alen = after.size();
    if (alen == 1)
        return replace(c, after.front(), cs);

    QVarLengthArray<char16_t> a = qt_from_latin1_to_qvla(after);
    return replace(&c, 1, (const QChar *)a.data(), alen, cs);
}

/*!
    \fn bool QString::operator==(const QString &lhs, const QString &rhs)
    \overload operator==()

    Returns \c true if string \a lhs is equal to string \a rhs; otherwise
    returns \c false.

    \include qstring.cpp compare-isNull-vs-isEmpty

    \sa {Comparing Strings}
*/

/*!
    \fn bool QString::operator==(const QString &lhs, const QLatin1StringView &rhs)

    \overload operator==()

    Returns \c true if \a lhs is equal to \a rhs; otherwise
    returns \c false.
*/

/*!
    \fn bool QString::operator==(const QLatin1StringView &lhs, const QString &rhs)

    \overload operator==()

    Returns \c true if \a lhs is equal to \a rhs; otherwise
    returns \c false.
*/

/*! \fn bool QString::operator==(const QString &lhs, const QByteArray &rhs)

    \overload operator==()

    The \a rhs byte array is converted to a QUtf8StringView.

    You can disable this operator by defining
    \l QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.

    Returns \c true if string \a lhs is lexically equal to \a rhs.
    Otherwise returns \c false.
*/

/*! \fn bool QString::operator==(const QString &lhs, const char * const &rhs)

    \overload operator==()

    The \a rhs const char pointer is converted to a QUtf8StringView.

    You can disable this operator by defining
    \l QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*!
    \fn bool QString::operator<(const QString &lhs, const QString &rhs)

    \overload operator<()

    Returns \c true if string \a lhs is lexically less than string
    \a rhs; otherwise returns \c false.

    \sa {Comparing Strings}
*/

/*!
    \fn bool QString::operator<(const QString &lhs, const QLatin1StringView &rhs)

    \overload operator<()

    Returns \c true if \a lhs is lexically less than \a rhs;
    otherwise returns \c false.
*/

/*!
    \fn bool QString::operator<(const QLatin1StringView &lhs, const QString &rhs)

    \overload operator<()

    Returns \c true if \a lhs is lexically less than \a rhs;
    otherwise returns \c false.
*/

/*! \fn bool QString::operator<(const QString &lhs, const QByteArray &rhs)

    \overload operator<()

    The \a rhs byte array is converted to a QUtf8StringView.
    If any NUL characters ('\\0') are embedded in the byte array, they will be
    included in the transformation.

    You can disable this operator
    \l QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn bool QString::operator<(const QString &lhs, const char * const &rhs)

    Returns \c true if string \a lhs is lexically less than string \a rhs.
    Otherwise returns \c false.

    \overload operator<()

    The \a rhs const char pointer is converted to a QUtf8StringView.

    You can disable this operator by defining
    \l QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn bool QString::operator<=(const QString &lhs, const QString &rhs)

    Returns \c true if string \a lhs is lexically less than or equal to
    string \a rhs; otherwise returns \c false.

    \sa {Comparing Strings}
*/

/*!
    \fn bool QString::operator<=(const QString &lhs, const QLatin1StringView &rhs)

    \overload operator<=()

    Returns \c true if \a lhs is lexically less than or equal to \a rhs;
    otherwise returns \c false.
*/

/*!
    \fn bool QString::operator<=(const QLatin1StringView &lhs, const QString &rhs)

    \overload operator<=()

    Returns \c true if \a lhs is lexically less than or equal to \a rhs;
    otherwise returns \c false.
*/

/*! \fn bool QString::operator<=(const QString &lhs, const QByteArray &rhs)

    \overload operator<=()

    The \a rhs byte array is converted to a QUtf8StringView.
    If any NUL characters ('\\0') are embedded in the byte array, they will be
    included in the transformation.

    You can disable this operator by defining
    \l QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn bool QString::operator<=(const QString &lhs, const char * const &rhs)

    \overload operator<=()

    The \a rhs const char pointer is converted to a QUtf8StringView.

    You can disable this operator by defining
    \l QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn bool QString::operator>(const QString &lhs, const QString &rhs)

    Returns \c true if string \a lhs is lexically greater than string \a rhs;
    otherwise returns \c false.

    \sa {Comparing Strings}
*/

/*!
    \fn bool QString::operator>(const QString &lhs, const QLatin1StringView &rhs)

    \overload operator>()

    Returns \c true if \a lhs is lexically greater than \a rhs;
    otherwise returns \c false.
*/

/*!
    \fn bool QString::operator>(const QLatin1StringView &lhs, const QString &rhs)

    \overload operator>()

    Returns \c true if \a lhs is lexically greater than \a rhs;
    otherwise returns \c false.
*/

/*! \fn bool QString::operator>(const QString &lhs, const QByteArray &rhs)

    \overload operator>()

    The \a rhs byte array is converted to a QUtf8StringView.
    If any NUL characters ('\\0') are embedded in the byte array, they will be
    included in the transformation.

    You can disable this operator by defining
    \l QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn bool QString::operator>(const QString &lhs, const char * const &rhs)

    \overload operator>()

    The \a rhs const char pointer is converted to a QUtf8StringView.

    You can disable this operator by defining \l QT_NO_CAST_FROM_ASCII
    when you compile your applications. This can be useful if you want
    to ensure that all user-visible strings go through QObject::tr(),
    for example.
*/

/*! \fn bool QString::operator>=(const QString &lhs, const QString &rhs)

    Returns \c true if string \a lhs is lexically greater than or equal to
    string \a rhs; otherwise returns \c false.

    \sa {Comparing Strings}
*/

/*!
    \fn bool QString::operator>=(const QString &lhs, const QLatin1StringView &rhs)

    \overload operator>=()

    Returns \c true if \a lhs is lexically greater than or equal to \a rhs;
    otherwise returns \c false.
*/

/*!
    \fn bool QString::operator>=(const QLatin1StringView &lhs, const QString &rhs)

    \overload operator>=()

    Returns \c true if \a lhs is lexically greater than or equal to \a rhs;
    otherwise returns \c false.
*/

/*! \fn bool QString::operator>=(const QString &lhs, const QByteArray &rhs)

    \overload operator>=()

    The \a rhs byte array is converted to a QUtf8StringView.
    If any NUL characters ('\\0') are embedded in the byte array, they will be
    included in the transformation.

    You can disable this operator by defining \l QT_NO_CAST_FROM_ASCII
    when you compile your applications. This can be useful if you want
    to ensure that all user-visible strings go through QObject::tr(),
    for example.
*/

/*! \fn bool QString::operator>=(const QString &lhs, const char * const &rhs)

    \overload operator>=()

    The \a rhs const char pointer is converted to a QUtf8StringView.

    You can disable this operator by defining \l QT_NO_CAST_FROM_ASCII
    when you compile your applications. This can be useful if you want
    to ensure that all user-visible strings go through QObject::tr(),
    for example.
*/

/*! \fn bool QString::operator!=(const QString &lhs, const QString &rhs)

    Returns \c true if string \a lhs is not equal to string \a rhs;
    otherwise returns \c false.

    \sa {Comparing Strings}
*/

/*! \fn bool QString::operator!=(const QString &lhs, const QLatin1StringView &rhs)

    Returns \c true if string \a lhs is not equal to string \a rhs.
    Otherwise returns \c false.

    \overload operator!=()
*/

/*! \fn bool QString::operator!=(const QString &lhs, const QByteArray &rhs)

    \overload operator!=()

    The \a rhs byte array is converted to a QUtf8StringView.
    If any NUL characters ('\\0') are embedded in the byte array, they will be
    included in the transformation.

    You can disable this operator by defining \l QT_NO_CAST_FROM_ASCII
    when you compile your applications. This can be useful if you want
    to ensure that all user-visible strings go through QObject::tr(),
    for example.
*/

/*! \fn bool QString::operator!=(const QString &lhs, const char * const &rhs)

    \overload operator!=()

    The \a rhs const char pointer is converted to a QUtf8StringView.

    You can disable this operator by defining
    \l QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn bool QString::operator==(const QByteArray &lhs, const QString &rhs)

    Returns \c true if byte array \a lhs is equal to the UTF-8 encoding of
    \a rhs; otherwise returns \c false.

    The comparison is case sensitive.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. You
    then need to call QString::fromUtf8(), QString::fromLatin1(),
    or QString::fromLocal8Bit() explicitly if you want to convert the byte
    array to a QString before doing the comparison.
*/

/*! \fn bool QString::operator!=(const QByteArray &lhs, const QString &rhs)

    Returns \c true if byte array \a lhs is not equal to the UTF-8 encoding of
    \a rhs; otherwise returns \c false.

    The comparison is case sensitive.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. You
    then need to call QString::fromUtf8(), QString::fromLatin1(),
    or QString::fromLocal8Bit() explicitly if you want to convert the byte
    array to a QString before doing the comparison.
*/

/*! \fn bool QString::operator<(const QByteArray &lhs, const QString &rhs)

    Returns \c true if byte array \a lhs is lexically less than the UTF-8 encoding
    of \a rhs; otherwise returns \c false.

    The comparison is case sensitive.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. You
    then need to call QString::fromUtf8(), QString::fromLatin1(),
    or QString::fromLocal8Bit() explicitly if you want to convert the byte
    array to a QString before doing the comparison.
*/

/*! \fn bool QString::operator>(const QByteArray &lhs, const QString &rhs)

    Returns \c true if byte array \a lhs is lexically greater than the UTF-8
    encoding of \a rhs; otherwise returns \c false.

    The comparison is case sensitive.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. You
    then need to call QString::fromUtf8(), QString::fromLatin1(),
    or QString::fromLocal8Bit() explicitly if you want to convert the byte
    array to a QString before doing the comparison.
*/

/*! \fn bool QString::operator<=(const QByteArray &lhs, const QString &rhs)

    Returns \c true if byte array \a lhs is lexically less than or equal to the
    UTF-8 encoding of \a rhs; otherwise returns \c false.

    The comparison is case sensitive.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. You
    then need to call QString::fromUtf8(), QString::fromLatin1(),
    or QString::fromLocal8Bit() explicitly if you want to convert the byte
    array to a QString before doing the comparison.
*/

/*! \fn bool QString::operator>=(const QByteArray &lhs, const QString &rhs)

    Returns \c true if byte array \a lhs is greater than or equal to the UTF-8
    encoding of \a rhs; otherwise returns \c false.

    The comparison is case sensitive.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. You
    then need to call QString::fromUtf8(), QString::fromLatin1(),
    or QString::fromLocal8Bit() explicitly if you want to convert the byte
    array to a QString before doing the comparison.
*/

/*!
  \include qstring.qdocinc {qstring-first-index-of} {string} {str}

  \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

  Example:

  \snippet qstring/main.cpp 24

  \include qstring.qdocinc negative-index-start-search-from-end

  \sa lastIndexOf(), contains(), count()
*/
qsizetype QString::indexOf(const QString &str, qsizetype from, Qt::CaseSensitivity cs) const
{
    return QtPrivate::findString(QStringView(unicode(), size()), from, QStringView(str.unicode(), str.size()), cs);
}

/*!
    \fn qsizetype QString::indexOf(QStringView str, qsizetype from, Qt::CaseSensitivity cs) const
    \since 5.14
    \overload indexOf()

    \include qstring.qdocinc {qstring-first-index-of} {string view} {str}

    \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    \include qstring.qdocinc negative-index-start-search-from-end

    \sa QStringView::indexOf(), lastIndexOf(), contains(), count()
*/

/*!
  \since 4.5

  \include {qstring.qdocinc} {qstring-first-index-of} {Latin-1 string viewed by} {str}

  \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

  Example:

  \snippet qstring/main.cpp 24

  \include qstring.qdocinc negative-index-start-search-from-end

  \sa lastIndexOf(), contains(), count()
*/

qsizetype QString::indexOf(QLatin1StringView str, qsizetype from, Qt::CaseSensitivity cs) const
{
    return QtPrivate::findString(QStringView(unicode(), size()), from, str, cs);
}

/*!
    \fn qsizetype QString::indexOf(QChar ch, qsizetype from, Qt::CaseSensitivity cs) const
    \overload indexOf()

    \include qstring.qdocinc {qstring-first-index-of} {character} {ch}
*/

/*!
  \include qstring.qdocinc {qstring-last-index-of} {string} {str}

  \include qstring.qdocinc negative-index-start-search-from-end

  Returns -1 if \a str is not found.

  \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

  Example:

  \snippet qstring/main.cpp 29

  \note When searching for a 0-length \a str, the match at the end of
  the data is excluded from the search by a negative \a from, even
  though \c{-1} is normally thought of as searching from the end of the
  string: the match at the end is \e after the last character, so it is
  excluded. To include such a final empty match, either give a positive
  value for \a from or omit the \a from parameter entirely.

  \sa indexOf(), contains(), count()
*/
qsizetype QString::lastIndexOf(const QString &str, qsizetype from, Qt::CaseSensitivity cs) const
{
    return QtPrivate::lastIndexOf(QStringView(*this), from, str, cs);
}

/*!
  \fn qsizetype QString::lastIndexOf(const QString &str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
  \since 6.2
  \overload lastIndexOf()

  Returns the index position of the last occurrence of the string \a
  str in this string. Returns -1 if \a str is not found.

  \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

  Example:

  \snippet qstring/main.cpp 29

  \sa indexOf(), contains(), count()
*/


/*!
  \since 4.5
  \overload lastIndexOf()

  \include qstring.qdocinc {qstring-last-index-of} {Latin-1 string viewed by} {str}

  \include qstring.qdocinc negative-index-start-search-from-end

  Returns -1 if \a str is not found.

  \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

  Example:

  \snippet qstring/main.cpp 29

  \note When searching for a 0-length \a str, the match at the end of
  the data is excluded from the search by a negative \a from, even
  though \c{-1} is normally thought of as searching from the end of the
  string: the match at the end is \e after the last character, so it is
  excluded. To include such a final empty match, either give a positive
  value for \a from or omit the \a from parameter entirely.

  \sa indexOf(), contains(), count()
*/
qsizetype QString::lastIndexOf(QLatin1StringView str, qsizetype from, Qt::CaseSensitivity cs) const
{
    return QtPrivate::lastIndexOf(*this, from, str, cs);
}

/*!
  \fn qsizetype QString::lastIndexOf(QLatin1StringView str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
  \since 6.2
  \overload lastIndexOf()

  Returns the index position of the last occurrence of the string \a
  str in this string. Returns -1 if \a str is not found.

  \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

  Example:

  \snippet qstring/main.cpp 29

  \sa indexOf(), contains(), count()
*/

/*!
  \fn qsizetype QString::lastIndexOf(QChar ch, qsizetype from, Qt::CaseSensitivity cs) const
  \overload lastIndexOf()

  \include qstring.qdocinc {qstring-last-index-of} {character} {ch}
*/

/*!
  \fn QString::lastIndexOf(QChar ch, Qt::CaseSensitivity) const
  \since 6.3
  \overload lastIndexOf()
*/

/*!
  \fn qsizetype QString::lastIndexOf(QStringView str, qsizetype from, Qt::CaseSensitivity cs) const
  \since 5.14
  \overload lastIndexOf()

  \include qstring.qdocinc {qstring-last-index-of} {string view} {str}

  \include qstring.qdocinc negative-index-start-search-from-end

  Returns -1 if \a str is not found.

  \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

  \note When searching for a 0-length \a str, the match at the end of
  the data is excluded from the search by a negative \a from, even
  though \c{-1} is normally thought of as searching from the end of the
  string: the match at the end is \e after the last character, so it is
  excluded. To include such a final empty match, either give a positive
  value for \a from or omit the \a from parameter entirely.

  \sa indexOf(), contains(), count()
*/

/*!
  \fn qsizetype QString::lastIndexOf(QStringView str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
  \since 6.2
  \overload lastIndexOf()

  Returns the index position of the last occurrence of the string view \a
  str in this string. Returns -1 if \a str is not found.

  \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

  \sa indexOf(), contains(), count()
*/

#if QT_CONFIG(regularexpression)
struct QStringCapture
{
    qsizetype pos;
    qsizetype len;
    int no;
};
Q_DECLARE_TYPEINFO(QStringCapture, Q_PRIMITIVE_TYPE);

/*!
  \overload replace()
  \since 5.0

  Replaces every occurrence of the regular expression \a re in the
  string with \a after. Returns a reference to the string. For
  example:

  \snippet qstring/main.cpp 87

  For regular expressions containing capturing groups,
  occurrences of \b{\\1}, \b{\\2}, ..., in \a after are replaced
  with the string captured by the corresponding capturing group.

  \snippet qstring/main.cpp 88

  \sa indexOf(), lastIndexOf(), remove(), QRegularExpression, QRegularExpressionMatch
*/
QString &QString::replace(const QRegularExpression &re, const QString &after)
{
    if (!re.isValid()) {
        qtWarnAboutInvalidRegularExpression(re, "QString", "replace");
        return *this;
    }

    const QString copy(*this);
    QRegularExpressionMatchIterator iterator = re.globalMatch(copy);
    if (!iterator.hasNext()) // no matches at all
        return *this;

    reallocData(d.size, QArrayData::KeepSize);

    qsizetype numCaptures = re.captureCount();

    // 1. build the backreferences list, holding where the backreferences
    // are in the replacement string
    QVarLengthArray<QStringCapture> backReferences;
    const qsizetype al = after.size();
    const QChar *ac = after.unicode();

    for (qsizetype i = 0; i < al - 1; i++) {
        if (ac[i] == u'\\') {
            int no = ac[i + 1].digitValue();
            if (no > 0 && no <= numCaptures) {
                QStringCapture backReference;
                backReference.pos = i;
                backReference.len = 2;

                if (i < al - 2) {
                    int secondDigit = ac[i + 2].digitValue();
                    if (secondDigit != -1 && ((no * 10) + secondDigit) <= numCaptures) {
                        no = (no * 10) + secondDigit;
                        ++backReference.len;
                    }
                }

                backReference.no = no;
                backReferences.append(backReference);
            }
        }
    }

    // 2. iterate on the matches. For every match, copy in chunks
    // - the part before the match
    // - the after string, with the proper replacements for the backreferences

    qsizetype newLength = 0; // length of the new string, with all the replacements
    qsizetype lastEnd = 0;
    QVarLengthArray<QStringView> chunks;
    const QStringView copyView{ copy }, afterView{ after };
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        qsizetype len;
        // add the part before the match
        len = match.capturedStart() - lastEnd;
        if (len > 0) {
            chunks << copyView.mid(lastEnd, len);
            newLength += len;
        }

        lastEnd = 0;
        // add the after string, with replacements for the backreferences
        for (const QStringCapture &backReference : std::as_const(backReferences)) {
            // part of "after" before the backreference
            len = backReference.pos - lastEnd;
            if (len > 0) {
                chunks << afterView.mid(lastEnd, len);
                newLength += len;
            }

            // backreference itself
            len = match.capturedLength(backReference.no);
            if (len > 0) {
                chunks << copyView.mid(match.capturedStart(backReference.no), len);
                newLength += len;
            }

            lastEnd = backReference.pos + backReference.len;
        }

        // add the last part of the after string
        len = afterView.size() - lastEnd;
        if (len > 0) {
            chunks << afterView.mid(lastEnd, len);
            newLength += len;
        }

        lastEnd = match.capturedEnd();
    }

    // 3. trailing string after the last match
    if (copyView.size() > lastEnd) {
        chunks << copyView.mid(lastEnd);
        newLength += copyView.size() - lastEnd;
    }

    // 4. assemble the chunks together
    resize(newLength);
    qsizetype i = 0;
    QChar *uc = data();
    for (const QStringView &chunk : std::as_const(chunks)) {
        qsizetype len = chunk.size();
        memcpy(uc + i, chunk.constData(), len * sizeof(QChar));
        i += len;
    }

    return *this;
}
#endif // QT_CONFIG(regularexpression)

/*!
    Returns the number of (potentially overlapping) occurrences of
    the string \a str in this string.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    \sa contains(), indexOf()
*/

qsizetype QString::count(const QString &str, Qt::CaseSensitivity cs) const
{
    return QtPrivate::count(QStringView(unicode(), size()), QStringView(str.unicode(), str.size()), cs);
}

/*!
    \overload count()

    Returns the number of occurrences of character \a ch in the string.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    \sa contains(), indexOf()
*/

qsizetype QString::count(QChar ch, Qt::CaseSensitivity cs) const
{
    return QtPrivate::count(QStringView(unicode(), size()), ch, cs);
}

/*!
    \since 6.0
    \overload count()
    Returns the number of (potentially overlapping) occurrences of the
    string view \a str in this string.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    \sa contains(), indexOf()
*/
qsizetype QString::count(QStringView str, Qt::CaseSensitivity cs) const
{
    return QtPrivate::count(*this, str, cs);
}

/*! \fn bool QString::contains(const QString &str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const

    Returns \c true if this string contains an occurrence of the string
    \a str; otherwise returns \c false.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    Example:
    \snippet qstring/main.cpp 17

    \sa indexOf(), count()
*/

/*! \fn bool QString::contains(QLatin1StringView str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    \since 5.3

    \overload contains()

    Returns \c true if this string contains an occurrence of the latin-1 string
    \a str; otherwise returns \c false.
*/

/*! \fn bool QString::contains(QChar ch, Qt::CaseSensitivity cs = Qt::CaseSensitive) const

    \overload contains()

    Returns \c true if this string contains an occurrence of the
    character \a ch; otherwise returns \c false.
*/

/*! \fn bool QString::contains(QStringView str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    \since 5.14
    \overload contains()

    Returns \c true if this string contains an occurrence of the string view
    \a str; otherwise returns \c false.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    \sa indexOf(), count()
*/

#if QT_CONFIG(regularexpression)
/*!
    \since 5.5

    Returns the index position of the first match of the regular
    expression \a re in the string, searching forward from index
    position \a from. Returns -1 if \a re didn't match anywhere.

    If the match is successful and \a rmatch is not \nullptr, it also
    writes the results of the match into the QRegularExpressionMatch object
    pointed to by \a rmatch.

    Example:

    \snippet qstring/main.cpp 93
*/
qsizetype QString::indexOf(const QRegularExpression &re, qsizetype from, QRegularExpressionMatch *rmatch) const
{
    return QtPrivate::indexOf(QStringView(*this), this, re, from, rmatch);
}

/*!
    \since 5.5

    Returns the index position of the last match of the regular
    expression \a re in the string, which starts before the index
    position \a from.

    \include qstring.qdocinc negative-index-start-search-from-end

    Returns -1 if \a re didn't match anywhere.

    If the match is successful and \a rmatch is not \nullptr, it also
    writes the results of the match into the QRegularExpressionMatch object
    pointed to by \a rmatch.

    Example:

    \snippet qstring/main.cpp 94

    \note Due to how the regular expression matching algorithm works,
    this function will actually match repeatedly from the beginning of
    the string until the position \a from is reached.

    \note When searching for a regular expression \a re that may match
    0 characters, the match at the end of the data is excluded from the
    search by a negative \a from, even though \c{-1} is normally
    thought of as searching from the end of the string: the match at
    the end is \e after the last character, so it is excluded. To
    include such a final empty match, either give a positive value for
    \a from or omit the \a from parameter entirely.
*/
qsizetype QString::lastIndexOf(const QRegularExpression &re, qsizetype from, QRegularExpressionMatch *rmatch) const
{
    return QtPrivate::lastIndexOf(QStringView(*this), this, re, from, rmatch);
}

/*!
    \fn qsizetype QString::lastIndexOf(const QRegularExpression &re, QRegularExpressionMatch *rmatch = nullptr) const
    \since 6.2
    \overload lastIndexOf()

    Returns the index position of the last match of the regular
    expression \a re in the string. Returns -1 if \a re didn't match anywhere.

    If the match is successful and \a rmatch is not \nullptr, it also
    writes the results of the match into the QRegularExpressionMatch object
    pointed to by \a rmatch.

    Example:

    \snippet qstring/main.cpp 94

    \note Due to how the regular expression matching algorithm works,
    this function will actually match repeatedly from the beginning of
    the string until the end of the string is reached.
*/

/*!
    \since 5.1

    Returns \c true if the regular expression \a re matches somewhere in this
    string; otherwise returns \c false.

    If the match is successful and \a rmatch is not \nullptr, it also
    writes the results of the match into the QRegularExpressionMatch object
    pointed to by \a rmatch.

    \sa QRegularExpression::match()
*/

bool QString::contains(const QRegularExpression &re, QRegularExpressionMatch *rmatch) const
{
    return QtPrivate::contains(QStringView(*this), this, re, rmatch);
}

/*!
    \overload count()
    \since 5.0

    Returns the number of times the regular expression \a re matches
    in the string.

    For historical reasons, this function counts overlapping matches,
    so in the example below, there are four instances of "ana" or
    "ama":

    \snippet qstring/main.cpp 95

    This behavior is different from simply iterating over the matches
    in the string using QRegularExpressionMatchIterator.

    \sa QRegularExpression::globalMatch()
*/
qsizetype QString::count(const QRegularExpression &re) const
{
    return QtPrivate::count(QStringView(*this), re);
}
#endif // QT_CONFIG(regularexpression)

#if QT_DEPRECATED_SINCE(6, 4)
/*! \fn qsizetype QString::count() const
    \deprecated [6.4] Use size() or length() instead.
    \overload count()

    Same as size().
*/
#endif

/*!
    \enum QString::SectionFlag

    This enum specifies flags that can be used to affect various
    aspects of the section() function's behavior with respect to
    separators and empty fields.

    \value SectionDefault Empty fields are counted, leading and
    trailing separators are not included, and the separator is
    compared case sensitively.

    \value SectionSkipEmpty Treat empty fields as if they don't exist,
    i.e. they are not considered as far as \e start and \e end are
    concerned.

    \value SectionIncludeLeadingSep Include the leading separator (if
    any) in the result string.

    \value SectionIncludeTrailingSep Include the trailing separator
    (if any) in the result string.

    \value SectionCaseInsensitiveSeps Compare the separator
    case-insensitively.

    \sa section()
*/

/*!
    \fn QString QString::section(QChar sep, qsizetype start, qsizetype end = -1, SectionFlags flags) const

    This function returns a section of the string.

    This string is treated as a sequence of fields separated by the
    character, \a sep. The returned string consists of the fields from
    position \a start to position \a end inclusive. If \a end is not
    specified, all fields from position \a start to the end of the
    string are included. Fields are numbered 0, 1, 2, etc., counting
    from the left, and -1, -2, etc., counting from right to left.

    The \a flags argument can be used to affect some aspects of the
    function's behavior, e.g. whether to be case sensitive, whether
    to skip empty fields and how to deal with leading and trailing
    separators; see \l{SectionFlags}.

    \snippet qstring/main.cpp 52

    If \a start or \a end is negative, we count fields from the right
    of the string, the right-most field being -1, the one from
    right-most field being -2, and so on.

    \snippet qstring/main.cpp 53

    \sa split()
*/

/*!
    \overload section()

    \snippet qstring/main.cpp 51
    \snippet qstring/main.cpp 54

    \sa split()
*/

QString QString::section(const QString &sep, qsizetype start, qsizetype end, SectionFlags flags) const
{
    const QList<QStringView> sections = QStringView{ *this }.split(
            sep, Qt::KeepEmptyParts, (flags & SectionCaseInsensitiveSeps) ? Qt::CaseInsensitive : Qt::CaseSensitive);
    const qsizetype sectionsSize = sections.size();
    if (!(flags & SectionSkipEmpty)) {
        if (start < 0)
            start += sectionsSize;
        if (end < 0)
            end += sectionsSize;
    } else {
        qsizetype skip = 0;
        for (qsizetype k = 0; k < sectionsSize; ++k) {
            if (sections.at(k).isEmpty())
                skip++;
        }
        if (start < 0)
            start += sectionsSize - skip;
        if (end < 0)
            end += sectionsSize - skip;
    }
    if (start >= sectionsSize || end < 0 || start > end)
        return QString();

    QString ret;
    qsizetype first_i = start, last_i = end;
    for (qsizetype x = 0, i = 0; x <= end && i < sectionsSize; ++i) {
        const QStringView &section = sections.at(i);
        const bool empty = section.isEmpty();
        if (x >= start) {
            if (x == start)
                first_i = i;
            if (x == end)
                last_i = i;
            if (x > start && i > 0)
                ret += sep;
            ret += section;
        }
        if (!empty || !(flags & SectionSkipEmpty))
            x++;
    }
    if ((flags & SectionIncludeLeadingSep) && first_i > 0)
        ret.prepend(sep);
    if ((flags & SectionIncludeTrailingSep) && last_i < sectionsSize - 1)
        ret += sep;
    return ret;
}

#if QT_CONFIG(regularexpression)
struct qt_section_chunk
{
    qsizetype length;
    QStringView string;
};
Q_DECLARE_TYPEINFO(qt_section_chunk, Q_RELOCATABLE_TYPE);

static QString extractSections(QSpan<qt_section_chunk> sections, qsizetype start, qsizetype end,
                               QString::SectionFlags flags)
{
    const qsizetype sectionsSize = sections.size();

    if (!(flags & QString::SectionSkipEmpty)) {
        if (start < 0)
            start += sectionsSize;
        if (end < 0)
            end += sectionsSize;
    } else {
        qsizetype skip = 0;
        for (qsizetype k = 0; k < sectionsSize; ++k) {
            const qt_section_chunk &section = sections[k];
            if (section.length == section.string.size())
                skip++;
        }
        if (start < 0)
            start += sectionsSize - skip;
        if (end < 0)
            end += sectionsSize - skip;
    }
    if (start >= sectionsSize || end < 0 || start > end)
        return QString();

    QString ret;
    qsizetype x = 0;
    qsizetype first_i = start, last_i = end;
    for (qsizetype i = 0; x <= end && i < sectionsSize; ++i) {
        const qt_section_chunk &section = sections[i];
        const bool empty = (section.length == section.string.size());
        if (x >= start) {
            if (x == start)
                first_i = i;
            if (x == end)
                last_i = i;
            if (x != start)
                ret += section.string;
            else
                ret += section.string.mid(section.length);
        }
        if (!empty || !(flags & QString::SectionSkipEmpty))
            x++;
    }

    if ((flags & QString::SectionIncludeLeadingSep) && first_i >= 0) {
        const qt_section_chunk &section = sections[first_i];
        ret.prepend(section.string.left(section.length));
    }

    if ((flags & QString::SectionIncludeTrailingSep)
        && last_i < sectionsSize - 1) {
        const qt_section_chunk &section = sections[last_i + 1];
        ret += section.string.left(section.length);
    }

    return ret;
}

/*!
    \overload section()
    \since 5.0

    This string is treated as a sequence of fields separated by the
    regular expression, \a re.

    \snippet qstring/main.cpp 89

    \warning Using this QRegularExpression version is much more expensive than
    the overloaded string and character versions.

    \sa split(), simplified()
*/
QString QString::section(const QRegularExpression &re, qsizetype start, qsizetype end, SectionFlags flags) const
{
    if (!re.isValid()) {
        qtWarnAboutInvalidRegularExpression(re, "QString", "section");
        return QString();
    }

    const QChar *uc = unicode();
    if (!uc)
        return QString();

    QRegularExpression sep(re);
    if (flags & SectionCaseInsensitiveSeps)
        sep.setPatternOptions(sep.patternOptions() | QRegularExpression::CaseInsensitiveOption);

    QVarLengthArray<qt_section_chunk> sections;
    qsizetype n = size(), m = 0, last_m = 0, last_len = 0;
    QRegularExpressionMatchIterator iterator = sep.globalMatch(*this);
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        m = match.capturedStart();
        sections.append(qt_section_chunk{last_len, QStringView{*this}.sliced(last_m, m - last_m)});
        last_m = m;
        last_len = match.capturedLength();
    }
    sections.append(qt_section_chunk{last_len, QStringView{*this}.sliced(last_m, n - last_m)});

    return extractSections(sections, start, end, flags);
}
#endif // QT_CONFIG(regularexpression)

/*!
    \fn QString QString::left(qsizetype n) const &
    \fn QString QString::left(qsizetype n) &&

    Returns a substring that contains the \a n leftmost characters of
    this string (that is, from the beginning of this string up to, but not
    including, the element at index position \a n).

    If you know that \a n cannot be out of bounds, use first() instead in new
    code, because it is faster.

    The entire string is returned if \a n is greater than or equal
    to size(), or less than zero.

    \sa first(), last(), startsWith(), chopped(), chop(), truncate()
*/

/*!
    \fn QString QString::right(qsizetype n) const &
    \fn QString QString::right(qsizetype n) &&

    Returns a substring that contains the \a n rightmost characters
    of the string.

    If you know that \a n cannot be out of bounds, use last() instead in new
    code, because it is faster.

    The entire string is returned if \a n is greater than or equal
    to size(), or less than zero.

    \sa endsWith(), last(), first(), sliced(), chopped(), chop(), truncate(), slice()
*/

/*!
    \fn QString QString::mid(qsizetype position, qsizetype n) const &
    \fn QString QString::mid(qsizetype position, qsizetype n) &&

    Returns a string that contains \a n characters of this string, starting
    at the specified \a position index up to, but not including, the element
    at index position \c {\a position + n}.

    If you know that \a position and \a n cannot be out of bounds, use sliced()
    instead in new code, because it is faster.

    Returns a null string if the \a position index exceeds the
    length of the string. If there are less than \a n characters
    available in the string starting at the given \a position, or if
    \a n is -1 (default), the function returns all characters that
    are available from the specified \a position.

    \sa first(), last(), sliced(), chopped(), chop(), truncate(), slice()
*/
QString QString::mid(qsizetype position, qsizetype n) const &
{
    qsizetype p = position;
    qsizetype l = n;
    using namespace QtPrivate;
    switch (QContainerImplHelper::mid(size(), &p, &l)) {
    case QContainerImplHelper::Null:
        return QString();
    case QContainerImplHelper::Empty:
        return QString(DataPointer::fromRawData(&_empty, 0));
    case QContainerImplHelper::Full:
        return *this;
    case QContainerImplHelper::Subset:
        return sliced(p, l);
    }
    Q_UNREACHABLE_RETURN(QString());
}

QString QString::mid(qsizetype position, qsizetype n) &&
{
    qsizetype p = position;
    qsizetype l = n;
    using namespace QtPrivate;
    switch (QContainerImplHelper::mid(size(), &p, &l)) {
    case QContainerImplHelper::Null:
        return QString();
    case QContainerImplHelper::Empty:
        resize(0);      // keep capacity if we've reserve()d
        [[fallthrough]];
    case QContainerImplHelper::Full:
        return std::move(*this);
    case QContainerImplHelper::Subset:
        return std::move(*this).sliced(p, l);
    }
    Q_UNREACHABLE_RETURN(QString());
}

/*!
    \fn QString QString::first(qsizetype n) const &
    \fn QString QString::first(qsizetype n) &&
    \since 6.0

    Returns a string that contains the first \a n characters of this string,
    (that is, from the beginning of this string up to, but not including,
    the element at index position \a n).

    \note The behavior is undefined when \a n < 0 or \a n > size().

    \snippet qstring/main.cpp 31

    \sa last(), sliced(), startsWith(), chopped(), chop(), truncate(), slice()
*/

/*!
    \fn QString QString::last(qsizetype n) const &
    \fn QString QString::last(qsizetype n) &&
    \since 6.0

    Returns the string that contains the last \a n characters of this string.

    \note The behavior is undefined when \a n < 0 or \a n > size().

    \snippet qstring/main.cpp 48

    \sa first(), sliced(), endsWith(), chopped(), chop(), truncate(), slice()
*/

/*!
    \fn QString QString::sliced(qsizetype pos, qsizetype n) const &
    \fn QString QString::sliced(qsizetype pos, qsizetype n) &&
    \since 6.0

    Returns a string that contains \a n characters of this string, starting
    at position \a pos up to, but not including, the element at index position
    \c {\a pos + n}.

    \note The behavior is undefined when \a pos < 0, \a n < 0,
    or \a pos + \a n > size().

    \snippet qstring/main.cpp 34

    \sa first(), last(), chopped(), chop(), truncate(), slice()
*/
QString QString::sliced_helper(QString &str, qsizetype pos, qsizetype n)
{
    if (n == 0)
        return QString(DataPointer::fromRawData(&_empty, 0));
    DataPointer d = std::move(str.d).sliced(pos, n);
    d.data()[n] = 0;
    return QString(std::move(d));
}

/*!
    \fn QString QString::sliced(qsizetype pos) const &
    \fn QString QString::sliced(qsizetype pos) &&
    \since 6.0
    \overload

    Returns a string that contains the portion of this string starting at
    position \a pos and extending to its end.

    \note The behavior is undefined when \a pos < 0 or \a pos > size().

    \sa first(), last(), chopped(), chop(), truncate(), slice()
*/

/*!
    \fn QString &QString::slice(qsizetype pos, qsizetype n)
    \since 6.8

    Modifies this string to start at position \a pos, up to, but not including,
    the character (code point) at index position \c {\a pos + n}; and returns
    a reference to this string.

    \note The behavior is undefined if \a pos < 0, \a n < 0,
    or \a pos + \a n > size().

    \snippet qstring/main.cpp slice97

    \sa sliced(), first(), last(), chopped(), chop(), truncate()
*/

/*!
    \fn QString &QString::slice(qsizetype pos)
    \since 6.8
    \overload

    Modifies this string to start at position \a pos and extending to its end,
    and returns a reference to this string.

    \note The behavior is undefined if \a pos < 0 or \a pos > size().

    \sa sliced(), first(), last(), chopped(), chop(), truncate()
*/

/*!
    \fn QString QString::chopped(qsizetype len) const &
    \fn QString QString::chopped(qsizetype len) &&
    \since 5.10

    Returns a string that contains the size() - \a len leftmost characters
    of this string.

    \note The behavior is undefined if \a len is negative or greater than size().

    \sa endsWith(), first(), last(), sliced(), chop(), truncate(), slice()
*/

/*!
    Returns \c true if the string starts with \a s; otherwise returns
    \c false.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    \snippet qstring/main.cpp 65

    \sa endsWith()
*/
bool QString::startsWith(const QString& s, Qt::CaseSensitivity cs) const
{
    return qt_starts_with_impl(QStringView(*this), QStringView(s), cs);
}

/*!
  \overload startsWith()
 */
bool QString::startsWith(QLatin1StringView s, Qt::CaseSensitivity cs) const
{
    return qt_starts_with_impl(QStringView(*this), s, cs);
}

/*!
  \overload startsWith()

  Returns \c true if the string starts with \a c; otherwise returns
  \c false.
*/
bool QString::startsWith(QChar c, Qt::CaseSensitivity cs) const
{
    if (!size())
        return false;
    if (cs == Qt::CaseSensitive)
        return at(0) == c;
    return foldCase(at(0)) == foldCase(c);
}

/*!
    \fn bool QString::startsWith(QStringView str, Qt::CaseSensitivity cs) const
    \since 5.10
    \overload

    Returns \c true if the string starts with the string view \a str;
    otherwise returns \c false.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    \sa endsWith()
*/

/*!
    Returns \c true if the string ends with \a s; otherwise returns
    \c false.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    \snippet qstring/main.cpp 20

    \sa startsWith()
*/
bool QString::endsWith(const QString &s, Qt::CaseSensitivity cs) const
{
    return qt_ends_with_impl(QStringView(*this), QStringView(s), cs);
}

/*!
    \fn bool QString::endsWith(QStringView str, Qt::CaseSensitivity cs) const
    \since 5.10
    \overload endsWith()
    Returns \c true if the string ends with the string view \a str;
    otherwise returns \c false.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    \sa startsWith()
*/

/*!
    \overload endsWith()
*/
bool QString::endsWith(QLatin1StringView s, Qt::CaseSensitivity cs) const
{
    return qt_ends_with_impl(QStringView(*this), s, cs);
}

/*!
  Returns \c true if the string ends with \a c; otherwise returns
  \c false.

  \overload endsWith()
 */
bool QString::endsWith(QChar c, Qt::CaseSensitivity cs) const
{
    if (!size())
        return false;
    if (cs == Qt::CaseSensitive)
        return at(size() - 1) == c;
    return foldCase(at(size() - 1)) == foldCase(c);
}

static bool checkCase(QStringView s, QUnicodeTables::Case c) noexcept
{
    QStringIterator it(s);
    while (it.hasNext()) {
        const char32_t uc = it.next();
        if (qGetProp(uc)->cases[c].diff)
            return false;
    }
    return true;
}

bool QtPrivate::isLower(QStringView s) noexcept
{
    return checkCase(s, QUnicodeTables::LowerCase);
}

bool QtPrivate::isUpper(QStringView s) noexcept
{
    return checkCase(s, QUnicodeTables::UpperCase);
}

/*!
    Returns \c true if the string is uppercase, that is, it's identical
    to its toUpper() folding.

    Note that this does \e not mean that the string does not contain
    lowercase letters (some lowercase letters do not have a uppercase
    folding; they are left unchanged by toUpper()).
    For more information, refer to the Unicode standard, section 3.13.

    \since 5.12

    \sa QChar::toUpper(), isLower()
*/
bool QString::isUpper() const
{
    return QtPrivate::isUpper(qToStringViewIgnoringNull(*this));
}

/*!
    Returns \c true if the string is lowercase, that is, it's identical
    to its toLower() folding.

    Note that this does \e not mean that the string does not contain
    uppercase letters (some uppercase letters do not have a lowercase
    folding; they are left unchanged by toLower()).
    For more information, refer to the Unicode standard, section 3.13.

    \since 5.12

    \sa QChar::toLower(), isUpper()
 */
bool QString::isLower() const
{
    return QtPrivate::isLower(qToStringViewIgnoringNull(*this));
}

static QByteArray qt_convert_to_latin1(QStringView string);

QByteArray QString::toLatin1_helper(const QString &string)
{
    return qt_convert_to_latin1(string);
}

/*!
    \since 6.0
    \internal
    \relates QAnyStringView

    Returns a UTF-16 representation of \a string as a QString.

    \sa QString::toLatin1(), QStringView::toLatin1(), QtPrivate::convertToUtf8(),
    QtPrivate::convertToLocal8Bit(), QtPrivate::convertToUcs4()
*/
QString QtPrivate::convertToQString(QAnyStringView string)
{
    return string.visit([] (auto string) { return string.toString(); });
}

/*!
    \since 5.10
    \internal
    \relates QStringView

    Returns a Latin-1 representation of \a string as a QByteArray.

    The behavior is undefined if \a string contains non-Latin1 characters.

    \sa QString::toLatin1(), QStringView::toLatin1(), QtPrivate::convertToUtf8(),
    QtPrivate::convertToLocal8Bit(), QtPrivate::convertToUcs4()
*/
QByteArray QtPrivate::convertToLatin1(QStringView string)
{
    return qt_convert_to_latin1(string);
}

Q_NEVER_INLINE
static QByteArray qt_convert_to_latin1(QStringView string)
{
    if (Q_UNLIKELY(string.isNull()))
        return QByteArray();

    QByteArray ba(string.size(), Qt::Uninitialized);

    // since we own the only copy, we're going to const_cast the constData;
    // that avoids an unnecessary call to detach() and expansion code that will never get used
    qt_to_latin1(reinterpret_cast<uchar *>(const_cast<char *>(ba.constData())),
                 string.utf16(), string.size());
    return ba;
}

QByteArray QString::toLatin1_helper_inplace(QString &s)
{
    if (!s.isDetached())
        return qt_convert_to_latin1(s);

    // We can return our own buffer to the caller.
    // Conversion to Latin-1 always shrinks the buffer by half.
    // This relies on the fact that we use QArrayData for everything behind the scenes

    // First, do the in-place conversion. Since isDetached() == true, the data
    // was allocated by QArrayData, so the null terminator must be there.
    qsizetype length = s.size();
    char16_t *sdata = s.d->data();
    Q_ASSERT(sdata[length] == u'\0');
    qt_to_latin1(reinterpret_cast<uchar *>(sdata), sdata, length + 1);

    // Move the internals over to the byte array.
    // Kids, avert your eyes. Don't try this at home.
    auto ba_d = std::move(s.d).reinterpreted<char>();

    // Some sanity checks
    Q_ASSERT(ba_d.d->allocatedCapacity() >= ba_d.size);
    Q_ASSERT(s.isNull());
    Q_ASSERT(s.isEmpty());
    Q_ASSERT(s.constData() == QString().constData());

    return QByteArray(std::move(ba_d));
}

/*!
    \since 6.9
    \internal
    \relates QLatin1StringView

    Returns a UTF-8 representation of \a string as a QByteArray.
*/
QByteArray QtPrivate::convertToUtf8(QLatin1StringView string)
{
    if (Q_UNLIKELY(string.isNull()))
        return QByteArray();

    // create a QByteArray with the worst case scenario size
    QByteArray ba(string.size() * 2, Qt::Uninitialized);
    const qsizetype sz = QUtf8::convertFromLatin1(ba.data(), string) - ba.data();
    ba.truncate(sz);

    return ba;
}

// QLatin1 methods that use helpers from qstring.cpp
char16_t *QLatin1::convertToUnicode(char16_t *out, QLatin1StringView in) noexcept
{
    const qsizetype len = in.size();
    qt_from_latin1(out, in.data(), len);
    return std::next(out, len);
}

char *QLatin1::convertFromUnicode(char *out, QStringView in) noexcept
{
    const qsizetype len = in.size();
    qt_to_latin1(reinterpret_cast<uchar *>(out), in.utf16(), len);
    return out + len;
}

/*!
    \fn QByteArray QString::toLatin1() const

    Returns a Latin-1 representation of the string as a QByteArray.

    The returned byte array is undefined if the string contains non-Latin1
    characters. Those characters may be suppressed or replaced with a
    question mark.

    \sa fromLatin1(), toUtf8(), toLocal8Bit(), QStringEncoder
*/

static QByteArray qt_convert_to_local_8bit(QStringView string);

/*!
    \fn QByteArray QString::toLocal8Bit() const

    Returns the local 8-bit representation of the string as a
    QByteArray.

    \include qstring.qdocinc {qstring-local-8-bit-equivalent} {toUtf8}

    If this string contains any characters that cannot be encoded in the
    local 8-bit encoding, the returned byte array is undefined. Those
    characters may be suppressed or replaced by another.

    \sa fromLocal8Bit(), toLatin1(), toUtf8(), QStringEncoder
*/

QByteArray QString::toLocal8Bit_helper(const QChar *data, qsizetype size)
{
    return qt_convert_to_local_8bit(QStringView(data, size));
}

static QByteArray qt_convert_to_local_8bit(QStringView string)
{
    if (string.isNull())
        return QByteArray();
    QStringEncoder fromUtf16(QStringEncoder::System, QStringEncoder::Flag::Stateless);
    return fromUtf16(string);
}

/*!
    \since 5.10
    \internal
    \relates QStringView

    Returns a local 8-bit representation of \a string as a QByteArray.

    On Unix systems this is equivalent to toUtf8(), on Windows the systems
    current code page is being used.

    The behavior is undefined if \a string contains characters not
    supported by the locale's 8-bit encoding.

    \sa QString::toLocal8Bit(), QStringView::toLocal8Bit()
*/
QByteArray QtPrivate::convertToLocal8Bit(QStringView string)
{
    return qt_convert_to_local_8bit(string);
}

static QByteArray qt_convert_to_utf8(QStringView str);

/*!
    \fn QByteArray QString::toUtf8() const

    Returns a UTF-8 representation of the string as a QByteArray.

    UTF-8 is a Unicode codec and can represent all characters in a Unicode
    string like QString.

    \sa fromUtf8(), toLatin1(), toLocal8Bit(), QStringEncoder
*/

QByteArray QString::toUtf8_helper(const QString &str)
{
    return qt_convert_to_utf8(str);
}

static QByteArray qt_convert_to_utf8(QStringView str)
{
    if (str.isNull())
        return QByteArray();

    return QUtf8::convertFromUnicode(str);
}

/*!
    \since 5.10
    \internal
    \relates QStringView

    Returns a UTF-8 representation of \a string as a QByteArray.

    UTF-8 is a Unicode codec and can represent all characters in a Unicode
    string like QStringView.

    \sa QString::toUtf8(), QStringView::toUtf8()
*/
QByteArray QtPrivate::convertToUtf8(QStringView string)
{
    return qt_convert_to_utf8(string);
}

static QList<uint> qt_convert_to_ucs4(QStringView string);

/*!
    \since 4.2

    Returns a UCS-4/UTF-32 representation of the string as a QList<uint>.

    UTF-32 is a Unicode codec and therefore it is lossless. All characters from
    this string will be encoded in UTF-32. Any invalid sequence of code units in
    this string is replaced by the Unicode replacement character
    (QChar::ReplacementCharacter, which corresponds to \c{U+FFFD}).

    The returned list is not 0-terminated.

    \sa fromUtf8(), toUtf8(), toLatin1(), toLocal8Bit(), QStringEncoder,
        fromUcs4(), toWCharArray()
*/
QList<uint> QString::toUcs4() const
{
    return qt_convert_to_ucs4(*this);
}

static QList<uint> qt_convert_to_ucs4(QStringView string)
{
    QList<uint> v(string.size());
    uint *a = const_cast<uint*>(v.constData());
    QStringIterator it(string);
    while (it.hasNext())
        *a++ = it.next();
    v.resize(a - v.constData());
    return v;
}

/*!
    \since 5.10
    \internal
    \relates QStringView

    Returns a UCS-4/UTF-32 representation of \a string as a QList<uint>.

    UTF-32 is a Unicode codec and therefore it is lossless. All characters from
    this string will be encoded in UTF-32. Any invalid sequence of code units in
    this string is replaced by the Unicode replacement character
    (QChar::ReplacementCharacter, which corresponds to \c{U+FFFD}).

    The returned list is not 0-terminated.

    \sa QString::toUcs4(), QStringView::toUcs4(), QtPrivate::convertToLatin1(),
    QtPrivate::convertToLocal8Bit(), QtPrivate::convertToUtf8()
*/
QList<uint> QtPrivate::convertToUcs4(QStringView string)
{
    return qt_convert_to_ucs4(string);
}

/*!
    \fn QString QString::fromLatin1(QByteArrayView str)
    \overload
    \since 6.0

    Returns a QString initialized with the Latin-1 string \a str.

    \note: any null ('\\0') bytes in the byte array will be included in this
    string, converted to Unicode null characters (U+0000).
*/
QString QString::fromLatin1(QByteArrayView ba)
{
    DataPointer d;
    if (!ba.data()) {
        // nothing to do
    } else if (ba.size() == 0) {
        d = DataPointer::fromRawData(&_empty, 0);
    } else {
        d = DataPointer(ba.size(), ba.size());
        Q_CHECK_PTR(d.data());
        d.data()[ba.size()] = '\0';
        char16_t *dst = d.data();

        qt_from_latin1(dst, ba.data(), size_t(ba.size()));
    }
    return QString(std::move(d));
}

/*!
    \fn QString QString::fromLatin1(const char *str, qsizetype size)
    Returns a QString initialized with the first \a size characters
    of the Latin-1 string \a str.

    If \a size is \c{-1}, \c{strlen(str)} is used instead.

    \sa toLatin1(), fromUtf8(), fromLocal8Bit()
*/

/*!
    \fn QString QString::fromLatin1(const QByteArray &str)
    \overload
    \since 5.0

    Returns a QString initialized with the Latin-1 string \a str.

    \note: any null ('\\0') bytes in the byte array will be included in this
    string, converted to Unicode null characters (U+0000). This behavior is
    different from Qt 5.x.
*/

/*!
    \fn QString QString::fromLocal8Bit(const char *str, qsizetype size)
    Returns a QString initialized with the first \a size characters
    of the 8-bit string \a str.

    If \a size is \c{-1}, \c{strlen(str)} is used instead.

    \include qstring.qdocinc {qstring-local-8-bit-equivalent} {fromUtf8}

    \sa toLocal8Bit(), fromLatin1(), fromUtf8()
*/

/*!
    \fn QString QString::fromLocal8Bit(const QByteArray &str)
    \overload
    \since 5.0

    Returns a QString initialized with the 8-bit string \a str.

    \include qstring.qdocinc {qstring-local-8-bit-equivalent} {fromUtf8}

    \note: any null ('\\0') bytes in the byte array will be included in this
    string, converted to Unicode null characters (U+0000). This behavior is
    different from Qt 5.x.
*/

/*!
    \fn QString QString::fromLocal8Bit(QByteArrayView str)
    \overload
    \since 6.0

    Returns a QString initialized with the 8-bit string \a str.

    \include qstring.qdocinc {qstring-local-8-bit-equivalent} {fromUtf8}

    \note: any null ('\\0') bytes in the byte array will be included in this
    string, converted to Unicode null characters (U+0000).
*/
QString QString::fromLocal8Bit(QByteArrayView ba)
{
    if (ba.isNull())
        return QString();
    if (ba.isEmpty())
        return QString(DataPointer::fromRawData(&_empty, 0));
    QStringDecoder toUtf16(QStringDecoder::System, QStringDecoder::Flag::Stateless);
    return toUtf16(ba);
}

/*! \fn QString QString::fromUtf8(const char *str, qsizetype size)
    Returns a QString initialized with the first \a size bytes
    of the UTF-8 string \a str.

    If \a size is \c{-1}, \c{strlen(str)} is used instead.

    UTF-8 is a Unicode codec and can represent all characters in a Unicode
    string like QString. However, invalid sequences are possible with UTF-8
    and, if any such are found, they will be replaced with one or more
    "replacement characters", or suppressed. These include non-Unicode
    sequences, non-characters, overlong sequences or surrogate codepoints
    encoded into UTF-8.

    This function can be used to process incoming data incrementally as long as
    all UTF-8 characters are terminated within the incoming data. Any
    unterminated characters at the end of the string will be replaced or
    suppressed. In order to do stateful decoding, please use \l QStringDecoder.

    \sa toUtf8(), fromLatin1(), fromLocal8Bit()
*/

/*!
    \fn QString QString::fromUtf8(const char8_t *str)
    \overload
    \since 6.1

    This overload is only available when compiling in C++20 mode.
*/

/*!
    \fn QString QString::fromUtf8(const char8_t *str, qsizetype size)
    \overload
    \since 6.0

    This overload is only available when compiling in C++20 mode.
*/

/*!
    \fn QString QString::fromUtf8(const QByteArray &str)
    \overload
    \since 5.0

    Returns a QString initialized with the UTF-8 string \a str.

    \note: any null ('\\0') bytes in the byte array will be included in this
    string, converted to Unicode null characters (U+0000). This behavior is
    different from Qt 5.x.
*/

/*!
    \fn QString QString::fromUtf8(QByteArrayView str)
    \overload
    \since 6.0

    Returns a QString initialized with the UTF-8 string \a str.

    \note: any null ('\\0') bytes in the byte array will be included in this
    string, converted to Unicode null characters (U+0000).
*/
QString QString::fromUtf8(QByteArrayView ba)
{
    if (ba.isNull())
        return QString();
    if (ba.isEmpty())
        return QString(DataPointer::fromRawData(&_empty, 0));
    return QUtf8::convertToUnicode(ba);
}

#ifndef QT_BOOTSTRAPPED
/*!
    \since 5.3
    Returns a QString initialized with the first \a size characters
    of the Unicode string \a unicode (ISO-10646-UTF-16 encoded).

    If \a size is -1 (default), \a unicode must be '\\0'-terminated.

    This function checks for a Byte Order Mark (BOM). If it is missing,
    host byte order is assumed.

    This function is slow compared to the other Unicode conversions.
    Use QString(const QChar *, qsizetype) or QString(const QChar *) if possible.

    QString makes a deep copy of the Unicode data.

    \sa utf16(), setUtf16(), fromStdU16String()
*/
QString QString::fromUtf16(const char16_t *unicode, qsizetype size)
{
    if (!unicode)
        return QString();
    if (size < 0)
        size = QtPrivate::qustrlen(unicode);
    QStringDecoder toUtf16(QStringDecoder::Utf16, QStringDecoder::Flag::Stateless);
    return toUtf16(QByteArrayView(reinterpret_cast<const char *>(unicode), size * 2));
}

/*!
    \fn QString QString::fromUtf16(const ushort *str, qsizetype size)
    \deprecated [6.0] Use the \c char16_t overload instead.
*/

/*!
    \fn QString QString::fromUcs4(const uint *str, qsizetype size)
    \since 4.2
    \deprecated [6.0] Use the \c char32_t overload instead.
*/

/*!
    \since 5.3

    Returns a QString initialized with the first \a size characters
    of the Unicode string \a unicode (encoded as UTF-32).

    If \a size is -1 (default), \a unicode must be '\\0'-terminated.

    \sa toUcs4(), fromUtf16(), utf16(), setUtf16(), fromWCharArray(),
        fromStdU32String()
*/
QString QString::fromUcs4(const char32_t *unicode, qsizetype size)
{
    if (!unicode)
        return QString();
    if (size < 0) {
        if constexpr (sizeof(char32_t) == sizeof(wchar_t))
            size = wcslen(reinterpret_cast<const wchar_t *>(unicode));
        else
            size = std::char_traits<char32_t>::length(unicode);
    }
    QStringDecoder toUtf16(QStringDecoder::Utf32, QStringDecoder::Flag::Stateless);
    return toUtf16(QByteArrayView(reinterpret_cast<const char *>(unicode), size * 4));
}
#endif // !QT_BOOTSTRAPPED

/*!
    Resizes the string to \a size characters and copies \a unicode
    into the string.

    If \a unicode is \nullptr, nothing is copied, but the string is still
    resized to \a size.

    \sa unicode(), setUtf16()
*/
QString& QString::setUnicode(const QChar *unicode, qsizetype size)
{
     resize(size);
     if (unicode && size)
         memcpy(d.data(), unicode, size * sizeof(QChar));
     return *this;
}

/*!
    \fn QString::setUnicode(const char16_t *unicode, qsizetype size)
    \overload
    \since 6.9

    \sa unicode(), setUtf16()
*/

/*!
    \fn QString::setUtf16(const char16_t *unicode, qsizetype size)
    \since 6.9

    Resizes the string to \a size characters and copies \a unicode
    into the string.

    If \a unicode is \nullptr, nothing is copied, but the string is still
    resized to \a size.

    Note that unlike fromUtf16(), this function does not consider BOMs and
    possibly differing byte ordering.

    \sa utf16(), setUnicode()
*/

/*!
    \fn QString &QString::setUtf16(const ushort *unicode, qsizetype size)
    \obsolete Use the \c char16_t overload instead.
*/

/*!
    \fn QString QString::simplified() const

    Returns a string that has whitespace removed from the start
    and the end, and that has each sequence of internal whitespace
    replaced with a single space.

    Whitespace means any character for which QChar::isSpace() returns
    \c true. This includes the ASCII characters '\\t', '\\n', '\\v',
    '\\f', '\\r', and ' '.

    Example:

    \snippet qstring/main.cpp 57

    \sa trimmed()
*/
QString QString::simplified_helper(const QString &str)
{
    return QStringAlgorithms<const QString>::simplified_helper(str);
}

QString QString::simplified_helper(QString &str)
{
    return QStringAlgorithms<QString>::simplified_helper(str);
}

namespace {
    template <typename StringView>
    StringView qt_trimmed(StringView s) noexcept
    {
        const auto [begin, end] = QStringAlgorithms<const StringView>::trimmed_helper_positions(s);
        return StringView{begin, end};
    }
}

/*!
    \fn QStringView QtPrivate::trimmed(QStringView s)
    \fn QLatin1StringView QtPrivate::trimmed(QLatin1StringView s)
    \internal
    \relates QStringView
    \since 5.10

    Returns \a s with whitespace removed from the start and the end.

    Whitespace means any character for which QChar::isSpace() returns
    \c true. This includes the ASCII characters '\\t', '\\n', '\\v',
    '\\f', '\\r', and ' '.

    \sa QString::trimmed(), QStringView::trimmed(), QLatin1StringView::trimmed()
*/
QStringView QtPrivate::trimmed(QStringView s) noexcept
{
    return qt_trimmed(s);
}

QLatin1StringView QtPrivate::trimmed(QLatin1StringView s) noexcept
{
    return qt_trimmed(s);
}

/*!
    \fn QString QString::trimmed() const

    Returns a string that has whitespace removed from the start and
    the end.

    Whitespace means any character for which QChar::isSpace() returns
    \c true. This includes the ASCII characters '\\t', '\\n', '\\v',
    '\\f', '\\r', and ' '.

    Example:

    \snippet qstring/main.cpp 82

    Unlike simplified(), trimmed() leaves internal whitespace alone.

    \sa simplified()
*/
QString QString::trimmed_helper(const QString &str)
{
    return QStringAlgorithms<const QString>::trimmed_helper(str);
}

QString QString::trimmed_helper(QString &str)
{
    return QStringAlgorithms<QString>::trimmed_helper(str);
}

/*! \fn const QChar QString::at(qsizetype position) const

    Returns the character at the given index \a position in the
    string.

    The \a position must be a valid index position in the string
    (i.e., 0 <= \a position < size()).

    \sa operator[]()
*/

/*!
    \fn QChar &QString::operator[](qsizetype position)

    Returns the character at the specified \a position in the string as a
    modifiable reference.

    Example:

    \snippet qstring/main.cpp 85

    \sa at()
*/

/*!
    \fn const QChar QString::operator[](qsizetype position) const

    \overload operator[]()
*/

/*!
    \fn QChar QString::front() const
    \since 5.10

    Returns the first character in the string.
    Same as \c{at(0)}.

    This function is provided for STL compatibility.

    \warning Calling this function on an empty string constitutes
    undefined behavior.

    \sa back(), at(), operator[]()
*/

/*!
    \fn QChar QString::back() const
    \since 5.10

    Returns the last character in the string.
    Same as \c{at(size() - 1)}.

    This function is provided for STL compatibility.

    \warning Calling this function on an empty string constitutes
    undefined behavior.

    \sa front(), at(), operator[]()
*/

/*!
    \fn QChar &QString::front()
    \since 5.10

    Returns a reference to the first character in the string.
    Same as \c{operator[](0)}.

    This function is provided for STL compatibility.

    \warning Calling this function on an empty string constitutes
    undefined behavior.

    \sa back(), at(), operator[]()
*/

/*!
    \fn QChar &QString::back()
    \since 5.10

    Returns a reference to the last character in the string.
    Same as \c{operator[](size() - 1)}.

    This function is provided for STL compatibility.

    \warning Calling this function on an empty string constitutes
    undefined behavior.

    \sa front(), at(), operator[]()
*/

/*!
    \fn void QString::truncate(qsizetype position)

    Truncates the string starting from, and including, the element at index
    \a position.

    If the specified \a position index is beyond the end of the
    string, nothing happens.

    Example:

    \snippet qstring/main.cpp 83

    If \a position is negative, it is equivalent to passing zero.

    \sa chop(), resize(), first(), QStringView::truncate()
*/

void QString::truncate(qsizetype pos)
{
    if (pos < size())
        resize(pos);
}


/*!
    Removes \a n characters from the end of the string.

    If \a n is greater than or equal to size(), the result is an
    empty string; if \a n is negative, it is equivalent to passing zero.

    Example:
    \snippet qstring/main.cpp 15

    If you want to remove characters from the \e beginning of the
    string, use remove() instead.

    \sa truncate(), resize(), remove(), QStringView::chop()
*/
void QString::chop(qsizetype n)
{
    if (n > 0)
        resize(d.size - n);
}

/*!
    Sets every character in the string to character \a ch. If \a size
    is different from -1 (default), the string is resized to \a
    size beforehand.

    Example:

    \snippet qstring/main.cpp 21

    \sa resize()
*/

QString& QString::fill(QChar ch, qsizetype size)
{
    resize(size < 0 ? d.size : size);
    if (d.size)
        std::fill(d.data(), d.data() + d.size, ch.unicode());
    return *this;
}

/*!
    \fn qsizetype QString::length() const

    Returns the number of characters in this string.  Equivalent to
    size().

    \sa resize()
*/

/*!
    \fn qsizetype QString::size() const

    Returns the number of characters in this string.

    The last character in the string is at position size() - 1.

    Example:
    \snippet qstring/main.cpp 58

    \sa isEmpty(), resize()
*/

/*!
    \fn qsizetype QString::max_size() const
    \fn qsizetype QString::maxSize()
    \since 6.8

    It returns the maximum number of elements that the string can
    theoretically hold. In practice, the number can be much smaller,
    limited by the amount of memory available to the system.
*/

/*! \fn bool QString::isNull() const

    Returns \c true if this string is null; otherwise returns \c false.

    Example:

    \snippet qstring/main.cpp 28

    Qt makes a distinction between null strings and empty strings for
    historical reasons. For most applications, what matters is
    whether or not a string contains any data, and this can be
    determined using the isEmpty() function.

    \sa isEmpty()
*/

/*! \fn bool QString::isEmpty() const

    Returns \c true if the string has no characters; otherwise returns
    \c false.

    Example:

    \snippet qstring/main.cpp 27

    \sa size()
*/

/*! \fn QString &QString::operator+=(const QString &other)

    Appends the string \a other onto the end of this string and
    returns a reference to this string.

    Example:

    \snippet qstring/main.cpp 84

    This operation is typically very fast (\l{constant time}),
    because QString preallocates extra space at the end of the string
    data so it can grow without reallocating the entire string each
    time.

    \sa append(), prepend()
*/

/*! \fn QString &QString::operator+=(QLatin1StringView str)

    \overload operator+=()

    Appends the Latin-1 string viewed by \a str to this string.
*/

/*! \fn QString &QString::operator+=(QUtf8StringView str)
    \since 6.5
    \overload operator+=()

    Appends the UTF-8 string view \a str to this string.
*/

/*! \fn QString &QString::operator+=(const QByteArray &ba)

    \overload operator+=()

    Appends the byte array \a ba to this string. The byte array is converted
    to Unicode using the fromUtf8() function. If any NUL characters ('\\0')
    are embedded in the \a ba byte array, they will be included in the
    transformation.

    You can disable this function by defining
    \l QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn QString &QString::operator+=(const char *str)

    \overload operator+=()

    Appends the string \a str to this string. The const char pointer
    is converted to Unicode using the fromUtf8() function.

    You can disable this function by defining \l QT_NO_CAST_FROM_ASCII
    when you compile your applications. This can be useful if you want
    to ensure that all user-visible strings go through QObject::tr(),
    for example.
*/

/*! \fn QString &QString::operator+=(QStringView str)
    \since 6.0
    \overload operator+=()

    Appends the string view \a str to this string.
*/

/*! \fn QString &QString::operator+=(QChar ch)

    \overload operator+=()

    Appends the character \a ch to the string.
*/

/*!
    \fn bool QString::operator==(const char * const &lhs, const QString &rhs)

    \overload operator==()

    Returns \c true if \a lhs is equal to \a rhs; otherwise returns \c false.
    Note that no string is equal to \a lhs being 0.

    Equivalent to \c {lhs != 0 && compare(lhs, rhs) == 0}.
*/

/*!
    \fn bool QString::operator!=(const char * const &lhs, const QString &rhs)

    Returns \c true if \a lhs is not equal to \a rhs; otherwise returns
    \c false.

    For \a lhs != 0, this is equivalent to \c {compare(} \a lhs, \a rhs
    \c {) != 0}. Note that no string is equal to \a lhs being 0.
*/

/*!
    \fn bool QString::operator<(const char * const &lhs, const QString &rhs)

    Returns \c true if \a lhs is lexically less than \a rhs; otherwise
    returns \c false.  For \a lhs != 0, this is equivalent to \c
    {compare(lhs, rhs) < 0}.

    \sa {Comparing Strings}
*/

/*!
    \fn bool QString::operator<=(const char * const &lhs, const QString &rhs)

    Returns \c true if \a lhs is lexically less than or equal to \a rhs;
    otherwise returns \c false.  For \a lhs != 0, this is equivalent to \c
    {compare(lhs, rhs) <= 0}.

    \sa {Comparing Strings}
*/

/*!
    \fn bool QString::operator>(const char * const &lhs, const QString &rhs)

    Returns \c true if \a lhs is lexically greater than \a rhs; otherwise
    returns \c false.  Equivalent to \c {compare(lhs, rhs) > 0}.

    \sa {Comparing Strings}
*/

/*!
    \fn bool QString::operator>=(const char * const &lhs, const QString &rhs)

    Returns \c true if \a lhs is lexically greater than or equal to \a rhs;
    otherwise returns \c false.  For \a lhs != 0, this is equivalent to \c
    {compare(lhs, rhs) >= 0}.

    \sa {Comparing Strings}
*/

/*!
    \fn QString operator+(const QString &s1, const QString &s2)
    \fn QString operator+(QString &&s1, const QString &s2)
    \relates QString

    Returns a string which is the result of concatenating \a s1 and \a
    s2.
*/

/*!
    \fn QString operator+(const QString &s1, const char *s2)
    \relates QString

    Returns a string which is the result of concatenating \a s1 and \a
    s2 (\a s2 is converted to Unicode using the QString::fromUtf8()
    function).

    \sa QString::fromUtf8()
*/

/*!
    \fn QString operator+(const char *s1, const QString &s2)
    \relates QString

    Returns a string which is the result of concatenating \a s1 and \a
    s2 (\a s1 is converted to Unicode using the QString::fromUtf8()
    function).

    \sa QString::fromUtf8()
*/

/*!
    \fn QString operator+(QStringView lhs, const QString &rhs)
    \fn QString operator+(const QString &lhs, QStringView rhs)

    \relates QString
    \since 6.9

    Returns a string that is the result of concatenating \a lhs and \a rhs.
*/

/*!
    \fn int QString::compare(const QString &s1, const QString &s2, Qt::CaseSensitivity cs)
    \since 4.2

    Compares the string \a s1 with the string \a s2 and returns a negative integer
    if \a s1 is less than \a s2, a positive integer if it is greater than \a s2,
    and zero if they are equal.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {comparison}

    Case sensitive comparison is based exclusively on the numeric
    Unicode values of the characters and is very fast, but is not what
    a human would expect.  Consider sorting user-visible strings with
    localeAwareCompare().

    \snippet qstring/main.cpp 16

//! [compare-isNull-vs-isEmpty]
    \note This function treats null strings the same as empty strings,
    for more details see \l {Distinction Between Null and Empty Strings}.
//! [compare-isNull-vs-isEmpty]

    \sa operator==(), operator<(), operator>(), {Comparing Strings}
*/

/*!
    \fn int QString::compare(const QString &s1, QLatin1StringView s2, Qt::CaseSensitivity cs)
    \since 4.2
    \overload compare()

    Performs a comparison of \a s1 and \a s2, using the case
    sensitivity setting \a cs.
*/

/*!
    \fn int QString::compare(QLatin1StringView s1, const QString &s2, Qt::CaseSensitivity cs = Qt::CaseSensitive)

    \since 4.2
    \overload compare()

    Performs a comparison of \a s1 and \a s2, using the case
    sensitivity setting \a cs.
*/

/*!
    \fn int QString::compare(QStringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const

    \since 5.12
    \overload compare()

    Performs a comparison of this with \a s, using the case
    sensitivity setting \a cs.
*/

/*!
    \fn int QString::compare(QChar ch, Qt::CaseSensitivity cs = Qt::CaseSensitive) const

    \since 5.14
    \overload compare()

    Performs a comparison of this with \a ch, using the case
    sensitivity setting \a cs.
*/

/*!
    \overload compare()
    \since 4.2

    Lexically compares this string with the string \a other and returns
    a negative integer if this string is less than \a other, a positive
    integer if it is greater than \a other, and zero if they are equal.

    Same as compare(*this, \a other, \a cs).
*/
int QString::compare(const QString &other, Qt::CaseSensitivity cs) const noexcept
{
    return QtPrivate::compareStrings(*this, other, cs);
}

/*!
    \internal
    \since 4.5
*/
int QString::compare_helper(const QChar *data1, qsizetype length1, const QChar *data2, qsizetype length2,
                            Qt::CaseSensitivity cs) noexcept
{
    Q_ASSERT(length1 >= 0);
    Q_ASSERT(length2 >= 0);
    Q_ASSERT(data1 || length1 == 0);
    Q_ASSERT(data2 || length2 == 0);
    return QtPrivate::compareStrings(QStringView(data1, length1), QStringView(data2, length2), cs);
}

/*!
    \overload compare()
    \since 4.2

    Same as compare(*this, \a other, \a cs).
*/
int QString::compare(QLatin1StringView other, Qt::CaseSensitivity cs) const noexcept
{
    return QtPrivate::compareStrings(*this, other, cs);
}

/*!
    \internal
    \since 5.0
*/
int QString::compare_helper(const QChar *data1, qsizetype length1, const char *data2, qsizetype length2,
                            Qt::CaseSensitivity cs) noexcept
{
    Q_ASSERT(length1 >= 0);
    Q_ASSERT(data1 || length1 == 0);
    if (!data2)
        return qt_lencmp(length1, 0);
    if (Q_UNLIKELY(length2 < 0))
        length2 = qsizetype(strlen(data2));
    return QtPrivate::compareStrings(QStringView(data1, length1),
                                     QUtf8StringView(data2, length2), cs);
}

/*!
  \fn int QString::compare(const QString &s1, QStringView s2, Qt::CaseSensitivity cs = Qt::CaseSensitive)
  \overload compare()
*/

/*!
  \fn int QString::compare(QStringView s1, const QString &s2, Qt::CaseSensitivity cs = Qt::CaseSensitive)
  \overload compare()
*/

bool comparesEqual(const QByteArrayView &lhs, const QChar &rhs) noexcept
{
    return QtPrivate::equalStrings(QUtf8StringView(lhs), QStringView(&rhs, 1));
}

Qt::strong_ordering compareThreeWay(const QByteArrayView &lhs, const QChar &rhs) noexcept
{
    const int res = QtPrivate::compareStrings(QUtf8StringView(lhs), QStringView(&rhs, 1));
    return Qt::compareThreeWay(res, 0);
}

bool comparesEqual(const QByteArrayView &lhs, char16_t rhs) noexcept
{
    return QtPrivate::equalStrings(QUtf8StringView(lhs), QStringView(&rhs, 1));
}

Qt::strong_ordering compareThreeWay(const QByteArrayView &lhs, char16_t rhs) noexcept
{
    const int res = QtPrivate::compareStrings(QUtf8StringView(lhs), QStringView(&rhs, 1));
    return Qt::compareThreeWay(res, 0);
}

bool comparesEqual(const QByteArray &lhs, const QChar &rhs) noexcept
{
    return QtPrivate::equalStrings(QUtf8StringView(lhs), QStringView(&rhs, 1));
}

Qt::strong_ordering compareThreeWay(const QByteArray &lhs, const QChar &rhs) noexcept
{
    const int res = QtPrivate::compareStrings(QUtf8StringView(lhs), QStringView(&rhs, 1));
    return Qt::compareThreeWay(res, 0);
}

bool comparesEqual(const QByteArray &lhs, char16_t rhs) noexcept
{
    return QtPrivate::equalStrings(QUtf8StringView(lhs), QStringView(&rhs, 1));
}

Qt::strong_ordering compareThreeWay(const QByteArray &lhs, char16_t rhs) noexcept
{
    const int res = QtPrivate::compareStrings(QUtf8StringView(lhs), QStringView(&rhs, 1));
    return Qt::compareThreeWay(res, 0);
}

/*!
    \internal
    \since 6.8
*/
bool QT_FASTCALL QChar::equal_helper(QChar lhs, const char *rhs) noexcept
{
    return QtPrivate::equalStrings(QStringView(&lhs, 1), QUtf8StringView(rhs));
}

int QT_FASTCALL QChar::compare_helper(QChar lhs, const char *rhs) noexcept
{
    return QtPrivate::compareStrings(QStringView(&lhs, 1), QUtf8StringView(rhs));
}

/*!
    \internal
    \since 6.8
*/
bool QStringView::equal_helper(QStringView sv, const char *data, qsizetype len)
{
    Q_ASSERT(len >= 0);
    Q_ASSERT(data || len == 0);
    return QtPrivate::equalStrings(sv, QUtf8StringView(data, len));
}

/*!
    \internal
    \since 6.8
*/
int QStringView::compare_helper(QStringView sv, const char *data, qsizetype len)
{
    Q_ASSERT(len >= 0);
    Q_ASSERT(data || len == 0);
    return QtPrivate::compareStrings(sv, QUtf8StringView(data, len));
}

/*!
    \internal
    \since 6.8
*/
bool QLatin1StringView::equal_helper(QLatin1StringView s1, const char *s2, qsizetype len) noexcept
{
    // because qlatin1stringview.h can't include qutf8stringview.h
    Q_ASSERT(len >= 0);
    Q_ASSERT(s2 || len == 0);
    return QtPrivate::equalStrings(s1, QUtf8StringView(s2, len));
}

/*!
    \internal
    \since 6.6
*/
int QLatin1StringView::compare_helper(const QLatin1StringView &s1, const char *s2, qsizetype len) noexcept
{
    // because qlatin1stringview.h can't include qutf8stringview.h
    Q_ASSERT(len >= 0);
    Q_ASSERT(s2 || len == 0);
    return QtPrivate::compareStrings(s1, QUtf8StringView(s2, len));
}

/*!
    \internal
    \since 4.5
*/
int QLatin1StringView::compare_helper(const QChar *data1, qsizetype length1, QLatin1StringView s2,
                                      Qt::CaseSensitivity cs) noexcept
{
    Q_ASSERT(length1 >= 0);
    Q_ASSERT(data1 || length1 == 0);
    return QtPrivate::compareStrings(QStringView(data1, length1), s2, cs);
}

/*!
    \fn int QString::localeAwareCompare(const QString & s1, const QString & s2)

    Compares \a s1 with \a s2 and returns an integer less than, equal
    to, or greater than zero if \a s1 is less than, equal to, or
    greater than \a s2.

    The comparison is performed in a locale- and also
    platform-dependent manner. Use this function to present sorted
    lists of strings to the user.

    \sa compare(), QLocale, {Comparing Strings}
*/

/*!
    \fn int QString::localeAwareCompare(QStringView other) const
    \since 6.0
    \overload localeAwareCompare()

    Compares this string with the \a other string and returns an
    integer less than, equal to, or greater than zero if this string
    is less than, equal to, or greater than the \a other string.

    The comparison is performed in a locale- and also
    platform-dependent manner. Use this function to present sorted
    lists of strings to the user.

    Same as \c {localeAwareCompare(*this, other)}.

    \sa {Comparing Strings}
*/

/*!
    \fn int QString::localeAwareCompare(QStringView s1, QStringView s2)
    \since 6.0
    \overload localeAwareCompare()

    Compares \a s1 with \a s2 and returns an integer less than, equal
    to, or greater than zero if \a s1 is less than, equal to, or
    greater than \a s2.

    The comparison is performed in a locale- and also
    platform-dependent manner. Use this function to present sorted
    lists of strings to the user.

    \sa {Comparing Strings}
*/


#if !defined(CSTR_LESS_THAN)
#define CSTR_LESS_THAN    1
#define CSTR_EQUAL        2
#define CSTR_GREATER_THAN 3
#endif

/*!
    \overload localeAwareCompare()

    Compares this string with the \a other string and returns an
    integer less than, equal to, or greater than zero if this string
    is less than, equal to, or greater than the \a other string.

    The comparison is performed in a locale- and also
    platform-dependent manner. Use this function to present sorted
    lists of strings to the user.

    Same as \c {localeAwareCompare(*this, other)}.

    \sa {Comparing Strings}
*/
int QString::localeAwareCompare(const QString &other) const
{
    return localeAwareCompare_helper(constData(), size(), other.constData(), other.size());
}

/*!
    \internal
    \since 4.5
*/
int QString::localeAwareCompare_helper(const QChar *data1, qsizetype length1,
                                       const QChar *data2, qsizetype length2)
{
    Q_ASSERT(length1 >= 0);
    Q_ASSERT(data1 || length1 == 0);
    Q_ASSERT(length2 >= 0);
    Q_ASSERT(data2 || length2 == 0);

    // do the right thing for null and empty
    if (length1 == 0 || length2 == 0)
        return QtPrivate::compareStrings(QStringView(data1, length1), QStringView(data2, length2),
                               Qt::CaseSensitive);

#if QT_CONFIG(icu)
    return QCollator::defaultCompare(QStringView(data1, length1), QStringView(data2, length2));
#else
    const QString lhs = QString::fromRawData(data1, length1).normalized(QString::NormalizationForm_C);
    const QString rhs = QString::fromRawData(data2, length2).normalized(QString::NormalizationForm_C);
#  if defined(Q_OS_WIN)
    int res = CompareStringEx(LOCALE_NAME_USER_DEFAULT, 0, (LPWSTR)lhs.constData(), lhs.length(), (LPWSTR)rhs.constData(), rhs.length(), NULL, NULL, 0);

    switch (res) {
    case CSTR_LESS_THAN:
        return -1;
    case CSTR_GREATER_THAN:
        return 1;
    default:
        return 0;
    }
#  elif defined (Q_OS_DARWIN)
    // Use CFStringCompare for comparing strings on Mac. This makes Qt order
    // strings the same way as native applications do, and also respects
    // the "Order for sorted lists" setting in the International preferences
    // panel.
    const CFStringRef thisString =
        CFStringCreateWithCharactersNoCopy(kCFAllocatorDefault,
            reinterpret_cast<const UniChar *>(lhs.constData()), lhs.length(), kCFAllocatorNull);
    const CFStringRef otherString =
        CFStringCreateWithCharactersNoCopy(kCFAllocatorDefault,
            reinterpret_cast<const UniChar *>(rhs.constData()), rhs.length(), kCFAllocatorNull);

    const int result = CFStringCompare(thisString, otherString, kCFCompareLocalized);
    CFRelease(thisString);
    CFRelease(otherString);
    return result;
#  elif defined(Q_OS_UNIX)
    // declared in <string.h> (no better than QtPrivate::compareStrings() on Android, sadly)
    return strcoll(lhs.toLocal8Bit().constData(), rhs.toLocal8Bit().constData());
#  else
#     error "This case shouldn't happen"
    return QtPrivate::compareStrings(lhs, rhs, Qt::CaseSensitive);
#  endif
#endif // !QT_CONFIG(icu)
}


/*!
    \fn const QChar *QString::unicode() const

    Returns a Unicode representation of the string.
    The result remains valid until the string is modified.

    \note The returned string may not be '\\0'-terminated.
    Use size() to determine the length of the array.

    \sa utf16(), fromRawData()
*/

/*!
    \fn const ushort *QString::utf16() const

    Returns the QString as a '\\0\'-terminated array of unsigned
    shorts. The result remains valid until the string is modified.

    The returned string is in host byte order.

    \sa unicode()
*/

const ushort *QString::utf16() const
{
    if (!d->isMutable()) {
        // ensure '\0'-termination for ::fromRawData strings
        const_cast<QString*>(this)->reallocData(d.size, QArrayData::KeepSize);
    }
    return reinterpret_cast<const ushort *>(d.data());
}

/*!
    \fn QString &QString::nullTerminate()
    \since 6.10

    If this string data isn't null-terminated, this method will make a deep
    copy of the data and make it null-terminated.

    A QString is null-terminated by default, however in some cases (e.g.
    when using fromRawData()), the string data doesn't necessarily end
    with a \c {\0} character, which could be a problem when calling methods
    that expect a null-terminated string.

    \sa nullTerminated(), fromRawData(), setRawData()
*/
QString &QString::nullTerminate()
{
    // ensure '\0'-termination for ::fromRawData strings
    if (!d->isMutable())
        *this = QString{constData(), size()};
    return *this;
}

/*!
    \fn QString QString::nullTerminated() const &
    \fn QString QString::nullTerminated() &&
    \since 6.10

    Returns a copy of this string that is always null-terminated.

    \sa nullTerminate(), fromRawData(), setRawData()
*/
QString QString::nullTerminated() const &
{
    // ensure '\0'-termination for ::fromRawData strings
    if (!d->isMutable())
        return QString{constData(), size()};
    return *this;
}

QString QString::nullTerminated() &&
{
    nullTerminate();
    return std::move(*this);
}

/*!
    Returns a string of size \a width that contains this string
    padded by the \a fill character.

    If \a truncate is \c false and the size() of the string is more than
    \a width, then the returned string is a copy of the string.

    \snippet qstring/main.cpp 32

    If \a truncate is \c true and the size() of the string is more than
    \a width, then any characters in a copy of the string after
    position \a width are removed, and the copy is returned.

    \snippet qstring/main.cpp 33

    \sa rightJustified()
*/

QString QString::leftJustified(qsizetype width, QChar fill, bool truncate) const
{
    QString result;
    qsizetype len = size();
    qsizetype padlen = width - len;
    if (padlen > 0) {
        result.resize(len+padlen);
        if (len)
            memcpy(result.d.data(), d.data(), sizeof(QChar)*len);
        QChar *uc = (QChar*)result.d.data() + len;
        while (padlen--)
           * uc++ = fill;
    } else {
        if (truncate)
            result = left(width);
        else
            result = *this;
    }
    return result;
}

/*!
    Returns a string of size() \a width that contains the \a fill
    character followed by the string. For example:

    \snippet qstring/main.cpp 49

    If \a truncate is \c false and the size() of the string is more than
    \a width, then the returned string is a copy of the string.

    If \a truncate is true and the size() of the string is more than
    \a width, then the resulting string is truncated at position \a
    width.

    \snippet qstring/main.cpp 50

    \sa leftJustified()
*/

QString QString::rightJustified(qsizetype width, QChar fill, bool truncate) const
{
    QString result;
    qsizetype len = size();
    qsizetype padlen = width - len;
    if (padlen > 0) {
        result.resize(len+padlen);
        QChar *uc = (QChar*)result.d.data();
        while (padlen--)
           * uc++ = fill;
        if (len)
          memcpy(static_cast<void *>(uc), static_cast<const void *>(d.data()), sizeof(QChar)*len);
    } else {
        if (truncate)
            result = left(width);
        else
            result = *this;
    }
    return result;
}

/*!
    \fn QString QString::toLower() const

    Returns a lowercase copy of the string.

    \snippet qstring/main.cpp 75

    The case conversion will always happen in the 'C' locale. For
    locale-dependent case folding use QLocale::toLower()

    \sa toUpper(), QLocale::toLower()
*/

namespace QUnicodeTables {
/*
    \internal
    Converts the \a str string starting from the position pointed to by the \a
    it iterator, using the Unicode case traits \c Traits, and returns the
    result. The input string must not be empty (the convertCase function below
    guarantees that).

    The string type \c{T} is also a template and is either \c{const QString} or
    \c{QString}. This function can do both copy-conversion and in-place
    conversion depending on the state of the \a str parameter:
    \list
       \li \c{T} is \c{const QString}: copy-convert
       \li \c{T} is \c{QString} and its refcount != 1: copy-convert
       \li \c{T} is \c{QString} and its refcount == 1: in-place convert
    \endlist

    In copy-convert mode, the local variable \c{s} is detached from the input
    \a str. In the in-place convert mode, \a str is in moved-from state and
    \c{s} contains the only copy of the string, without reallocation (thus,
    \a it is still valid).

    There is one pathological case left: when the in-place conversion needs to
    reallocate memory to grow the buffer. In that case, we need to adjust the \a
    it pointer.
 */
template <typename T>
Q_NEVER_INLINE
static QString detachAndConvertCase(T &str, QStringIterator it, QUnicodeTables::Case which)
{
    Q_ASSERT(!str.isEmpty());
    QString s = std::move(str);         // will copy if T is const QString
    QChar *pp = s.begin() + it.index(); // will detach if necessary

    do {
        const auto folded = fullConvertCase(it.next(), which);
        if (Q_UNLIKELY(folded.size() > 1)) {
            if (folded.chars[0] == *pp && folded.size() == 2) {
                // special case: only second actually changed (e.g. surrogate pairs),
                // avoid slow case
                ++pp;
                *pp++ = folded.chars[1];
            } else {
                // slow path: the string is growing
                qsizetype inpos = it.index() - 1;
                qsizetype outpos = pp - s.constBegin();

                s.replace(outpos, 1, reinterpret_cast<const QChar *>(folded.data()), folded.size());
                pp = const_cast<QChar *>(s.constBegin()) + outpos + folded.size();

                // Adjust the input iterator if we are performing an in-place conversion
                if constexpr (!std::is_const<T>::value)
                    it = QStringIterator(s.constBegin(), inpos + folded.size(), s.constEnd());
            }
        } else {
            *pp++ = folded.chars[0];
        }
    } while (it.hasNext());

    return s;
}

template <typename T>
static QString convertCase(T &str, QUnicodeTables::Case which)
{
    const QChar *p = str.constBegin();
    const QChar *e = p + str.size();

    // this avoids out of bounds check in the loop
    while (e != p && e[-1].isHighSurrogate())
        --e;

    QStringIterator it(p, e);
    while (it.hasNext()) {
        const char32_t uc = it.next();
        if (qGetProp(uc)->cases[which].diff) {
            it.recede();
            return detachAndConvertCase(str, it, which);
        }
    }
    return std::move(str);
}
} // namespace QUnicodeTables

QString QString::toLower_helper(const QString &str)
{
    return QUnicodeTables::convertCase(str, QUnicodeTables::LowerCase);
}

QString QString::toLower_helper(QString &str)
{
    return QUnicodeTables::convertCase(str, QUnicodeTables::LowerCase);
}

/*!
    \fn QString QString::toCaseFolded() const

    Returns the case folded equivalent of the string. For most Unicode
    characters this is the same as toLower().
*/

QString QString::toCaseFolded_helper(const QString &str)
{
    return QUnicodeTables::convertCase(str, QUnicodeTables::CaseFold);
}

QString QString::toCaseFolded_helper(QString &str)
{
    return QUnicodeTables::convertCase(str, QUnicodeTables::CaseFold);
}

/*!
    \fn QString QString::toUpper() const

    Returns an uppercase copy of the string.

    \snippet qstring/main.cpp 81

    The case conversion will always happen in the 'C' locale. For
    locale-dependent case folding use QLocale::toUpper().

    \note In some cases the uppercase form of a string may be longer than the
    original.

    \sa toLower(), QLocale::toLower()
*/

QString QString::toUpper_helper(const QString &str)
{
    return QUnicodeTables::convertCase(str, QUnicodeTables::UpperCase);
}

QString QString::toUpper_helper(QString &str)
{
    return QUnicodeTables::convertCase(str, QUnicodeTables::UpperCase);
}

/*!
    \since 5.5

    Safely builds a formatted string from the format string \a cformat
    and an arbitrary list of arguments.

    The format string supports the conversion specifiers, length modifiers,
    and flags provided by printf() in the standard C++ library. The \a cformat
    string and \c{%s} arguments must be UTF-8 encoded.

    \note The \c{%lc} escape sequence expects a unicode character of type
    \c char16_t, or \c ushort (as returned by QChar::unicode()).
    The \c{%ls} escape sequence expects a pointer to a zero-terminated array
    of unicode characters of type \c char16_t, or ushort (as returned by
    QString::utf16()). This is at odds with the printf() in the standard C++
    library, which defines \c {%lc} to print a wchar_t and \c{%ls} to print
    a \c{wchar_t*}, and might also produce compiler warnings on platforms
    where the size of \c {wchar_t} is not 16 bits.

    \warning We do not recommend using QString::asprintf() in new Qt
    code. Instead, consider using QTextStream or arg(), both of
    which support Unicode strings seamlessly and are type-safe.
    Here is an example that uses QTextStream:

    \snippet qstring/main.cpp 64

    For \l {QObject::tr()}{translations}, especially if the strings
    contains more than one escape sequence, you should consider using
    the arg() function instead. This allows the order of the
    replacements to be controlled by the translator.

    \sa arg()
*/

QString QString::asprintf(const char *cformat, ...)
{
    va_list ap;
    va_start(ap, cformat);
    QString s = vasprintf(cformat, ap);
    va_end(ap);
    return s;
}

static void append_utf8(QString &qs, const char *cs, qsizetype len)
{
    const qsizetype oldSize = qs.size();
    qs.resize(oldSize + len);
    const QChar *newEnd = QUtf8::convertToUnicode(qs.data() + oldSize, QByteArrayView(cs, len));
    qs.resize(newEnd - qs.constData());
}

static uint parse_flag_characters(const char * &c) noexcept
{
    uint flags = QLocaleData::ZeroPadExponent;
    while (true) {
        switch (*c) {
        case '#':
            flags |= QLocaleData::ShowBase | QLocaleData::AddTrailingZeroes
                    | QLocaleData::ForcePoint;
            break;
        case '0': flags |= QLocaleData::ZeroPadded; break;
        case '-': flags |= QLocaleData::LeftAdjusted; break;
        case ' ': flags |= QLocaleData::BlankBeforePositive; break;
        case '+': flags |= QLocaleData::AlwaysShowSign; break;
        case '\'': flags |= QLocaleData::GroupDigits; break;
        default: return flags;
        }
        ++c;
    }
}

static int parse_field_width(const char *&c, qsizetype size)
{
    Q_ASSERT(isAsciiDigit(*c));
    const char *const stop = c + size;

    // can't be negative - started with a digit
    // contains at least one digit
    auto [result, used] = qstrntoull(c, size, 10);
    c += used;
    if (used <= 0)
        return false;
    // preserve Qt 5.5 behavior of consuming all digits, no matter how many
    while (c < stop && isAsciiDigit(*c))
        ++c;
    return result < qulonglong(std::numeric_limits<int>::max()) ? int(result) : 0;
}

enum LengthMod { lm_none, lm_hh, lm_h, lm_l, lm_ll, lm_L, lm_j, lm_z, lm_t };

static inline bool can_consume(const char * &c, char ch) noexcept
{
    if (*c == ch) {
        ++c;
        return true;
    }
    return false;
}

static LengthMod parse_length_modifier(const char * &c) noexcept
{
    switch (*c++) {
    case 'h': return can_consume(c, 'h') ? lm_hh : lm_h;
    case 'l': return can_consume(c, 'l') ? lm_ll : lm_l;
    case 'L': return lm_L;
    case 'j': return lm_j;
    case 'z':
    case 'Z': return lm_z;
    case 't': return lm_t;
    }
    --c; // don't consume *c - it wasn't a flag
    return lm_none;
}

/*!
    \fn QString QString::vasprintf(const char *cformat, va_list ap)
    \since 5.5

    Equivalent method to asprintf(), but takes a va_list \a ap
    instead a list of variable arguments. See the asprintf()
    documentation for an explanation of \a cformat.

    This method does not call the va_end macro, the caller
    is responsible to call va_end on \a ap.

    \sa asprintf()
*/

QString QString::vasprintf(const char *cformat, va_list ap)
{
    if (!cformat || !*cformat) {
        // Qt 1.x compat
        return fromLatin1("");
    }

    // Parse cformat

    QString result;
    const char *c = cformat;
    const char *formatEnd = cformat + qstrlen(cformat);
    for (;;) {
        // Copy non-escape chars to result
        const char *cb = c;
        while (*c != '\0' && *c != '%')
            c++;
        append_utf8(result, cb, qsizetype(c - cb));

        if (*c == '\0')
            break;

        // Found '%'
        const char *escape_start = c;
        ++c;

        if (*c == '\0') {
            result.append(u'%'); // a % at the end of the string - treat as non-escape text
            break;
        }
        if (*c == '%') {
            result.append(u'%'); // %%
            ++c;
            continue;
        }

        uint flags = parse_flag_characters(c);

        if (*c == '\0') {
            result.append(QLatin1StringView(escape_start)); // incomplete escape, treat as non-escape text
            break;
        }

        // Parse field width
        int width = -1; // -1 means unspecified
        if (isAsciiDigit(*c)) {
            width = parse_field_width(c, formatEnd - c);
        } else if (*c == '*') { // can't parse this in another function, not portably, at least
            width = va_arg(ap, int);
            if (width < 0)
                width = -1; // treat all negative numbers as unspecified
            ++c;
        }

        if (*c == '\0') {
            result.append(QLatin1StringView(escape_start)); // incomplete escape, treat as non-escape text
            break;
        }

        // Parse precision
        int precision = -1; // -1 means unspecified
        if (*c == '.') {
            ++c;
            precision = 0;
            if (isAsciiDigit(*c)) {
                precision = parse_field_width(c, formatEnd - c);
            } else if (*c == '*') { // can't parse this in another function, not portably, at least
                precision = va_arg(ap, int);
                if (precision < 0)
                    precision = -1; // treat all negative numbers as unspecified
                ++c;
            }
        }

        if (*c == '\0') {
            result.append(QLatin1StringView(escape_start)); // incomplete escape, treat as non-escape text
            break;
        }

        const LengthMod length_mod = parse_length_modifier(c);

        if (*c == '\0') {
            result.append(QLatin1StringView(escape_start)); // incomplete escape, treat as non-escape text
            break;
        }

        // Parse the conversion specifier and do the conversion
        QString subst;
        switch (*c) {
            case 'd':
            case 'i': {
                qint64 i;
                switch (length_mod) {
                    case lm_none: i = va_arg(ap, int); break;
                    case lm_hh: i = va_arg(ap, int); break;
                    case lm_h: i = va_arg(ap, int); break;
                    case lm_l: i = va_arg(ap, long int); break;
                    case lm_ll: i = va_arg(ap, qint64); break;
                    case lm_j: i = va_arg(ap, long int); break;

                    /* ptrdiff_t actually, but it should be the same for us */
                    case lm_z: i = va_arg(ap, qsizetype); break;
                    case lm_t: i = va_arg(ap, qsizetype); break;
                    default: i = 0; break;
                }
                subst = QLocaleData::c()->longLongToString(i, precision, 10, width, flags);
                ++c;
                break;
            }
            case 'o':
            case 'u':
            case 'x':
            case 'X': {
                quint64 u;
                switch (length_mod) {
                    case lm_none: u = va_arg(ap, uint); break;
                    case lm_hh: u = va_arg(ap, uint); break;
                    case lm_h: u = va_arg(ap, uint); break;
                    case lm_l: u = va_arg(ap, ulong); break;
                    case lm_ll: u = va_arg(ap, quint64); break;
                    case lm_t: u = va_arg(ap, size_t); break;
                    case lm_z: u = va_arg(ap, size_t); break;
                    default: u = 0; break;
                }

                if (isAsciiUpper(*c))
                    flags |= QLocaleData::CapitalEorX;

                int base = 10;
                switch (QtMiscUtils::toAsciiLower(*c)) {
                    case 'o':
                        base = 8; break;
                    case 'u':
                        base = 10; break;
                    case 'x':
                        base = 16; break;
                    default: break;
                }
                subst = QLocaleData::c()->unsLongLongToString(u, precision, base, width, flags);
                ++c;
                break;
            }
            case 'E':
            case 'e':
            case 'F':
            case 'f':
            case 'G':
            case 'g':
            case 'A':
            case 'a': {
                double d;
                if (length_mod == lm_L)
                    d = va_arg(ap, long double); // not supported - converted to a double
                else
                    d = va_arg(ap, double);

                if (isAsciiUpper(*c))
                    flags |= QLocaleData::CapitalEorX;

                QLocaleData::DoubleForm form = QLocaleData::DFDecimal;
                switch (QtMiscUtils::toAsciiLower(*c)) {
                    case 'e': form = QLocaleData::DFExponent; break;
                    case 'a':                             // not supported - decimal form used instead
                    case 'f': form = QLocaleData::DFDecimal; break;
                    case 'g': form = QLocaleData::DFSignificantDigits; break;
                    default: break;
                }
                subst = QLocaleData::c()->doubleToString(d, precision, form, width, flags);
                ++c;
                break;
            }
            case 'c': {
                if (length_mod == lm_l)
                    subst = QChar::fromUcs2(va_arg(ap, int));
                else
                    subst = QLatin1Char((uchar) va_arg(ap, int));
                ++c;
                break;
            }
            case 's': {
                if (length_mod == lm_l) {
                    const char16_t *buff = va_arg(ap, const char16_t*);
                    const auto *ch = buff;
                    while (precision != 0 && *ch != 0) {
                        ++ch;
                        --precision;
                    }
                    subst.setUtf16(buff, ch - buff);
                } else if (precision == -1) {
                    subst = QString::fromUtf8(va_arg(ap, const char*));
                } else {
                    const char *buff = va_arg(ap, const char*);
                    subst = QString::fromUtf8(buff, qstrnlen(buff, precision));
                }
                ++c;
                break;
            }
            case 'p': {
                void *arg = va_arg(ap, void*);
                const quint64 i = reinterpret_cast<quintptr>(arg);
                flags |= QLocaleData::ShowBase;
                subst = QLocaleData::c()->unsLongLongToString(i, precision, 16, width, flags);
                ++c;
                break;
            }
            case 'n':
                switch (length_mod) {
                    case lm_hh: {
                        signed char *n = va_arg(ap, signed char*);
                        *n = result.size();
                        break;
                    }
                    case lm_h: {
                        short int *n = va_arg(ap, short int*);
                        *n = result.size();
                            break;
                    }
                    case lm_l: {
                        long int *n = va_arg(ap, long int*);
                        *n = result.size();
                        break;
                    }
                    case lm_ll: {
                        qint64 *n = va_arg(ap, qint64*);
                        *n = result.size();
                        break;
                    }
                    default: {
                        int *n = va_arg(ap, int*);
                        *n = int(result.size());
                        break;
                    }
                }
                ++c;
                break;

            default: // bad escape, treat as non-escape text
                for (const char *cc = escape_start; cc != c; ++cc)
                    result.append(QLatin1Char(*cc));
                continue;
        }

        if (flags & QLocaleData::LeftAdjusted)
            result.append(subst.leftJustified(width));
        else
            result.append(subst.rightJustified(width));
    }

    return result;
}

/*!
    \fn QString::toLongLong(bool *ok, int base) const

    Returns the string converted to a \c{long long} using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    If \a base is 0, the C language convention is used: if the string begins
    with "0x", base 16 is used; otherwise, if the string begins with "0b", base
    2 is used; otherwise, if the string begins with "0", base 8 is used;
    otherwise, base 10 is used.

    The string conversion will always happen in the 'C' locale. For
    locale-dependent conversion use QLocale::toLongLong()

    Example:

    \snippet qstring/main.cpp 74

    This function ignores leading and trailing whitespace.

    \note Support for the "0b" prefix was added in Qt 6.4.

    \sa number(), toULongLong(), toInt(), QLocale::toLongLong()
*/

template <typename Int>
static Int toIntegral(QStringView string, bool *ok, int base)
{
#if defined(QT_CHECK_RANGE)
    if (base != 0 && (base < 2 || base > 36)) {
        qWarning("QString::toIntegral: Invalid base (%d)", base);
        base = 10;
    }
#endif

    QVarLengthArray<uchar> latin1(string.size());
    qt_to_latin1(latin1.data(), string.utf16(), string.size());
    QSimpleParsedNumber<Int> r;
    if constexpr (std::is_signed_v<Int>)
        r = QLocaleData::bytearrayToLongLong(latin1, base);
    else
        r = QLocaleData::bytearrayToUnsLongLong(latin1, base);
    if (ok)
        *ok = r.ok();
    return r.result;
}

qlonglong QString::toIntegral_helper(QStringView string, bool *ok, int base)
{
    return toIntegral<qlonglong>(string, ok, base);
}

/*!
    \fn QString::toULongLong(bool *ok, int base) const

    Returns the string converted to an \c{unsigned long long} using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    If \a base is 0, the C language convention is used: if the string begins
    with "0x", base 16 is used; otherwise, if the string begins with "0b", base
    2 is used; otherwise, if the string begins with "0", base 8 is used;
    otherwise, base 10 is used.

    The string conversion will always happen in the 'C' locale. For
    locale-dependent conversion use QLocale::toULongLong()

    Example:

    \snippet qstring/main.cpp 79

    This function ignores leading and trailing whitespace.

    \note Support for the "0b" prefix was added in Qt 6.4.

    \sa number(), toLongLong(), QLocale::toULongLong()
*/

qulonglong QString::toIntegral_helper(QStringView string, bool *ok, uint base)
{
    return toIntegral<qulonglong>(string, ok, base);
}

/*!
    \fn long QString::toLong(bool *ok, int base) const

    Returns the string converted to a \c long using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    If \a base is 0, the C language convention is used: if the string begins
    with "0x", base 16 is used; otherwise, if the string begins with "0b", base
    2 is used; otherwise, if the string begins with "0", base 8 is used;
    otherwise, base 10 is used.

    The string conversion will always happen in the 'C' locale. For
    locale-dependent conversion use QLocale::toLongLong()

    Example:

    \snippet qstring/main.cpp 73

    This function ignores leading and trailing whitespace.

    \note Support for the "0b" prefix was added in Qt 6.4.

    \sa number(), toULong(), toInt(), QLocale::toInt()
*/

/*!
    \fn ulong QString::toULong(bool *ok, int base) const

    Returns the string converted to an \c{unsigned long} using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    If \a base is 0, the C language convention is used: if the string begins
    with "0x", base 16 is used; otherwise, if the string begins with "0b", base
    2 is used; otherwise, if the string begins with "0", base 8 is used;
    otherwise, base 10 is used.

    The string conversion will always happen in the 'C' locale. For
    locale-dependent conversion use QLocale::toULongLong()

    Example:

    \snippet qstring/main.cpp 78

    This function ignores leading and trailing whitespace.

    \note Support for the "0b" prefix was added in Qt 6.4.

    \sa number(), QLocale::toUInt()
*/

/*!
    \fn int QString::toInt(bool *ok, int base) const
    Returns the string converted to an \c int using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    If \a base is 0, the C language convention is used: if the string begins
    with "0x", base 16 is used; otherwise, if the string begins with "0b", base
    2 is used; otherwise, if the string begins with "0", base 8 is used;
    otherwise, base 10 is used.

    The string conversion will always happen in the 'C' locale. For
    locale-dependent conversion use QLocale::toInt()

    Example:

    \snippet qstring/main.cpp 72

    This function ignores leading and trailing whitespace.

    \note Support for the "0b" prefix was added in Qt 6.4.

    \sa number(), toUInt(), toDouble(), QLocale::toInt()
*/

/*!
    \fn uint QString::toUInt(bool *ok, int base) const
    Returns the string converted to an \c{unsigned int} using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    If \a base is 0, the C language convention is used: if the string begins
    with "0x", base 16 is used; otherwise, if the string begins with "0b", base
    2 is used; otherwise, if the string begins with "0", base 8 is used;
    otherwise, base 10 is used.

    The string conversion will always happen in the 'C' locale. For
    locale-dependent conversion use QLocale::toUInt()

    Example:

    \snippet qstring/main.cpp 77

    This function ignores leading and trailing whitespace.

    \note Support for the "0b" prefix was added in Qt 6.4.

    \sa number(), toInt(), QLocale::toUInt()
*/

/*!
    \fn short QString::toShort(bool *ok, int base) const

    Returns the string converted to a \c short using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    If \a base is 0, the C language convention is used: if the string begins
    with "0x", base 16 is used; otherwise, if the string begins with "0b", base
    2 is used; otherwise, if the string begins with "0", base 8 is used;
    otherwise, base 10 is used.

    The string conversion will always happen in the 'C' locale. For
    locale-dependent conversion use QLocale::toShort()

    Example:

    \snippet qstring/main.cpp 76

    This function ignores leading and trailing whitespace.

    \note Support for the "0b" prefix was added in Qt 6.4.

    \sa number(), toUShort(), toInt(), QLocale::toShort()
*/

/*!
    \fn ushort QString::toUShort(bool *ok, int base) const

    Returns the string converted to an \c{unsigned short} using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    If \a base is 0, the C language convention is used: if the string begins
    with "0x", base 16 is used; otherwise, if the string begins with "0b", base
    2 is used; otherwise, if the string begins with "0", base 8 is used;
    otherwise, base 10 is used.

    The string conversion will always happen in the 'C' locale. For
    locale-dependent conversion use QLocale::toUShort()

    Example:

    \snippet qstring/main.cpp 80

    This function ignores leading and trailing whitespace.

    \note Support for the "0b" prefix was added in Qt 6.4.

    \sa number(), toShort(), QLocale::toUShort()
*/

/*!
    Returns the string converted to a \c double value.

    Returns an infinity if the conversion overflows or 0.0 if the
    conversion fails for other reasons (e.g. underflow).

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    \snippet qstring/main.cpp 66

    \warning The QString content may only contain valid numerical characters
    which includes the plus/minus sign, the character e used in scientific
    notation, and the decimal point. Including the unit or additional characters
    leads to a conversion error.

    \snippet qstring/main.cpp 67

    The string conversion will always happen in the 'C' locale. For
    locale-dependent conversion use QLocale::toDouble()

    \snippet qstring/main.cpp 68

    For historical reasons, this function does not handle
    thousands group separators. If you need to convert such numbers,
    use QLocale::toDouble().

    \snippet qstring/main.cpp 69

    This function ignores leading and trailing whitespace.

    \sa number(), QLocale::setDefault(), QLocale::toDouble(), trimmed()
*/

double QString::toDouble(bool *ok) const
{
    return QStringView(*this).toDouble(ok);
}

double QStringView::toDouble(bool *ok) const
{
    QStringView string = qt_trimmed(*this);
    QVarLengthArray<uchar> latin1(string.size());
    qt_to_latin1(latin1.data(), string.utf16(), string.size());
    auto r = qt_asciiToDouble(reinterpret_cast<const char *>(latin1.data()), string.size());
    if (ok != nullptr)
        *ok = r.ok();
    return r.result;
}

/*!
    Returns the string converted to a \c float value.

    Returns an infinity if the conversion overflows or 0.0 if the
    conversion fails for other reasons (e.g. underflow).

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    \warning The QString content may only contain valid numerical characters
    which includes the plus/minus sign, the character e used in scientific
    notation, and the decimal point. Including the unit or additional characters
    leads to a conversion error.

    The string conversion will always happen in the 'C' locale. For
    locale-dependent conversion use QLocale::toFloat()

    For historical reasons, this function does not handle
    thousands group separators. If you need to convert such numbers,
    use QLocale::toFloat().

    Example:

    \snippet qstring/main.cpp 71

    This function ignores leading and trailing whitespace.

    \sa number(), toDouble(), toInt(), QLocale::toFloat(), trimmed()
*/

float QString::toFloat(bool *ok) const
{
    return QLocaleData::convertDoubleToFloat(toDouble(ok), ok);
}

float QStringView::toFloat(bool *ok) const
{
    return QLocaleData::convertDoubleToFloat(toDouble(ok), ok);
}

/*! \fn QString &QString::setNum(int n, int base)

    Sets the string to the printed value of \a n in the specified \a
    base, and returns a reference to the string.

    The base is 10 by default and must be between 2 and 36.

    \snippet qstring/main.cpp 56

   The formatting always uses QLocale::C, i.e., English/UnitedStates.
   To get a localized string representation of a number, use
   QLocale::toString() with the appropriate locale.

   \sa number()
*/

/*! \fn QString &QString::setNum(uint n, int base)

    \overload
*/

/*! \fn QString &QString::setNum(long n, int base)

    \overload
*/

/*! \fn QString &QString::setNum(ulong n, int base)

    \overload
*/

/*!
    \overload
*/
QString &QString::setNum(qlonglong n, int base)
{
    return *this = number(n, base);
}

/*!
    \overload
*/
QString &QString::setNum(qulonglong n, int base)
{
    return *this = number(n, base);
}

/*! \fn QString &QString::setNum(short n, int base)

    \overload
*/

/*! \fn QString &QString::setNum(ushort n, int base)

    \overload
*/

/*!
    \overload

    Sets the string to the printed value of \a n, formatted according to the
    given \a format and \a precision, and returns a reference to the string.

    \sa number(), QLocale::FloatingPointPrecisionOption, {Number Formats}
*/

QString &QString::setNum(double n, char format, int precision)
{
    return *this = number(n, format, precision);
}

/*!
    \fn QString &QString::setNum(float n, char format, int precision)
    \overload

    Sets the string to the printed value of \a n, formatted according
    to the given \a format and \a precision, and returns a reference
    to the string.

    The formatting always uses QLocale::C, i.e., English/UnitedStates.
    To get a localized string representation of a number, use
    QLocale::toString() with the appropriate locale.

    \sa number()
*/


/*!
    \fn QString QString::number(long n, int base)

    Returns a string equivalent of the number \a n according to the
    specified \a base.

    The base is 10 by default and must be between 2
    and 36. For bases other than 10, \a n is treated as an
    unsigned integer.

    The formatting always uses QLocale::C, i.e., English/UnitedStates.
    To get a localized string representation of a number, use
    QLocale::toString() with the appropriate locale.

    \snippet qstring/main.cpp 35

    \sa setNum()
*/

QString QString::number(long n, int base)
{
    return number(qlonglong(n), base);
}

/*!
  \fn QString QString::number(ulong n, int base)

    \overload
*/
QString QString::number(ulong n, int base)
{
    return number(qulonglong(n), base);
}

/*!
    \overload
*/
QString QString::number(int n, int base)
{
    return number(qlonglong(n), base);
}

/*!
    \overload
*/
QString QString::number(uint n, int base)
{
    return number(qulonglong(n), base);
}

/*!
    \overload
*/
QString QString::number(qlonglong n, int base)
{
#if defined(QT_CHECK_RANGE)
    if (base < 2 || base > 36) {
        qWarning("QString::setNum: Invalid base (%d)", base);
        base = 10;
    }
#endif
    bool negative = n < 0;
    /*
      Negating std::numeric_limits<qlonglong>::min() hits undefined behavior, so
      taking an absolute value has to take a slight detour.
    */
    return qulltoBasicLatin(negative ? 1u + qulonglong(-(n + 1)) : qulonglong(n), base, negative);
}

/*!
    \overload
*/
QString QString::number(qulonglong n, int base)
{
#if defined(QT_CHECK_RANGE)
    if (base < 2 || base > 36) {
        qWarning("QString::setNum: Invalid base (%d)", base);
        base = 10;
    }
#endif
    return qulltoBasicLatin(n, base, false);
}


/*!
    Returns a string representing the floating-point number \a n.

    Returns a string that represents \a n, formatted according to the specified
    \a format and \a precision.

    For formats with an exponent, the exponent will show its sign and have at
    least two digits, left-padding the exponent with zero if needed.

    \sa setNum(), QLocale::toString(), QLocale::FloatingPointPrecisionOption, {Number Formats}
*/
QString QString::number(double n, char format, int precision)
{
    QLocaleData::DoubleForm form = QLocaleData::DFDecimal;

    switch (QtMiscUtils::toAsciiLower(format)) {
        case 'f':
            form = QLocaleData::DFDecimal;
            break;
        case 'e':
            form = QLocaleData::DFExponent;
            break;
        case 'g':
            form = QLocaleData::DFSignificantDigits;
            break;
        default:
#if defined(QT_CHECK_RANGE)
            qWarning("QString::setNum: Invalid format char '%c'", format);
#endif
            break;
    }

    return qdtoBasicLatin(n, form, precision, isAsciiUpper(format));
}

namespace {
template<class ResultList, class StringSource>
static ResultList splitString(const StringSource &source, QStringView sep,
                              Qt::SplitBehavior behavior, Qt::CaseSensitivity cs)
{
    ResultList list;
    typename StringSource::size_type start = 0;
    typename StringSource::size_type end;
    typename StringSource::size_type extra = 0;
    while ((end = QtPrivate::findString(QStringView(source.constData(), source.size()), start + extra, sep, cs)) != -1) {
        if (start != end || behavior == Qt::KeepEmptyParts)
            list.append(source.sliced(start, end - start));
        start = end + sep.size();
        extra = (sep.size() == 0 ? 1 : 0);
    }
    if (start != source.size() || behavior == Qt::KeepEmptyParts)
        list.append(source.sliced(start));
    return list;
}

} // namespace

/*!
    Splits the string into substrings wherever \a sep occurs, and
    returns the list of those strings. If \a sep does not match
    anywhere in the string, split() returns a single-element list
    containing this string.

    \a cs specifies whether \a sep should be matched case
    sensitively or case insensitively.

    If \a behavior is Qt::SkipEmptyParts, empty entries don't
    appear in the result. By default, empty entries are kept.

    Example:

    \snippet qstring/main.cpp 62

    If \a sep is empty, split() returns an empty string, followed
    by each of the string's characters, followed by another empty string:

    \snippet qstring/main.cpp 62-empty

    To understand this behavior, recall that the empty string matches
    everywhere, so the above is qualitatively the same as:

    \snippet qstring/main.cpp 62-slashes

    \sa QStringList::join(), section()

    \since 5.14
*/
QStringList QString::split(const QString &sep, Qt::SplitBehavior behavior, Qt::CaseSensitivity cs) const
{
    return splitString<QStringList>(*this, sep, behavior, cs);
}

/*!
    \overload
    \since 5.14
*/
QStringList QString::split(QChar sep, Qt::SplitBehavior behavior, Qt::CaseSensitivity cs) const
{
    return splitString<QStringList>(*this, QStringView(&sep, 1), behavior, cs);
}

/*!
    \fn QList<QStringView> QStringView::split(QChar sep, Qt::SplitBehavior behavior, Qt::CaseSensitivity cs) const
    \fn QList<QStringView> QStringView::split(QStringView sep, Qt::SplitBehavior behavior, Qt::CaseSensitivity cs) const


    Splits the view into substring views wherever \a sep occurs, and
    returns the list of those string views.

    See QString::split() for how \a sep, \a behavior and \a cs interact to form
    the result.

    \note All the returned views are valid as long as the data referenced by
    this string view is valid. Destroying the data will cause all views to
    become dangling.

    \since 6.0
*/
QList<QStringView> QStringView::split(QStringView sep, Qt::SplitBehavior behavior, Qt::CaseSensitivity cs) const
{
    return splitString<QList<QStringView>>(QStringView(*this), sep, behavior, cs);
}

QList<QStringView> QStringView::split(QChar sep, Qt::SplitBehavior behavior, Qt::CaseSensitivity cs) const
{
    return split(QStringView(&sep, 1), behavior, cs);
}

#if QT_CONFIG(regularexpression)
namespace {
template<class ResultList, typename String, typename MatchingFunction>
static ResultList splitString(const String &source, const QRegularExpression &re,
                              MatchingFunction matchingFunction,
                              Qt::SplitBehavior behavior)
{
    ResultList list;
    if (!re.isValid()) {
        qtWarnAboutInvalidRegularExpression(re, "QString", "split");
        return list;
    }

    qsizetype start = 0;
    qsizetype end = 0;
    QRegularExpressionMatchIterator iterator = (re.*matchingFunction)(source, 0, QRegularExpression::NormalMatch, QRegularExpression::NoMatchOption);
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        end = match.capturedStart();
        if (start != end || behavior == Qt::KeepEmptyParts)
            list.append(source.sliced(start, end - start));
        start = match.capturedEnd();
    }

    if (start != source.size() || behavior == Qt::KeepEmptyParts)
        list.append(source.sliced(start));

    return list;
}
} // namespace

/*!
    \overload
    \since 5.14

    Splits the string into substrings wherever the regular expression
    \a re matches, and returns the list of those strings. If \a re
    does not match anywhere in the string, split() returns a
    single-element list containing this string.

    Here is an example where we extract the words in a sentence
    using one or more whitespace characters as the separator:

    \snippet qstring/main.cpp 90

    Here is a similar example, but this time we use any sequence of
    non-word characters as the separator:

    \snippet qstring/main.cpp 91

    Here is a third example where we use a zero-length assertion,
    \b{\\b} (word boundary), to split the string into an
    alternating sequence of non-word and word tokens:

    \snippet qstring/main.cpp 92

    \sa QStringList::join(), section()
*/
QStringList QString::split(const QRegularExpression &re, Qt::SplitBehavior behavior) const
{
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    const auto matchingFunction = qOverload<const QString &, qsizetype, QRegularExpression::MatchType, QRegularExpression::MatchOptions>(&QRegularExpression::globalMatch);
#else
    const auto matchingFunction = &QRegularExpression::globalMatch;
#endif
    return splitString<QStringList>(*this,
                                    re,
                                    matchingFunction,
                                    behavior);
}

/*!
    \overload
    \since 6.0

    Splits the string into substring views wherever the regular expression \a re
    matches, and returns the list of those strings. If \a re does not match
    anywhere in the string, split() returns a single-element list containing
    this string as view.

    \note The views in the returned list are sub-views of this view; as such,
    they reference the same data as it and only remain valid for as long as that
    data remains live.
*/
QList<QStringView> QStringView::split(const QRegularExpression &re, Qt::SplitBehavior behavior) const
{
    return splitString<QList<QStringView>>(*this, re, &QRegularExpression::globalMatchView, behavior);
}

#endif // QT_CONFIG(regularexpression)

/*!
    \enum QString::NormalizationForm

    This enum describes the various normalized forms of Unicode text.

    \value NormalizationForm_D  Canonical Decomposition
    \value NormalizationForm_C  Canonical Decomposition followed by Canonical Composition
    \value NormalizationForm_KD  Compatibility Decomposition
    \value NormalizationForm_KC  Compatibility Decomposition followed by Canonical Composition

    \sa normalized(),
        {https://www.unicode.org/reports/tr15/}{Unicode Standard Annex #15}
*/

/*!
    \since 4.5

    Returns a copy of this string repeated the specified number of \a times.

    If \a times is less than 1, an empty string is returned.

    Example:

    \snippet code/src_corelib_text_qstring.cpp 8
*/
QString QString::repeated(qsizetype times) const
{
    if (d.size == 0)
        return *this;

    if (times <= 1) {
        if (times == 1)
            return *this;
        return QString();
    }

    const qsizetype resultSize = times * d.size;

    QString result;
    result.reserve(resultSize);
    if (result.capacity() != resultSize)
        return QString(); // not enough memory

    memcpy(result.d.data(), d.data(), d.size * sizeof(QChar));

    qsizetype sizeSoFar = d.size;
    char16_t *end = result.d.data() + sizeSoFar;

    const qsizetype halfResultSize = resultSize >> 1;
    while (sizeSoFar <= halfResultSize) {
        memcpy(end, result.d.data(), sizeSoFar * sizeof(QChar));
        end += sizeSoFar;
        sizeSoFar <<= 1;
    }
    memcpy(end, result.d.data(), (resultSize - sizeSoFar) * sizeof(QChar));
    result.d.data()[resultSize] = '\0';
    result.d.size = resultSize;
    return result;
}

void qt_string_normalize(QString *data, QString::NormalizationForm mode, QChar::UnicodeVersion version, qsizetype from)
{
    {
        // check if it's fully ASCII first, because then we have no work
        auto start = reinterpret_cast<const char16_t *>(data->constData());
        const char16_t *p = start + from;
        if (isAscii_helper(p, p + data->size() - from))
            return;
        if (p > start + from)
            from = p - start - 1;        // need one before the non-ASCII to perform NFC
    }

    if (version == QChar::Unicode_Unassigned) {
        version = QChar::currentUnicodeVersion();
    } else if (int(version) <= NormalizationCorrectionsVersionMax) {
        const QString &s = *data;
        QChar *d = nullptr;
        for (const NormalizationCorrection &n : uc_normalization_corrections) {
            if (n.version > version) {
                qsizetype pos = from;
                if (QChar::requiresSurrogates(n.ucs4)) {
                    char16_t ucs4High = QChar::highSurrogate(n.ucs4);
                    char16_t ucs4Low = QChar::lowSurrogate(n.ucs4);
                    char16_t oldHigh = QChar::highSurrogate(n.old_mapping);
                    char16_t oldLow = QChar::lowSurrogate(n.old_mapping);
                    while (pos < s.size() - 1) {
                        if (s.at(pos).unicode() == ucs4High && s.at(pos + 1).unicode() == ucs4Low) {
                            if (!d)
                                d = data->data();
                            d[pos] = QChar(oldHigh);
                            d[++pos] = QChar(oldLow);
                        }
                        ++pos;
                    }
                } else {
                    while (pos < s.size()) {
                        if (s.at(pos).unicode() == n.ucs4) {
                            if (!d)
                                d = data->data();
                            d[pos] = QChar(n.old_mapping);
                        }
                        ++pos;
                    }
                }
            }
        }
    }

    if (normalizationQuickCheckHelper(data, mode, from, &from))
        return;

    decomposeHelper(data, mode < QString::NormalizationForm_KD, version, from);

    canonicalOrderHelper(data, version, from);

    if (mode == QString::NormalizationForm_D || mode == QString::NormalizationForm_KD)
        return;

    composeHelper(data, version, from);
}

/*!
    Returns the string in the given Unicode normalization \a mode,
    according to the given \a version of the Unicode standard.
*/
QString QString::normalized(QString::NormalizationForm mode, QChar::UnicodeVersion version) const
{
    QString copy = *this;
    qt_string_normalize(&copy, mode, version, 0);
    return copy;
}

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) && !defined(QT_BOOTSTRAPPED)
static void checkArgEscape(QStringView s)
{
    // If we're in here, it means that qArgDigitValue has accepted the
    // digit. We can skip the check in case we already know it will
    // succeed.
    if (!supportUnicodeDigitValuesInArg())
        return;

    const auto isNonAsciiDigit = [](QChar c) {
        return c.unicode() < u'0' || c.unicode() > u'9';
    };

    if (std::any_of(s.begin(), s.end(), isNonAsciiDigit)) {
        const auto accumulateDigit = [](int partial, QChar digit) {
            return partial * 10 + digit.digitValue();
        };
        const int parsedNumber = std::accumulate(s.begin(), s.end(), 0, accumulateDigit);

        qWarning("QString::arg(): the replacement \"%%%ls\" contains non-ASCII digits;\n"
                 "    it is currently being interpreted as the %d-th substitution.\n"
                 "    This is deprecated; support for non-ASCII digits will be dropped\n"
                 "    in a future version of Qt.",
                 qUtf16Printable(s.toString()),
                 parsedNumber);
    }
}
#endif

struct ArgEscapeData
{
    int min_escape;            // lowest escape sequence number
    qsizetype occurrences;     // number of occurrences of the lowest escape sequence number
    qsizetype locale_occurrences; // number of occurrences of the lowest escape sequence number that
                                  // contain 'L'
    qsizetype escape_len;      // total length of escape sequences which will be replaced
};

static ArgEscapeData findArgEscapes(QStringView s)
{
    const QChar *uc_begin = s.begin();
    const QChar *uc_end = s.end();

    ArgEscapeData d;

    d.min_escape = INT_MAX;
    d.occurrences = 0;
    d.escape_len = 0;
    d.locale_occurrences = 0;

    const QChar *c = uc_begin;
    while (c != uc_end) {
        while (c != uc_end && c->unicode() != '%')
            ++c;

        if (c == uc_end)
            break;
        const QChar *escape_start = c;
        if (++c == uc_end)
            break;

        bool locale_arg = false;
        if (c->unicode() == 'L') {
            locale_arg = true;
            if (++c == uc_end)
                break;
        }

        int escape = qArgDigitValue(*c);
        if (escape == -1)
            continue;

        // ### Qt 7: do not allow anything but ASCII digits
        // in arg()'s replacements.
#if QT_VERSION <= QT_VERSION_CHECK(7, 0, 0) && !defined(QT_BOOTSTRAPPED)
        const QChar *escapeBegin = c;
        const QChar *escapeEnd = escapeBegin + 1;
#endif

        ++c;

        if (c != uc_end) {
            const int next_escape = qArgDigitValue(*c);
            if (next_escape != -1) {
                escape = (10 * escape) + next_escape;
                ++c;
#if QT_VERSION <= QT_VERSION_CHECK(7, 0, 0) && !defined(QT_BOOTSTRAPPED)
                ++escapeEnd;
#endif
            }
        }

#if QT_VERSION <= QT_VERSION_CHECK(7, 0, 0) && !defined(QT_BOOTSTRAPPED)
        checkArgEscape(QStringView(escapeBegin, escapeEnd));
#endif

        if (escape > d.min_escape)
            continue;

        if (escape < d.min_escape) {
            d.min_escape = escape;
            d.occurrences = 0;
            d.escape_len = 0;
            d.locale_occurrences = 0;
        }

        ++d.occurrences;
        if (locale_arg)
            ++d.locale_occurrences;
        d.escape_len += c - escape_start;
    }
    return d;
}

static QString replaceArgEscapes(QStringView s, const ArgEscapeData &d, qsizetype field_width,
                                 QStringView arg, QStringView larg, QChar fillChar)
{
    // Negative field-width for right-padding, positive for left-padding:
    const qsizetype abs_field_width = qAbs(field_width);
    const qsizetype result_len =
            s.size() - d.escape_len
            + (d.occurrences - d.locale_occurrences) * qMax(abs_field_width, arg.size())
            + d.locale_occurrences * qMax(abs_field_width, larg.size());

    QString result(result_len, Qt::Uninitialized);
    QChar *rc = const_cast<QChar *>(result.unicode());
    QChar *const result_end = rc + result_len;
    qsizetype repl_cnt = 0;

    const QChar *c = s.begin();
    const QChar *const uc_end = s.end();
    while (c != uc_end) {
        Q_ASSERT(d.occurrences > repl_cnt);
        /* We don't have to check increments of c against uc_end because, as
           long as d.occurrences > repl_cnt, we KNOW there are valid escape
           sequences remaining. */

        const QChar *text_start = c;
        while (c->unicode() != '%')
            ++c;

        const QChar *escape_start = c++;
        const bool localize = c->unicode() == 'L';
        if (localize)
            ++c;

        int escape = qArgDigitValue(*c);
        if (escape != -1 && c + 1 != uc_end) {
            const int digit = qArgDigitValue(c[1]);
            if (digit != -1) {
                ++c;
                escape = 10 * escape + digit;
            }
        }

        if (escape != d.min_escape) {
            memcpy(rc, text_start, (c - text_start) * sizeof(QChar));
            rc += c - text_start;
        } else {
            ++c;

            memcpy(rc, text_start, (escape_start - text_start) * sizeof(QChar));
            rc += escape_start - text_start;

            const QStringView use = localize ? larg : arg;
            const qsizetype pad_chars = abs_field_width - use.size();
            // (If negative, relevant loops are no-ops: no need to check.)

            if (field_width > 0) { // left padded
                rc = std::fill_n(rc, pad_chars, fillChar);
            }

            if (use.size())
                memcpy(rc, use.data(), use.size() * sizeof(QChar));
            rc += use.size();

            if (field_width < 0) { // right padded
                rc = std::fill_n(rc, pad_chars, fillChar);
            }

            if (++repl_cnt == d.occurrences) {
                memcpy(rc, c, (uc_end - c) * sizeof(QChar));
                rc += uc_end - c;
                Q_ASSERT(rc == result_end);
                c = uc_end;
            }
        }
    }
    Q_ASSERT(rc == result_end);

    return result;
}

/*!
    \fn template <typename T, QString::if_string_like<T> = true> QString QString::arg(const T &a, int fieldWidth, QChar fillChar) const

    Returns a copy of this string with the lowest-numbered place-marker
    replaced by string \a a, i.e., \c %1, \c %2, ..., \c %99.

    \a fieldWidth specifies the minimum amount of space that \a a
    shall occupy. If \a a requires less space than \a fieldWidth, it
    is padded to \a fieldWidth with character \a fillChar.  A positive
    \a fieldWidth produces right-aligned text. A negative \a fieldWidth
    produces left-aligned text.

    This example shows how we might create a \c status string for
    reporting progress while processing a list of files:

    \snippet qstring/main.cpp 11-qstringview

    First, \c arg(i) replaces \c %1. Then \c arg(total) replaces \c
    %2. Finally, \c arg(fileName) replaces \c %3.

    One advantage of using arg() over asprintf() is that the order of the
    numbered place markers can change, if the application's strings are
    translated into other languages, but each arg() will still replace
    the lowest-numbered unreplaced place-marker, no matter where it
    appears. Also, if place-marker \c %i appears more than once in the
    string, arg() replaces all of them.

    If there is no unreplaced place-marker remaining, a warning message
    is printed and the result is undefined. Place-marker numbers must be
    in the range 1 to 99.

    \note In Qt versions prior to 6.9, this function was overloaded on
    \c{char}, QChar, QString, QStringView, and QLatin1StringView and in some
    cases, \c{wchar_t} and \c{char16_t} arguments would resolve to the integer
    overloads. In Qt versions prior to 5.10, this function lacked the
    QStringView and QLatin1StringView overloads.
*/
QString QString::arg_impl(QAnyStringView a, int fieldWidth, QChar fillChar) const
{
    ArgEscapeData d = findArgEscapes(*this);

    if (Q_UNLIKELY(d.occurrences == 0)) {
        qWarning("QString::arg: Argument missing: \"%ls\", \"%ls\"", qUtf16Printable(*this),
                  qUtf16Printable(a.toString()));
        return *this;
    }
    struct {
        QVarLengthArray<char16_t> out;
        QStringView operator()(QStringView in) noexcept { return in; }
        QStringView operator()(QLatin1StringView in)
        {
            out.resize(in.size());
            qt_from_latin1(out.data(), in.data(), size_t(in.size()));
            return out;
        }
        QStringView operator()(QUtf8StringView in)
        {
            out.resize(in.size());
            return QStringView{out.data(), QUtf8::convertToUnicode(out.data(), in)};
        }
    } convert;

    QStringView sv = a.visit(std::ref(convert));
    return replaceArgEscapes(*this, d, fieldWidth, sv, sv, fillChar);
}

/*!
  \fn template <typename T, QString::if_integral_non_char<T> = true> QString QString::arg(T a, int fieldWidth, int base, QChar fillChar) const
  \overload arg()

  The \a a argument is expressed in base \a base, which is 10 by
  default and must be between 2 and 36. For bases other than 10, \a a
  is treated as an unsigned integer.

  \a fieldWidth specifies the minimum amount of space that \a a is
  padded to and filled with the character \a fillChar. A positive
  value produces right-aligned text; a negative value produces
  left-aligned text.

  The '%' can be followed by an 'L', in which case the sequence is
  replaced with a localized representation of \a a. The conversion
  uses the default locale, set by QLocale::setDefault(). If no default
  locale was specified, the system locale is used. The 'L' flag is
  ignored if \a base is not 10.

  \snippet qstring/main.cpp 12
  \snippet qstring/main.cpp 14

  \note In Qt versions prior to 6.9, this function was overloaded on various
  integral types and sometimes incorrectly accepted \c char and \c char16_t
  arguments.

  \sa {Number Formats}
*/
QString QString::arg_impl(qlonglong a, int fieldWidth, int base, QChar fillChar) const
{
    ArgEscapeData d = findArgEscapes(*this);

    if (d.occurrences == 0) {
        qWarning("QString::arg: Argument missing: \"%ls\", %llu", qUtf16Printable(*this), a);
        return *this;
    }

    unsigned flags = QLocaleData::NoFlags;
    // ZeroPadded sorts out left-padding when the fill is zero, to the right of sign:
    if (fillChar == u'0')
        flags = QLocaleData::ZeroPadded;

    QString arg;
    if (d.occurrences > d.locale_occurrences) {
        arg = QLocaleData::c()->longLongToString(a, -1, base, fieldWidth, flags);
        Q_ASSERT(fillChar != u'0' || fieldWidth <= arg.size());
    }

    QString localeArg;
    if (d.locale_occurrences > 0) {
        QLocale locale;
        if (!(locale.numberOptions() & QLocale::OmitGroupSeparator))
            flags |= QLocaleData::GroupDigits;
        localeArg = locale.d->m_data->longLongToString(a, -1, base, fieldWidth, flags);
        Q_ASSERT(fillChar != u'0' || fieldWidth <= localeArg.size());
    }

    return replaceArgEscapes(*this, d, fieldWidth, arg, localeArg, fillChar);
}

QString QString::arg_impl(qulonglong a, int fieldWidth, int base, QChar fillChar) const
{
    ArgEscapeData d = findArgEscapes(*this);

    if (d.occurrences == 0) {
        qWarning("QString::arg: Argument missing: \"%ls\", %lld", qUtf16Printable(*this), a);
        return *this;
    }

    unsigned flags = QLocaleData::NoFlags;
    // ZeroPadded sorts out left-padding when the fill is zero, to the right of sign:
    if (fillChar == u'0')
        flags = QLocaleData::ZeroPadded;

    QString arg;
    if (d.occurrences > d.locale_occurrences) {
        arg = QLocaleData::c()->unsLongLongToString(a, -1, base, fieldWidth, flags);
        Q_ASSERT(fillChar != u'0' || fieldWidth <= arg.size());
    }

    QString localeArg;
    if (d.locale_occurrences > 0) {
        QLocale locale;
        if (!(locale.numberOptions() & QLocale::OmitGroupSeparator))
            flags |= QLocaleData::GroupDigits;
        localeArg = locale.d->m_data->unsLongLongToString(a, -1, base, fieldWidth, flags);
        Q_ASSERT(fillChar != u'0' || fieldWidth <= localeArg.size());
    }

    return replaceArgEscapes(*this, d, fieldWidth, arg, localeArg, fillChar);
}

/*!
  \fn template <typename T, QString::if_floating_point<T> = true> QString QString::arg(T a, int fieldWidth, char format, int precision, QChar fillChar) const
  \overload arg()

  Argument \a a is formatted according to the specified \a format and
  \a precision. See \l{Floating-point Formats} for details.

  \a fieldWidth specifies the minimum amount of space that \a a is
  padded to and filled with the character \a fillChar.  A positive
  value produces right-aligned text; a negative value produces
  left-aligned text.

  \snippet code/src_corelib_text_qstring.cpp 2

  \note In Qt versions prior to 6.9, this function was a regular function
  taking \c double. As a consequence of being a template function now, it no
  longer accepts arguments that merely implicitly convert to floating-point
  types. A backwards-compatible fix is to cast such types to one of the C++
  floating-point types.

  \sa QLocale::toString(), QLocale::FloatingPointPrecisionOption, {Number Formats}
*/
QString QString::arg_impl(double a, int fieldWidth, char format, int precision, QChar fillChar) const
{
    ArgEscapeData d = findArgEscapes(*this);

    if (d.occurrences == 0) {
        qWarning("QString::arg: Argument missing: \"%ls\", %g", qUtf16Printable(*this), a);
        return *this;
    }

    unsigned flags = QLocaleData::NoFlags;
    // ZeroPadded sorts out left-padding when the fill is zero, to the right of sign:
    if (fillChar == u'0')
        flags |= QLocaleData::ZeroPadded;

    if (isAsciiUpper(format))
        flags |= QLocaleData::CapitalEorX;

    QLocaleData::DoubleForm form = QLocaleData::DFDecimal;
    switch (QtMiscUtils::toAsciiLower(format)) {
    case 'f':
        form = QLocaleData::DFDecimal;
        break;
    case 'e':
        form = QLocaleData::DFExponent;
        break;
    case 'g':
        form = QLocaleData::DFSignificantDigits;
        break;
    default:
#if defined(QT_CHECK_RANGE)
        qWarning("QString::arg: Invalid format char '%c'", format);
#endif
        break;
    }

    QString arg;
    if (d.occurrences > d.locale_occurrences) {
        arg = QLocaleData::c()->doubleToString(a, precision, form, fieldWidth,
                                               flags | QLocaleData::ZeroPadExponent);
        Q_ASSERT(fillChar != u'0' || !qt_is_finite(a)
                 || fieldWidth <= arg.size());
    }

    QString localeArg;
    if (d.locale_occurrences > 0) {
        QLocale locale;

        const QLocale::NumberOptions numberOptions = locale.numberOptions();
        if (!(numberOptions & QLocale::OmitGroupSeparator))
            flags |= QLocaleData::GroupDigits;
        if (!(numberOptions & QLocale::OmitLeadingZeroInExponent))
            flags |= QLocaleData::ZeroPadExponent;
        if (numberOptions & QLocale::IncludeTrailingZeroesAfterDot)
            flags |= QLocaleData::AddTrailingZeroes;
        localeArg = locale.d->m_data->doubleToString(a, precision, form, fieldWidth, flags);
        Q_ASSERT(fillChar != u'0' || !qt_is_finite(a)
                 || fieldWidth <= localeArg.size());
    }

    return replaceArgEscapes(*this, d, fieldWidth, arg, localeArg, fillChar);
}

static inline char16_t to_unicode(const QChar c) { return c.unicode(); }
static inline char16_t to_unicode(const char c) { return QLatin1Char{c}.unicode(); }

template <typename Char>
static int getEscape(const Char *uc, qsizetype *pos, qsizetype len)
{
    qsizetype i = *pos;
    ++i;
    if (i < len && uc[i] == u'L')
        ++i;
    if (i < len) {
        int escape = to_unicode(uc[i]) - '0';
        if (uint(escape) >= 10U)
            return -1;
        ++i;
        if (i < len) {
            // there's a second digit
            int digit = to_unicode(uc[i]) - '0';
            if (uint(digit) < 10U) {
                escape = (escape * 10) + digit;
                ++i;
            }
        }
        *pos = i;
        return escape;
    }
    return -1;
}

/*
    Algorithm for multiArg:

    1. Parse the string as a sequence of verbatim text and placeholders (%L?\d{,3}).
       The L is parsed and accepted for compatibility with non-multi-arg, but since
       multiArg only accepts strings as replacements, the localization request can
       be safely ignored.
    2. The result of step (1) is a list of (string-ref,int)-tuples. The string-ref
       either points at text to be copied verbatim (in which case the int is -1),
       or, initially, at the textual representation of the placeholder. In that case,
       the int contains the numerical number as parsed from the placeholder.
    3. Next, collect all the non-negative ints found, sort them in ascending order and
       remove duplicates.
       3a. If the result has more entries than multiArg() was given replacement strings,
           we have found placeholders we can't satisfy with replacement strings. That is
           fine (there could be another .arg() call coming after this one), so just
           truncate the result to the number of actual multiArg() replacement strings.
       3b. If the result has less entries than multiArg() was given replacement strings,
           the string is missing placeholders. This is an error that the user should be
           warned about.
    4. The result of step (3) is a mapping from the index of any replacement string to
       placeholder number. This is the wrong way around, but since placeholder
       numbers could get as large as 999, while we typically don't have more than 9
       replacement strings, we trade 4K of sparsely-used memory for doing a reverse lookup
       each time we need to map a placeholder number to a replacement string index
       (that's a linear search; but still *much* faster than using an associative container).
    5. Next, for each of the tuples found in step (1), do the following:
       5a. If the int is negative, do nothing.
       5b. Otherwise, if the int is found in the result of step (3) at index I, replace
           the string-ref with a string-ref for the (complete) I'th replacement string.
       5c. Otherwise, do nothing.
    6. Concatenate all string refs into a single result string.
*/

namespace {
struct Part
{
    Part() = default; // for QVarLengthArray; do not use
    constexpr Part(QAnyStringView s, int num = -1)
        : string{s}, number{num} {}

    void reset(QAnyStringView s) noexcept { *this = {s, number}; }

    QAnyStringView string;
    int number;
};
} // unnamed namespace

Q_DECLARE_TYPEINFO(Part, Q_PRIMITIVE_TYPE);

namespace {

enum { ExpectedParts = 32 };

typedef QVarLengthArray<Part, ExpectedParts> ParseResult;
typedef QVarLengthArray<int, ExpectedParts/2> ArgIndexToPlaceholderMap;

template <typename StringView>
static ParseResult parseMultiArgFormatString_impl(StringView s)
{
    ParseResult result;

    const auto uc = s.data();
    const auto len = s.size();
    const auto end = len - 1;
    qsizetype i = 0;
    qsizetype last = 0;

    while (i < end) {
        if (uc[i] == u'%') {
            qsizetype percent = i;
            int number = getEscape(uc, &i, len);
            if (number != -1) {
                if (last != percent)
                    result.push_back(Part{s.sliced(last, percent - last)}); // literal text (incl. failed placeholders)
                result.push_back(Part{s.sliced(percent, i - percent), number});  // parsed placeholder
                last = i;
                continue;
            }
        }
        ++i;
    }

    if (last < len)
        result.push_back(Part{s.sliced(last, len - last)}); // trailing literal text

    return result;
}

static ParseResult parseMultiArgFormatString(QAnyStringView s)
{
    return s.visit([] (auto s) { return parseMultiArgFormatString_impl(s); });
}

static ArgIndexToPlaceholderMap makeArgIndexToPlaceholderMap(const ParseResult &parts)
{
    ArgIndexToPlaceholderMap result;

    for (const Part &part : parts) {
        if (part.number >= 0)
            result.push_back(part.number);
    }

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()),
                 result.end());

    return result;
}

static qsizetype resolveStringRefsAndReturnTotalSize(ParseResult &parts, const ArgIndexToPlaceholderMap &argIndexToPlaceholderMap, const QtPrivate::ArgBase *args[])
{
    using namespace QtPrivate;
    qsizetype totalSize = 0;
    for (Part &part : parts) {
        if (part.number != -1) {
            const auto it = std::find(argIndexToPlaceholderMap.begin(), argIndexToPlaceholderMap.end(), part.number);
            if (it != argIndexToPlaceholderMap.end()) {
                const auto &arg = *args[it - argIndexToPlaceholderMap.begin()];
                switch (arg.tag) {
                case ArgBase::L1:
                    part.reset(static_cast<const QLatin1StringArg&>(arg).string);
                    break;
                case ArgBase::Any:
                    part.reset(static_cast<const QAnyStringArg&>(arg).string);
                    break;
                case ArgBase::U16:
                    part.reset(static_cast<const QStringViewArg&>(arg).string);
                    break;
                }
            }
        }
        totalSize += part.string.size();
    }
    return totalSize;
}

} // unnamed namespace

QString QtPrivate::argToQString(QAnyStringView pattern, size_t numArgs, const ArgBase **args)
{
    // Step 1-2 above
    ParseResult parts = parseMultiArgFormatString(pattern);

    // 3-4
    ArgIndexToPlaceholderMap argIndexToPlaceholderMap = makeArgIndexToPlaceholderMap(parts);

    if (static_cast<size_t>(argIndexToPlaceholderMap.size()) > numArgs) // 3a
        argIndexToPlaceholderMap.resize(qsizetype(numArgs));
    else if (Q_UNLIKELY(static_cast<size_t>(argIndexToPlaceholderMap.size()) < numArgs)) // 3b
        qWarning("QString::arg: %d argument(s) missing in %ls",
                 int(numArgs - argIndexToPlaceholderMap.size()), qUtf16Printable(pattern.toString()));

    // 5
    const qsizetype totalSize = resolveStringRefsAndReturnTotalSize(parts, argIndexToPlaceholderMap, args);

    // 6:
    QString result(totalSize, Qt::Uninitialized);
    auto out = const_cast<QChar*>(result.constData());

    struct Concatenate {
        QChar *out;
        QChar *operator()(QLatin1String part) noexcept
        {
            if (part.size()) {
                qt_from_latin1(reinterpret_cast<char16_t*>(out),
                               part.data(), part.size());
            }
            return out + part.size();
        }
        QChar *operator()(QUtf8StringView part) noexcept
        {
            return QUtf8::convertToUnicode(out, part);
        }
        QChar *operator()(QStringView part) noexcept
        {
            if (part.size())
                memcpy(out, part.data(), part.size() * sizeof(QChar));
            return out + part.size();
        }
    };

    for (const Part &part : parts)
        out = part.string.visit(Concatenate{out});

    // UTF-8 decoding may have caused an overestimate of totalSize - correct it:
    result.truncate(out - result.cbegin());

    return result;
}

/*! \fn bool QString::isRightToLeft() const

    Returns \c true if the string is read right to left.

    \sa QStringView::isRightToLeft()
*/
bool QString::isRightToLeft() const
{
    return QtPrivate::isRightToLeft(QStringView(*this));
}

/*!
    \fn bool QString::isValidUtf16() const noexcept
    \since 5.15

    Returns \c true if the string contains valid UTF-16 encoded data,
    or \c false otherwise.

    Note that this function does not perform any special validation of the
    data; it merely checks if it can be successfully decoded from UTF-16.
    The data is assumed to be in host byte order; the presence of a BOM
    is meaningless.

    \sa QStringView::isValidUtf16()
*/

/*! \fn QChar *QString::data()

    Returns a pointer to the data stored in the QString. The pointer
    can be used to access and modify the characters that compose the
    string.

    Unlike constData() and unicode(), the returned data is always
    '\\0'-terminated.

    Example:

    \snippet qstring/main.cpp 19

    Note that the pointer remains valid only as long as the string is
    not modified by other means. For read-only access, constData() is
    faster because it never causes a \l{deep copy} to occur.

    \sa constData(), operator[]()
*/

/*! \fn const QChar *QString::data() const

    \overload

    \note The returned string may not be '\\0'-terminated.
    Use size() to determine the length of the array.

    \sa fromRawData()
*/

/*! \fn const QChar *QString::constData() const

    Returns a pointer to the data stored in the QString. The pointer
    can be used to access the characters that compose the string.

    Note that the pointer remains valid only as long as the string is
    not modified.

    \note The returned string may not be '\\0'-terminated.
    Use size() to determine the length of the array.

    \sa data(), operator[](), fromRawData()
*/

/*! \fn void QString::push_front(const QString &other)

    This function is provided for STL compatibility, prepending the
    given \a other string to the beginning of this string. It is
    equivalent to \c prepend(other).

    \sa prepend()
*/

/*! \fn void QString::push_front(QChar ch)

    \overload

    Prepends the given \a ch character to the beginning of this string.
*/

/*! \fn void QString::push_back(const QString &other)

    This function is provided for STL compatibility, appending the
    given \a other string onto the end of this string. It is
    equivalent to \c append(other).

    \sa append()
*/

/*! \fn void QString::push_back(QChar ch)

    \overload

    Appends the given \a ch character onto the end of this string.
*/

/*!
    \since 6.1

    Removes from the string the characters in the half-open range
    [ \a first , \a last ). Returns an iterator to the character
    immediately after the last erased character (i.e. the character
    referred to by \a last before the erase).
*/
QString::iterator QString::erase(QString::const_iterator first, QString::const_iterator last)
{
    const auto start = std::distance(cbegin(), first);
    const auto len = std::distance(first, last);
    remove(start, len);
    return begin() + start;
}

/*!
    \fn QString::iterator QString::erase(QString::const_iterator it)

    \overload
    \since 6.5

    Removes the character denoted by \c it from the string.
    Returns an iterator to the character immediately after the
    erased character.

    \code
    QString c = "abcdefg";
    auto it = c.erase(c.cbegin()); // c is now "bcdefg"; "it" points to "b"
    \endcode
*/

/*! \fn void QString::shrink_to_fit()
    \since 5.10

    This function is provided for STL compatibility. It is
    equivalent to squeeze().

    \sa squeeze()
*/

/*!
    \fn std::string QString::toStdString() const

    Returns a std::string object with the data contained in this
    QString. The Unicode data is converted into 8-bit characters using
    the toUtf8() function.

    This method is mostly useful to pass a QString to a function
    that accepts a std::string object.

    \sa toLatin1(), toUtf8(), toLocal8Bit(), QByteArray::toStdString()
*/
std::string QString::toStdString() const
{
    std::string result;
    if (isEmpty())
        return result;

    auto writeToBuffer = [this](char *out, size_t) {
        char *last = QUtf8::convertFromUnicode(out, *this);
        return last - out;
    };
    size_t maxSize = size() * 3;    // worst case for UTF-8
#ifdef __cpp_lib_string_resize_and_overwrite
    // C++23
    result.resize_and_overwrite(maxSize, writeToBuffer);
#else
    result.resize(maxSize);
    result.resize(writeToBuffer(result.data(), result.size()));
#endif
    return result;
}

/*!
    \fn QString QString::fromRawData(const char16_t *unicode, qsizetype size)
    \since 6.10

    Constructs a QString that uses the first \a size Unicode characters
    in the array \a unicode. The data in \a unicode is \e not
    copied. The caller must be able to guarantee that \a unicode will
    not be deleted or modified as long as the QString (or an
    unmodified copy of it) exists.

    Any attempts to modify the QString or copies of it will cause it
    to create a deep copy of the data, ensuring that the raw data
    isn't modified.

    Here is an example of how we can use a QRegularExpression on raw data in
    memory without requiring to copy the data into a QString:

    \snippet qstring/main.cpp 22
    \snippet qstring/main.cpp 23

    \warning A string created with fromRawData() is \e not
    '\\0'-terminated, unless the raw data contains a '\\0' character
    at position \a size. This means unicode() will \e not return a
    '\\0'-terminated string (although utf16() does, at the cost of
    copying the raw data).

    \sa fromUtf16(), setRawData(), data(), constData(),
    nullTerminate(), nullTerminated()
*/

/*!
    \fn QString QString::fromRawData(const QChar *unicode, qsizetype size)
    \overload
*/

/*!
    \since 4.7

    Resets the QString to use the first \a size Unicode characters
    in the array \a unicode. The data in \a unicode is \e not
    copied. The caller must be able to guarantee that \a unicode will
    not be deleted or modified as long as the QString (or an
    unmodified copy of it) exists.

    This function can be used instead of fromRawData() to re-use
    existings QString objects to save memory re-allocations.

    \sa fromRawData(), nullTerminate(), nullTerminated()
*/
QString &QString::setRawData(const QChar *unicode, qsizetype size)
{
    if (!unicode || !size) {
        clear();
    }
    *this = fromRawData(unicode, size);
    return *this;
}

/*! \fn QString QString::fromStdU16String(const std::u16string &str)
    \since 5.5

    \include qstring.cpp {from-std-string} {UTF-16} {fromUtf16()}

    \sa fromUtf16(), fromStdWString(), fromStdU32String()
*/

/*!
    \fn std::u16string QString::toStdU16String() const
    \since 5.5

    Returns a std::u16string object with the data contained in this
    QString. The Unicode data is the same as returned by the utf16()
    method.

    \sa utf16(), toStdWString(), toStdU32String()
*/

/*! \fn QString QString::fromStdU32String(const std::u32string &str)
    \since 5.5

    \include qstring.cpp {from-std-string} {UTF-32} {fromUcs4()}

    \sa fromUcs4(), fromStdWString(), fromStdU16String()
*/

/*!
    \fn std::u32string QString::toStdU32String() const
    \since 5.5

    Returns a std::u32string object with the data contained in this
    QString. The Unicode data is the same as returned by the toUcs4()
    method.

    \sa toUcs4(), toStdWString(), toStdU16String()
*/

#if !defined(QT_NO_DATASTREAM)
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QString &string)
    \relates QString

    Writes the given \a string to the specified \a stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &out, const QString &str)
{
    if (out.version() == 1) {
        out << str.toLatin1();
    } else {
        if (!str.isNull() || out.version() < 3) {
            if ((out.byteOrder() == QDataStream::BigEndian) == (QSysInfo::ByteOrder == QSysInfo::BigEndian)) {
                out.writeBytes(reinterpret_cast<const char *>(str.unicode()),
                               static_cast<qsizetype>(sizeof(QChar) * str.size()));
            } else {
                QVarLengthArray<char16_t> buffer(str.size());
                qbswap<sizeof(char16_t)>(str.constData(), str.size(), buffer.data());
                out.writeBytes(reinterpret_cast<const char *>(buffer.data()),
                               static_cast<qsizetype>(sizeof(char16_t) * buffer.size()));
            }
        } else {
            QDataStream::writeQSizeType(out, -1); // write null marker
        }
    }
    return out;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QString &string)
    \relates QString

    Reads a string from the specified \a stream into the given \a string.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &in, QString &str)
{
    if (in.version() == 1) {
        QByteArray l;
        in >> l;
        str = QString::fromLatin1(l);
    } else {
        qint64 size = QDataStream::readQSizeType(in);
        qsizetype bytes = size;
        if (size != bytes || size < -1) {
            str.clear();
            in.setStatus(QDataStream::SizeLimitExceeded);
            return in;
        }
        if (bytes == -1) { // null string
            str = QString();
        } else if (bytes > 0) {
            if (bytes & 0x1) {
                str.clear();
                in.setStatus(QDataStream::ReadCorruptData);
                return in;
            }

            const qsizetype Step = 1024 * 1024;
            qsizetype len = bytes / 2;
            qsizetype allocated = 0;

            while (allocated < len) {
                int blockSize = qMin(Step, len - allocated);
                str.resize(allocated + blockSize);
                if (in.readRawData(reinterpret_cast<char *>(str.data()) + allocated * 2,
                                   blockSize * 2) != blockSize * 2) {
                    str.clear();
                    in.setStatus(QDataStream::ReadPastEnd);
                    return in;
                }
                allocated += blockSize;
            }

            if ((in.byteOrder() == QDataStream::BigEndian)
                    != (QSysInfo::ByteOrder == QSysInfo::BigEndian)) {
                char16_t *data = reinterpret_cast<char16_t *>(str.data());
                qbswap<sizeof(*data)>(data, len, data);
            }
        } else {
            str = QString(QLatin1StringView(""));
        }
    }
    return in;
}
#endif // QT_NO_DATASTREAM

/*!
    \typedef QString::Data
    \internal
*/

/*!
    \typedef QString::DataPtr
    \internal
*/

/*!
    \fn DataPtr & QString::data_ptr()
    \internal
*/

/*!
    \since 5.11
    \internal
    \relates QStringView

    Returns \c true if the string is read right to left.

    \sa QString::isRightToLeft()
*/
bool QtPrivate::isRightToLeft(QStringView string) noexcept
{
    int isolateLevel = 0;

    for (QStringIterator i(string); i.hasNext();) {
        const char32_t c = i.next();

        switch (QChar::direction(c)) {
        case QChar::DirRLI:
        case QChar::DirLRI:
        case QChar::DirFSI:
            ++isolateLevel;
            break;
        case QChar::DirPDI:
            if (isolateLevel)
                --isolateLevel;
            break;
        case QChar::DirL:
            if (isolateLevel)
                break;
            return false;
        case QChar::DirR:
        case QChar::DirAL:
            if (isolateLevel)
                break;
            return true;
        case QChar::DirEN:
        case QChar::DirES:
        case QChar::DirET:
        case QChar::DirAN:
        case QChar::DirCS:
        case QChar::DirB:
        case QChar::DirS:
        case QChar::DirWS:
        case QChar::DirON:
        case QChar::DirLRE:
        case QChar::DirLRO:
        case QChar::DirRLE:
        case QChar::DirRLO:
        case QChar::DirPDF:
        case QChar::DirNSM:
        case QChar::DirBN:
            break;
        }
    }
    return false;
}

qsizetype QtPrivate::count(QStringView haystack, QStringView needle, Qt::CaseSensitivity cs) noexcept
{
    qsizetype num = 0;
    qsizetype i = -1;
    if (haystack.size() > 500 && needle.size() > 5) {
        QStringMatcher matcher(needle, cs);
        while ((i = matcher.indexIn(haystack, i + 1)) != -1)
            ++num;
    } else {
        while ((i = QtPrivate::findString(haystack, i + 1, needle, cs)) != -1)
            ++num;
    }
    return num;
}

qsizetype QtPrivate::count(QStringView haystack, QChar needle, Qt::CaseSensitivity cs) noexcept
{
    if (cs == Qt::CaseSensitive)
        return std::count(haystack.cbegin(), haystack.cend(), needle);

    needle = foldCase(needle);
    return std::count_if(haystack.cbegin(), haystack.cend(),
                         [needle](const QChar c) { return foldAndCompare(c, needle); });
}

qsizetype QtPrivate::count(QLatin1StringView haystack, QLatin1StringView needle, Qt::CaseSensitivity cs)
{
    qsizetype num = 0;
    qsizetype i = -1;

    QLatin1StringMatcher matcher(needle, cs);
    while ((i = matcher.indexIn(haystack, i + 1)) != -1)
        ++num;

    return num;
}

qsizetype QtPrivate::count(QLatin1StringView haystack, QStringView needle, Qt::CaseSensitivity cs)
{
    if (haystack.size() < needle.size())
        return 0;

    if (!QtPrivate::isLatin1(needle)) // won't find non-L1 UTF-16 needles in a L1 haystack!
        return 0;

    qsizetype num = 0;
    qsizetype i = -1;

    QVarLengthArray<uchar> s(needle.size());
    qt_to_latin1_unchecked(s.data(), needle.utf16(), needle.size());

    QLatin1StringMatcher matcher(QLatin1StringView(reinterpret_cast<char *>(s.data()), s.size()),
                                 cs);
    while ((i = matcher.indexIn(haystack, i + 1)) != -1)
        ++num;

    return num;
}

qsizetype QtPrivate::count(QStringView haystack, QLatin1StringView needle, Qt::CaseSensitivity cs)
{
    if (haystack.size() < needle.size())
        return -1;

    QVarLengthArray<char16_t> s = qt_from_latin1_to_qvla(needle);
    return QtPrivate::count(haystack, QStringView(s.data(), s.size()), cs);
}

qsizetype QtPrivate::count(QLatin1StringView haystack, QChar needle, Qt::CaseSensitivity cs) noexcept
{
    // non-L1 needles cannot possibly match in L1-only haystacks
    if (needle.unicode() > 0xff)
        return 0;

    if (cs == Qt::CaseSensitive) {
        return std::count(haystack.cbegin(), haystack.cend(), needle.toLatin1());
    } else {
        return std::count_if(haystack.cbegin(), haystack.cend(),
                             CaseInsensitiveL1::matcher(needle.toLatin1()));
    }
}

/*!
    \fn bool QtPrivate::startsWith(QStringView haystack, QStringView needle, Qt::CaseSensitivity cs)
    \since 5.10
    \fn bool QtPrivate::startsWith(QStringView haystack, QLatin1StringView needle, Qt::CaseSensitivity cs)
    \since 5.10
    \fn bool QtPrivate::startsWith(QLatin1StringView haystack, QStringView needle, Qt::CaseSensitivity cs)
    \since 5.10
    \fn bool QtPrivate::startsWith(QLatin1StringView haystack, QLatin1StringView needle, Qt::CaseSensitivity cs)
    \since 5.10
    \internal
    \relates QStringView

    Returns \c true if \a haystack starts with \a needle,
    otherwise returns \c false.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    \sa QtPrivate::endsWith(), QString::endsWith(), QStringView::endsWith(), QLatin1StringView::endsWith()
*/

bool QtPrivate::startsWith(QStringView haystack, QStringView needle, Qt::CaseSensitivity cs) noexcept
{
    return qt_starts_with_impl(haystack, needle, cs);
}

bool QtPrivate::startsWith(QStringView haystack, QLatin1StringView needle, Qt::CaseSensitivity cs) noexcept
{
    return qt_starts_with_impl(haystack, needle, cs);
}

bool QtPrivate::startsWith(QLatin1StringView haystack, QStringView needle, Qt::CaseSensitivity cs) noexcept
{
    return qt_starts_with_impl(haystack, needle, cs);
}

bool QtPrivate::startsWith(QLatin1StringView haystack, QLatin1StringView needle, Qt::CaseSensitivity cs) noexcept
{
    return qt_starts_with_impl(haystack, needle, cs);
}

/*!
    \fn bool QtPrivate::endsWith(QStringView haystack, QStringView needle, Qt::CaseSensitivity cs)
    \since 5.10
    \fn bool QtPrivate::endsWith(QStringView haystack, QLatin1StringView needle, Qt::CaseSensitivity cs)
    \since 5.10
    \fn bool QtPrivate::endsWith(QLatin1StringView haystack, QStringView needle, Qt::CaseSensitivity cs)
    \since 5.10
    \fn bool QtPrivate::endsWith(QLatin1StringView haystack, QLatin1StringView needle, Qt::CaseSensitivity cs)
    \since 5.10
    \internal
    \relates QStringView

    Returns \c true if \a haystack ends with \a needle,
    otherwise returns \c false.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    \sa QtPrivate::startsWith(), QString::endsWith(), QStringView::endsWith(), QLatin1StringView::endsWith()
*/

bool QtPrivate::endsWith(QStringView haystack, QStringView needle, Qt::CaseSensitivity cs) noexcept
{
    return qt_ends_with_impl(haystack, needle, cs);
}

bool QtPrivate::endsWith(QStringView haystack, QLatin1StringView needle, Qt::CaseSensitivity cs) noexcept
{
    return qt_ends_with_impl(haystack, needle, cs);
}

bool QtPrivate::endsWith(QLatin1StringView haystack, QStringView needle, Qt::CaseSensitivity cs) noexcept
{
    return qt_ends_with_impl(haystack, needle, cs);
}

bool QtPrivate::endsWith(QLatin1StringView haystack, QLatin1StringView needle, Qt::CaseSensitivity cs) noexcept
{
    return qt_ends_with_impl(haystack, needle, cs);
}

qsizetype QtPrivate::findString(QStringView haystack0, qsizetype from, QStringView needle0, Qt::CaseSensitivity cs) noexcept
{
    const qsizetype l = haystack0.size();
    const qsizetype sl = needle0.size();
    if (sl == 1)
        return findString(haystack0, from, needle0[0], cs);
    if (from < 0)
        from += l;
    if (std::size_t(sl + from) > std::size_t(l))
        return -1;
    if (!sl)
        return from;
    if (!l)
        return -1;

    /*
        We use the Boyer-Moore algorithm in cases where the overhead
        for the skip table should pay off, otherwise we use a simple
        hash function.
    */
    if (l > 500 && sl > 5)
        return qFindStringBoyerMoore(haystack0, from, needle0, cs);

    auto sv = [sl](const char16_t *v) { return QStringView(v, sl); };
    /*
        We use some hashing for efficiency's sake. Instead of
        comparing strings, we compare the hash value of str with that
        of a part of this QString. Only if that matches, we call
        qt_string_compare().
    */
    const char16_t *needle = needle0.utf16();
    const char16_t *haystack = haystack0.utf16() + from;
    const char16_t *end = haystack0.utf16() + (l - sl);
    const qregisteruint sl_minus_1 = sl - 1;
    qregisteruint hashNeedle = 0, hashHaystack = 0;
    qsizetype idx;

    if (cs == Qt::CaseSensitive) {
        for (idx = 0; idx < sl; ++idx) {
            hashNeedle = ((hashNeedle<<1) + needle[idx]);
            hashHaystack = ((hashHaystack<<1) + haystack[idx]);
        }
        hashHaystack -= haystack[sl_minus_1];

        while (haystack <= end) {
            hashHaystack += haystack[sl_minus_1];
            if (hashHaystack == hashNeedle
                 && QtPrivate::compareStrings(needle0, sv(haystack), Qt::CaseSensitive) == 0)
                return haystack - haystack0.utf16();

            REHASH(*haystack);
            ++haystack;
        }
    } else {
        const char16_t *haystack_start = haystack0.utf16();
        for (idx = 0; idx < sl; ++idx) {
            hashNeedle = (hashNeedle<<1) + foldCase(needle + idx, needle);
            hashHaystack = (hashHaystack<<1) + foldCase(haystack + idx, haystack_start);
        }
        hashHaystack -= foldCase(haystack + sl_minus_1, haystack_start);

        while (haystack <= end) {
            hashHaystack += foldCase(haystack + sl_minus_1, haystack_start);
            if (hashHaystack == hashNeedle
                 && QtPrivate::compareStrings(needle0, sv(haystack), Qt::CaseInsensitive) == 0)
                return haystack - haystack0.utf16();

            REHASH(foldCase(haystack, haystack_start));
            ++haystack;
        }
    }
    return -1;
}

qsizetype QtPrivate::findString(QStringView haystack, qsizetype from, QLatin1StringView needle, Qt::CaseSensitivity cs) noexcept
{
    if (haystack.size() < needle.size())
        return -1;

    QVarLengthArray<char16_t> s = qt_from_latin1_to_qvla(needle);
    return QtPrivate::findString(haystack, from, QStringView(reinterpret_cast<const QChar*>(s.constData()), s.size()), cs);
}

qsizetype QtPrivate::findString(QLatin1StringView haystack, qsizetype from, QStringView needle, Qt::CaseSensitivity cs) noexcept
{
    if (haystack.size() < needle.size())
        return -1;

    if (!QtPrivate::isLatin1(needle)) // won't find non-L1 UTF-16 needles in a L1 haystack!
        return -1;

    if (needle.size() == 1) {
        const char n = needle.front().toLatin1();
        return QtPrivate::findString(haystack, from, QLatin1StringView(&n, 1), cs);
    }

    QVarLengthArray<char> s(needle.size());
    qt_to_latin1_unchecked(reinterpret_cast<uchar *>(s.data()), needle.utf16(), needle.size());
    return QtPrivate::findString(haystack, from, QLatin1StringView(s.data(), s.size()), cs);
}

qsizetype QtPrivate::findString(QLatin1StringView haystack, qsizetype from, QLatin1StringView needle, Qt::CaseSensitivity cs) noexcept
{
    if (from < 0)
        from += haystack.size();
    if (from < 0)
        return -1;
    qsizetype adjustedSize = haystack.size() - from;
    if (adjustedSize < needle.size())
        return -1;
    if (needle.size() == 0)
        return from;

    if (cs == Qt::CaseSensitive) {

        if (needle.size() == 1) {
            Q_ASSERT(haystack.data() != nullptr); // see size check above
            if (auto it = memchr(haystack.data() + from, needle.front().toLatin1(), adjustedSize))
                return static_cast<const char *>(it) - haystack.data();
            return -1;
        }

        const QLatin1StringMatcher matcher(needle, Qt::CaseSensitivity::CaseSensitive);
        return matcher.indexIn(haystack, from);
    }

    // If the needle is sufficiently small we simply iteratively search through
    // the haystack. When the needle is too long we use a boyer-moore searcher
    // from the standard library, if available. If it is not available then the
    // QLatin1Strings are converted to QString and compared as such. Though
    // initialization is slower the boyer-moore search it employs still makes up
    // for it when haystack and needle are sufficiently long.
    // The needle size was chosen by testing various lengths using the
    // qstringtokenizer benchmark with the
    // "tokenize_qlatin1string_qlatin1string" test.
#ifdef Q_CC_MSVC
    const qsizetype threshold = 1;
#else
    const qsizetype threshold = 13;
#endif
    if (needle.size() <= threshold) {
        const auto begin = haystack.begin();
        const auto end = haystack.end() - needle.size() + 1;
        auto ciMatch = CaseInsensitiveL1::matcher(needle[0].toLatin1());
        const qsizetype nlen1 = needle.size() - 1;
        for (auto it = std::find_if(begin + from, end, ciMatch); it != end;
             it = std::find_if(it + 1, end, ciMatch)) {
            // In this comparison we skip the first character because we know it's a match
            if (!nlen1 || QLatin1StringView(it + 1, nlen1).compare(needle.sliced(1), cs) == 0)
                return std::distance(begin, it);
        }
        return -1;
    }

    QLatin1StringMatcher matcher(needle, Qt::CaseSensitivity::CaseInsensitive);
    return matcher.indexIn(haystack, from);
}

qsizetype QtPrivate::lastIndexOf(QStringView haystack, qsizetype from, char16_t needle, Qt::CaseSensitivity cs) noexcept
{
    return qLastIndexOf(haystack, QChar(needle), from, cs);
}

qsizetype QtPrivate::lastIndexOf(QStringView haystack, qsizetype from, QStringView needle, Qt::CaseSensitivity cs) noexcept
{
    return qLastIndexOf(haystack, from, needle, cs);
}

qsizetype QtPrivate::lastIndexOf(QStringView haystack, qsizetype from, QLatin1StringView needle, Qt::CaseSensitivity cs) noexcept
{
    return qLastIndexOf(haystack, from, needle, cs);
}

qsizetype QtPrivate::lastIndexOf(QLatin1StringView haystack, qsizetype from, QStringView needle, Qt::CaseSensitivity cs) noexcept
{
    return qLastIndexOf(haystack, from, needle, cs);
}

qsizetype QtPrivate::lastIndexOf(QLatin1StringView haystack, qsizetype from, QLatin1StringView needle, Qt::CaseSensitivity cs) noexcept
{
    return qLastIndexOf(haystack, from, needle, cs);
}

#if QT_CONFIG(regularexpression)
qsizetype QtPrivate::indexOf(QStringView viewHaystack, const QString *stringHaystack, const QRegularExpression &re, qsizetype from, QRegularExpressionMatch *rmatch)
{
    if (!re.isValid()) {
        qtWarnAboutInvalidRegularExpression(re, "QString(View)", "indexOf");
        return -1;
    }

    QRegularExpressionMatch match = stringHaystack
                ? re.match(*stringHaystack, from)
                : re.matchView(viewHaystack, from);
    if (match.hasMatch()) {
        const qsizetype ret = match.capturedStart();
        if (rmatch)
            *rmatch = std::move(match);
        return ret;
    }

    return -1;
}

qsizetype QtPrivate::indexOf(QStringView haystack, const QRegularExpression &re, qsizetype from, QRegularExpressionMatch *rmatch)
{
    return indexOf(haystack, nullptr, re, from, rmatch);
}

qsizetype QtPrivate::lastIndexOf(QStringView viewHaystack, const QString *stringHaystack, const QRegularExpression &re, qsizetype from, QRegularExpressionMatch *rmatch)
{
    if (!re.isValid()) {
        qtWarnAboutInvalidRegularExpression(re, "QString(View)", "lastIndexOf");
        return -1;
    }

    qsizetype endpos = (from < 0) ? (viewHaystack.size() + from + 1) : (from + 1);
    QRegularExpressionMatchIterator iterator = stringHaystack
            ? re.globalMatch(*stringHaystack)
            : re.globalMatchView(viewHaystack);
    qsizetype lastIndex = -1;
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        qsizetype start = match.capturedStart();
        if (start < endpos) {
            lastIndex = start;
            if (rmatch)
                *rmatch = std::move(match);
        } else {
            break;
        }
    }

    return lastIndex;
}

qsizetype QtPrivate::lastIndexOf(QStringView haystack, const QRegularExpression &re, qsizetype from, QRegularExpressionMatch *rmatch)
{
    return lastIndexOf(haystack, nullptr, re, from, rmatch);
}

bool QtPrivate::contains(QStringView viewHaystack, const QString *stringHaystack, const QRegularExpression &re, QRegularExpressionMatch *rmatch)
{
    if (!re.isValid()) {
        qtWarnAboutInvalidRegularExpression(re, "QString(View)", "contains");
        return false;
    }
    QRegularExpressionMatch m = stringHaystack
                ? re.match(*stringHaystack)
                : re.matchView(viewHaystack);
    bool hasMatch = m.hasMatch();
    if (hasMatch && rmatch)
        *rmatch = std::move(m);
    return hasMatch;
}

bool QtPrivate::contains(QStringView haystack, const QRegularExpression &re, QRegularExpressionMatch *rmatch)
{
    return contains(haystack, nullptr, re, rmatch);
}

qsizetype QtPrivate::count(QStringView haystack, const QRegularExpression &re)
{
    if (!re.isValid()) {
        qtWarnAboutInvalidRegularExpression(re, "QString(View)", "count");
        return 0;
    }
    qsizetype count = 0;
    qsizetype index = -1;
    qsizetype len = haystack.size();
    while (index <= len - 1) {
        QRegularExpressionMatch match = re.matchView(haystack, index + 1);
        if (!match.hasMatch())
            break;
        count++;

        // Search again, from the next character after the beginning of this
        // capture. If the capture starts with a surrogate pair, both together
        // count as "one character".
        index = match.capturedStart();
        if (index < len && haystack[index].isHighSurrogate())
            ++index;
    }
    return count;
}

#endif // QT_CONFIG(regularexpression)

/*!
    \since 5.0

    Converts a plain text string to an HTML string with
    HTML metacharacters \c{<}, \c{>}, \c{&}, and \c{"} replaced by HTML
    entities.

    Example:

    \snippet code/src_corelib_text_qstring.cpp 7
*/
QString QString::toHtmlEscaped() const
{
    const auto pos = std::u16string_view(*this).find_first_of(u"<>&\"");
    if (pos == std::u16string_view::npos)
        return *this;
    QString rich;
    const qsizetype len = size();
    rich.reserve(qsizetype(len * 1.1));
    rich += qToStringViewIgnoringNull(*this).first(pos);
    for (auto ch : qToStringViewIgnoringNull(*this).sliced(pos)) {
        if (ch == u'<')
            rich += "&lt;"_L1;
        else if (ch == u'>')
            rich += "&gt;"_L1;
        else if (ch == u'&')
            rich += "&amp;"_L1;
        else if (ch == u'"')
            rich += "&quot;"_L1;
        else
            rich += ch;
    }
    rich.squeeze();
    return rich;
}

/*!
  \macro QStringLiteral(str)
  \relates QString

  The macro generates the data for a QString out of the string literal \a str
  at compile time. Creating a QString from it is free in this case, and the
  generated string data is stored in the read-only segment of the compiled
  object file.

  If you have code that looks like this:

  \snippet code/src_corelib_text_qstring.cpp 9

  then a temporary QString will be created to be passed as the \c{hasAttribute}
  function parameter. This can be quite expensive, as it involves a memory
  allocation and the copy/conversion of the data into QString's internal
  encoding.

  This cost can be avoided by using QStringLiteral instead:

  \snippet code/src_corelib_text_qstring.cpp 10

  In this case, QString's internal data will be generated at compile time; no
  conversion or allocation will occur at runtime.

  Using QStringLiteral instead of a double quoted plain C++ string literal can
  significantly speed up creation of QString instances from data known at
  compile time.

  \note QLatin1StringView can still be more efficient than QStringLiteral
  when the string is passed to a function that has an overload taking
  QLatin1StringView and this overload avoids conversion to QString.  For
  instance, QString::operator==() can compare to a QLatin1StringView
  directly:

  \snippet code/src_corelib_text_qstring.cpp 11

  \note Some compilers have bugs encoding strings containing characters outside
  the US-ASCII character set. Make sure you prefix your string with \c{u} in
  those cases. It is optional otherwise.

  \sa QByteArrayLiteral
*/

#if QT_DEPRECATED_SINCE(6, 8)
/*!
  \fn QtLiterals::operator""_qs(const char16_t *str, size_t size)

  \relates QString
  \since 6.2
  \deprecated [6.8] Use \c _s from Qt::StringLiterals namespace instead.

  Literal operator that creates a QString out of the first \a size characters in
  the char16_t string literal \a str.

  The QString is created at compile time, and the generated string data is stored
  in the read-only segment of the compiled object file. Duplicate literals may
  share the same read-only memory. This functionality is interchangeable with
  QStringLiteral, but saves typing when many string literals are present in the
  code.

  The following code creates a QString:
  \code
  auto str = u"hello"_qs;
  \endcode

  \sa QStringLiteral, QtLiterals::operator""_qba(const char *str, size_t size)
*/
#endif // QT_DEPRECATED_SINCE(6, 8)

/*!
    \fn Qt::Literals::StringLiterals::operator""_s(const char16_t *str, size_t size)

    \relates QString
    \since 6.4

    Literal operator that creates a QString out of the first \a size characters in
    the char16_t string literal \a str.

    The QString is created at compile time, and the generated string data is stored
    in the read-only segment of the compiled object file. Duplicate literals may
    share the same read-only memory. This functionality is interchangeable with
    QStringLiteral, but saves typing when many string literals are present in the
    code.

    The following code creates a QString:
    \code
    using namespace Qt::Literals::StringLiterals;

    auto str = u"hello"_s;
    \endcode

    \sa Qt::Literals::StringLiterals
*/

/*!
    \internal
 */
void QAbstractConcatenable::appendLatin1To(QLatin1StringView in, QChar *out) noexcept
{
    qt_from_latin1(reinterpret_cast<char16_t *>(out), in.data(), size_t(in.size()));
}

/*!
  \fn template <typename T> qsizetype erase(QString &s, const T &t)
  \relates QString
  \since 6.1

  Removes all elements that compare equal to \a t from the
  string \a s. Returns the number of elements removed, if any.

  \sa erase_if
*/

/*!
  \fn template <typename Predicate> qsizetype erase_if(QString &s, Predicate pred)
  \relates QString
  \since 6.1

  Removes all elements for which the predicate \a pred returns true
  from the string \a s. Returns the number of elements removed, if
  any.

  \sa erase
*/

/*!
    \macro const char *qPrintable(const QString &str)
    \relates QString

    Returns \a str as a \c{const char *}. This is equivalent to
    \a{str}.toLocal8Bit().\l{QByteArray::}{constData()}.

    The char pointer will be invalid after the statement in which
    qPrintable() is used. This is because the array returned by
    QString::toLocal8Bit() will fall out of scope.

    \note qDebug(), qInfo(), qWarning(), qCritical(), qFatal() expect
    %s arguments to be UTF-8 encoded, while qPrintable() converts to
    local 8-bit encoding. Therefore qUtf8Printable() should be used
    for logging strings instead of qPrintable().

    \sa qUtf8Printable()
*/

/*!
    \macro const char *qUtf8Printable(const QString &str)
    \relates QString
    \since 5.4

    Returns \a str as a \c{const char *}. This is equivalent to
    \a{str}.toUtf8().\l{QByteArray::}{constData()}.

    The char pointer will be invalid after the statement in which
    qUtf8Printable() is used. This is because the array returned by
    QString::toUtf8() will fall out of scope.

    Example:

    \snippet code/src_corelib_text_qstring.cpp qUtf8Printable

    \sa qPrintable(), qDebug(), qInfo(), qWarning(), qCritical(), qFatal()
*/

/*!
    \macro const wchar_t *qUtf16Printable(const QString &str)
    \relates QString
    \since 5.7

    Returns \a str as a \c{const ushort *}, but cast to a \c{const wchar_t *}
    to avoid warnings. This is equivalent to \a{str}.utf16() plus some casting.

    The only useful thing you can do with the return value of this macro is to
    pass it to QString::asprintf() for use in a \c{%ls} conversion. In particular,
    the return value is \e{not} a valid \c{const wchar_t*}!

    In general, the pointer will be invalid after the statement in which
    qUtf16Printable() is used. This is because the pointer may have been
    obtained from a temporary expression, which will fall out of scope.

    Example:

    \snippet code/src_corelib_text_qstring.cpp qUtf16Printable

    \sa qPrintable(), qDebug(), qInfo(), qWarning(), qCritical(), qFatal()
*/

QT_END_NAMESPACE

#undef REHASH
