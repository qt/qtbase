// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qbytearray.h"
#include "qbytearraymatcher.h"
#include "private/qtools_p.h"
#include "qhashfunctions.h"
#include "qlist.h"
#include "qlocale_p.h"
#include "qlocale_tools_p.h"
#include "private/qnumeric_p.h"
#include "private/qsimd_p.h"
#include "qstringalgorithms_p.h"
#include "qscopedpointer.h"
#include "qbytearray_p.h"
#include "qstringconverter_p.h"
#include <qdatastream.h>
#include <qmath.h>
#if defined(Q_OS_WASM)
#include "private/qstdweb_p.h"
#endif

#ifndef QT_NO_COMPRESS
#include <zconf.h>
#include <zlib.h>
#include <qxpfunctional.h>
#endif
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

Q_CONSTINIT const char QByteArray::_empty = '\0';

// ASCII case system, used by QByteArray::to{Upper,Lower}() and qstr(n)icmp():
static constexpr inline uchar asciiUpper(uchar c)
{
    return c >= 'a' && c <= 'z' ? c & ~0x20 : c;
}

static constexpr inline uchar asciiLower(uchar c)
{
    return c >= 'A' && c <= 'Z' ? c | 0x20 : c;
}

qsizetype qFindByteArray(
        const char *haystack0, qsizetype haystackLen, qsizetype from,
        const char *needle0, qsizetype needleLen);

/*****************************************************************************
  Safe and portable C string functions; extensions to standard string.h
 *****************************************************************************/

/*! \relates QByteArray

    Returns a duplicate string.

    Allocates space for a copy of \a src, copies it, and returns a
    pointer to the copy. If \a src is \nullptr, it immediately returns
    \nullptr.

    Ownership is passed to the caller, so the returned string must be
    deleted using \c delete[].
*/

char *qstrdup(const char *src)
{
    if (!src)
        return nullptr;
    char *dst = new char[strlen(src) + 1];
    return qstrcpy(dst, src);
}

/*! \relates QByteArray

    Copies all the characters up to and including the '\\0' from \a
    src into \a dst and returns a pointer to \a dst. If \a src is
    \nullptr, it immediately returns \nullptr.

    This function assumes that \a dst is large enough to hold the
    contents of \a src.

    \note If \a dst and \a src overlap, the behavior is undefined.

    \sa qstrncpy()
*/

char *qstrcpy(char *dst, const char *src)
{
    if (!src)
        return nullptr;
#ifdef Q_CC_MSVC
    const size_t len = strlen(src);
    // This is actually not secure!!! It will be fixed
    // properly in a later release!
    if (len >= 0 && strcpy_s(dst, len+1, src) == 0)
        return dst;
    return nullptr;
#else
    return strcpy(dst, src);
#endif
}

/*! \relates QByteArray

    A safe \c strncpy() function.

    Copies at most \a len bytes from \a src (stopping at \a len or the
    terminating '\\0' whichever comes first) into \a dst. Guarantees that \a
    dst is '\\0'-terminated, except when \a dst is \nullptr or \a len is 0. If
    \a src is \nullptr, returns \nullptr, otherwise returns \a dst.

    This function assumes that \a dst is at least \a len characters
    long.

    \note If \a dst and \a src overlap, the behavior is undefined.

    \note Unlike strncpy(), this function does \e not write '\\0' to all \a
    len bytes of \a dst, but stops after the terminating '\\0'. In this sense,
    it's similar to C11's strncpy_s().

    \sa qstrcpy()
*/

char *qstrncpy(char *dst, const char *src, size_t len)
{
    if (dst && len > 0) {
        *dst = '\0';
        if (src)
            std::strncat(dst, src, len - 1);
    }
    return src ? dst : nullptr;
}

/*! \fn size_t qstrlen(const char *str)
    \relates QByteArray

    A safe \c strlen() function.

    Returns the number of characters that precede the terminating '\\0',
    or 0 if \a str is \nullptr.

    \sa qstrnlen()
*/

/*! \fn size_t qstrnlen(const char *str, size_t maxlen)
    \relates QByteArray
    \since 4.2

    A safe \c strnlen() function.

    Returns the number of characters that precede the terminating '\\0', but
    at most \a maxlen. If \a str is \nullptr, returns 0.

    \sa qstrlen()
*/

/*!
    \relates QByteArray

    A safe \c strcmp() function.

    Compares \a str1 and \a str2. Returns a negative value if \a str1
    is less than \a str2, 0 if \a str1 is equal to \a str2 or a
    positive value if \a str1 is greater than \a str2.

    If both strings are \nullptr, they are deemed equal; otherwise, if either is
    \nullptr, it is treated as less than the other (even if the other is an
    empty string).

    \sa qstrncmp(), qstricmp(), qstrnicmp(), {Character Case},
    QByteArray::compare()
*/
int qstrcmp(const char *str1, const char *str2)
{
    return (str1 && str2) ? strcmp(str1, str2)
        : (str1 ? 1 : (str2 ? -1 : 0));
}

/*! \fn int qstrncmp(const char *str1, const char *str2, size_t len);

    \relates QByteArray

    A safe \c strncmp() function.

    Compares at most \a len bytes of \a str1 and \a str2.

    Returns a negative value if \a str1 is less than \a str2, 0 if \a
    str1 is equal to \a str2 or a positive value if \a str1 is greater
    than \a str2.

    If both strings are \nullptr, they are deemed equal; otherwise, if either is
    \nullptr, it is treated as less than the other (even if the other is an
    empty string or \a len is 0).

    \sa qstrcmp(), qstricmp(), qstrnicmp(), {Character Case},
    QByteArray::compare()
*/

/*! \relates QByteArray

    A safe \c stricmp() function.

    Compares \a str1 and \a str2, ignoring differences in the case of any ASCII
    characters.

    Returns a negative value if \a str1 is less than \a str2, 0 if \a
    str1 is equal to \a str2 or a positive value if \a str1 is greater
    than \a str2.

    If both strings are \nullptr, they are deemed equal; otherwise, if either is
    \nullptr, it is treated as less than the other (even if the other is an
    empty string).

    \sa qstrcmp(), qstrncmp(), qstrnicmp(), {Character Case},
    QByteArray::compare()
*/

int qstricmp(const char *str1, const char *str2)
{
    const uchar *s1 = reinterpret_cast<const uchar *>(str1);
    const uchar *s2 = reinterpret_cast<const uchar *>(str2);
    if (!s1)
        return s2 ? -1 : 0;
    if (!s2)
        return 1;

    enum { Incomplete = 256 };
    qptrdiff offset = 0;
    auto innerCompare = [=, &offset](qptrdiff max, bool unlimited) {
        max += offset;
        do {
            uchar c = s1[offset];
            if (int res = QtMiscUtils::caseCompareAscii(c, s2[offset]))
                return res;
            if (!c)
                return 0;
            ++offset;
        } while (unlimited || offset < max);
        return int(Incomplete);
    };

#if defined(__SSE4_1__) && !(defined(__SANITIZE_ADDRESS__) || __has_feature(address_sanitizer))
    enum { PageSize = 4096, PageMask = PageSize - 1 };
    const __m128i zero = _mm_setzero_si128();
    forever {
        // Calculate how many bytes we can load until we cross a page boundary
        // for either source. This isn't an exact calculation, just something
        // very quick.
        quintptr u1 = quintptr(s1 + offset);
        quintptr u2 = quintptr(s2 + offset);
        size_t n = PageSize - ((u1 | u2) & PageMask);

        qptrdiff maxoffset = offset + n;
        for ( ; offset + 16 <= maxoffset; offset += sizeof(__m128i)) {
            // load 16 bytes from either source
            __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i *>(s1 + offset));
            __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i *>(s2 + offset));

            // compare the two against each other
            __m128i cmp = _mm_cmpeq_epi8(a, b);

            // find NUL terminators too
            cmp = _mm_min_epu8(cmp, a);
            cmp = _mm_cmpeq_epi8(cmp, zero);

            // was there any difference or a NUL?
            uint mask = _mm_movemask_epi8(cmp);
            if (mask) {
                // yes, find out where
                uint start = qCountTrailingZeroBits(mask);
                uint end = sizeof(mask) * 8 - qCountLeadingZeroBits(mask);
                Q_ASSUME(end >= start);
                offset += start;
                n = end - start;
                break;
            }
        }

        // using SIMD could cause a page fault, so iterate byte by byte
        int res = innerCompare(n, false);
        if (res != Incomplete)
            return res;
    }
#endif

    return innerCompare(-1, true);
}

/*! \relates QByteArray

    A safe \c strnicmp() function.

    Compares at most \a len bytes of \a str1 and \a str2, ignoring differences
    in the case of any ASCII characters.

    Returns a negative value if \a str1 is less than \a str2, 0 if \a str1
    is equal to \a str2 or a positive value if \a str1 is greater than \a
    str2.

    If both strings are \nullptr, they are deemed equal; otherwise, if either is
    \nullptr, it is treated as less than the other (even if the other is an
    empty string or \a len is 0).

    \sa qstrcmp(), qstrncmp(), qstricmp(), {Character Case},
    QByteArray::compare()
*/

int qstrnicmp(const char *str1, const char *str2, size_t len)
{
    const uchar *s1 = reinterpret_cast<const uchar *>(str1);
    const uchar *s2 = reinterpret_cast<const uchar *>(str2);
    if (!s1 || !s2)
        return s1 ? 1 : (s2 ? -1 : 0);
    for (; len--; ++s1, ++s2) {
        const uchar c = *s1;
        if (int res = QtMiscUtils::caseCompareAscii(c, *s2))
            return res;
        if (!c)                                // strings are equal
            break;
    }
    return 0;
}

/*!
    \internal
    \since 5.12

    A helper for QByteArray::compare. Compares \a len1 bytes from \a str1 to \a
    len2 bytes from \a str2. If \a len2 is -1, then \a str2 is expected to be
    '\\0'-terminated.
 */
int qstrnicmp(const char *str1, qsizetype len1, const char *str2, qsizetype len2)
{
    Q_ASSERT(len1 >= 0);
    Q_ASSERT(len2 >= -1);
    const uchar *s1 = reinterpret_cast<const uchar *>(str1);
    const uchar *s2 = reinterpret_cast<const uchar *>(str2);
    if (!s1 || !len1) {
        if (len2 == 0)
            return 0;
        if (len2 == -1)
            return (!s2 || !*s2) ? 0 : -1;
        Q_ASSERT(s2);
        return -1;
    }
    if (!s2)
        return len1 == 0 ? 0 : 1;

    if (len2 == -1) {
        // null-terminated str2
        qsizetype i;
        for (i = 0; i < len1; ++i) {
            const uchar c = s2[i];
            if (!c)
                return 1;

            if (int res = QtMiscUtils::caseCompareAscii(s1[i], c))
                return res;
        }
        return s2[i] ? -1 : 0;
    } else {
        // not null-terminated
        const qsizetype len = qMin(len1, len2);
        for (qsizetype i = 0; i < len; ++i) {
            if (int res = QtMiscUtils::caseCompareAscii(s1[i], s2[i]))
                return res;
        }
        if (len1 == len2)
            return 0;
        return len1 < len2 ? -1 : 1;
    }
}

/*!
    \internal
 */
int QtPrivate::compareMemory(QByteArrayView lhs, QByteArrayView rhs)
{
    if (!lhs.isNull() && !rhs.isNull()) {
        int ret = memcmp(lhs.data(), rhs.data(), qMin(lhs.size(), rhs.size()));
        if (ret != 0)
            return ret;
    }

    // they matched qMin(l1, l2) bytes
    // so the longer one is lexically after the shorter one
    return lhs.size() == rhs.size() ? 0 : lhs.size() > rhs.size() ? 1 : -1;
}

/*!
    \internal
*/
bool QtPrivate::isValidUtf8(QByteArrayView s) noexcept
{
    return QUtf8::isValidUtf8(s).isValidUtf8;
}

// the CRC table below is created by the following piece of code
#if 0
static void createCRC16Table()                        // build CRC16 lookup table
{
    unsigned int i;
    unsigned int j;
    unsigned short crc_tbl[16];
    unsigned int v0, v1, v2, v3;
    for (i = 0; i < 16; i++) {
        v0 = i & 1;
        v1 = (i >> 1) & 1;
        v2 = (i >> 2) & 1;
        v3 = (i >> 3) & 1;
        j = 0;
#undef SET_BIT
#define SET_BIT(x, b, v) (x) |= (v) << (b)
        SET_BIT(j,  0, v0);
        SET_BIT(j,  7, v0);
        SET_BIT(j, 12, v0);
        SET_BIT(j,  1, v1);
        SET_BIT(j,  8, v1);
        SET_BIT(j, 13, v1);
        SET_BIT(j,  2, v2);
        SET_BIT(j,  9, v2);
        SET_BIT(j, 14, v2);
        SET_BIT(j,  3, v3);
        SET_BIT(j, 10, v3);
        SET_BIT(j, 15, v3);
        crc_tbl[i] = j;
    }
    printf("static const quint16 crc_tbl[16] = {\n");
    for (int i = 0; i < 16; i +=4)
        printf("    0x%04x, 0x%04x, 0x%04x, 0x%04x,\n", crc_tbl[i], crc_tbl[i+1], crc_tbl[i+2], crc_tbl[i+3]);
    printf("};\n");
}
#endif

static const quint16 crc_tbl[16] = {
    0x0000, 0x1081, 0x2102, 0x3183,
    0x4204, 0x5285, 0x6306, 0x7387,
    0x8408, 0x9489, 0xa50a, 0xb58b,
    0xc60c, 0xd68d, 0xe70e, 0xf78f
};

/*!
    \relates QByteArray
    \since 5.9

    Returns the CRC-16 checksum of \a data.

    The checksum is independent of the byte order (endianness) and will
    be calculated accorded to the algorithm published in \a standard.
    By default the algorithm published in ISO 3309 (Qt::ChecksumIso3309) is used.

    \note This function is a 16-bit cache conserving (16 entry table)
    implementation of the CRC-16-CCITT algorithm.
*/
quint16 qChecksum(QByteArrayView data, Qt::ChecksumType standard)
{
    quint16 crc = 0x0000;
    switch (standard) {
    case Qt::ChecksumIso3309:
        crc = 0xffff;
        break;
    case Qt::ChecksumItuV41:
        crc = 0x6363;
        break;
    }
    uchar c;
    const uchar *p = reinterpret_cast<const uchar *>(data.data());
    qsizetype len = data.size();
    while (len--) {
        c = *p++;
        crc = ((crc >> 4) & 0x0fff) ^ crc_tbl[((crc ^ c) & 15)];
        c >>= 4;
        crc = ((crc >> 4) & 0x0fff) ^ crc_tbl[((crc ^ c) & 15)];
    }
    switch (standard) {
    case Qt::ChecksumIso3309:
        crc = ~crc;
        break;
    case Qt::ChecksumItuV41:
        break;
    }
    return crc & 0xffff;
}

/*!
    \fn QByteArray qCompress(const QByteArray& data, int compressionLevel)

    \relates QByteArray

    Compresses the \a data byte array and returns the compressed data
    in a new byte array.

    The \a compressionLevel parameter specifies how much compression
    should be used. Valid values are between 0 and 9, with 9
    corresponding to the greatest compression (i.e. smaller compressed
    data) at the cost of using a slower algorithm. Smaller values (8,
    7, ..., 1) provide successively less compression at slightly
    faster speeds. The value 0 corresponds to no compression at all.
    The default value is -1, which specifies zlib's default
    compression.

    \sa qUncompress(const QByteArray &data)
*/

/*!
    \fn QByteArray qCompress(const uchar* data, qsizetype nbytes, int compressionLevel)
    \relates QByteArray

    \overload

    Compresses the first \a nbytes of \a data at compression level
    \a compressionLevel and returns the compressed data in a new byte array.
*/

#ifndef QT_NO_COMPRESS
using CompressSizeHint_t = quint32; // 32-bit BE, historically

enum class ZLibOp : bool { Compression, Decompression };

Q_DECL_COLD_FUNCTION
static const char *zlibOpAsString(ZLibOp op)
{
    switch (op) {
    case ZLibOp::Compression: return "qCompress";
    case ZLibOp::Decompression: return "qUncompress";
    }
    Q_UNREACHABLE_RETURN(nullptr);
}

Q_DECL_COLD_FUNCTION
static QByteArray zlibError(ZLibOp op, const char *what)
{
    qWarning("%s: %s", zlibOpAsString(op), what);
    return QByteArray();
}

Q_DECL_COLD_FUNCTION
static QByteArray dataIsNull(ZLibOp op)
{
    return zlibError(op, "Data is null");
}

Q_DECL_COLD_FUNCTION
static QByteArray lengthIsNegative(ZLibOp op)
{
    return zlibError(op, "Input length is negative");
}

Q_DECL_COLD_FUNCTION
static QByteArray tooMuchData(ZLibOp op)
{
    return zlibError(op, "Not enough memory");
}

Q_DECL_COLD_FUNCTION
static QByteArray invalidCompressedData()
{
    return zlibError(ZLibOp::Decompression, "Input data is corrupted");
}

Q_DECL_COLD_FUNCTION
static QByteArray unexpectedZlibError(ZLibOp op, int err, const char *msg)
{
    qWarning("%s unexpected zlib error: %s (%d)",
             zlibOpAsString(op),
             msg ? msg : "",
             err);
    return QByteArray();
}

