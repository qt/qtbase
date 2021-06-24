/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qdecompresshelper_p.h"

#include <QtCore/private/qbytearray_p.h>
#include <QtCore/qiodevice.h>

#include <limits>
#include <zlib.h>

#if QT_CONFIG(brotli)
#    include <brotli/decode.h>
#endif

#if QT_CONFIG(zstd)
#    include <zstd.h>
#endif

#include <array>

QT_BEGIN_NAMESPACE
namespace {
struct ContentEncodingMapping
{
    char name[8];
    QDecompressHelper::ContentEncoding encoding;
};

constexpr ContentEncodingMapping contentEncodingMapping[] {
#if QT_CONFIG(zstd)
    { "zstd", QDecompressHelper::Zstandard },
#endif
#if QT_CONFIG(brotli)
    { "br", QDecompressHelper::Brotli },
#endif
    { "gzip", QDecompressHelper::GZip },
    { "deflate", QDecompressHelper::Deflate },
};

QDecompressHelper::ContentEncoding encodingFromByteArray(const QByteArray &ce) noexcept
{
    for (const auto &mapping : contentEncodingMapping) {
        if (ce.compare(QByteArrayView(mapping.name, strlen(mapping.name)), Qt::CaseInsensitive) == 0)
            return mapping.encoding;
    }
    return QDecompressHelper::None;
}

z_stream *toZlibPointer(void *ptr)
{
    return static_cast<z_stream_s *>(ptr);
}

#if QT_CONFIG(brotli)
BrotliDecoderState *toBrotliPointer(void *ptr)
{
    return static_cast<BrotliDecoderState *>(ptr);
}
#endif

#if QT_CONFIG(zstd)
ZSTD_DStream *toZstandardPointer(void *ptr)
{
    return static_cast<ZSTD_DStream *>(ptr);
}
#endif
}

bool QDecompressHelper::isSupportedEncoding(const QByteArray &encoding)
{
    return encodingFromByteArray(encoding) != QDecompressHelper::None;
}

QByteArrayList QDecompressHelper::acceptedEncoding()
{
    static QByteArrayList accepted = []() {
        QByteArrayList list;
        list.reserve(sizeof(contentEncodingMapping) / sizeof(contentEncodingMapping[0]));
        for (const auto &mapping : contentEncodingMapping) {
            list << QByteArray(mapping.name);
        }
        return list;
    }();
    return accepted;
}

QDecompressHelper::~QDecompressHelper()
{
    clear();
}

bool QDecompressHelper::setEncoding(const QByteArray &encoding)
{
    Q_ASSERT(contentEncoding == QDecompressHelper::None);
    if (contentEncoding != QDecompressHelper::None) {
        qWarning("Encoding is already set.");
        return false;
    }
    ContentEncoding ce = encodingFromByteArray(encoding);
    if (ce == None) {
        qWarning("An unsupported content encoding was selected: %s", encoding.data());
        return false;
    }
    return setEncoding(ce);
}

bool QDecompressHelper::setEncoding(ContentEncoding ce)
{
    Q_ASSERT(contentEncoding == None);
    contentEncoding = ce;
    switch (contentEncoding) {
    case None:
        Q_UNREACHABLE();
        break;
    case Deflate:
    case GZip: {
        z_stream *inflateStream = new z_stream;
        memset(inflateStream, 0, sizeof(z_stream));
        // "windowBits can also be greater than 15 for optional gzip decoding.
        // Add 32 to windowBits to enable zlib and gzip decoding with automatic header detection"
        // http://www.zlib.net/manual.html
        if (inflateInit2(inflateStream, MAX_WBITS + 32) != Z_OK) {
            delete inflateStream;
            inflateStream = nullptr;
        }
        decoderPointer = inflateStream;
        break;
    }
    case Brotli:
#if QT_CONFIG(brotli)
        decoderPointer = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
#else
        Q_UNREACHABLE();
#endif
        break;
    case Zstandard:
#if QT_CONFIG(zstd)
        decoderPointer = ZSTD_createDStream();
#else
        Q_UNREACHABLE();
#endif
        break;
    }
    if (!decoderPointer) {
        qWarning("Failed to initialize the decoder.");
        contentEncoding = QDecompressHelper::None;
        return false;
    }
    return true;
}

/*!
    \internal

    Returns true if the QDecompressHelper is measuring the
    size of the decompressed data.

    \sa setCountingBytesEnabled, uncompressedSize
*/
bool QDecompressHelper::isCountingBytes() const
{
    return countDecompressed;
}

