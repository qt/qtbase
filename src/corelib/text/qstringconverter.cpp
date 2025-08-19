// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2020 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include <qstringconverter.h>
#include <private/qstringconverter_p.h>
#include "qendian.h"

#include "private/qsimd_p.h"
#include "private/qstringiterator_p.h"
#include "private/qtools_p.h"
#include "qbytearraymatcher.h"
#include "qcontainertools_impl.h"
#include <QtCore/qbytearraylist.h>

#if QT_CONFIG(icu)

#include <unicode/ucnv.h>
#include <unicode/ucnv_cb.h>
#include <unicode/ucnv_err.h>
#include <unicode/ustring.h>
#define QT_USE_ICU_CODECS
#define QT_COM_THREAD_INIT

#elif QT_CONFIG(winsdkicu)

#include <icu.h>
#include <private/qfunctions_win_p.h>
#define QT_USE_ICU_CODECS
#define QT_COM_THREAD_INIT qt_win_ensureComInitializedOnThisThread();

#endif // QT_CONFIG(icu) || QT_CONFIG(winsdkicu)

#ifdef Q_OS_WIN
#include <qt_windows.h>
#ifndef QT_BOOTSTRAPPED
#include <QtCore/qvarlengtharray.h>
#include <QtCore/private/wcharhelpers_win_p.h>

#include <QtCore/q20iterator.h>
#include <QtCore/q26numeric.h>
#endif // !QT_BOOTSTRAPPED
#endif

#include <array>
#if __has_include(<bit>) && __cplusplus > 201703L
#include <bit>
#endif
#include <string>
#include <QtCore/q20utility.h>

QT_BEGIN_NAMESPACE

using namespace QtMiscUtils;

static_assert(std::is_nothrow_move_constructible_v<QStringEncoder>);
static_assert(std::is_nothrow_move_assignable_v<QStringEncoder>);
static_assert(std::is_nothrow_move_constructible_v<QStringDecoder>);
static_assert(std::is_nothrow_move_assignable_v<QStringDecoder>);

enum { Endian = 0, Data = 1 };

static const uchar utf8bom[] = { 0xef, 0xbb, 0xbf };

#if defined(__SSE2__) || defined(__ARM_NEON__)
Q_ALWAYS_INLINE static uint qBitScanReverse(unsigned v) noexcept
{
#if defined(__cpp_lib_int_pow2) && __cpp_lib_int_pow2 >= 202002L
     return std::bit_width(v) - 1;
#else
    uint result = qCountLeadingZeroBits(v);
    // Now Invert the result: clz will count *down* from the msb to the lsb, so the msb index is 31
    // and the lsb index is 0. The result for _bit_scan_reverse is expected to be the index when
    // counting up: msb index is 0 (because it starts there), and the lsb index is 31.
    result ^= sizeof(unsigned) * 8 - 1;
    return result;
#endif
}
#endif

#if defined(__SSE2__)
template <QCpuFeatureType Cpu = _compilerCpuFeatures> Q_ALWAYS_INLINE static bool
simdEncodeAscii(uchar *&dst, const char16_t *&nextAscii, const char16_t *&src, const char16_t *end)
{
    size_t sizeBytes = reinterpret_cast<const char *>(end) - reinterpret_cast<const char *>(src);

    // do sixteen characters at a time
    auto process16Chars = [](uchar *dst, const char16_t *src) {
        __m128i data1 = _mm_loadu_si128((const __m128i*)src);
        __m128i data2 = _mm_loadu_si128(1+(const __m128i*)src);

        // check if everything is ASCII
        // the highest ASCII value is U+007F
        // Do the packing directly:
        // The PACKUSWB instruction has packs a signed 16-bit integer to an unsigned 8-bit
        // with saturation. That is, anything from 0x0100 to 0x7fff is saturated to 0xff,
        // while all negatives (0x8000 to 0xffff) get saturated to 0x00. To detect non-ASCII,
        // we simply do a signed greater-than comparison to 0x00. That means we detect NULs as
        // "non-ASCII", but it's an acceptable compromise.
        __m128i packed = _mm_packus_epi16(data1, data2);
        __m128i nonAscii = _mm_cmpgt_epi8(packed, _mm_setzero_si128());

        // store, even if there are non-ASCII characters here
        _mm_storeu_si128((__m128i*)dst, packed);

        // n will contain 1 bit set per character in [data1, data2] that is non-ASCII (or NUL)
        ushort n = ~_mm_movemask_epi8(nonAscii);
        return n;
    };
    auto maybeFoundNonAscii = [&](auto n, qptrdiff offset = 0) {
        if (n) {
            // find the next probable ASCII character
            // we don't want to load 32 bytes again in this loop if we know there are non-ASCII
            // characters still coming
            src += offset;
            dst += offset;
            nextAscii = src + qBitScanReverse(n) + 1;

            n = qCountTrailingZeroBits(n);
            dst += n;
            src += n;
            return false;
        }
        return src == end;
    };
    auto adjustToEnd = [&] {
        dst += sizeBytes / sizeof(char16_t);
        src = end;
    };

    if constexpr (Cpu & CpuFeatureAVX2) {
        // The 256-bit VPACKUSWB[1] instruction interleaves the two input
        // operands, so we need an extra permutation to get them back in-order.
        // VPERMW takes 2 cyles to run while VPERMQ takes only 1.
        // [1] https://www.felixcloutier.com/x86/PACKUSWB.html
        constexpr size_t Step = 32;
        auto process32Chars = [](const char16_t *src, uchar *dst) {
            __m256i data1 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src));
            __m256i data2 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src) + 1);
            __m256i packed = _mm256_packus_epi16(data1, data2); // will be [A, B, A, B]
            __m256i permuted = _mm256_permute4x64_epi64(packed, _MM_SHUFFLE(3, 1, 2, 0));
            __m256i nonAscii = _mm256_cmpgt_epi8(permuted, _mm256_setzero_si256());

            // store, even if there are non-ASCII characters here
            _mm256_storeu_si256(reinterpret_cast<__m256i *>(dst), permuted);

            return ~_mm256_movemask_epi8(nonAscii);
        };

        if constexpr (Cpu & CpuFeatureAVX512VL) {
            // with AVX512/AXV10, we always process everything
            if (sizeBytes <= Step * sizeof(char16_t)) {
                uint mask = _bzhi_u32(-1, uint(sizeBytes / 2));
                __m256i data1 = _mm256_maskz_loadu_epi16(mask, src);
                __m256i data2 = _mm256_maskz_loadu_epi16(mask >> 16, src + Step / 2);
                __m256i packed = _mm256_packus_epi16(data1, data2);
                __m256i permuted = _mm256_permute4x64_epi64(packed, _MM_SHUFFLE(3, 1, 2, 0));
                __mmask32 nonAscii = _mm256_mask_cmple_epi8_mask(mask, permuted, _mm256_setzero_si256());

                // store, even if there are non-ASCII characters here
                _mm256_mask_storeu_epi8(dst, mask, permuted);
                if (nonAscii)
                    return maybeFoundNonAscii(nonAscii);
                adjustToEnd();
                return true;
            }
        }

        if (sizeBytes >= Step * sizeof(char16_t)) {
            // do 32 characters at a time
            qptrdiff offset = 0;
            for ( ; (offset + Step) * sizeof(char16_t) < sizeBytes; offset += Step) {
                if (uint n = process32Chars(src + offset, dst + offset))
                    return maybeFoundNonAscii(n, offset);
            }

            // do 32 characters again, possibly overlapping with the loop above
            adjustToEnd();
            uint n = process32Chars(src - Step, dst - Step);
            return maybeFoundNonAscii(n, -int(Step));
        }
    }

    constexpr size_t Step = 16;
    if (sizeBytes >= Step * sizeof(char16_t)) {

        qptrdiff offset = 0;
        for ( ; (offset + Step) * sizeof(char16_t) < sizeBytes; offset += Step) {
            ushort n = process16Chars(dst + offset, src + offset);
            if (n)
                return maybeFoundNonAscii(n, offset);
            if (Cpu & CpuFeatureAVX2)
                break;      // we can only ever loop once because of the code above
        }

        // do sixteen characters again, possibly overlapping with the loop above
        adjustToEnd();
        ushort n = process16Chars(dst - Step, src - Step);
        return maybeFoundNonAscii(n, -int(Step));
    }

#  if !defined(__OPTIMIZE_SIZE__)
    if (sizeBytes >= 8 * sizeof(char16_t)) {
        // do eight characters at a time
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));
        __m128i data2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(end - 8));
        __m128i packed = _mm_packus_epi16(data, data);
        __m128i nonAscii = _mm_cmpgt_epi8(packed, _mm_setzero_si128());

        // store even non-ASCII
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dst), packed);

        uchar n = ~_mm_movemask_epi8(nonAscii);
        if (n)
            return maybeFoundNonAscii(n);

        adjustToEnd();
        packed = _mm_packus_epi16(data2, data2);
        nonAscii = _mm_cmpgt_epi8(packed, _mm_setzero_si128());
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dst - 8), packed);
        n = ~_mm_movemask_epi8(nonAscii);
        return maybeFoundNonAscii(n, -8);
    } else if (sizeBytes >= 4 * sizeof(char16_t)) {
        // do four characters at a time
        __m128i data1 = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(src));
        __m128i data2 = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(end - 4));
        __m128i packed = _mm_packus_epi16(data1, data1);
        __m128i nonAscii = _mm_cmpgt_epi8(packed, _mm_setzero_si128());

        // store even non-ASCII
        qToUnaligned(_mm_cvtsi128_si32(packed), dst);

        uchar n = uchar(_mm_movemask_epi8(nonAscii) ^ 0xf);
        if (n)
            return maybeFoundNonAscii(n);

        adjustToEnd();
        packed = _mm_packus_epi16(data2, data2);
        nonAscii = _mm_cmpgt_epi8(packed, _mm_setzero_si128());
        qToUnaligned(_mm_cvtsi128_si32(packed), dst - 4);
        n = uchar(_mm_movemask_epi8(nonAscii) ^ 0xf);
        return maybeFoundNonAscii(n, -4);
    }
#endif

    return src == end;
}

template <QCpuFeatureType Cpu = _compilerCpuFeatures> Q_ALWAYS_INLINE static bool
simdDecodeAscii(char16_t *&dst, const uchar *&nextAscii, const uchar *&src, const uchar *end)
{
    // do sixteen characters at a time
    auto process16Chars = [](char16_t *dst, const uchar *src) {
        __m128i data = _mm_loadu_si128((const __m128i*)src);

        // check if everything is ASCII
        // movemask extracts the high bit of every byte, so n is non-zero if something isn't ASCII
        uint n = _mm_movemask_epi8(data);

        // store everything, even mojibake
        _mm_storeu_si128((__m128i*)dst, _mm_unpacklo_epi8(data, _mm_setzero_si128()));
        _mm_storeu_si128(1+(__m128i*)dst, _mm_unpackhi_epi8(data, _mm_setzero_si128()));
        return ushort(n);
    };
    auto maybeFoundNonAscii = [&](uint n, qptrdiff offset = 0) {
        // find the next probable ASCII character
        // we don't want to load 16 bytes again in this loop if we know there are non-ASCII
        // characters still coming
        if (n) {
            uint c = qCountTrailingZeroBits(n);
            src += offset;
            dst += offset;
            n = qBitScanReverse(n);
            nextAscii = src + n + 1;
            src += c;
            dst += c;
        }
        return src == end;
    };
    auto adjustToEnd = [&] {
        dst += end - src;
        src = end;
    };

    if constexpr (Cpu & CpuFeatureAVX2) {
        constexpr qsizetype Step = 32;
        auto process32Chars = [](char16_t *dst, const uchar *src) {
            __m128i data1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));
            __m128i data2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src) + 1);

            // the processor can execute this VPOR (dispatches 3/cycle) faster
            // than waiting for the VPMOVMSKB (1/cycle) of both data to check
            // their masks
            __m128i ored = _mm_or_si128(data1, data2);
            bool any = _mm_movemask_epi8(ored);

            // store everything, even mojibake
            __m256i extended1 = _mm256_cvtepu8_epi16(data1);
            __m256i extended2 = _mm256_cvtepu8_epi16(data2);
            _mm256_storeu_si256(reinterpret_cast<__m256i *>(dst), extended1);
            _mm256_storeu_si256(reinterpret_cast<__m256i *>(dst) + 1, extended2);

            uint n1 = _mm_movemask_epi8(data1);
            uint n2 = _mm_movemask_epi8(data2);
            struct R {
                uint n1, n2;
                bool any;
                operator bool() const { return any; }
                operator uint() const { return n1|(n2 << 16); }
            };
            return R{ n1, n2, any };
        };

        if constexpr (Cpu & CpuFeatureAVX512VL) {
            // with AVX512/AXV10, we always process everything
            if (end - src <= Step) {
                __mmask32 mask = _bzhi_u32(-1, uint(end - src));
                __m256i data = _mm256_maskz_loadu_epi8(mask, src);
                __mmask32 nonAscii = _mm256_mask_cmple_epi8_mask(mask, data, _mm256_setzero_si256());

                // store everything, even mojibake
                __m256i extended1 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(data));
                __m256i extended2 = _mm256_cvtepu8_epi16(_mm256_extracti64x2_epi64(data, 1));
                _mm256_mask_storeu_epi16(dst, mask, extended1);
                _mm256_mask_storeu_epi16(dst + Step/2, mask >> 16, extended2);
                if (nonAscii)
                    return maybeFoundNonAscii(nonAscii);
                adjustToEnd();
                return true;
            }
        }

        if (end - src >= Step) {
            // do 32 characters at a time
            qptrdiff offset = 0;
            for ( ; offset + Step < end - src; offset += Step) {
                auto r = process32Chars(dst + offset, src + offset);
                if (r)
                    return maybeFoundNonAscii(r, offset);
            }

            // do 32 characters again, possibly overlapping with the loop above
            adjustToEnd();
            auto r = process32Chars(dst - Step, src - Step);
            return maybeFoundNonAscii(r, -Step);
        }
    }

    constexpr qsizetype Step = 16;
    if (end - src >= Step) {
        qptrdiff offset = 0;
        for ( ; offset + Step < end - src; offset += Step) {
            ushort n = process16Chars(dst + offset, src + offset);
            if (n)
                return maybeFoundNonAscii(n, offset);
            if (Cpu & CpuFeatureAVX2)
                break;      // we can only ever loop once because of the code above
        }

        // do one chunk again, possibly overlapping with the loop above
        adjustToEnd();
        return maybeFoundNonAscii(process16Chars(dst - Step, src - Step), -Step);
    }

