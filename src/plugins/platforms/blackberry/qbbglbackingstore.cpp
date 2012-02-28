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

#include "qbbglbackingstore.h"
#include "qbbglcontext.h"
#include "qbbwindow.h"
#include "qbbscreen.h"

#include <QtGui/qwindow.h>

#include <QtOpenGL/private/qgl_p.h>
#include <QtOpenGL/QGLContext>

#include <QtCore/QDebug>

#include <errno.h>

QT_BEGIN_NAMESPACE

QBBGLPaintDevice::QBBGLPaintDevice(QWindow *window)
    : QGLPaintDevice(),
      m_window(0),
      m_glContext(0)
{
    m_window = static_cast<QBBWindow*>(window->handle());

    // Extract the QPlatformOpenGLContext from the window
    QPlatformOpenGLContext *platformOpenGLContext = m_window->platformOpenGLContext();

    // Convert this to a QGLContext
    m_glContext = QGLContext::fromOpenGLContext(platformOpenGLContext->context());
}

QBBGLPaintDevice::~QBBGLPaintDevice()
{
    // Cleanup GL context
    delete m_glContext;
}

QPaintEngine *QBBGLPaintDevice::paintEngine() const
{
    // Select a paint engine based on configued OpenGL version
    return qt_qgl_paint_engine();
}

QSize QBBGLPaintDevice::size() const
{
    // Get size of EGL surface
    return m_window->geometry().size();
}


QBBGLBackingStore::QBBGLBackingStore(QWindow *window)
    : QPlatformBackingStore(window),
      m_openGLContext(0),
      m_paintDevice(0),
      m_requestedSize(),
      m_size()
{
#if defined(QBBGLBACKINGSTORE_DEBUG)
    qDebug() << "QBBGLBackingStore::QBBGLBackingStore - w=" << window;
#endif

    // Create an OpenGL paint device which in turn creates a QGLContext for us
    m_paintDevice = new QBBGLPaintDevice(window);
    m_openGLContext = m_paintDevice->context()->contextHandle();
}

QBBGLBackingStore::~QBBGLBackingStore()
{
#if defined(QBBGLBACKINGSTORE_DEBUG)
    qDebug() << "QBBGLBackingStore::~QBBGLBackingStore - w=" << window();
#endif

    // cleanup OpenGL paint device
    delete m_paintDevice;
}

void QBBGLBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);
    Q_UNUSED(offset);

#if defined(QBBGLBACKINGSTORE_DEBUG)
    qDebug() << "QBBGLBackingStore::flush - w=" << window;
#endif

    // update the display with newly rendered content
    m_openGLContext->swapBuffers(window);
}

void QBBGLBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents);
#if defined(QBBGLBACKINGSTORE_DEBUG)
    qDebug() << "QBBGLBackingStore::resize - w=" << window() << ", s=" << size;
#endif
    // NOTE: defer resizing window buffers until next paint as
    // resize() can be called multiple times before a paint occurs
    m_requestedSize = size;
}

void QBBGLBackingStore::beginPaint(const QRegion &region)
{
    Q_UNUSED(region);

#if defined(QBBGLBACKINGSTORE_DEBUG)
    qDebug() << "QBBGLBackingStore::beginPaint - w=" << window();
#endif

    // resize EGL surface if window surface resized
    if (m_size != m_requestedSize) {
        resizeSurface(m_requestedSize);
    }
}

void QBBGLBackingStore::endPaint(const QRegion &region)
{
    Q_UNUSED(region);
#if defined(QBBGLBACKINGSTORE_DEBUG)
    qDebug() << "QBBGLBackingStore::endPaint - w=" << window();
#endif
}

void QBBGLBackingStore::resizeSurface(const QSize &size)
{
    // need to destroy surface so make sure its not current
    bool restoreCurrent = false;
    QBBGLContext *platformContext = static_cast<QBBGLContext *>(m_openGLContext->handle());
    if (platformContext->isCurrent()) {
        m_openGLContext->doneCurrent();
        restoreCurrent = true;
    }

    // destroy old EGL surface
    platformContext->destroySurface();

    // resize window's buffers
    static_cast<QBBWindow*>(window()->handle())->setBufferSize(size);

    // re-create EGL surface with new size
    m_size = size;
    platformContext->createSurface(window()->handle());

    // make context current again
    if (restoreCurrent) {
        m_openGLContext->makeCurrent(window());
    }
}

QT_END_NAMESPACE
