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

#include "qbytearray.h"
#include "qbytearraymatcher.h"
#include "qtools_p.h"
#include "qstring.h"
#include "qlist.h"
#include "qlocale.h"
#include "qlocale_p.h"
#include "qlocale_tools_p.h"
#include "private/qnumeric_p.h"
#include "qstringalgorithms_p.h"
#include "qscopedpointer.h"
#include "qbytearray_p.h"
#include <qdatastream.h>
#include <qmath.h>

#ifndef QT_NO_COMPRESS
#include <zconf.h>
#include <zlib.h>
#endif
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#define IS_RAW_DATA(d) ((d)->offset != sizeof(QByteArrayData))

QT_BEGIN_NAMESPACE

// Latin 1 case system, used by QByteArray::to{Upper,Lower}() and qstr(n)icmp():
/*
#!/usr/bin/perl -l
use feature "unicode_strings";
for (0..255) {
    $up = uc(chr($_));
    $up = chr($_) if ord($up) > 0x100 || length $up > 1;
    printf "0x%02x,", ord($up);
    print "" if ($_ & 0xf) == 0xf;
}
*/
static const uchar latin1_uppercased[256] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
    0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
    0x60,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
    0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x7b,0x7c,0x7d,0x7e,0x7f,
    0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
    0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
    0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
    0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
    0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
    0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
    0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
    0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xf7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xff
};

/*
#!/usr/bin/perl -l
use feature "unicode_strings";
for (0..255) {
    $up = lc(chr($_));
    $up = chr($_) if ord($up) > 0x100 || length $up > 1;
    printf "0x%02x,", ord($up);
    print "" if ($_ & 0xf) == 0xf;
}
*/
static const uchar latin1_lowercased[256] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
    0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x5b,0x5c,0x5d,0x5e,0x5f,
    0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
    0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
    0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
    0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
    0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
    0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
    0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
    0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xd7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xdf,
    0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
    0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
};

int qFindByteArray(
    const char *haystack0, int haystackLen, int from,
    const char *needle0, int needleLen);

/*
 * This pair of functions is declared in qtools_p.h and is used by the Qt
 * containers to allocate memory and grow the memory block during append
 * operations.
 *
 * They take size_t parameters and return size_t so they will change sizes
 * according to the pointer width. However, knowing Qt containers store the
 * container size and element indexes in ints, these functions never return a
 * size larger than INT_MAX. This is done by casting the element count and
 * memory block size to int in several comparisons: the check for negative is
 * very fast on most platforms as the code only needs to check the sign bit.
 *
 * These functions return SIZE_MAX on overflow, which can be passed to malloc()
 * and will surely cause a NULL return (there's no way you can allocate a
 * memory block the size of your entire VM space).
 */

/*!
    \internal
    \since 5.7

    Returns the memory block size for a container containing \a elementCount
    elements, each of \a elementSize bytes, plus a header of \a headerSize
    bytes. That is, this function returns \c
      {elementCount * elementSize + headerSize}

    but unlike the simple calculation, it checks for overflows during the
    multiplication and the addition.

    Both \a elementCount and \a headerSize can be zero, but \a elementSize
    cannot.

    This function returns SIZE_MAX (~0) on overflow or if the memory block size
    would not fit an int.
*/
size_t qCalculateBlockSize(size_t elementCount, size_t elementSize, size_t headerSize) Q_DECL_NOTHROW
{
    unsigned count = unsigned(elementCount);
    unsigned size = unsigned(elementSize);
    unsigned header = unsigned(headerSize);
    Q_ASSERT(elementSize);
    Q_ASSERT(size == elementSize);
    Q_ASSERT(header == headerSize);

    if (Q_UNLIKELY(count != elementCount))
        return std::numeric_limits<size_t>::max();

    unsigned bytes;
    if (Q_UNLIKELY(mul_overflow(size, count, &bytes)) ||
            Q_UNLIKELY(add_overflow(bytes, header, &bytes)))
        return std::numeric_limits<size_t>::max();
    if (Q_UNLIKELY(int(bytes) < 0))     // catches bytes >= 2GB
        return std::numeric_limits<size_t>::max();

    return bytes;
}

/*!
    \internal
    \since 5.7

    Returns the memory block size and the number of elements that will fit in
    that block for a container containing \a elementCount elements, each of \a
    elementSize bytes, plus a header of \a headerSize bytes. This function
    assumes the container will grow and pre-allocates a growth factor.

    Both \a elementCount and \a headerSize can be zero, but \a elementSize
    cannot.

    This function returns SIZE_MAX (~0) on overflow or if the memory block size
    would not fit an int.

    \note The memory block may contain up to \a elementSize - 1 bytes more than
    needed.
*/
CalculateGrowingBlockSizeResult
qCalculateGrowingBlockSize(size_t elementCount, size_t elementSize, size_t headerSize) Q_DECL_NOTHROW
{
    CalculateGrowingBlockSizeResult result = {
        std::numeric_limits<size_t>::max(),std::numeric_limits<size_t>::max()
    };

    unsigned bytes = unsigned(qCalculateBlockSize(elementCount, elementSize, headerSize));
    if (int(bytes) < 0)     // catches std::numeric_limits<size_t>::max()
        return result;

    unsigned morebytes = qNextPowerOfTwo(bytes);
    if (Q_UNLIKELY(int(morebytes) < 0)) {
        // catches morebytes == 2GB
        // grow by half the difference between bytes and morebytes
        bytes += (morebytes - bytes) / 2;
    } else {
        bytes = morebytes;
    }

    result.elementCount = (bytes - unsigned(headerSize)) / unsigned(elementSize);
    result.size = bytes;
    return result;
}

/*****************************************************************************
  Safe and portable C string functions; extensions to standard string.h
 *****************************************************************************/

/*! \relates QByteArray

    Returns a duplicate string.

    Allocates space for a copy of \a src, copies it, and returns a
    pointer to the copy. If \a src is 0, it immediately returns 0.

    Ownership is passed to the caller, so the returned string must be
    deleted using \c delete[].
*/

char *qstrdup(const char *src)
{
    if (!src)
        return 0;
    char *dst = new char[strlen(src) + 1];
    return qstrcpy(dst, src);
}

/*! \relates QByteArray

    Copies all the characters up to and including the '\\0' from \a
    src into \a dst and returns a pointer to \a dst. If \a src is 0,
    it immediately returns 0.

    This function assumes that \a dst is large enough to hold the
    contents of \a src.

    \sa qstrncpy()
*/

char *qstrcpy(char *dst, const char *src)
{
    if (!src)
        return 0;
#if defined(_MSC_VER) && _MSC_VER >= 1400
    const int len = int(strlen(src));
    // This is actually not secure!!! It will be fixed
    // properly in a later release!
    if (len >= 0 && strcpy_s(dst, len+1, src) == 0)
        return dst;
    return 0;
#else
    return strcpy(dst, src);
#endif
}

/*! \relates QByteArray

    A safe \c strncpy() function.

    Copies at most \a len bytes from \a src (stopping at \a len or the
    terminating '\\0' whichever comes first) into \a dst and returns a
    pointer to \a dst. Guarantees that \a dst is '\\0'-terminated. If
    \a src or \a dst is 0, returns 0 immediately.

    This function assumes that \a dst is at least \a len characters
    long.

    \note When compiling with Visual C++ compiler version 14.00
    (Visual C++ 2005) or later, internally the function strncpy_s
    will be used.

    \sa qstrcpy()
*/

char *qstrncpy(char *dst, const char *src, uint len)
{
    if (!src || !dst)
        return 0;
    if (len > 0) {
#if defined(_MSC_VER) && _MSC_VER >= 1400
        strncpy_s(dst, len, src, len - 1);
#else
        strncpy(dst, src, len);
#endif
        dst[len-1] = '\0';
    }
    return dst;
}

/*! \fn uint qstrlen(const char *str)
    \relates QByteArray

    A safe \c strlen() function.

    Returns the number of characters that precede the terminating '\\0',
    or 0 if \a str is 0.

    \sa qstrnlen()
*/

/*! \fn uint qstrnlen(const char *str, uint maxlen)
    \relates QByteArray
    \since 4.2

    A safe \c strnlen() function.

    Returns the number of characters that precede the terminating '\\0', but
    at most \a maxlen. If \a str is 0, returns 0.

    \sa qstrlen()
*/

/*!
    \relates QByteArray

    A safe \c strcmp() function.

    Compares \a str1 and \a str2. Returns a negative value if \a str1
    is less than \a str2, 0 if \a str1 is equal to \a str2 or a
    positive value if \a str1 is greater than \a str2.

    Special case 1: Returns 0 if \a str1 and \a str2 are both 0.

    Special case 2: Returns an arbitrary non-zero value if \a str1 is 0
    or \a str2 is 0 (but not both).

    \sa qstrncmp(), qstricmp(), qstrnicmp(), {8-bit Character Comparisons}
*/
int qstrcmp(const char *str1, const char *str2)
{
    return (str1 && str2) ? strcmp(str1, str2)
        : (str1 ? 1 : (str2 ? -1 : 0));
}

/*! \fn int qstrncmp(const char *str1, const char *str2, uint len);

    \relates QByteArray

    A safe \c strncmp() function.

    Compares at most \a len bytes of \a str1 and \a str2.

    Returns a negative value if \a str1 is less than \a str2, 0 if \a
    str1 is equal to \a str2 or a positive value if \a str1 is greater
    than \a str2.

    Special case 1: Returns 0 if \a str1 and \a str2 are both 0.

    Special case 2: Returns a random non-zero value if \a str1 is 0
    or \a str2 is 0 (but not both).

    \sa qstrcmp(), qstricmp(), qstrnicmp(), {8-bit Character Comparisons}
*/

/*! \relates QByteArray

    A safe \c stricmp() function.

    Compares \a str1 and \a str2 ignoring the case of the
    characters. The encoding of the strings is assumed to be Latin-1.

    Returns a negative value if \a str1 is less than \a str2, 0 if \a
    str1 is equal to \a str2 or a positive value if \a str1 is greater
    than \a str2.

    Special case 1: Returns 0 if \a str1 and \a str2 are both 0.

    Special case 2: Returns a random non-zero value if \a str1 is 0
    or \a str2 is 0 (but not both).

    \sa qstrcmp(), qstrncmp(), qstrnicmp(), {8-bit Character Comparisons}
*/

int qstricmp(const char *str1, const char *str2)
{
    const uchar *s1 = reinterpret_cast<const uchar *>(str1);
    const uchar *s2 = reinterpret_cast<const uchar *>(str2);
    int res;
    uchar c;
    if (!s1 || !s2)
        return s1 ? 1 : (s2 ? -1 : 0);
    for (; !(res = (c = latin1_lowercased[*s1]) - latin1_lowercased[*s2]); s1++, s2++)
        if (!c)                                // strings are equal
            break;
    return res;
}

/*! \relates QByteArray

    A safe \c strnicmp() function.

    Compares at most \a len bytes of \a str1 and \a str2 ignoring the
    case of the characters. The encoding of the strings is assumed to
    be Latin-1.

    Returns a negative value if \a str1 is less than \a str2, 0 if \a str1
    is equal to \a str2 or a positive value if \a str1 is greater than \a
    str2.

    Special case 1: Returns 0 if \a str1 and \a str2 are both 0.

    Special case 2: Returns a random non-zero value if \a str1 is 0
    or \a str2 is 0 (but not both).

    \sa qstrcmp(), qstrncmp(), qstricmp(), {8-bit Character Comparisons}
*/

int qstrnicmp(const char *str1, const char *str2, uint len)
{
    const uchar *s1 = reinterpret_cast<const uchar *>(str1);
    const uchar *s2 = reinterpret_cast<const uchar *>(str2);
    int res;
    uchar c;
    if (!s1 || !s2)
        return s1 ? 1 : (s2 ? -1 : 0);
    for (; len--; s1++, s2++) {
        if ((res = (c = latin1_lowercased[*s1]) - latin1_lowercased[*s2]))
            return res;
        if (!c)                                // strings are equal
            break;
    }
    return 0;
}

/*!
    \internal
 */
int qstrcmp(const QByteArray &str1, const char *str2)
{
    if (!str2)
        return str1.isEmpty() ? 0 : +1;

    const char *str1data = str1.constData();
    const char *str1end = str1data + str1.length();
    for ( ; str1data < str1end && *str2; ++str1data, ++str2) {
        int diff = int(uchar(*str1data)) - uchar(*str2);
        if (diff)
            // found a difference
            return diff;
    }

    // Why did we stop?
    if (*str2 != '\0')
        // not the null, so we stopped because str1 is shorter
        return -1;
    if (str1data < str1end)
        // we haven't reached the end, so str1 must be longer
        return +1;
    return 0;
}

/*!
    \internal
 */