static QByteArray xxflate(ZLibOp op, QArrayDataPointer<char> out, QByteArrayView input,
                          qxp::function_ref<int(z_stream *) const> init,
                          qxp::function_ref<int(z_stream *, size_t) const> processChunk,
                          qxp::function_ref<void(z_stream *) const> deinit)
{
    if (out.data() == nullptr) // allocation failed
        return tooMuchData(op);
    qsizetype capacity = out.allocatedCapacity();

    const auto initalSize = out.size;

    z_stream zs = {};
    zs.next_in = reinterpret_cast<uchar *>(const_cast<char *>(input.data())); // 1980s C API...
    if (const int err = init(&zs); err != Z_OK)
        return unexpectedZlibError(op, err, zs.msg);
    const auto sg = qScopeGuard([&] { deinit(&zs); });

    using ZlibChunkSize_t = decltype(zs.avail_in);
    static_assert(!std::is_signed_v<ZlibChunkSize_t>);
    static_assert(std::is_same_v<ZlibChunkSize_t, decltype(zs.avail_out)>);
    constexpr auto MaxChunkSize = std::numeric_limits<ZlibChunkSize_t>::max();
    [[maybe_unused]]
    constexpr auto MaxStatisticsSize = std::numeric_limits<decltype(zs.total_out)>::max();

    size_t inputLeft = size_t(input.size());

    int res;
    do {
        Q_ASSERT(out.freeSpaceAtBegin() == 0); // ensure prepend optimization stays out of the way
        Q_ASSERT(capacity == out.allocatedCapacity());

        if (zs.avail_out == 0) {
            Q_ASSERT(size_t(out.size) - initalSize > MaxStatisticsSize || // total_out overflow
                     size_t(out.size) - initalSize == zs.total_out);
            Q_ASSERT(out.size <= capacity);

            qsizetype avail_out = capacity - out.size;
            if (avail_out == 0) {
                out->reallocateAndGrow(QArrayData::GrowsAtEnd, 1); // grow to next natural capacity
                if (out.data() == nullptr) // reallocation failed
                    return tooMuchData(op);
                capacity = out.allocatedCapacity();
                avail_out = capacity - out.size;
            }
            zs.next_out = reinterpret_cast<uchar *>(out.data()) + out.size;
            zs.avail_out = size_t(avail_out) > size_t(MaxChunkSize) ? MaxChunkSize
                                                                    : ZlibChunkSize_t(avail_out);
            out.size += zs.avail_out;

            Q_ASSERT(zs.avail_out > 0);
        }

        if (zs.avail_in == 0) {
            // zs.next_in is kept up-to-date by processChunk(), so nothing to do
            zs.avail_in = inputLeft > MaxChunkSize ? MaxChunkSize : ZlibChunkSize_t(inputLeft);
            inputLeft -= zs.avail_in;
        }

        res = processChunk(&zs, inputLeft);
    } while (res == Z_OK);

    switch (res) {
    case Z_STREAM_END:
        out.size -= zs.avail_out;
        Q_ASSERT(size_t(out.size) - initalSize > MaxStatisticsSize || // total_out overflow
                 size_t(out.size) - initalSize == zs.total_out);
        Q_ASSERT(out.size <= out.allocatedCapacity());
        out.data()[out.size] = '\0';
        return QByteArray(std::move(out));

    case Z_MEM_ERROR:
        return tooMuchData(op);

    case Z_BUF_ERROR:
        Q_UNREACHABLE(); // cannot happen - we supply a buffer that can hold the result,
                         // or else error out early

    case Z_DATA_ERROR:   // can only happen on decompression
        Q_ASSERT(op == ZLibOp::Decompression);
        return invalidCompressedData();

    default:
        return unexpectedZlibError(op, res, zs.msg);
    }
}

QByteArray qCompress(const uchar* data, qsizetype nbytes, int compressionLevel)
{
    constexpr qsizetype HeaderSize = sizeof(CompressSizeHint_t);
    if (nbytes == 0) {
        return QByteArray(HeaderSize, '\0');
    }
    if (!data)
        return dataIsNull(ZLibOp::Compression);

    if (nbytes < 0)
        return lengthIsNegative(ZLibOp::Compression);

    if (compressionLevel < -1 || compressionLevel > 9)
        compressionLevel = -1;

    QArrayDataPointer out = [&] {
        constexpr qsizetype SingleAllocLimit = 256 * 1024; // the maximum size for which we use
                                                           // zlib's compressBound() to guarantee
                                                           // the output buffer size is sufficient
                                                           // to hold result
        qsizetype capacity = HeaderSize;
        if (nbytes < SingleAllocLimit) {
            // use maximum size
            capacity += compressBound(uLong(nbytes)); // cannot overflow (both times)!
            return QArrayDataPointer{QTypedArrayData<char>::allocate(capacity)};
        }

        // for larger buffers, assume it compresses optimally, and
        // grow geometrically from there:
        constexpr qsizetype MaxCompressionFactor = 1024; // max theoretical factor is 1032
                                                         // cf. http://www.zlib.org/zlib_tech.html,
                                                         // but use a nearby power-of-two (faster)
        capacity += std::max(qsizetype(compressBound(uLong(SingleAllocLimit))),
                             nbytes / MaxCompressionFactor);
        return QArrayDataPointer{QTypedArrayData<char>::allocate(capacity, QArrayData::Grow)};
    }();

    if (out.data() == nullptr) // allocation failed
      return tooMuchData(ZLibOp::Compression);

    qToBigEndian(qt_saturate<CompressSizeHint_t>(nbytes), out.data());
    out.size = HeaderSize;

    return xxflate(ZLibOp::Compression, std::move(out), {data, nbytes},
                   [=] (z_stream *zs) { return deflateInit(zs, compressionLevel); },
                   [] (z_stream *zs, size_t inputLeft) {
                       return deflate(zs, inputLeft ? Z_NO_FLUSH : Z_FINISH);
                   },
                   [] (z_stream *zs) { deflateEnd(zs); });
}
#endif

/*!
    \fn QByteArray qUncompress(const QByteArray &data)

    \relates QByteArray

    Uncompresses the \a data byte array and returns a new byte array
    with the uncompressed data.

    Returns an empty QByteArray if the input data was corrupt.

    This function will uncompress data compressed with qCompress()
    from this and any earlier Qt version, back to Qt 3.1 when this
    feature was added.

    \b{Note:} If you want to use this function to uncompress external
    data that was compressed using zlib, you first need to prepend a four
    byte header to the byte array containing the data. The header must
    contain the expected length (in bytes) of the uncompressed data,
    expressed as an unsigned, big-endian, 32-bit integer. This number is
    just a hint for the initial size of the output buffer size,
    though. If the indicated size is too small to hold the result, the
    output buffer size will still be increased until either the output
    fits or the system runs out of memory. So, despite the 32-bit
    header, this function, on 64-bit platforms, can produce more than
    4GiB of output.

    \note In Qt versions prior to Qt 6.5, more than 2GiB of data
    worked unreliably; in Qt versions prior to Qt 6.0, not at all.

    \sa qCompress()
*/

#ifndef QT_NO_COMPRESS
/*! \relates QByteArray

    \overload

    Uncompresses the first \a nbytes of \a data and returns a new byte
    array with the uncompressed data.
*/
QByteArray qUncompress(const uchar* data, qsizetype nbytes)
{
    if (!data)
        return dataIsNull(ZLibOp::Decompression);

    if (nbytes < 0)
        return lengthIsNegative(ZLibOp::Decompression);

    constexpr qsizetype HeaderSize = sizeof(CompressSizeHint_t);
    if (nbytes < HeaderSize)
        return invalidCompressedData();

    const auto expectedSize = qFromBigEndian<CompressSizeHint_t>(data);
    if (nbytes == HeaderSize) {
        if (expectedSize != 0)
            return invalidCompressedData();
        return QByteArray();
    }

    constexpr auto MaxDecompressedSize = size_t(MaxByteArraySize);
    if constexpr (MaxDecompressedSize < std::numeric_limits<CompressSizeHint_t>::max()) {
        if (expectedSize > MaxDecompressedSize)
            return tooMuchData(ZLibOp::Decompression);
    }

    // expectedSize may be truncated, so always use at least nbytes
    // (larger by at most 1%, according to zlib docs)
    qsizetype capacity = std::max(qsizetype(expectedSize), // cannot overflow!
                                  nbytes);

    QArrayDataPointer d(QTypedArrayData<char>::allocate(capacity, QArrayData::KeepSize));
    return xxflate(ZLibOp::Decompression, std::move(d), {data + HeaderSize, nbytes - HeaderSize},
                   [] (z_stream *zs) { return inflateInit(zs); },
                   [] (z_stream *zs, size_t) { return inflate(zs, Z_NO_FLUSH); },
                   [] (z_stream *zs) { inflateEnd(zs); });
}
#endif

/*!
    \class QByteArray
    \inmodule QtCore
    \brief The QByteArray class provides an array of bytes.

    \ingroup tools
    \ingroup shared
    \ingroup string-processing

    \reentrant

    QByteArray can be used to store both raw bytes (including '\\0's)
    and traditional 8-bit '\\0'-terminated strings. Using QByteArray
    is much more convenient than using \c{const char *}. Behind the
    scenes, it always ensures that the data is followed by a '\\0'
    terminator, and uses \l{implicit sharing} (copy-on-write) to
    reduce memory usage and avoid needless copying of data.

    In addition to QByteArray, Qt also provides the QString class to store
    string data. For most purposes, QString is the class you want to use. It
    understands its content as Unicode text (encoded using UTF-16) where
    QByteArray aims to avoid assumptions about the encoding or semantics of the
    bytes it stores (aside from a few legacy cases where it uses ASCII).
    Furthermore, QString is used throughout in the Qt API. The two main cases
    where QByteArray is appropriate are when you need to store raw binary data,
    and when memory conservation is critical (e.g., with Qt for Embedded Linux).

    One way to initialize a QByteArray is simply to pass a \c{const
    char *} to its constructor. For example, the following code
    creates a byte array of size 5 containing the data "Hello":

    \snippet code/src_corelib_text_qbytearray.cpp 0

    Although the size() is 5, the byte array also maintains an extra '\\0' byte
    at the end so that if a function is used that asks for a pointer to the
    underlying data (e.g. a call to data()), the data pointed to is guaranteed
    to be '\\0'-terminated.

    QByteArray makes a deep copy of the \c{const char *} data, so you can modify
    it later without experiencing side effects. (If, for example for performance
    reasons, you don't want to take a deep copy of the data, use
    QByteArray::fromRawData() instead.)

    Another approach is to set the size of the array using resize() and to
    initialize the data byte by byte. QByteArray uses 0-based indexes, just like
    C++ arrays. To access the byte at a particular index position, you can use
    operator[](). On non-const byte arrays, operator[]() returns a reference to
    a byte that can be used on the left side of an assignment. For example:

    \snippet code/src_corelib_text_qbytearray.cpp 1

    For read-only access, an alternative syntax is to use at():

    \snippet code/src_corelib_text_qbytearray.cpp 2

    at() can be faster than operator[](), because it never causes a
    \l{deep copy} to occur.

    To extract many bytes at a time, use first(), last(), or sliced().

    A QByteArray can embed '\\0' bytes. The size() function always
    returns the size of the whole array, including embedded '\\0'
    bytes, but excluding the terminating '\\0' added by QByteArray.
    For example:

    \snippet code/src_corelib_text_qbytearray.cpp 48

    If you want to obtain the length of the data up to and excluding the first
    '\\0' byte, call qstrlen() on the byte array.

    After a call to resize(), newly allocated bytes have undefined
    values. To set all the bytes to a particular value, call fill().

    To obtain a pointer to the actual bytes, call data() or constData(). These
    functions return a pointer to the beginning of the data. The pointer is
    guaranteed to remain valid until a non-const function is called on the
    QByteArray. It is also guaranteed that the data ends with a '\\0' byte
    unless the QByteArray was created from \l{fromRawData()}{raw data}. This
    '\\0' byte is automatically provided by QByteArray and is not counted in
    size().

    QByteArray provides the following basic functions for modifying
    the byte data: append(), prepend(), insert(), replace(), and
    remove(). For example:

    \snippet code/src_corelib_text_qbytearray.cpp 3

    In the above example the replace() function's first two arguments are the
    position from which to start replacing and the number of bytes that
    should be replaced.

    When data-modifying functions increase the size of the array,
    they may lead to reallocation of memory for the QByteArray object. When
    this happens, QByteArray expands by more than it immediately needs so as
    to have space for further expansion without reallocation until the size
    of the array has greatly increased.

    The insert(), remove() and, when replacing a sub-array with one of
    different size, replace() functions can be slow (\l{linear time}) for
    large arrays, because they require moving many bytes in the array by
    at least one position in memory.

    If you are building a QByteArray gradually and know in advance
    approximately how many bytes the QByteArray will contain, you
    can call reserve(), asking QByteArray to preallocate a certain amount
    of memory. You can also call capacity() to find out how much
    memory the QByteArray actually has allocated.

    Note that using non-const operators and functions can cause
    QByteArray to do a deep copy of the data, due to \l{implicit sharing}.

    QByteArray provides \l{STL-style iterators} (QByteArray::const_iterator and
    QByteArray::iterator). In practice, iterators are handy when working with
    generic algorithms provided by the C++ standard library.

    \note Iterators and references to individual QByteArray elements are subject
    to stability issues. They are often invalidated when a QByteArray-modifying
    operation (e.g. insert() or remove()) is called. When stability and
    iterator-like functionality is required, you should use indexes instead of
    iterators as they are not tied to QByteArray's internal state and thus do
    not get invalidated.

    \note Iterators over a QByteArray, and references to individual bytes
    within one, cannot be relied on to remain valid when any non-const method
    of the QByteArray is called. Accessing such an iterator or reference after
    the call to a non-const method leads to undefined behavior. When stability
    for iterator-like functionality is required, you should use indexes instead
    of iterators as they are not tied to QByteArray's internal state and thus do
    not get invalidated.

    If you want to find all occurrences of a particular byte or sequence of
    bytes in a QByteArray, use indexOf() or lastIndexOf(). The former searches
    forward starting from a given index position, the latter searches
    backward. Both return the index position of the byte sequence if they find
    it; otherwise, they return -1. For example, here's a typical loop that finds
    all occurrences of a particular string:

    \snippet code/src_corelib_text_qbytearray.cpp 4

    If you simply want to check whether a QByteArray contains a particular byte
    sequence, use contains(). If you want to find out how many times a
    particular byte sequence occurs in the byte array, use count(). If you want
    to replace all occurrences of a particular value with another, use one of
    the two-parameter replace() overloads.

    \l{QByteArray}s can be compared using overloaded operators such as
    operator<(), operator<=(), operator==(), operator>=(), and so on. The
    comparison is based exclusively on the numeric values of the bytes and is
    very fast, but is not what a human would
    expect. QString::localeAwareCompare() is a better choice for sorting
    user-interface strings.

    For historical reasons, QByteArray distinguishes between a null
    byte array and an empty byte array. A \e null byte array is a
    byte array that is initialized using QByteArray's default
    constructor or by passing (const char *)0 to the constructor. An
    \e empty byte array is any byte array with size 0. A null byte
    array is always empty, but an empty byte array isn't necessarily
    null:

    \snippet code/src_corelib_text_qbytearray.cpp 5

    All functions except isNull() treat null byte arrays the same as empty byte
    arrays. For example, data() returns a valid pointer (\e not nullptr) to a
    '\\0' byte for a null byte array and QByteArray() compares equal to
    QByteArray(""). We recommend that you always use isEmpty() and avoid
    isNull().

    \section1 Maximum size and out-of-memory conditions

    The maximum size of QByteArray depends on the architecture. Most 64-bit
    systems can allocate more than 2 GB of memory, with a typical limit
    of 2^63 bytes. The actual value also depends on the overhead required for
    managing the data block. As a result, you can expect the maximum size
    of 2 GB minus overhead on 32-bit platforms, and 2^63 bytes minus overhead
    on 64-bit platforms. The number of elements that can be stored in a
    QByteArray is this maximum size.

    When memory allocation fails, QByteArray throws a \c std::bad_alloc
    exception if the application is being compiled with exception support.
    Out of memory conditions in Qt containers are the only case where Qt
    will throw exceptions. If exceptions are disabled, then running out of
    memory is undefined behavior.

    Note that the operating system may impose further limits on applications
    holding a lot of allocated memory, especially large, contiguous blocks.
    Such considerations, the configuration of such behavior or any mitigation
    are outside the scope of the QByteArray API.

    \section1 C locale and ASCII functions

    QByteArray generally handles data as bytes, without presuming any semantics;
    where it does presume semantics, it uses the C locale and ASCII encoding.
    Standard Unicode encodings are supported by QString, other encodings may be
    supported using QStringEncoder and QStringDecoder to convert to Unicode. For
    locale-specific interpretation of text, use QLocale or QString.

    \section2 C Strings

    Traditional C strings, also known as '\\0'-terminated strings, are sequences
    of bytes, specified by a start-point and implicitly including each byte up
    to, but not including, the first '\\0' byte thereafter. Methods that accept
    such a pointer, without a length, will interpret it as this sequence of
    bytes. Such a sequence, by construction, cannot contain a '\\0' byte.

    Other overloads accept a start-pointer and a byte-count; these use the given
    number of bytes, following the start address, regardless of whether any of
    them happen to be '\\0' bytes. In some cases, where there is no overload
    taking only a pointer, passing a length of -1 will cause the method to use
    the offset of the first '\\0' byte after the pointer as the length; a length
    of -1 should only be passed if the method explicitly says it does this (in
    which case it is typically a default argument).

    \section2 Spacing Characters

    A frequent requirement is to remove spacing characters from a byte array
    (\c{'\n'}, \c{'\t'}, \c{' '}, etc.). If you want to remove spacing from both
    ends of a QByteArray, use trimmed(). If you want to also replace each run of
    spacing characters with a single space character within the byte array, use
    simplified(). Only ASCII spacing characters are recognized for these
    purposes.

    \section2 Number-String Conversions

    Functions that perform conversions between numeric data types and string
    representations are performed in the C locale, regardless of the user's
    locale settings. Use QLocale to perform locale-aware conversions between
    numbers and strings.

    \section2 Character Case

    In QByteArray, the notion of uppercase and lowercase and of case-independent
    comparison is limited to ASCII. Non-ASCII characters are treated as
    caseless, since their case depends on encoding. This affects functions that
    support a case insensitive option or that change the case of their
    arguments. Functions that this affects include compare(), isLower(),
    isUpper(), toLower() and toUpper().

    This issue does not apply to \l{QString}s since they represent characters
    using Unicode.

    \sa QByteArrayView, QString, QBitArray
*/

/*!
    \enum QByteArray::Base64Option
    \since 5.2

    This enum contains the options available for encoding and decoding Base64.
    Base64 is defined by \l{RFC 4648}, with the following options:

    \value Base64Encoding     (default) The regular Base64 alphabet, called simply "base64"
    \value Base64UrlEncoding  An alternate alphabet, called "base64url", which replaces two
                              characters in the alphabet to be more friendly to URLs.
    \value KeepTrailingEquals (default) Keeps the trailing padding equal signs at the end
                              of the encoded data, so the data is always a size multiple of
                              four.
    \value OmitTrailingEquals Omits adding the padding equal signs at the end of the encoded
                              data.
    \value IgnoreBase64DecodingErrors  When decoding Base64-encoded data, ignores errors
                                       in the input; invalid characters are simply skipped.
                                       This enum value has been added in Qt 5.15.
    \value AbortOnBase64DecodingErrors When decoding Base64-encoded data, stops at the first
                                       decoding error.
                                       This enum value has been added in Qt 5.15.

    QByteArray::fromBase64Encoding() and QByteArray::fromBase64()
    ignore the KeepTrailingEquals and OmitTrailingEquals options. If
    the IgnoreBase64DecodingErrors option is specified, they will not
    flag errors in case trailing equal signs are missing or if there
    are too many of them. If instead the AbortOnBase64DecodingErrors is
    specified, then the input must either have no padding or have the
    correct amount of equal signs.
*/

