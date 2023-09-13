// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandeglwindow_p.h"

#include <QtWaylandClient/private/qwaylandscreen_p.h>
#include <QtWaylandClient/private/qwaylandsurface_p.h>
#include "qwaylandglcontext_p.h"

#include <QtGui/private/qeglconvenience_p.h>

#include <QDebug>
#include <QtGui/QWindow>
#include <qpa/qwindowsysteminterface.h>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandEglWindow::QWaylandEglWindow(QWindow *window, QWaylandDisplay *display)
    : QWaylandWindow(window, display)
    , m_clientBufferIntegration(static_cast<QWaylandEglClientBufferIntegration *>(mDisplay->clientBufferIntegration()))
{
    connect(display, &QWaylandDisplay::connected, this, [this] {
        m_clientBufferIntegration = static_cast<QWaylandEglClientBufferIntegration *>(
                mDisplay->clientBufferIntegration());
    });
    ensureSize();
}

QWaylandEglWindow::~QWaylandEglWindow()
{
    if (m_eglSurface) {
        eglDestroySurface(m_clientBufferIntegration->eglDisplay(), m_eglSurface);
        m_eglSurface = 0;
    }

    if (m_waylandEglWindow)
        wl_egl_window_destroy(m_waylandEglWindow);

    delete m_contentFBO;
}

QWaylandWindow::WindowType QWaylandEglWindow::windowType() const
{
    return QWaylandWindow::Egl;
}

void QWaylandEglWindow::ensureSize()
{
    // this is always called on the main thread
    QRect rect = geometry();
    QMargins margins = clientSideMargins();
    QSize sizeWithMargins = (rect.size() + QSize(margins.left() + margins.right(), margins.top() + margins.bottom())) * scale();
    {
        // keep a cache of the size that is safely accessible from the render thread
        QWriteLocker lock(&m_bufferSizeLock);
        m_bufferSize = sizeWithMargins;
    }
}

QSize QWaylandEglWindow::bufferSize() const
{
    QReadLocker lock(&m_bufferSizeLock);
    return m_bufferSize;
}

void QWaylandEglWindow::updateSurfaceSize(const QSize &initialSize)
{
    // potentially called from a render thread
    // eglSurfaceLock should be locked before calling this method

    if (!m_waylandEglWindow)
        return;

    int current_width = 0;
    int current_height = 0;
    static bool disableResizeCheck = qgetenv("QT_WAYLAND_DISABLE_RESIZECHECK").toInt();

    if (!disableResizeCheck) {
        wl_egl_window_get_attached_size(m_waylandEglWindow, &current_width, &current_height);
    }
    if (disableResizeCheck || (current_width != initialSize.width() || current_height != initialSize.height()) || m_appliedBufferSize != initialSize) {
        wl_egl_window_resize(m_waylandEglWindow, initialSize.width(), initialSize.height(), mOffset.x(), mOffset.y());
        m_appliedBufferSize = initialSize;
        mOffset = QPoint();
        m_resize = true;
    }
}

void QWaylandEglWindow::createSurface(const QSize &initialSize)
{
    // potentially called from a render thread
    // eglSurfaceLock should be locked before calling this method

    QReadLocker locker(&mSurfaceLock);
    if (mSurface) {
        wl_egl_window *eglWindow = wl_egl_window_create(mSurface->object(), initialSize.width(), initialSize.height());
        if (Q_UNLIKELY(!eglWindow)) {
            qCWarning(lcQpaWayland, "Could not create wl_egl_window with size %dx%d\n", initialSize.width(), initialSize.height());
            return;
        }

        QSurfaceFormat fmt = window()->requestedFormat();
        if (mDisplay->supportsWindowDecoration())
            fmt.setAlphaBufferSize(8);
        EGLConfig eglConfig = q_configFromGLFormat(m_clientBufferIntegration->eglDisplay(), fmt);
        setFormat(q_glFormatFromConfig(m_clientBufferIntegration->eglDisplay(), eglConfig, fmt));

        EGLSurface eglSurface = eglCreateWindowSurface(m_clientBufferIntegration->eglDisplay(), eglConfig, (EGLNativeWindowType) eglWindow, 0);
        if (Q_UNLIKELY(eglSurface == EGL_NO_SURFACE)) {
            qCWarning(lcQpaWayland, "Could not create EGL surface (EGL error 0x%x)\n", eglGetError());
            wl_egl_window_destroy(eglWindow);
            return;
        }

        m_waylandEglWindow = eglWindow;
        m_eglSurface = eglSurface;
        m_appliedBufferSize = initialSize;
    }
}

EGLSurface QWaylandEglWindow::eglSurface() const
{
    return m_eglSurface;
}

QRect QWaylandEglWindow::contentsRect() const
{
    QRect r = geometry();
    QMargins m = clientSideMargins();
    return QRect(m.left(), m.bottom(), r.width(), r.height());
}

void QWaylandEglWindow::invalidateSurface()
{
    QMutexLocker lock (&m_eglSurfaceLock);

    if (m_eglSurface) {
        eglDestroySurface(m_clientBufferIntegration->eglDisplay(), m_eglSurface);
        m_eglSurface = 0;
    }
    if (m_waylandEglWindow) {
        wl_egl_window_destroy(m_waylandEglWindow);
        m_waylandEglWindow = nullptr;
    }
    delete m_contentFBO;
    m_contentFBO = nullptr;
}

QMutex* QWaylandEglWindow::eglSurfaceLock()
{
    return &m_eglSurfaceLock;
}
GLuint QWaylandEglWindow::contentFBO() const
{
    if (!decoration())
        return 0;

    if (m_resize || !m_contentFBO) {
        QOpenGLFramebufferObject *old = m_contentFBO;
        QSize fboSize = geometry().size() * scale();
        m_contentFBO = new QOpenGLFramebufferObject(fboSize.width(), fboSize.height(), QOpenGLFramebufferObject::CombinedDepthStencil);

        delete old;
        m_resize = false;
    }

    return m_contentFBO->handle();
}

GLuint QWaylandEglWindow::contentTexture() const
{
    return m_contentFBO->texture();
}

void QWaylandEglWindow::bindContentFBO()
{
    if (decoration()) {
        contentFBO();
        m_contentFBO->bind();
    }
}

}

QT_END_NAMESPACE

#include "moc_qwaylandeglwindow_p.cpp"
