// Copyright (C) 2011 - 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QQNXGLCONTEXT_H
#define QQNXGLCONTEXT_H

#include <qpa/qplatformopenglcontext.h>
#include <QtGui/QSurfaceFormat>
#include <QtCore/QAtomicInt>
#include <QtCore/QSize>

#include <EGL/egl.h>
#include <QtGui/private/qeglplatformcontext_p.h>

QT_BEGIN_NAMESPACE

class QQnxWindow;

class QQnxGLContext : public QEGLPlatformContext
{
public:
    QQnxGLContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share);
    virtual ~QQnxGLContext();

    bool makeCurrent(QPlatformSurface *surface) override;
    void swapBuffers(QPlatformSurface *surface) override;
    void doneCurrent() override;

protected:
    EGLSurface eglSurfaceForPlatformSurface(QPlatformSurface *surface) override;
};

QT_END_NAMESPACE

#endif // QQNXGLCONTEXT_H
