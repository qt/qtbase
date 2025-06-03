// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QWAYLANDGLCONTEXT_H
#define QWAYLANDGLCONTEXT_H

#include "qwaylandeglinclude_p.h" //must be first

#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtGui/private/qeglplatformcontext_p.h>
#include <qpa/qplatformopenglcontext.h>

QT_BEGIN_NAMESPACE

class QOpenGLShaderProgram;
class QOpenGLTextureCache;

namespace QtWaylandClient {

class QWaylandEglWindow;
class DecorationsBlitter;

class Q_WAYLANDCLIENT_EXPORT QWaylandGLContext : public QEGLPlatformContext
{
public:
    QWaylandGLContext();
    QWaylandGLContext(EGLDisplay eglDisplay, QWaylandDisplay *display, const QSurfaceFormat &format, QPlatformOpenGLContext *share);
    ~QWaylandGLContext();
    void swapBuffers(QPlatformSurface *surface) override;

    bool makeCurrent(QPlatformSurface *surface) override;
    void doneCurrent() override;

    void beginFrame() override;
    void endFrame() override;

    GLuint defaultFramebufferObject(QPlatformSurface *surface) const override;

    QFunctionPointer getProcAddress(const char *procName) override;

protected:
    EGLSurface eglSurfaceForPlatformSurface(QPlatformSurface *surface) override;
    EGLSurface createTemporaryOffscreenSurface() override;
    void destroyTemporaryOffscreenSurface(EGLSurface surface) override;
    void runGLChecks() override;

private:
    QWaylandDisplay *m_display = nullptr;
    EGLContext m_decorationsContext = EGL_NO_CONTEXT;
    DecorationsBlitter *m_blitter = nullptr;
    bool m_supportNonBlockingSwap = true;
    EGLenum m_api;
    wl_surface *m_wlSurface = nullptr;
    wl_egl_window *m_eglWindow = nullptr;
    QWaylandEglWindow *m_currentWindow = nullptr;
    QMetaObject::Connection m_reconnectionWatcher;
    bool m_doneCurrentWorkAround = false;
};

}

QT_END_NAMESPACE

#endif // QWAYLANDGLCONTEXT_H