/*!
    \internal

    Enable or disable counting the decompressed size of the data
    based on \a shouldCount. Enabling this means the data will be
    decompressed twice (once for counting and once when data is
    being read).

    \note Can only be called before contentEncoding is set and data
    is fed to the object.

    \sa isCountingBytes, uncompressedSize
*/
void QDecompressHelper::setCountingBytesEnabled(bool shouldCount)
{
    // These are a best-effort check to ensure that no data has already been processed before this
    // gets enabled
    Q_ASSERT(compressedDataBuffer.byteAmount() == 0);
    Q_ASSERT(contentEncoding == None);
    countDecompressed = shouldCount;
}

/*!
    \internal

    Returns the amount of uncompressed bytes left.

    \note Since this is only based on the data received
    so far the final size could be larger.

    \note It is only valid to call this if isCountingBytes()
    returns true

    \sa isCountingBytes, setCountBytes
*/
qint64 QDecompressHelper::uncompressedSize() const
{
    Q_ASSERT(countDecompressed);
    return uncompressedBytes;
}

/*!
    \internal
    \overload
*/
void QDecompressHelper::feed(const QByteArray &data)
{
    return feed(QByteArray(data));
}

/*!
    \internal
    Give \a data to the QDecompressHelper which will be stored until
    a read is attempted.

    If \c isCountingBytes() is true then it will decompress immediately
    before discarding the data, but will count the uncompressed byte
    size.
*/
void QDecompressHelper::feed(QByteArray &&data)
{
    Q_ASSERT(contentEncoding != None);
    totalCompressedBytes += data.size();
    if (!countInternal(data))
        clear(); // If our counting brother failed then so will we :|
    else
        compressedDataBuffer.append(std::move(data));
}

/*!
    \internal
    \overload
*/
void QDecompressHelper::feed(const QByteDataBuffer &buffer)
{
    Q_ASSERT(contentEncoding != None);
    totalCompressedBytes += buffer.byteAmount();
    if (!countInternal(buffer))
        clear(); // If our counting brother failed then so will we :|
    else
        compressedDataBuffer.append(buffer);
}

/*!
    \internal
    \overload
*/
void QDecompressHelper::feed(QByteDataBuffer &&buffer)
{
    Q_ASSERT(contentEncoding != None);
    totalCompressedBytes += buffer.byteAmount();
    if (!countInternal(buffer))
        clear(); // If our counting brother failed then so will we :|
    else
        compressedDataBuffer.append(std::move(buffer));
}

/*!
    \internal
    Decompress the data internally and immediately discard the
    uncompressed data, but count how many bytes were decoded.
    This lets us know the final size, unfortunately at the cost of
    increased computation.

    Potential @future improvement:
    Decompress XX MiB/KiB before starting the count.
        For smaller files the extra decompression can then be avoided.
*/
bool QDecompressHelper::countInternal()
{
    Q_ASSERT(countDecompressed);
    while (countHelper->hasData()) {
        std::array<char, 1024> temp;
        qsizetype bytesRead = countHelper->read(temp.data(), temp.size());
        if (bytesRead == -1)
            return false;
        uncompressedBytes += bytesRead;
    }
    return true;
}

/*!
    \internal
    \overload
*/
bool QDecompressHelper::countInternal(const QByteArray &data)
{
    if (countDecompressed) {
        if (!countHelper) {
            countHelper = std::make_unique<QDecompressHelper>();
            countHelper->setDecompressedSafetyCheckThreshold(archiveBombCheckThreshold);
            countHelper->setEncoding(contentEncoding);
        }
        countHelper->feed(data);
        return countInternal();
    }
    return true;
}

/*!
    \internal
    \overload
*/
bool QDecompressHelper::countInternal(const QByteDataBuffer &buffer)
{
    if (countDecompressed) {
        if (!countHelper) {
            countHelper = std::make_unique<QDecompressHelper>();
            countHelper->setDecompressedSafetyCheckThreshold(archiveBombCheckThreshold);
            countHelper->setEncoding(contentEncoding);
        }
        countHelper->feed(buffer);
        return countInternal();
    }
    return true;
}

