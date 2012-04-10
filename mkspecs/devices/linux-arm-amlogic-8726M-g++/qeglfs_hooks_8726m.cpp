/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the qmake spec of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglfs_hooks.h"
#include <EGL/fbdev_window.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>

void QEglFSHooks::platformInit()
{
}

void QEglFSHooks::platformDestroy()
{
}

EGLNativeDisplayType QEglFSHooks::platformDisplay() const
{
    return EGL_DEFAULT_DISPLAY;
}

QSize QEglFSHooks::screenSize() const
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

EGLNativeWindowType QEglFSHooks::createNativeWindow(const QSize &size)
{
    fbdev_window *window = new fbdev_window;
    window->width = size.width();
    window->height = size.height();

    return window;
}

void QEglFSHooks::destroyNativeWindow(EGLNativeWindowType window)
{
    delete window;
}

bool QEglFSHooks::hasCapability(QPlatformIntegration::Capability cap) const
{
    return false;
}

QEglFSHooks platform_hooks;
