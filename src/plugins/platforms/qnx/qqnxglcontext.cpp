/***************************************************************************
**
** Copyright (C) 2011 - 2013 BlackBerry Limited. All rights reserved.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qqnxglcontext.h"
#include "qqnxintegration.h"
#include "qqnxscreen.h"
#include "qqnxeglwindow.h"

#include "private/qeglconvenience_p.h"

#include <QtCore/QDebug>
#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>

#include <dlfcn.h>

#if defined(QQNXGLCONTEXT_DEBUG)
#define qGLContextDebug qDebug
#else
#define qGLContextDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

static QEGLPlatformContext::Flags makeFlags()
{
    QEGLPlatformContext::Flags result = 0;

    if (!QQnxIntegration::instance()->options().testFlag(QQnxIntegration::SurfacelessEGLContext))
        result |= QEGLPlatformContext::NoSurfaceless;

    return result;
}

QQnxGLContext::QQnxGLContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share)
    : QEGLPlatformContext(format, share, QQnxIntegration::instance()->eglDisplay(), 0, QVariant(),
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
    qGLContextDebug();
    return QEGLPlatformContext::makeCurrent(surface);
}

void QQnxGLContext::swapBuffers(QPlatformSurface *surface)
{
    qGLContextDebug();

    QEGLPlatformContext::swapBuffers(surface);

    QQnxEglWindow *platformWindow = static_cast<QQnxEglWindow*>(surface);
    platformWindow->windowPosted();
}

void QQnxGLContext::doneCurrent()
{
    QEGLPlatformContext::doneCurrent();
}

QT_END_NAMESPACE
