// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpkmhandler_p.h"
#include "qtexturefiledata_p.h"

#include <QFile>
#include <QDebug>
#include <QSize>
#include <qendian.h>

//#define ETC_DEBUG

QT_BEGIN_NAMESPACE

static const int qpkmh_headerSize = 16;

struct PkmType
{
    quint32 glFormat;
    quint32 bytesPerBlock;
};

static constexpr PkmType typeMap[5] = {
    { 0x8D64,  8 },   // GL_ETC1_RGB8_OES
    { 0x9274,  8 },   // GL_COMPRESSED_RGB8_ETC2
    { 0, 0 },         // unused (obsolete)
    { 0x9278, 16},    // GL_COMPRESSED_RGBA8_ETC2_EAC
    { 0x9276,  8 }    // GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
};

QPkmHandler::~QPkmHandler() = default;

bool QPkmHandler::canRead(const QByteArray &suffix, const QByteArray &block)
{
    Q_UNUSED(suffix);

    return block.startsWith("PKM ");
}

QTextureFileData QPkmHandler::read()
{
    QTextureFileData texData;

    if (!device())
        return texData;

    QByteArray fileData = device()->readAll();
    if (fileData.size() < qpkmh_headerSize || !canRead(QByteArray(), fileData)) {
        qCDebug(lcQtGuiTextureIO, "Invalid PKM file %s", logName().constData());
        return QTextureFileData();
    }
    texData.setData(fileData);

    const char *rawData = fileData.constData();

    // ignore version (rawData + 4 & 5)

    // texture type
    quint16 type = qFromBigEndian<quint16>(rawData + 6);
    if (type >= sizeof(typeMap)/sizeof(typeMap[0])) {
        qCDebug(lcQtGuiTextureIO, "Unknown compression format in PKM file %s", logName().constData());
        return QTextureFileData();
    }
    texData.setGLFormat(0);                    // 0 for compressed textures
    texData.setGLInternalFormat(typeMap[type].glFormat);
    //### setBaseInternalFormat

    // texture size
    texData.setNumLevels(1);
    texData.setNumFaces(1);
    const int bpb = typeMap[type].bytesPerBlock;
    QSize paddedSize(qFromBigEndian<quint16>(rawData + 8), qFromBigEndian<quint16>(rawData + 10));
    texData.setDataLength((paddedSize.width() / 4) * (paddedSize.height() / 4) * bpb);
    QSize texSize(qFromBigEndian<quint16>(rawData + 12), qFromBigEndian<quint16>(rawData + 14));
    texData.setSize(texSize);

    texData.setDataOffset(qpkmh_headerSize);

    if (!texData.isValid()) {
        qCDebug(lcQtGuiTextureIO, "Invalid values in header of PKM file %s", logName().constData());
        return QTextureFileData();
    }

    texData.setLogName(logName());

#ifdef ETC_DEBUG
    qDebug() << "PKM file handler read" << texData;
#endif
    return texData;
}

QT_END_NAMESPACE
