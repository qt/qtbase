/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwindowsurface_vglite.h"
#include "qgraphicssystem_vglite.h"
#include <QtOpenVG/qvg.h>
#include <QtOpenVG/private/qvg_p.h>
#include <QtOpenVG/private/qpaintengine_vg_p.h>

QT_BEGIN_NAMESPACE

QVGLiteWindowSurface::QVGLiteWindowSurface
        (QVGLiteGraphicsSystem *gs, QWidget *window)
    : QWindowSurface(window), graphicsSystem(gs),
      isPaintingActive(false), engine(0)
{
}

QVGLiteWindowSurface::~QVGLiteWindowSurface()
{
    graphicsSystem->surface = 0;
    if (engine)
        qt_vg_destroy_paint_engine(engine);
}

QPaintDevice *QVGLiteWindowSurface::paintDevice()
{
    qt_vg_make_current(graphicsSystem->context, graphicsSystem->rootWindow);
    isPaintingActive = true;
    // TODO: clear the parts of the back buffer that are not
    // covered by the window surface to black.
    return this;
}

void QVGLiteWindowSurface::flush(QWidget *widget, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(widget);
    Q_UNUSED(region);
    Q_UNUSED(offset);
    QEglContext *context = graphicsSystem->context;
    if (context) {
        if (!isPaintingActive)
            qt_vg_make_current(context, graphicsSystem->rootWindow);
        context->swapBuffers();
        qt_vg_done_current(context);
        context->setSurface(EGL_NO_SURFACE);
        isPaintingActive = false;
    }
}

void QVGLiteWindowSurface::setGeometry(const QRect &rect)
{
    QWindowSurface::setGeometry(rect);
}

bool QVGLiteWindowSurface::scroll(const QRegion &area, int dx, int dy)
{
    return QWindowSurface::scroll(area, dx, dy);
}

void QVGLiteWindowSurface::beginPaint(const QRegion &region)
{
    Q_UNUSED(region);
}

void QVGLiteWindowSurface::endPaint(const QRegion &region)
{
    Q_UNUSED(region);
}

QPaintEngine *QVGLiteWindowSurface::paintEngine() const
{
    if (!engine)
        engine = qt_vg_create_paint_engine();
    return engine;
}

// We need to get access to QWidget::metric() from QVGLiteWindowSurface::metric,
// but it is not a friend of QWidget.  To get around this, we create a
// fake QX11PaintEngine class, which is a friend.
class QX11PaintEngine
{
public:
    static int metric(const QWidget *widget, QPaintDevice::PaintDeviceMetric met)
    {
        return widget->metric(met);
    }
};

int QVGLiteWindowSurface::metric(PaintDeviceMetric met) const
{
    return QX11PaintEngine::metric(window(), met);
}
