// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qtextimagehandler_p.h"

#include <qguiapplication.h>
#include <qtextformat.h>
#include <qpainter.h>
#include <qdebug.h>
#include <qfile.h>
#include <private/qtextengine_p.h>
#include <qpalette.h>
#include <qthread.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

extern QString qt_findAtNxFile(const QString &baseFileName, qreal targetDevicePixelRatio,
                               qreal *sourceDevicePixelRatio);

static inline QUrl fromLocalfileOrResources(QString path)
{
    if (path.startsWith(":/"_L1)) // auto-detect resources and convert them to url
        path.prepend("qrc"_L1);
    return QUrl(path);
}

static QPixmap getPixmap(QTextDocument *doc, const QTextImageFormat &format, const qreal devicePixelRatio = 1.0)
{
    qreal sourcePixelRatio = 1.0;
    const QString name = qt_findAtNxFile(format.name(), devicePixelRatio, &sourcePixelRatio);
    const QUrl url = fromLocalfileOrResources(name);

    QPixmap pm;
    const QVariant data = doc->resource(QTextDocument::ImageResource, url);
    if (data.userType() == QMetaType::QPixmap || data.userType() == QMetaType::QImage) {
        pm = qvariant_cast<QPixmap>(data);
    } else if (data.userType() == QMetaType::QByteArray) {
        pm.loadFromData(data.toByteArray());
    }

    if (pm.isNull()) {
        QImage img;
        if (name.isEmpty() || !img.load(name))
            return QPixmap(":/qt-project.org/styles/commonstyle/images/file-16.png"_L1);

        pm = QPixmap::fromImage(img);
        doc->addResource(QTextDocument::ImageResource, url, pm);
    }

    if (name.contains("@2x"_L1))
        pm.setDevicePixelRatio(sourcePixelRatio);

    return pm;
}

static QSize getPixmapSize(QTextDocument *doc, const QTextImageFormat &format)
{
    QPixmap pm;

    const bool hasWidth = format.hasProperty(QTextFormat::ImageWidth);
    const int width = qRound(format.width());
    const bool hasHeight = format.hasProperty(QTextFormat::ImageHeight);
    const int height = qRound(format.height());

    QSize size(width, height);
    if (!hasWidth || !hasHeight) {
        pm = getPixmap(doc, format);
        const QSizeF pmSize = pm.deviceIndependentSize();

        if (!hasWidth) {
            if (!hasHeight)
                size.setWidth(pmSize.width());
            else
                size.setWidth(qRound(height * (pmSize.width() / (qreal) pmSize.height())));
        }
        if (!hasHeight) {
            if (!hasWidth)
                size.setHeight(pmSize.height());
            else
                size.setHeight(qRound(width * (pmSize.height() / (qreal) pmSize.width())));
        }
    }

    qreal scale = 1.0;
    QPaintDevice *pdev = doc->documentLayout()->paintDevice();
    if (pdev) {
        if (pm.isNull())
            pm = getPixmap(doc, format);
        if (!pm.isNull())
            scale = qreal(pdev->logicalDpiY()) / qreal(qt_defaultDpi());
    }
    size *= scale;

    return size;
}

static QImage getImage(QTextDocument *doc, const QTextImageFormat &format, const qreal devicePixelRatio = 1.0)
{
    qreal sourcePixelRatio = 1.0;
    const QString name = qt_findAtNxFile(format.name(), devicePixelRatio, &sourcePixelRatio);
    const QUrl url = fromLocalfileOrResources(name);

    QImage image;
    const QVariant data = doc->resource(QTextDocument::ImageResource, url);
    if (data.userType() == QMetaType::QImage) {
        image = qvariant_cast<QImage>(data);
    } else if (data.userType() == QMetaType::QByteArray) {
        image.loadFromData(data.toByteArray());
    }

    if (image.isNull()) {
        if (name.isEmpty() || !image.load(name))
            return QImage(":/qt-project.org/styles/commonstyle/images/file-16.png"_L1);

        doc->addResource(QTextDocument::ImageResource, url, image);
    }

    if (sourcePixelRatio != 1.0)
        image.setDevicePixelRatio(sourcePixelRatio);

    return image;
}

static QSize getImageSize(QTextDocument *doc, const QTextImageFormat &format)
{
    QImage image;

    const bool hasWidth = format.hasProperty(QTextFormat::ImageWidth);
    const int width = qRound(format.width());
    const bool hasHeight = format.hasProperty(QTextFormat::ImageHeight);
    const int height = qRound(format.height());

    QSize size(width, height);
    if (!hasWidth || !hasHeight) {
        image = getImage(doc, format);
        QSizeF imageSize = image.deviceIndependentSize();
        if (!hasWidth)
            size.setWidth(imageSize.width());
        if (!hasHeight)
            size.setHeight(imageSize.height());
    }

    qreal scale = 1.0;
    QPaintDevice *pdev = doc->documentLayout()->paintDevice();
    if (pdev) {
        if (image.isNull())
            image = getImage(doc, format);
        if (!image.isNull())
            scale = qreal(pdev->logicalDpiY()) / qreal(qt_defaultDpi());
    }
    size *= scale;

    return size;
}

QTextImageHandler::QTextImageHandler(QObject *parent)
    : QObject(parent)
{
}

QSizeF QTextImageHandler::intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(posInDocument);
    const QTextImageFormat imageFormat = format.toImageFormat();

    if (QCoreApplication::instance()->thread() != QThread::currentThread())
        return getImageSize(doc, imageFormat);
    return getPixmapSize(doc, imageFormat);
}

QImage QTextImageHandler::image(QTextDocument *doc, const QTextImageFormat &imageFormat)
{
    Q_ASSERT(doc != nullptr);

    return getImage(doc, imageFormat);
}

void QTextImageHandler::drawObject(QPainter *p, const QRectF &rect, QTextDocument *doc, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(posInDocument);
        const QTextImageFormat imageFormat = format.toImageFormat();

    if (QCoreApplication::instance()->thread() != QThread::currentThread()) {
        const QImage image = getImage(doc, imageFormat, p->device()->devicePixelRatio());
        p->drawImage(rect, image, image.rect());
    } else {
        const QPixmap pixmap = getPixmap(doc, imageFormat, p->device()->devicePixelRatio());
        p->drawPixmap(rect, pixmap, pixmap.rect());
    }
}

QT_END_NAMESPACE

#include "moc_qtextimagehandler_p.cpp"