/*! \fn QByteArray::iterator QByteArray::begin()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the first
    byte in the byte-array.

//! [iterator-invalidation-func-desc]
    \warning The returned iterator is invalidated on detachment or when the
    QByteArray is modified.
//! [iterator-invalidation-func-desc]

    \sa constBegin(), end()
*/

/*! \fn QByteArray::const_iterator QByteArray::begin() const

    \overload begin()
*/

/*! \fn QByteArray::const_iterator QByteArray::cbegin() const
    \since 5.0

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the
    first byte in the byte-array.

    \include qbytearray.cpp iterator-invalidation-func-desc

    \sa begin(), cend()
*/

/*! \fn QByteArray::const_iterator QByteArray::constBegin() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the
    first byte in the byte-array.

    \include qbytearray.cpp iterator-invalidation-func-desc

    \sa begin(), constEnd()
*/

/*! \fn QByteArray::iterator QByteArray::end()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing just after
    the last byte in the byte-array.

    \include qbytearray.cpp iterator-invalidation-func-desc

    \sa begin(), constEnd()
*/

/*! \fn QByteArray::const_iterator QByteArray::end() const

    \overload end()
*/

/*! \fn QByteArray::const_iterator QByteArray::cend() const
    \since 5.0

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing just
    after the last byte in the byte-array.

    \include qbytearray.cpp iterator-invalidation-func-desc

    \sa cbegin(), end()
*/

/*! \fn QByteArray::const_iterator QByteArray::constEnd() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing just
    after the last byte in the byte-array.

    \include qbytearray.cpp iterator-invalidation-func-desc

    \sa constBegin(), end()
*/

/*! \fn QByteArray::reverse_iterator QByteArray::rbegin()
    \since 5.6

    Returns a \l{STL-style iterators}{STL-style} reverse iterator pointing to
    the first byte in the byte-array, in reverse order.

    \include qbytearray.cpp iterator-invalidation-func-desc

    \sa begin(), crbegin(), rend()
*/

/*! \fn QByteArray::const_reverse_iterator QByteArray::rbegin() const
    \since 5.6
    \overload
*/

/*! \fn QByteArray::const_reverse_iterator QByteArray::crbegin() const
    \since 5.6

    Returns a const \l{STL-style iterators}{STL-style} reverse iterator pointing
    to the first byte in the byte-array, in reverse order.

    \include qbytearray.cpp iterator-invalidation-func-desc

    \sa begin(), rbegin(), rend()
*/

/*! \fn QByteArray::reverse_iterator QByteArray::rend()
    \since 5.6

    Returns a \l{STL-style iterators}{STL-style} reverse iterator pointing just
    after the last byte in the byte-array, in reverse order.

    \include qbytearray.cpp iterator-invalidation-func-desc

    \sa end(), crend(), rbegin()
*/

/*! \fn QByteArray::const_reverse_iterator QByteArray::rend() const
    \since 5.6
    \overload
*/

/*! \fn QByteArray::const_reverse_iterator QByteArray::crend() const
    \since 5.6

    Returns a const \l{STL-style iterators}{STL-style} reverse iterator pointing
    just after the last byte in the byte-array, in reverse order.

    \include qbytearray.cpp iterator-invalidation-func-desc

    \sa end(), rend(), rbegin()
*/

/*! \fn void QByteArray::push_back(const QByteArray &other)

    This function is provided for STL compatibility. It is equivalent
    to append(\a other).
*/

/*! \fn void QByteArray::push_back(QByteArrayView str)
    \since 6.0
    \overload

    Same as append(\a str).
*/

/*! \fn void QByteArray::push_back(const char *str)

    \overload

    Same as append(\a str).
*/

/*! \fn void QByteArray::push_back(char ch)

    \overload

    Same as append(\a ch).
*/

/*! \fn void QByteArray::push_front(const QByteArray &other)

    This function is provided for STL compatibility. It is equivalent
    to prepend(\a other).
*/

/*! \fn void QByteArray::push_front(QByteArrayView str)
    \since 6.0
    \overload

    Same as prepend(\a str).
*/

/*! \fn void QByteArray::push_front(const char *str)

    \overload

    Same as prepend(\a str).
*/

/*! \fn void QByteArray::push_front(char ch)

    \overload

    Same as prepend(\a ch).
*/

/*! \fn void QByteArray::shrink_to_fit()
    \since 5.10

    This function is provided for STL compatibility. It is equivalent to
    squeeze().
*/

/*!
    \since 6.1

    Removes from the byte array the characters in the half-open range
    [ \a first , \a last ). Returns an iterator to the character
    referred to by \a last before the erase.
*/
QByteArray::iterator QByteArray::erase(QByteArray::const_iterator first, QByteArray::const_iterator last)
{
    const auto start = std::distance(cbegin(), first);
    const auto len = std::distance(first, last);
    remove(start, len);
    return begin() + start;
}

/*!
    \fn QByteArray::iterator QByteArray::erase(QByteArray::const_iterator it)

    \since 6.5

    Removes the character denoted by \c it from the byte array.
    Returns an iterator to the character immediately after the
    erased character.

    \code
    QByteArray ba = "abcdefg";
    auto it = ba.erase(ba.cbegin()); // ba is now "bcdefg" and it points to "b"
    \endcode
*/

/*! \fn QByteArray::QByteArray(const QByteArray &other)

    Constructs a copy of \a other.

    This operation takes \l{constant time}, because QByteArray is
    \l{implicitly shared}. This makes returning a QByteArray from a
    function very fast. If a shared instance is modified, it will be
    copied (copy-on-write), taking \l{linear time}.

    \sa operator=()
*/

/*!
    \fn QByteArray::QByteArray(QByteArray &&other)

    Move-constructs a QByteArray instance, making it point at the same
    object that \a other was pointing to.

    \since 5.2
*/

/*! \fn QByteArray::QByteArray(QByteArrayDataPtr dd)

    \internal

    Constructs a byte array pointing to the same data as \a dd.
*/

/*! \fn QByteArray::~QByteArray()
    Destroys the byte array.
*/

/*!
    Assigns \a other to this byte array and returns a reference to
    this byte array.
*/
QByteArray &QByteArray::operator=(const QByteArray & other) noexcept
{
    d = other.d;
    return *this;
}


/*!
    \overload

    Assigns \a str to this byte array.
*/

QByteArray &QByteArray::operator=(const char *str)
{
    if (!str) {
        d.clear();
    } else if (!*str) {
        d = DataPointer::fromRawData(&_empty, 0);
    } else {
        const qsizetype len = qsizetype(strlen(str));
        const auto capacityAtEnd = d->allocatedCapacity() - d.freeSpaceAtBegin();
        if (d->needsDetach() || len > capacityAtEnd
                || (len < size() && len < (capacityAtEnd >> 1)))
            // ### inefficient! reallocData() does copy the old data and we then overwrite it in the next line
            reallocData(len, QArrayData::KeepSize);
        memcpy(d.data(), str, len + 1); // include null terminator
        d.size = len;
    }
    return *this;
}

/*!
    \fn QByteArray &QByteArray::operator=(QByteArray &&other)

    Move-assigns \a other to this QByteArray instance.

    \since 5.2
*/

/*! \fn void QByteArray::swap(QByteArray &other)
    \since 4.8

    Swaps byte array \a other with this byte array. This operation is very
    fast and never fails.
*/

/*! \fn qsizetype QByteArray::size() const

    Returns the number of bytes in this byte array.

    The last byte in the byte array is at position size() - 1. In addition,
    QByteArray ensures that the byte at position size() is always '\\0', so that
    you can use the return value of data() and constData() as arguments to
    functions that expect '\\0'-terminated strings. If the QByteArray object was
    created from a \l{fromRawData()}{raw data} that didn't include the trailing
    '\\0'-termination byte, then QByteArray doesn't add it automatically unless a
    \l{deep copy} is created.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 6

    \sa isEmpty(), resize()
*/

/*! \fn bool QByteArray::isEmpty() const

    Returns \c true if the byte array has size 0; otherwise returns \c false.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 7

    \sa size()
*/

/*! \fn qsizetype QByteArray::capacity() const

    Returns the maximum number of bytes that can be stored in the
    byte array without forcing a reallocation.

    The sole purpose of this function is to provide a means of fine
    tuning QByteArray's memory usage. In general, you will rarely
    ever need to call this function. If you want to know how many
    bytes are in the byte array, call size().

    \note a statically allocated byte array will report a capacity of 0,
    even if it's not empty.

    \note The free space position in the allocated memory block is undefined. In
    other words, one should not assume that the free memory is always located
    after the initialized elements.

    \sa reserve(), squeeze()
*/

/*! \fn void QByteArray::reserve(qsizetype size)

    Attempts to allocate memory for at least \a size bytes.

    If you know in advance how large the byte array will be, you can call
    this function, and if you call resize() often you are likely to
    get better performance.

    If in doubt about how much space shall be needed, it is usually better to
    use an upper bound as \a size, or a high estimate of the most likely size,
    if a strict upper bound would be much bigger than this. If \a size is an
    underestimate, the array will grow as needed once the reserved size is
    exceeded, which may lead to a larger allocation than your best overestimate
    would have and will slow the operation that triggers it.

    \warning reserve() reserves memory but does not change the size of the byte
    array. Accessing data beyond the end of the byte array is undefined
    behavior. If you need to access memory beyond the current end of the array,
    use resize().

    The sole purpose of this function is to provide a means of fine
    tuning QByteArray's memory usage. In general, you will rarely
    ever need to call this function.

    \sa squeeze(), capacity()
*/

/*! \fn void QByteArray::squeeze()

    Releases any memory not required to store the array's data.

    The sole purpose of this function is to provide a means of fine
    tuning QByteArray's memory usage. In general, you will rarely
    ever need to call this function.

    \sa reserve(), capacity()
*/

/*! \fn QByteArray::operator const char *() const
    \fn QByteArray::operator const void *() const

    \note Use constData() instead in new code.

    Returns a pointer to the data stored in the byte array. The
    pointer can be used to access the bytes that compose the array.
    The data is '\\0'-terminated.

//! [pointer-invalidation-desc]
    The pointer remains valid as long as no detach happens and the QByteArray
    is not modified.
//! [pointer-invalidation-desc]

    This operator is mostly useful to pass a byte array to a function
    that accepts a \c{const char *}.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_BYTEARRAY when you compile your applications.

    Note: A QByteArray can store any byte values including '\\0's,
    but most functions that take \c{char *} arguments assume that the
    data ends at the first '\\0' they encounter.

    \sa constData()
*/

/*!
  \macro QT_NO_CAST_FROM_BYTEARRAY
  \relates QByteArray

  Disables automatic conversions from QByteArray to
  const char * or const void *.

  \sa QT_NO_CAST_TO_ASCII, QT_NO_CAST_FROM_ASCII
*/

/*! \fn char *QByteArray::data()

    Returns a pointer to the data stored in the byte array. The pointer can be
    used to access and modify the bytes that compose the array. The data is
    '\\0'-terminated, i.e. the number of bytes you can access following the
    returned pointer is size() + 1, including the '\\0' terminator.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 8

    \include qbytearray.cpp pointer-invalidation-desc

    For read-only access, constData() is faster because it never
    causes a \l{deep copy} to occur.

    This function is mostly useful to pass a byte array to a function
    that accepts a \c{const char *}.

    The following example makes a copy of the char* returned by
    data(), but it will corrupt the heap and cause a crash because it
    does not allocate a byte for the '\\0' at the end:

    \snippet code/src_corelib_text_qbytearray.cpp 46

    This one allocates the correct amount of space:

    \snippet code/src_corelib_text_qbytearray.cpp 47

    Note: A QByteArray can store any byte values including '\\0's,
    but most functions that take \c{char *} arguments assume that the
    data ends at the first '\\0' they encounter.

    \sa constData(), operator[]()
*/

/*! \fn const char *QByteArray::data() const

    \overload
*/

/*! \fn const char *QByteArray::constData() const

    Returns a pointer to the const data stored in the byte array. The pointer
    can be used to access the bytes that compose the array. The data is
    '\\0'-terminated unless the QByteArray object was created from raw data.

    \include qbytearray.cpp pointer-invalidation-desc

    This function is mostly useful to pass a byte array to a function
    that accepts a \c{const char *}.

    Note: A QByteArray can store any byte values including '\\0's,
    but most functions that take \c{char *} arguments assume that the
    data ends at the first '\\0' they encounter.

    \sa data(), operator[](), fromRawData()
*/

/*! \fn void QByteArray::detach()

    \internal
*/

/*! \fn bool QByteArray::isDetached() const

    \internal
*/

/*! \fn bool QByteArray::isSharedWith(const QByteArray &other) const

    \internal
*/

/*! \fn char QByteArray::at(qsizetype i) const

    Returns the byte at index position \a i in the byte array.

    \a i must be a valid index position in the byte array (i.e., 0 <=
    \a i < size()).

    \sa operator[]()
*/

/*! \fn char &QByteArray::operator[](qsizetype i)

    Returns the byte at index position \a i as a modifiable reference.

    \a i must be a valid index position in the byte array (i.e., 0 <=
    \a i < size()).

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 9

    \sa at()
*/

/*! \fn char QByteArray::operator[](qsizetype i) const

    \overload

    Same as at(\a i).
*/

/*!
    \fn char QByteArray::front() const
    \since 5.10

    Returns the first byte in the byte array.
    Same as \c{at(0)}.

    This function is provided for STL compatibility.

    \warning Calling this function on an empty byte array constitutes
    undefined behavior.

    \sa back(), at(), operator[]()
*/

/*!
    \fn char QByteArray::back() const
    \since 5.10

    Returns the last byte in the byte array.
    Same as \c{at(size() - 1)}.

    This function is provided for STL compatibility.

    \warning Calling this function on an empty byte array constitutes
    undefined behavior.

    \sa front(), at(), operator[]()
*/

/*!
    \fn char &QByteArray::front()
    \since 5.10

    Returns a reference to the first byte in the byte array.
    Same as \c{operator[](0)}.

    This function is provided for STL compatibility.

    \warning Calling this function on an empty byte array constitutes
    undefined behavior.

    \sa back(), at(), operator[]()
*/

/*!
    \fn char &QByteArray::back()
    \since 5.10

    Returns a reference to the last byte in the byte array.
    Same as \c{operator[](size() - 1)}.

    This function is provided for STL compatibility.

    \warning Calling this function on an empty byte array constitutes
    undefined behavior.

    \sa front(), at(), operator[]()
*/

/*! \fn bool QByteArray::contains(QByteArrayView bv) const
    \since 6.0

    Returns \c true if this byte array contains an occurrence of the
    sequence of bytes viewed by \a bv; otherwise returns \c false.

    \sa indexOf(), count()
*/

/*! \fn bool QByteArray::contains(char ch) const

    \overload

    Returns \c true if the byte array contains the byte \a ch;
    otherwise returns \c false.
*/

/*!

    Truncates the byte array at index position \a pos.

    If \a pos is beyond the end of the array, nothing happens.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 10

    \sa chop(), resize(), first()
*/
void QByteArray::truncate(qsizetype pos)
{
    if (pos < size())
        resize(pos);
}

/*!

    Removes \a n bytes from the end of the byte array.

    If \a n is greater than size(), the result is an empty byte
    array.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 11

    \sa truncate(), resize(), first()
*/

void QByteArray::chop(qsizetype n)
{
    if (n > 0)
        resize(size() - n);
}


/*! \fn QByteArray &QByteArray::operator+=(const QByteArray &ba)

    Appends the byte array \a ba onto the end of this byte array and
    returns a reference to this byte array.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 12

    Note: QByteArray is an \l{implicitly shared} class. Consequently,
    if you append to an empty byte array, then the byte array will just
    share the data held in \a ba. In this case, no copying of data is done,
    taking \l{constant time}. If a shared instance is modified, it will
    be copied (copy-on-write), taking \l{linear time}.

    If the byte array being appended to is not empty, a deep copy of the
    data is performed, taking \l{linear time}.

    This operation typically does not suffer from allocation overhead,
    because QByteArray preallocates extra space at the end of the data
    so that it may grow without reallocating for each append operation.

    \sa append(), prepend()
*/

/*! \fn QByteArray &QByteArray::operator+=(const char *str)

    \overload

    Appends the '\\0'-terminated string \a str onto the end of this byte array
    and returns a reference to this byte array.
*/

/*! \fn QByteArray &QByteArray::operator+=(char ch)

    \overload

    Appends the byte \a ch onto the end of this byte array and returns a
    reference to this byte array.
*/

/*! \fn qsizetype QByteArray::length() const

    Same as size().
*/

/*! \fn bool QByteArray::isNull() const

    Returns \c true if this byte array is null; otherwise returns \c false.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 13

    Qt makes a distinction between null byte arrays and empty byte
    arrays for historical reasons. For most applications, what
    matters is whether or not a byte array contains any data,
    and this can be determined using isEmpty().

    \sa isEmpty()
*/

/*! \fn QByteArray::QByteArray()

    Constructs an empty byte array.

    \sa isEmpty()
*/

/*!
    Constructs a byte array containing the first \a size bytes of
    array \a data.

    If \a data is 0, a null byte array is constructed.

    If \a size is negative, \a data is assumed to point to a '\\0'-terminated
    string and its length is determined dynamically.

    QByteArray makes a deep copy of the string data.

    \sa fromRawData()
*/

QByteArray::QByteArray(const char *data, qsizetype size)
{
    if (!data) {
        d = DataPointer();
    } else {
        if (size < 0)
            size = qstrlen(data);
        if (!size) {
            d = DataPointer::fromRawData(&_empty, 0);
        } else {
            d = DataPointer(Data::allocate(size), size);
            Q_CHECK_PTR(d.data());
            memcpy(d.data(), data, size);
            d.data()[size] = '\0';
        }
    }
}

/*!
    Constructs a byte array of size \a size with every byte set to \a ch.

    \sa fill()
*/

QByteArray::QByteArray(qsizetype size, char ch)
{
    if (size <= 0) {
        d = DataPointer::fromRawData(&_empty, 0);
    } else {
        d = DataPointer(Data::allocate(size), size);
        Q_CHECK_PTR(d.data());
        memset(d.data(), ch, size);
        d.data()[size] = '\0';
    }
}

