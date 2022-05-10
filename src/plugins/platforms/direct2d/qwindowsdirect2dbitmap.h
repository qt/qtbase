// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSDIRECT2DBITMAP_H
#define QWINDOWSDIRECT2DBITMAP_H

#include <QtCore/qnamespace.h>
#include <QtCore/qrect.h>
#include <QtCore/qscopedpointer.h>

struct ID2D1DeviceContext;
struct ID2D1Bitmap1;

QT_BEGIN_NAMESPACE

class QWindowsDirect2DDeviceContext;
class QWindowsDirect2DBitmapPrivate;

class QImage;
class QSize;
class QColor;

class QWindowsDirect2DBitmap
{
    Q_DECLARE_PRIVATE(QWindowsDirect2DBitmap)
    Q_DISABLE_COPY_MOVE(QWindowsDirect2DBitmap)
public:
    QWindowsDirect2DBitmap();
    QWindowsDirect2DBitmap(ID2D1Bitmap1 *bitmap, ID2D1DeviceContext *dc);
    ~QWindowsDirect2DBitmap();

    bool resize(int width, int height);
    bool fromImage(const QImage &image, Qt::ImageConversionFlags flags);

    ID2D1Bitmap1* bitmap() const;
    QWindowsDirect2DDeviceContext* deviceContext() const;

    void fill(const QColor &color);
    QImage toImage(const QRect &rect = QRect());

    QSize size() const;

private:
    QScopedPointer<QWindowsDirect2DBitmapPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QWINDOWSDIRECT2DBITMAP_H
