// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qinternalmimedata_p.h"

#include <QtCore/qbuffer.h>
#include <QtGui/qimage.h>
#include <QtGui/qimagereader.h>
#include <QtGui/qimagewriter.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static QStringList imageMimeFormats(const QList<QByteArray> &imageFormats)
{
    QStringList formats;
    formats.reserve(imageFormats.size());
    for (const auto &format : imageFormats)
        formats.append("image/"_L1 + QLatin1StringView(format.toLower()));

    //put png at the front because it is best
    const qsizetype pngIndex = formats.indexOf("image/png"_L1);
    if (pngIndex != -1 && pngIndex != 0)
        formats.move(pngIndex, 0);

    return formats;
}

static inline QStringList imageReadMimeFormats()
{
    return imageMimeFormats(QImageReader::supportedImageFormats());
}

static inline QStringList imageWriteMimeFormats()
{
    return imageMimeFormats(QImageWriter::supportedImageFormats());
}

QInternalMimeData::QInternalMimeData()
    : QMimeData()
{
}

QInternalMimeData::~QInternalMimeData()
{
}

bool QInternalMimeData::hasFormat(const QString &mimeType) const
{
    bool foundFormat = hasFormat_sys(mimeType);
    if (!foundFormat && mimeType == "application/x-qt-image"_L1) {
        QStringList imageFormats = imageReadMimeFormats();
        for (int i = 0; i < imageFormats.size(); ++i) {
            if ((foundFormat = hasFormat_sys(imageFormats.at(i))))
                break;
        }
    }
    return foundFormat;
}

QStringList QInternalMimeData::formats() const
{
    QStringList realFormats = formats_sys();
    if (!realFormats.contains("application/x-qt-image"_L1)) {
        QStringList imageFormats = imageReadMimeFormats();
        for (int i = 0; i < imageFormats.size(); ++i) {
            if (realFormats.contains(imageFormats.at(i))) {
                realFormats += "application/x-qt-image"_L1;
                break;
            }
        }
    }
    return realFormats;
}

QVariant QInternalMimeData::retrieveData(const QString &mimeType, QMetaType type) const
{
    QVariant data = retrieveData_sys(mimeType, type);
    if (mimeType == "application/x-qt-image"_L1) {
        if (data.isNull() || (data.metaType().id() == QMetaType::QByteArray && data.toByteArray().isEmpty())) {
            // try to find an image
            QStringList imageFormats = imageReadMimeFormats();
            for (int i = 0; i < imageFormats.size(); ++i) {
                data = retrieveData_sys(imageFormats.at(i), type);
                if (data.isNull() || (data.metaType().id() == QMetaType::QByteArray && data.toByteArray().isEmpty()))
                    continue;
                break;
            }
        }
        int typeId = type.id();
        // we wanted some image type, but all we got was a byte array. Convert it to an image.
        if (data.metaType().id() == QMetaType::QByteArray
            && (typeId == QMetaType::QImage || typeId == QMetaType::QPixmap || typeId == QMetaType::QBitmap))
            data = QImage::fromData(data.toByteArray());

    } else if (mimeType == "application/x-color"_L1 && data.metaType().id() == QMetaType::QByteArray) {
        QColor c;
        QByteArray ba = data.toByteArray();
        if (ba.size() == 8) {
            ushort * colBuf = (ushort *)ba.data();
            c.setRgbF(qreal(colBuf[0]) / qreal(0xFFFF),
                      qreal(colBuf[1]) / qreal(0xFFFF),
                      qreal(colBuf[2]) / qreal(0xFFFF),
                      qreal(colBuf[3]) / qreal(0xFFFF));
            data = c;
        } else {
            qWarning("Qt: Invalid color format");
        }
    } else if (data.metaType() != type && data.metaType().id() == QMetaType::QByteArray) {
        // try to use mime data's internal conversion stuf.
        QInternalMimeData *that = const_cast<QInternalMimeData *>(this);
        that->setData(mimeType, data.toByteArray());
        data = QMimeData::retrieveData(mimeType, type);
        that->clear();
    }
    return data;
}

bool QInternalMimeData::canReadData(const QString &mimeType)
{
    return imageReadMimeFormats().contains(mimeType);
}

// helper functions for rendering mimedata to the system, this is needed because QMimeData is in core.
QStringList QInternalMimeData::formatsHelper(const QMimeData *data)
{
    QStringList realFormats = data->formats();
    if (realFormats.contains("application/x-qt-image"_L1)) {
        // add all supported image formats
        QStringList imageFormats = imageWriteMimeFormats();
        for (int i = 0; i < imageFormats.size(); ++i) {
            if (!realFormats.contains(imageFormats.at(i)))
                realFormats.append(imageFormats.at(i));
        }
    }
    return realFormats;
}

bool QInternalMimeData::hasFormatHelper(const QString &mimeType, const QMimeData *data)
{

    bool foundFormat = data->hasFormat(mimeType);
    if (!foundFormat) {
        if (mimeType == "application/x-qt-image"_L1) {
            // check all supported image formats
            QStringList imageFormats = imageWriteMimeFormats();
            for (int i = 0; i < imageFormats.size(); ++i) {
                if ((foundFormat = data->hasFormat(imageFormats.at(i))))
                    break;
            }
        } else if (mimeType.startsWith("image/"_L1)) {
            return data->hasImage() && imageWriteMimeFormats().contains(mimeType);
        }
    }
    return foundFormat;
}

QByteArray QInternalMimeData::renderDataHelper(const QString &mimeType, const QMimeData *data)
{
    QByteArray ba;
    if (mimeType == "application/x-color"_L1) {
        /* QMimeData can only provide colors as QColor or the name
           of a color as a QByteArray or a QString. So we need to do
           the conversion to application/x-color here.
           The application/x-color format is :
           type: application/x-color
           format: 16
           data[0]: red
           data[1]: green
           data[2]: blue
           data[3]: opacity
        */
        ba.resize(8);
        ushort * colBuf = (ushort *)ba.data();
        QColor c = qvariant_cast<QColor>(data->colorData());
        colBuf[0] = ushort(c.redF() * 0xFFFF);
        colBuf[1] = ushort(c.greenF() * 0xFFFF);
        colBuf[2] = ushort(c.blueF() * 0xFFFF);
        colBuf[3] = ushort(c.alphaF() * 0xFFFF);
    } else {
        ba = data->data(mimeType);
        if (ba.isEmpty()) {
            if (mimeType == "application/x-qt-image"_L1 && data->hasImage()) {
                QImage image = qvariant_cast<QImage>(data->imageData());
                QBuffer buf(&ba);
                buf.open(QBuffer::WriteOnly);
                // would there not be PNG ??
                image.save(&buf, "PNG");
            } else if (auto prefix = "image/"_L1; mimeType.startsWith(prefix) && data->hasImage()) {
                QImage image = qvariant_cast<QImage>(data->imageData());
                QBuffer buf(&ba);
                buf.open(QBuffer::WriteOnly);
                auto type = QStringView{mimeType}.sliced(prefix.size());
                image.save(&buf, type.toLatin1().toUpper().constData());
            }
        }
    }
    return ba;
}

QT_END_NAMESPACE

#include "moc_qinternalmimedata_p.cpp"
