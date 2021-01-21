/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
