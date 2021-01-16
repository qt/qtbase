/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the qmake spec of the Qt Toolkit.
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

#include "private/qeglfshooks_p.h"
#include <EGL/fbdev_window.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>

QT_BEGIN_NAMESPACE

class QEglFS8726MHooks : public QEglFSHooks
{
public:
    virtual QSize screenSize() const;
    virtual EGLNativeWindowType createNativeWindow(QPlatformWindow *window, const QSize &size, const QSurfaceFormat &format);
    virtual void destroyNativeWindow(EGLNativeWindowType window);
};

QSize QEglFS8726MHooks::screenSize() const
{
    int fd = open("/dev/fb0", O_RDONLY);
    if (fd == -1) {
        qFatal("Failed to open fb to detect screen resolution!");
    }

    struct fb_var_screeninfo vinfo;
    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        qFatal("Could not get variable screen info");
    }

    close(fd);

    return QSize(vinfo.xres, vinfo.yres);
}

EGLNativeWindowType QEglFS8726MHooks::createNativeWindow(QPlatformWindow *window, const QSize &size, const QSurfaceFormat &format)
{
    Q_UNUSED(window)
    Q_UNUSED(format)

    fbdev_window *window = new fbdev_window;
    window->width = size.width();
    window->height = size.height();

    return window;
}

void QEglFS8726MHooks::destroyNativeWindow(EGLNativeWindowType window)
{
    delete window;
}

QEglFS8726MHooks eglFS8726MHooks;
QEglFSHooks *platformHooks = &eglFS8726MHooks;

QT_END_NAMESPACE
