// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeglfsvivintegration.h"
#include <EGL/eglvivante.h>
#include <QDebug>

#ifdef Q_OS_INTEGRITY
extern "C" void VivanteInit(void);
#endif

QT_BEGIN_NAMESPACE

void QEglFSVivIntegration::platformInit()
{
    QEglFSDeviceIntegration::platformInit();

    int width, height;

    bool multiBufferNotEnabledYet = qEnvironmentVariableIsEmpty("FB_MULTI_BUFFER");
    bool multiBuffer = qEnvironmentVariableIsEmpty("QT_EGLFS_IMX6_NO_FB_MULTI_BUFFER");
    if (multiBufferNotEnabledYet && multiBuffer) {
        qWarning() << "QEglFSVivIntegration will set environment variable FB_MULTI_BUFFER=2 to enable double buffering and vsync.\n"
                   << "If this is not desired, you can override this via: export QT_EGLFS_IMX6_NO_FB_MULTI_BUFFER=1";
        qputenv("FB_MULTI_BUFFER", "2");
    }

#ifdef Q_OS_INTEGRITY
    VivanteInit();
    mNativeDisplay = fbGetDisplay();
#else
    mNativeDisplay = static_cast<EGLNativeDisplayType>(fbGetDisplayByIndex(framebufferIndex()));
#endif

    fbGetDisplayGeometry(mNativeDisplay, &width, &height);
    mScreenSize.setHeight(height);
    mScreenSize.setWidth(width);
}

QSize QEglFSVivIntegration::screenSize() const
{
    return mScreenSize;
}

EGLNativeDisplayType QEglFSVivIntegration::platformDisplay() const
{
    return mNativeDisplay;
}

EGLNativeWindowType QEglFSVivIntegration::createNativeWindow(QPlatformWindow *window, const QSize &size, const QSurfaceFormat &format)
{
    Q_UNUSED(window);
    Q_UNUSED(format);

    EGLNativeWindowType eglWindow = static_cast<EGLNativeWindowType>(fbCreateWindow(mNativeDisplay, 0, 0, size.width(), size.height()));
    return eglWindow;
}

void QEglFSVivIntegration::destroyNativeWindow(EGLNativeWindowType window)
{
    fbDestroyWindow(window);
}

QT_END_NAMESPACE
