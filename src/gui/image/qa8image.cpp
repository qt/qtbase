// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qa8image_p.h"
#include <QtCore/qalloc.h>
#include <private/qimage_p.h>

QT_BEGIN_NAMESPACE

QA8ImageData::QA8ImageData(const QA8ImageData &other)
    : QSharedData(other)
    , width(other.width)
    , height(other.height)
    , nbytes(other.nbytes)
{
    if (nbytes && other.data)
        data = static_cast<uchar *>(malloc(nbytes));
    else
        data = nullptr;

    if (data) {
        memcpy(data, other.data, nbytes);
    } else {
        width = 0;
        height = 0;
        nbytes = 0;
    }
}

QA8ImageData::~QA8ImageData()
{
    QtPrivate::sizedFree(data, nbytes);
}

QA8ImageData *QA8ImageData::create(QSize size)
{
    QA8ImageData *data = new QA8ImageData;
    if (size.isEmpty())
        return data;

    // pixel data stored as a 1-byte alpha value, tightly packed
    const quint64 nbytes = quint64(size.width()) * quint64(size.height());
    if (nbytes > INT_MAX) { // int used for bytes internally
        qWarning("QA8Image: Rejecting image as size exceeds the upper limit of %i bytes",
                 INT_MAX);
        return data;
    }

    data->data = static_cast<uchar *>(malloc(nbytes));
    if (data->data) {
        data->width = size.width();
        data->height = size.height();
        data->nbytes = int(nbytes);
    }

    return data;
}

QA8Image::QA8Image()
    : d(new QA8ImageData)
{
}

QA8Image::QA8Image(int width, int height)
    : QA8Image(QSize(width, height))
{
}

QA8Image::QA8Image(QSize size)
    : d(QA8ImageData::create(size))
{
}

QA8Image::QA8Image(QA8ImageData *data)
    : d(data)
{
}

bool QA8Image::isNull() const
{
    return !d->data;
}

int QA8Image::width() const
{
    return d->width;
}

int QA8Image::height() const
{
    return d->height;
}

QSize QA8Image::size() const
{
    return QSize(d->width, d->height);
}

qsizetype QA8Image::sizeInBytes() const
{
    return d->nbytes;
}

void QA8Image::fill(uchar value)
{
    if (isNull())
        return;

    memset(d->data, value, d->nbytes);
}

QA8Image QA8Image::copy(const QRect &r) const
{
    if (isNull())
        return QA8Image();

    if (r.isNull())
        return QA8Image(new QA8ImageData(*d));

    int x = r.x();
    int y = r.y();
    int w = r.width();
    int h = r.height();

    int dx = 0;
    int dy = 0;
    if (w <= 0 || h <= 0)
        return QA8Image();

    QA8Image image(w, h);
    if (image.isNull())
        return image;

    if (x < 0 || y < 0 || x + w > d->width || y + h > d->height) {
        image.fill(0);
        if (x < 0) {
            dx = -x;
            x = 0;
        }
        if (y < 0) {
            dy = -y;
            y = 0;
        }
    }

    int pixels_to_copy = qMax(w - dx, 0);
    if (x > d->width)
        pixels_to_copy = 0;
    else if (pixels_to_copy > d->width - x)
        pixels_to_copy = d->width - x;
    int lines_to_copy = qMax(h - dy, 0);
    if (y > d->height)
        lines_to_copy = 0;
    else if (lines_to_copy > d->height - y)
        lines_to_copy = d->height - y;

    const uchar *src = d->data + x + y * d->width;
    uchar *dest = image.d->data + dx + dy * image.d->width;
    for (int i = 0; i < lines_to_copy; ++i) {
        memcpy(dest, src, pixels_to_copy);
        src += d->width;
        dest += image.d->width;
    }

    return image;
}

uchar *QA8Image::bits()
{
    return d->data;
}

const uchar *QA8Image::bits() const
{
    return d->data;
}

const uchar *QA8Image::constBits() const
{
    return d->data;
}

uchar *QA8Image::scanLine(int i)
{
    if (isNull())
        return nullptr;

    Q_ASSERT(i >= 0 && i < d->height);
    return d->data + i * d->width;
}

const uchar *QA8Image::scanLine(int i) const
{
    if (isNull())
        return nullptr;

    Q_ASSERT(i >= 0 && i < d->height);
    return d->data + i * d->width;
}

const uchar *QA8Image::constScanLine(int i) const
{
    if (isNull())
        return nullptr;

    Q_ASSERT(i >= 0 && i < d->height);
    return d->data + i * d->width;
}

QImage QA8Image::toImage(QImage::Format format) const
{
    if (isNull())
        return QImage();

    QImage image(d->width, d->height, qt_depthForFormat(format) == 8 ?
                                      format : QImage::Format_ARGB32_Premultiplied);
    if (image.isNull())
        return image;

    if (image.depth() == 8) {
        for (int y = 0; y < d->height; ++y)
            memcpy(image.scanLine(y), constScanLine(y), d->width);
    } else {
        for (int y = 0; y < d->height; ++y) {
            for (int x = 0; x < d->width; ++x) {
                uint alpha = *(d->data + x + y * d->width);
                image.setPixel(x, y, alpha << 24);
            }
        }

        if (image.format() != format)
            image = std::move(image).convertToFormat(format);
    }

    return image;
}

QT_END_NAMESPACE
