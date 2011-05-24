/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QTime>
#include <QDebug>

#include <private/qt_x11_p.h>
#include <private/qimagepixmapcleanuphooks_p.h>

#include "qwindowsurface_x11gl_p.h"
#include "qpixmapdata_x11gl_p.h"

QT_BEGIN_NAMESPACE

QX11GLWindowSurface::QX11GLWindowSurface(QWidget* window)
    : QWindowSurface(window), m_windowGC(0), m_pixmapGC(0), m_window(window)
{
}

QX11GLWindowSurface::~QX11GLWindowSurface()
{
    if (m_windowGC)
        XFree(m_windowGC);
    if (m_pixmapGC)
        XFree(m_pixmapGC);
}

QPaintDevice *QX11GLWindowSurface::paintDevice()
{
    return &m_backBuffer;
}

extern void *qt_getClipRects(const QRegion &r, int &num); // in qpaintengine_x11.cpp

void QX11GLWindowSurface::flush(QWidget *widget, const QRegion &widgetRegion, const QPoint &offset)
{
    // We don't need to know the widget which initiated the flush. Instead we just use the offset
    // to translate the widgetRegion:
    Q_UNUSED(widget);

    if (m_backBuffer.isNull()) {
        qDebug("QX11GLWindowSurface::flush() - backBuffer is null, not flushing anything");
        return;
    }

    Q_ASSERT(window()->size() != m_backBuffer.size());

    // Wait for all GL rendering to the back buffer pixmap to complete before trying to
    // copy it to the window. We do this by making sure the pixmap's context is current
    // and then call eglWaitClient. The EGL 1.4 spec says eglWaitClient doesn't have to
    // block, just that "All rendering calls...are guaranteed to be executed before native
    // rendering calls". This makes it potentially less expensive than glFinish.
    QGLContext* ctx = static_cast<QX11GLPixmapData*>(m_backBuffer.data_ptr().data())->context();
    if (QGLContext::currentContext() != ctx && ctx && ctx->isValid())
        ctx->makeCurrent();
    eglWaitClient();

    if (m_windowGC == 0) {
        XGCValues attribs;
        attribs.graphics_exposures = False;
        m_windowGC = XCreateGC(X11->display, m_window->handle(), GCGraphicsExposures, &attribs);
    }

    int rectCount;
    XRectangle *rects = (XRectangle *)qt_getClipRects(widgetRegion, rectCount);
    if (rectCount <= 0)
        return;

    XSetClipRectangles(X11->display, m_windowGC, 0, 0, rects, rectCount, YXBanded);

    QRect dirtyRect = widgetRegion.boundingRect().translated(-offset);
    XCopyArea(X11->display, m_backBuffer.handle(), m_window->handle(), m_windowGC,
              dirtyRect.x(), dirtyRect.y(), dirtyRect.width(), dirtyRect.height(),
              dirtyRect.x(), dirtyRect.y());

    // Make sure the blit of the update from the back buffer to the window completes
    // before allowing rendering to start again to the back buffer. Otherwise the GPU
    // might start rendering to the back buffer again while the blit takes place.
    eglWaitNative(EGL_CORE_NATIVE_ENGINE);
}

void QX11GLWindowSurface::setGeometry(const QRect &rect)
{
    if (rect.width() > m_backBuffer.size().width() || rect.height() > m_backBuffer.size().height()) {
        QX11GLPixmapData *pd = new QX11GLPixmapData;
        QSize newSize = rect.size();
        pd->resize(newSize.width(), newSize.height());
        m_backBuffer = QPixmap(pd);
        if (window()->testAttribute(Qt::WA_TranslucentBackground))
            m_backBuffer.fill(Qt::transparent);
        if (m_pixmapGC) {
            XFreeGC(X11->display, m_pixmapGC);
            m_pixmapGC = 0;
        }
    }

    QWindowSurface::setGeometry(rect);
}

bool QX11GLWindowSurface::scroll(const QRegion &area, int dx, int dy)
{
    if (m_backBuffer.isNull())
        return false;

    Q_ASSERT(m_backBuffer.data_ptr()->classId() == QPixmapData::X11Class);

    // Make sure all GL rendering is complete before starting the scroll operation:
    QGLContext* ctx = static_cast<QX11GLPixmapData*>(m_backBuffer.data_ptr().data())->context();
    if (QGLContext::currentContext() != ctx && ctx && ctx->isValid())
        ctx->makeCurrent();
    eglWaitClient();

    if (!m_pixmapGC)
        m_pixmapGC = XCreateGC(X11->display, m_backBuffer.handle(), 0, 0);

    foreach (const QRect& rect, area.rects()) {
        XCopyArea(X11->display, m_backBuffer.handle(), m_backBuffer.handle(), m_pixmapGC,
                  rect.x(), rect.y(), rect.width(), rect.height(),
                  rect.x()+dx, rect.y()+dy);
    }

    // Make sure the scroll operation is complete before allowing GL rendering to resume
    eglWaitNative(EGL_CORE_NATIVE_ENGINE);

    return true;
}


QPixmap QX11GLWindowSurface::grabWidget(const QWidget *widget, const QRect& rect) const
{
    if (!widget || m_backBuffer.isNull())
        return QPixmap();

    QRect srcRect;

    // make sure the rect is inside the widget & clip to widget's rect
    if (!rect.isEmpty())
        srcRect = rect & widget->rect();
    else
        srcRect = widget->rect();

    if (srcRect.isEmpty())
        return QPixmap();

    // If it's a child widget we have to translate the coordinates
    if (widget != window())
        srcRect.translate(widget->mapTo(window(), QPoint(0, 0)));

    QPixmap::x11SetDefaultScreen(widget->x11Info().screen());

    QX11PixmapData *pmd = new QX11PixmapData(QPixmapData::PixmapType);
    pmd->resize(srcRect.width(), srcRect.height());
    QPixmap px(pmd);

    GC tmpGc = XCreateGC(X11->display, m_backBuffer.handle(), 0, 0);

    // Make sure all GL rendering is complete before copying the window
    QGLContext* ctx = static_cast<QX11GLPixmapData*>(m_backBuffer.pixmapData())->context();
    if (QGLContext::currentContext() != ctx && ctx && ctx->isValid())
        ctx->makeCurrent();
    eglWaitClient();

    // Copy srcRect from the backing store to the new pixmap
    XSetGraphicsExposures(X11->display, tmpGc, False);
    XCopyArea(X11->display, m_backBuffer.handle(), px.handle(), tmpGc,
              srcRect.x(), srcRect.y(), srcRect.width(), srcRect.height(), 0, 0);
    XFreeGC(X11->display, tmpGc);

    // Wait until the copy has finised before allowing more rendering into the back buffer
    eglWaitNative(EGL_CORE_NATIVE_ENGINE);

    return px;
}

QT_END_NAMESPACE
