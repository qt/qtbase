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

#ifndef QBACKINGSTORE_COCOA_H
#define QBACKINGSTORE_COCOA_H

#include <QtGraphicsSupport/private/qrasterbackingstore_p.h>

#include <private/qcore_mac_p.h>

#include <QScopedPointer>
#include "qiosurfacegraphicsbuffer.h"

#include <unordered_map>

QT_BEGIN_NAMESPACE

class QCocoaBackingStore : public QRasterBackingStore
{
protected:
    QCocoaBackingStore(QWindow *window);
    QCFType<CGColorSpaceRef> colorSpace() const;
};

class QNSWindowBackingStore : public QCocoaBackingStore
{
public:
    QNSWindowBackingStore(QWindow *window);
    ~QNSWindowBackingStore();

    void resize(const QSize &size, const QRegion &staticContents) override;
    void flush(QWindow *, const QRegion &, const QPoint &) override;

private:
    bool windowHasUnifiedToolbar() const;
    QImage::Format format() const override;
    void redrawRoundedBottomCorners(CGRect) const;
};

class QCALayerBackingStore : public QObject, public QCocoaBackingStore
{
    Q_OBJECT
public:
    QCALayerBackingStore(QWindow *window);
    ~QCALayerBackingStore();

    void resize(const QSize &size, const QRegion &staticContents) override;

    void beginPaint(const QRegion &region) override;
    QPaintDevice *paintDevice() override;
    void endPaint() override;

    void flush(QWindow *, const QRegion &, const QPoint &) override;
#ifndef QT_NO_OPENGL
    void composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
        QPlatformTextureList *textures, bool translucentBackground) override;
#endif

    QImage toImage() const override;
    QPlatformGraphicsBuffer *graphicsBuffer() const override;

private:
    void observeBackingPropertiesChanges();
    bool eventFilter(QObject *watched, QEvent *event) override;

    QSize m_requestedSize;
    QRegion m_paintedRegion;

    class GraphicsBuffer : public QIOSurfaceGraphicsBuffer
    {
    public:
        GraphicsBuffer(const QSize &size, qreal devicePixelRatio,
                const QPixelFormat &format, QCFType<CGColorSpaceRef> colorSpace);

        QRegion dirtyRegion; // In unscaled coordinates
        QImage *asImage();
        qreal devicePixelRatio() const { return m_devicePixelRatio; }

    private:
        qreal m_devicePixelRatio;
        QImage m_image;
    };

    void ensureBackBuffer();
    bool recreateBackBufferIfNeeded();
    bool prepareForFlush();

    void backingPropertiesChanged();
    QMacNotificationObserver m_backingPropertiesObserver;

    std::list<std::unique_ptr<GraphicsBuffer>> m_buffers;

    void flushSubWindow(QWindow *window);
    std::unordered_map<QWindow*, std::unique_ptr<QCALayerBackingStore>> m_subWindowBackingstores;
    void windowDestroyed(QObject *object);
    bool m_clearSurfaceOnPaint = true;
};

QT_END_NAMESPACE

#endif
