/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
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

#include <QtGui/QPaintDevice>
#include <QtGui/QWidget>
#include <QtOpenGL/QGLWidget>
#include "private/qglwindowsurface_qws_p.h"
#include "private/qpaintengine_opengl_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QWSGLWindowSurface
    \since 4.3
    \ingroup qws
    \preliminary

    \brief The QWSGLWindowSurface class provides the drawing area for top-level
    windows with Qt for Embedded Linux on EGL/OpenGL ES. It also provides the
    drawing area for \l{QGLWidget}s whether they are top-level windows or
    children of another QWidget.

    Note that this class is only available in Qt for Embedded Linux and only
    available if Qt is configured with OpenGL support.
*/

class QWSGLWindowSurfacePrivate
{
public:
    QWSGLWindowSurfacePrivate() :
        qglContext(0), ownsContext(false) {}

    QGLContext *qglContext;
    bool ownsContext;
};

/*!
    Constructs an empty QWSGLWindowSurface for the given top-level \a window.
    The window surface is later initialized from chooseContext() and resources for it
    is typically allocated in setGeometry().
*/
QWSGLWindowSurface::QWSGLWindowSurface(QWidget *window)
    : QWSWindowSurface(window),
      d_ptr(new QWSGLWindowSurfacePrivate)
{
}

/*!
    Constructs an empty QWSGLWindowSurface.
*/
QWSGLWindowSurface::QWSGLWindowSurface()
    : d_ptr(new QWSGLWindowSurfacePrivate)
{
}

/*!
    Destroys the QWSGLWindowSurface object and frees any
    allocated resources.
 */
QWSGLWindowSurface::~QWSGLWindowSurface()
{
    Q_D(QWSGLWindowSurface);
    if (d->ownsContext)
        delete d->qglContext;
    delete d;
}

/*!
    Returns the QGLContext of the window surface.
*/
QGLContext *QWSGLWindowSurface::context() const
{
    Q_D(const QWSGLWindowSurface);
    if (!d->qglContext) {
        QWSGLWindowSurface *that = const_cast<QWSGLWindowSurface*>(this);
        that->setContext(new QGLContext(QGLFormat::defaultFormat()));
        that->d_func()->ownsContext = true;
    }
    return d->qglContext;
}

/*!
    Sets the QGLContext for this window surface to \a context.
*/
void QWSGLWindowSurface::setContext(QGLContext *context)
{
    Q_D(QWSGLWindowSurface);
    if (d->ownsContext) {
        delete d->qglContext;
        d->ownsContext = false;
    }
    d->qglContext = context;
}

QT_END_NAMESPACE
