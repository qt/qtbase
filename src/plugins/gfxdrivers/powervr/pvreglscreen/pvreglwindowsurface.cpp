/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "pvreglwindowsurface.h"
#include "pvreglscreen.h"
#include <QScreen>
#include <QDebug>
#include <QWSDisplay>

PvrEglWindowSurface::PvrEglWindowSurface
        (QWidget *widget, PvrEglScreen *screen, int screenNum)
    : QWSGLWindowSurface(widget)
{
    setSurfaceFlags(QWSWindowSurface::Opaque);

    this->widget = widget;
    this->screen = screen;
    this->pdevice = 0;

    QPoint pos = offset(widget);
    QSize size = widget->size();

    PvrQwsRect pvrRect;
    pvrRect.x = pos.x();
    pvrRect.y = pos.y();
    pvrRect.width = size.width();
    pvrRect.height = size.height();
    transformRects(&pvrRect, 1);

    // Try to recover a previous PvrQwsDrawable object for the widget
    // if there is one.  This can happen when a PvrEglWindowSurface
    // is created for a widget, bound to a EGLSurface, and then destroyed.
    // When a new PvrEglWindowSurface is created for the widget, it will
    // pick up the previous PvrQwsDrawable if the EGLSurface has not been
    // destroyed in the meantime.
    drawable = pvrQwsFetchWindow((long)widget);
    if (drawable)
        pvrQwsSetGeometry(drawable, &pvrRect);
    else
        drawable = pvrQwsCreateWindow(screenNum, (long)widget, &pvrRect);
    pvrQwsSetRotation(drawable, screen->transformation());
}

PvrEglWindowSurface::PvrEglWindowSurface()
    : QWSGLWindowSurface()
{
    setSurfaceFlags(QWSWindowSurface::Opaque);
    drawable = 0;
    widget = 0;
    screen = 0;
    pdevice = 0;
}

PvrEglWindowSurface::~PvrEglWindowSurface()
{
    // Release the PvrQwsDrawable.  If it is bound to an EGLSurface,
    // then it will stay around until a new PvrEglWindowSurface is
    // created for the widget.  If it is not bound to an EGLSurface,
    // it will be destroyed immediately.
    if (drawable && pvrQwsReleaseWindow(drawable))
        pvrQwsDestroyDrawable(drawable);

    delete pdevice;
}

bool PvrEglWindowSurface::isValid() const
{
    return (widget != 0);
}

void PvrEglWindowSurface::setGeometry(const QRect &rect)
{
    if (drawable) {
        // XXX: adjust for the screen offset.
        PvrQwsRect pvrRect;
        pvrRect.x = rect.x();
        pvrRect.y = rect.y();
        pvrRect.width = rect.width();
        pvrRect.height = rect.height();
        transformRects(&pvrRect, 1);
        pvrQwsSetGeometry(drawable, &pvrRect);
        pvrQwsSetRotation(drawable, screen->transformation());
    }
    QWSGLWindowSurface::setGeometry(rect);
}

bool PvrEglWindowSurface::move(const QPoint &offset)
{
    QRect rect = geometry().translated(offset); 
    if (drawable) {
        PvrQwsRect pvrRect;
        pvrRect.x = rect.x();
        pvrRect.y = rect.y();
        pvrRect.width = rect.width();
        pvrRect.height = rect.height();
        transformRects(&pvrRect, 1);
        pvrQwsSetGeometry(drawable, &pvrRect);
        pvrQwsSetRotation(drawable, screen->transformation());
    }
    return QWSGLWindowSurface::move(offset);
}

QByteArray PvrEglWindowSurface::permanentState() const
{
    // Nothing interesting to pass to the server just yet.
    return QByteArray();
}

void PvrEglWindowSurface::setPermanentState(const QByteArray &state)
{
    Q_UNUSED(state);
}

