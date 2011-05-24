/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qxcbwindowsurface.h"

#include "qxcbconnection.h"
#include "qxcbscreen.h"
#include "qxcbwindow.h"

#include <xcb/shm.h>
#include <xcb/xcb_image.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <stdio.h>

#include <qdebug.h>
#include <qpainter.h>

class QXcbShmImage : public QXcbObject
{
public:
    QXcbShmImage(QXcbScreen *connection, const QSize &size, uint depth, QImage::Format format);
    ~QXcbShmImage() { destroy(); }

    QImage *image() { return &m_qimage; }

    void put(xcb_window_t window, const QPoint &dst, const QRect &source);
    void preparePaint(const QRegion &region);

private:
    void destroy();

    xcb_shm_segment_info_t m_shm_info;

    xcb_image_t *m_xcb_image;

    QImage m_qimage;

    xcb_gcontext_t m_gc;
    xcb_window_t m_gc_window;

    QRegion m_dirty;
};

QXcbShmImage::QXcbShmImage(QXcbScreen *screen, const QSize &size, uint depth, QImage::Format format)
    : QXcbObject(screen->connection())
    , m_gc(0)
    , m_gc_window(0)
{
    Q_XCB_NOOP(connection());
    m_xcb_image = xcb_image_create_native(xcb_connection(),
                                          size.width(),
                                          size.height(),
                                          XCB_IMAGE_FORMAT_Z_PIXMAP,
                                          depth,
                                          0,
                                          ~0,
                                          0);

    m_shm_info.shmid = shmget (IPC_PRIVATE,
          m_xcb_image->stride * m_xcb_image->height, IPC_CREAT|0777);

    m_shm_info.shmaddr = m_xcb_image->data = (quint8 *)shmat (m_shm_info.shmid, 0, 0);
    m_shm_info.shmseg = xcb_generate_id(xcb_connection());

    xcb_generic_error_t *error = xcb_request_check(xcb_connection(), xcb_shm_attach_checked(xcb_connection(), m_shm_info.shmseg, m_shm_info.shmid, false));
    if (error) {
        qWarning() << "QXcbWindowSurface: Unable to attach to shared memory segment";
        free(error);
    }

    if (shmctl(m_shm_info.shmid, IPC_RMID, 0) == -1)
        qWarning() << "QXcbWindowSurface: Error while marking the shared memory segment to be destroyed";

    m_qimage = QImage( (uchar*) m_xcb_image->data, m_xcb_image->width, m_xcb_image->height, m_xcb_image->stride, format);
}

void QXcbShmImage::destroy()
{
    Q_XCB_CALL(xcb_shm_detach(xcb_connection(), m_shm_info.shmseg));
    xcb_image_destroy(m_xcb_image);
    shmdt(m_shm_info.shmaddr);
    shmctl(m_shm_info.shmid, IPC_RMID, 0);
    if (m_gc)
        Q_XCB_CALL(xcb_free_gc(xcb_connection(), m_gc));
}

void QXcbShmImage::put(xcb_window_t window, const QPoint &target, const QRect &source)
{
    Q_XCB_NOOP(connection());
    if (m_gc_window != window) {
        if (m_gc)
            Q_XCB_CALL(xcb_free_gc(xcb_connection(), m_gc));

        m_gc = xcb_generate_id(xcb_connection());
        Q_XCB_CALL(xcb_create_gc(xcb_connection(), m_gc, window, 0, 0));

        m_gc_window = window;
    }

    Q_XCB_NOOP(connection());
    xcb_image_shm_put(xcb_connection(),
                      window,
                      m_gc,
                      m_xcb_image,
                      m_shm_info,
                      source.x(),
                      source.y(),
                      target.x(),
                      target.y(),
                      source.width(),
                      source.height(),
                      false);
    Q_XCB_NOOP(connection());

    m_dirty = m_dirty | source;

    xcb_flush(xcb_connection());
    Q_XCB_NOOP(connection());
}

void QXcbShmImage::preparePaint(const QRegion &region)
{
    // to prevent X from reading from the image region while we're writing to it
    if (m_dirty.intersects(region)) {
        connection()->sync();
        m_dirty = QRegion();
    }
}

QXcbWindowSurface::QXcbWindowSurface(QWidget *widget, bool setDefaultSurface)
    : QWindowSurface(widget, setDefaultSurface)
    , m_image(0)
    , m_syncingResize(false)
{
    QXcbScreen *screen = static_cast<QXcbScreen *>(QPlatformScreen::platformScreenForWidget(widget));
    setConnection(screen->connection());
}

QXcbWindowSurface::~QXcbWindowSurface()
{
    delete m_image;
}

QPaintDevice *QXcbWindowSurface::paintDevice()
{
    return m_image->image();
}

void QXcbWindowSurface::beginPaint(const QRegion &region)
{
    m_image->preparePaint(region);

    if (m_image->image()->hasAlphaChannel()) {
        QPainter p(m_image->image());
        p.setCompositionMode(QPainter::CompositionMode_Source);
        const QVector<QRect> rects = region.rects();
        const QColor blank = Qt::transparent;
        for (QVector<QRect>::const_iterator it = rects.begin(); it != rects.end(); ++it) {
            p.fillRect(*it, blank);
        }
    }
}

void QXcbWindowSurface::endPaint(const QRegion &)
{
}

void QXcbWindowSurface::flush(QWidget *widget, const QRegion &region, const QPoint &offset)
{
    QRect bounds = region.boundingRect();

    if (size().isEmpty() || !geometry().contains(bounds))
        return;

    Q_XCB_NOOP(connection());

    QXcbWindow *window = static_cast<QXcbWindow *>(widget->window()->platformWindow());

    extern QWidgetData* qt_widget_data(QWidget *);
    QPoint widgetOffset = qt_qwidget_data(widget)->wrect.topLeft();

    QVector<QRect> rects = region.rects();
    for (int i = 0; i < rects.size(); ++i)
        m_image->put(window->window(), rects.at(i).topLeft() - widgetOffset, rects.at(i).translated(offset));

    Q_XCB_NOOP(connection());

    if (m_syncingResize) {
        xcb_flush(xcb_connection());
        connection()->sync();
        m_syncingResize = false;
        window->updateSyncRequestCounter();
    }
}

void QXcbWindowSurface::resize(const QSize &size)
{
    if (size == QWindowSurface::size())
        return;

    Q_XCB_NOOP(connection());
    QWindowSurface::resize(size);

    QXcbScreen *screen = static_cast<QXcbScreen *>(QPlatformScreen::platformScreenForWidget(window()));
    QXcbWindow* win = static_cast<QXcbWindow *>(window()->platformWindow());

    delete m_image;
    m_image = new QXcbShmImage(screen, size, win->depth(), win->format());
    Q_XCB_NOOP(connection());

    m_syncingResize = true;
}

extern void qt_scrollRectInImage(QImage &img, const QRect &rect, const QPoint &offset);

bool QXcbWindowSurface::scroll(const QRegion &area, int dx, int dy)
{
    if (m_image->image()->isNull())
        return false;

    m_image->preparePaint(area);

    const QVector<QRect> rects = area.rects();
    for (int i = 0; i < rects.size(); ++i)
        qt_scrollRectInImage(*m_image->image(), rects.at(i), QPoint(dx, dy));

    return true;
}

