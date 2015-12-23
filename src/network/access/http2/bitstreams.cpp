/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#include "bitstreams_p.h"
#include "huffman_p.h"

#include <QtCore/qbytearray.h>

#include <limits>

QT_BEGIN_NAMESPACE

static_assert(std::numeric_limits<uchar>::digits == 8, "octets expected");

namespace HPack
{

BitOStream::BitOStream(std::vector<uchar> &b)
    : buffer(b),
      // All data 'packed' before:
      bitsSet(8 * quint64(b.size()))
{
}

void BitOStream::writeBits(uchar bits, quint8 bitLength)
{
    Q_ASSERT(bitLength <= 8);

    quint8 count = bitsSet % 8; // bits used in buffer.back(), but 0 means 8
    bits <<= 8 - bitLength; // at top of byte, lower bits clear
    if (count) { // we have a part-used byte; fill it some more:
        buffer.back() |= bits >> count;
        count = 8 - count;
    } // count bits have been consumed (and 0 now means 0)
    if (bitLength > count)
        buffer.push_back(bits << count);

    bitsSet += bitLength;
}

void BitOStream::write(quint32 src)
{
    const quint8 prefixLen = 8 - bitsSet % 8;
    const quint32 fullPrefix = (1 << prefixLen) - 1;

    // https://http2.github.io/http2-spec/compression.html#low-level.representation,
    // 5.1
    if (src < fullPrefix) {
        writeBits(uchar(src), prefixLen);
    } else {
        writeBits(uchar(fullPrefix), prefixLen);
        // We're on the byte boundary now,
        // so we can just 'push_back'.
        Q_ASSERT(!(bitsSet % 8));
        src -= fullPrefix;
        while (src >= 128) {
            buffer.push_back(uchar(src % 128 + 128));
            src /= 128;
            bitsSet += 8;
        }
        buffer.push_back(src);
        bitsSet += 8;
    }
}

void BitOStream::write(const QByteArray &src, bool compressed)
{
    quint32 byteLen = src.size();
    if (compressed && byteLen) {
        const auto bitLen = huffman_encoded_bit_length(src);
        Q_ASSERT(bitLen && std::numeric_limits<quint32>::max() >= (bitLen + 7) / 8);
        byteLen = (bitLen + 7) / 8;
        writeBits(uchar(1), 1); // bit set - compressed
    } else {
        writeBits(uchar(0), 1); // no compression.
    }

    write(byteLen);

    if (compressed) {
        huffman_encode_string(src, *this);
    } else {
        bitsSet += quint64(src.size()) * 8;
        buffer.insert(buffer.end(), src.begin(), src.end());
    }
}

quint64 BitOStream::bitLength() const
{
    return bitsSet;
}

quint64 BitOStream::byteLength() const
{
    return buffer.size();
}

const uchar *BitOStream::begin() const
{
    return &buffer[0];
}

const uchar *BitOStream::end() const
{
    return &buffer[0] + buffer.size();
}

void BitOStream::clear()
{
    buffer.clear();
    bitsSet = 0;
}

BitIStream::BitIStream()
    : first(),
      last(),
      offset(),
      streamError(Error::NoError)
{
}

BitIStream::BitIStream(const uchar *begin, const uchar *end)
                 : first(begin),
                   last(end),
                   offset(),
                   streamError(Error::NoError)
{
}

quint64 BitIStream::bitLength() const
{
    return quint64(last - first) * 8;
}

bool BitIStream::hasMoreBits() const
{
    return offset < bitLength();
}

bool BitIStream::skipBits(quint64 nBits)
{
    if (nBits > bitLength() || bitLength() - nBits < offset)
        return false;

    offset += nBits;
    return true;
}

bool BitIStream::rewindOffset(quint64 nBits)
{
    if (nBits > offset)
        return false;

    offset -= nBits;
    return true;
}

bool BitIStream::read(quint32 *dstPtr)
{
    Q_ASSERT(dstPtr);
    quint32 &dst = *dstPtr;

    // 5.1 Integer Representation
    //
    // Integers are used to represent name indexes, header field indexes, or string lengths.
    // An integer representation can start anywhere within an octet.
    // To allow for optimized processing, an integer representation always finishes at the end of an octet.
    // An integer is represented in two parts: a prefix that fills the current octet and an optional
    // list of octets that are used if the integer value does not fit within the prefix.
    // The number of bits of the prefix (called N) is a parameter of the integer representation.
    // If the integer value is small enough, i.e., strictly less than 2N-1, it is compressed within the N-bit prefix.
    // ...
    // The prefix size, N, is always between 1 and 8 bits. An integer
    // starting at an octet boundary will have an 8-bit prefix.

    // Technically, such integers can be of any size, but as we do not have arbitrary-long integers,
    // everything that does not fit into 'dst' we consider as an error (after all, try to allocate a string
    // of such size and ... hehehe - send it as a part of a header!

    // This function updates the offset _only_ if the read was successful.
    if (offset >= bitLength()) {
        setError(Error::NotEnoughData);
        return false;
    }

    setError(Error::NoError);

    const quint32 prefixLen = 8 - offset % 8;
    const quint32 fullPrefix = (1 << prefixLen) - 1;

    const uchar prefix = uchar(first[offset / 8] & fullPrefix);
    if (prefix < fullPrefix) {
        // The number fitted into the prefix bits.
        dst = prefix;
        offset += prefixLen;
        return true;
    }

    quint32 newOffset = offset + prefixLen;
    // We have a list of bytes representing an integer ...
    quint64 val = prefix;
    quint32 octetPower = 0;

    while (true) {
        if (newOffset >= bitLength()) {
            setError(Error::NotEnoughData);
            return false;
        }

        const uchar octet = first[newOffset / 8];

        if (octetPower == 28 && octet > 15) {
            qCritical("integer is too big");
            setError(Error::InvalidInteger);
            return false;
        }

        val += quint32(octet & 0x7f) << octetPower;
        newOffset += 8;

        if (!(octet & 0x80)) {
            // The most significant bit of each octet is used
            // as a continuation flag: its value is set to 1
            // except for the last octet in the list.
            break;
        }

        octetPower += 7;
    }

    dst = val;
    offset = newOffset;
    Q_ASSERT(!(offset % 8));

    return true;
}

bool BitIStream::read(QByteArray *dstPtr)
{
    Q_ASSERT(dstPtr);
    QByteArray &dst = *dstPtr;
    //5.2 String Literal Representation
    //
    // Header field names and header field values can be represented as string literals.
    // A string literal is compressed as a sequence of octets, either by directly encoding
    // the string literal's octets or by using a Huffman code.

    // We update the offset _only_ if the read was successful.

    const quint64 oldOffset = offset;
    uchar compressed = 0;
    if (peekBits(offset, 1, &compressed) != 1 || !skipBits(1)) {
        setError(Error::NotEnoughData);
        return false;
    }

    setError(Error::NoError);

    quint32 len = 0;
    if (read(&len)) {
        Q_ASSERT(!(offset % 8));
        if (len <= (bitLength() - offset) / 8) { // We have enough data to read a string ...
            if (!compressed) {
                // Now good news, integer always ends on a byte boundary.
                // We can read 'len' bytes without any bit magic.
                const char *src = reinterpret_cast<const char *>(first + offset / 8);
                dst = QByteArray(src, len);
                offset += quint64(len) * 8;
                return true;
            }

            BitIStream slice(first + offset / 8, first + offset / 8 + len);
            if (huffman_decode_string(slice, &dst)) {
                offset += quint64(len) * 8;
                return true;
            }

            setError(Error::CompressionError);
        } else {
            setError(Error::NotEnoughData);
        }
    } // else the exact reason was set by read(quint32).

    offset = oldOffset;
    return false;
}

BitIStream::Error BitIStream::error() const
{
    return streamError;
}

void BitIStream::setError(Error newState)
{
    streamError = newState;
}

} // namespace HPack

QT_END_NAMESPACE
