/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2018 Intel Corporation.
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

#include <qstringconverter.h>
#include <private/qstringconverter_p.h>
#include "qendian.h"

#include "private/qsimd_p.h"
#include "private/qstringiterator_p.h"

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

QT_BEGIN_NAMESPACE

enum { Endian = 0, Data = 1 };

static const uchar utf8bom[] = { 0xef, 0xbb, 0xbf };

#if (defined(__SSE2__) && defined(QT_COMPILER_SUPPORTS_SSE2)) \
    || (defined(__ARM_NEON__) && defined(Q_PROCESSOR_ARM_64))
static Q_ALWAYS_INLINE uint qBitScanReverse(unsigned v) noexcept
{
    uint result = qCountLeadingZeroBits(v);
    // Now Invert the result: clz will count *down* from the msb to the lsb, so the msb index is 31
    // and the lsb index is 0. The result for _bit_scan_reverse is expected to be the index when
    // counting up: msb index is 0 (because it starts there), and the lsb index is 31.
    result ^= sizeof(unsigned) * 8 - 1;
    return result;
}
#endif

#if defined(__SSE2__) && defined(QT_COMPILER_SUPPORTS_SSE2)
static inline bool simdEncodeAscii(uchar *&dst, const ushort *&nextAscii, const ushort *&src, const ushort *end)
{
    // do sixteen characters at a time
    for ( ; end - src >= 16; src += 16, dst += 16) {
#  ifdef __AVX2__
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src));
        __m128i data1 = _mm256_castsi256_si128(data);
        __m128i data2 = _mm256_extracti128_si256(data, 1);
#  else
        __m128i data1 = _mm_loadu_si128((const __m128i*)src);
        __m128i data2 = _mm_loadu_si128(1+(const __m128i*)src);
#  endif

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
        if (n) {
            // find the next probable ASCII character
            // we don't want to load 32 bytes again in this loop if we know there are non-ASCII
            // characters still coming
            nextAscii = src + qBitScanReverse(n) + 1;

            n = qCountTrailingZeroBits(n);
            dst += n;
            src += n;
            return false;
        }
    }

    if (end - src >= 8) {
        // do eight characters at a time
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));
        __m128i packed = _mm_packus_epi16(data, data);
        __m128i nonAscii = _mm_cmpgt_epi8(packed, _mm_setzero_si128());

        // store even non-ASCII
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dst), packed);

        uchar n = ~_mm_movemask_epi8(nonAscii);
        if (n) {
            nextAscii = src + qBitScanReverse(n) + 1;
            n = qCountTrailingZeroBits(n);
            dst += n;
            src += n;
            return false;
        }
    }

    return src == end;
}

static inline bool simdDecodeAscii(ushort *&dst, const uchar *&nextAscii, const uchar *&src, const uchar *end)
{
    // do sixteen characters at a time
    for ( ; end - src >= 16; src += 16, dst += 16) {
        __m128i data = _mm_loadu_si128((const __m128i*)src);

#ifdef __AVX2__
        const int BitSpacing = 2;
        // load and zero extend to an YMM register
        const __m256i extended = _mm256_cvtepu8_epi16(data);

        uint n = _mm256_movemask_epi8(extended);
        if (!n) {
            // store
            _mm256_storeu_si256((__m256i*)dst, extended);
            continue;
        }
#else
        const int BitSpacing = 1;

        // check if everything is ASCII
        // movemask extracts the high bit of every byte, so n is non-zero if something isn't ASCII
        uint n = _mm_movemask_epi8(data);
        if (!n) {
            // unpack
            _mm_storeu_si128((__m128i*)dst, _mm_unpacklo_epi8(data, _mm_setzero_si128()));
            _mm_storeu_si128(1+(__m128i*)dst, _mm_unpackhi_epi8(data, _mm_setzero_si128()));
            continue;
        }
#endif

        // copy the front part that is still ASCII
        while (!(n & 1)) {
            *dst++ = *src++;
            n >>= BitSpacing;
        }

        // find the next probable ASCII character
        // we don't want to load 16 bytes again in this loop if we know there are non-ASCII
        // characters still coming
        n = qBitScanReverse(n);
        nextAscii = src + (n / BitSpacing) + 1;
        return false;

    }

    if (end - src >= 8) {
        __m128i data = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(src));
        uint n = _mm_movemask_epi8(data) & 0xff;
        if (!n) {
            // unpack and store
            _mm_storeu_si128(reinterpret_cast<__m128i *>(dst), _mm_unpacklo_epi8(data, _mm_setzero_si128()));
        } else {
            while (!(n & 1)) {
                *dst++ = *src++;
                n >>= 1;
            }

            n = qBitScanReverse(n);
            nextAscii = src + n + 1;
            return false;
        }
    }

    return src == end;
}

