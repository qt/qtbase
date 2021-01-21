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

#ifndef QWINDOWSDIRECT2DBACKINGSTORE_H
#define QWINDOWSDIRECT2DBACKINGSTORE_H

#include <QtGui/qpa/qplatformbackingstore.h>

QT_BEGIN_NAMESPACE

class QWindowsDirect2DWindow;

class QWindowsDirect2DBackingStore : public QPlatformBackingStore
{
    Q_DISABLE_COPY_MOVE(QWindowsDirect2DBackingStore)

public:
    QWindowsDirect2DBackingStore(QWindow *window);
    ~QWindowsDirect2DBackingStore();

    void beginPaint(const QRegion &) override;
    void endPaint() override;

    QPaintDevice *paintDevice() override;
    void flush(QWindow *targetWindow, const QRegion &region, const QPoint &offset) override;
    void resize(const QSize &size, const QRegion &staticContents) override;

    QImage toImage() const override;
};

QT_END_NAMESPACE

#endif // QWINDOWSDIRECT2DBACKINGSTORE_H