int qstrcmp(const QByteArray &str1, const QByteArray &str2)
{
    int l1 = str1.length();
    int l2 = str2.length();
    int ret = memcmp(str1.constData(), str2.constData(), qMin(l1, l2));
    if (ret != 0)
        return ret;

    // they matched qMin(l1, l2) bytes
    // so the longer one is lexically after the shorter one
    return l1 - l2;
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

    Returns the CRC-16 checksum of the first \a len bytes of \a data.

    The checksum is independent of the byte order (endianness).

    \note This function is a 16-bit cache conserving (16 entry table)
    implementation of the CRC-16-CCITT algorithm.
*/

quint16 qChecksum(const char *data, uint len)
{
    quint16 crc = 0xffff;
    uchar c;
    const uchar *p = reinterpret_cast<const uchar *>(data);
    while (len--) {
        c = *p++;
        crc = ((crc >> 4) & 0x0fff) ^ crc_tbl[((crc ^ c) & 15)];
        c >>= 4;
        crc = ((crc >> 4) & 0x0fff) ^ crc_tbl[((crc ^ c) & 15)];
    }
    return ~crc & 0xffff;
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

    \sa qUncompress()
*/

/*! \relates QByteArray

    \overload

    Compresses the first \a nbytes of \a data at compression level
    \a compressionLevel and returns the compressed data in a new byte array.
*/

#ifndef QT_NO_COMPRESS
QByteArray qCompress(const uchar* data, int nbytes, int compressionLevel)
{
    if (nbytes == 0) {
        return QByteArray(4, '\0');
    }
    if (!data) {
        qWarning("qCompress: Data is null");
        return QByteArray();
    }
    if (compressionLevel < -1 || compressionLevel > 9)
        compressionLevel = -1;

    ulong len = nbytes + nbytes / 100 + 13;
    QByteArray bazip;
    int res;
    do {
        bazip.resize(len + 4);
        res = ::compress2((uchar*)bazip.data()+4, &len, data, nbytes, compressionLevel);

        switch (res) {
        case Z_OK:
            bazip.resize(len + 4);
            bazip[0] = (nbytes & 0xff000000) >> 24;
            bazip[1] = (nbytes & 0x00ff0000) >> 16;
            bazip[2] = (nbytes & 0x0000ff00) >> 8;
            bazip[3] = (nbytes & 0x000000ff);
            break;
        case Z_MEM_ERROR:
            qWarning("qCompress: Z_MEM_ERROR: Not enough memory");
            bazip.resize(0);
            break;
        case Z_BUF_ERROR:
            len *= 2;
            break;
        }
    } while (res == Z_BUF_ERROR);

    return bazip;
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
    expressed as an unsigned, big-endian, 32-bit integer.

    \sa qCompress()
*/

/*! \relates QByteArray

    \overload

    Uncompresses the first \a nbytes of \a data and returns a new byte
    array with the uncompressed data.
*/

#ifndef QT_NO_COMPRESS
QByteArray qUncompress(const uchar* data, int nbytes)
{
    if (!data) {
        qWarning("qUncompress: Data is null");
        return QByteArray();
    }
    if (nbytes <= 4) {
        if (nbytes < 4 || (data[0]!=0 || data[1]!=0 || data[2]!=0 || data[3]!=0))
            qWarning("qUncompress: Input data is corrupted");
        return QByteArray();
    }
    ulong expectedSize = uint((data[0] << 24) | (data[1] << 16) |
                              (data[2] <<  8) | (data[3]      ));
    ulong len = qMax(expectedSize, 1ul);
    QScopedPointer<QByteArray::Data, QScopedPointerPodDeleter> d;

    forever {
        ulong alloc = len;
        if (len  >= (1u << 31u) - sizeof(QByteArray::Data)) {
            //QByteArray does not support that huge size anyway.
            qWarning("qUncompress: Input data is corrupted");
            return QByteArray();
        }
        QByteArray::Data *p = static_cast<QByteArray::Data *>(::realloc(d.data(), sizeof(QByteArray::Data) + alloc + 1));
        if (!p) {
            // we are not allowed to crash here when compiling with QT_NO_EXCEPTIONS
            qWarning("qUncompress: could not allocate enough memory to uncompress data");
            return QByteArray();
        }
        d.take(); // realloc was successful
        d.reset(p);
        d->offset = sizeof(QByteArrayData);
        d->size = 0; // Shut up valgrind "uninitialized variable" warning

        int res = ::uncompress((uchar*)d->data(), &len,
                               data+4, nbytes-4);

        switch (res) {
        case Z_OK:
            if (len != alloc) {
                if (len  >= (1u << 31u) - sizeof(QByteArray::Data)) {
                    //QByteArray does not support that huge size anyway.
                    qWarning("qUncompress: Input data is corrupted");
                    return QByteArray();
                }
                QByteArray::Data *p = static_cast<QByteArray::Data *>(::realloc(d.data(), sizeof(QByteArray::Data) + len + 1));
                if (!p) {
                    // we are not allowed to crash here when compiling with QT_NO_EXCEPTIONS
                    qWarning("qUncompress: could not allocate enough memory to uncompress data");
                    return QByteArray();
                }
                d.take(); // realloc was successful
                d.reset(p);
            }
            d->ref.initializeOwned();
            d->size = len;
            d->alloc = uint(len) + 1u;
            d->capacityReserved = false;
            d->offset = sizeof(QByteArrayData);
            d->data()[len] = 0;

            {
                QByteArrayDataPtr dataPtr = { d.take() };
                return QByteArray(dataPtr);
            }

        case Z_MEM_ERROR:
            qWarning("qUncompress: Z_MEM_ERROR: Not enough memory");
            return QByteArray();

        case Z_BUF_ERROR:
            len *= 2;
            continue;

        case Z_DATA_ERROR:
            qWarning("qUncompress: Z_DATA_ERROR: Input data is corrupted");
            return QByteArray();
        }
    }
}
#endif

static inline bool qIsUpper(char c)
{
    return c >= 'A' && c <= 'Z';
}

static inline char qToLower(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 'a';
    else
        return c;
}

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

    In addition to QByteArray, Qt also provides the QString class to
    store string data. For most purposes, QString is the class you
    want to use. It stores 16-bit Unicode characters, making it easy
    to store non-ASCII/non-Latin-1 characters in your application.
    Furthermore, QString is used throughout in the Qt API. The two
    main cases where QByteArray is appropriate are when you need to
    store raw binary data, and when memory conservation is critical
    (e.g., with Qt for Embedded Linux).

    One way to initialize a QByteArray is simply to pass a \c{const
    char *} to its constructor. For example, the following code
    creates a byte array of size 5 containing the data "Hello":

    \snippet code/src_corelib_tools_qbytearray.cpp 0

    Although the size() is 5, the byte array also maintains an extra
    '\\0' character at the end so that if a function is used that
    asks for a pointer to the underlying data (e.g. a call to
    data()), the data pointed to is guaranteed to be
    '\\0'-terminated.

    QByteArray makes a deep copy of the \c{const char *} data, so you
    can modify it later without experiencing side effects. (If for
    performance reasons you don't want to take a deep copy of the
    character data, use QByteArray::fromRawData() instead.)

    Another approach is to set the size of the array using resize()
    and to initialize the data byte per byte. QByteArray uses 0-based
    indexes, just like C++ arrays. To access the byte at a particular
    index position, you can use operator[](). On non-const byte
    arrays, operator[]() returns a reference to a byte that can be
    used on the left side of an assignment. For example:

    \snippet code/src_corelib_tools_qbytearray.cpp 1

    For read-only access, an alternative syntax is to use at():

    \snippet code/src_corelib_tools_qbytearray.cpp 2

    at() can be faster than operator[](), because it never causes a
    \l{deep copy} to occur.

    To extract many bytes at a time, use left(), right(), or mid().

    A QByteArray can embed '\\0' bytes. The size() function always
    returns the size of the whole array, including embedded '\\0'
    bytes, but excluding the terminating '\\0' added by QByteArray.
    For example:

    \snippet code/src_corelib_tools_qbytearray.cpp 48

    If you want to obtain the length of the data up to and
    excluding the first '\\0' character, call qstrlen() on the byte
    array.

    After a call to resize(), newly allocated bytes have undefined
    values. To set all the bytes to a particular value, call fill().

    To obtain a pointer to the actual character data, call data() or
    constData(). These functions return a pointer to the beginning of the data.
    The pointer is guaranteed to remain valid until a non-const function is
    called on the QByteArray. It is also guaranteed that the data ends with a
    '\\0' byte unless the QByteArray was created from a \l{fromRawData()}{raw
    data}. This '\\0' byte is automatically provided by QByteArray and is not
    counted in size().

    QByteArray provides the following basic functions for modifying
    the byte data: append(), prepend(), insert(), replace(), and
    remove(). For example:

    \snippet code/src_corelib_tools_qbytearray.cpp 3

    The replace() and remove() functions' first two arguments are the
    position from which to start erasing and the number of bytes that
    should be erased.

    When you append() data to a non-empty array, the array will be
    reallocated and the new data copied to it. You can avoid this
    behavior by calling reserve(), which preallocates a certain amount
    of memory. You can also call capacity() to find out how much
    memory QByteArray actually allocated. Data appended to an empty
    array is not copied.

    A frequent requirement is to remove whitespace characters from a
    byte array ('\\n', '\\t', ' ', etc.). If you want to remove
    whitespace from both ends of a QByteArray, use trimmed(). If you
    want to remove whitespace from both ends and replace multiple
    consecutive whitespaces with a single space character within the
    byte array, use simplified().

    If you want to find all occurrences of a particular character or
    substring in a QByteArray, use indexOf() or lastIndexOf(). The
    former searches forward starting from a given index position, the
    latter searches backward. Both return the index position of the
    character or substring if they find it; otherwise, they return -1.
    For example, here's a typical loop that finds all occurrences of a
    particular substring:

    \snippet code/src_corelib_tools_qbytearray.cpp 4

    If you simply want to check whether a QByteArray contains a
    particular character or substring, use contains(). If you want to
    find out how many times a particular character or substring
    occurs in the byte array, use count(). If you want to replace all
    occurrences of a particular value with another, use one of the
    two-parameter replace() overloads.

    \l{QByteArray}s can be compared using overloaded operators such as
    operator<(), operator<=(), operator==(), operator>=(), and so on.
    The comparison is based exclusively on the numeric values
    of the characters and is very fast, but is not what a human would
    expect. QString::localeAwareCompare() is a better choice for
    sorting user-interface strings.

    For historical reasons, QByteArray distinguishes between a null
    byte array and an empty byte array. A \e null byte array is a
    byte array that is initialized using QByteArray's default
    constructor or by passing (const char *)0 to the constructor. An
    \e empty byte array is any byte array with size 0. A null byte
    array is always empty, but an empty byte array isn't necessarily
    null:

    \snippet code/src_corelib_tools_qbytearray.cpp 5

    All functions except isNull() treat null byte arrays the same as
    empty byte arrays. For example, data() returns a pointer to a
    '\\0' character for a null byte array (\e not a null pointer),
    and QByteArray() compares equal to QByteArray(""). We recommend
    that you always use isEmpty() and avoid isNull().

    \section1 Notes on Locale

    \section2 Number-String Conversions

    Functions that perform conversions between numeric data types and
    strings are performed in the C locale, irrespective of the user's
    locale settings. Use QString to perform locale-aware conversions
    between numbers and strings.

    \section2 8-bit Character Comparisons

    In QByteArray, the notion of uppercase and lowercase and of which
    character is greater than or less than another character is
    locale dependent. This affects functions that support a case
    insensitive option or that compare or lowercase or uppercase
    their arguments. Case insensitive operations and comparisons will
    be accurate if both strings contain only ASCII characters. (If \c
    $LC_CTYPE is set, most Unix systems do "the right thing".)
    Functions that this affects include contains(), indexOf(),
    lastIndexOf(), operator<(), operator<=(), operator>(),
    operator>=(), toLower() and toUpper().

    This issue does not apply to \l{QString}s since they represent
    characters using Unicode.

    \sa QString, QBitArray
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

    QByteArray::fromBase64() ignores the KeepTrailingEquals and
    OmitTrailingEquals options and will not flag errors in case they are
    missing or if there are too many of them.
*/

/*! \fn QByteArray::iterator QByteArray::begin()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the first character in
    the byte-array.

    \sa constBegin(), end()
*/

/*! \fn QByteArray::const_iterator QByteArray::begin() const

    \overload begin()
*/

/*! \fn QByteArray::const_iterator QByteArray::cbegin() const
    \since 5.0

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first character
    in the byte-array.

    \sa begin(), cend()
*/

/*! \fn QByteArray::const_iterator QByteArray::constBegin() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first character
    in the byte-array.

    \sa begin(), constEnd()
*/

/*! \fn QByteArray::iterator QByteArray::end()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the imaginary character
    after the last character in the byte-array.

    \sa begin(), constEnd()
*/

/*! \fn QByteArray::const_iterator QByteArray::end() const

    \overload end()
*/

/*! \fn QByteArray::const_iterator QByteArray::cend() const
    \since 5.0

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    character after the last character in the list.

    \sa cbegin(), end()
*/

/*! \fn QByteArray::const_iterator QByteArray::constEnd() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    character after the last character in the list.

    \sa constBegin(), end()
*/

/*! \fn QByteArray::reverse_iterator QByteArray::rbegin()
    \since 5.6

    Returns a \l{STL-style iterators}{STL-style} reverse iterator pointing to the first
    character in the byte-array, in reverse order.

    \sa begin(), crbegin(), rend()
*/

/*! \fn QByteArray::const_reverse_iterator QByteArray::rbegin() const
    \since 5.6
    \overload
*/

/*! \fn QByteArray::const_reverse_iterator QByteArray::crbegin() const
    \since 5.6

    Returns a const \l{STL-style iterators}{STL-style} reverse iterator pointing to the first
    character in the byte-array, in reverse order.

    \sa begin(), rbegin(), rend()
*/

/*! \fn QByteArray::reverse_iterator QByteArray::rend()
    \since 5.6

    Returns a \l{STL-style iterators}{STL-style} reverse iterator pointing to one past
    the last character in the byte-array, in reverse order.

    \sa end(), crend(), rbegin()
*/

/*! \fn QByteArray::const_reverse_iterator QByteArray::rend() const
    \since 5.6
    \overload
*/

/*! \fn QByteArray::const_reverse_iterator QByteArray::crend() const
    \since 5.6

    Returns a const \l{STL-style iterators}{STL-style} reverse iterator pointing to one
    past the last character in the byte-array, in reverse order.

    \sa end(), rend(), rbegin()
*/

/*! \fn void QByteArray::push_back(const QByteArray &other)

    This function is provided for STL compatibility. It is equivalent
    to append(\a other).
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

/*! \fn void QByteArray::push_front(const char *str)

    \overload

    Same as prepend(\a str).
*/

/*! \fn void QByteArray::push_front(char ch)

    \overload

    Same as prepend(\a ch).
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
QByteArray &QByteArray::operator=(const QByteArray & other) Q_DECL_NOTHROW
{
    other.d->ref.ref();
    if (!d->ref.deref())
        Data::deallocate(d);
    d = other.d;
    return *this;
}


/*!
    \overload

    Assigns \a str to this byte array.
*/

QByteArray &QByteArray::operator=(const char *str)
{
    Data *x;
    if (!str) {
        x = Data::sharedNull();
    } else if (!*str) {
        x = Data::allocate(0);
    } else {
        const int len = int(strlen(str));
        const uint fullLen = len + 1;
        if (d->ref.isShared() || fullLen > d->alloc
                || (len < d->size && fullLen < uint(d->alloc >> 1)))
            reallocData(fullLen, d->detachFlags());
        x = d;
        memcpy(x->data(), str, fullLen); // include null terminator
        x->size = len;
    }
    x->ref.ref();
    if (!d->ref.deref())
         Data::deallocate(d);
    d = x;
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

/*! \fn int QByteArray::size() const

    Returns the number of bytes in this byte array.

    The last byte in the byte array is at position size() - 1. In addition,
    QByteArray ensures that the byte at position size() is always '\\0', so
    that you can use the return value of data() and constData() as arguments to
    functions that expect '\\0'-terminated strings. If the QByteArray object
    was created from a \l{fromRawData()}{raw data} that didn't include the
    trailing null-termination character then QByteArray doesn't add it
    automaticall unless the \l{deep copy} is created.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 6

    \sa isEmpty(), resize()
*/

/*! \fn bool QByteArray::isEmpty() const

    Returns \c true if the byte array has size 0; otherwise returns \c false.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 7

    \sa size()
*/

/*! \fn int QByteArray::capacity() const

    Returns the maximum number of bytes that can be stored in the
    byte array without forcing a reallocation.

    The sole purpose of this function is to provide a means of fine
    tuning QByteArray's memory usage. In general, you will rarely
    ever need to call this function. If you want to know how many
    bytes are in the byte array, call size().

    \sa reserve(), squeeze()
*/

/*! \fn void QByteArray::reserve(int size)

    Attempts to allocate memory for at least \a size bytes. If you
    know in advance how large the byte array will be, you can call
    this function, and if you call resize() often you are likely to
    get better performance. If \a size is an underestimate, the worst
    that will happen is that the QByteArray will be a bit slower.

    The sole purpose of this function is to provide a means of fine
    tuning QByteArray's memory usage. In general, you will rarely
    ever need to call this function. If you want to change the size
    of the byte array, call resize().

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

    \obsolete Use constData() instead.

    Returns a pointer to the data stored in the byte array. The
    pointer can be used to access the bytes that compose the array.
    The data is '\\0'-terminated. The pointer remains valid as long
    as the array isn't reallocated or destroyed.

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

    Returns a pointer to the data stored in the byte array. The
    pointer can be used to access and modify the bytes that compose
    the array. The data is '\\0'-terminated, i.e. the number of
    bytes in the returned character string is size() + 1 for the
    '\\0' terminator.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 8

    The pointer remains valid as long as the byte array isn't
    reallocated or destroyed. For read-only access, constData() is
    faster because it never causes a \l{deep copy} to occur.

    This function is mostly useful to pass a byte array to a function
    that accepts a \c{const char *}.

    The following example makes a copy of the char* returned by
    data(), but it will corrupt the heap and cause a crash because it
    does not allocate a byte for the '\\0' at the end:

    \snippet code/src_corelib_tools_qbytearray.cpp 46

    This one allocates the correct amount of space:

    \snippet code/src_corelib_tools_qbytearray.cpp 47

    Note: A QByteArray can store any byte values including '\\0's,
    but most functions that take \c{char *} arguments assume that the
    data ends at the first '\\0' they encounter.

    \sa constData(), operator[]()
*/

/*! \fn const char *QByteArray::data() const

    \overload
*/

/*! \fn const char *QByteArray::constData() const

    Returns a pointer to the data stored in the byte array. The pointer can be
    used to access the bytes that compose the array. The data is
    '\\0'-terminated unless the QByteArray object was created from raw data.
    The pointer remains valid as long as the byte array isn't reallocated or
    destroyed.

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

/*! \fn char QByteArray::at(int i) const

    Returns the character at index position \a i in the byte array.

    \a i must be a valid index position in the byte array (i.e., 0 <=
    \a i < size()).

    \sa operator[]()
*/

/*! \fn QByteRef QByteArray::operator[](int i)

    Returns the byte at index position \a i as a modifiable reference.

    If an assignment is made beyond the end of the byte array, the
    array is extended with resize() before the assignment takes
    place.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 9

    The return value is of type QByteRef, a helper class for
    QByteArray. When you get an object of type QByteRef, you can use
    it as if it were a char &. If you assign to it, the assignment
    will apply to the character in the QByteArray from which you got
    the reference.

    \sa at()
*/

/*! \fn char QByteArray::operator[](int i) const

    \overload

    Same as at(\a i).
*/

/*! \fn QByteRef QByteArray::operator[](uint i)

    \overload
*/

/*! \fn char QByteArray::operator[](uint i) const

    \overload
*/

/*! \fn bool QByteArray::contains(const QByteArray &ba) const

    Returns \c true if the byte array contains an occurrence of the byte
    array \a ba; otherwise returns \c false.

    \sa indexOf(), count()
*/

/*! \fn bool QByteArray::contains(const char *str) const

    \overload

    Returns \c true if the byte array contains the string \a str;
    otherwise returns \c false.
*/

/*! \fn bool QByteArray::contains(char ch) const

    \overload

    Returns \c true if the byte array contains the character \a ch;
    otherwise returns \c false.
*/

/*!

    Truncates the byte array at index position \a pos.

    If \a pos is beyond the end of the array, nothing happens.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 10

    \sa chop(), resize(), left()
*/
void QByteArray::truncate(int pos)
{
    if (pos < d->size)
        resize(pos);
}

/*!

    Removes \a n bytes from the end of the byte array.

    If \a n is greater than size(), the result is an empty byte
    array.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 11

    \sa truncate(), resize(), left()
*/

void QByteArray::chop(int n)
{
    if (n > 0)
        resize(d->size - n);
}


/*! \fn QByteArray &QByteArray::operator+=(const QByteArray &ba)

    Appends the byte array \a ba onto the end of this byte array and
    returns a reference to this byte array.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 12

    Note: QByteArray is an \l{implicitly shared} class. Consequently,
    if \e this is an empty QByteArray, then \e this will just share
    the data held in \a ba. In this case, no copying of data is done,
    taking \l{constant time}. If a shared instance is modified, it will
    be copied (copy-on-write), taking \l{linear time}.

    If \e this is not an empty QByteArray, a deep copy of the data is
    performed, taking \l{linear time}.

    This operation typically does not suffer from allocation overhead,
    because QByteArray preallocates extra space at the end of the data
    so that it may grow without reallocating for each append operation.

    \sa append(), prepend()
*/

/*! \fn QByteArray &QByteArray::operator+=(const QString &str)

    \overload

    Appends the string \a str onto the end of this byte array and
    returns a reference to this byte array. The Unicode data is
    converted into 8-bit characters using QString::toUtf8().

    You can disable this function by defining \c QT_NO_CAST_TO_ASCII when you
    compile your applications. You then need to call QString::toUtf8() (or
    QString::toLatin1() or QString::toLocal8Bit()) explicitly if you want to
    convert the data to \c{const char *}.
*/

/*! \fn QByteArray &QByteArray::operator+=(const char *str)

    \overload

    Appends the string \a str onto the end of this byte array and
    returns a reference to this byte array.
*/

/*! \fn QByteArray &QByteArray::operator+=(char ch)

    \overload

    Appends the character \a ch onto the end of this byte array and
    returns a reference to this byte array.
*/

/*! \fn int QByteArray::length() const

    Same as size().
*/

/*! \fn bool QByteArray::isNull() const

    Returns \c true if this byte array is null; otherwise returns \c false.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 13

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

    If \a size is negative, \a data is assumed to point to a nul-terminated
    string and its length is determined dynamically. The terminating
    nul-character is not considered part of the byte array.

    QByteArray makes a deep copy of the string data.

    \sa fromRawData()
*/

QByteArray::QByteArray(const char *data, int size)
{
    if (!data) {
        d = Data::sharedNull();
    } else {
        if (size < 0)
            size = int(strlen(data));
        if (!size) {
            d = Data::allocate(0);
        } else {
            d = Data::allocate(uint(size) + 1u);
            Q_CHECK_PTR(d);
            d->size = size;
            memcpy(d->data(), data, size);
            d->data()[size] = '\0';
        }
    }
}

/*!
    Constructs a byte array of size \a size with every byte set to
    character \a ch.

    \sa fill()
*/

QByteArray::QByteArray(int size, char ch)
{
    if (size <= 0) {
        d = Data::allocate(0);
    } else {
        d = Data::allocate(uint(size) + 1u);
        Q_CHECK_PTR(d);
        d->size = size;
        memset(d->data(), ch, size);
        d->data()[size] = '\0';
    }
}

/*!
    \internal

    Constructs a byte array of size \a size with uninitialized contents.
*/

QByteArray::QByteArray(int size, Qt::Initialization)
{
    d = Data::allocate(uint(size) + 1u);
    Q_CHECK_PTR(d);
    d->size = size;
    d->data()[size] = '\0';
}

/*!
    Sets the size of the byte array to \a size bytes.

    If \a size is greater than the current size, the byte array is
    extended to make it \a size bytes with the extra bytes added to
    the end. The new bytes are uninitialized.

    If \a size is less than the current size, bytes are removed from
    the end.

    \sa size(), truncate()
*/
void QByteArray::resize(int size)
{
    if (size < 0)
        size = 0;

    if (IS_RAW_DATA(d) && !d->ref.isShared() && size < d->size) {
        d->size = size;
        return;
    }

    if (size == 0 && !d->capacityReserved) {
        Data *x = Data::allocate(0);
        if (!d->ref.deref())
            Data::deallocate(d);
        d = x;
    } else if (d->size == 0 && d->ref.isStatic()) {
        //
        // Optimize the idiom:
        //    QByteArray a;
        //    a.resize(sz);
        //    ...
        // which is used in place of the Qt 3 idiom:
        //    QByteArray a(sz);
        //
        Data *x = Data::allocate(uint(size) + 1u);
        Q_CHECK_PTR(x);
        x->size = size;
        x->data()[size] = '\0';
        d = x;
    } else {
        if (d->ref.isShared() || uint(size) + 1u > d->alloc
                || (!d->capacityReserved && size < d->size
                    && uint(size) + 1u < uint(d->alloc >> 1)))
            reallocData(uint(size) + 1u, d->detachFlags() | Data::Grow);
        if (d->alloc) {
            d->size = size;
            d->data()[size] = '\0';
        }
    }
}

/*!
    Sets every byte in the byte array to character \a ch. If \a size
    is different from -1 (the default), the byte array is resized to
    size \a size beforehand.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 14

    \sa resize()
*/

QByteArray &QByteArray::fill(char ch, int size)
{
    resize(size < 0 ? d->size : size);
    if (d->size)
        memset(d->data(), ch, d->size);
    return *this;
}

void QByteArray::reallocData(uint alloc, Data::AllocationOptions options)
{
    if (d->ref.isShared() || IS_RAW_DATA(d)) {
        Data *x = Data::allocate(alloc, options);
        Q_CHECK_PTR(x);
        x->size = qMin(int(alloc) - 1, d->size);
        ::memcpy(x->data(), d->data(), x->size);
        x->data()[x->size] = '\0';
        if (!d->ref.deref())
            Data::deallocate(d);
        d = x;
    } else {
        size_t blockSize;
        if (options & Data::Grow) {
            auto r = qCalculateGrowingBlockSize(alloc, sizeof(QChar), sizeof(Data));
            blockSize = r.size;
            alloc = uint(r.elementCount);
        } else {
            blockSize = qCalculateBlockSize(alloc, sizeof(QChar), sizeof(Data));
        }

        Data *x = static_cast<Data *>(::realloc(d, blockSize));
        Q_CHECK_PTR(x);
        x->alloc = alloc;
        x->capacityReserved = (options & Data::CapacityReserved) ? 1 : 0;
        d = x;
    }
}

void QByteArray::expand(int i)
{
    resize(qMax(i + 1, d->size));
}

/*!
   \internal
   Return a QByteArray that is sure to be NUL-terminated.

   By default, all QByteArray have an extra NUL at the end,
   guaranteeing that assumption. However, if QByteArray::fromRawData
   is used, then the NUL is there only if the user put it there. We
   can't be sure.
*/
QByteArray QByteArray::nulTerminated() const
{
    // is this fromRawData?
    if (!IS_RAW_DATA(d))
        return *this;           // no, then we're sure we're zero terminated

    QByteArray copy(*this);
    copy.detach();
    return copy;
}

/*!
    Prepends the byte array \a ba to this byte array and returns a
    reference to this byte array.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 15

    This is the same as insert(0, \a ba).

    Note: QByteArray is an \l{implicitly shared} class. Consequently,
    if \e this is an empty QByteArray, then \e this will just share
    the data held in \a ba. In this case, no copying of data is done,
    taking \l{constant time}. If a shared instance is modified, it will
    be copied (copy-on-write), taking \l{linear time}.

    If \e this is not an empty QByteArray, a deep copy of the data is
    performed, taking \l{linear time}.

    \sa append(), insert()
*/

QByteArray &QByteArray::prepend(const QByteArray &ba)
{
    if (d->size == 0 && d->ref.isStatic() && !IS_RAW_DATA(ba.d)) {
        *this = ba;
    } else if (ba.d->size != 0) {
        QByteArray tmp = *this;
        *this = ba;
        append(tmp);
    }
    return *this;
}

/*!
    \overload

    Prepends the string \a str to this byte array.
*/

QByteArray &QByteArray::prepend(const char *str)
{
    return prepend(str, qstrlen(str));
}

/*!
    \overload
    \since 4.6

    Prepends \a len bytes of the string \a str to this byte array.
*/

QByteArray &QByteArray::prepend(const char *str, int len)
{
    if (str) {
        if (d->ref.isShared() || uint(d->size + len) + 1u > d->alloc)
            reallocData(uint(d->size + len) + 1u, d->detachFlags() | Data::Grow);
        memmove(d->data()+len, d->data(), d->size);
        memcpy(d->data(), str, len);
        d->size += len;
        d->data()[d->size] = '\0';
    }
    return *this;
}

/*! \fn QByteArray &QByteArray::prepend(int count, char ch)

    \overload
    \since 5.7

    Prepends \a count copies of character \a ch to this byte array.
*/

/*!
    \overload

    Prepends the character \a ch to this byte array.
*/

QByteArray &QByteArray::prepend(char ch)
{
    if (d->ref.isShared() || uint(d->size) + 2u > d->alloc)
        reallocData(uint(d->size) + 2u, d->detachFlags() | Data::Grow);
    memmove(d->data()+1, d->data(), d->size);
    d->data()[0] = ch;
    ++d->size;
    d->data()[d->size] = '\0';
    return *this;
}

/*!
    Appends the byte array \a ba onto the end of this byte array.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 16

    This is the same as insert(size(), \a ba).

    Note: QByteArray is an \l{implicitly shared} class. Consequently,
    if \e this is an empty QByteArray, then \e this will just share
    the data held in \a ba. In this case, no copying of data is done,
    taking \l{constant time}. If a shared instance is modified, it will
    be copied (copy-on-write), taking \l{linear time}.

    If \e this is not an empty QByteArray, a deep copy of the data is
    performed, taking \l{linear time}.

    This operation typically does not suffer from allocation overhead,
    because QByteArray preallocates extra space at the end of the data
    so that it may grow without reallocating for each append operation.

    \sa operator+=(), prepend(), insert()
*/

QByteArray &QByteArray::append(const QByteArray &ba)
{
    if (d->size == 0 && d->ref.isStatic() && !IS_RAW_DATA(ba.d)) {
        *this = ba;
    } else if (ba.d->size != 0) {
        if (d->ref.isShared() || uint(d->size + ba.d->size) + 1u > d->alloc)
            reallocData(uint(d->size + ba.d->size) + 1u, d->detachFlags() | Data::Grow);
        memcpy(d->data() + d->size, ba.d->data(), ba.d->size);
        d->size += ba.d->size;
        d->data()[d->size] = '\0';
    }
    return *this;
}

/*! \fn QByteArray &QByteArray::append(const QString &str)

    \overload

    Appends the string \a str to this byte array. The Unicode data is
    converted into 8-bit characters using QString::toUtf8().

    You can disable this function by defining \c QT_NO_CAST_TO_ASCII when you
    compile your applications. You then need to call QString::toUtf8() (or
    QString::toLatin1() or QString::toLocal8Bit()) explicitly if you want to
    convert the data to \c{const char *}.
*/

/*!
    \overload

    Appends the string \a str to this byte array.
*/

QByteArray& QByteArray::append(const char *str)
{
    if (str) {
        const int len = int(strlen(str));
        if (d->ref.isShared() || uint(d->size + len) + 1u > d->alloc)
            reallocData(uint(d->size + len) + 1u, d->detachFlags() | Data::Grow);
        memcpy(d->data() + d->size, str, len + 1); // include null terminator
        d->size += len;
    }
    return *this;
}

/*!
    \overload append()

    Appends the first \a len characters of the string \a str to this byte
    array and returns a reference to this byte array.

    If \a len is negative, the length of the string will be determined
    automatically using qstrlen(). If \a len is zero or \a str is
    null, nothing is appended to the byte array. Ensure that \a len is
    \e not longer than \a str.
*/

QByteArray &QByteArray::append(const char *str, int len)
{
    if (len < 0)
        len = qstrlen(str);
    if (str && len) {
        if (d->ref.isShared() || uint(d->size + len) + 1u > d->alloc)
            reallocData(uint(d->size + len) + 1u, d->detachFlags() | Data::Grow);
        memcpy(d->data() + d->size, str, len); // include null terminator
        d->size += len;
        d->data()[d->size] = '\0';
    }
    return *this;
}

/*! \fn QByteArray &QByteArray::append(int count, char ch)

    \overload
    \since 5.7

    Appends \a count copies of character \a ch to this byte
    array and returns a reference to this byte array.

    If \a count is negative or zero nothing is appended to the byte array.
*/

/*!
    \overload

    Appends the character \a ch to this byte array.
*/

QByteArray& QByteArray::append(char ch)
{
    if (d->ref.isShared() || uint(d->size) + 2u > d->alloc)
        reallocData(uint(d->size) + 2u, d->detachFlags() | Data::Grow);
    d->data()[d->size++] = ch;
    d->data()[d->size] = '\0';
    return *this;
}

/*!
  \internal
  Inserts \a len bytes from the array \a arr at position \a pos and returns a
  reference the modified byte array.
*/
static inline QByteArray &qbytearray_insert(QByteArray *ba,
                                            int pos, const char *arr, int len)
{
    Q_ASSERT(pos >= 0);

    if (pos < 0 || len <= 0 || arr == 0)
        return *ba;

    int oldsize = ba->size();
    ba->resize(qMax(pos, oldsize) + len);
    char *dst = ba->data();
    if (pos > oldsize)
        ::memset(dst + oldsize, 0x20, pos - oldsize);
    else
        ::memmove(dst + pos + len, dst + pos, oldsize - pos);
    memcpy(dst + pos, arr, len);
    return *ba;
}

/*!
    Inserts the byte array \a ba at index position \a i and returns a
    reference to this byte array.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 17

    \sa append(), prepend(), replace(), remove()
*/

QByteArray &QByteArray::insert(int i, const QByteArray &ba)
{
    QByteArray copy(ba);
    return qbytearray_insert(this, i, copy.d->data(), copy.d->size);
}

/*!
    \fn QByteArray &QByteArray::insert(int i, const QString &str)

    \overload

    Inserts the string \a str at index position \a i in the byte
    array. The Unicode data is converted into 8-bit characters using
    QString::toUtf8().

    If \a i is greater than size(), the array is first extended using
    resize().

    You can disable this function by defining \c QT_NO_CAST_TO_ASCII when you
    compile your applications. You then need to call QString::toUtf8() (or
    QString::toLatin1() or QString::toLocal8Bit()) explicitly if you want to
    convert the data to \c{const char *}.
*/

/*!
    \overload

    Inserts the string \a str at position \a i in the byte array.

    If \a i is greater than size(), the array is first extended using
    resize().
*/

QByteArray &QByteArray::insert(int i, const char *str)
{
    return qbytearray_insert(this, i, str, qstrlen(str));
}

/*!
    \overload
    \since 4.6

    Inserts \a len bytes of the string \a str at position
    \a i in the byte array.

    If \a i is greater than size(), the array is first extended using
    resize().
*/

QByteArray &QByteArray::insert(int i, const char *str, int len)
{
    return qbytearray_insert(this, i, str, len);
}

/*!
    \overload

    Inserts character \a ch at index position \a i in the byte array.
    If \a i is greater than size(), the array is first extended using
    resize().
*/

QByteArray &QByteArray::insert(int i, char ch)
{
    return qbytearray_insert(this, i, &ch, 1);
}

/*! \fn QByteArray &QByteArray::insert(int i, int count, char ch)

    \overload
    \since 5.7

    Inserts \a count copies of character \a ch at index position \a i in the
    byte array.

    If \a i is greater than size(), the array is first extended using resize().
*/

QByteArray &QByteArray::insert(int i, int count, char ch)
{
    if (i < 0 || count <= 0)
        return *this;

    int oldsize = size();
    resize(qMax(i, oldsize) + count);
    char *dst = d->data();
    if (i > oldsize)
        ::memset(dst + oldsize, 0x20, i - oldsize);
    else if (i < oldsize)
        ::memmove(dst + i + count, dst + i, oldsize - i);
    ::memset(dst + i, ch, count);
    return *this;
}

/*!
    Removes \a len bytes from the array, starting at index position \a
    pos, and returns a reference to the array.

    If \a pos is out of range, nothing happens. If \a pos is valid,
    but \a pos + \a len is larger than the size of the array, the
    array is truncated at position \a pos.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 18

    \sa insert(), replace()
*/

QByteArray &QByteArray::remove(int pos, int len)
{
    if (len <= 0  || uint(pos) >= uint(d->size))
        return *this;
    detach();
    if (len >= d->size - pos) {
        resize(pos);
    } else {
        memmove(d->data() + pos, d->data() + pos + len, d->size - pos - len);
        resize(d->size - len);
    }
    return *this;
}

/*!
    Replaces \a len bytes from index position \a pos with the byte
    array \a after, and returns a reference to this byte array.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 19

    \sa insert(), remove()
*/

QByteArray &QByteArray::replace(int pos, int len, const QByteArray &after)
{
    if (len == after.d->size && (pos + len <= d->size)) {
        detach();
        memmove(d->data() + pos, after.d->data(), len*sizeof(char));
        return *this;
    } else {
        QByteArray copy(after);
        // ### optimize me
        remove(pos, len);
        return insert(pos, copy);
    }
}

/*! \fn QByteArray &QByteArray::replace(int pos, int len, const char *after)

    \overload

    Replaces \a len bytes from index position \a pos with the zero terminated
    string \a after.

    Notice: this can change the length of the byte array.
*/
QByteArray &QByteArray::replace(int pos, int len, const char *after)
{
    return replace(pos,len,after,qstrlen(after));
}

/*! \fn QByteArray &QByteArray::replace(int pos, int len, const char *after, int alen)

    \overload

    Replaces \a len bytes from index position \a pos with \a alen bytes
    from the string \a after. \a after is allowed to have '\\0' characters.

    \since 4.7
*/
QByteArray &QByteArray::replace(int pos, int len, const char *after, int alen)
{
    if (len == alen && (pos + len <= d->size)) {
        detach();
        memcpy(d->data() + pos, after, len*sizeof(char));
        return *this;
    } else {
        remove(pos, len);
        return qbytearray_insert(this, pos, after, alen);
    }
}

// ### optimize all other replace method, by offering
// QByteArray::replace(const char *before, int blen, const char *after, int alen)

/*!
    \overload

    Replaces every occurrence of the byte array \a before with the
    byte array \a after.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 20
*/

QByteArray &QByteArray::replace(const QByteArray &before, const QByteArray &after)
{
    if (isNull() || before.d == after.d)
        return *this;

    QByteArray aft = after;
    if (after.d == d)
        aft.detach();

    return replace(before.constData(), before.size(), aft.constData(), aft.size());
}

/*!
    \fn QByteArray &QByteArray::replace(const char *before, const QByteArray &after)
    \overload

    Replaces every occurrence of the string \a before with the
    byte array \a after.
*/

QByteArray &QByteArray::replace(const char *c, const QByteArray &after)
{
    QByteArray aft = after;
    if (after.d == d)
        aft.detach();

    return replace(c, qstrlen(c), aft.constData(), aft.size());
}

/*!
    \fn QByteArray &QByteArray::replace(const char *before, int bsize, const char *after, int asize)
    \overload

    Replaces every occurrence of the string \a before with the string \a after.
    Since the sizes of the strings are given by \a bsize and \a asize, they
    may contain zero characters and do not need to be zero-terminated.
*/

QByteArray &QByteArray::replace(const char *before, int bsize, const char *after, int asize)
{
    if (isNull() || (before == after && bsize == asize))
        return *this;

    // protect against before or after being part of this
    const char *a = after;
    const char *b = before;
    if (after >= d->data() && after < d->data() + d->size) {
        char *copy = (char *)malloc(asize);
        Q_CHECK_PTR(copy);
        memcpy(copy, after, asize);
        a = copy;
    }
    if (before >= d->data() && before < d->data() + d->size) {
        char *copy = (char *)malloc(bsize);
        Q_CHECK_PTR(copy);
        memcpy(copy, before, bsize);
        b = copy;
    }

    QByteArrayMatcher matcher(before, bsize);
    int index = 0;
    int len = d->size;
    char *d = data();

    if (bsize == asize) {
        if (bsize) {
            while ((index = matcher.indexIn(*this, index)) != -1) {
                memcpy(d + index, after, asize);
                index += bsize;
            }
        }
    } else if (asize < bsize) {
        uint to = 0;
        uint movestart = 0;
        uint num = 0;
        while ((index = matcher.indexIn(*this, index)) != -1) {
            if (num) {
                int msize = index - movestart;
                if (msize > 0) {
                    memmove(d + to, d + movestart, msize);
                    to += msize;
                }
            } else {
                to = index;
            }
            if (asize) {
                memcpy(d + to, after, asize);
                to += asize;
            }
            index += bsize;
            movestart = index;
            num++;
        }
        if (num) {
            int msize = len - movestart;
            if (msize > 0)
                memmove(d + to, d + movestart, msize);
            resize(len - num*(bsize-asize));
        }
    } else {
        // the most complex case. We don't want to lose performance by doing repeated
        // copies and reallocs of the string.
        while (index != -1) {
            uint indices[4096];
            uint pos = 0;
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
            int adjust = pos*(asize-bsize);
            // index has to be adjusted in case we get back into the loop above.
            if (index != -1)
                index += adjust;
            int newlen = len + adjust;
            int moveend = len;
            if (newlen > len) {
                resize(newlen);
                len = newlen;
            }
            d = this->d->data();

            while(pos) {
                pos--;
                int movestart = indices[pos] + bsize;
                int insertstart = indices[pos] + pos*(asize-bsize);
                int moveto = insertstart + asize;
                memmove(d + moveto, d + movestart, (moveend - movestart));
                if (asize)
                    memcpy(d + insertstart, after, asize);
                moveend = movestart - bsize;
            }
        }
    }

    if (a != after)
        ::free(const_cast<char *>(a));
    if (b != before)
        ::free(const_cast<char *>(b));


    return *this;
}


/*!
    \fn QByteArray &QByteArray::replace(const QByteArray &before, const char *after)
    \overload

    Replaces every occurrence of the byte array \a before with the
    string \a after.
*/

/*! \fn QByteArray &QByteArray::replace(const QString &before, const QByteArray &after)

    \overload

    Replaces every occurrence of the string \a before with the byte
    array \a after. The Unicode data is converted into 8-bit
    characters using QString::toUtf8().

    You can disable this function by defining \c QT_NO_CAST_TO_ASCII when you
    compile your applications. You then need to call QString::toUtf8() (or
    QString::toLatin1() or QString::toLocal8Bit()) explicitly if you want to
    convert the data to \c{const char *}.
*/

/*! \fn QByteArray &QByteArray::replace(const QString &before, const char *after)
    \overload

    Replaces every occurrence of the string \a before with the string
    \a after.
*/

/*! \fn QByteArray &QByteArray::replace(const char *before, const char *after)

    \overload

    Replaces every occurrence of the string \a before with the string
    \a after.
*/

/*!
    \overload

    Replaces every occurrence of the character \a before with the
    byte array \a after.
*/

QByteArray &QByteArray::replace(char before, const QByteArray &after)
{
    char b[2] = { before, '\0' };
    QByteArray cb = fromRawData(b, 1);
    return replace(cb, after);
}

/*! \fn QByteArray &QByteArray::replace(char before, const QString &after)

    \overload

    Replaces every occurrence of the character \a before with the
    string \a after. The Unicode data is converted into 8-bit
    characters using QString::toUtf8().

    You can disable this function by defining \c QT_NO_CAST_TO_ASCII when you
    compile your applications. You then need to call QString::toUtf8() (or
    QString::toLatin1() or QString::toLocal8Bit()) explicitly if you want to
    convert the data to \c{const char *}.
*/

/*! \fn QByteArray &QByteArray::replace(char before, const char *after)

    \overload

    Replaces every occurrence of the character \a before with the
    string \a after.
*/

/*!
    \overload

    Replaces every occurrence of the character \a before with the
    character \a after.
*/

QByteArray &QByteArray::replace(char before, char after)
{
    if (d->size) {
        char *i = data();
        char *e = i + d->size;
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
    int start = 0;
    int end;
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

    \code
        QByteArray ba("ab");
        ba.repeated(4);             // returns "abababab"
    \endcode
*/
QByteArray QByteArray::repeated(int times) const
{
    if (d->size == 0)
        return *this;

    if (times <= 1) {
        if (times == 1)
            return *this;
        return QByteArray();
    }

    const int resultSize = times * d->size;

    QByteArray result;
    result.reserve(resultSize);
    if (result.d->alloc != uint(resultSize) + 1u)
        return QByteArray(); // not enough memory

    memcpy(result.d->data(), d->data(), d->size);

    int sizeSoFar = d->size;
    char *end = result.d->data() + sizeSoFar;

    const int halfResultSize = resultSize >> 1;
    while (sizeSoFar <= halfResultSize) {
        memcpy(end, result.d->data(), sizeSoFar);
        end += sizeSoFar;
        sizeSoFar <<= 1;
    }
    memcpy(end, result.d->data(), resultSize - sizeSoFar);
    result.d->data()[resultSize] = '\0';
    result.d->size = resultSize;
    return result;
}

#define REHASH(a) \
    if (ol_minus_1 < sizeof(uint) * CHAR_BIT) \
        hashHaystack -= (a) << ol_minus_1; \
    hashHaystack <<= 1

/*!
    Returns the index position of the first occurrence of the byte
    array \a ba in this byte array, searching forward from index
    position \a from. Returns -1 if \a ba could not be found.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 21

    \sa lastIndexOf(), contains(), count()
*/

int QByteArray::indexOf(const QByteArray &ba, int from) const
{
    const int ol = ba.d->size;
    if (ol == 0)
        return from;
    if (ol == 1)
        return indexOf(*ba.d->data(), from);

    const int l = d->size;
    if (from > d->size || ol + from > l)
        return -1;

    return qFindByteArray(d->data(), d->size, from, ba.d->data(), ol);
}

/*! \fn int QByteArray::indexOf(const QString &str, int from) const

    \overload

    Returns the index position of the first occurrence of the string
    \a str in the byte array, searching forward from index position
    \a from. Returns -1 if \a str could not be found.

    The Unicode data is converted into 8-bit characters using
    QString::toUtf8().

    You can disable this function by defining \c QT_NO_CAST_TO_ASCII when you
    compile your applications. You then need to call QString::toUtf8() (or
    QString::toLatin1() or QString::toLocal8Bit()) explicitly if you want to
    convert the data to \c{const char *}.
*/

/*! \fn int QByteArray::indexOf(const char *str, int from) const

    \overload

    Returns the index position of the first occurrence of the string
    \a str in the byte array, searching forward from index position \a
    from. Returns -1 if \a str could not be found.
*/
int QByteArray::indexOf(const char *c, int from) const
{
    const int ol = qstrlen(c);
    if (ol == 1)
        return indexOf(*c, from);

    const int l = d->size;
    if (from > d->size || ol + from > l)
        return -1;
    if (ol == 0)
        return from;

    return qFindByteArray(d->data(), d->size, from, c, ol);
}

/*!
    \overload

    Returns the index position of the first occurrence of the
    character \a ch in the byte array, searching forward from index
    position \a from. Returns -1 if \a ch could not be found.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 22

    \sa lastIndexOf(), contains()
*/

int QByteArray::indexOf(char ch, int from) const
{
    if (from < 0)
        from = qMax(from + d->size, 0);
    if (from < d->size) {
        const char *n = d->data() + from - 1;
        const char *e = d->data() + d->size;
        while (++n != e)
        if (*n == ch)
            return  n - d->data();
    }
    return -1;
}


static int lastIndexOfHelper(const char *haystack, int l, const char *needle, int ol, int from)
{
    int delta = l - ol;
    if (from < 0)
        from = delta;
    if (from < 0 || from > l)
        return -1;
    if (from > delta)
        from = delta;

    const char *end = haystack;
    haystack += from;
    const uint ol_minus_1 = ol - 1;
    const char *n = needle + ol_minus_1;
    const char *h = haystack + ol_minus_1;
    uint hashNeedle = 0, hashHaystack = 0;
    int idx;
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

/*!
    \fn int QByteArray::lastIndexOf(const QByteArray &ba, int from) const

    Returns the index position of the last occurrence of the byte
    array \a ba in this byte array, searching backward from index
    position \a from. If \a from is -1 (the default), the search
    starts at the last byte. Returns -1 if \a ba could not be found.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 23

    \sa indexOf(), contains(), count()
*/

int QByteArray::lastIndexOf(const QByteArray &ba, int from) const
{
    const int ol = ba.d->size;
    if (ol == 1)
        return lastIndexOf(*ba.d->data(), from);

    return lastIndexOfHelper(d->data(), d->size, ba.d->data(), ol, from);
}

/*! \fn int QByteArray::lastIndexOf(const QString &str, int from) const

    \overload

    Returns the index position of the last occurrence of the string \a
    str in the byte array, searching backward from index position \a
    from. If \a from is -1 (the default), the search starts at the
    last (size() - 1) byte. Returns -1 if \a str could not be found.

    The Unicode data is converted into 8-bit characters using
    QString::toUtf8().

    You can disable this function by defining \c QT_NO_CAST_TO_ASCII when you
    compile your applications. You then need to call QString::toUtf8() (or
    QString::toLatin1() or QString::toLocal8Bit()) explicitly if you want to
    convert the data to \c{const char *}.
*/

/*! \fn int QByteArray::lastIndexOf(const char *str, int from) const
    \overload

    Returns the index position of the last occurrence of the string \a
    str in the byte array, searching backward from index position \a
    from. If \a from is -1 (the default), the search starts at the
    last (size() - 1) byte. Returns -1 if \a str could not be found.
*/
int QByteArray::lastIndexOf(const char *str, int from) const
{
    const int ol = qstrlen(str);
    if (ol == 1)
        return lastIndexOf(*str, from);

    return lastIndexOfHelper(d->data(), d->size, str, ol, from);
}

/*!
    \overload

    Returns the index position of the last occurrence of character \a
    ch in the byte array, searching backward from index position \a
    from. If \a from is -1 (the default), the search starts at the
    last (size() - 1) byte. Returns -1 if \a ch could not be found.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 24

    \sa indexOf(), contains()
*/

int QByteArray::lastIndexOf(char ch, int from) const
{
    if (from < 0)
        from += d->size;
    else if (from > d->size)
        from = d->size-1;
    if (from >= 0) {
        const char *b = d->data();
        const char *n = d->data() + from + 1;
        while (n-- != b)
            if (*n == ch)
                return  n - b;
    }
    return -1;
}

/*!
    Returns the number of (potentially overlapping) occurrences of
    byte array \a ba in this byte array.

    \sa contains(), indexOf()
*/

int QByteArray::count(const QByteArray &ba) const
{
    int num = 0;
    int i = -1;
    if (d->size > 500 && ba.d->size > 5) {
        QByteArrayMatcher matcher(ba);
        while ((i = matcher.indexIn(*this, i + 1)) != -1)
            ++num;
    } else {
        while ((i = indexOf(ba, i + 1)) != -1)
            ++num;
    }
    return num;
}

/*!
    \overload

    Returns the number of (potentially overlapping) occurrences of
    string \a str in the byte array.
*/

int QByteArray::count(const char *str) const
{
    return count(fromRawData(str, qstrlen(str)));
}

/*!
    \overload

    Returns the number of occurrences of character \a ch in the byte
    array.

    \sa contains(), indexOf()
*/

int QByteArray::count(char ch) const
{
    int num = 0;
    const char *i = d->data() + d->size;
    const char *b = d->data();
    while (i != b)
        if (*--i == ch)
            ++num;
    return num;
}

/*! \fn int QByteArray::count() const

    \overload

    Same as size().
*/

/*!
    Returns \c true if this byte array starts with byte array \a ba;
    otherwise returns \c false.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 25

    \sa endsWith(), left()
*/
bool QByteArray::startsWith(const QByteArray &ba) const
{
    if (d == ba.d || ba.d->size == 0)
        return true;
    if (d->size < ba.d->size)
        return false;
    return memcmp(d->data(), ba.d->data(), ba.d->size) == 0;
}

/*! \overload

    Returns \c true if this byte array starts with string \a str;
    otherwise returns \c false.
*/
bool QByteArray::startsWith(const char *str) const
{
    if (!str || !*str)
        return true;
    const int len = int(strlen(str));
    if (d->size < len)
        return false;
    return qstrncmp(d->data(), str, len) == 0;
}

/*! \overload

    Returns \c true if this byte array starts with character \a ch;
    otherwise returns \c false.
*/
bool QByteArray::startsWith(char ch) const
{
    if (d->size == 0)
        return false;
    return d->data()[0] == ch;
}

/*!
    Returns \c true if this byte array ends with byte array \a ba;
    otherwise returns \c false.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 26

    \sa startsWith(), right()
*/
bool QByteArray::endsWith(const QByteArray &ba) const
{
    if (d == ba.d || ba.d->size == 0)
        return true;
    if (d->size < ba.d->size)
        return false;
    return memcmp(d->data() + d->size - ba.d->size, ba.d->data(), ba.d->size) == 0;
}

/*! \overload

    Returns \c true if this byte array ends with string \a str; otherwise
    returns \c false.
*/
bool QByteArray::endsWith(const char *str) const
{
    if (!str || !*str)
        return true;
    const int len = int(strlen(str));
    if (d->size < len)
        return false;
    return qstrncmp(d->data() + d->size - len, str, len) == 0;
}

/*! \overload

    Returns \c true if this byte array ends with character \a ch;
    otherwise returns \c false.
*/
bool QByteArray::endsWith(char ch) const
{
    if (d->size == 0)
        return false;
    return d->data()[d->size - 1] == ch;
}

/*!
    Returns a byte array that contains the leftmost \a len bytes of
    this byte array.

    The entire byte array is returned if \a len is greater than
    size().

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 27

    \sa right(), mid(), startsWith(), truncate()
*/

QByteArray QByteArray::left(int len)  const
{
    if (len >= d->size)
        return *this;
    if (len < 0)
        len = 0;
    return QByteArray(d->data(), len);
}

/*!
    Returns a byte array that contains the rightmost \a len bytes of
    this byte array.

    The entire byte array is returned if \a len is greater than
    size().

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 28

    \sa endsWith(), left(), mid()
*/

QByteArray QByteArray::right(int len) const
{
    if (len >= d->size)
        return *this;
    if (len < 0)
        len = 0;
    return QByteArray(d->data() + d->size - len, len);
}

/*!
    Returns a byte array containing \a len bytes from this byte array,
    starting at position \a pos.

    If \a len is -1 (the default), or \a pos + \a len >= size(),
    returns a byte array containing all bytes starting at position \a
    pos until the end of the byte array.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 29

    \sa left(), right()
*/

QByteArray QByteArray::mid(int pos, int len) const
{
    using namespace QtPrivate;
    switch (QContainerImplHelper::mid(size(), &pos, &len)) {
    case QContainerImplHelper::Null:
        return QByteArray();
    case QContainerImplHelper::Empty:
    {
        QByteArrayDataPtr empty = { Data::allocate(0) };
        return QByteArray(empty);
    }
    case QContainerImplHelper::Full:
        return *this;
    case QContainerImplHelper::Subset:
        return QByteArray(d->data() + pos, len);
    }
    Q_UNREACHABLE();
    return QByteArray();
}

/*!
    \fn QByteArray QByteArray::toLower() const

    Returns a lowercase copy of the byte array. The bytearray is
    interpreted as a Latin-1 encoded string.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 30

    \sa toUpper(), {8-bit Character Comparisons}
*/

// prevent the compiler from inlining the function in each of
// toLower and toUpper when the only difference is the table being used
// (even with constant propagation, there's no gain in performance).
template <typename T>
Q_NEVER_INLINE
static QByteArray toCase_template(T &input, const uchar * table)
{
    // find the first bad character in input
    const char *orig_begin = input.constBegin();
    const char *firstBad = orig_begin;
    const char *e = input.constEnd();
    for ( ; firstBad != e ; ++firstBad) {
        uchar ch = uchar(*firstBad);
        uchar converted = table[ch];
        if (ch != converted)
            break;
    }

    if (firstBad == e)
        return qMove(input);

    // transform the rest
    QByteArray s = qMove(input);    // will copy if T is const QByteArray
    char *b = s.begin();            // will detach if necessary
    char *p = b + (firstBad - orig_begin);
    e = b + s.size();
    for ( ; p != e; ++p) {
        *p = char(uchar(table[uchar(*p)]));
    }
    return s;
}

QByteArray QByteArray::toLower_helper(const QByteArray &a)
{
    return toCase_template(a, latin1_lowercased);
}

QByteArray QByteArray::toLower_helper(QByteArray &a)
{
    return toCase_template(a, latin1_lowercased);
}

/*!
    \fn QByteArray QByteArray::toUpper() const

    Returns an uppercase copy of the byte array. The bytearray is
    interpreted as a Latin-1 encoded string.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 31

    \sa toLower(), {8-bit Character Comparisons}
*/

QByteArray QByteArray::toUpper_helper(const QByteArray &a)
{
    return toCase_template(a, latin1_uppercased);
}

QByteArray QByteArray::toUpper_helper(QByteArray &a)
{
    return toCase_template(a, latin1_uppercased);
}

/*! \fn void QByteArray::clear()

    Clears the contents of the byte array and makes it null.

    \sa resize(), isNull()
*/

void QByteArray::clear()
{
    if (!d->ref.deref())
        Data::deallocate(d);
    d = Data::sharedNull();
}

#if !defined(QT_NO_DATASTREAM) || (defined(QT_BOOTSTRAPPED) && !defined(QT_BUILD_QMAKE))

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
    if (len == 0xffffffff)
        return in;

    const quint32 Step = 1024 * 1024;
    quint32 allocated = 0;

    do {
        int blockSize = qMin(Step, len - allocated);
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

    Returns \c true if this byte array is equal to string \a str;
    otherwise returns \c false.

    The Unicode data is converted into 8-bit characters using
    QString::toUtf8().

    The comparison is case sensitive.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. You
    then need to call QString::fromUtf8(), QString::fromLatin1(),
    or QString::fromLocal8Bit() explicitly if you want to convert the byte
    array to a QString before doing the comparison.
*/

/*! \fn bool QByteArray::operator!=(const QString &str) const

    Returns \c true if this byte array is not equal to string \a str;
    otherwise returns \c false.

    The Unicode data is converted into 8-bit characters using
    QString::toUtf8().

    The comparison is case sensitive.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. You
    then need to call QString::fromUtf8(), QString::fromLatin1(),
    or QString::fromLocal8Bit() explicitly if you want to convert the byte
    array to a QString before doing the comparison.
*/

/*! \fn bool QByteArray::operator<(const QString &str) const

    Returns \c true if this byte array is lexically less than string \a
    str; otherwise returns \c false.

    The Unicode data is converted into 8-bit characters using
    QString::toUtf8().

    The comparison is case sensitive.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. You
    then need to call QString::fromUtf8(), QString::fromLatin1(),
    or QString::fromLocal8Bit() explicitly if you want to convert the byte
    array to a QString before doing the comparison.
*/

/*! \fn bool QByteArray::operator>(const QString &str) const

    Returns \c true if this byte array is lexically greater than string
    \a str; otherwise returns \c false.

    The Unicode data is converted into 8-bit characters using
    QString::toUtf8().

    The comparison is case sensitive.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. You
    then need to call QString::fromUtf8(), QString::fromLatin1(),
    or QString::fromLocal8Bit() explicitly if you want to convert the byte
    array to a QString before doing the comparison.
*/

/*! \fn bool QByteArray::operator<=(const QString &str) const

    Returns \c true if this byte array is lexically less than or equal
    to string \a str; otherwise returns \c false.

    The Unicode data is converted into 8-bit characters using
    QString::toUtf8().

    The comparison is case sensitive.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. You
    then need to call QString::fromUtf8(), QString::fromLatin1(),
    or QString::fromLocal8Bit() explicitly if you want to convert the byte
    array to a QString before doing the comparison.
*/

/*! \fn bool QByteArray::operator>=(const QString &str) const

    Returns \c true if this byte array is greater than or equal to string
    \a str; otherwise returns \c false.

    The Unicode data is converted into 8-bit characters using
    QString::toUtf8().

    The comparison is case sensitive.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. You
    then need to call QString::fromUtf8(), QString::fromLatin1(),
    or QString::fromLocal8Bit() explicitly if you want to convert the byte
    array to a QString before doing the comparison.
*/

/*! \fn bool operator==(const QByteArray &a1, const QByteArray &a2)
    \relates QByteArray

    \overload

    Returns \c true if byte array \a a1 is equal to byte array \a a2;
    otherwise returns \c false.
*/

/*! \fn bool operator==(const QByteArray &a1, const char *a2)
    \relates QByteArray

    \overload

    Returns \c true if byte array \a a1 is equal to string \a a2;
    otherwise returns \c false.
*/

/*! \fn bool operator==(const char *a1, const QByteArray &a2)
    \relates QByteArray

    \overload

    Returns \c true if string \a a1 is equal to byte array \a a2;
    otherwise returns \c false.
*/

/*! \fn bool operator!=(const QByteArray &a1, const QByteArray &a2)
    \relates QByteArray

    \overload

    Returns \c true if byte array \a a1 is not equal to byte array \a a2;
    otherwise returns \c false.
*/

/*! \fn bool operator!=(const QByteArray &a1, const char *a2)
    \relates QByteArray

    \overload

    Returns \c true if byte array \a a1 is not equal to string \a a2;
    otherwise returns \c false.
*/

/*! \fn bool operator!=(const char *a1, const QByteArray &a2)
    \relates QByteArray

    \overload

    Returns \c true if string \a a1 is not equal to byte array \a a2;
    otherwise returns \c false.
*/

/*! \fn bool operator<(const QByteArray &a1, const QByteArray &a2)
    \relates QByteArray

    \overload

    Returns \c true if byte array \a a1 is lexically less than byte array
    \a a2; otherwise returns \c false.
*/

/*! \fn inline bool operator<(const QByteArray &a1, const char *a2)
    \relates QByteArray

    \overload

    Returns \c true if byte array \a a1 is lexically less than string
    \a a2; otherwise returns \c false.
*/

/*! \fn bool operator<(const char *a1, const QByteArray &a2)
    \relates QByteArray

    \overload

    Returns \c true if string \a a1 is lexically less than byte array
    \a a2; otherwise returns \c false.
*/

/*! \fn bool operator<=(const QByteArray &a1, const QByteArray &a2)
    \relates QByteArray

    \overload

    Returns \c true if byte array \a a1 is lexically less than or equal
    to byte array \a a2; otherwise returns \c false.
*/

/*! \fn bool operator<=(const QByteArray &a1, const char *a2)
    \relates QByteArray

    \overload

    Returns \c true if byte array \a a1 is lexically less than or equal
    to string \a a2; otherwise returns \c false.
*/

/*! \fn bool operator<=(const char *a1, const QByteArray &a2)
    \relates QByteArray

    \overload

    Returns \c true if string \a a1 is lexically less than or equal
    to byte array \a a2; otherwise returns \c false.
*/

/*! \fn bool operator>(const QByteArray &a1, const QByteArray &a2)
    \relates QByteArray

    \overload

    Returns \c true if byte array \a a1 is lexically greater than byte
    array \a a2; otherwise returns \c false.
*/

/*! \fn bool operator>(const QByteArray &a1, const char *a2)
    \relates QByteArray

    \overload

    Returns \c true if byte array \a a1 is lexically greater than string
    \a a2; otherwise returns \c false.
*/

/*! \fn bool operator>(const char *a1, const QByteArray &a2)
    \relates QByteArray

    \overload

    Returns \c true if string \a a1 is lexically greater than byte array
    \a a2; otherwise returns \c false.
*/

/*! \fn bool operator>=(const QByteArray &a1, const QByteArray &a2)
    \relates QByteArray

    \overload

    Returns \c true if byte array \a a1 is lexically greater than or
    equal to byte array \a a2; otherwise returns \c false.
*/

/*! \fn bool operator>=(const QByteArray &a1, const char *a2)
    \relates QByteArray

    \overload

    Returns \c true if byte array \a a1 is lexically greater than or
    equal to string \a a2; otherwise returns \c false.
*/

/*! \fn bool operator>=(const char *a1, const QByteArray &a2)
    \relates QByteArray

    \overload

    Returns \c true if string \a a1 is lexically greater than or
    equal to byte array \a a2; otherwise returns \c false.
*/

/*! \fn const QByteArray operator+(const QByteArray &a1, const QByteArray &a2)
    \relates QByteArray

    Returns a byte array that is the result of concatenating byte
    array \a a1 and byte array \a a2.

    \sa QByteArray::operator+=()
*/

/*! \fn const QByteArray operator+(const QByteArray &a1, const char *a2)
    \relates QByteArray

    \overload

    Returns a byte array that is the result of concatenating byte
    array \a a1 and string \a a2.
*/

/*! \fn const QByteArray operator+(const QByteArray &a1, char a2)
    \relates QByteArray

    \overload

    Returns a byte array that is the result of concatenating byte
    array \a a1 and character \a a2.
*/

/*! \fn const QByteArray operator+(const char *a1, const QByteArray &a2)
    \relates QByteArray

    \overload

    Returns a byte array that is the result of concatenating string
    \a a1 and byte array \a a2.
*/

/*! \fn const QByteArray operator+(char a1, const QByteArray &a2)
    \relates QByteArray

    \overload

    Returns a byte array that is the result of concatenating character
    \a a1 and byte array \a a2.
*/

/*!
    \fn QByteArray QByteArray::simplified() const

    Returns a byte array that has whitespace removed from the start
    and the end, and which has each sequence of internal whitespace
    replaced with a single space.

    Whitespace means any character for which the standard C++
    \c isspace() function returns \c true in the C locale. This includes the ASCII
    isspace() function returns \c true in the C locale. This includes the ASCII
    characters '\\t', '\\n', '\\v', '\\f', '\\r', and ' '.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 32

    \sa trimmed()
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

    Returns a byte array that has whitespace removed from the start
    and the end.

    Whitespace means any character for which the standard C++
    \c isspace() function returns \c true in the C locale. This includes the ASCII
    characters '\\t', '\\n', '\\v', '\\f', '\\r', and ' '.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 33

    Unlike simplified(), \l {QByteArray::trimmed()}{trimmed()} leaves internal whitespace alone.

    \sa simplified()
*/
QByteArray QByteArray::trimmed_helper(const QByteArray &a)
{
    return QStringAlgorithms<const QByteArray>::trimmed_helper(a);
}

QByteArray QByteArray::trimmed_helper(QByteArray &a)
{
    return QStringAlgorithms<QByteArray>::trimmed_helper(a);
}


/*!
    Returns a byte array of size \a width that contains this byte
    array padded by the \a fill character.

    If \a truncate is false and the size() of the byte array is more
    than \a width, then the returned byte array is a copy of this byte
    array.

    If \a truncate is true and the size() of the byte array is more
    than \a width, then any bytes in a copy of the byte array
    after position \a width are removed, and the copy is returned.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 34

    \sa rightJustified()
*/

QByteArray QByteArray::leftJustified(int width, char fill, bool truncate) const
{
    QByteArray result;
    int len = d->size;
    int padlen = width - len;
    if (padlen > 0) {
        result.resize(len+padlen);
        if (len)
            memcpy(result.d->data(), d->data(), len);
        memset(result.d->data()+len, fill, padlen);
    } else {
        if (truncate)
            result = left(width);
        else
            result = *this;
    }
    return result;
}

/*!
    Returns a byte array of size \a width that contains the \a fill
    character followed by this byte array.

    If \a truncate is false and the size of the byte array is more
    than \a width, then the returned byte array is a copy of this byte
    array.

    If \a truncate is true and the size of the byte array is more
    than \a width, then the resulting byte array is truncated at
    position \a width.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 35

    \sa leftJustified()
*/

QByteArray QByteArray::rightJustified(int width, char fill, bool truncate) const
{
    QByteArray result;
    int len = d->size;
    int padlen = width - len;
    if (padlen > 0) {
        result.resize(len+padlen);
        if (len)
            memcpy(result.d->data()+padlen, data(), len);
        memset(result.d->data(), fill, padlen);
    } else {
        if (truncate)
            result = left(width);
        else
            result = *this;
    }
    return result;
}

bool QByteArray::isNull() const { return d == QArrayData::sharedNull(); }

static qlonglong toIntegral_helper(const char *data, bool *ok, int base, qlonglong)
{
    return QLocaleData::bytearrayToLongLong(data, base, ok);
}

static qulonglong toIntegral_helper(const char *data, bool *ok, int base, qulonglong)
{
    return QLocaleData::bytearrayToUnsLongLong(data, base, ok);
}

template <typename T> static inline
T toIntegral_helper(const char *data, bool *ok, int base)
{
    // ### Qt6: use std::conditional<std::is_unsigned<T>::value, qulonglong, qlonglong>::type
    const bool isUnsigned = T(0) < T(-1);
    typedef typename QtPrivate::QConditional<isUnsigned, qulonglong, qlonglong>::Type Int64;

#if defined(QT_CHECK_RANGE)
    if (base != 0 && (base < 2 || base > 36)) {
        qWarning("QByteArray::toIntegral: Invalid base %d", base);
        base = 10;
    }
#endif

    // we select the right overload by the last, unused parameter
    Int64 val = toIntegral_helper(data, ok, base, Int64());
    if (T(val) != val) {
        if (ok)
            *ok = false;
        val = 0;
    }
    return T(val);
}

/*!
    Returns the byte array converted to a \c {long long} using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.

    If \a base is 0, the base is determined automatically using the
    following rules: If the byte array begins with "0x", it is assumed to
    be hexadecimal; if it begins with "0", it is assumed to be octal;
    otherwise it is assumed to be decimal.

    Returns 0 if the conversion fails.

    If \a ok is not 0: if a conversion error occurs, *\a{ok} is set to
    false; otherwise *\a{ok} is set to true.

    \note The conversion of the number is performed in the default C locale,
    irrespective of the user's locale.

    \sa number()
*/

qlonglong QByteArray::toLongLong(bool *ok, int base) const
{
    return toIntegral_helper<qlonglong>(nulTerminated().constData(), ok, base);
}

/*!
    Returns the byte array converted to an \c {unsigned long long}
    using base \a base, which is 10 by default and must be between 2
    and 36, or 0.

    If \a base is 0, the base is determined automatically using the
    following rules: If the byte array begins with "0x", it is assumed to
    be hexadecimal; if it begins with "0", it is assumed to be octal;
    otherwise it is assumed to be decimal.

    Returns 0 if the conversion fails.

    If \a ok is not 0: if a conversion error occurs, *\a{ok} is set to
    false; otherwise *\a{ok} is set to true.

    \note The conversion of the number is performed in the default C locale,
    irrespective of the user's locale.

    \sa number()
*/

qulonglong QByteArray::toULongLong(bool *ok, int base) const
{
    return toIntegral_helper<qulonglong>(nulTerminated().constData(), ok, base);
}

/*!
    Returns the byte array converted to an \c int using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.

    If \a base is 0, the base is determined automatically using the
    following rules: If the byte array begins with "0x", it is assumed to
    be hexadecimal; if it begins with "0", it is assumed to be octal;
    otherwise it is assumed to be decimal.

    Returns 0 if the conversion fails.

    If \a ok is not 0: if a conversion error occurs, *\a{ok} is set to
    false; otherwise *\a{ok} is set to true.

    \snippet code/src_corelib_tools_qbytearray.cpp 36

    \note The conversion of the number is performed in the default C locale,
    irrespective of the user's locale.

    \sa number()
*/

int QByteArray::toInt(bool *ok, int base) const
{
    return toIntegral_helper<int>(nulTerminated().constData(), ok, base);
}

/*!
    Returns the byte array converted to an \c {unsigned int} using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.

    If \a base is 0, the base is determined automatically using the
    following rules: If the byte array begins with "0x", it is assumed to
    be hexadecimal; if it begins with "0", it is assumed to be octal;
    otherwise it is assumed to be decimal.

    Returns 0 if the conversion fails.

    If \a ok is not 0: if a conversion error occurs, *\a{ok} is set to
    false; otherwise *\a{ok} is set to true.

    \note The conversion of the number is performed in the default C locale,
    irrespective of the user's locale.

    \sa number()
*/

uint QByteArray::toUInt(bool *ok, int base) const
{
    return toIntegral_helper<uint>(nulTerminated().constData(), ok, base);
}

/*!
    \since 4.1

    Returns the byte array converted to a \c long int using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.

    If \a base is 0, the base is determined automatically using the
    following rules: If the byte array begins with "0x", it is assumed to
    be hexadecimal; if it begins with "0", it is assumed to be octal;
    otherwise it is assumed to be decimal.

    Returns 0 if the conversion fails.

    If \a ok is not 0: if a conversion error occurs, *\a{ok} is set to
    false; otherwise *\a{ok} is set to true.

    \snippet code/src_corelib_tools_qbytearray.cpp 37

    \note The conversion of the number is performed in the default C locale,
    irrespective of the user's locale.

    \sa number()
*/
long QByteArray::toLong(bool *ok, int base) const
{
    return toIntegral_helper<long>(nulTerminated().constData(), ok, base);
}

/*!
    \since 4.1

    Returns the byte array converted to an \c {unsigned long int} using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.

    If \a base is 0, the base is determined automatically using the
    following rules: If the byte array begins with "0x", it is assumed to
    be hexadecimal; if it begins with "0", it is assumed to be octal;
    otherwise it is assumed to be decimal.

    Returns 0 if the conversion fails.

    If \a ok is not 0: if a conversion error occurs, *\a{ok} is set to
    false; otherwise *\a{ok} is set to true.

    \note The conversion of the number is performed in the default C locale,
    irrespective of the user's locale.

    \sa number()
*/
ulong QByteArray::toULong(bool *ok, int base) const
{
    return toIntegral_helper<ulong>(nulTerminated().constData(), ok, base);
}

/*!
    Returns the byte array converted to a \c short using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.

    If \a base is 0, the base is determined automatically using the
    following rules: If the byte array begins with "0x", it is assumed to
    be hexadecimal; if it begins with "0", it is assumed to be octal;
    otherwise it is assumed to be decimal.

    Returns 0 if the conversion fails.

    If \a ok is not 0: if a conversion error occurs, *\a{ok} is set to
    false; otherwise *\a{ok} is set to true.

    \note The conversion of the number is performed in the default C locale,
    irrespective of the user's locale.

    \sa number()
*/

short QByteArray::toShort(bool *ok, int base) const
{
    return toIntegral_helper<short>(nulTerminated().constData(), ok, base);
}

/*!
    Returns the byte array converted to an \c {unsigned short} using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.

    If \a base is 0, the base is determined automatically using the
    following rules: If the byte array begins with "0x", it is assumed to
    be hexadecimal; if it begins with "0", it is assumed to be octal;
    otherwise it is assumed to be decimal.

    Returns 0 if the conversion fails.

    If \a ok is not 0: if a conversion error occurs, *\a{ok} is set to
    false; otherwise *\a{ok} is set to true.

    \note The conversion of the number is performed in the default C locale,
    irrespective of the user's locale.

    \sa number()
*/

ushort QByteArray::toUShort(bool *ok, int base) const
{
    return toIntegral_helper<ushort>(nulTerminated().constData(), ok, base);
}


/*!
    Returns the byte array converted to a \c double value.

    Returns 0.0 if the conversion fails.

    If \a ok is not 0: if a conversion error occurs, *\a{ok} is set to
    false; otherwise *\a{ok} is set to true.

    \snippet code/src_corelib_tools_qbytearray.cpp 38

    \note The conversion of the number is performed in the default C locale,
    irrespective of the user's locale.

    \sa number()
*/

double QByteArray::toDouble(bool *ok) const
{
    QByteArray nulled = nulTerminated();
    bool nonNullOk = false;
    int processed = 0;
    double d = asciiToDouble(nulled.constData(), nulled.length(), nonNullOk, processed);
    if (ok)
        *ok = nonNullOk;
    return d;
}

/*!
    Returns the byte array converted to a \c float value.

    Returns 0.0 if the conversion fails.

    If \a ok is not 0: if a conversion error occurs, *\a{ok} is set to
    false; otherwise *\a{ok} is set to true.

    \note The conversion of the number is performed in the default C locale,
    irrespective of the user's locale.

    \sa number()
*/

float QByteArray::toFloat(bool *ok) const
{
    return QLocaleData::convertDoubleToFloat(toDouble(ok), ok);
}

/*!
    Returns a copy of the byte array, encoded as Base64.

    \snippet code/src_corelib_tools_qbytearray.cpp 39

    The algorithm used to encode Base64-encoded data is defined in \l{RFC 4648}.

    \sa fromBase64()
*/
QByteArray QByteArray::toBase64() const
{
    return toBase64(Base64Encoding);
}

/*!
    \since 5.2
    \overload

    Returns a copy of the byte array, encoded using the options \a options.

    \snippet code/src_corelib_tools_qbytearray.cpp 39bis

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
    int padlen = 0;

    QByteArray tmp((d->size + 2) / 3 * 4, Qt::Uninitialized);

    int i = 0;
    char *out = tmp.data();
    while (i < d->size) {
        // encode 3 bytes at a time
        int chunk = 0;
        chunk |= int(uchar(d->data()[i++])) << 16;
        if (i == d->size) {
            padlen = 2;
        } else {
            chunk |= int(uchar(d->data()[i++])) << 8;
            if (i == d->size)
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

    Sets the byte array to the printed value of \a n in base \a base (10
    by default) and returns a reference to the byte array. The \a base can
    be any value between 2 and 36. For bases other than 10, n is treated
    as an unsigned integer.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 40

    \note The format of the number is not localized; the default C locale
    is used irrespective of the user's locale.

    \sa number(), toInt()
*/

/*!
    \fn QByteArray &QByteArray::setNum(uint n, int base)
    \overload

    \sa toUInt()
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

    if (n < 0 && base == 10) {
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

    Sets the byte array to the printed value of \a n, formatted in format
    \a f with precision \a prec, and returns a reference to the
    byte array.

    The format \a f can be any of the following:

    \table
    \header \li Format \li Meaning
    \row \li \c e \li format as [-]9.9e[+|-]999
    \row \li \c E \li format as [-]9.9E[+|-]999
    \row \li \c f \li format as [-]9.9
    \row \li \c g \li use \c e or \c f format, whichever is the most concise
    \row \li \c G \li use \c E or \c f format, whichever is the most concise
    \endtable

    With 'e', 'E', and 'f', \a prec is the number of digits after the
    decimal point. With 'g' and 'G', \a prec is the maximum number of
    significant digits (trailing zeroes are omitted).

    \note The format of the number is not localized; the default C locale
    is used irrespective of the user's locale.

    \sa toDouble()
*/

QByteArray &QByteArray::setNum(double n, char f, int prec)
{
    QLocaleData::DoubleForm form = QLocaleData::DFDecimal;
    uint flags = QLocaleData::ZeroPadExponent;

    if (qIsUpper(f))
        flags |= QLocaleData::CapitalEorX;
    f = qToLower(f);

    switch (f) {
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
            qWarning("QByteArray::setNum: Invalid format char '%c'", f);
#endif
            break;
    }

    *this = QLocaleData::c()->doubleToString(n, prec, form, -1, flags).toLatin1();
    return *this;
}

/*!
    \fn QByteArray &QByteArray::setNum(float n, char f, int prec)
    \overload

    Sets the byte array to the printed value of \a n, formatted in format
    \a f with precision \a prec, and returns a reference to the
    byte array.

    \note The format of the number is not localized; the default C locale
    is used irrespective of the user's locale.

    \sa toFloat()
*/

/*!
    Returns a byte array containing the string equivalent of the
    number \a n to base \a base (10 by default). The \a base can be
    any value between 2 and 36.

    Example:
    \snippet code/src_corelib_tools_qbytearray.cpp 41

    \note The format of the number is not localized; the default C locale
    is used irrespective of the user's locale.

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

    Returns a byte array that contains the printed value of \a n,
    formatted in format \a f with precision \a prec.

    Argument \a n is formatted according to the \a f format specified,
    which is \c g by default, and can be any of the following:

    \table
    \header \li Format \li Meaning
    \row \li \c e \li format as [-]9.9e[+|-]999
    \row \li \c E \li format as [-]9.9E[+|-]999
    \row \li \c f \li format as [-]9.9
    \row \li \c g \li use \c e or \c f format, whichever is the most concise
    \row \li \c G \li use \c E or \c f format, whichever is the most concise
    \endtable

    With 'e', 'E', and 'f', \a prec is the number of digits after the
    decimal point. With 'g' and 'G', \a prec is the maximum number of
    significant digits (trailing zeroes are omitted).

    \snippet code/src_corelib_tools_qbytearray.cpp 42

    \note The format of the number is not localized; the default C locale
    is used irrespective of the user's locale.

    \sa toDouble()
*/
QByteArray QByteArray::number(double n, char f, int prec)
{
    QByteArray s;
    s.setNum(n, f, prec);
    return s;
}

/*!
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

    \snippet code/src_corelib_tools_qbytearray.cpp 43

    \warning A byte array created with fromRawData() is \e not
    null-terminated, unless the raw data contains a 0 character at
    position \a size. While that does not matter for QDataStream or
    functions like indexOf(), passing the byte array to a function
    accepting a \c{const char *} expected to be '\\0'-terminated will
    fail.

    \sa setRawData(), data(), constData()
*/

QByteArray QByteArray::fromRawData(const char *data, int size)
{
    Data *x;
    if (!data) {
        x = Data::sharedNull();
    } else if (!size) {
        x = Data::allocate(0);
    } else {
        x = Data::fromRawData(data, size);
        Q_CHECK_PTR(x);
    }
    QByteArrayDataPtr dataPtr = { x };
    return QByteArray(dataPtr);
}

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
QByteArray &QByteArray::setRawData(const char *data, uint size)
{
    if (d->ref.isShared() || d->alloc) {
        *this = fromRawData(data, size);
    } else {
        if (data) {
            d->size = size;
            d->offset = data - reinterpret_cast<char *>(d);
        } else {
            d->offset = sizeof(QByteArrayData);
            d->size = 0;
            *d->data() = 0;
        }
    }
    return *this;
}

/*!
    Returns a decoded copy of the Base64 array \a base64. Input is not checked
    for validity; invalid characters in the input are skipped, enabling the
    decoding process to continue with subsequent characters.

    For example:

    \snippet code/src_corelib_tools_qbytearray.cpp 44

    The algorithm used to decode Base64-encoded data is defined in \l{RFC 4648}.

    \sa toBase64()
*/
QByteArray QByteArray::fromBase64(const QByteArray &base64)
{
    return fromBase64(base64, Base64Encoding);
}

/*!
    \since 5.2
    \overload

    Returns a decoded copy of the Base64 array \a base64, using the alphabet
    defined by \a options. Input is not checked for validity; invalid
    characters in the input are skipped, enabling the decoding process to
    continue with subsequent characters.

    For example:

    \snippet code/src_corelib_tools_qbytearray.cpp 44bis

    The algorithm used to decode Base64-encoded data is defined in \l{RFC 4648}.

    \sa toBase64()
*/
QByteArray QByteArray::fromBase64(const QByteArray &base64, Base64Options options)
{
    unsigned int buf = 0;
    int nbits = 0;
    QByteArray tmp((base64.size() * 3) / 4, Qt::Uninitialized);

    int offset = 0;
    for (int i = 0; i < base64.size(); ++i) {
        int ch = base64.at(i);
        int d;

        if (ch >= 'A' && ch <= 'Z')
            d = ch - 'A';
        else if (ch >= 'a' && ch <= 'z')
            d = ch - 'a' + 26;
        else if (ch >= '0' && ch <= '9')
            d = ch - '0' + 52;
        else if (ch == '+' && (options & Base64UrlEncoding) == 0)
            d = 62;
        else if (ch == '-' && (options & Base64UrlEncoding) != 0)
            d = 62;
        else if (ch == '/' && (options & Base64UrlEncoding) == 0)
            d = 63;
        else if (ch == '_' && (options & Base64UrlEncoding) != 0)
            d = 63;
        else
            d = -1;

        if (d != -1) {
            buf = (buf << 6) | d;
            nbits += 6;
            if (nbits >= 8) {
                nbits -= 8;
                tmp[offset++] = buf >> nbits;
                buf &= (1 << nbits) - 1;
            }
        }
    }

    tmp.truncate(offset);
    return tmp;
}

/*!
    Returns a decoded copy of the hex encoded array \a hexEncoded. Input is not checked
    for validity; invalid characters in the input are skipped, enabling the
    decoding process to continue with subsequent characters.

    For example:

    \snippet code/src_corelib_tools_qbytearray.cpp 45

    \sa toHex()
*/
QByteArray QByteArray::fromHex(const QByteArray &hexEncoded)
{
    QByteArray res((hexEncoded.size() + 1)/ 2, Qt::Uninitialized);
    uchar *result = (uchar *)res.data() + res.size();

    bool odd_digit = true;
    for (int i = hexEncoded.size() - 1; i >= 0; --i) {
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
    Returns a hex encoded copy of the byte array. The hex encoding uses the numbers 0-9 and
    the letters a-f.

    \sa fromHex()
*/
QByteArray QByteArray::toHex() const
{
    QByteArray hex(d->size * 2, Qt::Uninitialized);
    char *hexData = hex.data();
    const uchar *data = (const uchar *)d->data();
    for (int i = 0; i < d->size; ++i) {
        hexData[i*2] = QtMiscUtils::toHexLower(data[i] >> 4);
        hexData[i*2+1] = QtMiscUtils::toHexLower(data[i] & 0xf);
    }
    return hex;
}

static void q_fromPercentEncoding(QByteArray *ba, char percent)
{
    if (ba->isEmpty())
        return;

    char *data = ba->data();
    const char *inputPtr = data;

    int i = 0;
    int len = ba->count();
    int outlen = 0;
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

void q_fromPercentEncoding(QByteArray *ba)
{
    q_fromPercentEncoding(ba, '%');
}

/*!
    \since 4.4

    Returns a decoded copy of the URI/URL-style percent-encoded \a input.
    The \a percent parameter allows you to replace the '%' character for
    another (for instance, '_' or '=').

    For example:
    \code
        QByteArray text = QByteArray::fromPercentEncoding("Qt%20is%20great%33");
        text.data();            // returns "Qt is great!"
    \endcode

    \sa toPercentEncoding(), QUrl::fromPercentEncoding()
*/
QByteArray QByteArray::fromPercentEncoding(const QByteArray &input, char percent)
{
    if (input.isNull())
        return QByteArray();       // preserve null
    if (input.isEmpty())
        return QByteArray(input.data(), 0);

    QByteArray tmp = input;
    q_fromPercentEncoding(&tmp, percent);
    return tmp;
}

/*! \fn QByteArray QByteArray::fromStdString(const std::string &str)
    \since 5.4

    Returns a copy of the \a str string as a QByteArray.

    \sa toStdString(), QString::fromStdString()
*/

/*!
    \fn std::string QByteArray::toStdString() const
    \since 5.4

    Returns a std::string object with the data contained in this
    QByteArray.

    This operator is mostly useful to pass a QByteArray to a function
    that accepts a std::string object.

    \sa fromStdString(), QString::toStdString()
*/

static inline bool q_strchr(const char str[], char chr)
{
    if (!str) return false;

    const char *ptr = str;
    char c;
    while ((c = *ptr++))
        if (c == chr)
            return true;
    return false;
}

static void q_toPercentEncoding(QByteArray *ba, const char *dontEncode, const char *alsoEncode, char percent)
{
    if (ba->isEmpty())
        return;

    QByteArray input = *ba;
    int len = input.count();
    const char *inputData = input.constData();
    char *output = 0;
    int length = 0;

    for (int i = 0; i < len; ++i) {
        unsigned char c = *inputData++;
        if (((c >= 0x61 && c <= 0x7A) // ALPHA
             || (c >= 0x41 && c <= 0x5A) // ALPHA
             || (c >= 0x30 && c <= 0x39) // DIGIT
             || c == 0x2D // -
             || c == 0x2E // .
             || c == 0x5F // _
             || c == 0x7E // ~
             || q_strchr(dontEncode, c))
            && !q_strchr(alsoEncode, c)) {
            if (output)
                output[length] = c;
            ++length;
        } else {
            if (!output) {
                // detach now
                ba->resize(len*3); // worst case
                output = ba->data();
            }
            output[length++] = percent;
            output[length++] = QtMiscUtils::toHexUpper((c & 0xf0) >> 4);
            output[length++] = QtMiscUtils::toHexUpper(c & 0xf);
        }
    }
    if (output)
        ba->truncate(length);
}

void q_toPercentEncoding(QByteArray *ba, const char *exclude, const char *include)
{
    q_toPercentEncoding(ba, exclude, include, '%');
}

void q_normalizePercentEncoding(QByteArray *ba, const char *exclude)
{
    q_fromPercentEncoding(ba, '%');
    q_toPercentEncoding(ba, exclude, 0, '%');
}

/*!
    \since 4.4

    Returns a URI/URL-style percent-encoded copy of this byte array. The
    \a percent parameter allows you to override the default '%'
    character for another.

    By default, this function will encode all characters that are not
    one of the following:

        ALPHA ("a" to "z" and "A" to "Z") / DIGIT (0 to 9) / "-" / "." / "_" / "~"

    To prevent characters from being encoded pass them to \a
    exclude. To force characters to be encoded pass them to \a
    include. The \a percent character is always encoded.

    Example:

    \code
         QByteArray text = "{a fishy string?}";
         QByteArray ba = text.toPercentEncoding("{}", "s");
         qDebug(ba.constData());
         // prints "{a fi%73hy %73tring%3F}"
    \endcode

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

    QByteArray include2 = include;
    if (percent != '%')                        // the default
        if ((percent >= 0x61 && percent <= 0x7A) // ALPHA
            || (percent >= 0x41 && percent <= 0x5A) // ALPHA
            || (percent >= 0x30 && percent <= 0x39) // DIGIT
            || percent == 0x2D // -
            || percent == 0x2E // .
            || percent == 0x5F // _
            || percent == 0x7E) // ~
        include2 += percent;

    QByteArray result = *this;
    q_toPercentEncoding(&result, exclude.nulTerminated().constData(), include2.nulTerminated().constData(), percent);

    return result;
}

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

QT_END_NAMESPACE