qsizetype QDecompressHelper::read(char *data, qsizetype maxSize)
{
    if (!isValid())
        return -1;

    qsizetype bytesRead = -1;
    if (!hasData())
        return 0;

    switch (contentEncoding) {
    case None:
        Q_UNREACHABLE();
        break;
    case Deflate:
    case GZip:
        bytesRead = readZLib(data, maxSize);
        break;
    case Brotli:
        bytesRead = readBrotli(data, maxSize);
        break;
    case Zstandard:
        bytesRead = readZstandard(data, maxSize);
        break;
    }
    if (bytesRead == -1)
        clear();
    else if (countDecompressed)
        uncompressedBytes -= bytesRead;

    totalUncompressedBytes += bytesRead;
    if (isPotentialArchiveBomb())
        return -1;

    return bytesRead;
}

/*!
    \internal
    Set the \a threshold required before the archive bomb detection kicks in.
    By default this is 10MB. Setting it to -1 is treated as disabling the
    feature.
*/
void QDecompressHelper::setDecompressedSafetyCheckThreshold(qint64 threshold)
{
    if (threshold == -1)
        threshold = std::numeric_limits<qint64>::max();
    archiveBombCheckThreshold = threshold;
}

bool QDecompressHelper::isPotentialArchiveBomb() const
{
    if (totalCompressedBytes == 0)
        return false;

    if (totalUncompressedBytes <= archiveBombCheckThreshold)
        return false;

    // Some protection against malicious or corrupted compressed files that expand far more than
    // is reasonable.
    double ratio = double(totalUncompressedBytes) / double(totalCompressedBytes);
    switch (contentEncoding) {
    case None:
        Q_UNREACHABLE();
        break;
    case Deflate:
    case GZip:
        // This value is mentioned in docs for
        // QNetworkRequest::setMinimumArchiveBombSize, keep synchronized
        if (ratio > 40) {
            return true;
        }
        break;
    case Brotli:
    case Zstandard:
        // This value is mentioned in docs for
        // QNetworkRequest::setMinimumArchiveBombSize, keep synchronized
        if (ratio > 100) {
            return true;
        }
        break;
    }
    return false;
}

/*!
    \internal
    Returns true if there are encoded bytes left or there is some
    indication that the decoder still has data left internally.

    \note Even if this returns true the next call to read() might
    read 0 bytes. This most likely means the decompression is done.
*/
bool QDecompressHelper::hasData() const
{
    return encodedBytesAvailable() || decoderHasData;
}

qint64 QDecompressHelper::encodedBytesAvailable() const
{
    return compressedDataBuffer.byteAmount();
}

bool QDecompressHelper::isValid() const
{
    return contentEncoding != None;
}

void QDecompressHelper::clear()
{
    switch (contentEncoding) {
    case None:
        break;
    case Deflate:
    case GZip: {
        z_stream *inflateStream = toZlibPointer(decoderPointer);
        if (inflateStream)
            inflateEnd(inflateStream);
        delete inflateStream;
        break;
    }
    case Brotli: {
#if QT_CONFIG(brotli)
        BrotliDecoderState *brotliDecoderState = toBrotliPointer(decoderPointer);
        if (brotliDecoderState)
            BrotliDecoderDestroyInstance(brotliDecoderState);
#endif
        break;
    }
    case Zstandard: {
#if QT_CONFIG(zstd)
        ZSTD_DStream *zstdStream = toZstandardPointer(decoderPointer);
        if (zstdStream)
            ZSTD_freeDStream(zstdStream);
#endif
        break;
    }
    }
    decoderPointer = nullptr;
    contentEncoding = None;

    compressedDataBuffer.clear();
    decoderHasData = false;

    countDecompressed = false;
    countHelper.reset();
    uncompressedBytes = 0;
    totalUncompressedBytes = 0;
    totalCompressedBytes = 0;
}

