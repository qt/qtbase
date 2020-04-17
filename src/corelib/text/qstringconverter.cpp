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

QByteArray QUtf8::convertFromUnicode(const QChar *uc, qsizetype len, QStringConverter::State *state)
{
    uchar replacement = '?';
    qsizetype rlen = 3*len;
    int surrogate_high = -1;
    if (state) {
        if (state->flags & QStringConverter::ConvertInvalidToNull)
            replacement = 0;
        if (!(state->flags & QStringConverter::IgnoreHeader))
            rlen += 3;
        if (state->remainingChars)
            surrogate_high = state->state_data[0];
    }


    QByteArray rstr(rlen, Qt::Uninitialized);
    uchar *cursor = reinterpret_cast<uchar *>(const_cast<char *>(rstr.constData()));
    const ushort *src = reinterpret_cast<const ushort *>(uc);
    const ushort *const end = src + len;

    int invalid = 0;
    if (state && !(state->flags & QStringConverter::IgnoreHeader)) {
        // append UTF-8 BOM
        *cursor++ = utf8bom[0];
        *cursor++ = utf8bom[1];
        *cursor++ = utf8bom[2];
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
            ++invalid;
            *cursor++ = replacement;
        } else if (res == QUtf8BaseTraits::EndOfString) {
            surrogate_high = uc;
            break;
        }
    }

    rstr.resize(cursor - (const uchar*)rstr.constData());
    if (state) {
        state->invalidChars += invalid;
        state->flags |= QStringConverter::IgnoreHeader;
        state->remainingChars = 0;
        if (surrogate_high >= 0) {
            state->remainingChars = 1;
            state->state_data[0] = surrogate_high;
        }
    }
    return rstr;
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
    bool headerdone = false;
    ushort replacement = QChar::ReplacementCharacter;
    int invalid = 0;
    int res;
    uchar ch = 0;

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

    ushort *dst = reinterpret_cast<ushort *>(const_cast<QChar *>(result.constData()));
    const uchar *src = reinterpret_cast<const uchar *>(chars);
    const uchar *end = src + len;

    if (state) {
        if (state->flags & QStringConverter::IgnoreHeader)
            headerdone = true;
        if (state->flags & QStringConverter::ConvertInvalidToNull)
            replacement = QChar::Null;
        if (state->remainingChars) {
            // handle incoming state first
            uchar remainingCharsData[4]; // longest UTF-8 sequence possible
            qsizetype remainingCharsCount = state->remainingChars;
            qsizetype newCharsToCopy = qMin<int>(sizeof(remainingCharsData) - remainingCharsCount, end - src);

            memset(remainingCharsData, 0, sizeof(remainingCharsData));
            memcpy(remainingCharsData, &state->state_data[0], remainingCharsCount);
            memcpy(remainingCharsData + remainingCharsCount, src, newCharsToCopy);

            const uchar *begin = &remainingCharsData[1];
            res = QUtf8Functions::fromUtf8<QUtf8BaseTraits>(remainingCharsData[0], dst, begin,
                    static_cast<const uchar *>(remainingCharsData) + remainingCharsCount + newCharsToCopy);
            if (res == QUtf8BaseTraits::Error || (res == QUtf8BaseTraits::EndOfString && len == 0)) {
                // special case for len == 0:
                // if we were supplied an empty string, terminate the previous, unfinished sequence with error
                ++invalid;
                *dst++ = replacement;
            } else if (res == QUtf8BaseTraits::EndOfString) {
                // if we got EndOfString again, then there were too few bytes in src;
                // copy to our state and return
                state->remainingChars = remainingCharsCount + newCharsToCopy;
                memcpy(&state->state_data[0], remainingCharsData, state->remainingChars);
                return QString();
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
            ++invalid;
            *dst++ = replacement;
        }
    }

    if (!state && res == QUtf8BaseTraits::EndOfString) {
        // unterminated UTF sequence
        *dst++ = QChar::ReplacementCharacter;
        while (src++ < end)
            *dst++ = QChar::ReplacementCharacter;
    }

    result.truncate(dst - (const ushort *)result.unicode());
    if (state) {
        state->invalidChars += invalid;
        if (headerdone)
            state->flags |= QStringConverter::IgnoreHeader;
        if (res == QUtf8BaseTraits::EndOfString) {
            --src; // unread the byte in ch
            state->remainingChars = end - src;
            memcpy(&state->state_data[0], src, end - src);
        } else {
            state->remainingChars = 0;
        }
    }
    return result;
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

QByteArray QUtf16::convertFromUnicode(const QChar *uc, qsizetype len, QStringConverter::State *state, DataEndianness e)
{
    DataEndianness endian = e;
    qsizetype length =  2*len;
    if (!state || (!(state->flags & QStringConverter::IgnoreHeader))) {
        length += 2;
    }
    if (e == DetectEndianness) {
        endian = (QSysInfo::ByteOrder == QSysInfo::BigEndian) ? BigEndianness : LittleEndianness;
    }

    QByteArray d;
    d.resize(length);
    char *data = d.data();
    if (!state || !(state->flags & QStringConverter::IgnoreHeader)) {
        QChar bom(QChar::ByteOrderMark);
        if (endian == BigEndianness)
            qToBigEndian(bom.unicode(), data);
        else
            qToLittleEndian(bom.unicode(), data);
        data += 2;
    }
    if (endian == BigEndianness)
        qToBigEndian<ushort>(uc, len, data);
    else
        qToLittleEndian<ushort>(uc, len, data);

    if (state) {
        state->remainingChars = 0;
        state->flags |= QStringConverter::IgnoreHeader;
    }
    return d;
}

QString QUtf16::convertToUnicode(const char *chars, qsizetype len, QStringConverter::State *state, DataEndianness e)
{
    DataEndianness endian = e;
    bool half = false;
    uchar buf = 0;
    bool headerdone = false;
    if (state) {
        headerdone = state->flags & QStringConverter::IgnoreHeader;
        if (endian == DetectEndianness)
            endian = (DataEndianness)state->state_data[Endian];
        if (state->remainingChars) {
            half = true;
            buf = state->state_data[Data];
        }
    }
    if (headerdone && endian == DetectEndianness)
        endian = (QSysInfo::ByteOrder == QSysInfo::BigEndian) ? BigEndianness : LittleEndianness;

    QString result(len, Qt::Uninitialized); // worst case
    QChar *qch = (QChar *)result.data();
    while (len--) {
        if (half) {
            QChar ch;
            if (endian == LittleEndianness) {
                ch.setRow(*chars++);
                ch.setCell(buf);
            } else {
                ch.setRow(buf);
                ch.setCell(*chars++);
            }
            if (!headerdone) {
                headerdone = true;
                if (endian == DetectEndianness) {
                    if (ch == QChar::ByteOrderSwapped) {
                        endian = LittleEndianness;
                    } else if (ch == QChar::ByteOrderMark) {
                        endian = BigEndianness;
                    } else {
                        if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
                            endian = BigEndianness;
                        } else {
                            endian = LittleEndianness;
                            ch = QChar::fromUcs2((ch.unicode() >> 8) | ((ch.unicode() & 0xff) << 8));
                        }
                        *qch++ = ch;
                    }
                } else if (ch != QChar::ByteOrderMark) {
                    *qch++ = ch;
                }
            } else {
                *qch++ = ch;
            }
            half = false;
        } else {
            buf = *chars++;
            half = true;
        }
    }
    result.truncate(qch - result.unicode());

    if (state) {
        if (headerdone)
            state->flags |= QStringConverter::IgnoreHeader;
        state->state_data[Endian] = endian;
        if (half) {
            state->remainingChars = 1;
            state->state_data[Data] = buf;
        } else {
            state->remainingChars = 0;
            state->state_data[Data] = 0;
        }
    }
    return result;
}