/*!
    Constructs a byte array of size \a size with uninitialized contents.
*/

QByteArray::QByteArray(qsizetype size, Qt::Initialization)
{
    if (size <= 0) {
        d = DataPointer::fromRawData(&_empty, 0);
    } else {
        d = DataPointer(Data::allocate(size), size);
        Q_CHECK_PTR(d.data());
        d.data()[size] = '\0';
    }
}

/*!
    Sets the size of the byte array to \a size bytes.

    If \a size is greater than the current size, the byte array is
    extended to make it \a size bytes with the extra bytes added to
    the end. The new bytes are uninitialized.

    If \a size is less than the current size, bytes beyond position
    \a size are excluded from the byte array.

    \note While resize() will grow the capacity if needed, it never shrinks
    capacity. To shed excess capacity, use squeeze().

    \sa size(), truncate(), squeeze()
*/
void QByteArray::resize(qsizetype size)
{
    if (size < 0)
        size = 0;

    const auto capacityAtEnd = capacity() - d.freeSpaceAtBegin();
    if (d->needsDetach() || size > capacityAtEnd)
        reallocData(size, QArrayData::Grow);
    d.size = size;
    if (d->allocatedCapacity())
        d.data()[size] = 0;
}

/*!
    \since 6.4

    Sets the size of the byte array to \a newSize bytes.

    If \a newSize is greater than the current size, the byte array is
    extended to make it \a newSize bytes with the extra bytes added to
    the end. The new bytes are initialized to \a c.

    If \a newSize is less than the current size, bytes beyond position
    \a newSize are excluded from the byte array.

    \note While resize() will grow the capacity if needed, it never shrinks
    capacity. To shed excess capacity, use squeeze().

    \sa size(), truncate(), squeeze()
*/
void QByteArray::resize(qsizetype newSize, char c)
{
    const auto old = d.size;
    resize(newSize);
    if (old < d.size)
        memset(d.data() + old, c, d.size - old);
}

/*!
    Sets every byte in the byte array to \a ch. If \a size is different from -1
    (the default), the byte array is resized to size \a size beforehand.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 14

    \sa resize()
*/

QByteArray &QByteArray::fill(char ch, qsizetype size)
{
    resize(size < 0 ? this->size() : size);
    if (this->size())
        memset(d.data(), ch, this->size());
    return *this;
}

void QByteArray::reallocData(qsizetype alloc, QArrayData::AllocationOption option)
{
    if (!alloc) {
        d = DataPointer::fromRawData(&_empty, 0);
        return;
    }

    // don't use reallocate path when reducing capacity and there's free space
    // at the beginning: might shift data pointer outside of allocated space
    const bool cannotUseReallocate = d.freeSpaceAtBegin() > 0;

    if (d->needsDetach() || cannotUseReallocate) {
        DataPointer dd(Data::allocate(alloc, option), qMin(alloc, d.size));
        Q_CHECK_PTR(dd.data());
        if (dd.size > 0)
            ::memcpy(dd.data(), d.data(), dd.size);
        dd.data()[dd.size] = 0;
        d = dd;
    } else {
        d->reallocate(alloc, option);
    }
}

void QByteArray::reallocGrowData(qsizetype n)
{
    if (!n)  // expected to always allocate
        n = 1;

    if (d->needsDetach()) {
        DataPointer dd(DataPointer::allocateGrow(d, n, QArrayData::GrowsAtEnd));
        Q_CHECK_PTR(dd.data());
        dd->copyAppend(d.data(), d.data() + d.size);
        dd.data()[dd.size] = 0;
        d = dd;
    } else {
        d->reallocate(d.constAllocatedCapacity() + n, QArrayData::Grow);
    }
}

void QByteArray::expand(qsizetype i)
{
    resize(qMax(i + 1, size()));
}

/*!
    \fn QByteArray &QByteArray::prepend(QByteArrayView ba)

    Prepends the byte array view \a ba to this byte array and returns a
    reference to this byte array.

    This operation is typically very fast (\l{constant time}), because
    QByteArray preallocates extra space at the beginning of the data,
    so it can grow without reallocating the entire array each time.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 15

    This is the same as insert(0, \a ba).

    \sa append(), insert()
*/

/*!
    \fn QByteArray &QByteArray::prepend(const QByteArray &ba)
    \overload

    Prepends \a ba to this byte array.
*/
QByteArray &QByteArray::prepend(const QByteArray &ba)
{
    if (size() == 0 && ba.size() > d.constAllocatedCapacity() && ba.d.isMutable())
        return (*this = ba);
    return prepend(QByteArrayView(ba));
}

/*!
    \fn QByteArray &QByteArray::prepend(const char *str)
    \overload

    Prepends the '\\0'-terminated string \a str to this byte array.
*/

/*!
    \fn QByteArray &QByteArray::prepend(const char *str, qsizetype len)
    \overload
    \since 4.6

    Prepends \a len bytes starting at \a str to this byte array.
    The bytes prepended may include '\\0' bytes.
*/

/*! \fn QByteArray &QByteArray::prepend(qsizetype count, char ch)

    \overload
    \since 5.7

    Prepends \a count copies of byte \a ch to this byte array.
*/

/*!
    \fn QByteArray &QByteArray::prepend(char ch)
    \overload

    Prepends the byte \a ch to this byte array.
*/

/*!
    Appends the byte array \a ba onto the end of this byte array.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 16

    This is the same as insert(size(), \a ba).

    Note: QByteArray is an \l{implicitly shared} class. Consequently,
    if you append to an empty byte array, then the byte array will just
    share the data held in \a ba. In this case, no copying of data is done,
    taking \l{constant time}. If a shared instance is modified, it will
    be copied (copy-on-write), taking \l{linear time}.

    If the byte array being appended to is not empty, a deep copy of the
    data is performed, taking \l{linear time}.

    The append() function is typically very fast (\l{constant time}),
    because QByteArray preallocates extra space at the end of the data,
    so it can grow without reallocating the entire array each time.

    \sa operator+=(), prepend(), insert()
*/

QByteArray &QByteArray::append(const QByteArray &ba)
{
    if (size() == 0 && ba.size() > d->freeSpaceAtEnd() && ba.d.isMutable())
        return (*this = ba);
    return append(QByteArrayView(ba));
}

/*!
    \fn QByteArray &QByteArray::append(QByteArrayView data)
    \overload

    Appends \a data to this byte array.
*/

/*!
    \fn QByteArray& QByteArray::append(const char *str)
    \overload

    Appends the '\\0'-terminated string \a str to this byte array.
*/

/*!
    \fn QByteArray &QByteArray::append(const char *str, qsizetype len)
    \overload

    Appends the first \a len bytes starting at \a str to this byte array and
    returns a reference to this byte array. The bytes appended may include '\\0'
    bytes.

    If \a len is negative, \a str will be assumed to be a '\\0'-terminated
    string and the length to be copied will be determined automatically using
    qstrlen().

    If \a len is zero or \a str is null, nothing is appended to the byte
    array. Ensure that \a len is \e not longer than \a str.
*/

/*! \fn QByteArray &QByteArray::append(qsizetype count, char ch)

    \overload
    \since 5.7

    Appends \a count copies of byte \a ch to this byte array and returns a
    reference to this byte array.

    If \a count is negative or zero nothing is appended to the byte array.
*/

/*!
    \overload

    Appends the byte \a ch to this byte array.
*/

QByteArray& QByteArray::append(char ch)
{
    d.detachAndGrow(QArrayData::GrowsAtEnd, 1, nullptr, nullptr);
    d->copyAppend(1, ch);
    d.data()[d.size] = '\0';
    return *this;
}

/*!
    \fn QByteArray &QByteArray::assign(QByteArrayView v)
    \since 6.6

    Replaces the contents of this byte array with a copy of \a v and returns a
    reference to this byte array.

    The size of this byte array will be equal to the size of \a v.

    This function only allocates memory if the size of \a v exceeds the capacity
    of this byte array or this byte array is shared.
*/

/*!
    \fn QByteArray &QByteArray::assign(qsizetype n, char c)
    \since 6.6

    Replaces the contents of this byte array with \a n copies of \a c and
    returns a reference to this byte array.

    The size of this byte array will be equal to \a n, which has to be non-negative.

    This function will only allocate memory if \a n exceeds the capacity of this
    byte array or this byte array is shared.

    \sa fill()
*/

/*!
    \fn template <typename InputIterator, if_input_iterator<InputIterator>> QByteArray &QByteArray::assign(InputIterator first, InputIterator last)
    \since 6.6

    Replaces the contents of this byte array with a copy of the elements in the
    iterator range [\a first, \a last) and returns a reference to this
    byte array.

    The size of this byte array will be equal to the number of elements in the
    range [\a first, \a last).

    This function will only allocate memory if the number of elements in the
    range exceeds the capacity of this byte array or this byte array is shared.

    \note This function overload only participates in overload resolution if
    \c InputIterator meets the requirements of a
    \l {https://en.cppreference.com/w/cpp/named_req/InputIterator} {LegacyInputIterator}.

    \note The behavior is undefined if either argument is an iterator into *this or
    [\a first, \a last) is not a valid range.
*/

QByteArray &QByteArray::assign(QByteArrayView v)
{
    const auto len = v.size();

    if (len <= capacity() &&  isDetached()) {
        const auto offset = d.freeSpaceAtBegin();
        if (offset)
            d.setBegin(d.begin() - offset);
        std::memcpy(d.begin(), v.data(), len);
        d.size = len;
        d.data()[d.size] = '\0';
    } else {
        *this = v.toByteArray();
    }
    return *this;
}

/*!
    Inserts \a data at index position \a i and returns a
    reference to this byte array.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 17
    \since 6.0

    For large byte arrays, this operation can be slow (\l{linear time}),
    because it requires moving all the bytes at indexes \a i and
    above by at least one position further in memory.

//! [array-grow-at-insertion]
    This array grows to accommodate the insertion. If \a i is beyond
    the end of the array, the array is first extended with space characters
    to reach this \a i.
//! [array-grow-at-insertion]

    \sa append(), prepend(), replace(), remove()
*/
QByteArray &QByteArray::insert(qsizetype i, QByteArrayView data)
{
    const char *str = data.data();
    qsizetype size = data.size();
    if (i < 0 || size <= 0)
        return *this;

    // handle this specially, as QArrayDataOps::insert() doesn't handle out of
    // bounds positions
    if (i >= d->size) {
        // In case when data points into the range or is == *this, we need to
        // defer a call to free() so that it comes after we copied the data from
        // the old memory:
        DataPointer detached{};  // construction is free
        d.detachAndGrow(Data::GrowsAtEnd, (i - d.size) + size, &str, &detached);
        Q_CHECK_PTR(d.data());
        d->copyAppend(i - d->size, ' ');
        d->copyAppend(str, str + size);
        d.data()[d.size] = '\0';
        return *this;
    }

    if (!d->needsDetach() && QtPrivate::q_points_into_range(str, d)) {
        QVarLengthArray a(str, str + size);
        return insert(i, a);
    }

    d->insert(i, str, size);
    d.data()[d.size] = '\0';
    return *this;
}

/*!
    \fn QByteArray &QByteArray::insert(qsizetype i, const QByteArray &data)
    Inserts \a data at index position \a i and returns a
    reference to this byte array.

    \include qbytearray.cpp array-grow-at-insertion

    \sa append(), prepend(), replace(), remove()
*/

/*!
    \fn QByteArray &QByteArray::insert(qsizetype i, const char *s)
    Inserts \a s at index position \a i and returns a
    reference to this byte array.

    \include qbytearray.cpp array-grow-at-insertion

    The function is equivalent to \c{insert(i, QByteArrayView(s))}

    \sa append(), prepend(), replace(), remove()
*/

/*!
    \fn QByteArray &QByteArray::insert(qsizetype i, const char *data, qsizetype len)
    \overload
    \since 4.6

    Inserts \a len bytes, starting at \a data, at position \a i in the byte
    array.

    \include qbytearray.cpp array-grow-at-insertion
*/

/*!
    \fn QByteArray &QByteArray::insert(qsizetype i, char ch)
    \overload

    Inserts byte \a ch at index position \a i in the byte array.

    \include qbytearray.cpp array-grow-at-insertion
*/

/*! \fn QByteArray &QByteArray::insert(qsizetype i, qsizetype count, char ch)

    \overload
    \since 5.7

    Inserts \a count copies of byte \a ch at index position \a i in the byte
    array.

    \include qbytearray.cpp array-grow-at-insertion
*/

QByteArray &QByteArray::insert(qsizetype i, qsizetype count, char ch)
{
    if (i < 0 || count <= 0)
        return *this;

    if (i >= d->size) {
        // handle this specially, as QArrayDataOps::insert() doesn't handle out of bounds positions
        d.detachAndGrow(Data::GrowsAtEnd, (i - d.size) + count, nullptr, nullptr);
        Q_CHECK_PTR(d.data());
        d->copyAppend(i - d->size, ' ');
        d->copyAppend(count, ch);
        d.data()[d.size] = '\0';
        return *this;
    }

    d->insert(i, count, ch);
    d.data()[d.size] = '\0';
    return *this;
}

/*!
    Removes \a len bytes from the array, starting at index position \a
    pos, and returns a reference to the array.

    If \a pos is out of range, nothing happens. If \a pos is valid,
    but \a pos + \a len is larger than the size of the array, the
    array is truncated at position \a pos.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 18

    Element removal will preserve the array's capacity and not reduce the
    amount of allocated memory. To shed extra capacity and free as much memory
    as possible, call squeeze() after the last change to the array's size.

    \sa insert(), replace(), squeeze()
*/

QByteArray &QByteArray::remove(qsizetype pos, qsizetype len)
{
    if (len <= 0  || pos < 0 || size_t(pos) >= size_t(size()))
        return *this;
    if (pos + len > d->size)
        len = d->size - pos;

    auto begin = d.begin();
    if (!d->isShared()) {
        d->erase(begin + pos, len);
        d.data()[d.size] = '\0';
    } else {
        QByteArray copy{size() - len, Qt::Uninitialized};
        const auto toRemove_start = d.begin() + pos;
        copy.d->copyRanges({{d.begin(), toRemove_start},
                           {toRemove_start + len, d.end()}});
        swap(copy);
    }
    return *this;
}

/*!
  \fn QByteArray &QByteArray::removeAt(qsizetype pos)

  \since 6.5

  Removes the character at index \a pos. If \a pos is out of bounds
  (i.e. \a pos >= size()) this function does nothing.

  \sa remove()
*/

/*!
  \fn QByteArray &QByteArray::removeFirst()

  \since 6.5

  Removes the first character in this byte array. If the byte array is empty,
  this function does nothing.

  \sa remove()
*/
/*!
  \fn QByteArray &QByteArray::removeLast()

  \since 6.5

  Removes the last character in this byte array. If the byte array is empty,
  this function does nothing.

  \sa remove()
*/

/*!
    \fn template <typename Predicate> QByteArray &QByteArray::removeIf(Predicate pred)
    \since 6.1

    Removes all bytes for which the predicate \a pred returns true
    from the byte array. Returns a reference to the byte array.

    \sa remove()
*/

/*!
    Replaces \a len bytes from index position \a pos with the byte
    array \a after, and returns a reference to this byte array.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 19

    \sa insert(), remove()
*/

QByteArray &QByteArray::replace(qsizetype pos, qsizetype len, QByteArrayView after)
{
    if (QtPrivate::q_points_into_range(after.data(), d)) {
        QVarLengthArray copy(after.data(), after.data() + after.size());
        return replace(pos, len, QByteArrayView{copy});
    }
    if (len == after.size() && (pos + len <= size())) {
        // same size: in-place replacement possible
        if (len > 0) {
            detach();
            memcpy(d.data() + pos, after.data(), len*sizeof(char));
        }
        return *this;
    } else {
        // ### optimize me
        remove(pos, len);
        return insert(pos, after);
    }
}

/*! \fn QByteArray &QByteArray::replace(qsizetype pos, qsizetype len, const char *after, qsizetype alen)

    \overload

    Replaces \a len bytes from index position \a pos with \a alen bytes starting
    at position \a after. The bytes inserted may include '\\0' bytes.

    \since 4.7
*/

/*!
    \fn QByteArray &QByteArray::replace(const char *before, qsizetype bsize, const char *after, qsizetype asize)
    \overload

    Replaces every occurrence of the \a bsize bytes starting at \a before with
    the \a asize bytes starting at \a after. Since the sizes of the strings are
    given by \a bsize and \a asize, they may contain '\\0' bytes and do not need
    to be '\\0'-terminated.
*/

/*!
    \overload
    \since 6.0

    Replaces every occurrence of the byte array \a before with the
    byte array \a after.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 20
*/

QByteArray &QByteArray::replace(QByteArrayView before, QByteArrayView after)
{
    const char *b = before.data();
    qsizetype bsize = before.size();
    const char *a = after.data();
    qsizetype asize = after.size();

    if (isNull() || (b == a && bsize == asize))
        return *this;

    // protect against before or after being part of this
    if (QtPrivate::q_points_into_range(a, d)) {
        QVarLengthArray copy(a, a + asize);
        return replace(before, QByteArrayView{copy});
    }
    if (QtPrivate::q_points_into_range(b, d)) {
        QVarLengthArray copy(b, b + bsize);
        return replace(QByteArrayView{copy}, after);
    }

    QByteArrayMatcher matcher(b, bsize);
    qsizetype index = 0;
    qsizetype len = size();
    char *d = data(); // detaches

    if (bsize == asize) {
        if (bsize) {
            while ((index = matcher.indexIn(*this, index)) != -1) {
                memcpy(d + index, a, asize);
                index += bsize;
            }
        }
    } else if (asize < bsize) {
        size_t to = 0;
        size_t movestart = 0;
        size_t num = 0;
        while ((index = matcher.indexIn(*this, index)) != -1) {
            if (num) {
                qsizetype msize = index - movestart;
                if (msize > 0) {
                    memmove(d + to, d + movestart, msize);
                    to += msize;
                }
            } else {
                to = index;
            }
            if (asize) {
                memcpy(d + to, a, asize);
                to += asize;
            }
            index += bsize;
            movestart = index;
            num++;
        }
        if (num) {
            qsizetype msize = len - movestart;
            if (msize > 0)
                memmove(d + to, d + movestart, msize);
            resize(len - num*(bsize-asize));
        }
    } else {
        // the most complex case. We don't want to lose performance by doing repeated
        // copies and reallocs of the data.
        while (index != -1) {
            size_t indices[4096];
            size_t pos = 0;
            while(pos < 4095) {
                index = matcher.indexIn(*this, index);
                if (index == -1)
                    break;
                indices[pos++] = index;
                index += bsize;
                // avoid infinite loop
                if (!bsize)
                    index++;
            }
            if (!pos)
                break;

            // we have a table of replacement positions, use them for fast replacing
            qsizetype adjust = pos*(asize-bsize);
            // index has to be adjusted in case we get back into the loop above.
            if (index != -1)
                index += adjust;
            qsizetype newlen = len + adjust;
            qsizetype moveend = len;
            if (newlen > len) {
                resize(newlen);
                len = newlen;
            }
            d = this->d.data(); // data(), without the detach() check

            while(pos) {
                pos--;
                qsizetype movestart = indices[pos] + bsize;
                qsizetype insertstart = indices[pos] + pos*(asize-bsize);
                qsizetype moveto = insertstart + asize;
                memmove(d + moveto, d + movestart, (moveend - movestart));
                if (asize)
                    memcpy(d + insertstart, a, asize);
                moveend = movestart - bsize;
            }
        }
    }
    return *this;
}

