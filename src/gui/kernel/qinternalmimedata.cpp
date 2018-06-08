/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "qinternalmimedata_p.h"

#include <QtCore/qbuffer.h>
#include <QtGui/qimage.h>
#include <QtGui/qimagereader.h>
#include <QtGui/qimagewriter.h>

QT_BEGIN_NAMESPACE

static QStringList imageMimeFormats(const QList<QByteArray> &imageFormats)
{
    QStringList formats;
    formats.reserve(imageFormats.size());
    for (const auto &format : imageFormats)
        formats.append(QLatin1String("image/") + QLatin1String(format.toLower()));

    //put png at the front because it is best
    int pngIndex = formats.indexOf(QLatin1String("image/png"));
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
    if (!foundFormat && mimeType == QLatin1String("application/x-qt-image")) {
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
    if (!realFormats.contains(QLatin1String("application/x-qt-image"))) {
        QStringList imageFormats = imageReadMimeFormats();
        for (int i = 0; i < imageFormats.size(); ++i) {
            if (realFormats.contains(imageFormats.at(i))) {
                realFormats += QLatin1String("application/x-qt-image");
                break;
            }
        }
    }
    return realFormats;
}

QVariant QInternalMimeData::retrieveData(const QString &mimeType, QVariant::Type type) const
{
    QVariant data = retrieveData_sys(mimeType, type);
    if (mimeType == QLatin1String("application/x-qt-image")) {
        if (data.isNull() || (data.type() == QVariant::ByteArray && data.toByteArray().isEmpty())) {
            // try to find an image
            QStringList imageFormats = imageReadMimeFormats();
            for (int i = 0; i < imageFormats.size(); ++i) {
                data = retrieveData_sys(imageFormats.at(i), type);
                if (data.isNull() || (data.type() == QVariant::ByteArray && data.toByteArray().isEmpty()))
                    continue;
                break;
            }
        }
        // we wanted some image type, but all we got was a byte array. Convert it to an image.
        if (data.type() == QVariant::ByteArray
            && (type == QVariant::Image || type == QVariant::Pixmap || type == QVariant::Bitmap))
            data = QImage::fromData(data.toByteArray());

    } else if (mimeType == QLatin1String("application/x-color") && data.type() == QVariant::ByteArray) {
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
    } else if (data.type() != type && data.type() == QVariant::ByteArray) {
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
    if (realFormats.contains(QLatin1String("application/x-qt-image"))) {
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
        if (mimeType == QLatin1String("application/x-qt-image")) {
            // check all supported image formats
            QStringList imageFormats = imageWriteMimeFormats();
            for (int i = 0; i < imageFormats.size(); ++i) {
                if ((foundFormat = data->hasFormat(imageFormats.at(i))))
                    break;
            }
        } else if (mimeType.startsWith(QLatin1String("image/"))) {
            return data->hasImage() && imageWriteMimeFormats().contains(mimeType);
        }
    }
    return foundFormat;
}

QByteArray QInternalMimeData::renderDataHelper(const QString &mimeType, const QMimeData *data)
{
    QByteArray ba;
    if (mimeType == QLatin1String("application/x-color")) {
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
            if (mimeType == QLatin1String("application/x-qt-image") && data->hasImage()) {
                QImage image = qvariant_cast<QImage>(data->imageData());
                QBuffer buf(&ba);
                buf.open(QBuffer::WriteOnly);
                // would there not be PNG ??
                image.save(&buf, "PNG");
            } else if (mimeType.startsWith(QLatin1String("image/")) && data->hasImage()) {
                QImage image = qvariant_cast<QImage>(data->imageData());
                QBuffer buf(&ba);
                buf.open(QBuffer::WriteOnly);
                image.save(&buf, mimeType.mid(mimeType.indexOf(QLatin1Char('/')) + 1).toLatin1().toUpper());
            }
        }
    }
    return ba;
}

QT_END_NAMESPACE