QByteArray QUtf32::convertFromUnicode(const QChar *uc, qsizetype len, QStringConverter::State *state, DataEndianness e)
{
    DataEndianness endian = e;
    qsizetype length =  4*len;
    if (!state || (!(state->flags & QStringConverter::IgnoreHeader))) {
        length += 4;
    }
    if (e == DetectEndianness) {
        endian = (QSysInfo::ByteOrder == QSysInfo::BigEndian) ? BigEndianness : LittleEndianness;
    }

    QByteArray d(length, Qt::Uninitialized);
    char *data = d.data();
    if (!state || !(state->flags & QStringConverter::IgnoreHeader)) {
        if (endian == BigEndianness) {
            data[0] = 0;
            data[1] = 0;
            data[2] = (char)0xfe;
            data[3] = (char)0xff;
        } else {
            data[0] = (char)0xff;
            data[1] = (char)0xfe;
            data[2] = 0;
            data[3] = 0;
        }
        data += 4;
    }

    QStringIterator i(uc, uc + len);
    if (endian == BigEndianness) {
        while (i.hasNext()) {
            uint cp = i.next();
            qToBigEndian(cp, data);
            data += 4;
        }
    } else {
        while (i.hasNext()) {
            uint cp = i.next();
            qToLittleEndian(cp, data);
            data += 4;
        }
    }

    if (state) {
        state->remainingChars = 0;
        state->flags |= QStringConverter::IgnoreHeader;
    }
    return d;
}