#  if !defined(__OPTIMIZE_SIZE__)
    if (end - src >= 8) {
        __m128i data = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(src));
        __m128i data2 = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(end - 8));
        uint n = _mm_movemask_epi8(data) & 0xff;
        // store everything, even mojibake
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dst), _mm_unpacklo_epi8(data, _mm_setzero_si128()));
        if (n)
            return maybeFoundNonAscii(n);

        // do one chunk again, possibly overlapping the above
        adjustToEnd();
        n = _mm_movemask_epi8(data2) & 0xff;
        data2 = _mm_unpacklo_epi8(data2, _mm_setzero_si128());
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dst - 8), data2);
        return maybeFoundNonAscii(n, -8);
    }
    if (end - src >= 4) {
        __m128i data = _mm_cvtsi32_si128(qFromUnaligned<quint32>(src));
        __m128i data2 = _mm_cvtsi32_si128(qFromUnaligned<quint32>(end - 4));
        uchar n = uchar(_mm_movemask_epi8(data) & 0xf);
        // store everything, even mojibake
        data = _mm_unpacklo_epi8(data, _mm_setzero_si128());
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dst), data);
        if (n)
            return maybeFoundNonAscii(n);

        // do one chunk again, possibly overlapping the above
        adjustToEnd();
        n = uchar(_mm_movemask_epi8(data2) & 0xf);
        data2 = _mm_unpacklo_epi8(data2, _mm_setzero_si128());
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dst - 4), data2);
        return maybeFoundNonAscii(n, -4);
    }
#endif

    return src == end;
}

static inline const uchar *simdFindNonAscii(const uchar *src, const uchar *end, const uchar *&nextAscii)
{
#ifdef __AVX2__
    // do 32 characters at a time
    // (this is similar to simdTestMask in qstring.cpp)
    const __m256i mask = _mm256_set1_epi8(char(0x80));
    for ( ; end - src >= 32; src += 32) {
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src));
        if (_mm256_testz_si256(mask, data))
            continue;

        uint n = _mm256_movemask_epi8(data);
        Q_ASSERT(n);

        // find the next probable ASCII character
        // we don't want to load 32 bytes again in this loop if we know there are non-ASCII
        // characters still coming
        nextAscii = src + qBitScanReverse(n) + 1;

        // return the non-ASCII character
        return src + qCountTrailingZeroBits(n);
    }
#endif

    // do sixteen characters at a time
    for ( ; end - src >= 16; src += 16) {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src));

        // check if everything is ASCII
        // movemask extracts the high bit of every byte, so n is non-zero if something isn't ASCII
        uint n = _mm_movemask_epi8(data);
        if (!n)
            continue;

        // find the next probable ASCII character
        // we don't want to load 16 bytes again in this loop if we know there are non-ASCII
        // characters still coming
        nextAscii = src + qBitScanReverse(n) + 1;

        // return the non-ASCII character
        return src + qCountTrailingZeroBits(n);
    }

    // do four characters at a time
    for ( ; end - src >= 4; src += 4) {
        quint32 data = qFromUnaligned<quint32>(src);
        data &= 0x80808080U;
        if (!data)
            continue;

        // We don't try to guess which of the three bytes is ASCII and which
        // one isn't. The chance that at least two of them are non-ASCII is
        // better than 75%.
        nextAscii = src;
        return src;
    }
    nextAscii = end;
    return src;
}

// Compare only the US-ASCII beginning of [src8, end8) and [src16, end16)
// and advance src8 and src16 to the first character that could not be compared
static void simdCompareAscii(const qchar8_t *&src8, const qchar8_t *end8, const char16_t *&src16, const char16_t *end16)
{
    int bitSpacing = 1;
    qptrdiff len = qMin(end8 - src8, end16 - src16);
    qptrdiff offset = 0;
    uint mask = 0;

    // do sixteen characters at a time
    for ( ; offset + 16 < len; offset += 16) {
        __m128i data8 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src8 + offset));
#ifdef __AVX2__
        // AVX2 version, use 256-bit registers and VPMOVXZBW
        __m256i data16 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src16 + offset));

        // expand US-ASCII as if it were Latin1 and confirm it's US-ASCII
        __m256i datax8 = _mm256_cvtepu8_epi16(data8);
        mask = _mm256_movemask_epi8(datax8);
        if (mask)
            break;

        // compare Latin1 to UTF-16
        __m256i latin1cmp = _mm256_cmpeq_epi16(datax8, data16);
        mask = ~_mm256_movemask_epi8(latin1cmp);
        if (mask)
            break;
#else
        // non-AVX2 code
        __m128i datalo16 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src16 + offset));
        __m128i datahi16 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src16 + offset) + 1);

        // expand US-ASCII as if it were Latin1, we'll confirm later
        __m128i datalo8 = _mm_unpacklo_epi8(data8, _mm_setzero_si128());
        __m128i datahi8 = _mm_unpackhi_epi8(data8, _mm_setzero_si128());

        // compare Latin1 to UTF-16
        __m128i latin1cmplo = _mm_cmpeq_epi16(datalo8, datalo16);
        __m128i latin1cmphi = _mm_cmpeq_epi16(datahi8, datahi16);
        mask = _mm_movemask_epi8(latin1cmphi) << 16;
        mask |= ushort(_mm_movemask_epi8(latin1cmplo));
        mask = ~mask;
        if (mask)
            break;

        // confirm it was US-ASCII
        mask = _mm_movemask_epi8(data8);
        if (mask) {
            bitSpacing = 0;
            break;
        }
#endif
    }

    // helper for comparing 4 or 8 characters
    auto cmp_lt_16 = [&mask, &offset](int n, __m128i data8, __m128i data16) {
        // n = 4  ->  sizemask = 0xff
        // n = 8  ->  sizemask = 0xffff
        unsigned sizemask = (1U << (2 * n)) - 1;

        // expand as if Latin1
        data8 = _mm_unpacklo_epi8(data8, _mm_setzero_si128());

        // compare and confirm it's US-ASCII
        __m128i latin1cmp = _mm_cmpeq_epi16(data8, data16);
        mask = ~_mm_movemask_epi8(latin1cmp) & sizemask;
        mask |= _mm_movemask_epi8(data8);
        if (mask == 0)
            offset += n;
    };

    // do eight characters at a time
    if (mask == 0 && offset + 8 < len) {
        __m128i data8 = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(src8 + offset));
        __m128i data16 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src16 + offset));
        cmp_lt_16(8, data8, data16);
    }

    // do four characters
    if (mask == 0 && offset + 4 < len) {
        __m128i data8 = _mm_cvtsi32_si128(qFromUnaligned<quint32>(src8 + offset));
        __m128i data16 = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(src16 + offset));
        cmp_lt_16(4, data8, data16);
    }

    // correct the source pointers to point to the first character we couldn't deal with
    if (mask)
        offset += qCountTrailingZeroBits(mask) >> bitSpacing;
    src8 += offset;
    src16 += offset;
}
#elif defined(__ARM_NEON__)
static inline bool simdEncodeAscii(uchar *&dst, const char16_t *&nextAscii, const char16_t *&src, const char16_t *end)
{
    uint16x8_t maxAscii = vdupq_n_u16(0x7f);
    uint16x8_t mask1 = qvsetq_n_u16(1, 1 << 2, 1 << 4, 1 << 6, 1 << 8, 1 << 10, 1 << 12, 1 << 14 );
    uint16x8_t mask2 = vshlq_n_u16(mask1, 1);

    // do sixteen characters at a time
    for ( ; end - src >= 16; src += 16, dst += 16) {
        // load 2 lanes (or: "load interleaved")
        uint16x8x2_t in = vld2q_u16(reinterpret_cast<const uint16_t *>(src));

        // check if any of the elements > 0x7f, select 1 bit per element (element 0 -> bit 0, element 1 -> bit 1, etc),
        // add those together into a scalar, and merge the scalars.
        uint16_t nonAscii = vaddvq_u16(vandq_u16(vcgtq_u16(in.val[0], maxAscii), mask1))
                          | vaddvq_u16(vandq_u16(vcgtq_u16(in.val[1], maxAscii), mask2));

        // merge the two lanes by shifting the values of the second by 8 and inserting them
        uint16x8_t out = vsliq_n_u16(in.val[0], in.val[1], 8);

        // store, even if there are non-ASCII characters here
        vst1q_u8(dst, vreinterpretq_u8_u16(out));

        if (nonAscii) {
            // find the next probable ASCII character
            // we don't want to load 32 bytes again in this loop if we know there are non-ASCII
            // characters still coming
            nextAscii = src + qBitScanReverse(nonAscii) + 1;

            nonAscii = qCountTrailingZeroBits(nonAscii);
            dst += nonAscii;
            src += nonAscii;
            return false;
        }
    }
    return src == end;
}

static inline bool simdDecodeAscii(char16_t *&dst, const uchar *&nextAscii, const uchar *&src, const uchar *end)
{
    // do eight characters at a time
    uint8x8_t msb_mask = vdup_n_u8(0x80);
    uint8x8_t add_mask = qvset_n_u8(1, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7 );
    for ( ; end - src >= 8; src += 8, dst += 8) {
        uint8x8_t c = vld1_u8(src);
        uint8_t n = vaddv_u8(vand_u8(vcge_u8(c, msb_mask), add_mask));
        if (!n) {
            // store
            vst1q_u16(reinterpret_cast<uint16_t *>(dst), vmovl_u8(c));
            continue;
        }

        // copy the front part that is still ASCII
        while (!(n & 1)) {
            *dst++ = *src++;
            n >>= 1;
        }

        // find the next probable ASCII character
        // we don't want to load 16 bytes again in this loop if we know there are non-ASCII
        // characters still coming
        n = qBitScanReverse(n);
        nextAscii = src + n + 1;
        return false;

    }
    return src == end;
}

static inline const uchar *simdFindNonAscii(const uchar *src, const uchar *end, const uchar *&nextAscii)
{
    // The SIMD code below is untested, so just force an early return until
    // we've had the time to verify it works.
    nextAscii = end;
    return src;

    // do eight characters at a time
    uint8x8_t msb_mask = vdup_n_u8(0x80);
    uint8x8_t add_mask = qvset_n_u8(1, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7);
    for ( ; end - src >= 8; src += 8) {
        uint8x8_t c = vld1_u8(src);
        uint8_t n = vaddv_u8(vand_u8(vcge_u8(c, msb_mask), add_mask));
        if (!n)
            continue;

        // find the next probable ASCII character
        // we don't want to load 16 bytes again in this loop if we know there are non-ASCII
        // characters still coming
        nextAscii = src + qBitScanReverse(n) + 1;

        // return the non-ASCII character
        return src + qCountTrailingZeroBits(n);
    }
    nextAscii = end;
    return src;
}

static void simdCompareAscii(const qchar8_t *&, const qchar8_t *, const char16_t *&, const char16_t *)
{
}
#else
static inline bool simdEncodeAscii(uchar *, const char16_t *, const char16_t *, const char16_t *)
{
    return false;
}

static inline bool simdDecodeAscii(char16_t *, const uchar *, const uchar *, const uchar *)
{
    return false;
}

static inline const uchar *simdFindNonAscii(const uchar *src, const uchar *end, const uchar *&nextAscii)
{
    nextAscii = end;
    return src;
}

static void simdCompareAscii(const qchar8_t *&, const qchar8_t *, const char16_t *&, const char16_t *)
{
}
#endif

enum { HeaderDone = 1 };

template <typename OnErrorLambda> Q_ALWAYS_INLINE
char *QUtf8::convertFromUnicode(char *out, QStringView in, OnErrorLambda &&onError) noexcept
{
    qsizetype len = in.size();

    uchar *dst = reinterpret_cast<uchar *>(out);
    const char16_t *src = reinterpret_cast<const char16_t *>(in.data());
    const char16_t *const end = src + len;

    while (src != end) {
        const char16_t *nextAscii = end;
        if (simdEncodeAscii(dst, nextAscii, src, end))
            break;

        do {
            char16_t u = *src++;
            int res = QUtf8Functions::toUtf8<QUtf8BaseTraits>(u, dst, src, end);
            if (Q_UNLIKELY(res < 0))
                onError(dst, u, res);
        } while (src < nextAscii);
    }

    return reinterpret_cast<char *>(dst);
}

