// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef DECOMPRESS_HELPER_P_H
#define DECOMPRESS_HELPER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of the Network Access API. This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include <QtCore/private/qbytedata_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QIODevice;
class Q_AUTOTEST_EXPORT QDecompressHelper
{
public:
    enum ContentEncoding {
        None,
        Deflate,
        GZip,
        Brotli,
        Zstandard,
    };

    QDecompressHelper() = default;
    ~QDecompressHelper();

    bool setEncoding(const QByteArray &contentEncoding);

    bool isCountingBytes() const;
    void setCountingBytesEnabled(bool shouldCount);

    qint64 uncompressedSize() const;

    bool hasData() const;
    void feed(const QByteArray &data);
    void feed(QByteArray &&data);
    void feed(const QByteDataBuffer &buffer);
    void feed(QByteDataBuffer &&buffer);
    qsizetype read(char *data, qsizetype maxSize);

    bool isValid() const;

    void clear();

    void setDecompressedSafetyCheckThreshold(qint64 threshold);

    static bool isSupportedEncoding(const QByteArray &encoding);
    static QByteArrayList acceptedEncoding();

    QString errorString() const;

private:
    bool isPotentialArchiveBomb() const;
    bool hasDataInternal() const;
    qsizetype readInternal(char *data, qsizetype maxSize);

    bool countInternal();
    bool countInternal(const QByteArray &data);
    bool countInternal(const QByteDataBuffer &buffer);

    bool setEncoding(ContentEncoding ce);
    qint64 encodedBytesAvailable() const;

    qsizetype readZLib(char *data, qsizetype maxSize);
    qsizetype readBrotli(char *data, qsizetype maxSize);
    qsizetype readZstandard(char *data, qsizetype maxSize);

    QByteDataBuffer compressedDataBuffer;
    QByteDataBuffer decompressedDataBuffer;
    const qsizetype MaxDecompressedDataBufferSize = 10 * 1024 * 1024;
    bool decoderHasData = false;

    bool countDecompressed = false;
    std::unique_ptr<QDecompressHelper> countHelper;

    QString errorStr;

    // Used for calculating the ratio
    qint64 archiveBombCheckThreshold = 10 * 1024 * 1024;
    qint64 totalUncompressedBytes = 0;
    qint64 totalCompressedBytes = 0;
    qint64 totalBytesRead = 0;

    ContentEncoding contentEncoding = None;

    void *decoderPointer = nullptr;
#if QT_CONFIG(brotli)
    const uint8_t *brotliUnconsumedDataPtr = nullptr;
    size_t brotliUnconsumedAmount = 0;
#endif
};

QT_END_NAMESPACE

#endif // DECOMPRESS_HELPER_P_H