QString QUtf32::convertToUnicode(const char *chars, qsizetype len, QStringConverter::State *state, DataEndianness e)
{
    DataEndianness endian = e;
    uchar tuple[4];
    int num = 0;
    bool headerdone = false;
    if (state) {
        headerdone = state->flags & QStringConverter::IgnoreHeader;
        if (endian == DetectEndianness) {
            endian = (DataEndianness)state->state_data[Endian];
        }
        num = state->remainingChars;
        memcpy(tuple, &state->state_data[Data], 4);
    }
    if (headerdone && endian == DetectEndianness)
        endian = (QSysInfo::ByteOrder == QSysInfo::BigEndian) ? BigEndianness : LittleEndianness;

    QString result;
    result.resize((num + len) >> 2 << 1); // worst case
    QChar *qch = (QChar *)result.data();

    const char *end = chars + len;
    while (chars < end) {
        tuple[num++] = *chars++;
        if (num == 4) {
            if (!headerdone) {
                headerdone = true;
                if (endian == DetectEndianness) {
                    if (tuple[0] == 0xff && tuple[1] == 0xfe && tuple[2] == 0 && tuple[3] == 0 && endian != BigEndianness) {
                        endian = LittleEndianness;
                        num = 0;
                        continue;
                    } else if (tuple[0] == 0 && tuple[1] == 0 && tuple[2] == 0xfe && tuple[3] == 0xff && endian != LittleEndianness) {
                        endian = BigEndianness;
                        num = 0;
                        continue;
                    } else if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
                        endian = BigEndianness;
                    } else {
                        endian = LittleEndianness;
                    }
                } else if (((endian == BigEndianness) ? qFromBigEndian<quint32>(tuple) : qFromLittleEndian<quint32>(tuple)) == QChar::ByteOrderMark) {
                    num = 0;
                    continue;
                }
            }
            uint code = (endian == BigEndianness) ? qFromBigEndian<quint32>(tuple) : qFromLittleEndian<quint32>(tuple);
            for (char16_t c : QChar::fromUcs4(code))
                *qch++ = c;
            num = 0;
        }
    }
    result.truncate(qch - result.unicode());

    if (state) {
        if (headerdone)
            state->flags |= QStringConverter::IgnoreHeader;
        state->state_data[Endian] = endian;
        state->remainingChars = num;
        memcpy(&state->state_data[Data], tuple, 4);
    }
    return result;
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

