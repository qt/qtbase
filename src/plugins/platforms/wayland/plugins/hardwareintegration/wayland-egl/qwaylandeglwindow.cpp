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
        QWriteLocker lock(&m_bufferSizeLock);
        m_bufferSize = sizeWithMargins;
    }

    QMutexLocker lock (&m_eglSurfaceLock);
    updateSurface(false);
}

void QWaylandEglWindow::updateSurface(bool create)
{
    // eglSurfaceLock should be locked before calling this method

    QSize sizeWithMargins;
    {
        QReadLocker lock(&m_bufferSizeLock);
        sizeWithMargins = m_bufferSize;
    }

    // wl_egl_windows must have both width and height > 0
    // mesa's egl returns NULL if we try to create a, invalid wl_egl_window, however not all EGL
    // implementations may do that, so check the size ourself. Besides, we must deal with resizing
    // a valid window to 0x0, which would make it invalid. Hence, destroy it.
    if (sizeWithMargins.isEmpty()) {
        if (m_eglSurface) {
            eglDestroySurface(m_clientBufferIntegration->eglDisplay(), m_eglSurface);
            m_eglSurface = 0;
        }
        if (m_waylandEglWindow) {
            wl_egl_window_destroy(m_waylandEglWindow);
            m_waylandEglWindow = 0;
        }
        mOffset = QPoint();
    } else {
        QReadLocker locker(&mSurfaceLock);
        if (m_waylandEglWindow) {
            int current_width = 0;
            int current_height = 0;
            static bool disableResizeCheck = qgetenv("QT_WAYLAND_DISABLE_RESIZECHECK").toInt();

            if (!disableResizeCheck) {
                wl_egl_window_get_attached_size(m_waylandEglWindow, &current_width, &current_height);
            }
            if (disableResizeCheck || (current_width != sizeWithMargins.width() || current_height != sizeWithMargins.height()) || m_requestedSize != sizeWithMargins) {
                wl_egl_window_resize(m_waylandEglWindow, sizeWithMargins.width(), sizeWithMargins.height(), mOffset.x(), mOffset.y());
                m_requestedSize = sizeWithMargins;
                mOffset = QPoint();

                m_resize = true;
            }
        } else if (create && mSurface) {
            wl_egl_window *eglWindow = wl_egl_window_create(mSurface->object(), sizeWithMargins.width(), sizeWithMargins.height());
            if (Q_UNLIKELY(!eglWindow)) {
                qCWarning(lcQpaWayland, "Could not create wl_egl_window with size %dx%d\n", sizeWithMargins.width(), sizeWithMargins.height());
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
            m_requestedSize = sizeWithMargins;
        }
    }
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

EGLSurface QWaylandEglWindow::eglSurface() const
{
    return m_eglSurface;
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
