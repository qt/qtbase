/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qandroidplatformopenglcontext.h"
#include "qandroidplatformopenglwindow.h"
#include "qandroidplatformintegration.h"
#include "qandroidplatformoffscreensurface.h"

#include <QtEglSupport/private/qeglpbuffer_p.h>

#include <QSurface>
#include <QtGui/private/qopenglcontext_p.h>
#include <QtGui/QOffscreenSurface>

QT_BEGIN_NAMESPACE

QAndroidPlatformOpenGLContext::QAndroidPlatformOpenGLContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display,
                                                             const QVariant &nativeHandle)
    :QEGLPlatformContext(format, share, display, nullptr, nativeHandle)
{
}

void QAndroidPlatformOpenGLContext::swapBuffers(QPlatformSurface *surface)
{
    if (surface->surface()->surfaceClass() == QSurface::Window &&
            static_cast<QAndroidPlatformOpenGLWindow *>(surface)->checkNativeSurface(eglConfig())) {
        QEGLPlatformContext::makeCurrent(surface);
    }

    QEGLPlatformContext::swapBuffers(surface);
}

bool QAndroidPlatformOpenGLContext::makeCurrent(QPlatformSurface *surface)
{
    return QEGLPlatformContext::makeCurrent(surface);
}

EGLSurface QAndroidPlatformOpenGLContext::eglSurfaceForPlatformSurface(QPlatformSurface *surface)
{
    if (surface->surface()->surfaceClass() == QSurface::Window) {
        return static_cast<QAndroidPlatformOpenGLWindow *>(surface)->eglSurface(eglConfig());
    } else {
        auto platformOffscreenSurface = static_cast<QPlatformOffscreenSurface*>(surface);
        if (platformOffscreenSurface->offscreenSurface()->nativeHandle())
            return static_cast<QAndroidPlatformOffscreenSurface *>(surface)->surface();
        else
            return static_cast<QEGLPbuffer *>(surface)->pbuffer();
    }
}

QT_END_NAMESPACE