char *QUtf8::convertFromUnicode(char *dst, QStringView in) noexcept
{
    return convertFromUnicode(dst, in, [](auto *dst, ...) {
        // encoding error - append '?'
        *dst++ = '?';
    });
}

QByteArray QUtf8::convertFromUnicode(QStringView in)
{
    qsizetype len = in.size();

    // create a QByteArray with the worst case scenario size
    QByteArray result(len * 3, Qt::Uninitialized);
    char *dst = const_cast<char *>(result.constData());
    dst = convertFromUnicode(dst, in);
    result.truncate(dst - result.constData());
    return result;
}

QByteArray QUtf8::convertFromUnicode(QStringView in, QStringConverter::State *state)
{
    QByteArray ba(3*in.size() +3, Qt::Uninitialized);
    char *end = convertFromUnicode(ba.data(), in, state);
    ba.truncate(end - ba.data());
    return ba;
}

char *QUtf8::convertFromUnicode(char *out, QStringView in, QStringConverter::State *state)
{
    Q_ASSERT(state);
    qsizetype len = in.size();
    if (!len)
        return out;

    auto appendReplacementChar = [state](uchar *cursor) -> uchar * {
        if (state->flags & QStringConverter::Flag::ConvertInvalidToNull) {
            *cursor++ = 0;
        } else {
            // QChar::replacement encoded in utf8
            *cursor++ = 0xef;
            *cursor++ = 0xbf;
            *cursor++ = 0xbd;
        }
        return cursor;
    };

    uchar *cursor = reinterpret_cast<uchar *>(out);
    const char16_t *src = in.utf16();
    const char16_t *const end = src + len;

    if (!(state->flags & QStringDecoder::Flag::Stateless)) {
        if (state->remainingChars) {
            int res = QUtf8Functions::toUtf8<QUtf8BaseTraits>(state->state_data[0], cursor, src, end);
            if (res < 0)
                cursor = appendReplacementChar(cursor);
            state->state_data[0] = 0;
            state->remainingChars = 0;
        } else if (!(state->internalState & HeaderDone) && state->flags & QStringConverter::Flag::WriteBom) {
            // append UTF-8 BOM
            *cursor++ = utf8bom[0];
            *cursor++ = utf8bom[1];
            *cursor++ = utf8bom[2];
            state->internalState |= HeaderDone;
        }
    }

    out = reinterpret_cast<char *>(cursor);
    return convertFromUnicode(out, { src, end }, [&](uchar *&cursor, char16_t uc, int res) {
        if (res == QUtf8BaseTraits::Error) {
            // encoding error
            ++state->invalidChars;
            cursor = appendReplacementChar(cursor);
        } else if (res == QUtf8BaseTraits::EndOfString) {
            if (state->flags & QStringConverter::Flag::Stateless) {
                ++state->invalidChars;
                cursor = appendReplacementChar(cursor);
            } else {
                state->remainingChars = 1;
                state->state_data[0] = uc;
            }
        }
    });
}

char *QUtf8::convertFromLatin1(char *out, QLatin1StringView in)
{
    // ### SIMD-optimize:
    for (uchar ch : in) {
        if (ch < 128) {
            *out++ = ch;
        } else {
            // as per https://en.wikipedia.org/wiki/UTF-8#Encoding, 2nd row
            *out++ = 0b110'0'0000u | (ch >> 6);
            *out++ = 0b10'00'0000u | (ch & 0b0011'1111);
        }
    }
    return out;
}

QString QUtf8::convertToUnicode(QByteArrayView in)
{
    // UTF-8 to UTF-16 always needs the exact same number of words or less:
    //    UTF-8     UTF-16
    //   1 byte     1 word
    //   2 bytes    1 word
    //   3 bytes    1 word
    //   4 bytes    2 words (one surrogate pair)
    // That is, we'll use the full buffer if the input is US-ASCII (1-byte UTF-8),
    // half the buffer for U+0080-U+07FF text (e.g., Greek, Cyrillic, Arabic) or
    // non-BMP text, and one third of the buffer for U+0800-U+FFFF text (e.g, CJK).
    //
    // The table holds for invalid sequences too: we'll insert one replacement char
    // per invalid byte.
    QString result(in.size(), Qt::Uninitialized);
    QChar *data = const_cast<QChar*>(result.constData()); // we know we're not shared
    const QChar *end = convertToUnicode(data, in);
    result.truncate(end - data);
    return result;
}

/*! \internal
    \since 6.6
    \overload

    Converts the UTF-8 sequence of bytes viewed by \a in to a sequence of
    QChar starting at \a dst in the destination buffer. The buffer is expected
    to be large enough to hold the result. An upper bound for the size of the
    buffer is \c in.size() QChars.

    If, during decoding, an error occurs, a QChar::ReplacementCharacter is
    written.

    Returns a pointer to one past the last QChar written.

    This function never throws.

    For QChar buffers, instead of casting manually, you can use the static
    QUtf8::convertToUnicode(QChar *, QByteArrayView) directly.
*/
char16_t *QUtf8::convertToUnicode(char16_t *dst, QByteArrayView in) noexcept
{
    // check if have to skip a BOM
    auto bom = QByteArrayView::fromArray(utf8bom);
    if (in.size() >= bom.size() && in.first(bom.size()) == bom)
        in.slice(sizeof(utf8bom));

    return convertToUnicode(dst, in, [](char16_t *&dst, ...) {
        // decoding error
        *dst++ = QChar::ReplacementCharacter;
        return true;        // continue decoding
    });
}

template <typename OnErrorLambda> Q_ALWAYS_INLINE char16_t *
QUtf8::convertToUnicode(char16_t *dst, QByteArrayView in, OnErrorLambda &&onError) noexcept
{
    const uchar *const start = reinterpret_cast<const uchar *>(in.data());
    const uchar *src = start;
    const uchar *end = src + in.size();

    // attempt to do a full decoding in SIMD
    const uchar *nextAscii = end;
    while (src < end) {
        nextAscii = end;
        if (simdDecodeAscii(dst, nextAscii, src, end))
            break;

        do {
            uchar b = *src++;
            const qsizetype res = QUtf8Functions::fromUtf8<QUtf8BaseTraits>(b, dst, src, end);
            if (Q_LIKELY(res >= 0))
                continue;
            // decoding error
            if (!onError(dst, src, res))
                return dst;
        } while (src < nextAscii);
    }

    return dst;
}

QString QUtf8::convertToUnicode(QByteArrayView in, QStringConverter::State *state)
{
    // See above for buffer requirements for stateless decoding. However, that
    // fails if the state is not empty. The following situations can add to the
    // requirements:
    //  state contains      chars starts with           requirement
    //   1 of 2 bytes       valid continuation          0
    //   2 of 3 bytes       same                        0
    //   3 bytes of 4       same                        +1 (need to insert surrogate pair)
    //   1 of 2 bytes       invalid continuation        +1 (need to insert replacement and restart)
    //   2 of 3 bytes       same                        +1 (same)
    //   3 of 4 bytes       same                        +1 (same)
    QString result(in.size() + 1, Qt::Uninitialized);
    QChar *end = convertToUnicode(result.data(), in, state);
    result.truncate(end - result.constData());
    return result;
}

char16_t *QUtf8::convertToUnicode(char16_t *dst, QByteArrayView in, QStringConverter::State *state)
{
    qsizetype len = in.size();

    Q_ASSERT(state);
    if (!len)
        return dst;


    char16_t replacement = QChar::ReplacementCharacter;
    if (state->flags & QStringConverter::Flag::ConvertInvalidToNull)
        replacement = QChar::Null;

    qsizetype res;

    const uchar *src = reinterpret_cast<const uchar *>(in.data());
    const uchar *end = src + len;

    if (!(state->flags & QStringConverter::Flag::Stateless)) {
        bool headerdone = state->internalState & HeaderDone || state->flags & QStringConverter::Flag::ConvertInitialBom;
        if (state->remainingChars || !headerdone) {
            // handle incoming state first
            uchar remainingCharsData[4]; // longest UTF-8 sequence possible
            qsizetype remainingCharsCount = state->remainingChars;
            qsizetype newCharsToCopy = qMin<qsizetype>(sizeof(remainingCharsData) - remainingCharsCount, end - src);

            memset(remainingCharsData, 0, sizeof(remainingCharsData));
            memcpy(remainingCharsData, &state->state_data[0], remainingCharsCount);
            memcpy(remainingCharsData + remainingCharsCount, src, newCharsToCopy);

            const uchar *begin = &remainingCharsData[1];
            res = QUtf8Functions::fromUtf8<QUtf8BaseTraits>(remainingCharsData[0], dst, begin,
                    static_cast<const uchar *>(remainingCharsData) + remainingCharsCount + newCharsToCopy);
            if (res == QUtf8BaseTraits::Error) {
                ++state->invalidChars;
                *dst++ = replacement;
                ++src;
            } else if (res == QUtf8BaseTraits::EndOfString) {
                // if we got EndOfString again, then there were too few bytes in src;
                // copy to our state and return
                state->remainingChars = remainingCharsCount + newCharsToCopy;
                memcpy(&state->state_data[0], remainingCharsData, state->remainingChars);
                return dst;
            } else if (!headerdone) {
                // eat the UTF-8 BOM
                if (dst[-1] == 0xfeff)
                    --dst;
            }
            state->internalState |= HeaderDone;

            // adjust src now that we have maybe consumed a few chars
            if (res >= 0) {
                Q_ASSERT(res > remainingCharsCount);
                src += res - remainingCharsCount;
            }
        }
    } else if (!(state->flags & QStringConverter::Flag::ConvertInitialBom)) {
        // stateless, remove initial BOM
        if (len > 2 && src[0] == utf8bom[0] && src[1] == utf8bom[1] && src[2] == utf8bom[2])
            // skip BOM
            src += 3;
    }

    // main body, stateless decoding
    res = 0;
    dst = convertToUnicode(dst, { src, end }, [&](char16_t *&dst, const uchar *src_, int res_) {
        res = res_;
        src = src_;
        if (res == QUtf8BaseTraits::Error) {
            res = 0;
            ++state->invalidChars;
            *dst++ = replacement;
        }
        return res == 0;    // continue if plain decoding error
    });

    if (res == QUtf8BaseTraits::EndOfString) {
        // unterminated UTF sequence
        if (state->flags & QStringConverter::Flag::Stateless) {
            *dst++ = QChar::ReplacementCharacter;
            ++state->invalidChars;
            while (src++ < end) {
                *dst++ = QChar::ReplacementCharacter;
                ++state->invalidChars;
            }
            state->remainingChars = 0;
        } else {
            --src; // unread the byte in ch
            state->remainingChars = end - src;
            memcpy(&state->state_data[0], src, end - src);
        }
    } else {
        state->remainingChars = 0;
    }

    return dst;
}

struct QUtf8NoOutputTraits : public QUtf8BaseTraitsNoAscii
{
    struct NoOutput {};
    static void appendUtf16(const NoOutput &, char16_t) {}
    static void appendUcs4(const NoOutput &, char32_t) {}
};

QUtf8::ValidUtf8Result QUtf8::isValidUtf8(QByteArrayView in)
{
    const uchar *src = reinterpret_cast<const uchar *>(in.data());
    const uchar *end = src + in.size();
    const uchar *nextAscii = src;
    bool isValidAscii = true;

    while (src < end) {
        if (src >= nextAscii)
            src = simdFindNonAscii(src, end, nextAscii);
        if (src == end)
            break;

        do {
            uchar b = *src++;
            if ((b & 0x80) == 0)
                continue;

            isValidAscii = false;
            QUtf8NoOutputTraits::NoOutput output;
            const qsizetype res = QUtf8Functions::fromUtf8<QUtf8NoOutputTraits>(b, output, src, end);
            if (res < 0) {
                // decoding error
                return { false, false };
            }
        } while (src < nextAscii);
    }

    return { true, isValidAscii };
}

int QUtf8::compareUtf8(QByteArrayView utf8, QStringView utf16, Qt::CaseSensitivity cs) noexcept
{
    auto src1 = reinterpret_cast<const qchar8_t *>(utf8.data());
    auto end1 = src1 + utf8.size();
    auto src2 = reinterpret_cast<const char16_t *>(utf16.data());
    auto end2 = src2 + utf16.size();

    do {
        simdCompareAscii(src1, end1, src2, end2);

        if (src1 < end1 && src2 < end2) {
            char32_t uc1 = QUtf8Functions::nextUcs4FromUtf8(src1, end1);
            char32_t uc2 = *src2++;

            if (uc1 >= 0x80) {
                // Only decode the UTF-16 surrogate pair if the UTF-8 code point
                // wasn't US-ASCII (a surrogate cannot match US-ASCII).
                if (QChar::isHighSurrogate(uc2) && src2 < end2 && QChar::isLowSurrogate(*src2))
                    uc2 = QChar::surrogateToUcs4(uc2, *src2++);
            }
            if (cs == Qt::CaseInsensitive) {
                uc1 = QChar::toCaseFolded(uc1);
                uc2 = QChar::toCaseFolded(uc2);
            }
            if (uc1 != uc2)
                return int(uc1) - int(uc2);
        }
    } while (src1 < end1 && src2 < end2);

    // the shorter string sorts first
    return (end1 > src1) - int(end2 > src2);
}

int QUtf8::compareUtf8(QByteArrayView utf8, QLatin1StringView s, Qt::CaseSensitivity cs)
{
    auto src1 = reinterpret_cast<const qchar8_t *>(utf8.data());
    auto end1 = src1 + utf8.size();
    auto src2 = reinterpret_cast<const uchar *>(s.latin1());
    auto end2 = src2 + s.size();

    while (src1 < end1 && src2 < end2) {
        char32_t uc1 = QUtf8Functions::nextUcs4FromUtf8(src1, end1);
        char32_t uc2 = *src2++;
        if (cs == Qt::CaseInsensitive) {
            uc1 = QChar::toCaseFolded(uc1);
            uc2 = QChar::toCaseFolded(uc2);
        }
        if (uc1 != uc2)
            return int(uc1) - int(uc2);
    }

    // the shorter string sorts first
    return (end1 > src1) - (end2 > src2);
}

int QUtf8::compareUtf8(QByteArrayView lhs, QByteArrayView rhs, Qt::CaseSensitivity cs) noexcept
{
    if (lhs.isEmpty())
        return qt_lencmp(0, rhs.size());

    if (rhs.isEmpty())
        return qt_lencmp(lhs.size(), 0);

    if (cs == Qt::CaseSensitive) {
        const auto l = std::min(lhs.size(), rhs.size());
        int r = memcmp(lhs.data(), rhs.data(), l);
        return r ? r : qt_lencmp(lhs.size(), rhs.size());
    }

    auto src1 = reinterpret_cast<const qchar8_t *>(lhs.data());
    auto end1 = src1 + lhs.size();
    auto src2 = reinterpret_cast<const qchar8_t *>(rhs.data());
    auto end2 = src2 + rhs.size();

    while (src1 < end1 && src2 < end2) {
        char32_t uc1 = QUtf8Functions::nextUcs4FromUtf8(src1, end1);
        char32_t uc2 = QUtf8Functions::nextUcs4FromUtf8(src2, end2);

        uc1 = QChar::toCaseFolded(uc1);
        uc2 = QChar::toCaseFolded(uc2);
        if (uc1 != uc2)
            return int(uc1) - int(uc2);
    }

    // the shorter string sorts first
    return (end1 > src1) - (end2 > src2);
}

#ifndef QT_BOOTSTRAPPED
QByteArray QUtf16::convertFromUnicode(QStringView in, QStringConverter::State *state, DataEndianness endian)
{
    bool writeBom = !(state->internalState & HeaderDone) && state->flags & QStringConverter::Flag::WriteBom;
    qsizetype length = 2 * in.size();
    if (writeBom)
        length += 2;

    QByteArray d(length, Qt::Uninitialized);
    char *end = convertFromUnicode(d.data(), in, state, endian);
    Q_ASSERT(end - d.constData() == d.size());
    Q_UNUSED(end);
    return d;
}

char *QUtf16::convertFromUnicode(char *out, QStringView in, QStringConverter::State *state, DataEndianness endian)
{
    Q_ASSERT(state);
    bool writeBom = !(state->internalState & HeaderDone) && state->flags & QStringConverter::Flag::WriteBom;

    if (endian == DetectEndianness)
        endian = (QSysInfo::ByteOrder == QSysInfo::BigEndian) ? BigEndianness : LittleEndianness;

    if (writeBom) {
        // set them up the BOM
        QChar bom(QChar::ByteOrderMark);
        if (endian == BigEndianness)
            qToBigEndian(bom.unicode(), out);
        else
            qToLittleEndian(bom.unicode(), out);
        out += 2;
    }
    if (endian == BigEndianness)
        qToBigEndian<char16_t>(in.data(), in.size(), out);
    else
        qToLittleEndian<char16_t>(in.data(), in.size(), out);

    state->remainingChars = 0;
    state->internalState |= HeaderDone;
    return out + 2*in.size();
}

QString QUtf16::convertToUnicode(QByteArrayView in, QStringConverter::State *state, DataEndianness endian)
{
    QString result((in.size() + 1) >> 1, Qt::Uninitialized); // worst case
    QChar *qch = convertToUnicode(result.data(), in, state, endian);
    result.truncate(qch - result.constData());
    return result;
}

QChar *QUtf16::convertToUnicode(QChar *out, QByteArrayView in, QStringConverter::State *state, DataEndianness endian)
{
    qsizetype len = in.size();
    const char *chars = in.data();

    Q_ASSERT(state);

    if (endian == DetectEndianness)
        endian = (DataEndianness)state->state_data[Endian];

    const char *end = chars + len;

    // make sure we can decode at least one char
    if (state->remainingChars + len < 2) {
        if (len) {
            Q_ASSERT(state->remainingChars == 0 && len == 1);
            state->remainingChars = 1;
            state->state_data[Data] = *chars;
        }
        return out;
    }

    bool headerdone = state && state->internalState & HeaderDone;
    if (state->flags & QStringConverter::Flag::ConvertInitialBom)
        headerdone = true;

    if (!headerdone || state->remainingChars) {
        uchar buf;
        if (state->remainingChars)
            buf = state->state_data[Data];
        else
            buf = *chars++;

        // detect BOM, set endianness
        state->internalState |= HeaderDone;
        QChar ch(buf, *chars++);
        if (endian == DetectEndianness) {
            // someone set us up the BOM
            if (ch == QChar::ByteOrderSwapped) {
                endian = BigEndianness;
            } else if (ch == QChar::ByteOrderMark) {
                endian = LittleEndianness;
            } else {
                if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
                    endian = BigEndianness;
                } else {
                    endian = LittleEndianness;
                }
            }
        }
        if (endian == BigEndianness)
            ch = QChar::fromUcs2((ch.unicode() >> 8) | ((ch.unicode() & 0xff) << 8));
        if (headerdone || ch != QChar::ByteOrderMark)
            *out++ = ch;
    } else if (endian == DetectEndianness) {
        endian = (QSysInfo::ByteOrder == QSysInfo::BigEndian) ? BigEndianness : LittleEndianness;
    }

    qsizetype nPairs = (end - chars) >> 1;
    if (endian == BigEndianness)
        qFromBigEndian<char16_t>(chars, nPairs, out);
    else
        qFromLittleEndian<char16_t>(chars, nPairs, out);
    out += nPairs;

    state->state_data[Endian] = endian;
    state->remainingChars = 0;
    if ((end - chars) & 1) {
        if (state->flags & QStringConverter::Flag::Stateless) {
            *out++ = state->flags & QStringConverter::Flag::ConvertInvalidToNull ? QChar::Null : QChar::ReplacementCharacter;
        } else {
            state->remainingChars = 1;
            state->state_data[Data] = *(end - 1);
        }
    } else {
        state->state_data[Data] = 0;
    }

    return out;
}

