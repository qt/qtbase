/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QStringList>
#include <QFile>
#include <QtTest/QtTest>

#ifdef Q_OS_UNIX
#include <sys/mman.h>
#include <unistd.h>
#endif

// MAP_ANON is deprecated on Linux, and MAP_ANONYMOUS is not present on Mac
#ifndef MAP_ANONYMOUS
# define MAP_ANONYMOUS MAP_ANON
#endif

#include <private/qsimd_p.h>

#include "data.h"

class tst_QString: public QObject
{
    Q_OBJECT
public:
    tst_QString();
private slots:
    void equals() const;
    void equals_data() const;
    void equals2_data() const;
    void equals2() const;
    void ucstrncmp_data() const;
    void ucstrncmp() const;
    void fromUtf8() const;
    void fromLatin1_data() const;
    void fromLatin1() const;
    void fromLatin1Alternatives_data() const;
    void fromLatin1Alternatives() const;
    void fromUtf8Alternatives_data() const;
    void fromUtf8Alternatives() const;

    void toUpper_data();
    void toUpper();
    void toLower_data();
    void toLower();
    void toCaseFolded_data();
    void toCaseFolded();
};

void tst_QString::equals() const
{
    QFETCH(QString, a);
    QFETCH(QString, b);

    QBENCHMARK {
        a == b;
    }
}

tst_QString::tst_QString()
{
}

void tst_QString::equals_data() const
{
    static const struct {
        ushort data[80];
        int dummy;              // just to ensure 4-byte alignment
    } data = {
        {
            64, 64, 64, 64,  64, 64, 64, 64,
            64, 64, 64, 64,  64, 64, 64, 64, // 16
            64, 64, 64, 64,  64, 64, 64, 64,
            64, 64, 64, 64,  64, 64, 64, 64, // 32
            64, 64, 64, 64,  64, 64, 64, 64,
            64, 64, 64, 64,  64, 64, 64, 64, // 48
            64, 64, 64, 64,  64, 64, 64, 64,
            64, 64, 64, 64,  64, 64, 64, 64, // 64
            64, 64, 64, 64,  96, 96, 96, 96,
            64, 64, 96, 96,  96, 96, 96, 96  // 80
        }, 0
    };
    const QChar *ptr = reinterpret_cast<const QChar *>(data.data);

    QTest::addColumn<QString>("a");
    QTest::addColumn<QString>("b");
    QString base = QString::fromRawData(ptr, 64);

    QTest::newRow("different-length") << base << QString::fromRawData(ptr, 4);
    QTest::newRow("same-string") << base << base;
    QTest::newRow("same-data") << base << QString::fromRawData(ptr, 64);

    // try to avoid crossing a cache line (that is, at ptr[64])
    QTest::newRow("aligned-aligned-4n")
            << QString::fromRawData(ptr, 60) << QString::fromRawData(ptr + 2, 60);
    QTest::newRow("aligned-unaligned-4n")
            << QString::fromRawData(ptr, 60) << QString::fromRawData(ptr + 1, 60);
    QTest::newRow("unaligned-unaligned-4n")
            << QString::fromRawData(ptr + 1, 60) << QString::fromRawData(ptr + 3, 60);

    QTest::newRow("aligned-aligned-4n+1")
            << QString::fromRawData(ptr, 61) << QString::fromRawData(ptr + 2, 61);
    QTest::newRow("aligned-unaligned-4n+1")
            << QString::fromRawData(ptr, 61) << QString::fromRawData(ptr + 1, 61);
    QTest::newRow("unaligned-unaligned-4n+1")
            << QString::fromRawData(ptr + 1, 61) << QString::fromRawData(ptr + 3, 61);

    QTest::newRow("aligned-aligned-4n-1")
            << QString::fromRawData(ptr, 59) << QString::fromRawData(ptr + 2, 59);
    QTest::newRow("aligned-unaligned-4n-1")
            << QString::fromRawData(ptr, 59) << QString::fromRawData(ptr + 1, 59);
    QTest::newRow("unaligned-unaligned-4n-1")
            << QString::fromRawData(ptr + 1, 59) << QString::fromRawData(ptr + 3, 59);

    QTest::newRow("aligned-aligned-2n")
            << QString::fromRawData(ptr, 58) << QString::fromRawData(ptr + 2, 58);
    QTest::newRow("aligned-unaligned-2n")
            << QString::fromRawData(ptr, 58) << QString::fromRawData(ptr + 1, 58);
    QTest::newRow("unaligned-unaligned-2n")
            << QString::fromRawData(ptr + 1, 58) << QString::fromRawData(ptr + 3, 58);
}

static bool equals2_memcmp_call(const ushort *p1, const ushort *p2, int len)
{
    return memcmp(p1, p2, len * 2) == 0;
}

static bool equals2_bytewise(const ushort *p1, const ushort *p2, int len)
{
    if (p1 == p2 || !len)
        return true;
    uchar *b1 = (uchar *)p1;
    uchar *b2 = (uchar *)p2;
    len *= 2;
    while (len--)
        if (*b1++ != *b2++)
            return false;
    return true;
}

static bool equals2_shortwise(const ushort *p1, const ushort *p2, int len)
{
    if (p1 == p2 || !len)
        return true;
//    for (register int counter; counter < len; ++counter)
//        if (p1[counter] != p2[counter])
//            return false;
    while (len--) {
        if (p1[len] != p2[len])
            return false;
    }
    return true;
}

static bool equals2_intwise(const ushort *p1, const ushort *p2, int length)
{
    if (p1 == p2 || !length)
        return true;
    register union {
        const quint16 *w;
        const quint32 *d;
        quintptr value;
    } sa, sb;
    sa.w = p1;
    sb.w = p2;

    // check alignment
    if ((sa.value & 2) == (sb.value & 2)) {
        // both addresses have the same alignment
        if (sa.value & 2) {
            // both addresses are not aligned to 4-bytes boundaries
            // compare the first character
            if (*sa.w != *sb.w)
                return false;
            --length;
            ++sa.w;
            ++sb.w;

            // now both addresses are 4-bytes aligned
        }

        // both addresses are 4-bytes aligned
        // do a fast 32-bit comparison
        register const quint32 *e = sa.d + (length >> 1);
        for ( ; sa.d != e; ++sa.d, ++sb.d) {
            if (*sa.d != *sb.d)
                return false;
        }

        // do we have a tail?
        return (length & 1) ? *sa.w == *sb.w : true;
    } else {
        // one of the addresses isn't 4-byte aligned but the other is
        register const quint16 *e = sa.w + length;
        for ( ; sa.w != e; ++sa.w, ++sb.w) {
            if (*sa.w != *sb.w)
                return false;
        }
    }
    return true;
}

