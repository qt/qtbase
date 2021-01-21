/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qpkmhandler_p.h"
#include "qtexturefiledata_p.h"

#include <QFile>
#include <QDebug>
#include <QSize>
#include <qendian.h>

//#define ETC_DEBUG

QT_BEGIN_NAMESPACE

static const int headerSize = 16;

struct PkmType
{
    quint32 glFormat;
    quint32 bytesPerBlock;
};

static PkmType typeMap[5] = {
    { 0x8D64,  8 },   // GL_ETC1_RGB8_OES
    { 0x9274,  8 },   // GL_COMPRESSED_RGB8_ETC2
    { 0, 0 },         // unused (obsolete)
    { 0x9278, 16},    // GL_COMPRESSED_RGBA8_ETC2_EAC
    { 0x9276,  8 }    // GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
};

bool QPkmHandler::canRead(const QByteArray &suffix, const QByteArray &block)
{
    Q_UNUSED(suffix)

    return block.startsWith("PKM ");
}

QTextureFileData QPkmHandler::read()
{
    QTextureFileData texData;

    if (!device())
        return texData;

    QByteArray fileData = device()->readAll();
    if (fileData.size() < headerSize || !canRead(QByteArray(), fileData)) {
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
    const int bpb = typeMap[type].bytesPerBlock;
    QSize paddedSize(qFromBigEndian<quint16>(rawData + 8), qFromBigEndian<quint16>(rawData + 10));
    texData.setDataLength((paddedSize.width() / 4) * (paddedSize.height() / 4) * bpb);
    QSize texSize(qFromBigEndian<quint16>(rawData + 12), qFromBigEndian<quint16>(rawData + 14));
    texData.setSize(texSize);

    texData.setDataOffset(headerSize);

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