/*!
    \fn QByteArray &QByteArray::replace(char before, QByteArrayView after)
    \overload

    Replaces every occurrence of the byte \a before with the byte array \a
    after.
*/

/*!
    \overload

    Replaces every occurrence of the byte \a before with the byte \a after.
*/

QByteArray &QByteArray::replace(char before, char after)
{
    if (!isEmpty()) {
        char *i = data();
        char *e = i + size();
        for (; i != e; ++i)
            if (*i == before)
                * i = after;
    }
    return *this;
}

/*!
    Splits the byte array into subarrays wherever \a sep occurs, and
    returns the list of those arrays. If \a sep does not match
    anywhere in the byte array, split() returns a single-element list
    containing this byte array.
*/

QList<QByteArray> QByteArray::split(char sep) const
{
    QList<QByteArray> list;
    qsizetype start = 0;
    qsizetype end;
    while ((end = indexOf(sep, start)) != -1) {
        list.append(mid(start, end - start));
        start = end + 1;
    }
    list.append(mid(start));
    return list;
}

/*!
    \since 4.5

    Returns a copy of this byte array repeated the specified number of \a times.

    If \a times is less than 1, an empty byte array is returned.

    Example:

    \snippet code/src_corelib_text_qbytearray.cpp 49
*/
QByteArray QByteArray::repeated(qsizetype times) const
{
    if (isEmpty())
        return *this;

    if (times <= 1) {
        if (times == 1)
            return *this;
        return QByteArray();
    }

    const qsizetype resultSize = times * size();

    QByteArray result;
    result.reserve(resultSize);
    if (result.capacity() != resultSize)
        return QByteArray(); // not enough memory

    memcpy(result.d.data(), data(), size());

    qsizetype sizeSoFar = size();
    char *end = result.d.data() + sizeSoFar;

    const qsizetype halfResultSize = resultSize >> 1;
    while (sizeSoFar <= halfResultSize) {
        memcpy(end, result.d.data(), sizeSoFar);
        end += sizeSoFar;
        sizeSoFar <<= 1;
    }
    memcpy(end, result.d.data(), resultSize - sizeSoFar);
    result.d.data()[resultSize] = '\0';
    result.d.size = resultSize;
    return result;
}

#define REHASH(a) \
    if (ol_minus_1 < sizeof(std::size_t) * CHAR_BIT) \
        hashHaystack -= std::size_t(a) << ol_minus_1; \
    hashHaystack <<= 1

static inline qsizetype findCharHelper(QByteArrayView haystack, qsizetype from, char needle) noexcept
{
    if (from < 0)
        from = qMax(from + haystack.size(), qsizetype(0));
    if (from < haystack.size()) {
        const char *const b = haystack.data();
        if (const auto n = static_cast<const char *>(
                    memchr(b + from, needle, static_cast<size_t>(haystack.size() - from)))) {
            return n - b;
        }
    }
    return -1;
}

qsizetype QtPrivate::findByteArray(QByteArrayView haystack, qsizetype from, QByteArrayView needle) noexcept
{
    const auto ol = needle.size();
    const auto l = haystack.size();
    if (ol == 0) {
        if (from < 0)
            return qMax(from + l, 0);
        else
            return from > l ? -1 : from;
    }

    if (ol == 1)
        return findCharHelper(haystack, from, needle.front());

    if (from > l || ol + from > l)
        return -1;

    return qFindByteArray(haystack.data(), haystack.size(), from, needle.data(), ol);
}

/*! \fn qsizetype QByteArray::indexOf(QByteArrayView bv, qsizetype from) const
    \since 6.0

    Returns the index position of the start of the first occurrence of the
    sequence of bytes viewed by \a bv in this byte array, searching forward
    from index position \a from. Returns -1 if no match is found.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 21

    \sa lastIndexOf(), contains(), count()
*/

/*!
    \overload

    Returns the index position of the start of the first occurrence of the
    byte \a ch in this byte array, searching forward from index position \a from.
    Returns -1 if no match is found.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 22

    \sa lastIndexOf(), contains()
*/

qsizetype QByteArray::indexOf(char ch, qsizetype from) const
{
    return qToByteArrayViewIgnoringNull(*this).indexOf(ch, from);
}

static qsizetype lastIndexOfHelper(const char *haystack, qsizetype l, const char *needle,
                                   qsizetype ol, qsizetype from)
{
    auto delta = l - ol;
    if (from < 0)
        from = delta;
    if (from < 0 || from > l)
        return -1;
    if (from > delta)
        from = delta;

    const char *end = haystack;
    haystack += from;
    const auto ol_minus_1 = std::size_t(ol - 1);
    const char *n = needle + ol_minus_1;
    const char *h = haystack + ol_minus_1;
    std::size_t hashNeedle = 0, hashHaystack = 0;
    qsizetype idx;
    for (idx = 0; idx < ol; ++idx) {
        hashNeedle = ((hashNeedle<<1) + *(n-idx));
        hashHaystack = ((hashHaystack<<1) + *(h-idx));
    }
    hashHaystack -= *haystack;
    while (haystack >= end) {
        hashHaystack += *haystack;
        if (hashHaystack == hashNeedle && memcmp(needle, haystack, ol) == 0)
            return haystack - end;
        --haystack;
        REHASH(*(haystack + ol));
    }
    return -1;

}

static inline qsizetype lastIndexOfCharHelper(QByteArrayView haystack, qsizetype from, char needle) noexcept
{
    if (haystack.size() == 0)
        return -1;
    if (from < 0)
        from += haystack.size();
    else if (from > haystack.size())
        from = haystack.size() - 1;
    if (from >= 0) {
        const char *b = haystack.data();
        const char *n = b + from + 1;
        while (n-- != b) {
            if (*n == needle)
                return n - b;
        }
    }
    return -1;
}

qsizetype QtPrivate::lastIndexOf(QByteArrayView haystack, qsizetype from, QByteArrayView needle) noexcept
{
    if (haystack.isEmpty()) {
        if (needle.isEmpty() && from == 0)
            return 0;
        return -1;
    }
    const auto ol = needle.size();
    if (ol == 1)
        return lastIndexOfCharHelper(haystack, from, needle.front());

    return lastIndexOfHelper(haystack.data(), haystack.size(), needle.data(), ol, from);
}

/*! \fn qsizetype QByteArray::lastIndexOf(QByteArrayView bv, qsizetype from) const
    \since 6.0

    Returns the index position of the start of the last occurrence of the
    sequence of bytes viewed by \a bv in this byte array, searching backward
    from index position \a from.

    \include qstring.qdocinc negative-index-start-search-from-end

    Returns -1 if no match is found.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 23

    \note When searching for a 0-length \a bv, the match at the end of
    the data is excluded from the search by a negative \a from, even
    though \c{-1} is normally thought of as searching from the end of
    the byte array: the match at the end is \e after the last character, so
    it is excluded. To include such a final empty match, either give a
    positive value for \a from or omit the \a from parameter entirely.

    \sa indexOf(), contains(), count()
*/

/*! \fn qsizetype QByteArray::lastIndexOf(QByteArrayView bv) const
    \since 6.2
    \overload

    Returns the index position of the start of the last occurrence of the
    sequence of bytes viewed by \a bv in this byte array, searching backward
    from the end of the byte array. Returns -1 if no match is found.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 23

    \sa indexOf(), contains(), count()
*/

/*!
    \overload

    Returns the index position of the start of the last occurrence of byte \a ch
    in this byte array, searching backward from index position \a from.
    If \a from is -1 (the default), the search starts at the last byte
    (at index size() - 1). Returns -1 if no match is found.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 24

    \sa indexOf(), contains()
*/

qsizetype QByteArray::lastIndexOf(char ch, qsizetype from) const
{
    return qToByteArrayViewIgnoringNull(*this).lastIndexOf(ch, from);
}

static inline qsizetype countCharHelper(QByteArrayView haystack, char needle) noexcept
{
    qsizetype num = 0;
    for (char ch : haystack) {
        if (ch == needle)
            ++num;
    }
    return num;
}

qsizetype QtPrivate::count(QByteArrayView haystack, QByteArrayView needle) noexcept
{
    if (needle.size() == 0)
        return haystack.size() + 1;

    if (needle.size() == 1)
        return countCharHelper(haystack, needle[0]);

    qsizetype num = 0;
    qsizetype i = -1;
    if (haystack.size() > 500 && needle.size() > 5) {
        QByteArrayMatcher matcher(needle);
        while ((i = matcher.indexIn(haystack, i + 1)) != -1)
            ++num;
    } else {
        while ((i = haystack.indexOf(needle, i + 1)) != -1)
            ++num;
    }
    return num;
}

/*! \fn qsizetype QByteArray::count(QByteArrayView bv) const
    \since 6.0

    Returns the number of (potentially overlapping) occurrences of the
    sequence of bytes viewed by \a bv in this byte array.

    \sa contains(), indexOf()
*/

/*!
    \overload

    Returns the number of occurrences of byte \a ch in the byte array.

    \sa contains(), indexOf()
*/

qsizetype QByteArray::count(char ch) const
{
    return countCharHelper(*this, ch);
}

#if QT_DEPRECATED_SINCE(6, 4)
/*! \fn qsizetype QByteArray::count() const
    \deprecated [6.4] Use size() or length() instead.
    \overload

    Same as size().
*/
#endif

/*!
    \fn int QByteArray::compare(QByteArrayView bv, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    \since 6.0

    Returns an integer less than, equal to, or greater than zero depending on
    whether this QByteArray sorts before, at the same position as, or after the
    QByteArrayView \a bv. The comparison is performed according to case
    sensitivity \a cs.

    \sa operator==, {Character Case}
*/

bool QtPrivate::startsWith(QByteArrayView haystack, QByteArrayView needle) noexcept
{
    if (haystack.size() < needle.size())
        return false;
    if (haystack.data() == needle.data() || needle.size() == 0)
        return true;
    return memcmp(haystack.data(), needle.data(), needle.size()) == 0;
}

/*! \fn bool QByteArray::startsWith(QByteArrayView bv) const
    \since 6.0

    Returns \c true if this byte array starts with the sequence of bytes
    viewed by \a bv; otherwise returns \c false.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 25

    \sa endsWith(), first()
*/

/*!
    \fn bool QByteArray::startsWith(char ch) const
    \overload

    Returns \c true if this byte array starts with byte \a ch; otherwise returns
    \c false.
*/

bool QtPrivate::endsWith(QByteArrayView haystack, QByteArrayView needle) noexcept
{
    if (haystack.size() < needle.size())
        return false;
    if (haystack.end() == needle.end() || needle.size() == 0)
        return true;
    return memcmp(haystack.end() - needle.size(), needle.data(), needle.size()) == 0;
}

/*!
    \fn bool QByteArray::endsWith(QByteArrayView bv) const
    \since 6.0

    Returns \c true if this byte array ends with the sequence of bytes
    viewed by \a bv; otherwise returns \c false.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 26

    \sa startsWith(), last()
*/

/*!
    \fn bool QByteArray::endsWith(char ch) const
    \overload

    Returns \c true if this byte array ends with byte \a ch;
    otherwise returns \c false.
*/

/*
    Returns true if \a c is an uppercase ASCII letter.
 */
static constexpr inline bool isUpperCaseAscii(char c)
{
    return c >= 'A' && c <= 'Z';
}

/*
    Returns true if \a c is an lowercase ASCII letter.
 */
static constexpr inline bool isLowerCaseAscii(char c)
{
    return c >= 'a' && c <= 'z';
}

/*!
    Returns \c true if this byte array is uppercase, that is, if
    it's identical to its toUpper() folding.

    Note that this does \e not mean that the byte array only contains
    uppercase letters; only that it contains no ASCII lowercase letters.

    \since 5.12

    \sa isLower(), toUpper()
*/
bool QByteArray::isUpper() const
{
    return std::none_of(begin(), end(), isLowerCaseAscii);
}

/*!
    Returns \c true if this byte array is lowercase, that is, if
    it's identical to its toLower() folding.

    Note that this does \e not mean that the byte array only contains
    lowercase letters; only that it contains no ASCII uppercase letters.

    \since 5.12

    \sa isUpper(), toLower()
 */
bool QByteArray::isLower() const
{
    return std::none_of(begin(), end(), isUpperCaseAscii);
}

/*!
    \fn QByteArray::isValidUtf8() const

    Returns \c true if this byte array contains valid UTF-8 encoded data,
    or \c false otherwise.

    \since 6.3
*/

/*!
    Returns a byte array that contains the first \a len bytes of this byte
    array.

    If you know that \a len cannot be out of bounds, use first() instead in new
    code, because it is faster.

    The entire byte array is returned if \a len is greater than
    size().

    Returns an empty QByteArray if \a len is smaller than 0.

    \sa first(), last(), startsWith(), chopped(), chop(), truncate()
*/

QByteArray QByteArray::left(qsizetype len)  const
{
    if (len >= size())
        return *this;
    if (len < 0)
        len = 0;
    return QByteArray(data(), len);
}

/*!
    Returns a byte array that contains the last \a len bytes of this byte array.

    If you know that \a len cannot be out of bounds, use last() instead in new
    code, because it is faster.

    The entire byte array is returned if \a len is greater than
    size().

    Returns an empty QByteArray if \a len is smaller than 0.

    \sa endsWith(), last(), first(), sliced(), chopped(), chop(), truncate()
*/
QByteArray QByteArray::right(qsizetype len) const
{
    if (len >= size())
        return *this;
    if (len < 0)
        len = 0;
    return QByteArray(end() - len, len);
}

/*!
    Returns a byte array containing \a len bytes from this byte array,
    starting at position \a pos.

    If you know that \a pos and \a len cannot be out of bounds, use sliced()
    instead in new code, because it is faster.

    If \a len is -1 (the default), or \a pos + \a len >= size(),
    returns a byte array containing all bytes starting at position \a
    pos until the end of the byte array.

    \sa first(), last(), sliced(), chopped(), chop(), truncate()
*/

QByteArray QByteArray::mid(qsizetype pos, qsizetype len) const
{
    qsizetype p = pos;
    qsizetype l = len;
    using namespace QtPrivate;
    switch (QContainerImplHelper::mid(size(), &p, &l)) {
    case QContainerImplHelper::Null:
        return QByteArray();
    case QContainerImplHelper::Empty:
    {
        return QByteArray(DataPointer::fromRawData(&_empty, 0));
    }
    case QContainerImplHelper::Full:
        return *this;
    case QContainerImplHelper::Subset:
        return QByteArray(d.data() + p, l);
    }
    Q_UNREACHABLE_RETURN(QByteArray());
}

/*!
    \fn QByteArray QByteArray::first(qsizetype n) const
    \since 6.0

    Returns the first \a n bytes of the byte array.

    \note The behavior is undefined when \a n < 0 or \a n > size().

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 27

    \sa last(), sliced(), startsWith(), chopped(), chop(), truncate()
*/

/*!
    \fn QByteArray QByteArray::last(qsizetype n) const
    \since 6.0

    Returns the last \a n bytes of the byte array.

    \note The behavior is undefined when \a n < 0 or \a n > size().

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 28

    \sa first(), sliced(), endsWith(), chopped(), chop(), truncate()
*/

/*!
    \fn QByteArray QByteArray::sliced(qsizetype pos, qsizetype n) const
    \since 6.0

    Returns a byte array containing the \a n bytes of this object starting
    at position \a pos.

    \note The behavior is undefined when \a pos < 0, \a n < 0,
    or \a pos + \a n > size().

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 29

    \sa first(), last(), chopped(), chop(), truncate()
*/

/*!
    \fn QByteArray QByteArray::sliced(qsizetype pos) const
    \since 6.0
    \overload

    Returns a byte array containing the bytes starting at position \a pos
    in this object, and extending to the end of this object.

    \note The behavior is undefined when \a pos < 0 or \a pos > size().

    \sa first(), last(), sliced(), chopped(), chop(), truncate()
*/

/*!
    \fn QByteArray QByteArray::chopped(qsizetype len) const
    \since 5.10

    Returns a byte array that contains the leftmost size() - \a len bytes of
    this byte array.

    \note The behavior is undefined if \a len is negative or greater than size().

    \sa endsWith(), first(), last(), sliced(), chop(), truncate()
*/

/*!
    \fn QByteArray QByteArray::toLower() const

    Returns a copy of the byte array in which each ASCII uppercase letter
    converted to lowercase.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 30

    \sa isLower(), toUpper(), {Character Case}
*/

template <typename T>
static QByteArray toCase_template(T &input, uchar (*lookup)(uchar))
{
    // find the first bad character in input
    const char *orig_begin = input.constBegin();
    const char *firstBad = orig_begin;
    const char *e = input.constEnd();
    for ( ; firstBad != e ; ++firstBad) {
        uchar ch = uchar(*firstBad);
        uchar converted = lookup(ch);
        if (ch != converted)
            break;
    }

    if (firstBad == e)
        return std::move(input);

    // transform the rest
    QByteArray s = std::move(input);    // will copy if T is const QByteArray
    char *b = s.begin();            // will detach if necessary
    char *p = b + (firstBad - orig_begin);
    e = b + s.size();
    for ( ; p != e; ++p)
        *p = char(lookup(uchar(*p)));
    return s;
}

QByteArray QByteArray::toLower_helper(const QByteArray &a)
{
    return toCase_template(a, asciiLower);
}

QByteArray QByteArray::toLower_helper(QByteArray &a)
{
    return toCase_template(a, asciiLower);
}

