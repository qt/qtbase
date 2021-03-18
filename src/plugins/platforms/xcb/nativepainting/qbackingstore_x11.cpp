/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qbackingstore_x11_p.h"
#include "qxcbwindow.h"
#include "qpixmap_x11_p.h"

#include <private/qhighdpiscaling_p.h>
#include <QPainter>

#if QT_CONFIG(xrender)
# include <X11/extensions/Xrender.h>
#endif

#define register        /* C++17 deprecated register */
#include <X11/Xlib.h>
#undef register

#ifndef None
#define None 0L
#endif

QT_BEGIN_NAMESPACE

QXcbNativeBackingStore::QXcbNativeBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , m_translucentBackground(false)
{
    if (QXcbWindow *w = static_cast<QXcbWindow *>(window->handle())) {
        m_translucentBackground = w->connection()->hasXRender() &&
            QImage::toPixelFormat(w->imageFormat()).alphaUsage() == QPixelFormat::UsesAlpha;
    }
}

QXcbNativeBackingStore::~QXcbNativeBackingStore()
{}

QPaintDevice *QXcbNativeBackingStore::paintDevice()
{
    return &m_pixmap;
}

void QXcbNativeBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    if (m_pixmap.isNull())
        return;

    QSize pixmapSize = m_pixmap.size();

    QRegion clipped = region;
    clipped &= QRect(QPoint(), QHighDpi::toNativePixels(window->size(), window));
    clipped &= QRect(0, 0, pixmapSize.width(), pixmapSize.height()).translated(-offset);

    QRect br = clipped.boundingRect();
    if (br.isNull())
        return;

    QXcbWindow *platformWindow = static_cast<QXcbWindow *>(window->handle());
    if (!platformWindow) {
        qWarning("QXcbBackingStore::flush: QWindow has no platform window (QTBUG-32681)");
        return;
    }

    Window wid = platformWindow->xcb_window();
    Pixmap pid = qt_x11PixmapHandle(m_pixmap);

    QVector<XRectangle> clipRects = qt_region_to_xrectangles(clipped);

#if QT_CONFIG(xrender)
    if (m_translucentBackground)
    {
        XWindowAttributes attrib;
        XGetWindowAttributes(display(), wid, &attrib);
        XRenderPictFormat *format = XRenderFindVisualFormat(display(), attrib.visual);

        Picture srcPic = qt_x11PictureHandle(m_pixmap);
        Picture dstPic = XRenderCreatePicture(display(), wid, format, 0, 0);

        XRenderSetPictureClipRectangles(display(), dstPic, 0, 0, clipRects.constData(), clipRects.size());

        XRenderComposite(display(), PictOpSrc, srcPic, None, dstPic,
                         br.x() + offset.x(), br.y() + offset.y(),
                         0, 0,
                         br.x(), br.y(),
                         br.width(), br.height());

        XRenderFreePicture(display(), dstPic);
    }
    else
#endif
    {
        GC gc = XCreateGC(display(), wid, 0, nullptr);

        if (clipRects.size() != 1)
            XSetClipRectangles(display(), gc, 0, 0, clipRects.data(), clipRects.size(), YXBanded);

        XCopyArea(display(), pid, wid, gc, br.x() + offset.x(), br.y() + offset.y(), br.width(), br.height(), br.x(), br.y());
        XFreeGC(display(), gc);
    }


    if (platformWindow->needsSync()) {
        platformWindow->updateSyncRequestCounter();
    } else {
        XFlush(display());
    }
}

QImage QXcbNativeBackingStore::toImage() const
{
    return m_pixmap.toImage();
}

void QXcbNativeBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    if (size == m_pixmap.size())
        return;

    QPixmap newPixmap(size);

#if QT_CONFIG(xrender)
    if (m_translucentBackground && newPixmap.depth() != 32)
        qt_x11Pixmap(newPixmap)->convertToARGB32();
#endif

    if (!m_pixmap.isNull()) {
        Pixmap from = qt_x11PixmapHandle(m_pixmap);
        Pixmap to = qt_x11PixmapHandle(newPixmap);
        QRect br = staticContents.boundingRect().intersected(QRect(QPoint(0, 0), size));

        if (!br.isEmpty()) {
            GC gc = XCreateGC(display(), to, 0, nullptr);
            XCopyArea(display(), from, to, gc, br.x(), br.y(), br.width(), br.height(), br.x(), br.y());
            XFreeGC(display(), gc);
        }
    }

    m_pixmap = newPixmap;
}

bool QXcbNativeBackingStore::scroll(const QRegion &area, int dx, int dy)
{
    if (m_pixmap.isNull())
        return false;

    QRect rect = area.boundingRect();
    Pixmap pix = qt_x11PixmapHandle(m_pixmap);

    GC gc = XCreateGC(display(), pix, 0, nullptr);
    XCopyArea(display(), pix, pix, gc,
              rect.x(), rect.y(), rect.width(), rect.height(),
              rect.x()+dx, rect.y()+dy);
    XFreeGC(display(), gc);
    return true;
}

void QXcbNativeBackingStore::beginPaint(const QRegion &region)
{
    QX11PlatformPixmap *x11pm = qt_x11Pixmap(m_pixmap);
    if (x11pm)
        x11pm->setIsBackingStore(true);

#if QT_CONFIG(xrender)
    if (m_translucentBackground) {
        const QVector<XRectangle> xrects = qt_region_to_xrectangles(region);
        const XRenderColor color = { 0, 0, 0, 0 };
        XRenderFillRectangles(display(), PictOpSrc,
                              qt_x11PictureHandle(m_pixmap), &color,
                              xrects.constData(), xrects.size());
    }
#else
    Q_UNUSED(region);
#endif
}

Display *QXcbNativeBackingStore::display() const
{
    return static_cast<Display *>(static_cast<QXcbWindow *>(window()->handle())->connection()->xlib_display());
}

QT_END_NAMESPACE
