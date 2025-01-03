// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDPLATFORMOPENGLCONTEXT_H
#define QANDROIDPLATFORMOPENGLCONTEXT_H

#include <QtGui/private/qeglplatformcontext_p.h>

QT_BEGIN_NAMESPACE

class QAndroidPlatformOpenGLContext : public QEGLPlatformContext
{
public:
    using QEGLPlatformContext::QEGLPlatformContext;
    QAndroidPlatformOpenGLContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display);
    void swapBuffers(QPlatformSurface *surface) override;
    bool makeCurrent(QPlatformSurface *surface) override;

private:
    EGLSurface eglSurfaceForPlatformSurface(QPlatformSurface *surface) override;
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMOPENGLCONTEXT_H
