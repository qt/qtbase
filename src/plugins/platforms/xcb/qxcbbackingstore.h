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
#ifndef QT_NO_OPENGL
    void composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
                         QPlatformTextureList *textures,
                         bool translucentBackground) override;
#endif
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