/*!
    \fn QByteArray QByteArray::toUpper() const

    Returns a copy of the byte array in which each ASCII lowercase letter
    converted to uppercase.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 31

    \sa isUpper(), toLower(), {Character Case}
*/

QByteArray QByteArray::toUpper_helper(const QByteArray &a)
{
    return toCase_template(a, asciiUpper);
}

QByteArray QByteArray::toUpper_helper(QByteArray &a)
{
    return toCase_template(a, asciiUpper);
}

/*! \fn void QByteArray::clear()

    Clears the contents of the byte array and makes it null.

    \sa resize(), isNull()
*/

void QByteArray::clear()
{
    d.clear();
}

#if !defined(QT_NO_DATASTREAM) || defined(QT_BOOTSTRAPPED)

/*! \relates QByteArray

    Writes byte array \a ba to the stream \a out and returns a reference
    to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &out, const QByteArray &ba)
{
    if (ba.isNull() && out.version() >= 6) {
        out << (quint32)0xffffffff;
        return out;
    }
    return out.writeBytes(ba.constData(), ba.size());
}

/*! \relates QByteArray

    Reads a byte array into \a ba from the stream \a in and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &in, QByteArray &ba)
{
    ba.clear();
    quint32 len;
    in >> len;
    if (len == 0xffffffff) { // null byte-array
        ba = QByteArray();
        return in;
    }

    const quint32 Step = 1024 * 1024;
    quint32 allocated = 0;

    do {
        qsizetype blockSize = qMin(Step, len - allocated);
        ba.resize(allocated + blockSize);
        if (in.readRawData(ba.data() + allocated, blockSize) != blockSize) {
            ba.clear();
            in.setStatus(QDataStream::ReadPastEnd);
            return in;
        }
        allocated += blockSize;
    } while (allocated < len);

    return in;
}
#endif // QT_NO_DATASTREAM

/*! \fn bool QByteArray::operator==(const QString &str) const

    Returns \c true if this byte array is equal to the UTF-8 encoding of \a str;
    otherwise returns \c false.

    The comparison is case sensitive.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. You
    then need to call QString::fromUtf8(), QString::fromLatin1(),
    or QString::fromLocal8Bit() explicitly if you want to convert the byte
    array to a QString before doing the comparison.
*/

/*! \fn bool QByteArray::operator!=(const QString &str) const

    Returns \c true if this byte array is not equal to the UTF-8 encoding of \a
    str; otherwise returns \c false.

    The comparison is case sensitive.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. You
    then need to call QString::fromUtf8(), QString::fromLatin1(),
    or QString::fromLocal8Bit() explicitly if you want to convert the byte
    array to a QString before doing the comparison.
*/

/*! \fn bool QByteArray::operator<(const QString &str) const

    Returns \c true if this byte array is lexically less than the UTF-8 encoding
    of \a str; otherwise returns \c false.

    The comparison is case sensitive.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. You
    then need to call QString::fromUtf8(), QString::fromLatin1(),
    or QString::fromLocal8Bit() explicitly if you want to convert the byte
    array to a QString before doing the comparison.
*/

/*! \fn bool QByteArray::operator>(const QString &str) const

    Returns \c true if this byte array is lexically greater than the UTF-8
    encoding of \a str; otherwise returns \c false.

    The comparison is case sensitive.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. You
    then need to call QString::fromUtf8(), QString::fromLatin1(),
    or QString::fromLocal8Bit() explicitly if you want to convert the byte
    array to a QString before doing the comparison.
*/

/*! \fn bool QByteArray::operator<=(const QString &str) const

    Returns \c true if this byte array is lexically less than or equal to the
    UTF-8 encoding of \a str; otherwise returns \c false.

    The comparison is case sensitive.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. You
    then need to call QString::fromUtf8(), QString::fromLatin1(),
    or QString::fromLocal8Bit() explicitly if you want to convert the byte
    array to a QString before doing the comparison.
*/

/*! \fn bool QByteArray::operator>=(const QString &str) const

    Returns \c true if this byte array is greater than or equal to the UTF-8
    encoding of \a str; otherwise returns \c false.

    The comparison is case sensitive.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. You
    then need to call QString::fromUtf8(), QString::fromLatin1(),
    or QString::fromLocal8Bit() explicitly if you want to convert the byte
    array to a QString before doing the comparison.
*/

/*! \fn bool QByteArray::operator==(const QByteArray &a1, const QByteArray &a2)
    \overload

    Returns \c true if byte array \a a1 is equal to byte array \a a2;
    otherwise returns \c false.

    \sa QByteArray::compare()
*/

/*! \fn bool QByteArray::operator==(const QByteArray &a1, const char *a2)
    \overload

    Returns \c true if byte array \a a1 is equal to the '\\0'-terminated string
    \a a2; otherwise returns \c false.

    \sa QByteArray::compare()
*/

/*! \fn bool QByteArray::operator==(const char *a1, const QByteArray &a2)
    \overload

    Returns \c true if '\\0'-terminated string \a a1 is equal to byte array \a
    a2; otherwise returns \c false.

    \sa QByteArray::compare()
*/

/*! \fn bool QByteArray::operator!=(const QByteArray &a1, const QByteArray &a2)
    \overload

    Returns \c true if byte array \a a1 is not equal to byte array \a a2;
    otherwise returns \c false.

    \sa QByteArray::compare()
*/

/*! \fn bool QByteArray::operator!=(const QByteArray &a1, const char *a2)
    \overload

    Returns \c true if byte array \a a1 is not equal to the '\\0'-terminated
    string \a a2; otherwise returns \c false.

    \sa QByteArray::compare()
*/

/*! \fn bool QByteArray::operator!=(const char *a1, const QByteArray &a2)
    \overload

    Returns \c true if '\\0'-terminated string \a a1 is not equal to byte array
    \a a2; otherwise returns \c false.

    \sa QByteArray::compare()
*/

/*! \fn bool QByteArray::operator<(const QByteArray &a1, const QByteArray &a2)
    \overload

    Returns \c true if byte array \a a1 is lexically less than byte array
    \a a2; otherwise returns \c false.

    \sa QByteArray::compare()
*/

/*! \fn bool QByteArray::operator<(const QByteArray &a1, const char *a2)
    \overload

    Returns \c true if byte array \a a1 is lexically less than the
    '\\0'-terminated string \a a2; otherwise returns \c false.

    \sa QByteArray::compare()
*/

/*! \fn bool QByteArray::operator<(const char *a1, const QByteArray &a2)
    \overload

    Returns \c true if '\\0'-terminated string \a a1 is lexically less than byte
    array \a a2; otherwise returns \c false.

    \sa QByteArray::compare()
*/

/*! \fn bool QByteArray::operator<=(const QByteArray &a1, const QByteArray &a2)
    \overload

    Returns \c true if byte array \a a1 is lexically less than or equal
    to byte array \a a2; otherwise returns \c false.

    \sa QByteArray::compare()
*/

/*! \fn bool QByteArray::operator<=(const QByteArray &a1, const char *a2)
    \overload

    Returns \c true if byte array \a a1 is lexically less than or equal to the
    '\\0'-terminated string \a a2; otherwise returns \c false.

    \sa QByteArray::compare()
*/

/*! \fn bool QByteArray::operator<=(const char *a1, const QByteArray &a2)
    \overload

    Returns \c true if '\\0'-terminated string \a a1 is lexically less than or
    equal to byte array \a a2; otherwise returns \c false.

    \sa QByteArray::compare()
*/

/*! \fn bool QByteArray::operator>(const QByteArray &a1, const QByteArray &a2)
    \overload

    Returns \c true if byte array \a a1 is lexically greater than byte
    array \a a2; otherwise returns \c false.

    \sa QByteArray::compare()
*/

/*! \fn bool QByteArray::operator>(const QByteArray &a1, const char *a2)
    \overload

    Returns \c true if byte array \a a1 is lexically greater than the
    '\\0'-terminated string \a a2; otherwise returns \c false.

    \sa QByteArray::compare()
*/

/*! \fn bool QByteArray::operator>(const char *a1, const QByteArray &a2)
    \overload

    Returns \c true if '\\0'-terminated string \a a1 is lexically greater than
    byte array \a a2; otherwise returns \c false.

    \sa QByteArray::compare()
*/

/*! \fn bool QByteArray::operator>=(const QByteArray &a1, const QByteArray &a2)
    \overload

    Returns \c true if byte array \a a1 is lexically greater than or
    equal to byte array \a a2; otherwise returns \c false.

    \sa QByteArray::compare()
*/

/*! \fn bool QByteArray::operator>=(const QByteArray &a1, const char *a2)
    \overload

    Returns \c true if byte array \a a1 is lexically greater than or equal to
    the '\\0'-terminated string \a a2; otherwise returns \c false.

    \sa QByteArray::compare()
*/

/*! \fn bool QByteArray::operator>=(const char *a1, const QByteArray &a2)
    \overload

    Returns \c true if '\\0'-terminated string \a a1 is lexically greater than
    or equal to byte array \a a2; otherwise returns \c false.

    \sa QByteArray::compare()
*/

/*! \fn QByteArray operator+(const QByteArray &a1, const QByteArray &a2)
    \relates QByteArray

    Returns a byte array that is the result of concatenating byte
    array \a a1 and byte array \a a2.

    \sa QByteArray::operator+=()
*/

/*! \fn QByteArray operator+(const QByteArray &a1, const char *a2)
    \relates QByteArray

    \overload

    Returns a byte array that is the result of concatenating byte array \a a1
    and '\\0'-terminated string \a a2.
*/

/*! \fn QByteArray operator+(const QByteArray &a1, char a2)
    \relates QByteArray

    \overload

    Returns a byte array that is the result of concatenating byte
    array \a a1 and byte \a a2.
*/

/*! \fn QByteArray operator+(const char *a1, const QByteArray &a2)
    \relates QByteArray

    \overload

    Returns a byte array that is the result of concatenating '\\0'-terminated
    string \a a1 and byte array \a a2.
*/

/*! \fn QByteArray operator+(char a1, const QByteArray &a2)
    \relates QByteArray

    \overload

    Returns a byte array that is the result of concatenating byte \a a1 and byte
    array \a a2.
*/

/*!
    \fn QByteArray QByteArray::simplified() const

    Returns a copy of this byte array that has spacing characters removed from
    the start and end, and in which each sequence of internal spacing characters
    is replaced with a single space.

    The spacing characters are those for which the standard C++ \c isspace()
    function returns \c true in the C locale; these are the ASCII characters
    tabulation '\\t', line feed '\\n', carriage return '\\r', vertical
    tabulation '\\v', form feed '\\f', and space ' '.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 32

    \sa trimmed(), QChar::SpecialCharacter, {Spacing Characters}
*/
QByteArray QByteArray::simplified_helper(const QByteArray &a)
{
    return QStringAlgorithms<const QByteArray>::simplified_helper(a);
}

QByteArray QByteArray::simplified_helper(QByteArray &a)
{
    return QStringAlgorithms<QByteArray>::simplified_helper(a);
}

/*!
    \fn QByteArray QByteArray::trimmed() const

    Returns a copy of this byte array with spacing characters removed from the
    start and end.

    The spacing characters are those for which the standard C++ \c isspace()
    function returns \c true in the C locale; these are the ASCII characters
    tabulation '\\t', line feed '\\n', carriage return '\\r', vertical
    tabulation '\\v', form feed '\\f', and space ' '.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 33

    Unlike simplified(), \l {QByteArray::trimmed()}{trimmed()} leaves internal
    spacing unchanged.

    \sa simplified(), QChar::SpecialCharacter, {Spacing Characters}
*/
QByteArray QByteArray::trimmed_helper(const QByteArray &a)
{
    return QStringAlgorithms<const QByteArray>::trimmed_helper(a);
}

QByteArray QByteArray::trimmed_helper(QByteArray &a)
{
    return QStringAlgorithms<QByteArray>::trimmed_helper(a);
}

QByteArrayView QtPrivate::trimmed(QByteArrayView view) noexcept
{
    auto start = view.begin();
    auto stop = view.end();
    QStringAlgorithms<QByteArrayView>::trimmed_helper_positions(start, stop);
    return QByteArrayView(start, stop);
}

/*!
    Returns a byte array of size \a width that contains this byte array padded
    with the \a fill byte.

    If \a truncate is false and the size() of the byte array is more
    than \a width, then the returned byte array is a copy of this byte
    array.

    If \a truncate is true and the size() of the byte array is more
    than \a width, then any bytes in a copy of the byte array
    after position \a width are removed, and the copy is returned.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 34

    \sa rightJustified()
*/

QByteArray QByteArray::leftJustified(qsizetype width, char fill, bool truncate) const
{
    QByteArray result;
    qsizetype len = size();
    qsizetype padlen = width - len;
    if (padlen > 0) {
        result.resize(len+padlen);
        if (len)
            memcpy(result.d.data(), data(), len);
        memset(result.d.data()+len, fill, padlen);
    } else {
        if (truncate)
            result = left(width);
        else
            result = *this;
    }
    return result;
}

/*!
    Returns a byte array of size \a width that contains the \a fill byte
    followed by this byte array.

    If \a truncate is false and the size of the byte array is more
    than \a width, then the returned byte array is a copy of this byte
    array.

    If \a truncate is true and the size of the byte array is more
    than \a width, then the resulting byte array is truncated at
    position \a width.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 35

    \sa leftJustified()
*/

QByteArray QByteArray::rightJustified(qsizetype width, char fill, bool truncate) const
{
    QByteArray result;
    qsizetype len = size();
    qsizetype padlen = width - len;
    if (padlen > 0) {
        result.resize(len+padlen);
        if (len)
            memcpy(result.d.data()+padlen, data(), len);
        memset(result.d.data(), fill, padlen);
    } else {
        if (truncate)
            result = left(width);
        else
            result = *this;
    }
    return result;
}

auto QtPrivate::toSignedInteger(QByteArrayView data, int base) -> ParsedNumber<qlonglong>
{
#if defined(QT_CHECK_RANGE)
    if (base != 0 && (base < 2 || base > 36)) {
        qWarning("QByteArray::toIntegral: Invalid base %d", base);
        base = 10;
    }
#endif
    if (data.isEmpty())
        return {};

    bool ok = false;
    const auto i = QLocaleData::bytearrayToLongLong(data, base, &ok);
    if (ok)
        return ParsedNumber(i);
    return {};
}

auto QtPrivate::toUnsignedInteger(QByteArrayView data, int base) -> ParsedNumber<qulonglong>
{
#if defined(QT_CHECK_RANGE)
    if (base != 0 && (base < 2 || base > 36)) {
        qWarning("QByteArray::toIntegral: Invalid base %d", base);
        base = 10;
    }
#endif
    if (data.isEmpty())
        return {};

    bool ok = false;
    const auto u = QLocaleData::bytearrayToUnsLongLong(data, base, &ok);
    if (ok)
        return ParsedNumber(u);
    return {};
}

/*!
    Returns the byte array converted to a \c {long long} using base \a base,
    which is ten by default. Bases 0 and 2 through 36 are supported, using
    letters for digits beyond 9; A is ten, B is eleven and so on.

    If \a base is 0, the base is determined automatically using the following
    rules: If the byte array begins with "0x", it is assumed to be hexadecimal
    (base 16); otherwise, if it begins with "0b", it is assumed to be binary
    (base 2); otherwise, if it begins with "0", it is assumed to be octal
    (base 8); otherwise it is assumed to be decimal.

    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    \note The conversion of the number is performed in the default C locale,
    regardless of the user's locale. Use QLocale to perform locale-aware
    conversions between numbers and strings.

    \note Support for the "0b" prefix was added in Qt 6.4.

    \sa number()
*/

qlonglong QByteArray::toLongLong(bool *ok, int base) const
{
    return QtPrivate::toIntegral<qlonglong>(qToByteArrayViewIgnoringNull(*this), ok, base);
}

/*!
    Returns the byte array converted to an \c {unsigned long long} using base \a
    base, which is ten by default. Bases 0 and 2 through 36 are supported, using
    letters for digits beyond 9; A is ten, B is eleven and so on.

    If \a base is 0, the base is determined automatically using the following
    rules: If the byte array begins with "0x", it is assumed to be hexadecimal
    (base 16); otherwise, if it begins with "0b", it is assumed to be binary
    (base 2); otherwise, if it begins with "0", it is assumed to be octal
    (base 8); otherwise it is assumed to be decimal.

    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    \note The conversion of the number is performed in the default C locale,
    regardless of the user's locale. Use QLocale to perform locale-aware
    conversions between numbers and strings.

    \note Support for the "0b" prefix was added in Qt 6.4.

    \sa number()
*/

qulonglong QByteArray::toULongLong(bool *ok, int base) const
{
    return QtPrivate::toIntegral<qulonglong>(qToByteArrayViewIgnoringNull(*this), ok, base);
}

/*!
    Returns the byte array converted to an \c int using base \a base, which is
    ten by default. Bases 0 and 2 through 36 are supported, using letters for
    digits beyond 9; A is ten, B is eleven and so on.

    If \a base is 0, the base is determined automatically using the following
    rules: If the byte array begins with "0x", it is assumed to be hexadecimal
    (base 16); otherwise, if it begins with "0b", it is assumed to be binary
    (base 2); otherwise, if it begins with "0", it is assumed to be octal
    (base 8); otherwise it is assumed to be decimal.

    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    \snippet code/src_corelib_text_qbytearray.cpp 36

    \note The conversion of the number is performed in the default C locale,
    regardless of the user's locale. Use QLocale to perform locale-aware
    conversions between numbers and strings.

    \note Support for the "0b" prefix was added in Qt 6.4.

    \sa number()
*/

int QByteArray::toInt(bool *ok, int base) const
{
    return QtPrivate::toIntegral<int>(qToByteArrayViewIgnoringNull(*this), ok, base);
}

/*!
    Returns the byte array converted to an \c {unsigned int} using base \a base,
    which is ten by default. Bases 0 and 2 through 36 are supported, using
    letters for digits beyond 9; A is ten, B is eleven and so on.

    If \a base is 0, the base is determined automatically using the following
    rules: If the byte array begins with "0x", it is assumed to be hexadecimal
    (base 16); otherwise, if it begins with "0b", it is assumed to be binary
    (base 2); otherwise, if it begins with "0", it is assumed to be octal
    (base 8); otherwise it is assumed to be decimal.

    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    \note The conversion of the number is performed in the default C locale,
    regardless of the user's locale. Use QLocale to perform locale-aware
    conversions between numbers and strings.

    \note Support for the "0b" prefix was added in Qt 6.4.

    \sa number()
*/

