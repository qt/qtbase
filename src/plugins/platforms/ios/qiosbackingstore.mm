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

#include "qiosbackingstore.h"
#include "qioswindow.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/private/qwindow_p.h>

#include <QtDebug>

QT_BEGIN_NAMESPACE

/*!
    \class QIOSBackingStore

    QBackingStore enables the use of QPainter to paint on a QWindow, as opposed
    to rendering to a QWindow through the use of OpenGL with QOpenGLContext.
*/
QIOSBackingStore::QIOSBackingStore(QWindow *window)
    : QRasterBackingStore(window)
{
    // We use the surface both for raster operations and for GL drawing (when
    // we blit the raster image), so the type needs to cover both use cases.
    if (window->surfaceType() == QSurface::RasterSurface)
        window->setSurfaceType(QSurface::RasterGLSurface);

    Q_ASSERT_X(window->surfaceType() != QSurface::OpenGLSurface, "QIOSBackingStore",
        "QBackingStore on iOS can only be used with raster-enabled surfaces.");
}

QIOSBackingStore::~QIOSBackingStore()
{
}

void QIOSBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_ASSERT(!qt_window_private(window)->compositing);

    Q_UNUSED(region);
    Q_UNUSED(offset);

    if (window != this->window()) {
        // We skip flushing raster-based child windows, to avoid the extra cost of copying from the
        // parent FBO into the child FBO. Since the child is already drawn inside the parent FBO, it
        // will become visible when flushing the parent. The only case we end up not supporting is if
        // the child window overlaps a sibling window that's draws using a separate QOpenGLContext.
        return;
    }

    static QPlatformTextureList emptyTextureList;
    composeAndFlush(window, region, offset, &emptyTextureList, false);
}

QT_END_NAMESPACE
