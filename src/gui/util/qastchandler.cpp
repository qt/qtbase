// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qastchandler_p.h"
#include "qtexturefiledata_p.h"

#include <private/qnumeric_p.h>

#include <QFile>
#include <QDebug>
#include <QSize>

QT_BEGIN_NAMESPACE

struct AstcHeader
{
    quint8 magic[4];
    quint8 blockDimX;
    quint8 blockDimY;
    quint8 blockDimZ;
    quint8 xSize[3];
    quint8 ySize[3];
    quint8 zSize[3];
};

QAstcHandler::~QAstcHandler() = default;

bool QAstcHandler::canRead(const QByteArray &suffix, const QByteArray &block)
{
    Q_UNUSED(suffix);

    return block.startsWith("\x13\xAB\xA1\x5C");
}

quint32 QAstcHandler::astcGLFormat(quint8 xBlockDim, quint8 yBlockDim) const
{
    static const quint32 glFormatRGBABase = 0x93B0;    // GL_COMPRESSED_RGBA_ASTC_4x4_KHR
    static const quint32 glFormatSRGBBase = 0x93D0;    // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR

    Q_CONSTINIT static QSize dims[14] = {
        {  4, 4  },     // GL_COMPRESSED_xxx_ASTC_4x4_KHR
        {  5, 4  },     // GL_COMPRESSED_xxx_ASTC_5x4_KHR
        {  5, 5  },     // GL_COMPRESSED_xxx_ASTC_5x5_KHR
        {  6, 5  },     // GL_COMPRESSED_xxx_ASTC_6x5_KHR
        {  6, 6  },     // GL_COMPRESSED_xxx_ASTC_6x6_KHR
        {  8, 5  },     // GL_COMPRESSED_xxx_ASTC_8x5_KHR
        {  8, 6  },     // GL_COMPRESSED_xxx_ASTC_8x6_KHR
        {  8, 8  },     // GL_COMPRESSED_xxx_ASTC_8x8_KHR
        { 10, 5  },     // GL_COMPRESSED_xxx_ASTC_10x5_KHR
        { 10, 6  },     // GL_COMPRESSED_xxx_ASTC_10x6_KHR
        { 10, 8  },     // GL_COMPRESSED_xxx_ASTC_10x8_KHR
        { 10, 10 },     // GL_COMPRESSED_xxx_ASTC_10x10_KHR
        { 12, 10 },     // GL_COMPRESSED_xxx_ASTC_12x10_KHR
        { 12, 12 }      // GL_COMPRESSED_xxx_ASTC_12x12_KHR
    };

    const QSize dim(xBlockDim, yBlockDim);
    int index = -1;
    for (int i = 0; i < 14; i++) {
        if (dim == dims[i]) {
            index = i;
            break;
        }
    }
    if (index < 0)
        return 0;

    bool useSrgb = qEnvironmentVariableIsSet("QT_ASTCHANDLER_USE_SRGB")
                   || logName().toLower().contains("srgb");

    return useSrgb ? (glFormatSRGBBase + index) : (glFormatRGBABase + index);
}

QTextureFileData QAstcHandler::read()
{
    QTextureFileData nullData;
    QTextureFileData res;

    if (!device())
        return nullData;

    QByteArray fileData = device()->readAll();
    if (fileData.size() < int(sizeof(AstcHeader)) || !canRead(QByteArray(), fileData)) {
        qCDebug(lcQtGuiTextureIO, "Not an ASTC file: %s", logName().constData());
        return nullData;
    }
    res.setData(fileData);

    const AstcHeader *header = reinterpret_cast<const AstcHeader *>(fileData.constData());

    int xSz = int(header->xSize[0]) | int(header->xSize[1]) << 8 | int(header->xSize[2]) << 16;
    int ySz = int(header->ySize[0]) | int(header->ySize[1]) << 8 | int(header->ySize[2]) << 16;
    int zSz = int(header->zSize[0]) | int(header->zSize[1]) << 8 | int(header->zSize[2]) << 16;

    quint32 glFmt = astcGLFormat(header->blockDimX, header->blockDimY);

    if (!xSz || !ySz || !zSz || !glFmt || header->blockDimZ != 1) {
        qCDebug(lcQtGuiTextureIO, "Invalid ASTC header data in file %s", logName().constData());
        return nullData;
    }

    res.setSize(QSize(xSz, ySz));
    res.setGLFormat(0);                    // 0 for compressed textures
    res.setGLInternalFormat(glFmt);
    //? BaseInternalFormat

    int xBlocks = (xSz + header->blockDimX - 1) / header->blockDimX;
    int yBlocks = (ySz + header->blockDimY - 1) / header->blockDimY;
    int zBlocks = (zSz + header->blockDimZ - 1) / header->blockDimZ;

    int byteCount = 0;
    bool oob = qMulOverflow(xBlocks, yBlocks, &byteCount)
               || qMulOverflow(byteCount, zBlocks, &byteCount)
               || qMulOverflow(byteCount, 16, &byteCount);


    res.setDataOffset(sizeof(AstcHeader));
    res.setNumLevels(1);
    res.setNumFaces(1);
    res.setDataLength(byteCount);

    if (oob || !res.isValid()) {
        qCDebug(lcQtGuiTextureIO, "Invalid ASTC file %s", logName().constData());
        return nullData;
    }

    res.setLogName(logName());

#if 0
    qDebug() << "ASTC file handler read" << res << res.dataOffset() << res.dataLength();
#endif
    return res;
}

QT_END_NAMESPACE