QByteArray QUtf32::convertFromUnicode(QStringView in, QStringConverter::State *state, DataEndianness endian)
{
    bool writeBom = !(state->internalState & HeaderDone) && state->flags & QStringConverter::Flag::WriteBom;
    qsizetype length =  4*in.size();
    if (writeBom)
        length += 4;
    QByteArray ba(length, Qt::Uninitialized);
    char *end = convertFromUnicode(ba.data(), in, state, endian);
    ba.truncate(end - ba.constData());
    return ba;
}

char *QUtf32::convertFromUnicode(char *out, QStringView in, QStringConverter::State *state, DataEndianness endian)
{
    Q_ASSERT(state);

    bool writeBom = !(state->internalState & HeaderDone) && state->flags & QStringConverter::Flag::WriteBom;
    if (endian == DetectEndianness)
        endian = (QSysInfo::ByteOrder == QSysInfo::BigEndian) ? BigEndianness : LittleEndianness;

    if (writeBom) {
        // set them up the BOM
        if (endian == BigEndianness) {
            out[0] = 0;
            out[1] = 0;
            out[2] = (char)0xfe;
            out[3] = (char)0xff;
        } else {
            out[0] = (char)0xff;
            out[1] = (char)0xfe;
            out[2] = 0;
            out[3] = 0;
        }
        out += 4;
        state->internalState |= HeaderDone;
    }

    const QChar *uc = in.data();
    const QChar *end = in.data() + in.size();
    QChar ch;
    char32_t ucs4;
    if (state->remainingChars == 1) {
        auto character = state->state_data[Data];
        Q_ASSERT(character <= 0xFFFF);
        ch = QChar(character);
        // this is ugly, but shortcuts a whole lot of logic that would otherwise be required
        state->remainingChars = 0;
        goto decode_surrogate;
    }

    while (uc < end) {
        ch = *uc++;
        if (Q_LIKELY(!ch.isSurrogate())) {
            ucs4 = ch.unicode();
        } else if (Q_LIKELY(ch.isHighSurrogate())) {
decode_surrogate:
            if (uc == end) {
                if (state->flags & QStringConverter::Flag::Stateless) {
                    ucs4 = state->flags & QStringConverter::Flag::ConvertInvalidToNull ? 0 : QChar::ReplacementCharacter;
                } else {
                    state->remainingChars = 1;
                    state->state_data[Data] = ch.unicode();
                    return out;
                }
            } else if (uc->isLowSurrogate()) {
                ucs4 = QChar::surrogateToUcs4(ch, *uc++);
            } else {
                ucs4 = state->flags & QStringConverter::Flag::ConvertInvalidToNull ? 0 : QChar::ReplacementCharacter;
            }
        } else {
            ucs4 = state->flags & QStringConverter::Flag::ConvertInvalidToNull ? 0 : QChar::ReplacementCharacter;
        }
        if (endian == BigEndianness)
            qToBigEndian(ucs4, out);
        else
            qToLittleEndian(ucs4, out);
        out += 4;
    }

    return out;
}

QString QUtf32::convertToUnicode(QByteArrayView in, QStringConverter::State *state, DataEndianness endian)
{
    QString result;
    result.resize((in.size() + 7) >> 1); // worst case
    QChar *end = convertToUnicode(result.data(), in, state, endian);
    result.truncate(end - result.constData());
    return result;
}

QChar *QUtf32::convertToUnicode(QChar *out, QByteArrayView in, QStringConverter::State *state, DataEndianness endian)
{
    qsizetype len = in.size();
    const char *chars = in.data();

    Q_ASSERT(state);
    if (endian == DetectEndianness)
        endian = (DataEndianness)state->state_data[Endian];

    const char *end = chars + len;

    uchar tuple[4];
    memcpy(tuple, &state->state_data[Data], 4);

    // make sure we can decode at least one char
    if (state->remainingChars + len < 4) {
        if (len) {
            while (chars < end) {
                tuple[state->remainingChars] = *chars;
                ++state->remainingChars;
                ++chars;
            }
            Q_ASSERT(state->remainingChars < 4);
            memcpy(&state->state_data[Data], tuple, 4);
        }
        return out;
    }

    bool headerdone = state->internalState & HeaderDone;
    if (state->flags & QStringConverter::Flag::ConvertInitialBom)
        headerdone = true;

    qsizetype num = state->remainingChars;
    state->remainingChars = 0;

    if (!headerdone || endian == DetectEndianness || num) {
        while (num < 4)
            tuple[num++] = *chars++;
        if (endian == DetectEndianness) {
            // someone set us up the BOM?
            if (tuple[0] == 0xff && tuple[1] == 0xfe && tuple[2] == 0 && tuple[3] == 0) {
                endian = LittleEndianness;
            } else if (tuple[0] == 0 && tuple[1] == 0 && tuple[2] == 0xfe && tuple[3] == 0xff) {
                endian = BigEndianness;
            } else if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
                endian = BigEndianness;
            } else {
                endian = LittleEndianness;
            }
        }
        char32_t code = (endian == BigEndianness) ? qFromBigEndian<char32_t>(tuple) : qFromLittleEndian<char32_t>(tuple);
        if (headerdone || code != QChar::ByteOrderMark) {
            if (QChar::requiresSurrogates(code)) {
                *out++ = QChar(QChar::highSurrogate(code));
                *out++ = QChar(QChar::lowSurrogate(code));
            } else {
                *out++ = QChar(code);
            }
        }
        num = 0;
    } else if (endian == DetectEndianness) {
        endian = (QSysInfo::ByteOrder == QSysInfo::BigEndian) ? BigEndianness : LittleEndianness;
    }
    state->state_data[Endian] = endian;
    state->internalState |= HeaderDone;

    while (chars < end) {
        tuple[num++] = *chars++;
        if (num == 4) {
            char32_t code = (endian == BigEndianness) ? qFromBigEndian<char32_t>(tuple) : qFromLittleEndian<char32_t>(tuple);
            for (char16_t c : QChar::fromUcs4(code))
                *out++ = c;
            num = 0;
        }
    }

    if (num) {
        if (state->flags & QStringDecoder::Flag::Stateless) {
            *out++ = QChar::ReplacementCharacter;
        } else {
            state->state_data[Endian] = endian;
            state->remainingChars = num;
            memcpy(&state->state_data[Data], tuple, 4);
        }
    }

    return out;
}
#endif // !QT_BOOTSTRAPPED

#if defined(Q_OS_WIN) && !defined(QT_BOOTSTRAPPED)
int QLocal8Bit::checkUtf8()
{
    return GetACP() == CP_UTF8 ? 1 : -1;
}

