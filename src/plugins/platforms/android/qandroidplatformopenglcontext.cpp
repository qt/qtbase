// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qandroidplatformopenglcontext.h"
#include "qandroidplatformopenglwindow.h"
#include "qandroidplatformintegration.h"
#include "qandroidplatformoffscreensurface.h"

#include <QtGui/private/qeglpbuffer_p.h>

#include <QSurface>
#include <QtGui/private/qopenglcontext_p.h>
#include <QtGui/QOffscreenSurface>

QT_BEGIN_NAMESPACE

QAndroidPlatformOpenGLContext::QAndroidPlatformOpenGLContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display)
    : QEGLPlatformContext(format, share, display, nullptr)
{
}

void QAndroidPlatformOpenGLContext::swapBuffers(QPlatformSurface *surface)
{
    if (surface->surface()->surfaceClass() != QSurface::Window) {
        QEGLPlatformContext::swapBuffers(surface);
        return;
    }

    QAndroidPlatformOpenGLWindow *window = static_cast<QAndroidPlatformOpenGLWindow *>(surface);
    if (window->eglSurface(eglConfig()) == EGL_NO_SURFACE)
        return;

    QEGLPlatformContext::makeCurrent(surface);
    QEGLPlatformContext::swapBuffers(surface);
}

bool QAndroidPlatformOpenGLContext::makeCurrent(QPlatformSurface *surface)
{
    if (surface->surface()->surfaceClass() != QSurface::Window)
        return QEGLPlatformContext::makeCurrent(surface);

    QAndroidPlatformOpenGLWindow *window = static_cast<QAndroidPlatformOpenGLWindow *>(surface);
    // Has the Surface been destroyed?
    if (window->eglSurface(eglConfig()) == EGL_NO_SURFACE) {
        qWarning() << "makeCurrent(): no EGLSurface, likely Surface destroyed by Android.";
        return false;
    }
    return QEGLPlatformContext::makeCurrent(surface);
}

EGLSurface QAndroidPlatformOpenGLContext::eglSurfaceForPlatformSurface(QPlatformSurface *surface)
{
    if (surface->surface()->surfaceClass() == QSurface::Window) {
        return static_cast<QAndroidPlatformOpenGLWindow *>(surface)->eglSurface(eglConfig());
    } else {
        if (auto *platformOffscreenSurface = dynamic_cast<QAndroidPlatformOffscreenSurface *>(surface))
            return platformOffscreenSurface->surface();
        else
            return static_cast<QEGLPbuffer *>(surface)->pbuffer();
    }
}

QT_END_NAMESPACE
