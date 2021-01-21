/****************************************************************************
**
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

#ifndef QXCBEGLCONTEXT_H
#define QXCBEGLCONTEXT_H

#include "qxcbeglwindow.h"
#include <QtEglSupport/private/qeglplatformcontext_p.h>
#include <QtEglSupport/private/qeglpbuffer_p.h>
#include <QtPlatformHeaders/QEGLNativeContext>

QT_BEGIN_NAMESPACE

class QXcbEglContext : public QEGLPlatformContext
{
public:
    QXcbEglContext(const QSurfaceFormat &glFormat, QPlatformOpenGLContext *share,
                           EGLDisplay display, const QVariant &nativeHandle)
        : QEGLPlatformContext(glFormat, share, display, nullptr, nativeHandle)
    {
    }

    void swapBuffers(QPlatformSurface *surface)
    {
        QEGLPlatformContext::swapBuffers(surface);
        if (surface->surface()->surfaceClass() == QSurface::Window) {
            QXcbWindow *platformWindow = static_cast<QXcbWindow *>(surface);
            // OpenGL context might be bound to a non-gui thread use QueuedConnection to sync
            // the window from the platformWindow's thread as QXcbWindow is no QObject, an
            // event is sent to QXcbConnection. (this is faster than a metacall)
            if (platformWindow->needsSync())
                platformWindow->postSyncWindowRequest();
        }
    }

    bool makeCurrent(QPlatformSurface *surface)
    {
        return QEGLPlatformContext::makeCurrent(surface);
    }

    void doneCurrent()
    {
        QEGLPlatformContext::doneCurrent();
    }

    EGLSurface eglSurfaceForPlatformSurface(QPlatformSurface *surface)
    {
        if (surface->surface()->surfaceClass() == QSurface::Window)
            return static_cast<QXcbEglWindow *>(surface)->eglSurface();
        else
            return static_cast<QEGLPbuffer *>(surface)->pbuffer();
    }

    QVariant nativeHandle() const {
        return QVariant::fromValue<QEGLNativeContext>(QEGLNativeContext(eglContext(), eglDisplay()));
    }
};

QT_END_NAMESPACE
#endif //QXCBEGLCONTEXT_H

