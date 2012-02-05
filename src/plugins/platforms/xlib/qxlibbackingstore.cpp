/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qxlibbackingstore.h"
#include "qxlibintegration.h"

#include <QtCore/qdebug.h>
#include <QWindowSystemInterface>

#include "qxlibwindow.h"
#include "qxlibscreen.h"
#include "qxlibdisplay.h"

#include "qpainter.h"

# include <sys/ipc.h>
# include <sys/shm.h>
# include <X11/extensions/XShm.h>

QT_BEGIN_NAMESPACE

struct QXlibShmImageInfo {
    QXlibShmImageInfo(Display *xdisplay) :  image(0), display(xdisplay) {}
    ~QXlibShmImageInfo() { destroy(); }

    void destroy();

    XShmSegmentInfo shminfo;
    XImage *image;
    Display *display;
};


#ifndef DONT_USE_MIT_SHM
void QXlibShmImageInfo::destroy()
{
    XShmDetach (display, &shminfo);
    XDestroyImage (image);
    shmdt (shminfo.shmaddr);
    shmctl (shminfo.shmid, IPC_RMID, 0);
}
#endif

void QXlibBackingStore::resizeShmImage(int width, int height)
{
    QXlibScreen *screen = QXlibScreen::testLiteScreenForWidget(window());
    QXlibWindow *win = static_cast<QXlibWindow*>(window()->handle());

    QImage::Format format = win->depth() == 16 ? QImage::Format_RGB16 : QImage::Format_RGB32;

#ifdef DONT_USE_MIT_SHM
    shm_img = QImage(width, height, format);
#else

    if (image_info)
        image_info->destroy();
    else
        image_info = new QXlibShmImageInfo(screen->display()->nativeDisplay());

    XImage *image = XShmCreateImage (screen->display()->nativeDisplay(), win->visual(), win->depth(), ZPixmap, 0,
                                     &image_info->shminfo, width, height);


    image_info->shminfo.shmid = shmget (IPC_PRIVATE,
          image->bytes_per_line * image->height, IPC_CREAT|0777);

    image_info->shminfo.shmaddr = image->data = (char*)shmat (image_info->shminfo.shmid, 0, 0);
    image_info->shminfo.readOnly = False;

    image_info->image = image;

    Status shm_attach_status = XShmAttach(screen->display()->nativeDisplay(), &image_info->shminfo);

    Q_ASSERT(shm_attach_status == True);

    shm_img = QImage((uchar*) image->data, image->width, image->height, image->bytes_per_line, format);
#endif
    painted = false;
}


void QXlibBackingStore::resizeBuffer(QSize s)
{
    if (shm_img.size() != s)
        resizeShmImage(s.width(), s.height());
}

QSize QXlibBackingStore::bufferSize() const
{
    return shm_img.size();
}

QXlibBackingStore::QXlibBackingStore(QWindow *window)
    : QPlatformBackingStore(window),
      painted(false), image_info(0)
{
    xw = static_cast<QXlibWindow*>(window->handle());
//    qDebug() << "QTestLiteWindowSurface::QTestLiteWindowSurface:" << xw->window;
}

QXlibBackingStore::~QXlibBackingStore()
{
    delete image_info;
}

QPaintDevice *QXlibBackingStore::paintDevice()
{
    return &shm_img;
}


void QXlibBackingStore::flush(QWindow *w, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);
    Q_UNUSED(offset);

    if (!painted)
        return;

    QXlibScreen *screen = QXlibScreen::testLiteScreenForWidget(w);
    GC gc = xw->graphicsContext();
    Window window = xw->xWindow();
#ifdef DONT_USE_MIT_SHM
    // just convert the image every time...
    if (!shm_img.isNull()) {
        QXlibWindow *win = static_cast<QXlibWindow*>(w->handle());

        QImage image = shm_img;
        //img.convertToFormat(
        XImage *xi = XCreateImage(screen->display()->nativeDisplay(), win->visual(), win->depth(), ZPixmap,
                                  0, (char *) image.scanLine(0), image.width(), image.height(),
                                  32, image.bytesPerLine());

        int x = 0;
        int y = 0;

        /*int r =*/  XPutImage(screen->display()->nativeDisplay(), window, gc, xi, 0, 0, x, y, image.width(), image.height());

        xi->data = 0; // QImage owns these bits
        XDestroyImage(xi);
    }
#else
    // Use MIT_SHM
    if (image_info && image_info->image) {
        //qDebug() << "Here we go" << image_info->image->width << image_info->image->height;
        int x = 0;
        int y = 0;

        // We could set send_event to true, and then use the ShmCompletion to synchronize,
        // but let's do like Qt/11 and just use XSync
        XShmPutImage (screen->display()->nativeDisplay(), window, gc, image_info->image, 0, 0,
                      x, y, image_info->image->width, image_info->image->height,
                      /*send_event*/ False);

        screen->display()->sync();
    }
#endif
}

void QXlibBackingStore::resize(const QSize &size, const QRegion &)
{
    resizeBuffer(size);
}


void QXlibBackingStore::beginPaint(const QRegion &region)
{
    Q_UNUSED(region);

    if (shm_img.hasAlphaChannel()) {
        QPainter p(&shm_img);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        const QVector<QRect> rects = region.rects();
        const QColor blank = Qt::transparent;
        for (QVector<QRect>::const_iterator it = rects.begin(); it != rects.end(); ++it) {
            p.fillRect(*it, blank);
        }
    }
}

void QXlibBackingStore::endPaint()
{
    painted = true; //there is content in the buffer
}
QT_END_NAMESPACE
