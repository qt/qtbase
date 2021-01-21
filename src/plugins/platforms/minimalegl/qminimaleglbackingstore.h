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

#ifndef QMINIMALEGLBACKINGSTORE_H
#define QMINIMALEGLBACKINGSTORE_H

#include <qpa/qplatformbackingstore.h>

QT_BEGIN_NAMESPACE

class QOpenGLContext;
class QOpenGLPaintDevice;

class QMinimalEglBackingStore : public QPlatformBackingStore
{
public:
    QMinimalEglBackingStore(QWindow *window);
    ~QMinimalEglBackingStore();

    QPaintDevice *paintDevice() override;

    void beginPaint(const QRegion &) override;
    void endPaint() override;

    void flush(QWindow *window, const QRegion &region, const QPoint &offset) override;
    void resize(const QSize &size, const QRegion &staticContents) override;

private:
    QOpenGLContext *m_context;
    QOpenGLPaintDevice *m_device;
};

QT_END_NAMESPACE

#endif // QMINIMALEGLBACKINGSTORE_H
