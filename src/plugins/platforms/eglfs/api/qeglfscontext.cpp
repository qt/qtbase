// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeglfsglobal_p.h"
#include <QtGui/QSurface>
#include <QtGui/private/qeglconvenience_p.h>
#include <QtGui/private/qeglpbuffer_p.h>

#include "qeglfscontext_p.h"
#include "qeglfswindow_p.h"
#include "qeglfshooks_p.h"
#include "qeglfscursor_p.h"

QT_BEGIN_NAMESPACE

QEglFSContext::QEglFSContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display,
                             EGLConfig *config)
    : QEGLPlatformContext(format, share, display, config,
                          qt_egl_device_integration()->supportsSurfacelessContexts() ? Flags() : QEGLPlatformContext::NoSurfaceless)
{
}

EGLSurface QEglFSContext::eglSurfaceForPlatformSurface(QPlatformSurface *surface)
{
    if (surface->surface()->surfaceClass() == QSurface::Window) {

        QEglFSWindow *w = static_cast<QEglFSWindow *>(surface);
        EGLSurface s = w->surface();
        if (s == EGL_NO_SURFACE) {
            w->resetSurface();
            s = w->surface();
        }
        return s;

    } else {
        return static_cast<QEGLPbuffer *>(surface)->pbuffer();
    }
}

EGLSurface QEglFSContext::createTemporaryOffscreenSurface()
{
    if (qt_egl_device_integration()->supportsPBuffers())
        return QEGLPlatformContext::createTemporaryOffscreenSurface();

    if (!m_tempWindow) {
        m_tempWindow = qt_egl_device_integration()->createNativeOffscreenWindow(format());
        if (!m_tempWindow) {
            qWarning("QEglFSContext: Failed to create temporary native window");
            return EGL_NO_SURFACE;
        }
    }
    EGLConfig config = q_configFromGLFormat(eglDisplay(), format());
    return eglCreateWindowSurface(eglDisplay(), config, m_tempWindow, nullptr);
}

void QEglFSContext::destroyTemporaryOffscreenSurface(EGLSurface surface)
{
    if (qt_egl_device_integration()->supportsPBuffers()) {
        QEGLPlatformContext::destroyTemporaryOffscreenSurface(surface);
    } else {
        eglDestroySurface(eglDisplay(), surface);
        qt_egl_device_integration()->destroyNativeWindow(m_tempWindow);
        m_tempWindow = 0;
    }
}

void QEglFSContext::runGLChecks()
{
    // Note that even though there is an EGL context current here,
    // QOpenGLContext and QOpenGLFunctions are not yet usable at this stage.
    const char *renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    // Be nice and warn about a common source of confusion.
    if (renderer && strstr(renderer, "llvmpipe"))
        qWarning("Running on a software rasterizer (LLVMpipe), expect limited performance.");
}

void QEglFSContext::swapBuffers(QPlatformSurface *surface)
{
    // draw the cursor
    if (surface->surface()->surfaceClass() == QSurface::Window) {
        QPlatformWindow *window = static_cast<QPlatformWindow *>(surface);
        if (QEglFSCursor *cursor = qobject_cast<QEglFSCursor *>(window->screen()->cursor()))
            cursor->paintOnScreen();
    }

    qt_egl_device_integration()->waitForVSync(surface);
    QEGLPlatformContext::swapBuffers(surface);
    qt_egl_device_integration()->presentBuffer(surface);
}

QT_END_NAMESPACE
