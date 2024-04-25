// Copyright (C) 2011 - 2013 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxglcontext.h"
#include "qqnxintegration.h"
#include "qqnxscreen.h"
#include "qqnxeglwindow.h"

#include "private/qeglconvenience_p.h"

#include <QtCore/QDebug>
#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>

#include <dlfcn.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaGLContext, "qt.qpa.glcontext");

static QEGLPlatformContext::Flags makeFlags()
{
    QEGLPlatformContext::Flags result = {};

    if (!QQnxIntegration::instance()->options().testFlag(QQnxIntegration::SurfacelessEGLContext))
        result |= QEGLPlatformContext::NoSurfaceless;

    return result;
}

QQnxGLContext::QQnxGLContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share)
    : QEGLPlatformContext(format, share, QQnxIntegration::instance()->eglDisplay(), nullptr,
                          makeFlags())
{
}

QQnxGLContext::~QQnxGLContext()
{
}

EGLSurface QQnxGLContext::eglSurfaceForPlatformSurface(QPlatformSurface *surface)
{
    QQnxEglWindow *window = static_cast<QQnxEglWindow *>(surface);
    window->ensureInitialized(this);
    return window->surface();
}

bool QQnxGLContext::makeCurrent(QPlatformSurface *surface)
{
    qCDebug(lcQpaGLContext) << Q_FUNC_INFO;
    return QEGLPlatformContext::makeCurrent(surface);
}

void QQnxGLContext::swapBuffers(QPlatformSurface *surface)
{
    qCDebug(lcQpaGLContext) << Q_FUNC_INFO;

    QEGLPlatformContext::swapBuffers(surface);

    QQnxEglWindow *platformWindow = static_cast<QQnxEglWindow*>(surface);
    platformWindow->windowPosted();
}

void QQnxGLContext::doneCurrent()
{
    QEGLPlatformContext::doneCurrent();
}

QT_END_NAMESPACE