static inline const uchar *simdFindNonAscii(const uchar *src, const uchar *end, const uchar *&nextAscii)
{
#ifdef __AVX2__
    // do 32 characters at a time
    // (this is similar to simdTestMask in qstring.cpp)
    const __m256i mask = _mm256_set1_epi8(0x80);
    for ( ; end - src >= 32; src += 32) {
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src));
        if (_mm256_testz_si256(mask, data))
            continue;

        uint n = _mm256_movemask_epi8(data);
        Q_ASSUME(n);

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
#elif defined(__ARM_NEON__) && defined(Q_PROCESSOR_ARM_64) // vaddv is only available on Aarch64
static inline bool simdEncodeAscii(uchar *&dst, const ushort *&nextAscii, const ushort *&src, const ushort *end)
{
    uint16x8_t maxAscii = vdupq_n_u16(0x7f);
    uint16x8_t mask1 = { 1,      1 << 2, 1 << 4, 1 << 6, 1 << 8, 1 << 10, 1 << 12, 1 << 14 };
    uint16x8_t mask2 = vshlq_n_u16(mask1, 1);

    // do sixteen characters at a time
    for ( ; end - src >= 16; src += 16, dst += 16) {
        // load 2 lanes (or: "load interleaved")
        uint16x8x2_t in = vld2q_u16(src);

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

static inline bool simdDecodeAscii(ushort *&dst, const uchar *&nextAscii, const uchar *&src, const uchar *end)
{
    // do eight characters at a time
    uint8x8_t msb_mask = vdup_n_u8(0x80);
    uint8x8_t add_mask = { 1, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7 };
    for ( ; end - src >= 8; src += 8, dst += 8) {
        uint8x8_t c = vld1_u8(src);
        uint8_t n = vaddv_u8(vand_u8(vcge_u8(c, msb_mask), add_mask));
        if (!n) {
            // store
            vst1q_u16(dst, vmovl_u8(c));
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
    uint8x8_t add_mask = { 1, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7 };
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
#else
static inline bool simdEncodeAscii(uchar *, const ushort *, const ushort *, const ushort *)
{
    return false;
}

static inline bool simdDecodeAscii(ushort *, const uchar *, const uchar *, const uchar *)
{
    return false;
}

static inline const uchar *simdFindNonAscii(const uchar *src, const uchar *end, const uchar *&nextAscii)
{
    nextAscii = end;
    return src;
}
#endif

enum { HeaderDone = 1 };

QByteArray QUtf8::convertFromUnicode(const QChar *uc, qsizetype len)
{
    // create a QByteArray with the worst case scenario size
    QByteArray result(len * 3, Qt::Uninitialized);
    uchar *dst = reinterpret_cast<uchar *>(const_cast<char *>(result.constData()));
    const ushort *src = reinterpret_cast<const ushort *>(uc);
    const ushort *const end = src + len;

    while (src != end) {
        const ushort *nextAscii = end;
        if (simdEncodeAscii(dst, nextAscii, src, end))
            break;

        do {
            ushort uc = *src++;
            int res = QUtf8Functions::toUtf8<QUtf8BaseTraits>(uc, dst, src, end);
            if (res < 0) {
                // encoding error - append '?'
                *dst++ = '?';
            }
        } while (src < nextAscii);
    }

    result.truncate(dst - reinterpret_cast<uchar *>(const_cast<char *>(result.constData())));
    return result;
}

QByteArray QUtf8::convertFromUnicode(const QChar *uc, qsizetype len, QStringConverterBase::State *state)
{
    QByteArray ba(3*len +3, Qt::Uninitialized);
    char *end = convertFromUnicode(ba.data(), QStringView(uc, len), state);
    ba.truncate(end - ba.data());
    return ba;
}

char *QUtf8::convertFromUnicode(char *out, QStringView in, QStringConverter::State *state)
{
    Q_ASSERT(state);
    const QChar *uc = in.data();
    qsizetype len = in.length();

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
    const ushort *src = reinterpret_cast<const ushort *>(uc);
    const ushort *const end = src + len;

    int surrogate_high = -1;
    if (state->remainingChars) {
        surrogate_high = state->state_data[0];
    } else if (!(state->internalState & HeaderDone) && state->flags & QStringConverter::Flag::WriteBom) {
        // append UTF-8 BOM
        *cursor++ = utf8bom[0];
        *cursor++ = utf8bom[1];
        *cursor++ = utf8bom[2];
        state->internalState |= HeaderDone;
    }

    const ushort *nextAscii = src;
    while (src != end) {
        int res;
        ushort uc;
        if (surrogate_high != -1) {
            uc = surrogate_high;
            surrogate_high = -1;
            res = QUtf8Functions::toUtf8<QUtf8BaseTraits>(uc, cursor, src, end);
        } else {
            if (src >= nextAscii && simdEncodeAscii(cursor, nextAscii, src, end))
                break;

            uc = *src++;
            res = QUtf8Functions::toUtf8<QUtf8BaseTraits>(uc, cursor, src, end);
        }
        if (Q_LIKELY(res >= 0))
            continue;

        if (res == QUtf8BaseTraits::Error) {
            // encoding error
            ++state->invalidChars;
            cursor = appendReplacementChar(cursor);
        } else if (res == QUtf8BaseTraits::EndOfString) {
            surrogate_high = uc;
            break;
        }
    }

    state->internalState |= HeaderDone;
    state->remainingChars = 0;
    if (surrogate_high >= 0) {
        if (state->flags & QStringConverter::Flag::Stateless) {
            ++state->invalidChars;
            cursor = appendReplacementChar(cursor);
        } else {
            state->remainingChars = 1;
            state->state_data[0] = surrogate_high;
        }
    }
    return reinterpret_cast<char *>(cursor);
}

QString QUtf8::convertToUnicode(const char *chars, qsizetype len)
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
    QString result(len, Qt::Uninitialized);
    QChar *data = const_cast<QChar*>(result.constData()); // we know we're not shared
    const QChar *end = convertToUnicode(data, chars, len);
    result.truncate(end - data);
    return result;
}

/*!
    \since 5.7
    \overload

    Converts the UTF-8 sequence of \a len octets beginning at \a chars to
    a sequence of QChar starting at \a buffer. The buffer is expected to be
    large enough to hold the result. An upper bound for the size of the
    buffer is \a len QChars.

    If, during decoding, an error occurs, a QChar::ReplacementCharacter is
    written.

    Returns a pointer to one past the last QChar written.

    This function never throws.
*/

QChar *QUtf8::convertToUnicode(QChar *buffer, const char *chars, qsizetype len) noexcept
{
    ushort *dst = reinterpret_cast<ushort *>(buffer);
    const uchar *src = reinterpret_cast<const uchar *>(chars);
    const uchar *end = src + len;

    // attempt to do a full decoding in SIMD
    const uchar *nextAscii = end;
    if (!simdDecodeAscii(dst, nextAscii, src, end)) {
        // at least one non-ASCII entry
        // check if we failed to decode the UTF-8 BOM; if so, skip it
        if (Q_UNLIKELY(src == reinterpret_cast<const uchar *>(chars))
                && end - src >= 3
                && Q_UNLIKELY(src[0] == utf8bom[0] && src[1] == utf8bom[1] && src[2] == utf8bom[2])) {
            src += 3;
        }

        while (src < end) {
            nextAscii = end;
            if (simdDecodeAscii(dst, nextAscii, src, end))
                break;

            do {
                uchar b = *src++;
                int res = QUtf8Functions::fromUtf8<QUtf8BaseTraits>(b, dst, src, end);
                if (res < 0) {
                    // decoding error
                    *dst++ = QChar::ReplacementCharacter;
                }
            } while (src < nextAscii);
        }
    }

    return reinterpret_cast<QChar *>(dst);
}

QString QUtf8::convertToUnicode(const char *chars, qsizetype len, QStringConverter::State *state)
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
    QString result(len + 1, Qt::Uninitialized);
    QChar *end = convertToUnicode(result.data(), chars, len, state);
    result.truncate(end - result.constData());
    return result;
}

QChar *QUtf8::convertToUnicode(QChar *out, const char *chars, qsizetype len, QStringConverter::State *state)
{
    Q_ASSERT(state);

    bool headerdone = state->internalState & HeaderDone || state->flags & QStringConverter::Flag::DontSkipInitialBom;

    ushort replacement = QChar::ReplacementCharacter;
    if (state->flags & QStringConverter::Flag::ConvertInvalidToNull)
        replacement = QChar::Null;

    int res;
    uchar ch = 0;

    ushort *dst = reinterpret_cast<ushort *>(out);
    const uchar *src = reinterpret_cast<const uchar *>(chars);
    const uchar *end = src + len;

    if (state->remainingChars) {
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
        if (res == QUtf8BaseTraits::Error || (res == QUtf8BaseTraits::EndOfString && len == 0)) {
            // special case for len == 0:
            // if we were supplied an empty string, terminate the previous, unfinished sequence with error
            ++state->invalidChars;
            *dst++ = replacement;
        } else if (res == QUtf8BaseTraits::EndOfString) {
            // if we got EndOfString again, then there were too few bytes in src;
            // copy to our state and return
            state->remainingChars = remainingCharsCount + newCharsToCopy;
            memcpy(&state->state_data[0], remainingCharsData, state->remainingChars);
            return out;
        } else if (!headerdone && res >= 0) {
            // eat the UTF-8 BOM
            headerdone = true;
            if (dst[-1] == 0xfeff)
                --dst;
        }

        // adjust src now that we have maybe consumed a few chars
        if (res >= 0) {
            Q_ASSERT(res > remainingCharsCount);
            src += res - remainingCharsCount;
        }
    }

    // main body, stateless decoding
    res = 0;
    const uchar *nextAscii = src;
    const uchar *start = src;
    while (res >= 0 && src < end) {
        if (src >= nextAscii && simdDecodeAscii(dst, nextAscii, src, end))
            break;

        ch = *src++;
        res = QUtf8Functions::fromUtf8<QUtf8BaseTraits>(ch, dst, src, end);
        if (!headerdone && res >= 0) {
            headerdone = true;
            if (src == start + 3) { // 3 == sizeof(utf8-bom)
                // eat the UTF-8 BOM (it can only appear at the beginning of the string).
                if (dst[-1] == 0xfeff)
                    --dst;
            }
        }
        if (res == QUtf8BaseTraits::Error) {
            res = 0;
            ++state->invalidChars;
            *dst++ = replacement;
        }
    }

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

    if (headerdone)
        state->internalState |= HeaderDone;

    return reinterpret_cast<QChar *>(dst);
}

struct QUtf8NoOutputTraits : public QUtf8BaseTraitsNoAscii
{
    struct NoOutput {};
    static void appendUtf16(const NoOutput &, ushort) {}
    static void appendUcs4(const NoOutput &, uint) {}
};

QUtf8::ValidUtf8Result QUtf8::isValidUtf8(const char *chars, qsizetype len)
{
    const uchar *src = reinterpret_cast<const uchar *>(chars);
    const uchar *end = src + len;
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
            int res = QUtf8Functions::fromUtf8<QUtf8NoOutputTraits>(b, output, src, end);
            if (res < 0) {
                // decoding error
                return { false, false };
            }
        } while (src < nextAscii);
    }

    return { true, isValidAscii };
}

int QUtf8::compareUtf8(const char *utf8, qsizetype u8len, const QChar *utf16, qsizetype u16len)
{
    uint uc1, uc2;
    auto src1 = reinterpret_cast<const uchar *>(utf8);
    auto end1 = src1 + u8len;
    QStringIterator src2(utf16, utf16 + u16len);

    while (src1 < end1 && src2.hasNext()) {
        uchar b = *src1++;
        uint *output = &uc1;
        int res = QUtf8Functions::fromUtf8<QUtf8BaseTraits>(b, output, src1, end1);
        if (res < 0) {
            // decoding error
            uc1 = QChar::ReplacementCharacter;
        }

        uc2 = src2.next();
        if (uc1 != uc2)
            return int(uc1) - int(uc2);
    }

    // the shorter string sorts first
    return (end1 > src1) - int(src2.hasNext());
}

int QUtf8::compareUtf8(const char *utf8, qsizetype u8len, QLatin1String s)
{
    uint uc1;
    auto src1 = reinterpret_cast<const uchar *>(utf8);
    auto end1 = src1 + u8len;
    auto src2 = reinterpret_cast<const uchar *>(s.latin1());
    auto end2 = src2 + s.size();

    while (src1 < end1 && src2 < end2) {
        uchar b = *src1++;
        uint *output = &uc1;
        int res = QUtf8Functions::fromUtf8<QUtf8BaseTraits>(b, output, src1, end1);
        if (res < 0) {
            // decoding error
            uc1 = QChar::ReplacementCharacter;
        }

        uint uc2 = *src2++;
        if (uc1 != uc2)
            return int(uc1) - int(uc2);
    }

    // the shorter string sorts first
    return (end1 > src1) - (end2 > src2);
}

QByteArray QUtf16::convertFromUnicode(const QChar *uc, qsizetype len, QStringConverter::State *state, DataEndianness endian)
{
    bool writeBom = !(state->internalState & HeaderDone) && state->flags & QStringConverter::Flag::WriteBom;
    qsizetype length =  2*len;
    if (writeBom)
        length += 2;

    QByteArray d(length, Qt::Uninitialized);
    char *end = convertFromUnicode(d.data(), QStringView(uc, len), state, endian);
    Q_ASSERT(end - d.constData() == d.length());
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
        QChar bom(QChar::ByteOrderMark);
        if (endian == BigEndianness)
            qToBigEndian(bom.unicode(), out);
        else
            qToLittleEndian(bom.unicode(), out);
        out += 2;
    }
    if (endian == BigEndianness)
        qToBigEndian<ushort>(in.data(), in.length(), out);
    else
        qToLittleEndian<ushort>(in.data(), in.length(), out);

    state->remainingChars = 0;
    state->internalState |= HeaderDone;
    return out + 2*in.length();
}

