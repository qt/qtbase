/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
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

#include "qqnxglbackingstore.h"
#include "qqnxglcontext.h"
#include "qqnxwindow.h"
#include "qqnxscreen.h"

#include <QtGui/qwindow.h>

#include <QtOpenGL/private/qgl_p.h>
#include <QtOpenGL/QGLContext>

#include <QtCore/QDebug>

#include <errno.h>

#ifdef QQNXGLBACKINGSTORE_DEBUG
#define qGLBackingStoreDebug qDebug
#else
#define qGLBackingStoreDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QQnxGLPaintDevice::QQnxGLPaintDevice(QWindow *window)
    : QGLPaintDevice(),
      m_window(0),
      m_glContext(0)
{
    m_window = static_cast<QQnxWindow*>(window->handle());

    // Extract the QPlatformOpenGLContext from the window
    QPlatformOpenGLContext *platformOpenGLContext = m_window->platformOpenGLContext();

    // Convert this to a QGLContext
    m_glContext = QGLContext::fromOpenGLContext(platformOpenGLContext->context());
}

QQnxGLPaintDevice::~QQnxGLPaintDevice()
{
    // Cleanup GL context
    delete m_glContext;
}

QPaintEngine *QQnxGLPaintDevice::paintEngine() const
{
    // Select a paint engine based on configued OpenGL version
    return qt_qgl_paint_engine();
}

QSize QQnxGLPaintDevice::size() const
{
    // Get size of EGL surface
    return m_window->geometry().size();
}


QQnxGLBackingStore::QQnxGLBackingStore(QWindow *window)
    : QPlatformBackingStore(window),
      m_openGLContext(0),
      m_paintDevice(0),
      m_requestedSize(),
      m_size()
{
    qGLBackingStoreDebug() << Q_FUNC_INFO << "w =" << window;

    // Create an OpenGL paint device which in turn creates a QGLContext for us
    m_paintDevice = new QQnxGLPaintDevice(window);
    m_openGLContext = m_paintDevice->context()->contextHandle();
}

QQnxGLBackingStore::~QQnxGLBackingStore()
{
    qGLBackingStoreDebug() << Q_FUNC_INFO << "w =" << window();

    // cleanup OpenGL paint device
    delete m_paintDevice;
}

void QQnxGLBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);
    Q_UNUSED(offset);
    qGLBackingStoreDebug() << Q_FUNC_INFO << "w =" << window;

    // update the display with newly rendered content
    m_openGLContext->swapBuffers(window);
}

void QQnxGLBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents);
    qGLBackingStoreDebug() << Q_FUNC_INFO << "w =" << window() << ", s =" << size;

    // NOTE: defer resizing window buffers until next paint as
    // resize() can be called multiple times before a paint occurs
    m_requestedSize = size;
}

void QQnxGLBackingStore::beginPaint(const QRegion &region)
{
    Q_UNUSED(region);
    qGLBackingStoreDebug() << Q_FUNC_INFO << "w =" << window();

    // resize EGL surface if window surface resized
    if (m_size != m_requestedSize) {
        resizeSurface(m_requestedSize);
    }
}

void QQnxGLBackingStore::endPaint(const QRegion &region)
{
    Q_UNUSED(region);
    qGLBackingStoreDebug() << Q_FUNC_INFO << "w =" << window();
}

void QQnxGLBackingStore::resizeSurface(const QSize &size)
{
    // need to destroy surface so make sure its not current
    bool restoreCurrent = false;
    QQnxGLContext *platformContext = static_cast<QQnxGLContext *>(m_openGLContext->handle());
    if (platformContext->isCurrent()) {
        m_openGLContext->doneCurrent();
        restoreCurrent = true;
    }

    // destroy old EGL surface
    platformContext->destroySurface();

    // resize window's buffers
    static_cast<QQnxWindow*>(window()->handle())->setBufferSize(size);

    // re-create EGL surface with new size
    m_size = size;
    platformContext->createSurface(window()->handle());

    // make context current again
    if (restoreCurrent) {
        m_openGLContext->makeCurrent(window());
    }
}

QT_END_NAMESPACE
