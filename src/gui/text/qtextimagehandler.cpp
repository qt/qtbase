// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qtextimagehandler_p.h"

#include <qbuffer.h>
#include <qguiapplication.h>
#include <qtextformat.h>
#include <qpainter.h>
#include <qdebug.h>
#include <qicon.h>
#include <qimagereader.h>
#include <qpixmapcache.h>
#include <qthread.h>
#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>
#include <private/qhexstring_p.h>
#include <private/qtextengine_p.h>
#include <limits>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static inline QUrl findAtNxFileOrResource(const QString &baseFileName,
                                          qreal targetDevicePixelRatio,
                                          qreal *sourceDevicePixelRatio,
                                          QString *name)
{
    // qt_findAtNxFile expects a file name that can be tested with QFile::exists.
    // so if the format.name() is a file:/ or qrc:/ URL, then we need to strip away the schema.
    QString localFile;
    const QUrl url(baseFileName);
    bool hasFileScheme = false;
    bool isResource = false;
    if (url.isLocalFile()) {
        localFile = url.toLocalFile();
        hasFileScheme = true;
    } else if (baseFileName.startsWith("qrc:/"_L1)) {
        // QFile::exists() can only handle ":/file.txt"
        localFile = baseFileName.sliced(3);
        isResource = true;
    } else {
        localFile = baseFileName;
        isResource = baseFileName.startsWith(":/"_L1);
    }
    *name = qt_findAtNxFile(localFile, targetDevicePixelRatio, sourceDevicePixelRatio);

    if (hasFileScheme)
        return QUrl::fromLocalFile(*name);
    if (isResource)
        return QUrl("qrc"_L1 + *name);
    return QUrl(*name);
}

static QImage getImage(QTextDocument *doc, const QTextImageFormat &format,
                       const qreal devicePixelRatio = 1.0, const QSizeF &size = {})
{
    qreal sourcePixelRatio = 1.0;
    QString name;
    const QUrl url = findAtNxFileOrResource(format.name(), devicePixelRatio, &sourcePixelRatio, &name);
    const QVariant data = doc->resource(QTextDocument::ImageResource, url);

    // directly provided as QPixmap/QImage or already cached
    if (data.userType() == QMetaType::QPixmap || data.userType() == QMetaType::QImage)
        return data.value<QImage>();

    // some image formats are scalable (e.g. svg) so we should not store a single
    // QImage as QTextDocument::ImageResource to avoid scaling artefacts later on.
    // But otoh we don't want to load them on every usage. Therefore cache the result
    // in QPixmapCache
    const bool canUsePixmapCache = (QThread::isMainThread()
                                    || QGuiApplicationPrivate::platformIntegration()->hasCapability(
                                            QPlatformIntegration::ThreadedPixmaps));
    const auto buildCacheKey = [](const QSizeF &size, qreal dpr) -> QString {
        return QLatin1String("qt_textimagehandler_") % HexString<int>(size.width())
                % HexString<int>(size.height())
                % HexString<qint16>(qRound(dpr * 1000));
    };
    const QString cacheKey = canUsePixmapCache
            ? buildCacheKey(size * devicePixelRatio, sourcePixelRatio)
            : QString();

    if (canUsePixmapCache) {
        QPixmap pm;
        if (QPixmapCache::find(cacheKey, &pm))
            return pm.toImage();
    }

    const auto readImage = [&](auto &&content) {
        QImageReader imgReader(content);
        if (imgReader.canRead()) {
            const bool supportsScaledSize = imgReader.supportsOption(QImageIOHandler::ScaledSize);
            if (size.isValid() && supportsScaledSize)
                imgReader.setScaledSize((size * devicePixelRatio).toSize());
            QImage result = imgReader.read();
            result.setDevicePixelRatio(sourcePixelRatio);
            if (!supportsScaledSize)
                doc->addResource(QTextDocument::ImageResource, url, result);
            else if (canUsePixmapCache)
                QPixmapCache::insert(cacheKey, QPixmap::fromImage(result));
            return result;
        }
        return QImage();
    };

    QImage result;
    if (data.metaType() == QMetaType::fromType<QByteArray>()) {
        QByteArray ba(data.toByteArray());
        QBuffer buf(&ba);
        result = readImage(&buf);
    }
    if (result.isNull())
        result = readImage(name);

    if (result.isNull())
        result = QImage(":/qt-project.org/styles/commonstyle/images/file-16.png"_L1);
    return result;
}

static QSize getSize(QTextDocument *doc, const QTextImageFormat &format)
{
    const bool hasWidth = format.hasProperty(QTextFormat::ImageWidth);
    int width = qRound(format.width());
    const bool hasHeight = format.hasProperty(QTextFormat::ImageHeight);
    const int height = qRound(format.height());

    const bool hasMaxWidth = format.hasProperty(QTextFormat::ImageMaxWidth);
    const auto maxWidth = format.maximumWidth();

    int effectiveMaxWidth = std::numeric_limits<int>::max();
    if (hasMaxWidth) {
        if (maxWidth.type() == QTextLength::PercentageLength)
            effectiveMaxWidth = (doc->pageSize().width() - 2 * doc->documentMargin()) * maxWidth.value(100) / 100;
        else
            effectiveMaxWidth = maxWidth.rawValue();

        width = qMin(effectiveMaxWidth, width);
    }

    QImage source;
    QSize size(width, height);
    if (!hasWidth || !hasHeight) {
        source = getImage(doc, format);
        QSizeF sourceSize = source.deviceIndependentSize();

        if (sourceSize.width() > effectiveMaxWidth) {
            // image is bigger than effectiveMaxWidth, scale it down
            sourceSize.setHeight(effectiveMaxWidth * (sourceSize.height() / qreal(sourceSize.width())));
            sourceSize.setWidth(effectiveMaxWidth);
        }

        if (!hasWidth) {
            if (!hasHeight)
                size.setWidth(sourceSize.width());
            else
                size.setWidth(qMin(effectiveMaxWidth, qRound(height * (sourceSize.width() / qreal(sourceSize.height())))));
        }
        if (!hasHeight) {
            if (!hasWidth)
                size.setHeight(sourceSize.height());
            else
                size.setHeight(qRound(width * (sourceSize.height() / qreal(sourceSize.width()))));
        }
    }

    const QPaintDevice *pdev = doc->documentLayout()->paintDevice();
    if (pdev)
        size *= qreal(pdev->logicalDpiY()) / qreal(qt_defaultDpi());

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

    return getSize(doc, imageFormat);
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

    const QImage image = getImage(doc, imageFormat, p->device()->devicePixelRatio(), rect.size());
    p->drawImage(rect, image, image.rect());
}

QT_END_NAMESPACE

#include "moc_qtextimagehandler_p.cpp"