uint QByteArray::toUInt(bool *ok, int base) const
{
    return QtPrivate::toIntegral<uint>(qToByteArrayViewIgnoringNull(*this), ok, base);
}

/*!
    \since 4.1

    Returns the byte array converted to a \c long int using base \a base, which
    is ten by default. Bases 0 and 2 through 36 are supported, using letters for
    digits beyond 9; A is ten, B is eleven and so on.

    If \a base is 0, the base is determined automatically using the following
    rules: If the byte array begins with "0x", it is assumed to be hexadecimal
    (base 16); otherwise, if it begins with "0b", it is assumed to be binary
    (base 2); otherwise, if it begins with "0", it is assumed to be octal
    (base 8); otherwise it is assumed to be decimal.

    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    \snippet code/src_corelib_text_qbytearray.cpp 37

    \note The conversion of the number is performed in the default C locale,
    regardless of the user's locale. Use QLocale to perform locale-aware
    conversions between numbers and strings.

    \note Support for the "0b" prefix was added in Qt 6.4.

    \sa number()
*/
long QByteArray::toLong(bool *ok, int base) const
{
    return QtPrivate::toIntegral<long>(qToByteArrayViewIgnoringNull(*this), ok, base);
}

/*!
    \since 4.1

    Returns the byte array converted to an \c {unsigned long int} using base \a
    base, which is ten by default. Bases 0 and 2 through 36 are supported, using
    letters for digits beyond 9; A is ten, B is eleven and so on.

    If \a base is 0, the base is determined automatically using the following
    rules: If the byte array begins with "0x", it is assumed to be hexadecimal
    (base 16); otherwise, if it begins with "0b", it is assumed to be binary
    (base 2); otherwise, if it begins with "0", it is assumed to be octal
    (base 8); otherwise it is assumed to be decimal.

    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    \note The conversion of the number is performed in the default C locale,
    regardless of the user's locale. Use QLocale to perform locale-aware
    conversions between numbers and strings.

    \note Support for the "0b" prefix was added in Qt 6.4.

    \sa number()
*/
ulong QByteArray::toULong(bool *ok, int base) const
{
    return QtPrivate::toIntegral<ulong>(qToByteArrayViewIgnoringNull(*this), ok, base);
}

/*!
    Returns the byte array converted to a \c short using base \a base, which is
    ten by default. Bases 0 and 2 through 36 are supported, using letters for
    digits beyond 9; A is ten, B is eleven and so on.

    If \a base is 0, the base is determined automatically using the following
    rules: If the byte array begins with "0x", it is assumed to be hexadecimal
    (base 16); otherwise, if it begins with "0b", it is assumed to be binary
    (base 2); otherwise, if it begins with "0", it is assumed to be octal
    (base 8); otherwise it is assumed to be decimal.

    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    \note The conversion of the number is performed in the default C locale,
    regardless of the user's locale. Use QLocale to perform locale-aware
    conversions between numbers and strings.

    \note Support for the "0b" prefix was added in Qt 6.4.

    \sa number()
*/

short QByteArray::toShort(bool *ok, int base) const
{
    return QtPrivate::toIntegral<short>(qToByteArrayViewIgnoringNull(*this), ok, base);
}

/*!
    Returns the byte array converted to an \c {unsigned short} using base \a
    base, which is ten by default. Bases 0 and 2 through 36 are supported, using
    letters for digits beyond 9; A is ten, B is eleven and so on.

    If \a base is 0, the base is determined automatically using the following
    rules: If the byte array begins with "0x", it is assumed to be hexadecimal
    (base 16); otherwise, if it begins with "0b", it is assumed to be binary
    (base 2); otherwise, if it begins with "0", it is assumed to be octal
    (base 8); otherwise it is assumed to be decimal.

    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    \note The conversion of the number is performed in the default C locale,
    regardless of the user's locale. Use QLocale to perform locale-aware
    conversions between numbers and strings.

    \note Support for the "0b" prefix was added in Qt 6.4.

    \sa number()
*/

ushort QByteArray::toUShort(bool *ok, int base) const
{
    return QtPrivate::toIntegral<ushort>(qToByteArrayViewIgnoringNull(*this), ok, base);
}

/*!
    Returns the byte array converted to a \c double value.

    Returns an infinity if the conversion overflows or 0.0 if the
    conversion fails for other reasons (e.g. underflow).

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    \snippet code/src_corelib_text_qbytearray.cpp 38

    \warning The QByteArray content may only contain valid numerical characters
    which includes the plus/minus sign, the character e used in scientific
    notation, and the decimal point. Including the unit or additional characters
    leads to a conversion error.

    \note The conversion of the number is performed in the default C locale,
    regardless of the user's locale. Use QLocale to perform locale-aware
    conversions between numbers and strings.

    This function ignores leading and trailing whitespace.

    \sa number()
*/

double QByteArray::toDouble(bool *ok) const
{
    return QByteArrayView(*this).toDouble(ok);
}

auto QtPrivate::toDouble(QByteArrayView a) noexcept -> ParsedNumber<double>
{
    auto r = qt_asciiToDouble(a.data(), a.size(), WhitespacesAllowed);
    if (r.ok())
        return ParsedNumber{r.result};
    else
        return {};
}

/*!
    Returns the byte array converted to a \c float value.

    Returns an infinity if the conversion overflows or 0.0 if the
    conversion fails for other reasons (e.g. underflow).

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    \snippet code/src_corelib_text_qbytearray.cpp 38float

    \warning The QByteArray content may only contain valid numerical characters
    which includes the plus/minus sign, the character e used in scientific
    notation, and the decimal point. Including the unit or additional characters
    leads to a conversion error.

    \note The conversion of the number is performed in the default C locale,
    regardless of the user's locale. Use QLocale to perform locale-aware
    conversions between numbers and strings.

    This function ignores leading and trailing whitespace.

    \sa number()
*/

float QByteArray::toFloat(bool *ok) const
{
    return QLocaleData::convertDoubleToFloat(toDouble(ok), ok);
}

auto QtPrivate::toFloat(QByteArrayView a) noexcept -> ParsedNumber<float>
{
    if (const auto r = toDouble(a)) {
        bool ok = true;
        const auto f = QLocaleData::convertDoubleToFloat(*r, &ok);
        if (ok)
            return ParsedNumber(f);
    }
    return {};
}

/*!
    \since 5.2

    Returns a copy of the byte array, encoded using the options \a options.

    \snippet code/src_corelib_text_qbytearray.cpp 39

    The algorithm used to encode Base64-encoded data is defined in \l{RFC 4648}.

    \sa fromBase64()
*/
QByteArray QByteArray::toBase64(Base64Options options) const
{
    const char alphabet_base64[] = "ABCDEFGH" "IJKLMNOP" "QRSTUVWX" "YZabcdef"
                                   "ghijklmn" "opqrstuv" "wxyz0123" "456789+/";
    const char alphabet_base64url[] = "ABCDEFGH" "IJKLMNOP" "QRSTUVWX" "YZabcdef"
                                      "ghijklmn" "opqrstuv" "wxyz0123" "456789-_";
    const char *const alphabet = options & Base64UrlEncoding ? alphabet_base64url : alphabet_base64;
    const char padchar = '=';
    qsizetype padlen = 0;

    const qsizetype sz = size();

    QByteArray tmp((sz + 2) / 3 * 4, Qt::Uninitialized);

    qsizetype i = 0;
    char *out = tmp.data();
    while (i < sz) {
        // encode 3 bytes at a time
        int chunk = 0;
        chunk |= int(uchar(data()[i++])) << 16;
        if (i == sz) {
            padlen = 2;
        } else {
            chunk |= int(uchar(data()[i++])) << 8;
            if (i == sz)
                padlen = 1;
            else
                chunk |= int(uchar(data()[i++]));
        }

        int j = (chunk & 0x00fc0000) >> 18;
        int k = (chunk & 0x0003f000) >> 12;
        int l = (chunk & 0x00000fc0) >> 6;
        int m = (chunk & 0x0000003f);
        *out++ = alphabet[j];
        *out++ = alphabet[k];

        if (padlen > 1) {
            if ((options & OmitTrailingEquals) == 0)
                *out++ = padchar;
        } else {
            *out++ = alphabet[l];
        }
        if (padlen > 0) {
            if ((options & OmitTrailingEquals) == 0)
                *out++ = padchar;
        } else {
            *out++ = alphabet[m];
        }
    }
    Q_ASSERT((options & OmitTrailingEquals) || (out == tmp.size() + tmp.data()));
    if (options & OmitTrailingEquals)
        tmp.truncate(out - tmp.data());
    return tmp;
}

/*!
    \fn QByteArray &QByteArray::setNum(int n, int base)

    Represent the whole number \a n as text.

    Sets this byte array to a string representing \a n in base \a base (ten by
    default) and returns a reference to this byte array. Bases 2 through 36 are
    supported, using letters for digits beyond 9; A is ten, B is eleven and so
    on.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 40

    \note The format of the number is not localized; the default C locale is
    used regardless of the user's locale. Use QLocale to perform locale-aware
    conversions between numbers and strings.

    \sa number(), toInt()
*/

/*!
    \fn QByteArray &QByteArray::setNum(uint n, int base)
    \overload

    \sa toUInt()
*/

/*!
    \fn QByteArray &QByteArray::setNum(long n, int base)
    \overload

    \sa toLong()
*/

/*!
    \fn QByteArray &QByteArray::setNum(ulong n, int base)
    \overload

    \sa toULong()
*/

/*!
    \fn QByteArray &QByteArray::setNum(short n, int base)
    \overload

    \sa toShort()
*/

/*!
    \fn QByteArray &QByteArray::setNum(ushort n, int base)
    \overload

    \sa toUShort()
*/

static char *qulltoa2(char *p, qulonglong n, int base)
{
#if defined(QT_CHECK_RANGE)
    if (base < 2 || base > 36) {
        qWarning("QByteArray::setNum: Invalid base %d", base);
        base = 10;
    }
#endif
    const char b = 'a' - 10;
    do {
        const int c = n % base;
        n /= base;
        *--p = c + (c < 10 ? '0' : b);
    } while (n);

    return p;
}

/*!
    \overload

    \sa toLongLong()
*/
QByteArray &QByteArray::setNum(qlonglong n, int base)
{
    const int buffsize = 66; // big enough for MAX_ULLONG in base 2
    char buff[buffsize];
    char *p;

    if (n < 0) {
        // Take care to avoid overflow on negating min value:
        p = qulltoa2(buff + buffsize, qulonglong(-(1 + n)) + 1, base);
        *--p = '-';
    } else {
        p = qulltoa2(buff + buffsize, qulonglong(n), base);
    }

    clear();
    append(p, buffsize - (p - buff));
    return *this;
}

/*!
    \overload

    \sa toULongLong()
*/

QByteArray &QByteArray::setNum(qulonglong n, int base)
{
    const int buffsize = 66; // big enough for MAX_ULLONG in base 2
    char buff[buffsize];
    char *p = qulltoa2(buff + buffsize, n, base);

    clear();
    append(p, buffsize - (p - buff));
    return *this;
}

/*!
    \overload

    Represent the floating-point number \a n as text.

    Sets this byte array to a string representing \a n, with a given \a format
    and \a precision (with the same meanings as for \l {QString::number(double,
    char, int)}), and returns a reference to this byte array.

    \sa toDouble(), QLocale::FloatingPointPrecisionOption
*/

QByteArray &QByteArray::setNum(double n, char format, int precision)
{
    return *this = QByteArray::number(n, format, precision);
}

/*!
    \fn QByteArray &QByteArray::setNum(float n, char format, int precision)
    \overload

    Represent the floating-point number \a n as text.

    Sets this byte array to a string representing \a n, with a given \a format
    and \a precision (with the same meanings as for \l {QString::number(double,
    char, int)}), and returns a reference to this byte array.

    \sa toFloat()
*/

/*!
    Returns a byte-array representing the whole number \a n as text.

    Returns a byte array containing a string representing \a n, using the
    specified \a base (ten by default). Bases 2 through 36 are supported, using
    letters for digits beyond 9: A is ten, B is eleven and so on.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 41

    \note The format of the number is not localized; the default C locale is
    used regardless of the user's locale. Use QLocale to perform locale-aware
    conversions between numbers and strings.

    \sa setNum(), toInt()
*/
QByteArray QByteArray::number(int n, int base)
{
    QByteArray s;
    s.setNum(n, base);
    return s;
}

/*!
    \overload

    \sa toUInt()
*/
QByteArray QByteArray::number(uint n, int base)
{
    QByteArray s;
    s.setNum(n, base);
    return s;
}

/*!
    \overload

    \sa toLong()
*/
QByteArray QByteArray::number(long n, int base)
{
    QByteArray s;
    s.setNum(n, base);
    return s;
}

/*!
    \overload

    \sa toULong()
*/
QByteArray QByteArray::number(ulong n, int base)
{
    QByteArray s;
    s.setNum(n, base);
    return s;
}

/*!
    \overload

    \sa toLongLong()
*/
QByteArray QByteArray::number(qlonglong n, int base)
{
    QByteArray s;
    s.setNum(n, base);
    return s;
}

/*!
    \overload

    \sa toULongLong()
*/
QByteArray QByteArray::number(qulonglong n, int base)
{
    QByteArray s;
    s.setNum(n, base);
    return s;
}

/*!
    \overload
    Returns a byte-array representing the floating-point number \a n as text.

    Returns a byte array containing a string representing \a n, with a given \a
    format and \a precision, with the same meanings as for \l
    {QString::number(double, char, int)}. For example:

    \snippet code/src_corelib_text_qbytearray.cpp 42

    \sa toDouble(), QLocale::FloatingPointPrecisionOption
*/
QByteArray QByteArray::number(double n, char format, int precision)
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
            qWarning("QByteArray::setNum: Invalid format char '%c'", format);
#endif
            break;
    }

    return qdtoAscii(n, form, precision, isUpperCaseAscii(format));
}

/*!
    \fn QByteArray QByteArray::fromRawData(const char *data, qsizetype size) constexpr

    Constructs a QByteArray that uses the first \a size bytes of the
    \a data array. The bytes are \e not copied. The QByteArray will
    contain the \a data pointer. The caller guarantees that \a data
    will not be deleted or modified as long as this QByteArray and any
    copies of it exist that have not been modified. In other words,
    because QByteArray is an \l{implicitly shared} class and the
    instance returned by this function contains the \a data pointer,
    the caller must not delete \a data or modify it directly as long
    as the returned QByteArray and any copies exist. However,
    QByteArray does not take ownership of \a data, so the QByteArray
    destructor will never delete the raw \a data, even when the
    last QByteArray referring to \a data is destroyed.

    A subsequent attempt to modify the contents of the returned
    QByteArray or any copy made from it will cause it to create a deep
    copy of the \a data array before doing the modification. This
    ensures that the raw \a data array itself will never be modified
    by QByteArray.

    Here is an example of how to read data using a QDataStream on raw
    data in memory without copying the raw data into a QByteArray:

    \snippet code/src_corelib_text_qbytearray.cpp 43

    \warning A byte array created with fromRawData() is \e not '\\0'-terminated,
    unless the raw data contains a '\\0' byte at position \a size. While that
    does not matter for QDataStream or functions like indexOf(), passing the
    byte array to a function accepting a \c{const char *} expected to be
    '\\0'-terminated will fail.

    \sa setRawData(), data(), constData()
*/

/*!
    \since 4.7

    Resets the QByteArray to use the first \a size bytes of the
    \a data array. The bytes are \e not copied. The QByteArray will
    contain the \a data pointer. The caller guarantees that \a data
    will not be deleted or modified as long as this QByteArray and any
    copies of it exist that have not been modified.

    This function can be used instead of fromRawData() to re-use
    existing QByteArray objects to save memory re-allocations.

    \sa fromRawData(), data(), constData()
*/
QByteArray &QByteArray::setRawData(const char *data, qsizetype size)
{
    if (!data || !size)
        clear();
    else
        *this = fromRawData(data, size);
    return *this;
}

namespace {
struct fromBase64_helper_result {
    qsizetype decodedLength;
    QByteArray::Base64DecodingStatus status;
};

fromBase64_helper_result fromBase64_helper(const char *input, qsizetype inputSize,
                                           char *output /* may alias input */,
                                           QByteArray::Base64Options options)
{
    fromBase64_helper_result result{ 0, QByteArray::Base64DecodingStatus::Ok };

    unsigned int buf = 0;
    int nbits = 0;

    qsizetype offset = 0;
    for (qsizetype i = 0; i < inputSize; ++i) {
        int ch = input[i];
        int d;

        if (ch >= 'A' && ch <= 'Z') {
            d = ch - 'A';
        } else if (ch >= 'a' && ch <= 'z') {
            d = ch - 'a' + 26;
        } else if (ch >= '0' && ch <= '9') {
            d = ch - '0' + 52;
        } else if (ch == '+' && (options & QByteArray::Base64UrlEncoding) == 0) {
            d = 62;
        } else if (ch == '-' && (options & QByteArray::Base64UrlEncoding) != 0) {
            d = 62;
        } else if (ch == '/' && (options & QByteArray::Base64UrlEncoding) == 0) {
            d = 63;
        } else if (ch == '_' && (options & QByteArray::Base64UrlEncoding) != 0) {
            d = 63;
        } else {
            if (options & QByteArray::AbortOnBase64DecodingErrors) {
                if (ch == '=') {
                    // can have 1 or 2 '=' signs, in both cases padding base64Size to
                    // a multiple of 4. Any other case is illegal.
                    if ((inputSize % 4) != 0) {
                        result.status = QByteArray::Base64DecodingStatus::IllegalInputLength;
                        return result;
                    } else if ((i == inputSize - 1) ||
                        (i == inputSize - 2 && input[++i] == '=')) {
                        d = -1; // ... and exit the loop, normally
                    } else {
                        result.status = QByteArray::Base64DecodingStatus::IllegalPadding;
                        return result;
                    }
                } else {
                    result.status = QByteArray::Base64DecodingStatus::IllegalCharacter;
                    return result;
                }
            } else {
                d = -1;
            }
        }

        if (d != -1) {
            buf = (buf << 6) | d;
            nbits += 6;
            if (nbits >= 8) {
                nbits -= 8;
                Q_ASSERT(offset < i);
                output[offset++] = buf >> nbits;
                buf &= (1 << nbits) - 1;
            }
        }
    }

    result.decodedLength = offset;
    return result;
}
} // anonymous namespace