void PvrEglWindowSurface::flush
        (QWidget *widget, const QRegion &region, const QPoint &offset)
{
    // The GL paint engine is responsible for the swapBuffers() call.
    // If we were to call the base class's implementation of flush()
    // then it would fetch the image() and manually blit it to the
    // screeen instead of using the fast PVR2D blit.
    Q_UNUSED(widget);
    Q_UNUSED(region);
    Q_UNUSED(offset);
}

QImage PvrEglWindowSurface::image() const
{
    if (drawable) {
        PvrQwsRect pvrRect;
        pvrQwsGetGeometry(drawable, &pvrRect);
        void *data = pvrQwsGetRenderBuffer(drawable);
        if (data) {
            return QImage((uchar *)data, pvrRect.width, pvrRect.height,
                          pvrQwsGetStride(drawable), screen->pixelFormat());
        }
    }
    return QImage(16, 16, screen->pixelFormat());
}

QPaintDevice *PvrEglWindowSurface::paintDevice()
{
    return widget;
}

void PvrEglWindowSurface::setDirectRegion(const QRegion &r, int id)
{
    QWSGLWindowSurface::setDirectRegion(r, id);

    if (!drawable)
        return;

    // Clip the region to the window boundaries in case the child
    // is partially outside the geometry of the parent.
    QWidget *window = widget->window();
    QRegion region = r;
    if (widget != window) {
	QRect rect = window->geometry();
        rect.moveTo(window->mapToGlobal(QPoint(0, 0)));
        region = region.intersect(rect);
    }

    if (region.isEmpty()) {
        pvrQwsClearVisibleRegion(drawable);
    } else if (region.rectCount() == 1) {
        QRect rect = region.boundingRect();
        PvrQwsRect pvrRect;
        pvrRect.x = rect.x();
        pvrRect.y = rect.y();
        pvrRect.width = rect.width();
        pvrRect.height = rect.height();
        transformRects(&pvrRect, 1);
        pvrQwsSetVisibleRegion(drawable, &pvrRect, 1);
        pvrQwsSetRotation(drawable, screen->transformation());
        if (!pvrQwsSwapBuffers(drawable, 1))
            screen->solidFill(QColor(0, 0, 0), region);
    } else {
        QVector<QRect> rects = region.rects();
        PvrQwsRect *pvrRects = new PvrQwsRect [rects.size()];
        for (int index = 0; index < rects.size(); ++index) {
            QRect rect = rects[index];
            pvrRects[index].x = rect.x();
            pvrRects[index].y = rect.y();
            pvrRects[index].width = rect.width();
            pvrRects[index].height = rect.height();
        }
        transformRects(pvrRects, rects.size());
        pvrQwsSetVisibleRegion(drawable, pvrRects, rects.size());
        pvrQwsSetRotation(drawable, screen->transformation());
        if (!pvrQwsSwapBuffers(drawable, 1))
            screen->solidFill(QColor(0, 0, 0), region);
        delete [] pvrRects;
    }
}

void PvrEglWindowSurface::transformRects(PvrQwsRect *rects, int count) const
{
    switch (screen->transformation()) {
    case 0: break;

    case 90:
    {
        for (int index = 0; index < count; ++index) {
            int x = rects[index].y;
            int y = screen->height() - (rects[index].x + rects[index].width);
            rects[index].x = x;
            rects[index].y = y;
            qSwap(rects[index].width, rects[index].height);
        }
    }
    break;

    case 180:
    {
        for (int index = 0; index < count; ++index) {
            int x = screen->width() - (rects[index].x + rects[index].width);
            int y = screen->height() - (rects[index].y + rects[index].height);
            rects[index].x = x;
            rects[index].y = y;
        }
    }
    break;

    case 270:
    {
        for (int index = 0; index < count; ++index) {
            int x = screen->width() - (rects[index].y + rects[index].height);
            int y = rects[index].x;
            rects[index].x = x;
            rects[index].y = y;
            qSwap(rects[index].width, rects[index].height);
        }
    }
    break;
    }
}
