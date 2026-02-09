// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandmimehelper_p.h"
#include <QImage>
#include <QColor>
#include <QUrl>
#include <QBuffer>
#include <QImageWriter>

QT_BEGIN_NAMESPACE

QByteArray QWaylandMimeHelper::getByteArray(QMimeData *mimeData, const QString &mimeType)
{
    QByteArray content;
    if (mimeType == QLatin1String("text/plain")) {
        content = mimeData->text().toUtf8();
    } else if (mimeData->hasImage()
               && (mimeType == QLatin1String("application/x-qt-image")
                   || mimeType.startsWith(QLatin1String("image/")))) {
        QImage image = qvariant_cast<QImage>(mimeData->imageData());
        if (!image.isNull()) {
            QBuffer buf;
            buf.open(QIODevice::ReadWrite);
            QByteArray fmt = "BMP";
            if (mimeType.startsWith(QLatin1String("image/"))) {
                QByteArray imgFmt = mimeType.mid(6).toLower().toLatin1();
                if (QImageWriter::supportedImageFormats().contains(imgFmt))
                    fmt = imgFmt;
            }
            QImageWriter wr(&buf, fmt);
            wr.write(image);
            content = buf.buffer();
        }
    } else if (mimeType == QLatin1String("application/x-color")) {
        content = qvariant_cast<QColor>(mimeData->colorData()).name().toLatin1();
    } else if (mimeType == QLatin1String("text/uri-list")) {
        QList<QUrl> urls = mimeData->urls();
        for (int i = 0; i < urls.size(); ++i) {
            content.append(urls.at(i).toEncoded());
            content.append("\r\n");
        }
    } else {
        content = mimeData->data(mimeType);
    }
    return content;
}

QT_END_NAMESPACE
