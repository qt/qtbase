/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qiosbackingstore.h"
#include "qioswindow.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QOpenGLFramebufferObject>
#include <QtGui/QOffscreenSurface>
#include <QtGui/qpainter.h>
#include <QtGui/private/qwindow_p.h>

#include <QtDebug>

QT_BEGIN_NAMESPACE

class QIOSPaintDevice : public QOpenGLPaintDevice
{
public:
    QIOSPaintDevice(QIOSBackingStore *backingStore) : m_backingStore(backingStore) { }
    void ensureActiveTarget() Q_DECL_OVERRIDE;

private:
    QIOSBackingStore *m_backingStore;
};

void QIOSPaintDevice::ensureActiveTarget()
{
    m_backingStore->makeCurrent();
}

/*!
    \class QIOSBackingStore

    \brief The QPlatformBackingStore on iOS.

    QBackingStore enables the use of QPainter to paint on a QWindow, as opposed
    to rendering to a QWindow through the use of OpenGL with QOpenGLContext.

    Historically, the iOS port initially implemented the backing store by using
    an QOpenGLPaintDevice as its paint device, triggering the use of the OpenGL
    paint engine for QPainter based drawing. This was due to raster drawing
    operations being too slow when not being NEON-optimized, and got the port
    up and running quickly.

    As of 3e892e4a97, released in Qt 5.7, the backing store now uses a QImage,
    for its paint device, giving normal raster-based QPainter operations, and
    enabling features such as antialiased drawing.

    To account for regressions in performance, the old code path is still
    available by setting the surface type of the QWindow to OpenGLSurface.
    This surface type is normally used when rendering though QOpenGLContext,
    but will in the case of QIOSBackingStore trigger the old OpenGL based
    painter.

    This fallback path is not too intrusive, as the QImage based path still
    uses OpenGL to composite the image at flush() time using composeAndFlush.
*/
QIOSBackingStore::QIOSBackingStore(QWindow *window)
    : QRasterBackingStore(window)
    , m_context(new QOpenGLContext)
    , m_glDevice(nullptr)
{
    QSurfaceFormat fmt = window->requestedFormat();

    // Due to sharing QIOSContext redirects our makeCurrent on window() attempts to
    // the global share context. Hence it is essential to have a compatible format.
    fmt.setDepthBufferSize(QSurfaceFormat::defaultFormat().depthBufferSize());
    fmt.setStencilBufferSize(QSurfaceFormat::defaultFormat().stencilBufferSize());

    if (fmt.depthBufferSize() == 0)
        qWarning("No depth in default format, expect rendering errors");

    // We use the surface both for raster operations and for GL drawing (when
    // we blit the raster image), so the type needs to cover both use cases.
    if (window->surfaceType() == QSurface::RasterSurface)
        window->setSurfaceType(QSurface::RasterGLSurface);

    m_context->setFormat(fmt);
    m_context->setScreen(window->screen());
    Q_ASSERT(QOpenGLContext::globalShareContext());
    m_context->setShareContext(QOpenGLContext::globalShareContext());
    m_context->create();
}

QIOSBackingStore::~QIOSBackingStore()
{
    delete m_context;
    delete m_glDevice;
}

void QIOSBackingStore::makeCurrent()
{
    if (!m_context->makeCurrent(window()))
        qWarning("QIOSBackingStore: makeCurrent() failed");
}

void QIOSBackingStore::beginPaint(const QRegion &region)
{
    makeCurrent();

    if (!m_glDevice)
        m_glDevice = new QIOSPaintDevice(this);

    if (window()->surfaceType() == QSurface::RasterGLSurface)
        QRasterBackingStore::beginPaint(region);
}

void QIOSBackingStore::endPaint()
{
}

QPaintDevice *QIOSBackingStore::paintDevice()
{
    Q_ASSERT(m_glDevice);

    // Keep paint device size and device pixel ratio in sync with window
    qreal devicePixelRatio = window()->devicePixelRatio();
    m_glDevice->setSize(window()->size() * devicePixelRatio);
    m_glDevice->setDevicePixelRatio(devicePixelRatio);

    if (window()->surfaceType() == QSurface::RasterGLSurface)
        return QRasterBackingStore::paintDevice();
    else
        return m_glDevice;
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

    if (window->surfaceType() == QSurface::RasterGLSurface) {
        static QPlatformTextureList emptyTextureList;
        composeAndFlush(window, region, offset, &emptyTextureList, m_context, false);
    } else {
        m_context->makeCurrent(window);
        m_context->swapBuffers(window);
    }
}

void QIOSBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents);

    if (window()->surfaceType() == QSurface::OpenGLSurface) {
        // Resizing the backing store would in this case mean resizing the QWindow,
        // as we use an QOpenGLPaintDevice that we target at the window. That's
        // probably not what the user intended, so we ignore resizes of the backing
        // store and always keep the paint device's size in sync with the window
        // size in beginPaint().

        if (size != window()->size() && !window()->inherits("QWidgetWindow"))
            qWarning("QIOSBackingStore needs to have the same size as its window");

        return;
    }

    QRasterBackingStore::resize(size, staticContents);
}

QT_END_NAMESPACE
