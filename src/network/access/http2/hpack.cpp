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
#include "hpack_p.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qdebug.h>

#include <limits>

QT_BEGIN_NAMESPACE

namespace HPack
{

HeaderSize header_size(const HttpHeader &header)
{
    HeaderSize size(true, 0);
    for (const HeaderField &field : header) {
        HeaderSize delta = entry_size(field);
        if (!delta.first)
            return HeaderSize();
        if (std::numeric_limits<quint32>::max() - size.second < delta.second)
            return HeaderSize();
        size.second += delta.second;
    }

    return size;
}

struct BitPattern
{
    uchar value;
    uchar bitLength;
};

bool operator == (const BitPattern &lhs, const BitPattern &rhs)
{
    return lhs.bitLength == rhs.bitLength && lhs.value == rhs.value;
}

namespace
{

using StreamError = BitIStream::Error;

// There are several bit patterns to distinguish header fields:
// 1 - indexed
// 01 - literal with incremented indexing
// 0000 - literal without indexing
// 0001 - literal, never indexing
// 001 - dynamic table size update.

// It's always 1 or 0 actually, but the number of bits to extract
// from the input stream - differs.
const BitPattern Indexed = {1, 1};
const BitPattern LiteralIncrementalIndexing = {1, 2};
const BitPattern LiteralNoIndexing = {0, 4};
const BitPattern LiteralNeverIndexing = {1, 4};
const BitPattern SizeUpdate = {1, 3};

bool is_literal_field(const BitPattern &pattern)
{
    return pattern == LiteralIncrementalIndexing
           || pattern == LiteralNoIndexing
           || pattern == LiteralNeverIndexing;
}

void write_bit_pattern(const BitPattern &pattern, BitOStream &outputStream)
{
    outputStream.writeBits(pattern.value, pattern.bitLength);
}

bool read_bit_pattern(const BitPattern &pattern, BitIStream &inputStream)
{
    uchar chunk = 0;

    const quint32 bitsRead = inputStream.peekBits(inputStream.streamOffset(),
                                                  pattern.bitLength, &chunk);
    if (bitsRead != pattern.bitLength)
        return false;

    // Since peekBits packs in the most significant bits, shift it!
    chunk >>= (8 - bitsRead);
    if (chunk != pattern.value)
        return false;

    inputStream.skipBits(pattern.bitLength);

    return true;
}

bool is_request_pseudo_header(const QByteArray &name)
{
    return name == ":method" || name == ":scheme" ||
           name == ":authority" || name == ":path";
}

} // unnamed namespace

Encoder::Encoder(quint32 size, bool compress)
    : lookupTable(size, true /*encoder needs search index*/),
      compressStrings(compress)
{
}

quint32 Encoder::dynamicTableSize() const
{
    return lookupTable.dynamicDataSize();
}

bool Encoder::encodeRequest(BitOStream &outputStream, const HttpHeader &header)
{
    if (!header.size()) {
        qDebug("empty header");
        return false;
    }

    if (!encodeRequestPseudoHeaders(outputStream, header))
        return false;

    for (const auto &field : header) {
        if (is_request_pseudo_header(field.name))
            continue;

        if (!encodeHeaderField(outputStream, field))
            return false;
    }

    return true;
}

bool Encoder::encodeResponse(BitOStream &outputStream, const HttpHeader &header)
{
    if (!header.size()) {
        qDebug("empty header");
        return false;
    }

    if (!encodeResponsePseudoHeaders(outputStream, header))
        return false;

    for (const auto &field : header) {
        if (field.name == ":status")
            continue;

        if (!encodeHeaderField(outputStream, field))
            return false;
    }

    return true;
}

bool Encoder::encodeSizeUpdate(BitOStream &outputStream, quint32 newSize)
{
    if (!lookupTable.updateDynamicTableSize(newSize)) {
        qDebug("failed to update own table size");
        return false;
    }

    write_bit_pattern(SizeUpdate, outputStream);
    outputStream.write(newSize);

    return true;
}

void Encoder::setMaxDynamicTableSize(quint32 size)
{
    // Up to a caller (HTTP2 protocol handler)
    // to validate this size first.
    lookupTable.setMaxDynamicTableSize(size);
}

bool Encoder::encodeRequestPseudoHeaders(BitOStream &outputStream,
                                         const HttpHeader &header)
{
    // The following pseudo-header fields are defined for HTTP/2 requests:
    // - The :method pseudo-header field includes the HTTP method
    // - The :scheme pseudo-header field includes the scheme portion of the target URI
    // - The :authority pseudo-header field includes the authority portion of the target URI
    // - The :path pseudo-header field includes the path and query parts of the target URI

    // All HTTP/2 requests MUST include exactly one valid value for the :method,
    // :scheme, and :path pseudo-header fields, unless it is a CONNECT request
    // (Section 8.3). An HTTP request that omits mandatory pseudo-header fields
    // is malformed (Section 8.1.2.6).

    using size_type = decltype(header.size());

    bool methodFound = false;
    const char *headerName[] = {":authority", ":scheme", ":path"};
    const size_type nHeaders = sizeof headerName / sizeof headerName[0];
    bool headerFound[nHeaders] = {};

    for (const auto &field : header) {
        if (field.name == ":status") {
            qCritical("invalid pseudo-header (:status) in a request");
            return false;
        }

        if (field.name == ":method") {
            if (methodFound) {
                qCritical("only one :method pseudo-header is allowed");
                return false;
            }

            if (!encodeMethod(outputStream, field))
                return false;
            methodFound = true;
        } else if (field.name == "cookie") {
            // "crumbs" ...
        } else {
            for (size_type j = 0; j < nHeaders; ++j) {
                if (field.name == headerName[j]) {
                    if (headerFound[j]) {
                        qCritical() << "only one" << headerName[j] << "pseudo-header is allowed";
                        return false;
                    }
                    if (!encodeHeaderField(outputStream, field))
                        return false;
                    headerFound[j] = true;
                    break;
                }
            }
        }
    }

    if (!methodFound) {
        qCritical("mandatory :method pseudo-header not found");
        return false;
    }

    // 1: don't demand headerFound[0], as :authority isn't mandatory.
    for (size_type i = 1; i < nHeaders; ++i) {
        if (!headerFound[i]) {
            qCritical() << "mandatory" << headerName[i]
                        << "pseudo-header not found";
            return false;
        }
    }

    return true;
}

bool Encoder::encodeHeaderField(BitOStream &outputStream, const HeaderField &field)
{
    // TODO: at the moment we never use LiteralNo/Never Indexing ...

    // Here we try:
    // 1. indexed
    // 2. literal indexed with indexed name/literal value
    // 3. literal indexed with literal name/literal value
    if (const auto index = lookupTable.indexOf(field.name, field.value))
        return encodeIndexedField(outputStream, index);

    if (const auto index = lookupTable.indexOf(field.name)) {
        return encodeLiteralField(outputStream, LiteralIncrementalIndexing,
                                  index, field.value, compressStrings);
    }

    return encodeLiteralField(outputStream, LiteralIncrementalIndexing,
                              field.name, field.value, compressStrings);
}

bool Encoder::encodeMethod(BitOStream &outputStream, const HeaderField &field)
{
    Q_ASSERT(field.name == ":method");
    quint32 index = lookupTable.indexOf(field.name, field.value);
    if (index)
        return encodeIndexedField(outputStream, index);

    index = lookupTable.indexOf(field.name);
    Q_ASSERT(index); // ":method" is always in the static table ...
    return encodeLiteralField(outputStream, LiteralIncrementalIndexing,
                              index, field.value, compressStrings);
}

bool Encoder::encodeResponsePseudoHeaders(BitOStream &outputStream, const HttpHeader &header)
{
    bool statusFound = false;
    for (const auto &field : header) {
        if (is_request_pseudo_header(field.name)) {
            qCritical() << "invalid pseudo-header" << field.name << "in http response";
            return false;
        }

        if (field.name == ":status") {
            if (statusFound) {
                qDebug("only one :status pseudo-header is allowed");
                return false;
            }
            if (!encodeHeaderField(outputStream, field))
                return false;
            statusFound = true;
        } else if (field.name == "cookie") {
            // "crumbs"..
        }
    }

    if (!statusFound)
        qCritical("mandatory :status pseudo-header not found");

    return statusFound;
}

bool Encoder::encodeIndexedField(BitOStream &outputStream, quint32 index) const
{
    Q_ASSERT(lookupTable.indexIsValid(index));

    write_bit_pattern(Indexed, outputStream);
    outputStream.write(index);

    return true;
}

bool Encoder::encodeLiteralField(BitOStream &outputStream, const BitPattern &fieldType,
                                 const QByteArray &name, const QByteArray &value,
                                 bool withCompression)
{
    Q_ASSERT(is_literal_field(fieldType));
    // According to HPACK, the bit pattern is
    // 01 | 000000 (integer 0 that fits into 6-bit prefix),
    // since integers always end on byte boundary,
    // this also implies that we always start at bit offset == 0.
    if (outputStream.bitLength() % 8) {
        qCritical("invalid bit offset");
        return false;
    }

    if (fieldType == LiteralIncrementalIndexing) {
        if (!lookupTable.prependField(name, value))
            qDebug("failed to prepend a new field");
    }

    write_bit_pattern(fieldType, outputStream);

    outputStream.write(0);
    outputStream.write(name, withCompression);
    outputStream.write(value, withCompression);

    return true;
}

bool Encoder::encodeLiteralField(BitOStream &outputStream, const BitPattern &fieldType,
                                 quint32 nameIndex, const QByteArray &value,
                                 bool withCompression)
{
    Q_ASSERT(is_literal_field(fieldType));

    QByteArray name;
    const bool found = lookupTable.fieldName(nameIndex, &name);
    Q_UNUSED(found) Q_ASSERT(found);

    if (fieldType == LiteralIncrementalIndexing) {
        if (!lookupTable.prependField(name, value))
            qDebug("failed to prepend a new field");
    }

    write_bit_pattern(fieldType, outputStream);
    outputStream.write(nameIndex);
    outputStream.write(value, withCompression);

    return true;
}

Decoder::Decoder(quint32 size)
    : lookupTable{size, false /* we do not need search index ... */}
{
}

bool Decoder::decodeHeaderFields(BitIStream &inputStream)
{
    header.clear();
    while (true) {
        if (read_bit_pattern(Indexed, inputStream)) {
            if (!decodeIndexedField(inputStream))
                return false;
        } else if (read_bit_pattern(LiteralIncrementalIndexing, inputStream)) {
            if (!decodeLiteralField(LiteralIncrementalIndexing, inputStream))
                return false;
        } else if (read_bit_pattern(LiteralNoIndexing, inputStream)) {
            if (!decodeLiteralField(LiteralNoIndexing, inputStream))
                return false;
        } else if (read_bit_pattern(LiteralNeverIndexing, inputStream)) {
            if (!decodeLiteralField(LiteralNeverIndexing, inputStream))
                return false;
        } else if (read_bit_pattern(SizeUpdate, inputStream)) {
            if (!decodeSizeUpdate(inputStream))
                return false;
        } else {
            return inputStream.bitLength() == inputStream.streamOffset();
        }
    }

    return false;
}

quint32 Decoder::dynamicTableSize() const
{
    return lookupTable.dynamicDataSize();
}

void Decoder::setMaxDynamicTableSize(quint32 size)
{
    // Up to a caller (HTTP2 protocol handler)
    // to validate this size first.
    lookupTable.setMaxDynamicTableSize(size);
}

bool Decoder::decodeIndexedField(BitIStream &inputStream)
{
    quint32 index = 0;
    if (inputStream.read(&index)) {
        if (!index) {
            // "The index value of 0 is not used.
            //  It MUST be treated as a decoding
            //  error if found in an indexed header
            //  field representation."
            return false;
        }

        QByteArray name, value;
        if (lookupTable.field(index, &name, &value))
            return processDecodedField(Indexed, name, value);
    } else {
        handleStreamError(inputStream);
    }

    return false;
}

bool Decoder::decodeSizeUpdate(BitIStream &inputStream)
{
    // For now, just read and skip bits.
    quint32 maxSize = 0;
    if (inputStream.read(&maxSize)) {
        if (!lookupTable.updateDynamicTableSize(maxSize))
            return false;

        return true;
    }

    handleStreamError(inputStream);
    return false;
}

bool Decoder::decodeLiteralField(const BitPattern &fieldType, BitIStream &inputStream)
{
    // https://http2.github.io/http2-spec/compression.html
    // 6.2.1, 6.2.2, 6.2.3
    // Format for all 'literal' is similar,
    // the difference - is how we update/not our lookup table.
    quint32 index = 0;
    if (inputStream.read(&index)) {
        QByteArray name;
        if (!index) {
            // Read a string.
            if (!inputStream.read(&name)) {
                handleStreamError(inputStream);
                return false;
            }
        } else {
            if (!lookupTable.fieldName(index, &name))
                return false;
        }

        QByteArray value;
        if (inputStream.read(&value))
            return processDecodedField(fieldType, name, value);
    }

    handleStreamError(inputStream);

    return false;
}

bool Decoder::processDecodedField(const BitPattern &fieldType,
                                 const QByteArray &name,
                                 const QByteArray &value)
{
    if (fieldType == LiteralIncrementalIndexing) {
        if (!lookupTable.prependField(name, value))
            return false;
    }

    header.push_back(HeaderField(name, value));
    return true;
}

void Decoder::handleStreamError(BitIStream &inputStream)
{
    const auto errorCode(inputStream.error());
    if (errorCode == StreamError::NoError)
        return;

    // For now error handling not needed here,
    // HTTP2 layer will end with session error/COMPRESSION_ERROR.
}

}

QT_END_NAMESPACE
