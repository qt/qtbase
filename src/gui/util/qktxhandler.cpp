// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qktxhandler_p.h"
#include "qtexturefiledata_p.h"
#include <QtEndian>
#include <QSize>
#include <QMap>
#include <QtCore/qiodevice.h>

//#define KTX_DEBUG
#ifdef KTX_DEBUG
#include <QDebug>
#include <QMetaEnum>
#include <QOpenGLTexture>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#define KTX_IDENTIFIER_LENGTH 12
static const char ktxIdentifier[KTX_IDENTIFIER_LENGTH] = { '\xAB', 'K', 'T', 'X', ' ', '1', '1', '\xBB', '\r', '\n', '\x1A', '\n' };
static const quint32 platformEndianIdentifier = 0x04030201;
static const quint32 inversePlatformEndianIdentifier = 0x01020304;

struct KTXHeader {
    quint8 identifier[KTX_IDENTIFIER_LENGTH]; // Must match ktxIdentifier
    quint32 endianness; // Either platformEndianIdentifier or inversePlatformEndianIdentifier, other values not allowed.
    quint32 glType;
    quint32 glTypeSize;
    quint32 glFormat;
    quint32 glInternalFormat;
    quint32 glBaseInternalFormat;
    quint32 pixelWidth;
    quint32 pixelHeight;
    quint32 pixelDepth;
    quint32 numberOfArrayElements;
    quint32 numberOfFaces;
    quint32 numberOfMipmapLevels;
    quint32 bytesOfKeyValueData;
};

static const quint32 qktxh_headerSize = sizeof(KTXHeader);

// Currently unused, declared for future reference
struct KTXKeyValuePairItem {
    quint32   keyAndValueByteSize;
    /*
    quint8 keyAndValue[keyAndValueByteSize];
    quint8 valuePadding[3 - ((keyAndValueByteSize + 3) % 4)];
    */
};

struct KTXMipmapLevel {
    quint32 imageSize;
    /*
    for each array_element in numberOfArrayElements*
        for each face in numberOfFaces
            for each z_slice in pixelDepth*
                for each row or row_of_blocks in pixelHeight*
                    for each pixel or block_of_pixels in pixelWidth
                        Byte data[format-specific-number-of-bytes]**
                    end
                end
            end
            Byte cubePadding[0-3]
        end
    end
    quint8 mipPadding[3 - ((imageSize + 3) % 4)]
    */
};

// Returns the nearest multiple of 'rounding' greater than or equal to 'value'
constexpr quint32 withPadding(quint32 value, quint32 rounding)
{
    Q_ASSERT(rounding > 1);
    return value + (rounding - 1) - ((value + (rounding - 1)) % rounding);
}

QKtxHandler::~QKtxHandler() = default;

bool QKtxHandler::canRead(const QByteArray &suffix, const QByteArray &block)
{
    Q_UNUSED(suffix);

    return (qstrncmp(block.constData(), ktxIdentifier, KTX_IDENTIFIER_LENGTH) == 0);
}

QTextureFileData QKtxHandler::read()
{
    if (!device())
        return QTextureFileData();

    const QByteArray buf = device()->readAll();
    const quint32 dataSize = quint32(buf.size());
    if (dataSize < qktxh_headerSize || !canRead(QByteArray(), buf)) {
        qCDebug(lcQtGuiTextureIO, "Invalid KTX file %s", logName().constData());
        return QTextureFileData();
    }

    const KTXHeader *header = reinterpret_cast<const KTXHeader *>(buf.data());
    if (!checkHeader(*header)) {
        qCDebug(lcQtGuiTextureIO, "Unsupported KTX file format in %s", logName().constData());
        return QTextureFileData();
    }

    QTextureFileData texData;
    texData.setData(buf);

    texData.setSize(QSize(decode(header->pixelWidth), decode(header->pixelHeight)));
    texData.setGLFormat(decode(header->glFormat));
    texData.setGLInternalFormat(decode(header->glInternalFormat));
    texData.setGLBaseInternalFormat(decode(header->glBaseInternalFormat));

    texData.setNumLevels(decode(header->numberOfMipmapLevels));
    texData.setNumFaces(decode(header->numberOfFaces));

    const quint32 bytesOfKeyValueData = decode(header->bytesOfKeyValueData);
    if (qktxh_headerSize + bytesOfKeyValueData < quint64(buf.size())) // oob check
        texData.setKeyValueMetadata(decodeKeyValues(
                QByteArrayView(buf.data() + qktxh_headerSize, bytesOfKeyValueData)));
    quint32 offset = qktxh_headerSize + bytesOfKeyValueData;

    constexpr int MAX_ITERATIONS = 32; // cap iterations in case of corrupt data

    for (int level = 0; level < qMin(texData.numLevels(), MAX_ITERATIONS); level++) {
        if (offset + sizeof(quint32) > dataSize) // Corrupt file; avoid oob read
            break;

        const quint32 imageSize = decode(qFromUnaligned<quint32>(buf.data() + offset));
        offset += sizeof(quint32);

        for (int face = 0; face < qMin(texData.numFaces(), MAX_ITERATIONS); face++) {
            texData.setDataOffset(offset, level, face);
            texData.setDataLength(imageSize, level, face);

            // Add image data and padding to offset
            offset += withPadding(imageSize, 4);
        }
    }

    if (!texData.isValid()) {
        qCDebug(lcQtGuiTextureIO, "Invalid values in header of KTX file %s", logName().constData());
        return QTextureFileData();
    }

    texData.setLogName(logName());

#ifdef KTX_DEBUG
    qDebug() << "KTX file handler read" << texData;
#endif

    return texData;
}

