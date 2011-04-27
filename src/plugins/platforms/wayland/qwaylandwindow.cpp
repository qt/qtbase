/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the config.tests of the Qt Toolkit.
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

#include "qwaylandwindow.h"

#include "qwaylandbuffer.h"
#include "qwaylanddisplay.h"
#include "qwaylandinputdevice.h"
#include "qwaylandscreen.h"

#include <QtGui/QWidget>
#include <QtGui/QWindowSystemInterface>

#include <QDebug>

QWaylandWindow::QWaylandWindow(QWidget *window)
    : QPlatformWindow(window)
    , mDisplay(QWaylandScreen::waylandScreenFromWidget(window)->display())
    , mBuffer(0)
    , mWaitingForFrameSync(false)
{
    static WId id = 1;
    mWindowId = id++;

    mSurface = mDisplay->createSurface(this);
}

QWaylandWindow::~QWaylandWindow()
{
    if (mSurface)
        wl_surface_destroy(mSurface);

    QList<QWaylandInputDevice *> inputDevices = mDisplay->inputDevices();
    for (int i = 0; i < inputDevices.size(); ++i)
        inputDevices.at(i)->handleWindowDestroyed(this);
}

WId QWaylandWindow::winId() const
{
    return mWindowId;
}

void QWaylandWindow::setParent(const QPlatformWindow *parent)
{
    Q_UNUSED(parent);
    qWarning("Sub window is not supported");
}

void QWaylandWindow::setVisible(bool visible)
{
    if (!mSurface && visible) {
        mSurface = mDisplay->createSurface(this);
        newSurfaceCreated();
    }

    if (visible) {
        wl_surface_map_toplevel(mSurface);
    } else {
        wl_surface_destroy(mSurface);
        mSurface = NULL;
    }
}

void QWaylandWindow::configure(uint32_t time, uint32_t edges,
                               int32_t x, int32_t y,
                               int32_t width, int32_t height)
{
    Q_UNUSED(time);
    Q_UNUSED(edges);
    QRect geometry = QRect(x, y, width, height);

    setGeometry(geometry);

    QWindowSystemInterface::handleGeometryChange(widget(), geometry);
}

void QWaylandWindow::attach(QWaylandBuffer *buffer)
{
    mBuffer = buffer;
    if (mSurface) {
        wl_surface_attach(mSurface, buffer->buffer(),0,0);
    }
}


void QWaylandWindow::damage(const QRegion &region)
{
    //We have to do sync stuff before calling damage, or we might
    //get a frame callback before we get the timestamp
    mDisplay->frameCallback(QWaylandWindow::frameCallback, mSurface, this);
    mWaitingForFrameSync = true;

    QVector<QRect> rects = region.rects();
    for (int i = 0; i < rects.size(); i++) {
        const QRect rect = rects.at(i);
        wl_surface_damage(mSurface,
                          rect.x(), rect.y(), rect.width(), rect.height());
    }
}

void QWaylandWindow::newSurfaceCreated()
{
    if (mBuffer) {
        wl_surface_attach(mSurface,mBuffer->buffer(),0,0);
    }
}

void QWaylandWindow::frameCallback(struct wl_surface *surface, void *data, uint32_t time)
{
    Q_UNUSED(time);
    QWaylandWindow *self = static_cast<QWaylandWindow*>(data);
    self->mWaitingForFrameSync = false;
}

void QWaylandWindow::waitForFrameSync()
{
    mDisplay->flushRequests();
    while (mWaitingForFrameSync)
        mDisplay->blockingReadEvents();
}
