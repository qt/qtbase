/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenVG module of the Qt Toolkit.
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

#include "qwindowsurface_vg_p.h"
#include "qwindowsurface_vgegl_p.h"
#include "qpaintengine_vg_p.h"
#include "qpixmapdata_vg_p.h"
#include "qvg_p.h"

#if !defined(QT_NO_EGL)

#include <QtGui/private/qeglcontext_p.h>
#include <QtGui/private/qwidget_p.h>

QT_BEGIN_NAMESPACE

QVGWindowSurface::QVGWindowSurface(QWidget *window)
    : QWindowSurface(window)
{
    // Create the default type of EGL window surface for windows.
    d_ptr = new QVGEGLWindowSurfaceDirect(this);
}

QVGWindowSurface::QVGWindowSurface
        (QWidget *window, QVGEGLWindowSurfacePrivate *d)
    : QWindowSurface(window), d_ptr(d)
{
}

QVGWindowSurface::~QVGWindowSurface()
{
    delete d_ptr;
}

QPaintDevice *QVGWindowSurface::paintDevice()
{
    return this;
}

void QVGWindowSurface::flush(QWidget *widget, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(offset);

#ifdef Q_OS_SYMBIAN
    if (window() != widget) {
        // For performance reasons we don't support
        // flushing native child widgets on Symbian.
        // It breaks overlapping native child widget 
        // rendering in some cases but we prefer performance.
        return;
    }
#endif

    QWidget *parent = widget->internalWinId() ? widget : widget->nativeParentWidget();
    d_ptr->endPaint(parent, region);
}

void QVGWindowSurface::setGeometry(const QRect &rect)
{
    QWindowSurface::setGeometry(rect);
}

bool QVGWindowSurface::scroll(const QRegion &area, int dx, int dy)
{
    if (!d_ptr->scroll(window(), area, dx, dy))
        return QWindowSurface::scroll(area, dx, dy);
    return true;
}

void QVGWindowSurface::beginPaint(const QRegion &region)
{
    d_ptr->beginPaint(window());

    // If the window is not opaque, then fill the region we are about
    // to paint with the transparent color.
    if (!qt_widget_private(window())->isOpaque &&
            window()->testAttribute(Qt::WA_TranslucentBackground)) {
        QVGPaintEngine *engine = static_cast<QVGPaintEngine *>
            (d_ptr->paintEngine());
        engine->fillRegion(region, Qt::transparent, d_ptr->surfaceSize());
    }
}

void QVGWindowSurface::endPaint(const QRegion &region)
{
    // Nothing to do here.
    Q_UNUSED(region);
}

QPaintEngine *QVGWindowSurface::paintEngine() const
{
    return d_ptr->paintEngine();
}

QWindowSurface::WindowSurfaceFeatures QVGWindowSurface::features() const
{
    WindowSurfaceFeatures features = PartialUpdates | PreservedContents;
    if (d_ptr->supportsStaticContents())
        features |= StaticContents;
    return features;
}

int QVGWindowSurface::metric(PaintDeviceMetric met) const
{
    return qt_paint_device_metric(window(), met);
}

QT_END_NAMESPACE

#endif