bool QKtxHandler::checkHeader(const KTXHeader &header)
{
    if (header.endianness != platformEndianIdentifier && header.endianness != inversePlatformEndianIdentifier)
        return false;
    inverseEndian = (header.endianness == inversePlatformEndianIdentifier);
#ifdef KTX_DEBUG
    QMetaEnum tfme = QMetaEnum::fromType<QOpenGLTexture::TextureFormat>();
    QMetaEnum ptme = QMetaEnum::fromType<QOpenGLTexture::PixelType>();
    qDebug("Header of %s:", logName().constData());
    qDebug("  glType: 0x%x (%s)", decode(header.glType), ptme.valueToKey(decode(header.glType)));
    qDebug("  glTypeSize: %u", decode(header.glTypeSize));
    qDebug("  glFormat: 0x%x (%s)", decode(header.glFormat),
           tfme.valueToKey(decode(header.glFormat)));
    qDebug("  glInternalFormat: 0x%x (%s)", decode(header.glInternalFormat),
           tfme.valueToKey(decode(header.glInternalFormat)));
    qDebug("  glBaseInternalFormat: 0x%x (%s)", decode(header.glBaseInternalFormat),
           tfme.valueToKey(decode(header.glBaseInternalFormat)));
    qDebug("  pixelWidth: %u", decode(header.pixelWidth));
    qDebug("  pixelHeight: %u", decode(header.pixelHeight));
    qDebug("  pixelDepth: %u", decode(header.pixelDepth));
    qDebug("  numberOfArrayElements: %u", decode(header.numberOfArrayElements));
    qDebug("  numberOfFaces: %u", decode(header.numberOfFaces));
    qDebug("  numberOfMipmapLevels: %u", decode(header.numberOfMipmapLevels));
    qDebug("  bytesOfKeyValueData: %u", decode(header.bytesOfKeyValueData));
#endif
    const bool isCompressedImage = decode(header.glType) == 0 && decode(header.glFormat) == 0
            && decode(header.pixelDepth) == 0;
    const bool isCubeMap = decode(header.numberOfFaces) == 6;
    const bool is2D = decode(header.pixelDepth) == 0 && decode(header.numberOfArrayElements) == 0;

    return is2D && (isCubeMap || isCompressedImage);
}

QMap<QByteArray, QByteArray> QKtxHandler::decodeKeyValues(QByteArrayView view) const
{
    QMap<QByteArray, QByteArray> output;
    quint32 offset = 0;
    while (offset < view.size() + sizeof(quint32)) {
        const quint32 keyAndValueByteSize =
                decode(qFromUnaligned<quint32>(view.constData() + offset));
        offset += sizeof(quint32);

        if (offset + keyAndValueByteSize > quint64(view.size()))
            break; // oob read

        // 'key' is a UTF-8 string ending with a null terminator, 'value' is the rest.
        // To separate the key and value we convert the complete data to utf-8 and find the first
        // null terminator from the left, here we split the data into two.
        const auto str = QString::fromUtf8(view.constData() + offset, keyAndValueByteSize);
        const int idx = str.indexOf('\0'_L1);
        if (idx == -1)
            continue;

        const QByteArray key = str.left(idx).toUtf8();
        const size_t keySize = key.size() + 1; // Actual data size
        const QByteArray value = QByteArray::fromRawData(view.constData() + offset + keySize,
                                                         keyAndValueByteSize - keySize);

        offset = withPadding(offset + keyAndValueByteSize, 4);
        output.insert(key, value);
    }

    return output;
}

quint32 QKtxHandler::decode(quint32 val) const
{
    return inverseEndian ? qbswap<quint32>(val) : val;
}

QT_END_NAMESPACE
