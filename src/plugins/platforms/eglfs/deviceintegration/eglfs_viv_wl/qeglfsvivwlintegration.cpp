// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeglfsvivwlintegration.h"
#include <EGL/eglvivante.h>
#include <QDebug>

#include <wayland-server.h>

QT_BEGIN_NAMESPACE

void QEglFSVivWaylandIntegration::platformInit()
{
    QEglFSDeviceIntegration::platformInit();

    int width, height;

    bool multiBufferNotEnabledYet = qEnvironmentVariableIsEmpty("FB_MULTI_BUFFER");
    bool multiBuffer = qEnvironmentVariableIsEmpty("QT_EGLFS_IMX6_NO_FB_MULTI_BUFFER");
    if (multiBufferNotEnabledYet && multiBuffer) {
        qWarning() << "QEglFSVivWaylandIntegration will set environment variable FB_MULTI_BUFFER=2 to enable double buffering and vsync.\n"
                   << "If this is not desired, you can override this via: export QT_EGLFS_IMX6_NO_FB_MULTI_BUFFER=1";
        qputenv("FB_MULTI_BUFFER", "2");
    }

    mWaylandDisplay = wl_display_create();
    mNativeDisplay = static_cast<EGLNativeDisplayType>(fbGetDisplay(mWaylandDisplay));
    fbGetDisplayGeometry(mNativeDisplay, &width, &height);
    mScreenSize.setHeight(height);
    mScreenSize.setWidth(width);
}

void QEglFSVivWaylandIntegration::platformDestroy()
{
    wl_display_destroy(mWaylandDisplay);
}

QSize QEglFSVivWaylandIntegration::screenSize() const
{
    return mScreenSize;
}

EGLNativeDisplayType QEglFSVivWaylandIntegration::platformDisplay() const
{
    return mNativeDisplay;
}

EGLNativeWindowType QEglFSVivWaylandIntegration::createNativeWindow(QPlatformWindow *window, const QSize &size, const QSurfaceFormat &format)
{
    Q_UNUSED(window);
    Q_UNUSED(format);

    EGLNativeWindowType eglWindow = static_cast<EGLNativeWindowType>(fbCreateWindow(mNativeDisplay, 0, 0, size.width(), size.height()));
    return eglWindow;
}

void QEglFSVivWaylandIntegration::destroyNativeWindow(EGLNativeWindowType window)
{
    fbDestroyWindow(window);
}

void *QEglFSVivWaylandIntegration::wlDisplay() const
{
    return mWaylandDisplay;
}


QT_END_NAMESPACE