QString QUtf16::convertToUnicode(const char *chars, qsizetype len, QStringConverter::State *state, DataEndianness endian)
{
    QString result((len + 1) >> 1, Qt::Uninitialized); // worst case
    QChar *qch = convertToUnicode(result.data(), chars, len, state, endian);
    result.truncate(qch - result.constData());
    return result;
}

QChar *QUtf16::convertToUnicode(QChar *out, const char *chars, qsizetype len, QStringConverter::State *state, DataEndianness endian)
{
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
    if (state->flags & QStringConverter::Flag::DontSkipInitialBom)
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

    int nPairs = (end - chars) >> 1;
    if (endian == BigEndianness)
        qFromBigEndian<ushort>(chars, nPairs, out);
    else
        qFromLittleEndian<ushort>(chars, nPairs, out);
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

QByteArray QUtf32::convertFromUnicode(const QChar *uc, qsizetype len, QStringConverter::State *state, DataEndianness endian)
{
    bool writeBom = !(state->internalState & HeaderDone) && state->flags & QStringConverter::Flag::WriteBom;
    int length =  4*len;
    if (writeBom)
        length += 4;
    QByteArray ba(length, Qt::Uninitialized);
    char *end = convertFromUnicode(ba.data(), QStringView(uc, len), state, endian);
    Q_ASSERT(end - ba.constData() == length);
    Q_UNUSED(end);
    return ba;
}

char *QUtf32::convertFromUnicode(char *out, QStringView in, QStringConverter::State *state, DataEndianness endian)
{
    Q_ASSERT(state);

    bool writeBom = !(state->internalState & HeaderDone) && state->flags & QStringConverter::Flag::WriteBom;
    qsizetype length =  4*in.length();
    if (writeBom)
        length += 4;

    if (endian == DetectEndianness)
        endian = (QSysInfo::ByteOrder == QSysInfo::BigEndian) ? BigEndianness : LittleEndianness;

    if (writeBom) {
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
    const QChar *end = in.data() + in.length();
    QChar ch;
    uint ucs4;
    if (state->remainingChars == 1) {
        ch = state->state_data[Data];
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

QString QUtf32::convertToUnicode(const char *chars, qsizetype len, QStringConverter::State *state, DataEndianness endian)
{
    QString result;
    result.resize((len + 7) >> 1); // worst case
    QChar *end = convertToUnicode(result.data(), chars, len, state, endian);
    result.truncate(end - result.constData());
    return result;
}

QChar *QUtf32::convertToUnicode(QChar *out, const char *chars, qsizetype len, QStringConverter::State *state, DataEndianness endian)
{
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
    if (state->flags & QStringConverter::Flag::DontSkipInitialBom)
        headerdone = true;

    int num = state->remainingChars;
    state->remainingChars = 0;

    if (!headerdone || endian == DetectEndianness || num) {
        while (num < 4)
            tuple[num++] = *chars++;
        if (endian == DetectEndianness) {
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
        uint code = (endian == BigEndianness) ? qFromBigEndian<quint32>(tuple) : qFromLittleEndian<quint32>(tuple);
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
            uint code = (endian == BigEndianness) ? qFromBigEndian<quint32>(tuple) : qFromLittleEndian<quint32>(tuple);
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

QString qFromUtfEncoded(const QByteArray &ba)
{
    const qsizetype arraySize = ba.size();
    const uchar *buf = reinterpret_cast<const uchar *>(ba.constData());
    const uint bom = 0xfeff;

    if (arraySize > 3) {
        uint uc = qFromUnaligned<uint>(buf);
        if (uc == qToBigEndian(bom) || uc == qToLittleEndian(bom))
            return QUtf32::convertToUnicode(ba.constData(), ba.length(), nullptr); // utf-32
    }

    if (arraySize > 1) {
        ushort uc = qFromUnaligned<ushort>(buf);
        if (uc == qToBigEndian(ushort(bom)) || qToLittleEndian(ushort(bom)))
            return QUtf16::convertToUnicode(ba.constData(), ba.length(), nullptr); // utf-16
    }
    return QUtf8::convertToUnicode(ba.constData(), ba.length());
}

#if defined(Q_OS_WIN) && !defined(QT_BOOTSTRAPPED)
static QString convertToUnicodeCharByChar(const char *chars, qsizetype length, QStringConverter::State *state)
{
    Q_ASSERT(state);
    if (state->flags & QStringConverter::Flag::Stateless) // temporary
        state = nullptr;

    if (!chars || !length)
        return QString();

    int copyLocation = 0;
    int extra = 2;
    if (state && state->remainingChars) {
        copyLocation = state->remainingChars;
        extra += copyLocation;
    }
    int newLength = length + extra;
    char *mbcs = new char[newLength];
    //ensure that we have a NULL terminated string
    mbcs[newLength-1] = 0;
    mbcs[newLength-2] = 0;
    memcpy(&(mbcs[copyLocation]), chars, length);
    if (copyLocation) {
        //copy the last character from the state
        mbcs[0] = (char)state->state_data[0];
        state->remainingChars = 0;
    }
    const char *mb = mbcs;
#if !defined(Q_OS_WINRT)
    const char *next = 0;
    QString s;
    while ((next = CharNextExA(CP_ACP, mb, 0)) != mb) {
        wchar_t wc[2] ={0};
        int charlength = next - mb;
        int len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED|MB_ERR_INVALID_CHARS, mb, charlength, wc, 2);
        if (len>0) {
            s.append(QChar(wc[0]));
        } else {
            int r = GetLastError();
            //check if the character being dropped is the last character
            if (r == ERROR_NO_UNICODE_TRANSLATION && mb == (mbcs+newLength -3) && state) {
                state->remainingChars = 1;
                state->state_data[0] = (char)*mb;
            }
        }
        mb = next;
    }
#else
    QString s;
    size_t size = mbstowcs(NULL, mb, length);
    if (size == size_t(-1)) {
        Q_ASSERT("Error in CE TextCodec");
        return QString();
    }
    wchar_t* ws = new wchar_t[size + 2];
    ws[size +1] = 0;
    ws[size] = 0;
    size = mbstowcs(ws, mb, length);
    for (size_t i = 0; i < size; i++)
        s.append(QChar(ws[i]));
    delete [] ws;
#endif
    delete [] mbcs;
    return s;
}


QString QLocal8Bit::convertToUnicode(const char *chars, qsizetype length, QStringConverter::State *state)
{
    Q_ASSERT(length < INT_MAX); // ### FIXME
    const char *mb = chars;
    int mblen = length;

    if (!mb || !mblen)
        return QString();

    QVarLengthArray<wchar_t, 4096> wc(4096);
    int len;
    QString sp;
    bool prepend = false;
    char state_data = 0;
    int remainingChars = 0;

    //save the current state information
    if (state) {
        state_data = (char)state->state_data[0];
        remainingChars = state->remainingChars;
    }

    //convert the pending character (if available)
    if (state && remainingChars) {
        char prev[3] = {0};
        prev[0] = state_data;
        prev[1] = mb[0];
        remainingChars = 0;
        len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                                    prev, 2, wc.data(), wc.length());
        if (len) {
            sp.append(QChar(wc[0]));
            if (mblen == 1) {
                state->remainingChars = 0;
                return sp;
            }
            prepend = true;
            mb++;
            mblen--;
            wc[0] = 0;
        }
    }

    while (!(len=MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED|MB_ERR_INVALID_CHARS,
                mb, mblen, wc.data(), wc.length()))) {
        int r = GetLastError();
        if (r == ERROR_INSUFFICIENT_BUFFER) {
                const int wclen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                                    mb, mblen, 0, 0);
                wc.resize(wclen);
        } else if (r == ERROR_NO_UNICODE_TRANSLATION) {
            //find the last non NULL character
            while (mblen > 1  && !(mb[mblen-1]))
                mblen--;
            //check whether,  we hit an invalid character in the middle
            if ((mblen <= 1) || (remainingChars && state_data))
                return convertToUnicodeCharByChar(chars, length, state);
            //Remove the last character and try again...
            state_data = mb[mblen-1];
            remainingChars = 1;
            mblen--;
        } else {
            // Fail.
            qWarning("MultiByteToWideChar: Cannot convert multibyte text");
            break;
        }
    }

    if (len <= 0)
        return QString();

    if (wc[len-1] == 0) // len - 1: we don't want terminator
        --len;

    //save the new state information
    if (state) {
        state->state_data[0] = (char)state_data;
        state->remainingChars = remainingChars;
    }
    QString s((QChar*)wc.data(), len);
    if (prepend) {
        return sp+s;
    }
    return s;
}

QByteArray QLocal8Bit::convertFromUnicode(const QChar *ch, qsizetype uclen, QStringConverter::State *state)
{
    Q_ASSERT(uclen < INT_MAX); // ### FIXME
    Q_ASSERT(state);
    Q_UNUSED(state); // ### Fixme
    if (state->flags & QStringConverter::Flag::Stateless) // temporary
        state = nullptr;

    if (!ch)
        return QByteArray();
    if (uclen == 0)
        return QByteArray("");
    BOOL used_def;
    QByteArray mb(4096, 0);
    int len;
    while (!(len=WideCharToMultiByte(CP_ACP, 0, (const wchar_t*)ch, uclen,
                mb.data(), mb.size()-1, 0, &used_def)))
    {
        int r = GetLastError();
        if (r == ERROR_INSUFFICIENT_BUFFER) {
            mb.resize(1+WideCharToMultiByte(CP_ACP, 0,
                                (const wchar_t*)ch, uclen,
                                0, 0, 0, &used_def));
                // and try again...
        } else {
            // Fail.  Probably can't happen in fact (dwFlags is 0).
#ifndef QT_NO_DEBUG
            // Can't use qWarning(), as it'll recurse to handle %ls
            fprintf(stderr,
                    "WideCharToMultiByte: Cannot convert multibyte text (error %d): %ls\n",
                    r, reinterpret_cast<const wchar_t*>(QString(ch, uclen).utf16()));
#endif
            break;
        }
    }
    mb.resize(len);
    return mb;
}
#endif

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
    \value DontSkipInitialBom When converting from an input encoding to a QString the QTextDecoder usually skips an
                              leading QChar::ByteOrderMark. When this flag is set, the byte order mark will not be
                              skipped, but inserted at the start of the created QString.

    \value Stateless Ignore possible converter states between different function calls
           to encode or decode strings.
*/


void QStringConverter::State::clear()
{
    if (clearFn)
        clearFn(this);
    else
        state_data[0] = state_data[1] = state_data[2] = state_data[3] = 0;
    remainingChars = 0;
    invalidChars = 0;
    internalState = 0;
}

static QChar *fromUtf16(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    return QUtf16::convertToUnicode(out, in, length, state, DetectEndianness);
}

static char *toUtf16(char *out, QStringView in, QStringConverter::State *state)
{
    return QUtf16::convertFromUnicode(out, in, state, DetectEndianness);
}

static QChar *fromUtf16BE(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    return QUtf16::convertToUnicode(out, in, length, state, BigEndianness);
}

static char *toUtf16BE(char *out, QStringView in, QStringConverter::State *state)
{
    return QUtf16::convertFromUnicode(out, in, state, BigEndianness);
}

static QChar *fromUtf16LE(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    return QUtf16::convertToUnicode(out, in, length, state, LittleEndianness);
}

static char *toUtf16LE(char *out, QStringView in, QStringConverter::State *state)
{
    return QUtf16::convertFromUnicode(out, in, state, LittleEndianness);
}

static QChar *fromUtf32(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    return QUtf32::convertToUnicode(out, in, length, state, DetectEndianness);
}

static char *toUtf32(char *out, QStringView in, QStringConverter::State *state)
{
    return QUtf32::convertFromUnicode(out, in, state, DetectEndianness);
}

static QChar *fromUtf32BE(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    return QUtf32::convertToUnicode(out, in, length, state, BigEndianness);
}

static char *toUtf32BE(char *out, QStringView in, QStringConverter::State *state)
{
    return QUtf32::convertFromUnicode(out, in, state, BigEndianness);
}

static QChar *fromUtf32LE(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    return QUtf32::convertToUnicode(out, in, length, state, LittleEndianness);
}

static char *toUtf32LE(char *out, QStringView in, QStringConverter::State *state)
{
    return QUtf32::convertFromUnicode(out, in, state, LittleEndianness);
}

void qt_from_latin1(char16_t *dst, const char *str, size_t size) noexcept;

static QChar *fromLatin1(QChar *out, const char *chars, qsizetype len, QStringConverter::State *state)
{
    Q_ASSERT(state);
    Q_UNUSED(state);

    qt_from_latin1(reinterpret_cast<char16_t *>(out), chars, size_t(len));
    return out + len;
}


static char *toLatin1(char *out, QStringView in, QStringConverter::State *state)
{
    Q_ASSERT(state);
    if (state->flags & QStringConverter::Flag::Stateless) // temporary
        state = nullptr;

    const char replacement = (state && state->flags & QStringConverter::Flag::ConvertInvalidToNull) ? 0 : '?';
    int invalid = 0;
    for (qsizetype i = 0; i < in.length(); ++i) {
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

static QChar *fromLocal8Bit(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    QString s = QLocal8Bit::convertToUnicode(in, length, state);
    memcpy(out, s.constData(), s.length()*sizeof(QChar));
    return out + s.length();
}

static char *toLocal8Bit(char *out, QStringView in, QStringConverter::State *state)
{
    QByteArray s = QLocal8Bit::convertFromUnicode(in.data(), in.length(), state);
    memcpy(out, s.constData(), s.length());
    return out + s.length();
}


static qsizetype fromUtf8Len(qsizetype l) { return l + 1; }
static qsizetype toUtf8Len(qsizetype l) { return 3*(l + 1); }

static qsizetype fromUtf16Len(qsizetype l) { return l/2 + 2; }
static qsizetype toUtf16Len(qsizetype l) { return 2*(l + 1); }

static qsizetype fromUtf32Len(qsizetype l) { return l/2 + 2; }
static qsizetype toUtf32Len(qsizetype l) { return 4*(l + 1); }

static qsizetype fromLatin1Len(qsizetype l) { return l + 1; }
static qsizetype toLatin1Len(qsizetype l) { return l + 1; }

const QStringConverter::Interface QStringConverter::encodingInterfaces[QStringConverter::LastEncoding + 1] =
{
    { "UTF-8", QUtf8::convertToUnicode, fromUtf8Len, QUtf8::convertFromUnicode, toUtf8Len },
    { "UTF-16", fromUtf16, fromUtf16Len, toUtf16, toUtf16Len },
    { "UTF-16LE", fromUtf16LE, fromUtf16Len, toUtf16LE, toUtf16Len },
    { "UTF-16BE", fromUtf16BE, fromUtf16Len, toUtf16BE, toUtf16Len },
    { "UTF-32", fromUtf32, fromUtf32Len, toUtf32, toUtf32Len },
    { "UTF-32LE", fromUtf32LE, fromUtf32Len, toUtf32LE, toUtf32Len },
    { "UTF-32BE", fromUtf32BE, fromUtf32Len, toUtf32BE, toUtf32Len },
    { "ISO-8859-1", fromLatin1, fromLatin1Len, toLatin1, toLatin1Len },
    { "Locale", fromLocal8Bit, fromUtf8Len, toLocal8Bit, toUtf8Len }
};

// match names case insensitive and skipping '-' and '_'
static bool nameMatch(const char *a, const char *b)
{
    while (*a && *b) {
        if (*a == '-' || *a == '_') {
            ++a;
            continue;
        }
        if (*b == '-' || *b == '_') {
            ++b;
            continue;
        }
        if (toupper(*a) != toupper(*b))
            return false;
        ++a;
        ++b;
    }
    return !*a && !*b;
}

QStringConverter::QStringConverter(const char *name)
    : iface(nullptr)
{
    auto e = encodingForName(name);
    if (e)
        iface = encodingInterfaces + int(e.value());
}

std::optional<QStringConverter::Encoding> QStringConverter::encodingForName(const char *name)
{
    for (int i = 0; i < LastEncoding + 1; ++i) {
        if (nameMatch(encodingInterfaces[i].name, name))
            return QStringConverter::Encoding(i);
    }
    if (nameMatch(name, "latin1"))
        return QStringConverter::Latin1;
    return std::nullopt;
}

std::optional<QStringConverter::Encoding> QStringConverter::encodingForData(const char *buf, qsizetype arraySize, char16_t expectedFirstCharacter)
{
    if (arraySize > 3) {
        uint uc = qFromUnaligned<uint>(buf);
        if (uc == qToBigEndian(uint(QChar::ByteOrderMark)))
            return QStringConverter::Utf32BE;
        if (uc == qToLittleEndian(uint(QChar::ByteOrderMark)))
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
        static const char utf8bom[] = "\xef\xbb\xbf";
        if (memcmp(buf, utf8bom, sizeof(utf8bom) - 1) == 0)
            return QStringConverter::Utf8;
    }

    if (arraySize > 1) {
        ushort uc = qFromUnaligned<ushort>(buf);
        if (uc == qToBigEndian(ushort(QChar::ByteOrderMark)))
            return QStringConverter::Utf16BE;
        if (uc == qToLittleEndian(ushort(QChar::ByteOrderMark)))
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

const char *QStringConverter::nameForEncoding(QStringConverter::Encoding e)
{
    return encodingInterfaces[int(e)].name;
}

QT_END_NAMESPACE