QString QLocal8Bit::convertToUnicode_sys(QByteArrayView in, QStringConverter::State *state)
{
    return convertToUnicode_sys(in, CP_ACP, state);
}

QString QLocal8Bit::convertToUnicode_sys(QByteArrayView in, quint32 codePage,
                                         QStringConverter::State *state)
{
    const char *mb = in.data();
    qsizetype mblen = in.size();

    Q_ASSERT(state);
    qsizetype &invalidChars = state->invalidChars;
    using Flag = QStringConverter::Flag;
    const bool useNullForReplacement = !!(state->flags & Flag::ConvertInvalidToNull);
    const char16_t replacementCharacter = useNullForReplacement ? QChar::Null
                                                                : QChar::ReplacementCharacter;
    if (state->flags & Flag::Stateless) {
        Q_ASSERT(state->remainingChars == 0);
        state = nullptr;
    }

    if (!mb || !mblen)
        return QString();

    // Use a local stack-buffer at first to allow us a decently large container
    // to avoid a lot of resizing, without also returning an overallocated
    // QString to the user for small strings.
    // Then we can be fast for small strings and take the hit of extra resizes
    // and measuring how much storage is needed for large strings.
    std::array<wchar_t, 4096> buf;
    wchar_t *out = buf.data();
    qsizetype outlen = buf.size();

    QString sp;

    // Return a pointer to storage where we have enough space for `size`
    const auto growOut = [&](qsizetype size) -> std::tuple<wchar_t *, qsizetype> {
        if (outlen >= size)
            return {out, outlen};
        const bool wasStackBuffer = sp.isEmpty();
        const auto begin = wasStackBuffer ? buf.data() : reinterpret_cast<wchar_t *>(sp.data());
        const qsizetype offset = qsizetype(std::distance(begin, out));
        qsizetype newSize = 0;
        if (Q_UNLIKELY(qAddOverflow(offset, size, &newSize))) {
            Q_CHECK_PTR(false);
            return {nullptr, 0};
        }
        sp.resize(newSize);
        auto it = reinterpret_cast<wchar_t *>(sp.data());
        if (wasStackBuffer)
            it = std::copy_n(buf.data(), offset, it);
        else
            it += offset;
        return {it, size};
    };

    // Convert the pending characters (if available)
    while (state && state->remainingChars && mblen) {
        QStringConverter::State localState;
        localState.flags = state->flags;
        // Use at most 6 characters as a guess for the longest encoded character
        // in any multibyte encoding.
        // Even with a total of 2 bytes of overhead that would leave around
        // 2^(4 * 8) possible characters
        std::array<char, 6> prev = {0};
        Q_ASSERT(state->remainingChars <= q20::ssize(state->state_data));
        qsizetype index = 0;
        for (; index < state->remainingChars; ++index)
            prev[index] = state->state_data[index];
        const qsizetype toCopy = std::min(q20::ssize(prev) - index, mblen);
        for (qsizetype i = 0; i < toCopy; ++i, ++index)
            prev[index] = mb[i];
        mb += toCopy;
        mblen -= toCopy;

        // Recursing:
        // Since we are using a clean local state it will try to decode what was
        // stored in our state + some extra octets from input (`prev`). If some
        // part fails we will have those characters stored in the local state's
        // storage, and we can extract those. It may also output some
        // replacement characters, which we'll count in the invalidChars.
        // In the best case we only do this once, but we will loop until we have
        // resolved all the remaining characters or we have run out of new input
        // in which case we may still have remaining characters.
        const QString tmp = convertToUnicode_sys(QByteArrayView(prev.data(), index), codePage,
                                                    &localState);
        std::tie(out, outlen) = growOut(tmp.size());
        if (!out)
            return {};
        out = std::copy_n(reinterpret_cast<const wchar_t *>(tmp.constData()), tmp.size(), out);
        outlen -= tmp.size();
        const qsizetype tail = toCopy - localState.remainingChars;
        if (tail >= 0) {
            // Everything left to process comes from `in`, so we can stop
            // looping. Adjust the window for `in` and unset remainingChars to
            // signal that we're done.
            mb -= localState.remainingChars;
            mblen += localState.remainingChars;
            localState.remainingChars = 0;
        }
        state->remainingChars = localState.remainingChars;
        state->invalidChars += localState.invalidChars;
        std::copy_n(localState.state_data, state->remainingChars, state->state_data);
    }

    Q_ASSERT(!state || state->remainingChars == 0 || mblen == 0);

    // Need it in this scope, since we try to decrease our window size if we
    // encounter an error
    int nextIn = q26::saturate_cast<int>(mblen);
    while (mblen > 0) {
        std::tie(out, outlen) = growOut(1); // Need space for at least one character
        if (!out)
            return {};
        const int nextOut = q26::saturate_cast<int>(outlen);
        int len = MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, mb, nextIn, out, nextOut);
        if (len) {
            mb += nextIn;
            mblen -= nextIn;
            out += len;
            outlen -= len;
        } else {
            int r = GetLastError();
            if (r == ERROR_INSUFFICIENT_BUFFER) {
                const int wclen = MultiByteToWideChar(codePage, 0, mb, nextIn, 0, 0);
                std::tie(out, outlen) = growOut(wclen);
                if (!out)
                    return {};
            } else if (r == ERROR_NO_UNICODE_TRANSLATION) {
                // Can't decode the current window, so either store the state,
                // reduce window size or output a replacement character.

                // Check if we can store all remaining characters in the state
                // to be used next time we're called:
                if (state && mblen <= q20::ssize(state->state_data)) {
                    state->remainingChars = mblen;
                    std::copy_n(mb, mblen, state->state_data);
                    mb += mblen;
                    mblen = 0;
                    break;
                }

                // .. if not, try to find the last valid character in the window
                // and try again with a shrunken window:
                if (nextIn > 1) {
                    // There may be some incomplete data at the end of our current
                    // window, so decrease the window size and try again.
                    // In the worst case scenario there is gigs of undecodable
                    // garbage, but what are we supposed to do about that?
                    const auto it = CharPrevExA(codePage, mb, mb + nextIn, 0);
                    if (it != mb)
                        nextIn = int(it - mb);
                    else
                        --nextIn;
                    continue;
                }

                // Finally, we are forced to output a replacement character for
                // the first byte in the window:
                std::tie(out, outlen) = growOut(1);
                if (!out)
                    return {};
                *out = replacementCharacter;
                ++invalidChars;
                ++out;
                --outlen;
                ++mb;
                --mblen;
            } else {
                // Fail.
                qWarning("MultiByteToWideChar: Cannot convert multibyte text");
                break;
            }
        }
        nextIn = q26::saturate_cast<int>(mblen);
    }

    if (sp.isEmpty()) {
        // We must have only used the stack buffer
        if (out != buf.data()) // else: we return null-string
            sp = QStringView(buf.data(), out).toString();
    } else{
        const auto begin = reinterpret_cast<wchar_t *>(sp.data());
        sp.truncate(std::distance(begin, out));
    }

    if (sp.size() && sp.back().isNull())
        sp.chop(1);

    if (!state && mblen > 0) {
        // We have trailing character(s) that could not be converted, and
        // nowhere to cache them
        sp.resize(sp.size() + mblen, replacementCharacter);
        invalidChars += mblen;
    }
    return sp;
}

QByteArray QLocal8Bit::convertFromUnicode_sys(QStringView in, QStringConverter::State *state)
{
    return convertFromUnicode_sys(in, CP_ACP, state);
}

QByteArray QLocal8Bit::convertFromUnicode_sys(QStringView in, quint32 codePage,
                                              QStringConverter::State *state)
{
    const wchar_t *ch = reinterpret_cast<const wchar_t *>(in.data());
    qsizetype uclen = in.size();

    Q_ASSERT(state);
    // The Windows API has a *boolean* out-parameter that says if a replacement
    // character was used, but it gives us no way to know _how many_ were used.
    // Since we cannot simply scan the string for replacement characters
    // (which is potentially a question mark, and thus a valid character),
    // we simply do not track the number of invalid characters here.
    // auto &invalidChars = state->invalidChars;

    using Flag = QStringConverter::Flag;
    if (state->flags & Flag::Stateless) { // temporary
        Q_ASSERT(state->remainingChars == 0);
        state = nullptr;
    }

    if (!ch)
        return QByteArray();
    if (uclen == 0)
        return QByteArray("");

    // Use a local stack-buffer at first to allow us a decently large container
    // to avoid a lot of resizing, without also returning an overallocated
    // QByteArray to the user for small strings.
    // Then we can be fast for small strings and take the hit of extra resizes
    // and measuring how much storage is needed for large strings.
    std::array<char, 4096> buf;
    char *out = buf.data();
    qsizetype outlen = buf.size();
    QByteArray mb;

    if (state && state->remainingChars > 0) {
        Q_ASSERT(state->remainingChars == 1);
        // Let's try to decode the pending character
        wchar_t wc[2] = { wchar_t(state->state_data[0]), ch[0] };
        // Check if the second character is a valid low surrogate,
        // otherwise we'll just decode the first character, for which windows
        // will output a replacement character.
        const bool validCodePoint = QChar::isLowSurrogate(wc[1]);
        int len = WideCharToMultiByte(codePage, 0, wc, validCodePoint ? 2 : 1, out, outlen, nullptr,
                                      nullptr);
        if (!len)
            return {}; // Cannot recover, and I refuse to believe it was a size limitation
        out += len;
        outlen -= len;
        if (validCodePoint) {
            ++ch;
            --uclen;
        }
        state->remainingChars = 0;
        state->state_data[0] = 0;
        if (uclen == 0)
            return QByteArrayView(buf.data(), len).toByteArray();
    }

    if (state && QChar::isHighSurrogate(ch[uclen - 1])) {
        // We can handle a missing low surrogate at the end of the string,
        // so if there is one, exclude it now and store it in the state.
        state->remainingChars = 1;
        state->state_data[0] = ch[uclen - 1];
        --uclen;
        if (uclen == 0)
            return QByteArray();
    }

    Q_ASSERT(uclen > 0);

    // Return a pointer to storage where we have enough space for `size`
    const auto growOut = [&](qsizetype size) -> std::tuple<char *, qsizetype> {
        if (outlen >= size)
            return {out, outlen};
        const bool wasStackBuffer = mb.isEmpty();
        const auto begin = wasStackBuffer ? buf.data() : mb.data();
        const qsizetype offset = qsizetype(std::distance(begin, out));
        qsizetype newSize = 0;
        if (Q_UNLIKELY(qAddOverflow(offset, size, &newSize))) {
            Q_CHECK_PTR(false);
            return {nullptr, 0};
        }
        mb.resize(newSize);
        auto it = mb.data();
        if (wasStackBuffer)
            it = std::copy_n(buf.data(), offset, it);
        else
            it += offset;
        return {it, size};
    };

    const auto getNextWindowSize = [&]() {
        int nextIn = q26::saturate_cast<int>(uclen);
        // The Windows API has some issues if the current window ends in the
        // middle of a surrogate pair, so we avoid that:
        if (nextIn > 1 && QChar::isHighSurrogate(ch[nextIn - 1]))
            --nextIn;
        return nextIn;
    };

    int len = 0;
    while (uclen > 0) {
        const int nextIn = getNextWindowSize();
        std::tie(out, outlen) = growOut(1); // We need at least one byte
        if (!out)
            return {};
        const int nextOut = q26::saturate_cast<int>(outlen);
        len = WideCharToMultiByte(codePage, 0, ch, nextIn, out, nextOut, nullptr, nullptr);
        if (len > 0) {
            ch += nextIn;
            uclen -= nextIn;
            out += len;
            outlen -= len;
        } else {
            int r = GetLastError();
            if (r == ERROR_INSUFFICIENT_BUFFER) {
                int neededLength = WideCharToMultiByte(codePage, 0, ch, nextIn, nullptr, 0,
                                                       nullptr, nullptr);
                if (neededLength <= 0) {
                    // Fail. Observed with UTF8 where the input window was max int and ended in an
                    // incomplete sequence, probably a Windows bug. We try to avoid that from
                    // happening by reducing the window size in that case. But let's keep this
                    // branch just in case of other bugs.
#ifndef QT_NO_DEBUG
                    r = GetLastError();
                    fprintf(stderr,
                            "WideCharToMultiByte: Cannot convert multibyte text (error %d)\n", r);
#endif // !QT_NO_DEBUG
                    break;
                }
                std::tie(out, outlen) = growOut(neededLength);
                if (!out)
                    return {};
                // and try again...
            } else {
                // Fail.  Probably can't happen in fact (dwFlags is 0).
#ifndef QT_NO_DEBUG
                // Can't use qWarning(), as it'll recurse to handle %ls
                fprintf(stderr,
                        "WideCharToMultiByte: Cannot convert multibyte text (error %d): %ls\n",
                        r, qt_castToWchar(QStringView(ch, uclen).left(100).toString()));
#endif
                break;
            }
        }
    }
    if (mb.isEmpty()) {
        // We must have only used the stack buffer
        if (out != buf.data()) // else: we return null-array
            mb = QByteArrayView(buf.data(), out).toByteArray();
    } else {
        mb.truncate(std::distance(mb.data(), out));
    }
    return mb;
}
#endif

void QStringConverter::State::clear() noexcept
{
    if (clearFn)
        clearFn(this);
    else
        state_data[0] = state_data[1] = state_data[2] = state_data[3] = 0;
    remainingChars = 0;
    invalidChars = 0;
    internalState = 0;
}

