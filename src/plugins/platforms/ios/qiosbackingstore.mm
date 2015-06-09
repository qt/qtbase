/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#include <QtGui/private/qwindow_p.h>

#include <QtDebug>

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

QIOSBackingStore::QIOSBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , m_context(new QOpenGLContext)
    , m_device(0)
    , m_fbo(0)
    , m_surface(0)
{
    QSurfaceFormat fmt = window->requestedFormat();
    // Due to sharing QIOSContext redirects our makeCurrent on window() attempts to
    // the global share context. Hence it is essential to have a compatible format.
    fmt.setDepthBufferSize(QSurfaceFormat::defaultFormat().depthBufferSize());
    fmt.setStencilBufferSize(QSurfaceFormat::defaultFormat().stencilBufferSize());

    if (fmt.depthBufferSize() == 0)
        qWarning("No depth in default format, expect rendering errors");

    if (window->surfaceType() == QSurface::RasterSurface)
        window->setSurfaceType(QSurface::OpenGLSurface);

    m_context->setFormat(fmt);
    m_context->setScreen(window->screen());
    Q_ASSERT(QOpenGLContext::globalShareContext());
    m_context->setShareContext(QOpenGLContext::globalShareContext());
    m_context->create();
}

QIOSBackingStore::~QIOSBackingStore()
{
    delete m_fbo;
    delete m_surface;
    delete m_context;
    delete m_device;
}

void QIOSBackingStore::makeCurrent()
{
    QSurface *surface = m_surface ? m_surface : static_cast<QSurface *>(window());
    if (!m_context->makeCurrent(surface))
        qWarning("QIOSBackingStore: makeCurrent() failed");
    if (m_fbo)
        m_fbo->bind();
}

void QIOSBackingStore::beginPaint(const QRegion &)
{
    if (qt_window_private(window())->compositing) {
        if (!m_fbo) {
            delete m_device;
            m_device = 0;
        }
        if (!m_surface) {
            m_surface = new QOffscreenSurface;
            m_surface->setFormat(m_context->format());
            m_surface->create();
        }
        if (!m_context->makeCurrent(m_surface))
            qWarning("QIOSBackingStore: Failed to make offscreen surface current");
        const QSize size = window()->size() * window()->devicePixelRatio();
        if (m_fbo && m_fbo->size() != size) {
            delete m_fbo;
            m_fbo = 0;
        }
        if (!m_fbo)
            m_fbo = new QOpenGLFramebufferObject(size, QOpenGLFramebufferObject::CombinedDepthStencil);
    } else if (m_fbo) {
        delete m_fbo;
        m_fbo = 0;
        delete m_surface;
        m_surface = 0;
        delete m_device;
        m_device = 0;
    }

    makeCurrent();

    if (!m_device)
        m_device = new QIOSPaintDevice(this);
}

void QIOSBackingStore::endPaint()
{
    if (m_fbo) {
        m_fbo->release();
        glFlush();
    }
}

QPaintDevice *QIOSBackingStore::paintDevice()
{
    Q_ASSERT(m_device);

    // Keep paint device size and device pixel ratio in sync with window
    qreal devicePixelRatio = window()->devicePixelRatio();
    m_device->setSize(window()->size() * devicePixelRatio);
    m_device->setDevicePixelRatio(devicePixelRatio);

    return m_device;
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

    m_context->makeCurrent(window);
    m_context->swapBuffers(window);
}

void QIOSBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents);

    // Resizing the backing store would in our case mean resizing the QWindow,
    // as we cheat and use an QOpenGLPaintDevice that we target at the window.
    // That's probably not what the user intended, so we ignore resizes of the
    // backing store and always keep the paint device's size in sync with the
    // window size in beginPaint().

    if (size != window()->size() && !window()->inherits("QWidgetWindow"))
        qWarning() << "QIOSBackingStore needs to have the same size as its window";
}

GLuint QIOSBackingStore::toTexture(const QRegion &dirtyRegion, QSize *textureSize, TextureFlags *flags) const
{
    Q_ASSERT(qt_window_private(window())->compositing);
    Q_UNUSED(dirtyRegion);

    if (flags)
        *flags = TextureFlip;

    if (!m_fbo)
        return 0;

    if (textureSize)
        *textureSize = m_fbo->size();

    return m_fbo->texture();
}

QT_END_NAMESPACE
