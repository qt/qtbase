// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

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

static constexpr quint32 qktxh_headerSize = sizeof(KTXHeader);

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

// Returns the nearest multiple of 4 greater than or equal to 'value'
static const std::optional<quint32> nearestMultipleOf4(quint32 value)
{
    constexpr quint32 rounding = 4;
    quint32 result = 0;
    if (qAddOverflow(value, rounding - 1, &result))
        return std::nullopt;
    result &= ~(rounding - 1);
    return result;
}

// Returns a view with prechecked bounds
static QByteArrayView safeView(QByteArrayView view, quint32 start, quint32 length)
{
    quint32 end = 0;
    if (qAddOverflow(start, length, &end) || end > quint32(view.length()))
        return {};
    return view.sliced(start, length);
}

QKtxHandler::~QKtxHandler() = default;

bool QKtxHandler::canRead(const QByteArray &suffix, const QByteArray &block)
{
    Q_UNUSED(suffix);
    return block.startsWith(ktxIdentifier);
}

QTextureFileData QKtxHandler::read()
{
    if (!device())
        return QTextureFileData();

    const QByteArray buf = device()->readAll();
    if (static_cast<size_t>(buf.size()) > std::numeric_limits<quint32>::max()) {
        qCWarning(lcQtGuiTextureIO, "Too big KTX file %s", logName().constData());
        return QTextureFileData();
    }

    if (!canRead(QByteArray(), buf)) {
        qCWarning(lcQtGuiTextureIO, "Invalid KTX file %s", logName().constData());
        return QTextureFileData();
    }

    if (buf.size() < qsizetype(qktxh_headerSize)) {
        qCWarning(lcQtGuiTextureIO, "Invalid KTX header size in %s", logName().constData());
        return QTextureFileData();
    }

    KTXHeader header;
    memcpy(&header, buf.data(), qktxh_headerSize);
    if (!checkHeader(header)) {
        qCWarning(lcQtGuiTextureIO, "Unsupported KTX file format in %s", logName().constData());
        return QTextureFileData();
    }

    QTextureFileData texData;
    texData.setData(buf);

    texData.setSize(QSize(decode(header.pixelWidth), decode(header.pixelHeight)));
    texData.setGLFormat(decode(header.glFormat));
    texData.setGLInternalFormat(decode(header.glInternalFormat));
    texData.setGLBaseInternalFormat(decode(header.glBaseInternalFormat));

    texData.setNumLevels(decode(header.numberOfMipmapLevels));
    texData.setNumFaces(decode(header.numberOfFaces));

    const quint32 bytesOfKeyValueData = decode(header.bytesOfKeyValueData);
    quint32 headerKeyValueSize;
    if (qAddOverflow(qktxh_headerSize, bytesOfKeyValueData, &headerKeyValueSize)) {
        qCWarning(lcQtGuiTextureIO, "Overflow in size of key value data in header of KTX file %s",
                 logName().constData());
        return QTextureFileData();
    }

    if (headerKeyValueSize >= quint32(buf.size())) {
        qCWarning(lcQtGuiTextureIO, "OOB request in KTX file %s", logName().constData());
        return QTextureFileData();
    }

    // File contains key/values
    if (bytesOfKeyValueData > 0) {
        auto keyValueDataView = safeView(buf, qktxh_headerSize, bytesOfKeyValueData);
        if (keyValueDataView.isEmpty()) {
            qCWarning(lcQtGuiTextureIO, "Invalid view in KTX file %s", logName().constData());
            return QTextureFileData();
        }

        auto keyValues = decodeKeyValues(keyValueDataView);
        if (!keyValues) {
            qCWarning(lcQtGuiTextureIO, "Could not parse key values in KTX file %s",
                     logName().constData());
            return QTextureFileData();
        }

        texData.setKeyValueMetadata(*keyValues);
    }

    // Technically, any number of levels is allowed but if the value is bigger than
    // what is possible in KTX V2 (and what makes sense) we return an error.
    // maxLevels = log2(max(width, height, depth))
    const int maxLevels = (sizeof(quint32) * 8)
            - qCountLeadingZeroBits(std::max(
                    { header.pixelWidth, header.pixelHeight, header.pixelDepth }));

    if (texData.numLevels() > maxLevels) {
        qCWarning(lcQtGuiTextureIO, "Too many levels in KTX file %s", logName().constData());
        return QTextureFileData();
    }

    if (texData.numFaces() != 1 && texData.numFaces() != 6) {
        qCWarning(lcQtGuiTextureIO, "Invalid number of faces in KTX file %s", logName().constData());
        return QTextureFileData();
    }

    quint32 offset = headerKeyValueSize;
    for (int level = 0; level < texData.numLevels(); level++) {
        const auto imageSizeView = safeView(buf, offset, sizeof(quint32));
        if (imageSizeView.isEmpty()) {
            qCWarning(lcQtGuiTextureIO, "OOB request in KTX file %s", logName().constData());
            return QTextureFileData();
        }

        const quint32 imageSize = decode(qFromUnaligned<quint32>(imageSizeView.data()));
        offset += sizeof(quint32); // overflow checked indirectly above

        for (int face = 0; face < texData.numFaces(); face++) {
            texData.setDataOffset(offset, level, face);
            texData.setDataLength(imageSize, level, face);

            // Add image data and padding to offset
            const auto padded = nearestMultipleOf4(imageSize);
            if (!padded) {
                qCWarning(lcQtGuiTextureIO, "Overflow in KTX file %s", logName().constData());
                return QTextureFileData();
            }

            quint32 offsetNext;
            if (qAddOverflow(offset, *padded, &offsetNext)) {
                qCWarning(lcQtGuiTextureIO, "OOB request in KTX file %s", logName().constData());
                return QTextureFileData();
            }

            offset = offsetNext;
        }
    }

    if (!texData.isValid()) {
        qCWarning(lcQtGuiTextureIO, "Invalid values in header of KTX file %s",
                 logName().constData());
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

std::optional<QMap<QByteArray, QByteArray>> QKtxHandler::decodeKeyValues(QByteArrayView view) const
{
    QMap<QByteArray, QByteArray> output;
    quint32 offset = 0;
    while (offset < quint32(view.size())) {
        const auto keyAndValueByteSizeView = safeView(view, offset, sizeof(quint32));
        if (keyAndValueByteSizeView.isEmpty()) {
            qCWarning(lcQtGuiTextureIO, "Invalid view in KTX key-value");
            return std::nullopt;
        }

        const quint32 keyAndValueByteSize =
                decode(qFromUnaligned<quint32>(keyAndValueByteSizeView.data()));

        quint32 offsetKeyAndValueStart;
        if (qAddOverflow(offset, quint32(sizeof(quint32)), &offsetKeyAndValueStart)) {
            qCWarning(lcQtGuiTextureIO, "Overflow in KTX key-value");
            return std::nullopt;
        }

        quint32 offsetKeyAndValueEnd;
        if (qAddOverflow(offsetKeyAndValueStart, keyAndValueByteSize, &offsetKeyAndValueEnd)) {
            qCWarning(lcQtGuiTextureIO, "Overflow in KTX key-value");
            return std::nullopt;
        }

        const auto keyValueView = safeView(view, offsetKeyAndValueStart, keyAndValueByteSize);
        if (keyValueView.isEmpty()) {
            qCWarning(lcQtGuiTextureIO, "Invalid view in KTX key-value");
            return std::nullopt;
        }

        // 'key' is a UTF-8 string ending with a null terminator, 'value' is the rest.
        // To separate the key and value we convert the complete data to utf-8 and find the first
        // null terminator from the left, here we split the data into two.

        const int idx = keyValueView.indexOf('\0');
        if (idx == -1) {
            qCWarning(lcQtGuiTextureIO, "Invalid key in KTX key-value");
            return std::nullopt;
        }

        const QByteArrayView keyView = safeView(view, offsetKeyAndValueStart, idx);
        if (keyView.isEmpty()) {
            qCWarning(lcQtGuiTextureIO, "Overflow in KTX key-value");
            return std::nullopt;
        }

        const quint32 keySize = idx + 1; // Actual data size

        quint32 offsetValueStart;
        if (qAddOverflow(offsetKeyAndValueStart, keySize, &offsetValueStart)) {
            qCWarning(lcQtGuiTextureIO, "Overflow in KTX key-value");
            return std::nullopt;
        }

        quint32 valueSize;
        if (qSubOverflow(keyAndValueByteSize, keySize, &valueSize)) {
            qCWarning(lcQtGuiTextureIO, "Underflow in KTX key-value");
            return std::nullopt;
        }

        const QByteArrayView valueView = safeView(view, offsetValueStart, valueSize);
        if (valueView.isEmpty()) {
            qCWarning(lcQtGuiTextureIO, "Invalid view in KTX key-value");
            return std::nullopt;
        }

        output.insert(keyView.toByteArray(), valueView.toByteArray());

        const auto offsetNext = nearestMultipleOf4(offsetKeyAndValueEnd);
        if (!offsetNext) {
            qCWarning(lcQtGuiTextureIO, "Overflow in KTX key-value");
            return std::nullopt;
        }

        offset = *offsetNext;
    }

    return output;
}

quint32 QKtxHandler::decode(quint32 val) const
{
    return inverseEndian ? qbswap<quint32>(val) : val;
}

QT_END_NAMESPACE