qsizetype QDecompressHelper::readZLib(char *data, const qsizetype maxSize)
{
    bool triedRawDeflate = false;

    z_stream *inflateStream = toZlibPointer(decoderPointer);
    static const size_t zlibMaxSize =
            size_t(std::numeric_limits<decltype(inflateStream->avail_in)>::max());

    QByteArrayView input = compressedDataBuffer.readPointer();
    if (size_t(input.size()) > zlibMaxSize)
        input = input.sliced(zlibMaxSize);

    inflateStream->avail_in = input.size();
    inflateStream->next_in = reinterpret_cast<Bytef *>(const_cast<char *>(input.data()));

    bool bigMaxSize = (zlibMaxSize < size_t(maxSize));
    qsizetype adjustedAvailableOut = bigMaxSize ? qsizetype(zlibMaxSize) : maxSize;
    inflateStream->avail_out = adjustedAvailableOut;
    inflateStream->next_out = reinterpret_cast<Bytef *>(data);

    qsizetype bytesDecoded = 0;
    do {
        auto previous_avail_out = inflateStream->avail_out;
        int ret = inflate(inflateStream, Z_NO_FLUSH);
        // All negative return codes are errors, in the context of HTTP compression, Z_NEED_DICT is
        // also an error.
        // in the case where we get Z_DATA_ERROR this could be because we received raw deflate
        // compressed data.
        if (ret == Z_DATA_ERROR && !triedRawDeflate) {
            inflateEnd(inflateStream);
            triedRawDeflate = true;
            inflateStream->zalloc = Z_NULL;
            inflateStream->zfree = Z_NULL;
            inflateStream->opaque = Z_NULL;
            inflateStream->avail_in = 0;
            inflateStream->next_in = Z_NULL;
            int ret = inflateInit2(inflateStream, -MAX_WBITS);
            if (ret != Z_OK) {
                return -1;
            } else {
                inflateStream->avail_in = input.size();
                inflateStream->next_in =
                        reinterpret_cast<Bytef *>(const_cast<char *>(input.data()));
                continue;
            }
        } else if (ret < 0 || ret == Z_NEED_DICT) {
            return -1;
        }
        bytesDecoded += qsizetype(previous_avail_out - inflateStream->avail_out);
        if (ret == Z_STREAM_END) {

            // If there's more data after the stream then this is probably composed of multiple
            // streams.
            if (inflateStream->avail_in != 0) {
                inflateEnd(inflateStream);
                Bytef *next_in = inflateStream->next_in;
                uInt avail_in = inflateStream->avail_in;
                inflateStream->zalloc = Z_NULL;
                inflateStream->zfree = Z_NULL;
                inflateStream->opaque = Z_NULL;
                if (inflateInit2(inflateStream, MAX_WBITS + 32) != Z_OK) {
                    delete inflateStream;
                    decoderPointer = nullptr;
                    // Failed to reinitialize, so we'll just return what we have
                    compressedDataBuffer.advanceReadPointer(input.size() - avail_in);
                    return bytesDecoded;
                } else {
                    inflateStream->next_in = next_in;
                    inflateStream->avail_in = avail_in;
                    // Keep going to handle the other cases below
                }
            } else {
                // No extra data, stream is at the end. We're done.
                compressedDataBuffer.advanceReadPointer(input.size());
                return bytesDecoded;
            }
        }

        if (bigMaxSize && inflateStream->avail_out == 0) {
            // Need to adjust the next_out and avail_out parameters since we reached the end
            // of the current range
            bigMaxSize = (zlibMaxSize < size_t(maxSize - bytesDecoded));
            inflateStream->avail_out = bigMaxSize ? qsizetype(zlibMaxSize) : maxSize - bytesDecoded;
            inflateStream->next_out = reinterpret_cast<Bytef *>(data + bytesDecoded);
        }

        if (inflateStream->avail_in == 0 && inflateStream->avail_out > 0) {
            // Grab the next input!
            compressedDataBuffer.advanceReadPointer(input.size());
            input = compressedDataBuffer.readPointer();
            if (size_t(input.size()) > zlibMaxSize)
                input = input.sliced(zlibMaxSize);
            inflateStream->avail_in = input.size();
            inflateStream->next_in =
                    reinterpret_cast<Bytef *>(const_cast<char *>(input.data()));
        }
    } while (inflateStream->avail_out > 0 && inflateStream->avail_in > 0);

    compressedDataBuffer.advanceReadPointer(input.size() - inflateStream->avail_in);

    return bytesDecoded;
}

