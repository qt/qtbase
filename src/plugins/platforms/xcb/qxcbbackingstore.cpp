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

#include "qxcbbackingstore.h"

#include "qxcbconnection.h"
#include "qxcbscreen.h"
#include "qxcbwindow.h"

#include <xcb/shm.h>
#include <xcb/xcb_image.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <stdio.h>
#include <errno.h>

#include <qdebug.h>
#include <qpainter.h>

class QXcbShmImage : public QXcbObject
{
public:
    QXcbShmImage(QXcbScreen *connection, const QSize &size, uint depth, QImage::Format format);
    ~QXcbShmImage() { destroy(); }

    QImage *image() { return &m_qimage; }
    QSize size() const { return m_qimage.size(); }

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

    const int segmentSize = m_xcb_image->stride * m_xcb_image->height;
    if (!segmentSize)
        return;

    int id = shmget(IPC_PRIVATE, segmentSize, IPC_CREAT | 0777);
    if (id == -1)
        qWarning("QXcbShmImage: shmget() failed (%d) for size %d (%dx%d)",
                 errno, segmentSize, size.width(), size.height());
    else
        m_shm_info.shmid = id;
    m_shm_info.shmaddr = m_xcb_image->data = (quint8 *)shmat (m_shm_info.shmid, 0, 0);
    m_shm_info.shmseg = xcb_generate_id(xcb_connection());

    xcb_generic_error_t *error = xcb_request_check(xcb_connection(), xcb_shm_attach_checked(xcb_connection(), m_shm_info.shmseg, m_shm_info.shmid, false));
    if (error) {
        qWarning() << "QXcbBackingStore: Unable to attach to shared memory segment";
        free(error);
    }

    if (shmctl(m_shm_info.shmid, IPC_RMID, 0) == -1)
        qWarning() << "QXcbBackingStore: Error while marking the shared memory segment to be destroyed";

    m_qimage = QImage( (uchar*) m_xcb_image->data, m_xcb_image->width, m_xcb_image->height, m_xcb_image->stride, format);
}

void QXcbShmImage::destroy()
{
    const int segmentSize = m_xcb_image ? (m_xcb_image->stride * m_xcb_image->height) : 0;
    if (segmentSize)
        Q_XCB_CALL(xcb_shm_detach(xcb_connection(), m_shm_info.shmseg));

    xcb_image_destroy(m_xcb_image);

    if (segmentSize) {
        shmdt(m_shm_info.shmaddr);
        shmctl(m_shm_info.shmid, IPC_RMID, 0);
    }

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

QXcbBackingStore::QXcbBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , m_image(0)
    , m_syncingResize(false)
{
    QXcbScreen *screen = static_cast<QXcbScreen *>(QPlatformScreen::platformScreenForWindow(window));
    setConnection(screen->connection());
}

QXcbBackingStore::~QXcbBackingStore()
{
    delete m_image;
}

QPaintDevice *QXcbBackingStore::paintDevice()
{
    return m_image->image();
}

void QXcbBackingStore::beginPaint(const QRegion &region)
{
    m_image->preparePaint(region);

#if 0
    if (m_image->image()->hasAlphaChannel()) {
        QPainter p(m_image->image());
        p.setCompositionMode(QPainter::CompositionMode_Source);
        const QVector<QRect> rects = region.rects();
        const QColor blank = Qt::transparent;
        for (QVector<QRect>::const_iterator it = rects.begin(); it != rects.end(); ++it) {
            p.fillRect(*it, blank);
        }
    }
#endif
}

void QXcbBackingStore::endPaint(const QRegion &)
{
}

void QXcbBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    QRect bounds = region.boundingRect();

    if (!m_image || m_image->size().isEmpty())
        return;

    Q_XCB_NOOP(connection());

    QXcbWindow *platformWindow = static_cast<QXcbWindow *>(window->handle());

    QVector<QRect> rects = region.rects();
    for (int i = 0; i < rects.size(); ++i)
        m_image->put(platformWindow->xcb_window(), rects.at(i).topLeft(), rects.at(i).translated(offset));

    Q_XCB_NOOP(connection());

    if (m_syncingResize) {
        xcb_flush(xcb_connection());
        connection()->sync();
        m_syncingResize = false;
        platformWindow->updateSyncRequestCounter();
    }
}

void QXcbBackingStore::resize(const QSize &size, const QRegion &)
{
    if (m_image && size == m_image->size())
        return;

    Q_XCB_NOOP(connection());

    QXcbScreen *screen = static_cast<QXcbScreen *>(QPlatformScreen::platformScreenForWindow(window()));
    QXcbWindow* win = static_cast<QXcbWindow *>(window()->handle());

    delete m_image;
    m_image = new QXcbShmImage(screen, size, win->depth(), win->format());
    Q_XCB_NOOP(connection());

    m_syncingResize = true;
}

extern void qt_scrollRectInImage(QImage &img, const QRect &rect, const QPoint &offset);

bool QXcbBackingStore::scroll(const QRegion &area, int dx, int dy)
{
    if (!m_image || m_image->image()->isNull())
        return false;

    m_image->preparePaint(area);

    const QVector<QRect> rects = area.rects();
    for (int i = 0; i < rects.size(); ++i)
        qt_scrollRectInImage(*m_image->image(), rects.at(i), QPoint(dx, dy));

    return true;
}