/*!
    \fn QByteArray::FromBase64Result QByteArray::fromBase64Encoding(QByteArray &&base64, Base64Options options)
    \fn QByteArray::FromBase64Result QByteArray::fromBase64Encoding(const QByteArray &base64, Base64Options options)
    \since 5.15
    \overload

    Decodes the Base64 array \a base64, using the options
    defined by \a options. If \a options contains \c{IgnoreBase64DecodingErrors}
    (the default), the input is not checked for validity; invalid
    characters in the input are skipped, enabling the decoding process to
    continue with subsequent characters. If \a options contains
    \c{AbortOnBase64DecodingErrors}, then decoding will stop at the first
    invalid character.

    For example:

    \snippet code/src_corelib_text_qbytearray.cpp 44ter

    The algorithm used to decode Base64-encoded data is defined in \l{RFC 4648}.

    Returns a QByteArrayFromBase64Result object, containing the decoded
    data and a flag telling whether decoding was successful. If the
    \c{AbortOnBase64DecodingErrors} option was passed and the input
    data was invalid, it is unspecified what the decoded data contains.

    \sa toBase64()
*/
QByteArray::FromBase64Result QByteArray::fromBase64Encoding(QByteArray &&base64, Base64Options options)
{
    // try to avoid a detach when calling data(), as it would over-allocate
    // (we need less space when decoding than the one required by the full copy)
    if (base64.isDetached()) {
        const auto base64result = fromBase64_helper(base64.data(),
                                                    base64.size(),
                                                    base64.data(), // in-place
                                                    options);
        base64.truncate(base64result.decodedLength);
        return { std::move(base64), base64result.status };
    }

    return fromBase64Encoding(base64, options);
}


QByteArray::FromBase64Result QByteArray::fromBase64Encoding(const QByteArray &base64, Base64Options options)
{
    const auto base64Size = base64.size();
    QByteArray result((base64Size * 3) / 4, Qt::Uninitialized);
    const auto base64result = fromBase64_helper(base64.data(),
                                                base64Size,
                                                const_cast<char *>(result.constData()),
                                                options);
    result.truncate(base64result.decodedLength);
    return { std::move(result), base64result.status };
}

/*!
    \since 5.2

    Returns a decoded copy of the Base64 array \a base64, using the options
    defined by \a options. If \a options contains \c{IgnoreBase64DecodingErrors}
    (the default), the input is not checked for validity; invalid
    characters in the input are skipped, enabling the decoding process to
    continue with subsequent characters. If \a options contains
    \c{AbortOnBase64DecodingErrors}, then decoding will stop at the first
    invalid character.

    For example:

    \snippet code/src_corelib_text_qbytearray.cpp 44

    The algorithm used to decode Base64-encoded data is defined in \l{RFC 4648}.

    Returns the decoded data, or, if the \c{AbortOnBase64DecodingErrors}
    option was passed and the input data was invalid, an empty byte array.

    \note The fromBase64Encoding() function is recommended in new code.

    \sa toBase64(), fromBase64Encoding()
*/
QByteArray QByteArray::fromBase64(const QByteArray &base64, Base64Options options)
{
    if (auto result = fromBase64Encoding(base64, options))
        return std::move(result.decoded);
    return QByteArray();
}

/*!
    Returns a decoded copy of the hex encoded array \a hexEncoded. Input is not
    checked for validity; invalid characters in the input are skipped, enabling
    the decoding process to continue with subsequent characters.

    For example:

    \snippet code/src_corelib_text_qbytearray.cpp 45

    \sa toHex()
*/
QByteArray QByteArray::fromHex(const QByteArray &hexEncoded)
{
    QByteArray res((hexEncoded.size() + 1)/ 2, Qt::Uninitialized);
    uchar *result = (uchar *)res.data() + res.size();

    bool odd_digit = true;
    for (qsizetype i = hexEncoded.size() - 1; i >= 0; --i) {
        uchar ch = uchar(hexEncoded.at(i));
        int tmp = QtMiscUtils::fromHex(ch);
        if (tmp == -1)
            continue;
        if (odd_digit) {
            --result;
            *result = tmp;
            odd_digit = false;
        } else {
            *result |= tmp << 4;
            odd_digit = true;
        }
    }

    res.remove(0, result - (const uchar *)res.constData());
    return res;
}

/*!
    Returns a hex encoded copy of the byte array.

    The hex encoding uses the numbers 0-9 and the letters a-f.

    If \a separator is not '\0', the separator character is inserted between
    the hex bytes.

    Example:
    \snippet code/src_corelib_text_qbytearray.cpp 50

    \since 5.9
    \sa fromHex()
*/
QByteArray QByteArray::toHex(char separator) const
{
    if (isEmpty())
        return QByteArray();

    const qsizetype length = separator ? (size() * 3 - 1) : (size() * 2);
    QByteArray hex(length, Qt::Uninitialized);
    char *hexData = hex.data();
    const uchar *data = (const uchar *)this->data();
    for (qsizetype i = 0, o = 0; i < size(); ++i) {
        hexData[o++] = QtMiscUtils::toHexLower(data[i] >> 4);
        hexData[o++] = QtMiscUtils::toHexLower(data[i] & 0xf);

        if ((separator) && (o < length))
            hexData[o++] = separator;
    }
    return hex;
}

static void q_fromPercentEncoding(QByteArray *ba, char percent)
{
    if (ba->isEmpty())
        return;

    char *data = ba->data();
    const char *inputPtr = data;

    qsizetype i = 0;
    qsizetype len = ba->size();
    qsizetype outlen = 0;
    int a, b;
    char c;
    while (i < len) {
        c = inputPtr[i];
        if (c == percent && i + 2 < len) {
            a = inputPtr[++i];
            b = inputPtr[++i];

            if (a >= '0' && a <= '9') a -= '0';
            else if (a >= 'a' && a <= 'f') a = a - 'a' + 10;
            else if (a >= 'A' && a <= 'F') a = a - 'A' + 10;

            if (b >= '0' && b <= '9') b -= '0';
            else if (b >= 'a' && b <= 'f') b  = b - 'a' + 10;
            else if (b >= 'A' && b <= 'F') b  = b - 'A' + 10;

            *data++ = (char)((a << 4) | b);
        } else {
            *data++ = c;
        }

        ++i;
        ++outlen;
    }

    if (outlen != len)
        ba->truncate(outlen);
}

/*!
    \since 6.4

    Decodes URI/URL-style percent-encoding.

    Returns a byte array containing the decoded text. The \a percent parameter
    allows use of a different character than '%' (for instance, '_' or '=') as
    the escape character.

    For example:
    \snippet code/src_corelib_text_qbytearray.cpp 54

    \note Given invalid input (such as a string containing the sequence "%G5",
    which is not a valid hexadecimal number) the output will be invalid as
    well. As an example: the sequence "%G5" could be decoded to 'W'.

    \sa toPercentEncoding(), QUrl::fromPercentEncoding()
*/
QByteArray QByteArray::percentDecoded(char percent) const
{
    if (isEmpty())
        return *this; // Preserves isNull().

    QByteArray tmp = *this;
    q_fromPercentEncoding(&tmp, percent);
    return tmp;
}

/*!
    \since 4.4

    Decodes \a input from URI/URL-style percent-encoding.

    Returns a byte array containing the decoded text. The \a percent parameter
    allows use of a different character than '%' (for instance, '_' or '=') as
    the escape character. Equivalent to input.percentDecoded(percent).

    For example:
    \snippet code/src_corelib_text_qbytearray.cpp 51

    \sa percentDecoded()
*/
QByteArray QByteArray::fromPercentEncoding(const QByteArray &input, char percent)
{
    return input.percentDecoded(percent);
}

/*! \fn QByteArray QByteArray::fromStdString(const std::string &str)
    \since 5.4

    Returns a copy of the \a str string as a QByteArray.

    \sa toStdString(), QString::fromStdString()
*/
QByteArray QByteArray::fromStdString(const std::string &s)
{
    return QByteArray(s.data(), qsizetype(s.size()));
}

/*!
    \fn std::string QByteArray::toStdString() const
    \since 5.4

    Returns a std::string object with the data contained in this
    QByteArray.

    This operator is mostly useful to pass a QByteArray to a function
    that accepts a std::string object.

    \sa fromStdString(), QString::toStdString()
*/
std::string QByteArray::toStdString() const
{
    return std::string(data(), size_t(size()));
}

/*!
    \since 4.4

    Returns a URI/URL-style percent-encoded copy of this byte array. The
    \a percent parameter allows you to override the default '%'
    character for another.

    By default, this function will encode all bytes that are not one of the
    following:

        ALPHA ("a" to "z" and "A" to "Z") / DIGIT (0 to 9) / "-" / "." / "_" / "~"

    To prevent bytes from being encoded pass them to \a exclude. To force bytes
    to be encoded pass them to \a include. The \a percent character is always
    encoded.

    Example:

    \snippet code/src_corelib_text_qbytearray.cpp 52

    The hex encoding uses the numbers 0-9 and the uppercase letters A-F.

    \sa fromPercentEncoding(), QUrl::toPercentEncoding()
*/
QByteArray QByteArray::toPercentEncoding(const QByteArray &exclude, const QByteArray &include,
                                         char percent) const
{
    if (isNull())
        return QByteArray();    // preserve null
    if (isEmpty())
        return QByteArray(data(), 0);

    const auto contains = [](const QByteArray &view, char c) {
        // As view.contains(c), but optimised to bypass a lot of overhead:
        return view.size() > 0 && memchr(view.data(), c, view.size()) != nullptr;
    };

    QByteArray result = *this;
    char *output = nullptr;
    qsizetype length = 0;

    for (unsigned char c : *this) {
        if (char(c) != percent
            && ((c >= 0x61 && c <= 0x7A) // ALPHA
                || (c >= 0x41 && c <= 0x5A) // ALPHA
                || (c >= 0x30 && c <= 0x39) // DIGIT
                || c == 0x2D // -
                || c == 0x2E // .
                || c == 0x5F // _
                || c == 0x7E // ~
                || contains(exclude, c))
            && !contains(include, c)) {
            if (output)
                output[length] = c;
            ++length;
        } else {
            if (!output) {
                // detach now
                result.resize(size() * 3); // worst case
                output = result.data();
            }
            output[length++] = percent;
            output[length++] = QtMiscUtils::toHexUpper((c & 0xf0) >> 4);
            output[length++] = QtMiscUtils::toHexUpper(c & 0xf);
        }
    }
    if (output)
        result.truncate(length);

    return result;
}

#if defined(Q_OS_WASM) || defined(Q_QDOC)

/*!
    Constructs a new QByteArray containing a copy of the Uint8Array \a uint8array.

    This function transfers data from a JavaScript data buffer - which
    is not addressable from C++ code - to heap memory owned by a QByteArray.
    The Uint8Array can be released once this function returns and a copy
    has been made.

    The \a uint8array argument must an emscripten::val referencing an Uint8Array
    object, e.g. obtained from a global JavaScript variable:

    \snippet code/src_corelib_text_qbytearray.cpp 55

    This function returns a null QByteArray if the size of the Uint8Array
    exceeds the maximum capacity of QByteArray, or if the \a uint8array
    argument is not of the Uint8Array type.

    \since 6.5
    \ingroup platform-type-conversions

    \sa toEcmaUint8Array()
*/

QByteArray QByteArray::fromEcmaUint8Array(emscripten::val uint8array)
{
    return qstdweb::Uint8Array(uint8array).copyToQByteArray();
}

/*!
    Creates a Uint8Array from a QByteArray.

    This function transfers data from heap memory owned by a QByteArray
    to a JavaScript data buffer. The function allocates and copies into an
    ArrayBuffer, and returns a Uint8Array view to that buffer.

    The JavaScript objects own a copy of the data, and this
    QByteArray can be safely deleted after the copy has been made.

    \snippet code/src_corelib_text_qbytearray.cpp 56

    \since 6.5
    \ingroup platform-type-conversions

    \sa toEcmaUint8Array()
*/
emscripten::val QByteArray::toEcmaUint8Array()
{
    return qstdweb::Uint8Array::copyFrom(*this).val();
}

#endif

/*! \typedef QByteArray::ConstIterator
    \internal
*/

/*! \typedef QByteArray::Iterator
    \internal
*/

/*! \typedef QByteArray::const_iterator

    This typedef provides an STL-style const iterator for QByteArray.

    \sa QByteArray::const_reverse_iterator, QByteArray::iterator
*/

/*! \typedef QByteArray::iterator

    This typedef provides an STL-style non-const iterator for QByteArray.

    \sa QByteArray::reverse_iterator, QByteArray::const_iterator
*/

/*! \typedef QByteArray::const_reverse_iterator
    \since 5.6

    This typedef provides an STL-style const reverse iterator for QByteArray.

    \sa QByteArray::reverse_iterator, QByteArray::const_iterator
*/

/*! \typedef QByteArray::reverse_iterator
    \since 5.6

    This typedef provides an STL-style non-const reverse iterator for QByteArray.

    \sa QByteArray::const_reverse_iterator, QByteArray::iterator
*/

/*! \typedef QByteArray::size_type
    \internal
*/

/*! \typedef QByteArray::difference_type
    \internal
*/

/*! \typedef QByteArray::const_reference
    \internal
*/

/*! \typedef QByteArray::reference
    \internal
*/

/*! \typedef QByteArray::const_pointer
    \internal
*/

/*! \typedef QByteArray::pointer
    \internal
*/

/*! \typedef QByteArray::value_type
  \internal
 */

/*!
    \fn DataPtr &QByteArray::data_ptr()
    \internal
*/

/*!
    \typedef QByteArray::DataPtr
    \internal
*/

/*!
    \macro QByteArrayLiteral(ba)
    \relates QByteArray

    The macro generates the data for a QByteArray out of the string literal \a
    ba at compile time. Creating a QByteArray from it is free in this case, and
    the generated byte array data is stored in the read-only segment of the
    compiled object file.

    For instance:

    \snippet code/src_corelib_text_qbytearray.cpp 53

    Using QByteArrayLiteral instead of a double quoted plain C++ string literal
    can significantly speed up creation of QByteArray instances from data known
    at compile time.

    \sa QStringLiteral
*/

#if QT_DEPRECATED_SINCE(6, 8)
/*!
  \fn QtLiterals::operator""_qba(const char *str, size_t size)

  \relates QByteArray
  \since 6.2
  \deprecated [6.8] Use \c _ba from Qt::StringLiterals namespace instead.

  Literal operator that creates a QByteArray out of the first \a size characters
  in the char string literal \a str.

  The QByteArray is created at compile time, and the generated string data is stored
  in the read-only segment of the compiled object file. Duplicate literals may share
  the same read-only memory. This functionality is interchangeable with
  QByteArrayLiteral, but saves typing when many string literals are present in the
  code.

  The following code creates a QByteArray:
  \code
  auto str = "hello"_qba;
  \endcode

  \sa QByteArrayLiteral, QtLiterals::operator""_qs(const char16_t *str, size_t size)
*/
#endif // QT_DEPRECATED_SINCE(6, 8)

/*!
    \fn Qt::Literals::StringLiterals::operator""_ba(const char *str, size_t size)

    \relates QByteArray
    \since 6.4

    Literal operator that creates a QByteArray out of the first \a size characters
    in the char string literal \a str.

    The QByteArray is created at compile time, and the generated string data is stored
    in the read-only segment of the compiled object file. Duplicate literals may share
    the same read-only memory. This functionality is interchangeable with
    QByteArrayLiteral, but saves typing when many string literals are present in the
    code.

    The following code creates a QByteArray:
    \code
    using namespace Qt::Literals::StringLiterals;

    auto str = "hello"_ba;
    \endcode

    \sa Qt::Literals::StringLiterals
*/

/*!
    \class QByteArray::FromBase64Result
    \inmodule QtCore
    \ingroup tools
    \since 5.15

    \brief The QByteArray::FromBase64Result class holds the result of
    a call to QByteArray::fromBase64Encoding.

    Objects of this class can be used to check whether the conversion
    was successful, and if so, retrieve the decoded QByteArray. The
    conversion operators defined for QByteArray::FromBase64Result make
    its usage straightforward:

    \snippet code/src_corelib_text_qbytearray.cpp 44ter

    Alternatively, it is possible to access the conversion status
    and the decoded data directly:

    \snippet code/src_corelib_text_qbytearray.cpp 44quater

    \sa QByteArray::fromBase64
*/

/*!
    \variable QByteArray::FromBase64Result::decoded

    Contains the decoded byte array.
*/

/*!
    \variable QByteArray::FromBase64Result::decodingStatus

    Contains whether the decoding was successful, expressed as a value
    of type QByteArray::Base64DecodingStatus.
*/

/*!
    \fn QByteArray::FromBase64Result::operator bool() const

    Returns whether the decoding was successful. This is equivalent
    to checking whether the \c{decodingStatus} member is equal to
    QByteArray::Base64DecodingStatus::Ok.
*/

/*!
    \fn QByteArray &QByteArray::FromBase64Result::operator*() const

    Returns the decoded byte array.
*/

/*!
    \fn bool QByteArray::FromBase64Result::operator==(const QByteArray::FromBase64Result &lhs, const QByteArray::FromBase64Result &rhs) noexcept

    Returns \c true if \a lhs and \a rhs are equal, otherwise returns \c false.

    \a lhs and \a rhs are equal if and only if they contain the same decoding
    status and, if the status is QByteArray::Base64DecodingStatus::Ok, if and
    only if they contain the same decoded data.
*/

/*!
    \fn bool QByteArray::FromBase64Result::operator!=(const QByteArray::FromBase64Result &lhs, const QByteArray::FromBase64Result &rhs) noexcept

    Returns \c true if \a lhs and \a rhs are different, otherwise
    returns \c false.
*/

/*!
    \relates QByteArray::FromBase64Result

    Returns the hash value for \a key, using
    \a seed to seed the calculation.
*/
size_t qHash(const QByteArray::FromBase64Result &key, size_t seed) noexcept
{
    return qHashMulti(seed, key.decoded, static_cast<int>(key.decodingStatus));
}

/*! \fn template <typename T> qsizetype erase(QByteArray &ba, const T &t)
    \relates QByteArray
    \since 6.1

    Removes all elements that compare equal to \a t from the
    byte array \a ba. Returns the number of elements removed, if any.

    \sa erase_if
*/

/*! \fn template <typename Predicate> qsizetype erase_if(QByteArray &ba, Predicate pred)
    \relates QByteArray
    \since 6.1

    Removes all elements for which the predicate \a pred returns true
    from the byte array \a ba. Returns the number of elements removed, if
    any.

    \sa erase
*/

QT_END_NAMESPACE

#undef REHASH
