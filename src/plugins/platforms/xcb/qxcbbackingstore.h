// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QXCBBACKINGSTORE_H
#define QXCBBACKINGSTORE_H

#include <qpa/qplatformbackingstore.h>
#include <QtCore/QStack>

#include <xcb/xcb.h>

#include "qxcbobject.h"

QT_BEGIN_NAMESPACE

class QXcbBackingStoreImage;

class QXcbBackingStore : public QXcbObject, public QPlatformBackingStore
{
public:
    QXcbBackingStore(QWindow *window);
    ~QXcbBackingStore();

    QPaintDevice *paintDevice() override;
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) override;
    FlushResult rhiFlush(QWindow *window,
                         qreal sourceDevicePixelRatio,
                         const QRegion &region,
                         const QPoint &offset,
                         QPlatformTextureList *textures,
                         bool translucentBackground) override;
    QImage toImage() const override;

    QPlatformGraphicsBuffer *graphicsBuffer() const override;

    void resize(const QSize &size, const QRegion &staticContents) override;
    bool scroll(const QRegion &area, int dx, int dy) override;

    void beginPaint(const QRegion &) override;
    void endPaint() override;

    static bool createSystemVShmSegment(xcb_connection_t *c, size_t segmentSize = 1,
                                        void *shmInfo = nullptr);

protected:
    virtual void render(xcb_window_t window, const QRegion &region, const QPoint &offset);
    virtual void recreateImage(QXcbWindow *win, const QSize &size);

    QXcbBackingStoreImage *m_image = nullptr;
    QStack<QRegion> m_paintRegions;
    QImage m_rgbImage;
};

class QXcbSystemTrayBackingStore : public QXcbBackingStore
{
public:
    QXcbSystemTrayBackingStore(QWindow *window);
    ~QXcbSystemTrayBackingStore();

    void beginPaint(const QRegion &) override;

protected:
    void render(xcb_window_t window, const QRegion &region, const QPoint &offset) override;
    void recreateImage(QXcbWindow *win, const QSize &size) override;

private:
    void initXRenderMode();

    xcb_pixmap_t m_xrenderPixmap = XCB_NONE;
    xcb_render_picture_t m_xrenderPicture = XCB_NONE;
    xcb_render_pictformat_t m_xrenderPictFormat  = XCB_NONE;
    xcb_render_picture_t m_windowPicture = XCB_NONE;

    bool m_usingXRenderMode = false;
    bool m_useGrabbedBackgound = false;
    QPixmap m_grabbedBackground;
};

QT_END_NAMESPACE

#endif
