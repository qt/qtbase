// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qandroidplatformopenglcontext.h"
#include "qandroidplatformoffscreensurface.h"
#include "qandroidplatformopenglwindow.h"

#include <QtGui/QOffscreenSurface>
#include <QtGui/QSurface>
#include <QtGui/private/qopenglcontext_p.h>
#include <QtGui/private/qeglpbuffer_p.h>

QT_BEGIN_NAMESPACE

QAndroidPlatformOpenGLContext::QAndroidPlatformOpenGLContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display)
    : QEGLPlatformContext(format, share, display, nullptr)
{
}

QAndroidPlatformOpenGLContext::~QAndroidPlatformOpenGLContext()
{
    if (m_fallbackPbuffer != EGL_NO_SURFACE)
        eglDestroySurface(eglDisplay(), m_fallbackPbuffer);
}

EGLSurface QAndroidPlatformOpenGLContext::ensureFallbackPbuffer()
{
    if (m_fallbackPbuffer != EGL_NO_SURFACE)
        return m_fallbackPbuffer;
    static const EGLint pbufferAttribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    m_fallbackPbuffer = eglCreatePbufferSurface(eglDisplay(), eglConfig(), pbufferAttribs);
    if (Q_UNLIKELY(m_fallbackPbuffer == EGL_NO_SURFACE))
        qWarning("EGL Error: could not create pbuffer fallback surface: 0x%x", eglGetError());
    return m_fallbackPbuffer;
}

void QAndroidPlatformOpenGLContext::swapBuffers(QPlatformSurface *surface)
{
    if (surface->surface()->surfaceClass() != QSurface::Window) {
        QEGLPlatformContext::swapBuffers(surface);
        return;
    }

    QAndroidPlatformOpenGLWindow *window = static_cast<QAndroidPlatformOpenGLWindow *>(surface);
    // No Surface yet, so there is nothing visible to swap.
    if (window->eglSurface(eglConfig()) == EGL_NO_SURFACE)
        return;

    QEGLPlatformContext::makeCurrent(surface);
    QEGLPlatformContext::swapBuffers(surface);
}

bool QAndroidPlatformOpenGLContext::makeCurrent(QPlatformSurface *surface)
{
    return QEGLPlatformContext::makeCurrent(surface);
}

EGLSurface QAndroidPlatformOpenGLContext::eglSurfaceForPlatformSurface(QPlatformSurface *surface)
{
    if (surface->surface()->surfaceClass() == QSurface::Window) {
        EGLSurface eglSurface =
                static_cast<QAndroidPlatformOpenGLWindow *>(surface)->eglSurface(eglConfig());
        // No Surface from Android yet. Use a 1x1 pbuffer so makeCurrent()
        // succeeds, a later makeCurrent() binds the real surface.
        if (eglSurface == EGL_NO_SURFACE)
            eglSurface = ensureFallbackPbuffer();

        return eglSurface;
    }

    if (auto *platformOffscreenSurface = dynamic_cast<QAndroidPlatformOffscreenSurface *>(surface))
        return platformOffscreenSurface->surface();

    return static_cast<QEGLPbuffer *>(surface)->pbuffer();
}

QT_END_NAMESPACE
