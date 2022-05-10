// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
