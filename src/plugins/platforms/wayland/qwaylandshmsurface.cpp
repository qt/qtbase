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
#include "qwaylandshmsurface.h"

#include <QtCore/qdebug.h>
#include <QtGui/private/qapplication_p.h>

#include "qwaylanddisplay.h"
#include "qwaylandshmwindow.h"
#include "qwaylandscreen.h"

#include <wayland-client.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>

QT_BEGIN_NAMESPACE

QWaylandShmBuffer::QWaylandShmBuffer(QWaylandDisplay *display,
				     const QSize &size, QImage::Format format)
{
    int stride = size.width() * 4;
    int alloc = stride * size.height();
    char filename[] = "/tmp/wayland-shm-XXXXXX";
    int fd = mkstemp(filename);
    if (fd < 0)
	qWarning("open %s failed: %s", filename, strerror(errno));
    if (ftruncate(fd, alloc) < 0) {
	qWarning("ftruncate failed: %s", strerror(errno));
	close(fd);
	return;
    }
    uchar *data = (uchar *)
	mmap(NULL, alloc, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    unlink(filename);

    if (data == (uchar *) MAP_FAILED) {
	qWarning("mmap /dev/zero failed: %s", strerror(errno));
	close(fd);
	return;
    }

    mImage = QImage(data, size.width(), size.height(), stride, format);
    mBuffer = display->createShmBuffer(fd, size.width(), size.height(),
				       stride, display->argbVisual());
    close(fd);
}

QWaylandShmBuffer::~QWaylandShmBuffer(void)
{
    munmap((void *) mImage.constBits(), mImage.byteCount());
    wl_buffer_destroy(mBuffer);
}

QWaylandShmWindowSurface::QWaylandShmWindowSurface(QWidget *window)
    : QWindowSurface(window)
    , mBuffer(0)
    , mDisplay(QWaylandScreen::waylandScreenFromWidget(window)->display())
{
}

QWaylandShmWindowSurface::~QWaylandShmWindowSurface()
{
}

QPaintDevice *QWaylandShmWindowSurface::paintDevice()
{
    return mBuffer->image();
}

void QWaylandShmWindowSurface::beginPaint(const QRegion &)
{
    QWaylandShmWindow *waylandWindow = static_cast<QWaylandShmWindow *>(window()->platformWindow());
    Q_ASSERT(waylandWindow->windowType() == QWaylandWindow::Shm);
    waylandWindow->waitForFrameSync();
}

void QWaylandShmWindowSurface::flush(QWidget *widget, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(widget);
    Q_UNUSED(offset);
    QWaylandShmWindow *waylandWindow = static_cast<QWaylandShmWindow *>(window()->platformWindow());
    Q_ASSERT(waylandWindow->windowType() == QWaylandWindow::Shm);
    QVector<QRect> rects = region.rects();
    for (int i = 0; i < rects.size(); i++) {
        const QRect rect = rects.at(i);
        wl_buffer_damage(mBuffer->buffer(),rect.x(),rect.y(),rect.width(),rect.height());
        waylandWindow->damage(rect);
    }
}

void QWaylandShmWindowSurface::resize(const QSize &size)
{
    QWaylandShmWindow *waylandWindow = static_cast<QWaylandShmWindow *>(window()->platformWindow());
    Q_ASSERT(waylandWindow->windowType() == QWaylandWindow::Shm);

    QWindowSurface::resize(size);
    QImage::Format format = QPlatformScreen::platformScreenForWidget(window())->format();

    if (mBuffer != NULL && mBuffer->size() == size)
	return;

    if (mBuffer != NULL)
	delete mBuffer;

    mBuffer = new QWaylandShmBuffer(mDisplay, size, format);

    waylandWindow->attach(mBuffer);
}

QT_END_NAMESPACE
