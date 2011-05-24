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

#ifndef QWINDOWSURFACE_GL_P_H
#define QWINDOWSURFACE_GL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <qglobal.h>
#include <qgl.h>
#include <private/qwindowsurface_p.h>
#include <private/qglpaintdevice_p.h>

QT_BEGIN_NAMESPACE

class QPaintDevice;
class QPoint;
class QRegion;
class QWidget;
struct QGLWindowSurfacePrivate;

Q_OPENGL_EXPORT QGLWidget* qt_gl_share_widget();
Q_OPENGL_EXPORT void qt_destroy_gl_share_widget();

class QGLWindowSurfaceGLPaintDevice : public QGLPaintDevice
{
public:
    QPaintEngine* paintEngine() const;
    void endPaint();
    QSize size() const;
    int metric(PaintDeviceMetric m) const;
    QGLContext* context() const;
    QGLWindowSurfacePrivate* d;
};

class Q_OPENGL_EXPORT QGLWindowSurface : public QObject, public QWindowSurface // , public QPaintDevice
{
    Q_OBJECT
public:
    QGLWindowSurface(QWidget *window);
    ~QGLWindowSurface();

    QPaintDevice *paintDevice();
    void flush(QWidget *widget, const QRegion &region, const QPoint &offset);

#if !defined(Q_WS_QPA)
    void setGeometry(const QRect &rect);
#else
    virtual void resize(const QSize &size);
#endif

    void updateGeometry();
    bool scroll(const QRegion &area, int dx, int dy);

    void beginPaint(const QRegion &region);
    void endPaint(const QRegion &region);

    QImage *buffer(const QWidget *widget);

    WindowSurfaceFeatures features() const;

    QGLContext *context() const;

    static QGLFormat surfaceFormat;

    enum SwapMode { AutomaticSwap, AlwaysFullSwap, AlwaysPartialSwap, KillSwap };
    static SwapMode swapBehavior;

private slots:
    void deleted(QObject *object);

private:
    void hijackWindow(QWidget *widget);
    bool initializeOffscreenTexture(const QSize &size);

    QGLWindowSurfacePrivate *d_ptr;
};

QT_END_NAMESPACE

#endif // QWINDOWSURFACE_GL_P_H

