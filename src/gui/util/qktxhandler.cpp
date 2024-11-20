/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qktxhandler_p.h"
#include "qtexturefiledata_p.h"
#include <QtEndian>
#include <QSize>

//#define KTX_DEBUG
#ifdef KTX_DEBUG
#include <QDebug>
#include <QMetaEnum>
#include <QOpenGLTexture>
#endif

QT_BEGIN_NAMESPACE

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

static bool qAddOverflow(quint32 v1, quint32 v2, quint32 *r) {
    // unsigned additions are well-defined
    *r = v1 + v2;
    return v1 > quint32(v1 + v2);
}

// Returns the nearest multiple of 4 greater than or equal to 'value'
static bool nearestMultipleOf4(quint32 value, quint32 *result)
{
    constexpr quint32 rounding = 4;
    *result = 0;
    if (qAddOverflow(value, rounding - 1, result))
        return true;
    *result &= ~(rounding - 1);
    return false;
}

// Returns a slice with prechecked bounds
static QByteArray safeSlice(const QByteArray& array, quint32 start, quint32 length)
{
    quint32 end = 0;
    if (qAddOverflow(start, length, &end) || end > quint32(array.length()))
        return {};
    return QByteArray(array.data() + start, length);
}

bool QKtxHandler::canRead(const QByteArray &suffix, const QByteArray &block)
{
    Q_UNUSED(suffix);
    return block.startsWith(QByteArray::fromRawData(ktxIdentifier, KTX_IDENTIFIER_LENGTH));
}

QTextureFileData QKtxHandler::read()
{
    if (!device())
        return QTextureFileData();

    const QByteArray buf = device()->readAll();
    if (size_t(buf.size()) > std::numeric_limits<quint32>::max()) {
        qWarning(lcQtGuiTextureIO, "Too big KTX file %s", logName().constData());
        return QTextureFileData();
    }

    if (!canRead(QByteArray(), buf)) {
        qWarning(lcQtGuiTextureIO, "Invalid KTX file %s", logName().constData());
        return QTextureFileData();
    }

    if (buf.size() < qsizetype(qktxh_headerSize)) {
        qWarning(lcQtGuiTextureIO, "Invalid KTX header size in %s", logName().constData());
        return QTextureFileData();
    }

    KTXHeader header;
    memcpy(&header, buf.data(), qktxh_headerSize);
    if (!checkHeader(header)) {
        qWarning(lcQtGuiTextureIO, "Unsupported KTX file format in %s", logName().constData());
        return QTextureFileData();
    }

    QTextureFileData texData;
    texData.setData(buf);

    texData.setSize(QSize(decode(header.pixelWidth), decode(header.pixelHeight)));
    texData.setGLFormat(decode(header.glFormat));
    texData.setGLInternalFormat(decode(header.glInternalFormat));
    texData.setGLBaseInternalFormat(decode(header.glBaseInternalFormat));

    texData.setNumLevels(decode(header.numberOfMipmapLevels));

    const quint32 bytesOfKeyValueData = decode(header.bytesOfKeyValueData);
    quint32 headerKeyValueSize;
    if (qAddOverflow(qktxh_headerSize, bytesOfKeyValueData, &headerKeyValueSize)) {
        qWarning(lcQtGuiTextureIO, "Overflow in size of key value data in header of KTX file %s",
                 logName().constData());
        return QTextureFileData();
    }

    if (headerKeyValueSize >= quint32(buf.size())) {
        qWarning(lcQtGuiTextureIO, "OOB request in KTX file %s", logName().constData());
        return QTextureFileData();
    }

    // Technically, any number of levels is allowed but if the value is bigger than
    // what is possible in KTX V2 (and what makes sense) we return an error.
    // maxLevels = log2(max(width, height, depth))
    const int maxLevels = (sizeof(quint32) * 8)
            - qCountLeadingZeroBits(std::max(
                    { header.pixelWidth, header.pixelHeight, header.pixelDepth }));

    if (texData.numLevels() > maxLevels) {
        qWarning(lcQtGuiTextureIO, "Too many levels in KTX file %s", logName().constData());
        return QTextureFileData();
    }

    quint32 offset = headerKeyValueSize;
    for (int level = 0; level < texData.numLevels(); level++) {
        const auto imageSizeSlice = safeSlice(buf, offset, sizeof(quint32));
        if (imageSizeSlice.isEmpty()) {
            qWarning(lcQtGuiTextureIO, "OOB request in KTX file %s", logName().constData());
            return QTextureFileData();
        }

        const quint32 imageSize = decode(qFromUnaligned<quint32>(imageSizeSlice.data()));
        offset += sizeof(quint32); // overflow checked indirectly above

        texData.setDataOffset(offset, level);
        texData.setDataLength(imageSize, level);

        // Add image data and padding to offset
        quint32 padded = 0;
        if (nearestMultipleOf4(imageSize, &padded)) {
            qWarning(lcQtGuiTextureIO, "Overflow in KTX file %s", logName().constData());
            return QTextureFileData();
        }

        quint32 offsetNext;
        if (qAddOverflow(offset, padded, &offsetNext)) {
            qWarning(lcQtGuiTextureIO, "OOB request in KTX file %s", logName().constData());
            return QTextureFileData();
        }

        offset = offsetNext;
    }

    if (!texData.isValid()) {
        qWarning(lcQtGuiTextureIO, "Invalid values in header of KTX file %s",
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
    qDebug("  glFormat: 0x%x (%s)", decode(header.glFormat), tfme.valueToKey(decode(header.glFormat)));
    qDebug("  glInternalFormat: 0x%x (%s)", decode(header.glInternalFormat), tfme.valueToKey(decode(header.glInternalFormat)));
    qDebug("  glBaseInternalFormat: 0x%x (%s)", decode(header.glBaseInternalFormat), tfme.valueToKey(decode(header.glBaseInternalFormat)));
    qDebug("  pixelWidth: %u", decode(header.pixelWidth));
    qDebug("  pixelHeight: %u", decode(header.pixelHeight));
    qDebug("  pixelDepth: %u", decode(header.pixelDepth));
    qDebug("  numberOfArrayElements: %u", decode(header.numberOfArrayElements));
    qDebug("  numberOfFaces: %u", decode(header.numberOfFaces));
    qDebug("  numberOfMipmapLevels: %u", decode(header.numberOfMipmapLevels));
    qDebug("  bytesOfKeyValueData: %u", decode(header.bytesOfKeyValueData));
#endif
    return ((decode(header.glType) == 0) &&
            (decode(header.glFormat) == 0) &&
            (decode(header.pixelDepth) == 0) &&
            (decode(header.numberOfFaces) == 1));
}

quint32 QKtxHandler::decode(quint32 val) const
{
    return inverseEndian ? qbswap<quint32>(val) : val;
}

QT_END_NAMESPACE
