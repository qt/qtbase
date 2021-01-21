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

#ifndef QWINDOWSDIRECT2DPLATFORMPIXMAP_H
#define QWINDOWSDIRECT2DPLATFORMPIXMAP_H

#include "qwindowsdirect2dpaintengine.h"
#include <QtGui/qpa/qplatformpixmap.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE

class QWindowsDirect2DPlatformPixmapPrivate;
class QWindowsDirect2DBitmap;

class QWindowsDirect2DPlatformPixmap : public QPlatformPixmap
{
    Q_DECLARE_PRIVATE(QWindowsDirect2DPlatformPixmap)
public:
    QWindowsDirect2DPlatformPixmap(PixelType pixelType);

    // We do NOT take ownership of the bitmap through this constructor!
    QWindowsDirect2DPlatformPixmap(PixelType pixelType, QWindowsDirect2DPaintEngine::Flags flags, QWindowsDirect2DBitmap *bitmap);
    ~QWindowsDirect2DPlatformPixmap();

    void resize(int width, int height) override;
    void fromImage(const QImage &image, Qt::ImageConversionFlags flags) override;

    int metric(QPaintDevice::PaintDeviceMetric metric) const override;
    void fill(const QColor &color) override;

    bool hasAlphaChannel() const override;

    QImage toImage() const override;
    QImage toImage(const QRect &rect) const override;

    QPaintEngine* paintEngine() const override;

    qreal devicePixelRatio() const override;
    void setDevicePixelRatio(qreal scaleFactor) override;

    QWindowsDirect2DBitmap *bitmap() const;

private:
    QScopedPointer<QWindowsDirect2DPlatformPixmapPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QWINDOWSDIRECT2DPLATFORMPIXMAP_H