void QStringConverter::State::reset() noexcept
{
    if (flags & Flag::UsesIcu) {
#if defined(QT_USE_ICU_CODECS)
        QT_COM_THREAD_INIT
        UConverter *converter = static_cast<UConverter *>(d[0]);
        if (converter)
            ucnv_reset(converter);
#else
        Q_UNREACHABLE();
#endif
    } else {
        clear();
    }
}

#ifndef QT_BOOTSTRAPPED
static QChar *fromUtf16(QChar *out, QByteArrayView in, QStringConverter::State *state)
{
    return QUtf16::convertToUnicode(out, in, state, DetectEndianness);
}

static char *toUtf16(char *out, QStringView in, QStringConverter::State *state)
{
    return QUtf16::convertFromUnicode(out, in, state, DetectEndianness);
}

static QChar *fromUtf16BE(QChar *out, QByteArrayView in, QStringConverter::State *state)
{
    return QUtf16::convertToUnicode(out, in, state, BigEndianness);
}

static char *toUtf16BE(char *out, QStringView in, QStringConverter::State *state)
{
    return QUtf16::convertFromUnicode(out, in, state, BigEndianness);
}

static QChar *fromUtf16LE(QChar *out, QByteArrayView in, QStringConverter::State *state)
{
    return QUtf16::convertToUnicode(out, in, state, LittleEndianness);
}

static char *toUtf16LE(char *out, QStringView in, QStringConverter::State *state)
{
    return QUtf16::convertFromUnicode(out, in, state, LittleEndianness);
}

static QChar *fromUtf32(QChar *out, QByteArrayView in, QStringConverter::State *state)
{
    return QUtf32::convertToUnicode(out, in, state, DetectEndianness);
}

static char *toUtf32(char *out, QStringView in, QStringConverter::State *state)
{
    return QUtf32::convertFromUnicode(out, in, state, DetectEndianness);
}

static QChar *fromUtf32BE(QChar *out, QByteArrayView in, QStringConverter::State *state)
{
    return QUtf32::convertToUnicode(out, in, state, BigEndianness);
}

static char *toUtf32BE(char *out, QStringView in, QStringConverter::State *state)
{
    return QUtf32::convertFromUnicode(out, in, state, BigEndianness);
}

static QChar *fromUtf32LE(QChar *out, QByteArrayView in, QStringConverter::State *state)
{
    return QUtf32::convertToUnicode(out, in, state, LittleEndianness);
}

static char *toUtf32LE(char *out, QStringView in, QStringConverter::State *state)
{
    return QUtf32::convertFromUnicode(out, in, state, LittleEndianness);
}
#endif // !QT_BOOTSTRAPPED

char *QLatin1::convertFromUnicode(char *out, QStringView in, QStringConverter::State *state) noexcept
{
    Q_ASSERT(state);
    if (state->flags & QStringConverter::Flag::Stateless) // temporary
        state = nullptr;

    const char replacement = (state && state->flags & QStringConverter::Flag::ConvertInvalidToNull) ? 0 : '?';
    qsizetype invalid = 0;
    for (qsizetype i = 0; i < in.size(); ++i) {
        if (in[i] > QChar(0xff)) {
            *out = replacement;
            ++invalid;
        } else {
            *out = (char)in[i].cell();
        }
        ++out;
    }
    if (state)
        state->invalidChars += invalid;
    return out;
}

static QChar *fromLocal8Bit(QChar *out, QByteArrayView in, QStringConverter::State *state)
{
    QString s = QLocal8Bit::convertToUnicode(in, state);
    memcpy(out, s.constData(), s.size()*sizeof(QChar));
    return out + s.size();
}

static char *toLocal8Bit(char *out, QStringView in, QStringConverter::State *state)
{
    QByteArray s = QLocal8Bit::convertFromUnicode(in, state);
    memcpy(out, s.constData(), s.size());
    return out + s.size();
}


static qsizetype fromUtf8Len(qsizetype l) { return l + 1; }
static qsizetype toUtf8Len(qsizetype l) { return 3*(l + 1); }

#ifndef QT_BOOTSTRAPPED
static qsizetype fromUtf16Len(qsizetype l) { return l/2 + 2; }
static qsizetype toUtf16Len(qsizetype l) { return 2*(l + 1); }

static qsizetype fromUtf32Len(qsizetype l) { return l/2 + 2; }
static qsizetype toUtf32Len(qsizetype l) { return 4*(l + 1); }
#endif

static qsizetype fromLatin1Len(qsizetype l) { return l + 1; }
static qsizetype toLatin1Len(qsizetype l) { return l + 1; }



/*!
    \class QStringConverter
    \inmodule QtCore
    \brief The QStringConverter class provides a base class for encoding and decoding text.
    \reentrant
    \ingroup i18n
    \ingroup string-processing

    Qt uses UTF-16 to store, draw and manipulate strings. In many
    situations you may wish to deal with data that uses a different
    encoding. Most text data transferred over files and network connections is encoded
    in UTF-8.

    The QStringConverter class is a base class for the \l {QStringEncoder} and
    \l {QStringDecoder} classes that help with converting between different
    text encodings. QStringDecoder can decode a string from an encoded representation
    into UTF-16, the format Qt uses internally. QStringEncoder does the opposite
    operation, encoding UTF-16 encoded data (usually in the form of a QString) to
    the requested encoding.

    The following encodings are always supported:

    \list
    \li UTF-8
    \li UTF-16
    \li UTF-16BE
    \li UTF-16LE
    \li UTF-32
    \li UTF-32BE
    \li UTF-32LE
    \li ISO-8859-1 (Latin-1)
    \li The system encoding
    \endlist

    QStringConverter may support more encodings depending on how Qt was
    compiled. If more codecs are supported, they can be listed using
    availableCodecs().

    \l {QStringConverter}s can be used as follows to convert some encoded
    string to and from UTF-16.

    Suppose you have some string encoded in UTF-8, and
    want to convert it to a QString. The simple way
    to do it is to use a \l {QStringDecoder} like this:

    \snippet code/src_corelib_text_qstringconverter.cpp 0

    After this, \c string holds the text in decoded form.
    Converting a string from Unicode to the local encoding is just as
    easy using the \l {QStringEncoder} class:

    \snippet code/src_corelib_text_qstringconverter.cpp 1

    To read or write text files in various encodings, use QTextStream and
    its \l{QTextStream::setEncoding()}{setEncoding()} function.

    Some care must be taken when trying to convert the data in chunks,
    for example, when receiving it over a network. In such cases it is
    possible that a multi-byte character will be split over two
    chunks. At best this might result in the loss of a character and
    at worst cause the entire conversion to fail.

    Both QStringEncoder and QStringDecoder make this easy, by tracking
    this in an internal state. So simply calling the encoder or decoder
    again with the next chunk of data will automatically continue encoding
    or decoding the data correctly:

    \snippet code/src_corelib_text_qstringconverter.cpp 2

    The QStringDecoder object maintains state between chunks and therefore
    works correctly even if a multi-byte character is split between
    chunks.

    QStringConverter objects can't be copied because of their internal state, but
    can be moved.

    \sa QTextStream, QStringDecoder, QStringEncoder
*/

/*!
    \enum QStringConverter::Flag

    \value Default Default conversion rules apply.
    \value ConvertInvalidToNull  If this flag is set, each invalid input
                                 character is output as a null character. If it is not set,
                                 invalid input characters are represented as QChar::ReplacementCharacter
                                 if the output encoding can represent that character, otherwise as a question mark.
    \value WriteBom When converting from a QString to an output encoding, write a QChar::ByteOrderMark as the first
                    character if the output encoding supports this. This is the case for UTF-8, UTF-16 and UTF-32
                    encodings.
    \value ConvertInitialBom When converting from an input encoding to a QString the QStringDecoder usually skips an
                              leading QChar::ByteOrderMark. When this flag is set, the byte order mark will not be
                              skipped, but converted to utf-16 and inserted at the start of the created QString.
    \value Stateless Ignore possible converter states between different function calls
           to encode or decode strings. This will also cause the QStringConverter to raise an error if an incomplete
           sequence of data is encountered.
    \omitvalue UsesIcu
*/

/*!
    \enum QStringConverter::Encoding
    \value Utf8 Create a converter to or from UTF-8
    \value Utf16 Create a converter to or from UTF-16. When decoding, the byte order will get automatically
           detected by a leading byte order mark. If none exists or when encoding, the system byte order will
           be assumed.
    \value Utf16BE Create a converter to or from big-endian UTF-16.
    \value Utf16LE Create a converter to or from little-endian UTF-16.
    \value Utf32 Create a converter to or from UTF-32. When decoding, the byte order will get automatically
           detected by a leading byte order mark. If none exists or when encoding, the system byte order will
           be assumed.
    \value Utf32BE Create a converter to or from big-endian UTF-32.
    \value Utf32LE Create a converter to or from little-endian UTF-32.
    \value Latin1 Create a converter to or from ISO-8859-1 (Latin1).
    \value System Create a converter to or from the underlying encoding of the
           operating systems locale. This is always assumed to be UTF-8 for Unix based
           systems. On Windows, this converts to and from the locale code page.
    \omitvalue LastEncoding
*/

/*!
    \struct QStringConverter::Interface
    \internal
*/

const QStringConverter::Interface QStringConverter::encodingInterfaces[QStringConverter::LastEncoding + 1] =
{
    { "UTF-8", QUtf8::convertToUnicode, fromUtf8Len, QUtf8::convertFromUnicode, toUtf8Len },
#ifndef QT_BOOTSTRAPPED
    { "UTF-16", fromUtf16, fromUtf16Len, toUtf16, toUtf16Len },
    { "UTF-16LE", fromUtf16LE, fromUtf16Len, toUtf16LE, toUtf16Len },
    { "UTF-16BE", fromUtf16BE, fromUtf16Len, toUtf16BE, toUtf16Len },
    { "UTF-32", fromUtf32, fromUtf32Len, toUtf32, toUtf32Len },
    { "UTF-32LE", fromUtf32LE, fromUtf32Len, toUtf32LE, toUtf32Len },
    { "UTF-32BE", fromUtf32BE, fromUtf32Len, toUtf32BE, toUtf32Len },
#endif
    { "ISO-8859-1", QLatin1::convertToUnicode, fromLatin1Len, QLatin1::convertFromUnicode, toLatin1Len },
    { "Locale", fromLocal8Bit, fromUtf8Len, toLocal8Bit, toUtf8Len }
};

// match names case insensitive and skipping '-' and '_'
template <typename Char>
static bool nameMatch_impl_impl(const char *a, const Char *b, const Char *b_end)
{
    do {
        while (*a == '-' || *a == '_')
            ++a;
        while (b != b_end && (*b == Char{'-'} || *b == Char{'_'}))
            ++b;
        if (!*a && b == b_end) // end of both strings
            return true;
        if (char16_t(*b) > 127)
            return false; // non-US-ASCII cannot match US-ASCII (prevents narrowing below)
    } while (QtMiscUtils::toAsciiLower(*a++) == QtMiscUtils::toAsciiLower(char(*b++)));

    return false;
}

static bool nameMatch_impl(const char *a, QLatin1StringView b)
{
    return nameMatch_impl_impl(a, b.begin(), b.end());
}

static bool nameMatch_impl(const char *a, QUtf8StringView b)
{
    return nameMatch_impl(a, QLatin1StringView{QByteArrayView{b}});
}

static bool nameMatch_impl(const char *a, QStringView b)
{
    return nameMatch_impl_impl(a, b.utf16(), b.utf16() + b.size()); // uses char16_t*, not QChar*
}

static bool nameMatch(const char *a, QAnyStringView b)
{
    return b.visit([a](auto b) { return nameMatch_impl(a, b); });
}


/*!
    \fn constexpr QStringConverter::QStringConverter()
    \internal
*/

/*!
    \fn constexpr QStringConverter::QStringConverter(Encoding, Flags)
    \internal
*/


#if defined(QT_USE_ICU_CODECS)
// only derives from QStringConverter to get access to protected types
struct QStringConverterICU : QStringConverter
{
    static void clear_function(QStringConverter::State *state) noexcept
    {
        QT_COM_THREAD_INIT
        ucnv_close(static_cast<UConverter *>(state->d[0]));
        state->d[0] = nullptr;
    }

    static void ensureConverter(QStringConverter::State *state)
    {
        // old code might reset the state via clear instead of reset
        // in that case, the converter has been closed, and we have to reopen it
        if (state->d[0] == nullptr)
            state->d[0] = createConverterForName(static_cast<const char *>(state->d[1]), state);
    }