static inline bool equals2_short_tail(const ushort *p1, const ushort *p2, int len)
{
    if (len) {
        if (*p1 != *p2)
            return false;
        if (--len) {
            if (p1[1] != p2[1])
                return false;
            if (--len) {
                if (p1[2] != p2[2])
                    return false;
                if (--len) {
                    if (p1[3] != p2[3])
                        return false;
                    if (--len) {
                        if (p1[4] != p2[4])
                            return false;
                        if (--len) {
                            if (p1[5] != p2[5])
                                return false;
                            if (--len) {
                                if (p1[6] != p2[6])
                                    return false;
                                return p1[7] == p2[7];
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}

//#pragma GCC optimize("no-unroll-loops")
#ifdef __SSE2__
static bool equals2_sse2_aligned(const ushort *p1, const ushort *p2, int len)
{
    if (len >= 8) {
        qptrdiff counter = 0;
        while (len > 8) {
            __m128i q1 = _mm_load_si128((__m128i *)(p1 + counter));
            __m128i q2 = _mm_load_si128((__m128i *)(p2 + counter));
            __m128i cmp = _mm_cmpeq_epi16(q1, q2);
            if (ushort(_mm_movemask_epi8(cmp)) != ushort(0xffff))
                return false;

            len -= 8;
            counter += 8;
        }
        p1 += counter;
        p2 += counter;
    }

    return equals2_short_tail(p1, p2, len);
}

static bool equals2_sse2(const ushort *p1, const ushort *p2, int len)
{
    if (p1 == p2 || !len)
        return true;

    if (len >= 8) {
        qptrdiff counter = 0;
        while (len >= 8) {
            __m128i q1 = _mm_loadu_si128((__m128i *)(p1 + counter));
            __m128i q2 = _mm_loadu_si128((__m128i *)(p2 + counter));
            __m128i cmp = _mm_cmpeq_epi16(q1, q2);
            if (ushort(_mm_movemask_epi8(cmp)) != 0xffff)
                return false;

            len -= 8;
            counter += 8;
        }
        p1 += counter;
        p2 += counter;
    }

    return equals2_short_tail(p1, p2, len);
}

//static bool equals2_sse2(const ushort *p1, const ushort *p2, int len)
//{
//    register int val1 = quintptr(p1) & 0xf;
//    register int val2 = quintptr(p2) & 0xf;
//    if (false && val1 + val2 == 0)
//        return equals2_sse2_aligned(p1, p2, len);
//    else
//        return equals2_sse2_unaligned(p1, p2, len);
//}

static bool equals2_sse2_aligning(const ushort *p1, const ushort *p2, int len)
{
    if (len < 8)
        return equals2_short_tail(p1, p2, len);

    qptrdiff counter = 0;

    // which one is easier to align, p1 or p2 ?
    register int val1 = quintptr(p1) & 0xf;
    register int val2 = quintptr(p2) & 0xf;
    if (val1 && val2) {
#if 0
        // we'll align the one which requires the least number of steps
        if (val1 > val2) {
            qSwap(p1, p2);
            val1 = val2;
        }

        // val1 contains the number of bytes past the 16-aligned mark
        // we must read 16-val1 bytes to align
        val1 = 16 - val1;
        if (val1 & 0x2) {
            if (*p1 != *p2)
                return false;
            --len;
            ++counter;
        }
        while (val1 & 12) {
            if (*(uint*)p1 != *(uint*)p2)
                return false;
            --len;
            counter += 2;
            val1 -= 4;
        }
#else
        // we'll align the one closest to the 16-byte mark
        if (val1 > val2) {
            qSwap(p1, p2);
            val1 = val2;
        }

        // we're reading val1 bytes too many
        __m128i q2 = _mm_loadu_si128((__m128i *)(p2 - val1/2));
        __m128i cmp = _mm_cmpeq_epi16(*(__m128i *)(p1 - val1/2), q2);
        if (short(_mm_movemask_epi8(cmp)) >> val1 != short(-1))
            return false;

        counter = 8 - val1/2;
        len -= 8 - val1/2;
#endif
    } else if (!val2) {
        // p2 is already aligned
        qSwap(p1, p2);
    }

    // p1 is aligned

    while (len >= 8) {
        __m128i q1 = _mm_load_si128((__m128i *)(p1 + counter));
        __m128i q2 = _mm_loadu_si128((__m128i *)(p2 + counter));
        __m128i cmp = _mm_cmpeq_epi16(q1, q2);
        if (ushort(_mm_movemask_epi8(cmp)) != ushort(0xffff))
            return false;

        len -= 8;
        counter += 8;
    }

    // tail
    return equals2_short_tail(p1 + counter, p2 + counter, len);
}

#ifdef __SSE3__
static bool equals2_sse3(const ushort *p1, const ushort *p2, int len)
{
    if (p1 == p2 || !len)
        return true;

    if (len >= 8) {
        qptrdiff counter = 0;
        while (len >= 8) {
            __m128i q1 = _mm_lddqu_si128((__m128i *)(p1 + counter));
            __m128i q2 = _mm_lddqu_si128((__m128i *)(p2 + counter));
            __m128i cmp = _mm_cmpeq_epi16(q1, q2);
            if (ushort(_mm_movemask_epi8(cmp)) != 0xffff)
                return false;

            len -= 8;
            counter += 8;
        }
        p1 += counter;
        p2 += counter;
    }

    return equals2_short_tail(p1, p2, len);
}

#ifdef __SSSE3__
template<int N> static inline bool equals2_ssse3_alignr(__m128i *m1, __m128i *m2, int len)
{
    __m128i lower = _mm_load_si128(m1);
    while (len >= 8) {
        __m128i upper = _mm_load_si128(m1 + 1);
        __m128i correct;
        correct = _mm_alignr_epi8(upper, lower, N);

        __m128i q2 = _mm_lddqu_si128(m2);
        __m128i cmp = _mm_cmpeq_epi16(correct, q2);
        if (ushort(_mm_movemask_epi8(cmp)) != 0xffff)
            return false;

        len -= 8;
        ++m2;
        ++m1;
        lower = upper;
    }

    // tail
    return len == 0 || equals2_short_tail((const ushort *)m1 + N / 2, (const ushort*)m2, len);
}

static inline bool equals2_ssse3_aligned(__m128i *m1, __m128i *m2, int len)
{
    while (len >= 8) {
        __m128i q2 = _mm_lddqu_si128(m2);
        __m128i cmp = _mm_cmpeq_epi16(*m1, q2);
        if (ushort(_mm_movemask_epi8(cmp)) != 0xffff)
            return false;

        len -= 8;
        ++m1;
        ++m2;
    }
    return len == 0 || equals2_short_tail((const ushort *)m1, (const ushort *)m2, len);
}

static bool equals2_ssse3(const ushort *p1, const ushort *p2, int len)
{
    // p1 & 0xf can be:
    //   0,  2,  4,  6,  8, 10, 12, 14
    // If it's 0, we're aligned
    // If it's not, then we're interested in the 16 - (p1 & 0xf) bytes only

    if (len >= 8) {
        // find the last aligned position below the p1 memory
        __m128i *m1 = (__m128i *)(quintptr(p1) & ~0xf);
        __m128i *m2 = (__m128i *)p2;
        qptrdiff diff = quintptr(p1) - quintptr(m1);

        // diff contains the number of extra bytes
        if (diff == 10)
            return equals2_ssse3_alignr<10>(m1, m2, len);
        else if (diff == 2)
            return equals2_ssse3_alignr<2>(m1, m2, len);
        if (diff < 8) {
            if (diff < 4) {
                return equals2_ssse3_aligned(m1, m2, len);
            } else {
                if (diff == 4)
                    return equals2_ssse3_alignr<4>(m1, m2, len);
                else // diff == 6
                    return equals2_ssse3_alignr<6>(m1, m2, len);
            }
        } else {
            if (diff < 12) {
                return equals2_ssse3_alignr<8>(m1, m2, len);
            } else {
                if (diff == 12)
                    return equals2_ssse3_alignr<12>(m1, m2, len);
                else // diff == 14
                    return equals2_ssse3_alignr<14>(m1, m2, len);
            }
        }
    }

    // tail
    return equals2_short_tail(p1, p2, len);
}

template<int N> static inline bool equals2_ssse3_aligning_alignr(__m128i *m1, __m128i *m2, int len)
{
    __m128i lower = _mm_load_si128(m1);
    while (len >= 8) {
        __m128i upper = _mm_load_si128(m1 + 1);
        __m128i correct;
        correct = _mm_alignr_epi8(upper, lower, N);

        __m128i cmp = _mm_cmpeq_epi16(correct, *m2);
        if (ushort(_mm_movemask_epi8(cmp)) != 0xffff)
            return false;

        len -= 8;
        ++m2;
        ++m1;
        lower = upper;
    }

    // tail
    return len == 0 || equals2_short_tail((const ushort *)m1 + N / 2, (const ushort*)m2, len);
}

static bool equals2_ssse3_aligning(const ushort *p1, const ushort *p2, int len)
{
    if (len < 8)
        return equals2_short_tail(p1, p2, len);
    qptrdiff counter = 0;

    // which one is easier to align, p1 or p2 ?
    {
        register int val1 = quintptr(p1) & 0xf;
        register int val2 = quintptr(p2) & 0xf;
        if (val1 && val2) {
            // we'll align the one closest to the 16-byte mark
            if (val1 < val2) {
                qSwap(p1, p2);
                val2 = val1;
            }

            // we're reading val1 bytes too many
            __m128i q1 = _mm_lddqu_si128((__m128i *)(p1 - val2/2));
            __m128i cmp = _mm_cmpeq_epi16(q1, *(__m128i *)(p2 - val2/2));
            if (short(_mm_movemask_epi8(cmp)) >> val1 != short(-1))
                return false;

            counter = 8 - val2/2;
            len -= 8 - val2/2;
        } else if (!val1) {
            // p1 is already aligned
            qSwap(p1, p2);
        }
    }

    // p2 is aligned now
    // we want to use palignr in the mis-alignment of p1
    __m128i *m1 = (__m128i *)(quintptr(p1 + counter) & ~0xf);
    __m128i *m2 = (__m128i *)(p2 + counter);
    register int val1 = quintptr(p1 + counter) - quintptr(m1);

    // val1 contains the number of extra bytes
    if (val1 == 8)
        return equals2_ssse3_aligning_alignr<8>(m1, m2, len);
    if (val1 == 0)
        return equals2_sse2_aligned(p1 + counter, p2 + counter, len);
    if (val1 < 8) {
        if (val1 < 4) {
            return equals2_ssse3_aligning_alignr<2>(m1, m2, len);
        } else {
            if (val1 == 4)
                return equals2_ssse3_aligning_alignr<4>(m1, m2, len);
            else // diff == 6
                return equals2_ssse3_aligning_alignr<6>(m1, m2, len);
        }
    } else {
        if (val1 < 12) {
            return equals2_ssse3_aligning_alignr<10>(m1, m2, len);
        } else {
            if (val1 == 12)
                return equals2_ssse3_aligning_alignr<12>(m1, m2, len);
            else // diff == 14
                return equals2_ssse3_aligning_alignr<14>(m1, m2, len);
        }
    }
}

#ifdef __SSE4_1__
static bool equals2_sse4(const ushort *p1, const ushort *p2, int len)
{
    // We use the pcmpestrm instruction searching for differences (negative polarity)
    // it will reset CF if it's all equal
    // it will reset OF if the first char is equal
    // it will set ZF & SF if the length is less than 8 (which means we've done the last operation)
    // the three possible conditions are:
    //  difference found:         CF = 1
    //  all equal, not finished:  CF = ZF = SF = 0
    //  all equal, finished:      CF = 0, ZF = SF = 1
    // We use the JA instruction that jumps if ZF = 0 and CF = 0
    if (p1 == p2 || !len)
        return true;

    // This function may read some bytes past the end of p1 or p2
    // It is safe to do that, as long as those extra bytes (beyond p1+len and p2+len)
    // are on the same page as the last valid byte.
    // If len is a multiple of 8, we'll never load invalid bytes.
    if (len & 7) {
        // The last load would load (len & ~7) valid bytes and (8 - (len & ~7)) invalid bytes.
        // So we can't do the last load if any of those bytes is in a different
        // page. That is, if:
        //    pX + len      is on a different page from     pX + (len & ~7) + 8
        //
        // that is, if second-to-last load ended up less than 16 bytes from the page end:
        //    pX + (len & ~7)  is the last ushort read in the second-to-last load
        if (len < 8)
            return equals2_short_tail(p1, p2, len);
        if ((quintptr(p1 + (len & ~7)) & 0xfff) > 0xff0 ||
                (quintptr(p2 + (len & ~7)) & 0xfff) > 0xff0) {

            // yes, so we mustn't do the final 128-bit load
            bool result;
            asm (
            "sub        %[p1], %[p2]\n\t"
            "sub        $16, %[p1]\n\t"
            "add        $8, %[len]\n\t"

            // main loop:
            "0:\n\t"
            "add        $16, %[p1]\n\t"
            "sub        $8, %[len]\n\t"
            "jz         1f\n\t"
            "lddqu      (%[p1]), %%xmm0\n\t"
            "mov        %[len], %%edx\n\t"
            "pcmpestri  %[mode], (%[p2],%[p1]), %%xmm0\n\t"

            "jna        1f\n\t"
            "add        $16, %[p1]\n\t"
            "sub        $8, %[len]\n\t"
            "jz         1f\n\t"
            "lddqu      (%[p1]), %%xmm0\n\t"
            "mov        %[len], %%edx\n\t"
            "pcmpestri  %[mode], (%[p2],%[p1]), %%xmm0\n\t"

            "ja         0b\n\t"
            "1:\n\t"
            "setnc      %[result]\n\t"
            : [result] "=a" (result),
              [p1] "+r" (p1),
              [p2] "+r" (p2)
            : [len] "0" (len & ~7),
              [mode] "i" (_SIDD_UWORD_OPS | _SIDD_CMP_EQUAL_EACH | _SIDD_NEGATIVE_POLARITY)
            : "%edx", "%ecx", "%xmm0"
            );
            return result && equals2_short_tail(p1, (const ushort *)(quintptr(p1) + quintptr(p2)), len & 7);
        }
    }

//    const qptrdiff disp = p2 - p1;
//    p1 -= 8;
//    len += 8;
//    while (true) {
//        enum { Mode = _SIDD_UWORD_OPS | _SIDD_CMP_EQUAL_EACH | _SIDD_NEGATIVE_POLARITY };

//        p1 += 8;
//        len -= 8;
//        if (!len)
//            return true;

//        __m128i q1 = _mm_lddqu_si128((__m128i *)(p1 + disp));
//        __m128i *m2 = (__m128i *)p1;

//        bool cmp_a = _mm_cmpestra(q1, len, *m2, len, Mode);
//        if (cmp_a)
//            continue;
//        return !_mm_cmpestrc(q1, len, *m2, len, Mode);
//    }
//    return true;
    bool result;
    asm (
        "sub        %[p1], %[p2]\n\t"
        "sub        $16, %[p1]\n\t"
        "add        $8, %[len]\n\t"

    "0:\n\t"
        "add        $16, %[p1]\n\t"
        "sub        $8, %[len]\n\t"
        "jz         1f\n\t"
        "lddqu      (%[p2],%[p1]), %%xmm0\n\t"
        "mov        %[len], %%edx\n\t"
        "pcmpestri  %[mode], (%[p1]), %%xmm0\n\t"

        "jna        1f\n\t"
        "add        $16, %[p1]\n\t"
        "sub        $8, %[len]\n\t"
        "jz         1f\n\t"
        "lddqu      (%[p2],%[p1]), %%xmm0\n\t"
        "mov        %[len], %%edx\n\t"
        "pcmpestri  %[mode], (%[p1]), %%xmm0\n\t"

        "ja         0b\n\t"

    "1:\n\t"
        "setnc      %[result]\n\t"
        : [result] "=a" (result)
        : [len] "0" (len),
          [p1] "r" (p1),
          [p2] "r" (p2),
          [mode] "i" (_SIDD_UWORD_OPS | _SIDD_CMP_EQUAL_EACH | _SIDD_NEGATIVE_POLARITY)
        : "%edx", "%ecx", "%xmm0"
    );
    return result;
}

#endif
#endif
#endif
#endif

typedef bool (* FuncPtr)(const ushort *, const ushort *, int);
static const FuncPtr func[] = {
    equals2_memcmp_call, // 0
    equals2_bytewise, // 1
    equals2_shortwise, // 1
    equals2_intwise, // 3
#ifdef __SSE2__
    equals2_sse2, // 4
    equals2_sse2_aligning, // 5
#ifdef __SSE3__
    equals2_sse3, // 6
#ifdef __SSSE3__
    equals2_ssse3, // 7
    equals2_ssse3, // 8
#ifdef __SSE4_1__
    equals2_sse4, // 9
#endif
#endif
#endif
#endif
    0
};
static const int functionCount = sizeof(func)/sizeof(func[0]) - 1;

void tst_QString::equals2_data() const
{
    QTest::addColumn<int>("algorithm");
    QTest::newRow("selftest") << -1;
    QTest::newRow("memcmp_call") << 0;
    QTest::newRow("bytewise") << 1;
    QTest::newRow("shortwise") << 2;
    QTest::newRow("intwise") << 3;
#ifdef __SSE2__
    QTest::newRow("sse2") << 4;
    QTest::newRow("sse2_aligning") << 5;
#ifdef __SSE3__
    QTest::newRow("sse3") << 6;
#ifdef __SSSE3__
    QTest::newRow("ssse3") << 7;
    QTest::newRow("ssse3_aligning") << 8;
#ifdef __SSE4_1__
    QTest::newRow("sse4.2") << 9;
#endif
#endif
#endif
#endif
}

static void __attribute__((noinline)) equals2_selftest()
{
#ifdef Q_OS_UNIX
    const long pagesize = sysconf(_SC_PAGESIZE);
    void *page1, *page3;
    ushort *page2;
    page1 = mmap(0, pagesize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    page2 = (ushort *)mmap(0, pagesize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    page3 = mmap(0, pagesize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    Q_ASSERT(quintptr(page2) == quintptr(page1) + pagesize || quintptr(page2) == quintptr(page1) - pagesize);
    Q_ASSERT(quintptr(page3) == quintptr(page2) + pagesize || quintptr(page3) == quintptr(page2) - pagesize);
    munmap(page1, pagesize);
    munmap(page3, pagesize);

    // populate our page
    for (uint i = 0; i < pagesize / sizeof(long long); ++i)
        ((long long *)page2)[i] = Q_INT64_C(0x0041004100410041);

    // the following should crash:
    //page2[-1] = 0xdead;
    //page2[pagesize / sizeof(ushort) + 1] = 0xbeef;

    static const ushort needle[] = {
        0x41, 0x41, 0x41, 0x41,   0x41, 0x41, 0x41, 0x41,
        0x41, 0x41, 0x41, 0x41,   0x41, 0x41, 0x41, 0x41,
        0x41
    };

    for (int algo = 0; algo < functionCount; ++algo) {
        // boundary condition test:
        for (int i = 0; i < 8; ++i) {
            (func[algo])(page2 + i, needle, sizeof needle / 2);
            (func[algo])(page2 - i - 1 - sizeof(needle)/2 + pagesize/2, needle, sizeof needle/2);
        }
    }

    munmap(page2, pagesize);
#endif

    for (int algo = 0; algo < functionCount; ++algo) {
        for (int i = 0; i < stringCollectionCount; ++i) {
            const ushort *p1 = stringCollectionData + stringCollection[i].offset1;
            const ushort *p2 = stringCollectionData + stringCollection[i].offset2;
            bool expected = memcmp(p1, p2, stringCollection[i].len * 2) == 0;

            bool result = (func[algo])(p1, p2, stringCollection[i].len);
            if (expected != result)
                qWarning().nospace()
                        << "algo=" << algo
                        << " i=" << i
                        << " failed (" << result << "!=" << expected
                        << "); strings were "
                        << QByteArray((char*)p1, stringCollection[i].len).toHex()
                        << " and "
                        << QByteArray((char*)p2, stringCollection[i].len).toHex();
        }
    }
}

void tst_QString::equals2() const
{
    QFETCH(int, algorithm);
    if (algorithm == -1) {
        equals2_selftest();
        return;
    }

    QBENCHMARK {
        for (int i = 0; i < stringCollectionCount; ++i) {
            const ushort *p1 = stringCollectionData + stringCollection[i].offset1;
            const ushort *p2 = stringCollectionData + stringCollection[i].offset2;
            bool result = (func[algorithm])(p1, p2, stringCollection[i].len);
            Q_UNUSED(result);
        }
    }
}

static int ucstrncmp_shortwise(const ushort *a, const ushort *b, int l)
{
    while (l-- && *a == *b)
        a++,b++;
    if (l==-1)
        return 0;
    return *a - *b;
}

static int ucstrncmp_intwise(const ushort *a, const ushort *b, int len)
{
    // do both strings have the same alignment?
    if ((quintptr(a) & 2) == (quintptr(b) & 2)) {
        // are we aligned to 4 bytes?
        if (quintptr(a) & 2) {
            if (*a != *b)
                return *a - *b;
            ++a;
            ++b;
            --len;
        }

        const uint *p1 = (const uint *)a;
        const uint *p2 = (const uint *)b;
        quintptr counter = 0;
        for ( ; len > 1 ; len -= 2, ++counter) {
            if (p1[counter] != p2[counter]) {
                // which ushort isn't equal?
                int diff = a[2*counter] - b[2*counter];
                return diff ? diff : a[2*counter + 1] - b[2*counter + 1];
            }
        }

        return len ? a[2*counter] - b[2*counter] : 0;
    } else {
        while (len-- && *a == *b)
            a++,b++;
        if (len==-1)
            return 0;
        return *a - *b;
    }
}

#ifdef __SSE2__
static inline int ucstrncmp_short_tail(const ushort *p1, const ushort *p2, int len)
{
    if (len) {
        if (*p1 != *p2)
            return *p1 - *p2;
        if (--len) {
            if (p1[1] != p2[1])
                return p1[1] - p2[1];
            if (--len) {
                if (p1[2] != p2[2])
                    return p1[2] - p2[2];
                if (--len) {
                    if (p1[3] != p2[3])
                        return p1[3] - p2[3];
                    if (--len) {
                        if (p1[4] != p2[4])
                            return p1[4] - p2[4];
                        if (--len) {
                            if (p1[5] != p2[5])
                                return p1[5] - p2[5];
                            if (--len) {
                                if (p1[6] != p2[6])
                                    return p1[6] - p2[6];
                                return p1[7] - p2[7];
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}

static inline int bsf_nonzero(register int val)
{
    int result;
# ifdef Q_CC_GNU
    // returns the first non-zero bit on a non-zero reg
    asm ("bsf   %1, %0" : "=r" (result) : "r" (val));
    return result;
# elif defined(Q_CC_MSVC)
    _BitScanForward(&result, val);
    return result;
# endif
}

static int ucstrncmp_sse2(const ushort *a, const ushort *b, int len)
{
    qptrdiff counter = 0;
    while (len >= 8) {
        __m128i m1 = _mm_loadu_si128((__m128i *)(a + counter));
        __m128i m2 = _mm_loadu_si128((__m128i *)(b + counter));
        __m128i cmp = _mm_cmpeq_epi16(m1, m2);
        ushort mask = ~uint(_mm_movemask_epi8(cmp));
        if (mask) {
            // which ushort isn't equal?
            counter += bsf_nonzero(mask)/2;
            return a[counter] - b[counter];
        }

        counter += 8;
        len -= 8;
    }
    return ucstrncmp_short_tail(a + counter, b + counter, len);
}

static int ucstrncmp_sse2_aligning(const ushort *a, const ushort *b, int len)
{
    if (len >= 8) {
        __m128i m1 = _mm_loadu_si128((__m128i *)a);
        __m128i m2 = _mm_loadu_si128((__m128i *)b);
        __m128i cmp = _mm_cmpeq_epi16(m1, m2);
        ushort mask = ~uint(_mm_movemask_epi8(cmp));
        if (mask) {
            // which ushort isn't equal?
            int counter = bsf_nonzero(mask)/2;
            return a[counter] - b[counter];
        }


        // now align to do 16-byte loads
        int diff = 8 - (quintptr(a) & 0xf)/2;
        len -= diff;
        a += diff;
        b += diff;
    }

    qptrdiff counter = 0;
    while (len >= 8) {
        __m128i m1 = _mm_load_si128((__m128i *)(a + counter));
        __m128i m2 = _mm_loadu_si128((__m128i *)(b + counter));
        __m128i cmp = _mm_cmpeq_epi16(m1, m2);
        ushort mask = ~uint(_mm_movemask_epi8(cmp));
        if (mask) {
            // which ushort isn't equal?
            counter += bsf_nonzero(mask)/2;
            return a[counter] - b[counter];
        }

        counter += 8;
        len -= 8;
    }
    return ucstrncmp_short_tail(a + counter, b + counter, len);
}

static inline int ucstrncmp_sse2_aligned(const ushort *a, const ushort *b, int len)
{
    quintptr counter = 0;
    while (len >= 8) {
        __m128i m1 = _mm_load_si128((__m128i *)(a + counter));
        __m128i m2 = _mm_load_si128((__m128i *)(b + counter));
        __m128i cmp = _mm_cmpeq_epi16(m1, m2);
        ushort mask = ~uint(_mm_movemask_epi8(cmp));
        if (mask) {
            // which ushort isn't equal?
            counter += bsf_nonzero(mask)/2;
            return a[counter] - b[counter];
        }

        counter += 8;
        len -= 8;
    }
    return ucstrncmp_short_tail(a + counter, b + counter, len);
}

#ifdef __SSSE3__
static inline int ucstrncmp_ssse3_alignr_aligned(const ushort *a, const ushort *b, int len)
{
    quintptr counter = 0;
    while (len >= 8) {
        __m128i m1 = _mm_load_si128((__m128i *)(a + counter));
        __m128i m2 = _mm_lddqu_si128((__m128i *)(b + counter));
        __m128i cmp = _mm_cmpeq_epi16(m1, m2);
        ushort mask = ~uint(_mm_movemask_epi8(cmp));
        if (mask) {
            // which ushort isn't equal?
            counter += bsf_nonzero(mask)/2;
            return a[counter] - b[counter];
        }

        counter += 8;
        len -= 8;
    }
    return ucstrncmp_short_tail(a + counter, b + counter, len);
}


typedef __m128i (* MMLoadFunction)(const __m128i *);
template<int N, MMLoadFunction LoadFunction>
static inline int ucstrncmp_ssse3_alignr(const ushort *a, const ushort *b, int len)
{
    qptrdiff counter = 0;
    __m128i lower, upper;
    upper = _mm_load_si128((__m128i *)a);

    do {
        lower = upper;
        upper = _mm_load_si128((__m128i *)(a + counter) + 1);
        __m128i merged = _mm_alignr_epi8(upper, lower, N);

        __m128i m2 = LoadFunction((__m128i *)(b + counter));
        __m128i cmp = _mm_cmpeq_epi16(merged, m2);
        ushort mask = ~uint(_mm_movemask_epi8(cmp));
        if (mask) {
            // which ushort isn't equal?
            counter += bsf_nonzero(mask)/2;
            return a[counter + N/2] - b[counter];
        }

        counter += 8;
        len -= 8;
    } while (len >= 8);

    return ucstrncmp_short_tail(a + counter + N/2, b + counter, len);
}

// external linkage to be used as the MMLoadFunction template argument for ucstrncmp_ssse3_alignr
__m128i EXT_mm_lddqu_si128(const __m128i *p)
{ return _mm_lddqu_si128(p); }
__m128i EXT_mm_load_si128(__m128i const *p)
{ return _mm_load_si128(p); }

static int ucstrncmp_ssse3(const ushort *a, const ushort *b, int len)
{
    if (len >= 8) {
        int val = quintptr(a) & 0xf;
        a -= val/2;

        if (val == 10)
            return ucstrncmp_ssse3_alignr<10, EXT_mm_lddqu_si128>(a, b, len);
        else if (val == 2)
            return ucstrncmp_ssse3_alignr<2, EXT_mm_lddqu_si128>(a, b, len);
        if (val < 8) {
            if (val < 4)
                return ucstrncmp_ssse3_alignr_aligned(a, b, len);
            else if (val == 4)
                    return ucstrncmp_ssse3_alignr<4, EXT_mm_lddqu_si128>(a, b, len);
            else
                    return ucstrncmp_ssse3_alignr<6, EXT_mm_lddqu_si128>(a, b, len);
        } else {
            if (val < 12)
                return ucstrncmp_ssse3_alignr<8, EXT_mm_lddqu_si128>(a, b, len);
            else if (val == 12)
                return ucstrncmp_ssse3_alignr<12, EXT_mm_lddqu_si128>(a, b, len);
            else
                return ucstrncmp_ssse3_alignr<14, EXT_mm_lddqu_si128>(a, b, len);
        }
    }
    return ucstrncmp_short_tail(a, b, len);
}

static int ucstrncmp_ssse3_aligning(const ushort *a, const ushort *b, int len)
{
    if (len >= 8) {
        __m128i m1 = _mm_loadu_si128((__m128i *)a);
        __m128i m2 = _mm_loadu_si128((__m128i *)b);
        __m128i cmp = _mm_cmpeq_epi16(m1, m2);
        ushort mask = ~uint(_mm_movemask_epi8(cmp));
        if (mask) {
            // which ushort isn't equal?
            int counter = bsf_nonzero(mask)/2;
            return a[counter] - b[counter];
        }


        // now 'b' align to do 16-byte loads
        int diff = 8 - (quintptr(b) & 0xf)/2;
        len -= diff;
        a += diff;
        b += diff;
    }

    if (len < 8)
        return ucstrncmp_short_tail(a, b, len);

    // 'b' is aligned
    int val = quintptr(a) & 0xf;
    a -= val/2;

    if (val == 8)
        return ucstrncmp_ssse3_alignr<8, EXT_mm_load_si128>(a, b, len);
    else if (val == 0)
        return ucstrncmp_sse2_aligned(a, b, len);
    if (val < 8) {
        if (val < 4)
            return ucstrncmp_ssse3_alignr<2, EXT_mm_load_si128>(a, b, len);
        else if (val == 4)
            return ucstrncmp_ssse3_alignr<4, EXT_mm_load_si128>(a, b, len);
        else
            return ucstrncmp_ssse3_alignr<6, EXT_mm_load_si128>(a, b, len);
    } else {
        if (val < 12)
            return ucstrncmp_ssse3_alignr<10, EXT_mm_load_si128>(a, b, len);
        else if (val == 12)
            return ucstrncmp_ssse3_alignr<12, EXT_mm_load_si128>(a, b, len);
        else
            return ucstrncmp_ssse3_alignr<14, EXT_mm_load_si128>(a, b, len);
    }
}

static inline
int ucstrncmp_ssse3_aligning2_aligned(const ushort *a, const ushort *b, int len, int garbage)
{
    // len >= 8
    __m128i m1 = _mm_load_si128((const __m128i *)a);
    __m128i m2 = _mm_load_si128((const __m128i *)b);
    __m128i cmp = _mm_cmpeq_epi16(m1, m2);
    int mask = short(_mm_movemask_epi8(cmp)); // force sign extension
    mask >>= garbage;
    if (~mask) {
        // which ushort isn't equal?
        uint counter = (garbage + bsf_nonzero(~mask));
        return a[counter/2] - b[counter/2];
    }

    // the first 16-garbage bytes (8-garbage/2 ushorts) were equal
    len -= 8 - garbage/2;
    return ucstrncmp_sse2_aligned(a + 8, b + 8, len);
}

template<int N> static inline
int ucstrncmp_ssse3_aligning2_alignr(const ushort *a, const ushort *b, int len, int garbage)
{
    // len >= 8
    __m128i lower, upper, merged;
    lower = _mm_load_si128((const __m128i*)a);
    upper = _mm_load_si128((const __m128i*)(a + 8));
    merged = _mm_alignr_epi8(upper, lower, N);

    __m128i m2 = _mm_load_si128((const __m128i*)b);
    __m128i cmp = _mm_cmpeq_epi16(merged, m2);
    int mask = short(_mm_movemask_epi8(cmp)); // force sign extension
    mask >>= garbage;
    if (~mask) {
        // which ushort isn't equal?
        uint counter = (garbage + bsf_nonzero(~mask));
        return a[counter/2 + N/2] - b[counter/2];
    }

    // the first 16-garbage bytes (8-garbage/2 ushorts) were equal
    quintptr counter = 8;
    len -= 8 - garbage/2;
    while (len >= 8) {
        lower = upper;
        upper = _mm_load_si128((__m128i *)(a + counter) + 1);
        merged = _mm_alignr_epi8(upper, lower, N);

        m2 = _mm_load_si128((__m128i *)(b + counter));
        cmp = _mm_cmpeq_epi16(merged, m2);
        ushort mask = ~uint(_mm_movemask_epi8(cmp));
        if (mask) {
            // which ushort isn't equal?
            counter += bsf_nonzero(mask)/2;
            return a[counter + N/2] - b[counter];
        }

        counter += 8;
        len -= 8;
    }

    return ucstrncmp_short_tail(a + counter + N/2, b + counter, len);
}

static inline int conditional_invert(int result, bool invert)
{
    if (invert)
        return -result;
    return result;
}

static int ucstrncmp_ssse3_aligning2(const ushort *a, const ushort *b, int len)
{
    // Different strategy from above: instead of doing two unaligned loads
    // when trying to align, we'll only do aligned loads and round down the
    // addresses of a and b. This means the first load will contain garbage
    // in the beginning of the string, which we'll shift out of the way
    // (after _mm_movemask_epi8)

    if (len < 8)
        return ucstrncmp_intwise(a, b, len);

    // both a and b are misaligned
    // we'll call the alignr function with the alignment *difference* between the two
    int offset = (quintptr(a) & 0xf) - (quintptr(b) & 0xf);
    if (offset >= 0) {
        // from this point on, b has the shortest alignment
        // and align(a) = align(b) + offset
        // round down the alignment so align(b) == align(a) == 0
        int garbage = (quintptr(b) & 0xf);
        a = (const ushort*)(quintptr(a) & ~0xf);
        b = (const ushort*)(quintptr(b) & ~0xf);

        // now the first load of b will load 'garbage' extra bytes
        // and the first load of a will load 'garbage + offset' extra bytes
        if (offset == 8)
            return ucstrncmp_ssse3_aligning2_alignr<8>(a, b, len, garbage);
        if (offset == 0)
            return ucstrncmp_ssse3_aligning2_aligned(a, b, len, garbage);
        if (offset < 8) {
            if (offset < 4)
                return ucstrncmp_ssse3_aligning2_alignr<2>(a, b, len, garbage);
            else if (offset == 4)
                return ucstrncmp_ssse3_aligning2_alignr<4>(a, b, len, garbage);
            else
                return ucstrncmp_ssse3_aligning2_alignr<6>(a, b, len, garbage);
        } else {
            if (offset < 12)
                return ucstrncmp_ssse3_aligning2_alignr<10>(a, b, len, garbage);
            else if (offset == 12)
                return ucstrncmp_ssse3_aligning2_alignr<12>(a, b, len, garbage);
            else
                return ucstrncmp_ssse3_aligning2_alignr<14>(a, b, len, garbage);
        }
    } else {
        // same as above but inverted
        int garbage = (quintptr(a) & 0xf);
        a = (const ushort*)(quintptr(a) & ~0xf);
        b = (const ushort*)(quintptr(b) & ~0xf);

        offset = -offset;
        if (offset == 8)
            return -ucstrncmp_ssse3_aligning2_alignr<8>(b, a, len, garbage);
        if (offset < 8) {
            if (offset < 4)
                return -ucstrncmp_ssse3_aligning2_alignr<2>(b, a, len, garbage);
            else if (offset == 4)
                return -ucstrncmp_ssse3_aligning2_alignr<4>(b, a, len, garbage);
            else
                return -ucstrncmp_ssse3_aligning2_alignr<6>(b, a, len, garbage);
        } else {
            if (offset < 12)
                return -ucstrncmp_ssse3_aligning2_alignr<10>(b, a, len, garbage);
            else if (offset == 12)
                return -ucstrncmp_ssse3_aligning2_alignr<12>(b, a, len, garbage);
            else
                return -ucstrncmp_ssse3_aligning2_alignr<14>(b, a, len, garbage);
        }
    }
}

#endif
#endif

typedef int (* UcstrncmpFunction)(const ushort *, const ushort *, int);
Q_DECLARE_METATYPE(UcstrncmpFunction)

void tst_QString::ucstrncmp_data() const
{
    QTest::addColumn<UcstrncmpFunction>("function");
    QTest::newRow("selftest") << UcstrncmpFunction(0);
    QTest::newRow("shortwise") << &ucstrncmp_shortwise;
    QTest::newRow("intwise") << &ucstrncmp_intwise;
#ifdef __SSE2__
    QTest::newRow("sse2") << &ucstrncmp_sse2;
    QTest::newRow("sse2_aligning") << &ucstrncmp_sse2_aligning;
#ifdef __SSSE3__
    QTest::newRow("ssse3") << &ucstrncmp_ssse3;
    QTest::newRow("ssse3_aligning") << &ucstrncmp_ssse3_aligning;
    QTest::newRow("ssse3_aligning2") << &ucstrncmp_ssse3_aligning2;
#endif
#endif
}

void tst_QString::ucstrncmp() const
{
    QFETCH(UcstrncmpFunction, function);
    if (!function) {
        static const UcstrncmpFunction func[] = {
            &ucstrncmp_shortwise,
            &ucstrncmp_intwise,
#ifdef __SSE2__
            &ucstrncmp_sse2,
            &ucstrncmp_sse2_aligning,
#ifdef __SSSE3__
            &ucstrncmp_ssse3,
            &ucstrncmp_ssse3_aligning,
            &ucstrncmp_ssse3_aligning2
#endif
#endif
        };
        static const int functionCount = sizeof func / sizeof func[0];

#ifdef Q_OS_UNIX
        const long pagesize = sysconf(_SC_PAGESIZE);
        void *page1, *page3;
        ushort *page2;
        page1 = mmap(0, pagesize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        page2 = (ushort *)mmap(0, pagesize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        page3 = mmap(0, pagesize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        Q_ASSERT(quintptr(page2) == quintptr(page1) + pagesize || quintptr(page2) == quintptr(page1) - pagesize);
        Q_ASSERT(quintptr(page3) == quintptr(page2) + pagesize || quintptr(page3) == quintptr(page2) - pagesize);
        munmap(page1, pagesize);
        munmap(page3, pagesize);

        // populate our page
        for (uint i = 0; i < pagesize / sizeof(long long); ++i)
            ((long long *)page2)[i] = Q_INT64_C(0x0041004100410041);

        // the following should crash:
        //page2[-1] = 0xdead;
        //page2[pagesize / sizeof(ushort) + 1] = 0xbeef;

        static const ushort needle[] = {
            0x41, 0x41, 0x41, 0x41,   0x41, 0x41, 0x41, 0x41,
            0x41, 0x41, 0x41, 0x41,   0x41, 0x41, 0x41, 0x41,
            0x41
        };

        for (int algo = 0; algo < functionCount; ++algo) {
            // boundary condition test:
            for (int i = 0; i < 8; ++i) {
                (func[algo])(page2 + i, needle, sizeof needle / 2);
                (func[algo])(page2 - i - 1 - sizeof(needle)/2 + pagesize/2, needle, sizeof needle/2);
            }
        }

        munmap(page2, pagesize);
#endif

        for (int algo = 0; algo < functionCount; ++algo) {
            for (int i = 0; i < stringCollectionCount; ++i) {
                const ushort *p1 = stringCollectionData + stringCollection[i].offset1;
                const ushort *p2 = stringCollectionData + stringCollection[i].offset2;
                int expected = ucstrncmp_shortwise(p1, p2, stringCollection[i].len);
                expected = qBound(-1, expected, 1);

                int result = (func[algo])(p1, p2, stringCollection[i].len);
                result = qBound(-1, result, 1);
                if (expected != result)
                    qWarning().nospace()
                        << "algo=" << algo
                        << " i=" << i
                        << " failed (" << result << "!=" << expected
                        << "); strings were "
                        << QByteArray((char*)p1, stringCollection[i].len).toHex()
                        << " and "
                        << QByteArray((char*)p2, stringCollection[i].len).toHex();
            }
        }
        return;
    }

    QBENCHMARK {
        for (int i = 0; i < stringCollectionCount; ++i) {
            const ushort *p1 = stringCollectionData + stringCollection[i].offset1;
            const ushort *p2 = stringCollectionData + stringCollection[i].offset2;
            (function)(p1, p2, stringCollection[i].len);
        }
    }
}

void tst_QString::fromUtf8() const
{
    QString testFile = QFINDTESTDATA("utf-8.txt");
    QVERIFY2(!testFile.isEmpty(), "cannot find test file utf-8.txt!");
    QFile file(testFile);
    if (!file.open(QFile::ReadOnly)) {
        qFatal("Cannot open input file");
        return;
    }
    QByteArray data = file.readAll();
    const char *d = data.constData();
    int size = data.size();

    QBENCHMARK {
        QString::fromUtf8(d, size);
    }
}

void tst_QString::fromLatin1_data() const
{
    QTest::addColumn<QByteArray>("latin1");

    // make all the strings have the same length
    QTest::newRow("ascii-only") << QByteArray("HelloWorld");
    QTest::newRow("ascii+control") << QByteArray("Hello\1\r\n\x7f\t");
    QTest::newRow("ascii+nul") << QByteArray("a\0zbc\0defg", 10);
    QTest::newRow("non-ascii") << QByteArray("\x80\xc0\xff\x81\xc1\xfe\x90\xd0\xef\xa0");
}

void tst_QString::fromLatin1() const
{
    QFETCH(QByteArray, latin1);

    while (latin1.length() < 128) {
        latin1 += latin1;
    }

    QByteArray copy1 = latin1, copy2 = latin1, copy3 = latin1;
    copy1.chop(1);
    copy2.detach();
    copy3 += latin1; // longer length
    copy2.clear();

    QBENCHMARK {
        QString s1 = QString::fromLatin1(latin1);
        QString s2 = QString::fromLatin1(latin1);
        QString s3 = QString::fromLatin1(copy1);
        QString s4 = QString::fromLatin1(copy3);
        s3 = QString::fromLatin1(copy3);
    }
}

typedef void (* FromLatin1Function)(ushort *, const char *, int);
Q_DECLARE_METATYPE(FromLatin1Function)

void fromLatin1_regular(ushort *dst, const char *str, int size)
{
    // from qstring.cpp:
    while (size--)
        *dst++ = (uchar)*str++;
}

#ifdef __SSE2__
void fromLatin1_sse2_qt47(ushort *dst, const char *str, int size)
{
    if (size >= 16) {
        int chunkCount = size >> 4; // divided by 16
        const __m128i nullMask = _mm_set1_epi32(0);
        for (int i = 0; i < chunkCount; ++i) {
            const __m128i chunk = _mm_loadu_si128((__m128i*)str); // load
            str += 16;

            // unpack the first 8 bytes, padding with zeros
            const __m128i firstHalf = _mm_unpacklo_epi8(chunk, nullMask);
            _mm_storeu_si128((__m128i*)dst, firstHalf); // store
            dst += 8;

            // unpack the last 8 bytes, padding with zeros
            const __m128i secondHalf = _mm_unpackhi_epi8 (chunk, nullMask);
            _mm_storeu_si128((__m128i*)dst, secondHalf); // store
            dst += 8;
        }
        size = size % 16;
    }
    while (size--)
        *dst++ = (uchar)*str++;
}

static inline void fromLatin1_epilog(ushort *dst, const char *str, int size)
{
    if (!size) return;
    dst[0] = (uchar)str[0];
    if (!--size) return;
    dst[1] = (uchar)str[1];
    if (!--size) return;
    dst[2] = (uchar)str[2];
    if (!--size) return;
    dst[3] = (uchar)str[3];
    if (!--size) return;
    dst[4] = (uchar)str[4];
    if (!--size) return;
    dst[5] = (uchar)str[5];
    if (!--size) return;
    dst[6] = (uchar)str[6];
    if (!--size) return;
    dst[7] = (uchar)str[7];
    if (!--size) return;
    dst[8] = (uchar)str[8];
    if (!--size) return;
    dst[9] = (uchar)str[9];
    if (!--size) return;
    dst[10] = (uchar)str[10];
    if (!--size) return;
    dst[11] = (uchar)str[11];
    if (!--size) return;
    dst[12] = (uchar)str[12];
    if (!--size) return;
    dst[13] = (uchar)str[13];
    if (!--size) return;
    dst[14] = (uchar)str[14];
    if (!--size) return;
    dst[15] = (uchar)str[15];
}

void fromLatin1_sse2_improved(ushort *dst, const char *str, int size)
{
    const __m128i nullMask = _mm_set1_epi32(0);
    qptrdiff counter = 0;
    size -= 16;
    while (size >= counter) {
        const __m128i chunk = _mm_loadu_si128((__m128i*)(str + counter)); // load

        // unpack the first 8 bytes, padding with zeros
        const __m128i firstHalf = _mm_unpacklo_epi8(chunk, nullMask);
        _mm_storeu_si128((__m128i*)(dst + counter), firstHalf); // store

        // unpack the last 8 bytes, padding with zeros
        const __m128i secondHalf = _mm_unpackhi_epi8 (chunk, nullMask);
        _mm_storeu_si128((__m128i*)(dst + counter + 8), secondHalf); // store

        counter += 16;
    }
    size += 16;
    fromLatin1_epilog(dst + counter, str + counter, size - counter);
}

void fromLatin1_sse2_improved2(ushort *dst, const char *str, int size)
{
    const __m128i nullMask = _mm_set1_epi32(0);
    qptrdiff counter = 0;
    size -= 32;
    while (size >= counter) {
        const __m128i chunk1 = _mm_loadu_si128((__m128i*)(str + counter)); // load
        const __m128i chunk2 = _mm_loadu_si128((__m128i*)(str + counter + 16)); // load

        // unpack the first 8 bytes, padding with zeros
        const __m128i firstHalf1 = _mm_unpacklo_epi8(chunk1, nullMask);
        _mm_storeu_si128((__m128i*)(dst + counter), firstHalf1); // store

        // unpack the last 8 bytes, padding with zeros
        const __m128i secondHalf1 = _mm_unpackhi_epi8(chunk1, nullMask);
        _mm_storeu_si128((__m128i*)(dst + counter + 8), secondHalf1); // store

        // unpack the first 8 bytes, padding with zeros
        const __m128i firstHalf2 = _mm_unpacklo_epi8(chunk2, nullMask);
        _mm_storeu_si128((__m128i*)(dst + counter + 16), firstHalf2); // store

        // unpack the last 8 bytes, padding with zeros
        const __m128i secondHalf2 = _mm_unpackhi_epi8(chunk2, nullMask);
        _mm_storeu_si128((__m128i*)(dst + counter + 24), secondHalf2); // store

        counter += 32;
    }
    size += 16;
    if (size >= counter) {
        const __m128i chunk = _mm_loadu_si128((__m128i*)(str + counter)); // load

        // unpack the first 8 bytes, padding with zeros
        const __m128i firstHalf = _mm_unpacklo_epi8(chunk, nullMask);
        _mm_storeu_si128((__m128i*)(dst + counter), firstHalf); // store

        // unpack the last 8 bytes, padding with zeros
        const __m128i secondHalf = _mm_unpackhi_epi8 (chunk, nullMask);
        _mm_storeu_si128((__m128i*)(dst + counter + 8), secondHalf); // store

        counter += 16;
    }
    size += 16;
    fromLatin1_epilog(dst + counter, str + counter, size - counter);
}

void fromLatin1_prolog_unrolled(ushort *dst, const char *str, int size)
{
    // QString's data pointer is most often ending in 0x2 or 0xa
    // that means the two most common values for size are (8-1)=7 and (8-5)=3
    if (size == 7)
        goto copy_7;
    if (size == 3)
        goto copy_3;

    if (size == 6)
        goto copy_6;
    if (size == 5)
        goto copy_5;
    if (size == 4)
        goto copy_4;
    if (size == 2)
        goto copy_2;
    if (size == 1)
        goto copy_1;
    return;

copy_7:
    dst[6] = (uchar)str[6];
copy_6:
    dst[5] = (uchar)str[5];
copy_5:
    dst[4] = (uchar)str[4];
copy_4:
    dst[3] = (uchar)str[3];
copy_3:
    dst[2] = (uchar)str[2];
copy_2:
    dst[1] = (uchar)str[1];
copy_1:
    dst[0] = (uchar)str[0];
}

void fromLatin1_prolog_sse2_overcommit(ushort *dst, const char *str, int)
{
    // do one iteration of conversion
    const __m128i chunk = _mm_loadu_si128((__m128i*)str); // load

    // unpack only the first 8 bytes, padding with zeros
    const __m128i nullMask = _mm_set1_epi32(0);
    const __m128i firstHalf = _mm_unpacklo_epi8(chunk, nullMask);
    _mm_storeu_si128((__m128i*)dst, firstHalf); // store
}

template<FromLatin1Function prologFunction>
void fromLatin1_sse2_withprolog(ushort *dst, const char *str, int size)
{
    // same as the improved code, but we attempt to align at the prolog
    // therefore, we issue aligned stores

    if (size >= 16) {
        uint misalignment = uint(quintptr(dst) & 0xf);
        uint prologCount = (16 - misalignment) / 2;

        prologFunction(dst, str, prologCount);

        size -= prologCount;
        dst += prologCount;
        str += prologCount;
    }

    const __m128i nullMask = _mm_set1_epi32(0);
    qptrdiff counter = 0;
    size -= 16;
    while (size >= counter) {
        const __m128i chunk = _mm_loadu_si128((__m128i*)(str + counter)); // load

        // unpack the first 8 bytes, padding with zeros
        const __m128i firstHalf = _mm_unpacklo_epi8(chunk, nullMask);
        _mm_store_si128((__m128i*)(dst + counter), firstHalf); // store

        // unpack the last 8 bytes, padding with zeros
        const __m128i secondHalf = _mm_unpackhi_epi8 (chunk, nullMask);
        _mm_store_si128((__m128i*)(dst + counter + 8), secondHalf); // store

        counter += 16;
    }
    size += 16;
    fromLatin1_epilog(dst + counter, str + counter, size - counter);
}

#ifdef __SSE4_1__
void fromLatin1_sse4_pmovzxbw(ushort *dst, const char *str, int size)
{
    qptrdiff counter = 0;
    size -= 16;
    while (size >= counter) {
        __m128i chunk = _mm_loadu_si128((__m128i*)(str + counter)); // load

        // unpack the first 8 bytes, padding with zeros
        const __m128i firstHalf = _mm_cvtepu8_epi16(chunk);
        _mm_storeu_si128((__m128i*)(dst + counter), firstHalf); // store

        // unpack the last 8 bytes, padding with zeros
        chunk = _mm_srli_si128(chunk, 8);
        const __m128i secondHalf = _mm_cvtepu8_epi16(chunk);
        _mm_storeu_si128((__m128i*)(dst + counter + 8), secondHalf); // store

        counter += 16;
    }
    size += 16;
    fromLatin1_epilog(dst + counter, str + counter, size - counter);
}

void fromLatin1_prolog_sse4_overcommit(ushort *dst, const char *str, int)
{
    // load 8 bytes and zero-extend them to 16
    const __m128i chunk = _mm_cvtepu8_epi16(*(__m128i*)str); // load
    _mm_storeu_si128((__m128i*)dst, chunk); // store
}
#endif
#endif

#ifdef __ARM_NEON__
static inline void fromLatin1_epilog(ushort *dst, const char *str, int size)
{
    if (!size) return;
    dst[0] = (uchar)str[0];
    if (!--size) return;
    dst[1] = (uchar)str[1];
    if (!--size) return;
    dst[2] = (uchar)str[2];
    if (!--size) return;
    dst[3] = (uchar)str[3];
    if (!--size) return;
    dst[4] = (uchar)str[4];
    if (!--size) return;
    dst[5] = (uchar)str[5];
    if (!--size) return;
    dst[6] = (uchar)str[6];
    if (!--size) return;
    dst[7] = (uchar)str[7];
    if (!--size) return;
}

void fromLatin1_neon_improved(ushort *dst, const char *str, int len)
{
    while (len >= 8) {
        // load 8 bytes into one doubleword Neon register
        const uint8x8_t chunk = vld1_u8((uint8_t *)str);
        str += 8;

        // expand 8 bytes into 16 bytes in a quadword register
        const uint16x8_t expanded = vmovl_u8(chunk);
        vst1q_u16(dst, expanded); // store
        dst += 8;

        len -= 8;
    }
    fromLatin1_epilog(dst, str, len);
}

void fromLatin1_neon_improved2(ushort *dst, const char *str, int len)
{
    while (len >= 16) {
        // load 16 bytes into one quadword Neon register
        const uint8x16_t chunk = vld1q_u8((uint8_t *)str);
        str += 16;

        // expand each doubleword of the quadword register into a quadword
        const uint16x8_t expanded_low = vmovl_u8(vget_low_u8(chunk));
        vst1q_u16(dst, expanded_low); // store
        dst += 8;
        const uint16x8_t expanded_high = vmovl_u8(vget_high_u8(chunk));
        vst1q_u16(dst, expanded_high); // store
        dst += 8;

        len -= 16;
    }

    if (len >= 8) {
        // load 8 bytes into one doubleword Neon register
        const uint8x8_t chunk = vld1_u8((uint8_t *)str);
        str += 8;

        // expand 8 bytes into 16 bytes in a quadword register
        const uint16x8_t expanded = vmovl_u8(chunk);
        vst1q_u16(dst, expanded); // store
        dst += 8;

        len -= 8;
    }
    fromLatin1_epilog(dst, str, len);
}

void fromLatin1_neon_handwritten(ushort *dst, const char *str, int len)
{
    // same as above, but handwritten Neon
    while (len >= 8) {
        uint16x8_t chunk;
        asm (
            "vld1.8     %[chunk], [%[str]]!\n"
            "vmovl.u8   %q[chunk], %[chunk]\n"
            "vst1.16    %h[chunk], [%[dst]]!\n"
            : [dst] "+r" (dst),
              [str] "+r" (str),
              [chunk] "=w" (chunk));
        len -= 8;
    }

    fromLatin1_epilog(dst, str, len);
}

void fromLatin1_neon_handwritten2(ushort *dst, const char *str, int len)
{
    // same as above, but handwritten Neon
    while (len >= 16) {
        uint16x8_t chunk1, chunk2;
        asm (
            "vld1.8     %h[chunk1], [%[str]]!\n"
            "vmovl.u8   %q[chunk2], %f[chunk1]\n"
            "vmovl.u8   %q[chunk1], %e[chunk1]\n"
            "vst1.16    %h[chunk1], [%[dst]]!\n"
            "vst1.16    %h[chunk2], [%[dst]]!\n"
          : [dst] "+r" (dst),
            [str] "+r" (str),
            [chunk1] "=w" (chunk1),
            [chunk2] "=w" (chunk2));
        len -= 16;
    }

    if (len >= 8) {
        uint16x8_t chunk;
        asm (
            "vld1.8     %[chunk], [%[str]]!\n"
            "vmovl.u8   %q[chunk], %[chunk]\n"
            "vst1.16    %h[chunk], [%[dst]]!\n"
            : [dst] "+r" (dst),
              [str] "+r" (str),
              [chunk] "=w" (chunk));
        len -= 8;
    }

    fromLatin1_epilog(dst, str, len);
}
#endif

void tst_QString::fromLatin1Alternatives_data() const
{
    QTest::addColumn<FromLatin1Function>("function");
    QTest::newRow("empty") << FromLatin1Function(0);
    QTest::newRow("regular") << &fromLatin1_regular;
#ifdef __SSE2__
    QTest::newRow("sse2-qt4.7") << &fromLatin1_sse2_qt47;
    QTest::newRow("sse2-improved") << &fromLatin1_sse2_improved;
    QTest::newRow("sse2-improved2") << &fromLatin1_sse2_improved2;
    QTest::newRow("sse2-with-prolog-regular") << &fromLatin1_sse2_withprolog<&fromLatin1_regular>;
    QTest::newRow("sse2-with-prolog-unrolled") << &fromLatin1_sse2_withprolog<&fromLatin1_prolog_unrolled>;
    QTest::newRow("sse2-with-prolog-sse2-overcommit") << &fromLatin1_sse2_withprolog<&fromLatin1_prolog_sse2_overcommit>;
#ifdef __SSE4_1__
    QTest::newRow("sse2-with-prolog-sse4-overcommit") << &fromLatin1_sse2_withprolog<&fromLatin1_prolog_sse4_overcommit>;
    QTest::newRow("sse4-pmovzxbw") << &fromLatin1_sse4_pmovzxbw;
#endif
#endif
#ifdef __ARM_NEON__
    QTest::newRow("neon-improved") << &fromLatin1_neon_improved;
    QTest::newRow("neon-improved2") << &fromLatin1_neon_improved2;
    QTest::newRow("neon-handwritten") << &fromLatin1_neon_handwritten;
    QTest::newRow("neon-handwritten2") << &fromLatin1_neon_handwritten2;
#endif
}

extern StringData fromLatin1Data;
static void fromLatin1Alternatives_internal(FromLatin1Function function, QString &dst, bool doVerify)
{
    struct Entry
    {
        int len;
        int offset1, offset2;
        int align1, align2;
    };
    const Entry *entries = reinterpret_cast<const Entry *>(fromLatin1Data.entries);

    for (int i = 0; i < fromLatin1Data.entryCount; ++i) {
        int len = entries[i].len;
        const char *src = fromLatin1Data.charData + entries[i].offset1;

        if (!function)
            continue;
        if (!doVerify) {
            (function)(&dst.data()->unicode(), src, len);
        } else {
            dst.fill(QChar('x'), dst.length());

            (function)(&dst.data()->unicode() + 8, src, len);

            QString zeroes(8, QChar('x'));
            QString final = dst.mid(8, len);
            QCOMPARE(final, QString::fromLatin1(src, len));
            QCOMPARE(dst.left(8), zeroes);
            QCOMPARE(dst.mid(len + 8, 8), zeroes);
        }
    }
}

void tst_QString::fromLatin1Alternatives() const
{
    QFETCH(FromLatin1Function, function);

    QString dst(fromLatin1Data.maxLength + 16, QChar('x'));
    fromLatin1Alternatives_internal(function, dst, true);

    QBENCHMARK {
        fromLatin1Alternatives_internal(function, dst, false);
    }
}

typedef int (* FromUtf8Function)(ushort *, const char *, int);
Q_DECLARE_METATYPE(FromUtf8Function)

extern QTextCodec::ConverterState *state;
QTextCodec::ConverterState *state = 0; // just because the code in qutfcodec.cpp uses a state

int fromUtf8_latin1_regular(ushort *dst, const char *chars, int len)
{
    fromLatin1_regular(dst, chars, len);
    return len;
}

#ifdef __SSE2__
int fromUtf8_latin1_qt47(ushort *dst, const char *chars, int len)
{
    fromLatin1_sse2_qt47(dst, chars, len);
    return len;
}

int fromUtf8_latin1_sse2_improved(ushort *dst, const char *chars, int len)
{
    fromLatin1_sse2_improved(dst, chars, len);
    return len;
}
#endif

int fromUtf8_qt47(ushort *dst, const char *chars, int len)
{
    // this is almost the code found in Qt 4.7's qutfcodec.cpp QUtf8Codec::convertToUnicode
    // That function returns a QString, this one returns the number of characters converted
    // That's to avoid doing malloc() inside the benchmark test
    // Any differences between this code and the original are just because of that, I promise

    bool headerdone = false;
    ushort replacement = QChar::ReplacementCharacter;
    int need = 0;
    int error = -1;
    uint uc = 0;
    uint min_uc = 0;
    if (state) {
        if (state->flags & QTextCodec::IgnoreHeader)
            headerdone = true;
        if (state->flags & QTextCodec::ConvertInvalidToNull)
            replacement = QChar::Null;
        need = state->remainingChars;
        if (need) {
            uc = state->state_data[0];
            min_uc = state->state_data[1];
        }
    }
    if (!headerdone && len > 3
        && (uchar)chars[0] == 0xef && (uchar)chars[1] == 0xbb && (uchar)chars[2] == 0xbf) {
        // starts with a byte order mark
        chars += 3;
        len -= 3;
        headerdone = true;
    }

    // QString result(need + len + 1, Qt::Uninitialized); // worst case
    // ushort *qch = (ushort *)result.unicode();
    ushort *qch = dst;
    uchar ch;
    int invalid = 0;

    for (int i = 0; i < len; ++i) {
        ch = chars[i];
        if (need) {
            if ((ch&0xc0) == 0x80) {
                uc = (uc << 6) | (ch & 0x3f);
                --need;
                if (!need) {
                    // utf-8 bom composes into 0xfeff code point
                    if (!headerdone && uc == 0xfeff) {
                        // don't do anything, just skip the BOM
                    } else if (QChar::requiresSurrogates(uc) && uc <= QChar::LastValidCodePoint) {
                        // surrogate pair
                        //Q_ASSERT((qch - (ushort*)result.unicode()) + 2 < result.length());
                        *qch++ = QChar::highSurrogate(uc);
                        *qch++ = QChar::lowSurrogate(uc);
                    } else if ((uc < min_uc) || QChar::isSurrogate(uc) || uc > QChar::LastValidCodePoint) {
                        // error: overlong sequence or UTF16 surrogate
                        *qch++ = replacement;
                        ++invalid;
                    } else {
                        *qch++ = uc;
                    }
                    headerdone = true;
                }
            } else {
                // error
                i = error;
                *qch++ = replacement;
                ++invalid;
                need = 0;
                headerdone = true;
            }
        } else {
            if (ch < 128) {
                *qch++ = ushort(ch);
                headerdone = true;
            } else if ((ch & 0xe0) == 0xc0) {
                uc = ch & 0x1f;
                need = 1;
                error = i;
                min_uc = 0x80;
                headerdone = true;
            } else if ((ch & 0xf0) == 0xe0) {
                uc = ch & 0x0f;
                need = 2;
                error = i;
                min_uc = 0x800;
            } else if ((ch&0xf8) == 0xf0) {
                uc = ch & 0x07;
                need = 3;
                error = i;
                min_uc = 0x10000;
                headerdone = true;
            } else {
                // error
                *qch++ = replacement;
                ++invalid;
                headerdone = true;
            }
        }
    }
    if (!state && need > 0) {
        // unterminated UTF sequence
        for (int i = error; i < len; ++i) {
            *qch++ = replacement;
            ++invalid;
        }
    }
    //result.truncate(qch - (ushort *)result.unicode());
    if (state) {
        state->invalidChars += invalid;
        state->remainingChars = need;
        if (headerdone)
            state->flags |= QTextCodec::IgnoreHeader;
        state->state_data[0] = need ? uc : 0;
        state->state_data[1] = need ? min_uc : 0;
    }
    //return result;
    return qch - dst;
}

int fromUtf8_qt47_stateless(ushort *dst, const char *chars, int len)
{
    // This is the same code as above, but for stateless UTF-8 conversion
    // no other improvements
    bool headerdone = false;
    const ushort replacement = QChar::ReplacementCharacter;
    int need = 0;
    int error = -1;
    uint uc = 0;
    uint min_uc = 0;

    if (len > 3
        && (uchar)chars[0] == 0xef && (uchar)chars[1] == 0xbb && (uchar)chars[2] == 0xbf) {
        // starts with a byte order mark
        chars += 3;
        len -= 3;
    }

    // QString result(need + len + 1, Qt::Uninitialized); // worst case
    // ushort *qch = (ushort *)result.unicode();
    ushort *qch = dst;
    uchar ch;
    int invalid = 0;

    for (int i = 0; i < len; ++i) {
        ch = chars[i];
        if (need) {
            if ((ch&0xc0) == 0x80) {
                uc = (uc << 6) | (ch & 0x3f);
                --need;
                if (!need) {
                    // utf-8 bom composes into 0xfeff code point
                    if (!headerdone && uc == 0xfeff) {
                        // don't do anything, just skip the BOM
                    } else if (QChar::requiresSurrogates(uc) && uc <= QChar::LastValidCodePoint) {
                        // surrogate pair
                        //Q_ASSERT((qch - (ushort*)result.unicode()) + 2 < result.length());
                        *qch++ = QChar::highSurrogate(uc);
                        *qch++ = QChar::lowSurrogate(uc);
                    } else if ((uc < min_uc) || QChar::isSurrogate(uc) || uc > QChar::LastValidCodePoint) {
                        // error: overlong sequence or UTF16 surrogate
                        *qch++ = replacement;
                        ++invalid;
                    } else {
                        *qch++ = uc;
                    }
                    headerdone = true;
                }
            } else {
                // error
                i = error;
                *qch++ = replacement;
                ++invalid;
                need = 0;
                headerdone = true;
            }
        } else {
            if (ch < 128) {
                *qch++ = ushort(ch);
                headerdone = true;
            } else if ((ch & 0xe0) == 0xc0) {
                uc = ch & 0x1f;
                need = 1;
                error = i;
                min_uc = 0x80;
                headerdone = true;
            } else if ((ch & 0xf0) == 0xe0) {
                uc = ch & 0x0f;
                need = 2;
                error = i;
                min_uc = 0x800;
            } else if ((ch&0xf8) == 0xf0) {
                uc = ch & 0x07;
                need = 3;
                error = i;
                min_uc = 0x10000;
                headerdone = true;
            } else {
                // error
                *qch++ = replacement;
                ++invalid;
                headerdone = true;
            }
        }
    }
    if (need > 0) {
        // unterminated UTF sequence
        for (int i = error; i < len; ++i) {
            *qch++ = replacement;
            ++invalid;
        }
    }
    //result.truncate(qch - (ushort *)result.unicode());
    //return result;
    return qch - dst;
}

template <bool trusted>
static inline void extract_utf8_multibyte(ushort *&dst, const char *&chars, qptrdiff &counter, int &len)
{
    uchar ch = chars[counter];

    // is it a leading or a continuation one?
    if (!trusted && (ch & 0xc0) == 0x80) {
        // continuation character found without the leading
        dst[counter++] = QChar::ReplacementCharacter;
        return;
    }

    if ((ch & 0xe0) == 0xc0) {
        // two-byte UTF-8 sequence
        if (!trusted && counter + 1 == len) {
            dst[counter++] = QChar::ReplacementCharacter;
            return;
        }

        uchar ch2 = chars[counter + 1];
        if (!trusted)
            if ((ch2 & 0xc0) != 0x80) {
                dst[counter++] = QChar::ReplacementCharacter;
                return;
            }

        ushort ucs = (ch & 0x1f);
        ucs <<= 6;
        ucs |= (ch2 & 0x3f);

        // dst[counter] will correspond to chars[counter..counter+1], so adjust
        ++chars;
        --len;
        if (trusted || ucs >= 0x80)
            dst[counter] = ucs;
        else
            dst[counter] = QChar::ReplacementCharacter;
        ++counter;
        return;
    }

    if ((ch & 0xf0) == 0xe0) {
        // three-byte UTF-8 sequence
        if (!trusted && counter + 2 >= len) {
            dst[counter++] = QChar::ReplacementCharacter;
            return;
        }

        uchar ch2 = chars[counter + 1];
        uchar ch3 = chars[counter + 2];
        if (!trusted)
            if ((ch2 & 0xc0) != 0x80 || (ch3 & 0xc0) != 0x80) {
                dst[counter++] = QChar::ReplacementCharacter;
                return;
            }

        ushort ucs = (ch & 0x1f) << 12 | (ch2 & 0x3f) << 6 | (ch3 & 0x3f);

        // dst[counter] will correspond to chars[counter..counter+2], so adjust
        chars += 2;
        len -= 2;
        if (!trusted &&
            (ucs < 0x800 || QChar::isSurrogate(ucs)))
            dst[counter] = QChar::ReplacementCharacter;
        else
            dst[counter] = ucs;
        ++counter;
        return;
    }

    if ((ch & 0xf8) == 0xf0) {
        // four-byte UTF-8 sequence
        // will require an UTF-16 surrogate pair
        if (!trusted && counter + 3 >= len) {
            dst[counter++] = QChar::ReplacementCharacter;
            return;
        }

        uchar ch2 = chars[counter + 1];
        uchar ch3 = chars[counter + 2];
        uchar ch4 = chars[counter + 3];
        if (!trusted)
            if ((ch2 & 0xc0) != 0x80 || (ch3 & 0xc0) != 0x80 || (ch4 & 0xc0) != 0x80) {
                dst[counter++] = QChar::ReplacementCharacter;
                return;
            }

        uint ucs = (ch & 0x1f) << 18 | (ch2 & 0x3f) << 12
                   | (ch3 & 0x3f) << 6 | (ch4 & 0x3f);

        // dst[counter] will correspond to chars[counter..counter+2], so adjust
        chars += 3;
        len -= 3;
        if (trusted || (QChar::requiresSurrogates(ucs) && ucs <= QChar::LastValidCodePoint)) {
            dst[counter + 0] = QChar::highSurrogate(ucs);
            dst[counter + 1] = QChar::lowSurrogate(ucs);
            counter += 2;
        } else {
            dst[counter++] = QChar::ReplacementCharacter;
        }
        return;
    }

    ++counter;
}

int fromUtf8_optimised_for_ascii(ushort *qch, const char *chars, int len)
{
    if (len > 3
        && (uchar)chars[0] == 0xef && (uchar)chars[1] == 0xbb && (uchar)chars[2] == 0xbf) {
        // starts with a byte order mark
        chars += 3;
        len -= 3;
    }

    qptrdiff counter = 0;
    ushort *dst = qch;
    while (counter < len) {
        uchar ch = chars[counter];
        if ((ch & 0x80) == 0) {
            dst[counter] = ch;
            ++counter;
            continue;
        }

        // UTF-8 character found
        extract_utf8_multibyte<false>(dst, chars, counter, len);
    }
    return dst + counter - qch;
}

#ifdef __SSE2__
int fromUtf8_sse2_optimised_for_ascii(ushort *qch, const char *chars, int len)
{
    if (len > 3
        && (uchar)chars[0] == 0xef && (uchar)chars[1] == 0xbb && (uchar)chars[2] == 0xbf) {
        // starts with a byte order mark
        chars += 3;
        len -= 3;
    }

    qptrdiff counter = 0;
    ushort *dst = qch;

    len -= 16;
    const __m128i nullMask = _mm_set1_epi32(0);
    while (counter < len) {
        const __m128i chunk = _mm_loadu_si128((__m128i*)(chars + counter)); // load
        ushort highbytes = _mm_movemask_epi8(chunk);

        // unpack the first 8 bytes, padding with zeros
        const __m128i firstHalf = _mm_unpacklo_epi8(chunk, nullMask);
        _mm_storeu_si128((__m128i*)(dst + counter), firstHalf); // store

        if (!uchar(highbytes)) {
            // unpack the last 8 bytes, padding with zeros
            const __m128i secondHalf = _mm_unpackhi_epi8 (chunk, nullMask);
            _mm_storeu_si128((__m128i*)(dst + counter + 8), secondHalf); // store

            if (!highbytes) {
                counter += 16;
                continue;
            }
        }

        // UTF-8 character found
        // which one?
        counter += bsf_nonzero(highbytes);
        len += 16;
        extract_utf8_multibyte<false>(dst, chars, counter, len);
        len -= 16;
    }
    len += 16;

    while (counter < len) {
        uchar ch = chars[counter];
        if ((ch & 0x80) == 0) {
            dst[counter] = ch;
            ++counter;
            continue;
        }

        // UTF-8 character found
        extract_utf8_multibyte<false>(dst, chars, counter, len);
    }
    return dst + counter - qch;
}

int fromUtf8_sse2_trusted_no_bom(ushort *qch, const char *chars, int len)
{
    qptrdiff counter = 0;
    ushort *dst = qch;

    len -= 16;
    const __m128i nullMask = _mm_set1_epi32(0);
    while (counter < len) {
        const __m128i chunk = _mm_loadu_si128((__m128i*)(chars + counter)); // load
        ushort highbytes = _mm_movemask_epi8(chunk);

        // unpack the first 8 bytes, padding with zeros
        const __m128i firstHalf = _mm_unpacklo_epi8(chunk, nullMask);
        _mm_storeu_si128((__m128i*)(dst + counter), firstHalf); // store

        if (!uchar(highbytes)) {
            // unpack the last 8 bytes, padding with zeros
            const __m128i secondHalf = _mm_unpackhi_epi8 (chunk, nullMask);
            _mm_storeu_si128((__m128i*)(dst + counter + 8), secondHalf); // store

            if (!highbytes) {
                counter += 16;
                continue;
            }
        }

        // UTF-8 character found
        // which one?
        counter += bsf_nonzero(highbytes);
        len += 16;
        extract_utf8_multibyte<true>(dst, chars, counter, len);
        len -= 16;
    }
    len += 16;

    while (counter < len) {
        uchar ch = chars[counter];
        if ((ch & 0x80) == 0) {
            dst[counter] = ch;
            ++counter;
            continue;
        }

        // UTF-8 character found
        extract_utf8_multibyte<true>(dst, chars, counter, len);
    }
    return dst + counter - qch;
}
#endif

#ifdef __ARM_NEON__
int fromUtf8_latin1_neon(ushort *dst, const char *chars, int len)
{
    fromLatin1_neon_improved(dst, chars, len);
    return len;
}

int fromUtf8_neon(ushort *qch, const char *chars, int len)
{
    if (len > 3
        && (uchar)chars[0] == 0xef && (uchar)chars[1] == 0xbb && (uchar)chars[2] == 0xbf) {
        // starts with a byte order mark
        chars += 3;
        len -= 3;
    }

    ushort *dst = qch;
    const uint8x8_t highBit = vdup_n_u8(0x80);
    while (len >= 8) {
        // load 8 bytes into one doubleword Neon register
        const uint8x8_t chunk = vld1_u8((uint8_t *)chars);
        const uint16x8_t expanded = vmovl_u8(chunk);
        vst1q_u16(dst, expanded);

        uint8x8_t highBits = vtst_u8(chunk, highBit);
        // we need to find the lowest byte set
        int mask_low = vget_lane_u32(vreinterpret_u32_u8(highBits), 0);
        int mask_high = vget_lane_u32(vreinterpret_u32_u8(highBits), 1);

        if (__builtin_expect(mask_low == 0 && mask_high == 0, 1)) {
            chars += 8;
            dst += 8;
            len -= 8;
        } else {
            // UTF-8 character found
            // which one?
            qptrdiff pos;
            asm ("rbit  %0, %1\n"
                 "clz   %1, %1\n"
               : "=r" (pos)
               : "r" (mask_low ? mask_low : mask_high));
            // now mask_low contains the number of leading zeroes
            // or the value 32 (0x20) if no zeroes were found
            // the number of leading zeroes is 8*pos
            pos /= 8;

            extract_utf8_multibyte<false>(dst, chars, pos, len);
            chars += pos;
            dst += pos;
            len -= pos;
        }
    }

    qptrdiff counter = 0;
    while (counter < len) {
        uchar ch = chars[counter];
        if ((ch & 0x80) == 0) {
            dst[counter] = ch;
            ++counter;
            continue;
        }
        // UTF-8 character found
        extract_utf8_multibyte<false>(dst, chars, counter, len);
    }
    return dst + counter - qch;
}

int fromUtf8_neon_trusted(ushort *qch, const char *chars, int len)
{
    ushort *dst = qch;
    const uint8x8_t highBit = vdup_n_u8(0x80);
    while (len >= 8) {
        // load 8 bytes into one doubleword Neon register
        const uint8x8_t chunk = vld1_u8((uint8_t *)chars);
        const uint16x8_t expanded = vmovl_u8(chunk);
        vst1q_u16(dst, expanded);

        uint8x8_t highBits = vtst_u8(chunk, highBit);
        // we need to find the lowest byte set
        int mask_low = vget_lane_u32(vreinterpret_u32_u8(highBits), 0);
        int mask_high = vget_lane_u32(vreinterpret_u32_u8(highBits), 1);

        if (__builtin_expect(mask_low == 0 && mask_high == 0, 1)) {
            chars += 8;
            dst += 8;
            len -= 8;
        } else {
            // UTF-8 character found
            // which one?
            qptrdiff pos;
            asm ("rbit  %0, %1\n"
                 "clz   %1, %1\n"
               : "=r" (pos)
               : "r" (mask_low ? mask_low : mask_high));
            // now mask_low contains the number of leading zeroes
            // or the value 32 (0x20) if no zeroes were found
            // the number of leading zeroes is 8*pos
            pos /= 8;

            extract_utf8_multibyte<true>(dst, chars, pos, len);
            chars += pos;
            dst += pos;
            len -= pos;
        }
    }

    qptrdiff counter = 0;
    while (counter < len) {
        uchar ch = chars[counter];
        if ((ch & 0x80) == 0) {
            dst[counter] = ch;
            ++counter;
            continue;
        }

        // UTF-8 character found
        extract_utf8_multibyte<true>(dst, chars, counter, len);
    }
    return dst + counter - qch;
}
#endif

void tst_QString::fromUtf8Alternatives_data() const
{
    QTest::addColumn<FromUtf8Function>("function");
    QTest::newRow("empty") << FromUtf8Function(0);
    QTest::newRow("qt-4.7") << &fromUtf8_qt47;
    QTest::newRow("qt-4.7-stateless") << &fromUtf8_qt47_stateless;
    QTest::newRow("optimized-for-ascii") << &fromUtf8_optimised_for_ascii;
#ifdef __SSE2__
    QTest::newRow("sse2-optimized-for-ascii") << &fromUtf8_sse2_optimised_for_ascii;
    QTest::newRow("sse2-trusted-no-bom") << &fromUtf8_sse2_trusted_no_bom;
#endif
#ifdef __ARM_NEON__
    QTest::newRow("neon") << &fromUtf8_neon;
    QTest::newRow("neon-trusted-no-bom") << &fromUtf8_neon_trusted;
#endif

    QTest::newRow("latin1-generic") << &fromUtf8_latin1_regular;
#ifdef __SSE2__
    QTest::newRow("latin1-sse2-qt4.7") << &fromUtf8_latin1_qt47;
    QTest::newRow("latin1-sse2-improved") << &fromUtf8_latin1_sse2_improved;
#endif
#ifdef __ARM_NEON__
    QTest::newRow("latin1-neon-improved") << &fromUtf8_latin1_neon;
#endif
}

extern StringData fromUtf8Data;
static void fromUtf8Alternatives_internal(FromUtf8Function function, QString &dst, bool doVerify)
{
    if (!doVerify) {
        // NOTE: this only works because the Latin1 data is ASCII-only
        fromLatin1Alternatives_internal(reinterpret_cast<FromLatin1Function>(function), dst, doVerify);
    } else {
        if (strncmp(QTest::currentDataTag(), "latin1-", 7) == 0)
            return;
    }

    struct Entry
    {
        int len;
        int offset1, offset2;
        int align1, align2;
    };
    const Entry *entries = reinterpret_cast<const Entry *>(fromUtf8Data.entries);

    for (int i = 0; i < fromUtf8Data.entryCount; ++i) {
        int len = entries[i].len;
        const char *src = fromUtf8Data.charData + entries[i].offset1;

        if (!function)
            continue;
        if (!doVerify) {
            (function)(&dst.data()->unicode(), src, len);
        } else {
            dst.fill(QChar('x'), dst.length());

            int utf8len = (function)(&dst.data()->unicode() + 8, src, len);

            QString expected = QString::fromUtf8(src, len);
            QString final = dst.mid(8, expected.length());
            if (final != expected || utf8len != expected.length())
                qDebug() << i << entries[i].offset1 << utf8len << final << expected.length() << expected;

            QCOMPARE(final, expected);
            QCOMPARE(utf8len, expected.length());

            QString zeroes(8, QChar('x'));
            QCOMPARE(dst.left(8), zeroes);
            QCOMPARE(dst.mid(len + 8, 8), zeroes);
        }
    }
}

void tst_QString::fromUtf8Alternatives() const
{
    QFETCH(FromUtf8Function, function);

    QString dst(fromUtf8Data.maxLength + 16, QChar('x'));
    fromUtf8Alternatives_internal(function, dst, true);

    QBENCHMARK {
        fromUtf8Alternatives_internal(function, dst, false);
    }
}

void tst_QString::toUpper_data()
{
    QTest::addColumn<QString>("s");

    QString lowerLatin1(300, QChar('a'));
    QString upperLatin1(300, QChar('A'));

    QString lowerDeseret;
    {
        QString pattern;
        pattern += QChar(QChar::highSurrogate(0x10428));
        pattern += QChar(QChar::lowSurrogate(0x10428));
        for (int i = 0; i < 300 / pattern.size(); ++i)
            lowerDeseret += pattern;
    }
    QString upperDeseret;
    {
        QString pattern;
        pattern += QChar(QChar::highSurrogate(0x10400));
        pattern += QChar(QChar::lowSurrogate(0x10400));
        for (int i = 0; i < 300 / pattern.size(); ++i)
            upperDeseret += pattern;
    }

    QString lowerLigature(600, QChar(0xFB03));

    QTest::newRow("600<a>") << (lowerLatin1 + lowerLatin1);
    QTest::newRow("600<A>") << (upperLatin1 + upperLatin1);

    QTest::newRow("300<a>+300<A>") << (lowerLatin1 + upperLatin1);
    QTest::newRow("300<A>+300<a>") << (upperLatin1 + lowerLatin1);

    QTest::newRow("300<10428>") << (lowerDeseret + lowerDeseret);
    QTest::newRow("300<10400>") << (upperDeseret + upperDeseret);

    QTest::newRow("150<10428>+150<10400>") << (lowerDeseret + upperDeseret);
    QTest::newRow("150<10400>+150<10428>") << (upperDeseret + lowerDeseret);

    QTest::newRow("300a+150<10400>") << (lowerLatin1 + upperDeseret);
    QTest::newRow("300a+150<10428>") << (lowerLatin1 + lowerDeseret);
    QTest::newRow("300A+150<10400>") << (upperLatin1 + upperDeseret);
    QTest::newRow("300A+150<10428>") << (upperLatin1 + lowerDeseret);

    QTest::newRow("600<FB03> (ligature)") << lowerLigature;
}

void tst_QString::toUpper()
{
    QFETCH(QString, s);

    QBENCHMARK {
        s.toUpper();
    }
}

void tst_QString::toLower_data()
{
    toUpper_data();
}

void tst_QString::toLower()
{
    QFETCH(QString, s);

    QBENCHMARK {
        s.toLower();
    }
}

void tst_QString::toCaseFolded_data()
{
    toUpper_data();
}

void tst_QString::toCaseFolded()
{
    QFETCH(QString, s);

    QBENCHMARK {
        s.toCaseFolded();
    }
}

QTEST_APPLESS_MAIN(tst_QString)

#include "main.moc"
