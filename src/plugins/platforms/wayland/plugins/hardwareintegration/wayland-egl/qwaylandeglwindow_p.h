// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#pragma once

#include <QtWaylandClient/private/qwaylandwindow_p.h>
#include "qwaylandeglclientbufferintegration_p.h"

QT_BEGIN_NAMESPACE

class QOpenGLFramebufferObject;

namespace QtWaylandClient {

class QWaylandGLContext;

class Q_WAYLANDCLIENT_EXPORT QWaylandEglWindow : public QWaylandWindow
{
    Q_OBJECT
public:
    QWaylandEglWindow(QWindow *window, QWaylandDisplay *display);
    ~QWaylandEglWindow();
    WindowType windowType() const override;
    void ensureSize() override;
    QSize bufferSize() const;

    void createSurface(const QSize &initialSize);
    void updateSurfaceSize(const QSize &bufferSize);
    QRect contentsRect() const;

    EGLSurface eglSurface() const;
    GLuint contentFBO() const;
    GLuint contentTexture() const;
    bool needToUpdateContentFBO() const { return decoration() && (m_resize || !m_contentFBO); }

    void bindContentFBO();

    void invalidateSurface() override;

    QMutex* eglSurfaceLock();

private:
    QWaylandEglClientBufferIntegration *m_clientBufferIntegration = nullptr;
    struct wl_egl_window *m_waylandEglWindow = nullptr;

    // Locks any manipulation of the eglSurface size
    QMutex m_eglSurfaceLock;
    EGLSurface m_eglSurface = EGL_NO_SURFACE;
    mutable bool m_resize = false;
    mutable QOpenGLFramebufferObject *m_contentFBO = nullptr;

    // Size of the buffer used by QWaylandWindow
    // This is always written to from the main thread, potentially read from the rendering thread
    mutable QReadWriteLock m_bufferSizeLock;
    QSize m_bufferSize;

    // Size used in the last call to wl_egl_window_resize, only updated from the rendering thread
    QSize m_appliedBufferSize;
};

}

QT_END_NAMESPACE