    static QChar *toUtf16(QChar *out, QByteArrayView in, QStringConverter::State *state)
    {
        QT_COM_THREAD_INIT
        ensureConverter(state);

        auto icu_conv = static_cast<UConverter *>(state->d[0]);
        UErrorCode err = U_ZERO_ERROR;
        auto source = in.data();
        auto sourceLimit = in.data() + in.size();

        qsizetype length = toLen(in.size());

        UChar *target = reinterpret_cast<UChar *>(out);
        auto targetLimit = target + length;
        // We explicitly clean up anyway, so no need to set flush to true,
        // which would just reset the converter.
        UBool flush = false;

        // If the QStringConverter was moved, the state that we used as a context is stale now.
        UConverterToUCallback action;
        const void *context;
        ucnv_getToUCallBack(icu_conv, &action, &context);
        if (context != state)
             ucnv_setToUCallBack(icu_conv, action, state, nullptr, nullptr, &err);

        ucnv_toUnicode(icu_conv, &target, targetLimit, &source, sourceLimit, nullptr, flush, &err);
        // We did reserve enough space:
        Q_ASSERT(err != U_BUFFER_OVERFLOW_ERROR);
        if (state->flags.testFlag(QStringConverter::Flag::Stateless)) {
            if (auto leftOver = ucnv_toUCountPending(icu_conv, &err)) {
                ucnv_reset(icu_conv);
                state->invalidChars += leftOver;
            }
        }
        return reinterpret_cast<QChar *>(target);
    }

    static char *fromUtf16(char *out, QStringView in, QStringConverter::State *state)
    {
        QT_COM_THREAD_INIT
        ensureConverter(state);
        auto icu_conv = static_cast<UConverter *>(state->d[0]);
        UErrorCode err = U_ZERO_ERROR;
        auto source = reinterpret_cast<const UChar *>(in.data());
        auto sourceLimit = reinterpret_cast<const UChar *>(in.data() + in.size());

        qsizetype length = UCNV_GET_MAX_BYTES_FOR_STRING(in.size(), ucnv_getMaxCharSize(icu_conv));

        char *target = out;
        char *targetLimit = out + length;
        UBool flush = false;

        // If the QStringConverter was moved, the state that we used as a context is stale now.
        UConverterFromUCallback action;
        const void *context;
        ucnv_getFromUCallBack(icu_conv, &action, &context);
        if (context != state)
             ucnv_setFromUCallBack(icu_conv, action, state, nullptr, nullptr, &err);

        ucnv_fromUnicode(icu_conv, &target, targetLimit, &source, sourceLimit, nullptr, flush, &err);
        // We did reserve enough space:
        Q_ASSERT(err != U_BUFFER_OVERFLOW_ERROR);
        if (state->flags.testFlag(QStringConverter::Flag::Stateless)) {
            if (auto leftOver = ucnv_fromUCountPending(icu_conv, &err)) {
                ucnv_reset(icu_conv);
                state->invalidChars += leftOver;
            }
        }
        return target;
    }

    Q_DISABLE_COPY_MOVE(QStringConverterICU)

    template<qsizetype X>
    static qsizetype fromLen(qsizetype inLength)
    {
        return X * inLength * sizeof(UChar);
    }

    static qsizetype toLen(qsizetype inLength)
    {

        /* Assumption: each input char might map to a different codepoint
           Each codepoint can take up to 4 bytes == 2 QChar
           We can ignore reserving space for a BOM, as only UTF encodings use one
           and those are not handled by the ICU converter.
         */
        return 2 * inLength;
    }

    static constexpr QStringConverter::Interface forLength[] = {
        {"icu, recompile if you see this", QStringConverterICU::toUtf16, QStringConverterICU::toLen, QStringConverterICU::fromUtf16, QStringConverterICU::fromLen<1>},
        {"icu, recompile if you see this", QStringConverterICU::toUtf16, QStringConverterICU::toLen, QStringConverterICU::fromUtf16, QStringConverterICU::fromLen<2>},
        {"icu, recompile if you see this", QStringConverterICU::toUtf16, QStringConverterICU::toLen, QStringConverterICU::fromUtf16, QStringConverterICU::fromLen<3>},
        {"icu, recompile if you see this", QStringConverterICU::toUtf16, QStringConverterICU::toLen, QStringConverterICU::fromUtf16, QStringConverterICU::fromLen<4>},
        {"icu, recompile if you see this", QStringConverterICU::toUtf16, QStringConverterICU::toLen, QStringConverterICU::fromUtf16, QStringConverterICU::fromLen<5>},
        {"icu, recompile if you see this", QStringConverterICU::toUtf16, QStringConverterICU::toLen, QStringConverterICU::fromUtf16, QStringConverterICU::fromLen<6>},
        {"icu, recompile if you see this", QStringConverterICU::toUtf16, QStringConverterICU::toLen, QStringConverterICU::fromUtf16, QStringConverterICU::fromLen<7>},
        {"icu, recompile if you see this", QStringConverterICU::toUtf16, QStringConverterICU::toLen, QStringConverterICU::fromUtf16, QStringConverterICU::fromLen<8>}
    };

    static UConverter *createConverterForName(const char *name, const State *state)
    {
        Q_ASSERT(name);
        Q_ASSERT(state);
        QT_COM_THREAD_INIT
        UErrorCode status = U_ZERO_ERROR;
        UConverter *conv = ucnv_open(name, &status);
        if (status != U_ZERO_ERROR && status != U_AMBIGUOUS_ALIAS_WARNING) {
            ucnv_close(conv);
            return nullptr;
        }

        if (state->flags.testFlag(Flag::ConvertInvalidToNull)) {
            UErrorCode error = U_ZERO_ERROR;

            auto nullToSubstituter = [](const void *context, UConverterToUnicodeArgs *toUArgs,
                                        const char *, int32_t length,
                                        UConverterCallbackReason reason, UErrorCode *err) {
                if (reason <= UCNV_IRREGULAR) {
                    *err = U_ZERO_ERROR;
                    UChar c = '\0';
                    ucnv_cbToUWriteUChars(toUArgs, &c, 1, 0, err);
                    // Recover outer scope's state (which isn't const) from context:
                    auto state = const_cast<State *>(static_cast<const State *>(context));
                    state->invalidChars += length;
                }
            };
            ucnv_setToUCallBack(conv, nullToSubstituter, state, nullptr, nullptr, &error);

            auto nullFromSubstituter = [](const void *context, UConverterFromUnicodeArgs *fromUArgs,
                                          const UChar *, int32_t length,
                                          UChar32, UConverterCallbackReason reason, UErrorCode *err) {
                if (reason <= UCNV_IRREGULAR) {
                    *err = U_ZERO_ERROR;
                    const UChar replacement[] = { 0 };
                    const UChar *stringBegin = std::begin(replacement);
                    ucnv_cbFromUWriteUChars(fromUArgs, &stringBegin, std::end(replacement), 0, err);
                    // Recover outer scope's state (which isn't const) from context:
                    auto state = const_cast<State *>(static_cast<const State *>(context));
                    state->invalidChars += length;
                }
            };
            ucnv_setFromUCallBack(conv, nullFromSubstituter, state, nullptr, nullptr, &error);
        } else {
            UErrorCode error = U_ZERO_ERROR;

            auto qmarkToSubstituter = [](const void *context, UConverterToUnicodeArgs *toUArgs,
                                         const char *codeUnits,int32_t length,
                                         UConverterCallbackReason reason, UErrorCode *err) {
                if (reason <= UCNV_IRREGULAR) {
                    // Recover outer scope's state (which isn't const) from context:
                    auto state = const_cast<State *>(static_cast<const State *>(context));
                    state->invalidChars += length;
                }
                // use existing ICU callback for logic
                UCNV_TO_U_CALLBACK_SUBSTITUTE(nullptr, toUArgs, codeUnits, length, reason, err);

            };
            ucnv_setToUCallBack(conv, qmarkToSubstituter, state, nullptr, nullptr, &error);

            auto qmarkFromSubstituter = [](const void *context, UConverterFromUnicodeArgs *fromUArgs,
                                           const UChar *codeUnits, int32_t length,
                                           UChar32 codePoint, UConverterCallbackReason reason, UErrorCode *err) {
                if (reason <= UCNV_IRREGULAR) {
                    // Recover outer scope's state (which isn't const) from context:
                    auto state = const_cast<State *>(static_cast<const State *>(context));
                    state->invalidChars += length;
                }
                // use existing ICU callback for logic
                UCNV_FROM_U_CALLBACK_SUBSTITUTE(nullptr, fromUArgs, codeUnits, length,
                                                codePoint, reason, err);
            };
            ucnv_setFromUCallBack(conv, qmarkFromSubstituter, state, nullptr, nullptr, &error);
        }
        return conv;
    }

    static std::string nul_terminate_impl(QLatin1StringView name)
    { return name.isNull() ? std::string() : std::string{name.data(), size_t(name.size())}; }

    static std::string nul_terminate_impl(QUtf8StringView name)
    { return nul_terminate_impl(QLatin1StringView{QByteArrayView{name}}); }

    static std::string nul_terminate_impl(QStringView name)
    {
        std::string result;
        const auto convert = [&](char *p, size_t n) {
                const auto sz = QLatin1::convertFromUnicode(p, name) - p;
                Q_ASSERT(q20::cmp_less_equal(sz, n));
                return sz;
            };
#ifdef __cpp_lib_string_resize_and_overwrite
        result.resize_and_overwrite(size_t(name.size()), convert);
#else
        result.resize(size_t(name.size()));
        result.resize(convert(result.data(), result.size()));
#endif // __cpp_lib_string_resize_and_overwrite
        return result;
    }

    static std::string nul_terminate(QAnyStringView name)
    { return name.visit([](auto name) { return nul_terminate_impl(name); }); }

    static const QStringConverter::Interface *
    make_icu_converter(QStringConverter::State *state, QAnyStringView name)
    { return make_icu_converter(state, nul_terminate(name).data()); }

    static const QStringConverter::Interface *make_icu_converter(
            QStringConverter::State *state,
            const char *name)
    {
        QT_COM_THREAD_INIT
        UErrorCode status = U_ZERO_ERROR;
        UConverter *conv = createConverterForName(name, state);
        if (!conv)
            return nullptr;

        const char *icuName = ucnv_getName(conv, &status);
        // ucnv_getStandardName returns a name which is owned by the library
        // we can thus store it in the state without worrying aobut its lifetime
        const char *persistentName = ucnv_getStandardName(icuName, "MIME", &status);
        if (U_FAILURE(status) || !persistentName) {
             status = U_ZERO_ERROR;
             persistentName = ucnv_getStandardName(icuName, "IANA", &status);
        }
        state->d[1] = const_cast<char *>(persistentName);
        state->d[0] = conv;
        state->flags |= QStringConverter::Flag::UsesIcu;
        qsizetype maxCharSize = ucnv_getMaxCharSize(conv);
        state->clearFn = QStringConverterICU::clear_function;
        if (maxCharSize > 8 || maxCharSize < 1) {
            qWarning("Encountered unexpected codec \"%s\" which requires >8x space", name);
            return nullptr;
        } else {
            return &forLength[maxCharSize - 1];
        }

    }

};
#endif

/*!
    \internal
*/
QStringConverter::QStringConverter(QAnyStringView name, Flags f)
    : iface(nullptr), state(f)
{
    auto e = encodingForName(name);
    if (e)
        iface = encodingInterfaces + int(*e);
#if defined(QT_USE_ICU_CODECS)
    else
        iface = QStringConverterICU::make_icu_converter(&state, name);
#endif
}


const char *QStringConverter::name() const noexcept
{
    if (!iface)
        return nullptr;
    if (state.flags & QStringConverter::Flag::UsesIcu) {
#if defined(QT_USE_ICU_CODECS)
        return static_cast<const char*>(state.d[1]);
#else
        return nullptr;
#endif
    } else {
        return iface->name;
    }
}

/*!
    \fn bool QStringConverter::isValid() const

    Returns true if this is a valid string converter that can be used for encoding or
    decoding text.

    Default constructed string converters or converters constructed with an unsupported
    name are not valid.
*/

/*!
    \fn void QStringConverter::resetState()

    Resets the internal state of the converter, clearing potential errors or partial
    conversions.
*/

/*!
    \fn bool QStringConverter::hasError() const

    Returns true if a conversion could not correctly convert a character. This could for example
    get triggered by an invalid UTF-8 sequence or when a character can't get converted due to
    limitations in the target encoding.
*/

/*!
    \fn const char *QStringConverter::name() const

    Returns the canonical name of the encoding this QStringConverter can encode or decode.
    Returns a nullptr if the converter is not valid.
    The returned name is UTF-8 encoded.

    \sa isValid()
*/

/*!
    Convert \a name to the corresponding \l Encoding member, if there is one.

    If the \a name is not the name of a codec listed in the Encoding enumeration,
    \c{std::nullopt} is returned. Such a name may, none the less, be accepted by
    the QStringConverter constructor when Qt is built with ICU, if ICU provides a
    converter with the given name.

    \note In Qt versions prior to 6.8, this function took only a \c{const char *},
    which was expected to be UTF-8-encoded.
*/
std::optional<QStringConverter::Encoding> QStringConverter::encodingForName(QAnyStringView name) noexcept
{
    if (name.isEmpty())
        return std::nullopt;
    for (qsizetype i = 0; i < LastEncoding + 1; ++i) {
        if (nameMatch(encodingInterfaces[i].name, name))
            return QStringConverter::Encoding(i);
    }
    if (nameMatch("latin1", name))
        return QStringConverter::Latin1;
    return std::nullopt;
}

#ifndef QT_BOOTSTRAPPED
/*!
   Returns the encoding for the content of \a data if it can be determined.
   \a expectedFirstCharacter can be passed as an additional hint to help determine
   the encoding.

   The returned optional is empty, if the encoding is unclear.
 */