qsizetype QDecompressHelper::readBrotli(char *data, const qsizetype maxSize)
{
#if !QT_CONFIG(brotli)
    Q_UNUSED(data);
    Q_UNUSED(maxSize);
    Q_UNREACHABLE();
#else
    qint64 bytesDecoded = 0;

    BrotliDecoderState *brotliDecoderState = toBrotliPointer(decoderPointer);

    while (decoderHasData && bytesDecoded < maxSize) {
        Q_ASSERT(brotliUnconsumedDataPtr || BrotliDecoderHasMoreOutput(brotliDecoderState));
        if (brotliUnconsumedDataPtr) {
            Q_ASSERT(brotliUnconsumedAmount);
            size_t toRead = std::min(size_t(maxSize - bytesDecoded), brotliUnconsumedAmount);
            memcpy(data + bytesDecoded, brotliUnconsumedDataPtr, toRead);
            bytesDecoded += toRead;
            brotliUnconsumedAmount -= toRead;
            brotliUnconsumedDataPtr += toRead;
            if (brotliUnconsumedAmount == 0) {
                brotliUnconsumedDataPtr = nullptr;
                decoderHasData = false;
            }
        }
        if (BrotliDecoderHasMoreOutput(brotliDecoderState) == BROTLI_TRUE) {
            brotliUnconsumedDataPtr =
                    BrotliDecoderTakeOutput(brotliDecoderState, &brotliUnconsumedAmount);
            decoderHasData = true;
        }
    }
    if (bytesDecoded == maxSize)
        return bytesDecoded;
    Q_ASSERT(bytesDecoded < maxSize);

    QByteArrayView input = compressedDataBuffer.readPointer();
    const uint8_t *encodedPtr = reinterpret_cast<const uint8_t *>(input.data());
    size_t encodedBytesRemaining = input.size();

    uint8_t *decodedPtr = reinterpret_cast<uint8_t *>(data + bytesDecoded);
    size_t unusedDecodedSize = size_t(maxSize - bytesDecoded);
    while (unusedDecodedSize > 0) {
        auto previousUnusedDecodedSize = unusedDecodedSize;
        BrotliDecoderResult result = BrotliDecoderDecompressStream(
                brotliDecoderState, &encodedBytesRemaining, &encodedPtr, &unusedDecodedSize,
                &decodedPtr, nullptr);
        bytesDecoded += previousUnusedDecodedSize - unusedDecodedSize;

        switch (result) {
        case BROTLI_DECODER_RESULT_ERROR:
            qWarning("Brotli error: %s",
                     BrotliDecoderErrorString(BrotliDecoderGetErrorCode(brotliDecoderState)));
            return -1;
        case BROTLI_DECODER_RESULT_SUCCESS:
            BrotliDecoderDestroyInstance(brotliDecoderState);
            decoderPointer = nullptr;
            compressedDataBuffer.clear();
            return bytesDecoded;
        case BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT:
            compressedDataBuffer.advanceReadPointer(input.size());
            input = compressedDataBuffer.readPointer();
            if (!input.isEmpty()) {
                encodedPtr = reinterpret_cast<const uint8_t *>(input.constData());
                encodedBytesRemaining = input.size();
                break;
            }
            return bytesDecoded;
        case BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT:
            // Some data is leftover inside the brotli decoder, remember for next time
            decoderHasData = BrotliDecoderHasMoreOutput(brotliDecoderState);
            Q_ASSERT(unusedDecodedSize == 0);
            break;
        }
    }
    compressedDataBuffer.advanceReadPointer(input.size() - encodedBytesRemaining);
    return bytesDecoded;
#endif
}

qsizetype QDecompressHelper::readZstandard(char *data, const qsizetype maxSize)
{
#if !QT_CONFIG(zstd)
    Q_UNUSED(data);
    Q_UNUSED(maxSize);
    Q_UNREACHABLE();
#else
    ZSTD_DStream *zstdStream = toZstandardPointer(decoderPointer);

    QByteArrayView input = compressedDataBuffer.readPointer();
    ZSTD_inBuffer inBuf { input.data(), size_t(input.size()), 0 };

    ZSTD_outBuffer outBuf { data, size_t(maxSize), 0 };

    qsizetype bytesDecoded = 0;
    while (outBuf.pos < outBuf.size && (inBuf.pos < inBuf.size || decoderHasData)) {
        size_t retValue = ZSTD_decompressStream(zstdStream, &outBuf, &inBuf);
        if (ZSTD_isError(retValue)) {
            qWarning("ZStandard error: %s", ZSTD_getErrorName(retValue));
            return -1;
        } else {
            decoderHasData = false;
            bytesDecoded = outBuf.pos;
            // if pos == size then there may be data left over in internal buffers
            if (outBuf.pos == outBuf.size) {
                decoderHasData = true;
            } else if (inBuf.pos == inBuf.size) {
                compressedDataBuffer.advanceReadPointer(input.size());
                input = compressedDataBuffer.readPointer();
                inBuf = { input.constData(), size_t(input.size()), 0 };
            }
        }
    }
    compressedDataBuffer.advanceReadPointer(inBuf.pos);
    return bytesDecoded;
#endif
}

QT_END_NAMESPACE
