// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qtextimagehandler_p.h"

#include <qguiapplication.h>
#include <qtextformat.h>
#include <qpainter.h>
#include <qdebug.h>
#include <qfile.h>
#include <qicon.h>
#include <private/qtextengine_p.h>
#include <qpalette.h>
#include <qthread.h>
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

template<typename T>
static T getAs(QTextDocument *doc, const QTextImageFormat &format, const qreal devicePixelRatio = 1.0)
{
    qreal sourcePixelRatio = 1.0;
    QString name;
    const QUrl url = findAtNxFileOrResource(format.name(), devicePixelRatio, &sourcePixelRatio, &name);
    const QVariant data = doc->resource(QTextDocument::ImageResource, url);

    T result;
    if (data.userType() == QMetaType::QPixmap || data.userType() == QMetaType::QImage)
        result = data.value<T>();
    else if (data.metaType() == QMetaType::fromType<QByteArray>())
        result.loadFromData(data.toByteArray());

    if (result.isNull()) {
        if (name.isEmpty() || !result.load(name))
            return T(":/qt-project.org/styles/commonstyle/images/file-16.png"_L1);
        doc->addResource(QTextDocument::ImageResource, url, result);
    }

    if (sourcePixelRatio != 1.0)
        result.setDevicePixelRatio(sourcePixelRatio);
    return result;
}

template<typename T>
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

    T source;
    QSize size(width, height);
    if (!hasWidth || !hasHeight) {
        source = getAs<T>(doc, format);
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

    qreal scale = 1.0;
    QPaintDevice *pdev = doc->documentLayout()->paintDevice();
    if (pdev) {
        if (source.isNull())
            source = getAs<T>(doc, format);
        if (!source.isNull())
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
        return getSize<QImage>(doc, imageFormat);
    return getSize<QPixmap>(doc, imageFormat);
}

QImage QTextImageHandler::image(QTextDocument *doc, const QTextImageFormat &imageFormat)
{
    Q_ASSERT(doc != nullptr);

    return getAs<QImage>(doc, imageFormat);
}

void QTextImageHandler::drawObject(QPainter *p, const QRectF &rect, QTextDocument *doc, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(posInDocument);
        const QTextImageFormat imageFormat = format.toImageFormat();

    if (QCoreApplication::instance()->thread() != QThread::currentThread()) {
        const QImage image = getAs<QImage>(doc, imageFormat, p->device()->devicePixelRatio());
        p->drawImage(rect, image, image.rect());
    } else {
        const QPixmap pixmap = getAs<QPixmap>(doc, imageFormat, p->device()->devicePixelRatio());
        p->drawPixmap(rect, pixmap, pixmap.rect());
    }
}

QT_END_NAMESPACE

#include "moc_qtextimagehandler_p.cpp"