std::optional<QStringConverter::Encoding>
QStringConverter::encodingForData(QByteArrayView data, char16_t expectedFirstCharacter) noexcept
{
    // someone set us up the BOM?
    qsizetype arraySize = data.size();
    if (arraySize > 3) {
        char32_t uc = qFromUnaligned<char32_t>(data.data());
        if (uc == qToBigEndian(char32_t(QChar::ByteOrderMark)))
            return QStringConverter::Utf32BE;
        if (uc == qToLittleEndian(char32_t(QChar::ByteOrderMark)))
            return QStringConverter::Utf32LE;
        if (expectedFirstCharacter) {
            // catch also anything starting with the expected character
            if (qToLittleEndian(uc) == expectedFirstCharacter)
                return QStringConverter::Utf32LE;
            else if (qToBigEndian(uc) == expectedFirstCharacter)
                return QStringConverter::Utf32BE;
        }
    }

    if (arraySize > 2) {
        if (memcmp(data.data(), utf8bom, sizeof(utf8bom)) == 0)
            return QStringConverter::Utf8;
    }

    if (arraySize > 1) {
        char16_t uc = qFromUnaligned<char16_t>(data.data());
        if (uc == qToBigEndian(char16_t(QChar::ByteOrderMark)))
            return QStringConverter::Utf16BE;
        if (uc == qToLittleEndian(char16_t(QChar::ByteOrderMark)))
            return QStringConverter::Utf16LE;
        if (expectedFirstCharacter) {
            // catch also anything starting with the expected character
            if (qToLittleEndian(uc) == expectedFirstCharacter)
                return QStringConverter::Utf16LE;
            else if (qToBigEndian(uc) == expectedFirstCharacter)
                return QStringConverter::Utf16BE;
        }
    }
    return std::nullopt;
}

static QByteArray parseHtmlMetaForEncoding(QByteArrayView data)
{
    static constexpr auto metaSearcher = qMakeStaticByteArrayMatcher("meta ");
    static constexpr auto charsetSearcher = qMakeStaticByteArrayMatcher("charset=");

    QByteArray header = data.first(qMin(data.size(), qsizetype(1024))).toByteArray().toLower();
    qsizetype pos = metaSearcher.indexIn(header);
    if (pos != -1) {
        pos = charsetSearcher.indexIn(header, pos);
        if (pos != -1) {
            pos += qstrlen("charset=");
            if (pos < header.size() && (header.at(pos) == '\"' || header.at(pos) == '\''))
                ++pos;

            qsizetype pos2 = pos;
            // The attribute can be closed with either """, "'", ">" or "/",
            // none of which are valid charset characters.
            while (++pos2 < header.size()) {
                char ch = header.at(pos2);
                if (ch == '\"' || ch == '\'' || ch == '>' || ch == '/') {
                    QByteArray name = header.mid(pos, pos2 - pos);
                    qsizetype colon = name.indexOf(':');
                    if (colon > 0)
                        name = name.left(colon);
                    name = name.simplified();
                    if (name == "unicode") // QTBUG-41998, ICU will return UTF-16.
                        name = QByteArrayLiteral("UTF-8");
                    if (!name.isEmpty())
                        return name;
                }
            }
        }
    }
    return QByteArray();
}

/*!
    Tries to determine the encoding of the HTML in \a data by looking at leading byte
    order marks or a charset specifier in the HTML meta tag. If the optional is empty,
    the encoding specified is not supported by QStringConverter. If no encoding is
    detected, the method returns Utf8.

    \sa QStringDecoder::decoderForHtml()
*/
std::optional<QStringConverter::Encoding> QStringConverter::encodingForHtml(QByteArrayView data)
{
    // determine charset
    std::optional<QStringConverter::Encoding> encoding = encodingForData(data);
    if (encoding)
        // trust the initial BOM
        return encoding;

    QByteArray encodingTag = parseHtmlMetaForEncoding(data);
    if (!encodingTag.isEmpty())
        return encodingForName(encodingTag);

    return Utf8;
}

static qsizetype availableCodecCount()
{
#if !defined(QT_USE_ICU_CODECS)
    return QStringConverter::Encoding::LastEncoding;
#else
    QT_COM_THREAD_INIT
    /* icu contains also the names of what Qt provides
       except for the special Locale one (so add one for it)
    */
    return 1 + ucnv_countAvailable();
#endif
}

/*!
    Returns a list of names of supported codecs. The names returned
    by this function can be passed to QStringEncoder's and
    QStringDecoder's constructor to create a en- or decoder for
    the given codec.

    This function may be used to obtain a listing of additional codecs beyond
    the standard ones. Support for additional codecs requires Qt be compiled
    with support for the ICU library.

    \note The order of codecs is an internal implementation detail
    and not guaranteed to be stable.
 */
QStringList QStringConverter::availableCodecs()
{
    auto availableCodec = [](qsizetype index) -> QString
    {
    #if !defined(QT_USE_ICU_CODECS)
        return QString::fromLatin1(encodingInterfaces[index].name);
    #else
        if (index == 0) // "Locale", not provided by icu
            return QString::fromLatin1(
                        encodingInterfaces[QStringConverter::Encoding::System].name);
        QT_COM_THREAD_INIT
        // this mirrors the setup we do to set a converters name
        UErrorCode status = U_ZERO_ERROR;
        auto icuName = ucnv_getAvailableName(int32_t(index - 1));
        const char *standardName = ucnv_getStandardName(icuName, "MIME", &status);
        if (U_FAILURE(status) || !standardName) {
            status = U_ZERO_ERROR;
            standardName = ucnv_getStandardName(icuName, "IANA", &status);
        }
        if (!standardName)
            standardName = icuName;
        return QString::fromLatin1(standardName);
    #endif
    };

    qsizetype codecCount = availableCodecCount();
    QStringList result;
    result.reserve(codecCount);
    for (qsizetype i = 0; i < codecCount; ++i)
        result.push_back(availableCodec(i));
    return result;
}

/*!
    Tries to determine the encoding of the HTML in \a data by looking at leading byte
    order marks or a charset specifier in the HTML meta tag and returns a QStringDecoder
    matching the encoding. If the returned decoder is not valid,
    the encoding specified is not supported by QStringConverter. If no encoding is
    detected, the method returns a decoder for Utf8.

    \sa isValid()
*/
QStringDecoder QStringDecoder::decoderForHtml(QByteArrayView data)
{
    // determine charset
    std::optional<QStringConverter::Encoding> encoding = encodingForData(data);
    if (encoding)
        // trust the initial BOM
        return QStringDecoder(encoding.value());

    QByteArray encodingTag = parseHtmlMetaForEncoding(data);
    if (!encodingTag.isEmpty())
        return QStringDecoder(encodingTag);

    return QStringDecoder(Utf8);
}
#endif // !QT_BOOTSTRAPPED

/*!
    Returns the canonical name for encoding \a e or \nullptr if \a e is an
    invalid value.

    \note In Qt versions prior to 6.10, 6.9.1, 6.8.4 or 6.5.9, calling this
    function with an invalid argument resulted in undefined behavior. Since the
    above-mentioned Qt versions, it returns nullptr instead.
*/
const char *QStringConverter::nameForEncoding(QStringConverter::Encoding e) noexcept
{
    auto i = size_t(e);
    if (Q_UNLIKELY(i >= std::size(encodingInterfaces)))
        return nullptr;
    return encodingInterfaces[i].name;
}

/*!
    \class QStringEncoder
    \inmodule QtCore
    \brief The QStringEncoder class provides a state-based encoder for text.
    \reentrant
    \ingroup i18n
    \ingroup string-processing

    A text encoder converts text from Qt's internal representation into an encoded
    text format using a specific encoding.

    Converting a string from Unicode to the local encoding can be achieved
    using the following code:

    \snippet code/src_corelib_text_qstringconverter.cpp 1

    The encoder remembers any state that is required between calls, so converting
    data received in chunks, for example, when receiving it over a network, is just as
    easy, by calling the encoder whenever new data is available:

    \snippet code/src_corelib_text_qstringconverter.cpp 3

    The QStringEncoder object maintains state between chunks and therefore
    works correctly even if a UTF-16 surrogate character is split between
    chunks.

    QStringEncoder objects can't be copied because of their internal state, but
    can be moved.

    \sa QStringConverter, QStringDecoder
*/

/*!
    \fn constexpr QStringEncoder::QStringEncoder(const Interface *i)
    \internal
*/

/*!
    \fn constexpr QStringEncoder::QStringEncoder()

    Default constructs an encoder. The default encoder is not valid,
    and can't be used for converting text.
*/

/*!
    \fn constexpr QStringEncoder::QStringEncoder(Encoding encoding, Flags flags = Flag::Default)

    Creates an encoder object using \a encoding and \a flags.
*/

/*!
    \fn QStringEncoder::QStringEncoder(QAnyStringView name, Flags flags = Flag::Default)

    Creates an encoder object using \a name and \a flags.
    If \a name is not the name of a known encoding an invalid converter will get created.

    \note In Qt versions prior to 6.8, this function took only a \c{const char *},
    which was expected to be UTF-8-encoded.

    \sa isValid()
*/

/*!
    \fn QStringEncoder::DecodedData<const QString &> QStringEncoder::encode(const QString &in)
    \fn QStringEncoder::DecodedData<QStringView> QStringEncoder::encode(QStringView in)
    \fn QStringEncoder::DecodedData<const QString &> QStringEncoder::operator()(const QString &in)
    \fn QStringEncoder::DecodedData<QStringView> QStringEncoder::operator()(QStringView in)

    Converts \a in and returns a struct that is implicitly convertible to QByteArray.

    \snippet code/src_corelib_text_qstringconverter.cpp 5
*/

/*!
    \fn qsizetype QStringEncoder::requiredSpace(qsizetype inputLength) const

    Returns the maximum amount of characters required to be able to process
    \a inputLength decoded data.

    \sa appendToBuffer()
*/

/*!
    \fn char *QStringEncoder::appendToBuffer(char *out, QStringView in)

    Encodes \a in and writes the encoded result into the buffer
    starting at \a out. Returns a pointer to the end of the data written.

    \note \a out must be large enough to be able to hold all the decoded data. Use
    requiredSpace() to determine the maximum size requirement to be able to encode
    \a in.

    \sa requiredSpace()
*/

/*!
    \class QStringDecoder
    \inmodule QtCore
    \brief The QStringDecoder class provides a state-based decoder for text.
    \reentrant
    \ingroup i18n
    \ingroup string-processing

    A text decoder converts text an encoded text format that uses a specific encoding
    into Qt's internal representation.

    Converting encoded data into a QString can be achieved
    using the following code:

    \snippet code/src_corelib_text_qstringconverter.cpp 0

    The decoder remembers any state that is required between calls, so converting
    data received in chunks, for example, when receiving it over a network, is just as
    easy, by calling the decoder whenever new data is available:

    \snippet code/src_corelib_text_qstringconverter.cpp 2

    The QStringDecoder object maintains state between chunks and therefore
    works correctly even if chunks are split in the middle of a multi-byte character
    sequence.

    QStringDecoder objects can't be copied because of their internal state, but
    can be moved.

    \sa QStringConverter, QStringEncoder
*/

/*!
    \fn constexpr QStringDecoder::QStringDecoder(const Interface *i)
    \internal
*/

/*!
    \fn constexpr QStringDecoder::QStringDecoder()

    Default constructs an decoder. The default decoder is not valid,
    and can't be used for converting text.
*/

/*!
    \fn constexpr QStringDecoder::QStringDecoder(Encoding encoding, Flags flags = Flag::Default)

    Creates an decoder object using \a encoding and \a flags.
*/

/*!
    \fn QStringDecoder::QStringDecoder(QAnyStringView name, Flags flags = Flag::Default)

    Creates an decoder object using \a name and \a flags.
    If \a name is not the name of a known encoding an invalid converter will get created.

    \note In Qt versions prior to 6.8, this function took only a \c{const char *},
    which was expected to be UTF-8-encoded.

    \sa isValid()
*/

/*!
    \fn QStringDecoder::EncodedData<const QByteArray &> QStringDecoder::operator()(const QByteArray &ba)
    \fn QStringDecoder::EncodedData<const QByteArray &> QStringDecoder::decode(const QByteArray &ba)
    \fn QStringDecoder::EncodedData<QByteArrayView> QStringDecoder::operator()(QByteArrayView ba)
    \fn QStringDecoder::EncodedData<QByteArrayView> QStringDecoder::decode(QByteArrayView ba)

    Converts \a ba and returns a struct that is implicitly convertible to QString.


    \snippet code/src_corelib_text_qstringconverter.cpp 4
*/

/*!
    \fn qsizetype QStringDecoder::requiredSpace(qsizetype inputLength) const

    Returns the maximum amount of UTF-16 code units required to be able to process
    \a inputLength encoded data.

    \sa appendToBuffer
*/

/*!
    \fn QChar *QStringDecoder::appendToBuffer(QChar *out, QByteArrayView in)

    Decodes the sequence of bytes viewed by \a in and writes the decoded result into
    the buffer starting at \a out. Returns a pointer to the end of data written.

    \a out needs to be large enough to be able to hold all the decoded data. Use
    \l{requiredSpace} to determine the maximum size requirements to decode an encoded
    data buffer of \c in.size() bytes.

    \sa requiredSpace
*/

/*!
    \fn char16_t *QStringDecoder::appendToBuffer(char16_t *out, QByteArrayView in)
    \since 6.6
    \overload
*/

QT_END_NAMESPACE
