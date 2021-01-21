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

#include "qeglfsvivintegration.h"
#include <EGL/eglvivante.h>
#include <QDebug>

#if QT_CONFIG(vulkan)
#include "private/qeglfsvulkaninstance_p.h"
#include "private/qeglfsvulkanwindow_p.h"
#endif

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
    Q_UNUSED(window)
    Q_UNUSED(format)

    EGLNativeWindowType eglWindow = static_cast<EGLNativeWindowType>(fbCreateWindow(mNativeDisplay, 0, 0, size.width(), size.height()));
    return eglWindow;
}

void QEglFSVivIntegration::destroyNativeWindow(EGLNativeWindowType window)
{
    fbDestroyWindow(window);
}

#if QT_CONFIG(vulkan)

QEglFSWindow *QEglFSVivIntegration::createWindow(QWindow *window) const
{
    if (window->surfaceType() == QSurface::VulkanSurface)
        return new QEglFSVulkanWindow(window);
    return QEglFSDeviceIntegration::createWindow(window);
}

QPlatformVulkanInstance *QEglFSVivIntegration::createPlatformVulkanInstance(QVulkanInstance *instance)
{
    return new QEglFSVulkanInstance(instance);
}

#endif // vulkan

QT_END_NAMESPACE