/*!
    \enum QStringConverter::Flag

    \value DefaultConversion  No flag is set.
    \value ConvertInvalidToNull  If this flag is set, each invalid input
                                 character is output as a null character.
    \value IgnoreHeader  Ignore any Unicode byte-order mark and don't generate any.

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
}

static QChar *fromUtf8(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    QString s = QUtf8::convertToUnicode(in, length, state);
    memcpy(out, s.constData(), s.length()*sizeof(QChar));
    return out + s.length();
}

static char *toUtf8(char *out, QStringView in, QStringConverter::State *state)
{
    QByteArray s = QUtf8::convertFromUnicode(in.data(), in.length(), state);
    memcpy(out, s.constData(), s.length());
    return out + s.length();
}

static QChar *fromUtf16(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    QString s = QUtf16::convertToUnicode(in, length, state);
    memcpy(out, s.constData(), s.length()*sizeof(QChar));
    return out + s.length();
}

static char *toUtf16(char *out, QStringView in, QStringConverter::State *state)
{
    QByteArray s = QUtf16::convertFromUnicode(in.data(), in.length(), state);
    memcpy(out, s.constData(), s.length());
    return out + s.length();
}

static QChar *fromUtf16BE(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    QString s = QUtf16::convertToUnicode(in, length, state, BigEndianness);
    memcpy(out, s.constData(), s.length()*sizeof(QChar));
    return out + s.length();
}

static char *toUtf16BE(char *out, QStringView in, QStringConverter::State *state)
{
    QByteArray s = QUtf16::convertFromUnicode(in.data(), in.length(), state, BigEndianness);
    memcpy(out, s.constData(), s.length());
    return out + s.length();
}

static QChar *fromUtf16LE(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    QString s = QUtf16::convertToUnicode(in, length, state, LittleEndianness);
    memcpy(out, s.constData(), s.length()*sizeof(QChar));
    return out + s.length();
}

static char *toUtf16LE(char *out, QStringView in, QStringConverter::State *state)
{
    QByteArray s = QUtf16::convertFromUnicode(in.data(), in.length(), state, LittleEndianness);
    memcpy(out, s.constData(), s.length());
    return out + s.length();
}

static QChar *fromUtf32(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    QString s = QUtf32::convertToUnicode(in, length, state);
    memcpy(out, s.constData(), s.length()*sizeof(QChar));
    return out + s.length();
}

static char *toUtf32(char *out, QStringView in, QStringConverter::State *state)
{
    QByteArray s = QUtf32::convertFromUnicode(in.data(), in.length(), state);
    memcpy(out, s.constData(), s.length());
    return out + s.length();
}

static QChar *fromUtf32BE(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    QString s = QUtf32::convertToUnicode(in, length, state, BigEndianness);
    memcpy(out, s.constData(), s.length()*sizeof(QChar));
    return out + s.length();
}

static char *toUtf32BE(char *out, QStringView in, QStringConverter::State *state)
{
    QByteArray s = QUtf32::convertFromUnicode(in.data(), in.length(), state, BigEndianness);
    memcpy(out, s.constData(), s.length());
    return out + s.length();
}

static QChar *fromUtf32LE(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    QString s = QUtf32::convertToUnicode(in, length, state, LittleEndianness);
    memcpy(out, s.constData(), s.length()*sizeof(QChar));
    return out + s.length();
}

static char *toUtf32LE(char *out, QStringView in, QStringConverter::State *state)
{
    QByteArray s = QUtf32::convertFromUnicode(in.data(), in.length(), state, LittleEndianness);
    memcpy(out, s.constData(), s.length());
    return out + s.length();
}

static qsizetype fromUtf8Len(qsizetype l) { return l + 1; }
static qsizetype toUtf8Len(qsizetype l) { return 3*(l + 1); }

static qsizetype fromUtf16Len(qsizetype l) { return l/2 + 2; }
static qsizetype toUtf16Len(qsizetype l) { return 2*(l + 1); }

static qsizetype fromUtf32Len(qsizetype l) { return l + 1; }
static qsizetype toUtf32Len(qsizetype l) { return 4*(l + 1); }

const QStringConverter::Interface QStringConverter::encodingInterfaces[QStringConverter::LastEncoding + 1] =
{
    { fromUtf8, fromUtf8Len, toUtf8, toUtf8Len },
    { fromUtf16, fromUtf16Len, toUtf16, toUtf16Len },
    { fromUtf16LE, fromUtf16Len, toUtf16LE, toUtf16Len },
    { fromUtf16BE, fromUtf16Len, toUtf16BE, toUtf16Len },
    { fromUtf32, fromUtf32Len, toUtf32, toUtf32Len },
    { fromUtf32LE, fromUtf32Len, toUtf32LE, toUtf32Len },
    { fromUtf32BE, fromUtf32Len, toUtf32BE, toUtf32Len }
};

QT_END_NAMESPACE
