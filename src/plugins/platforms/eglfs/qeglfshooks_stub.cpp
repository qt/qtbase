/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake spec of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglfshooks.h"

#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>

QT_BEGIN_NAMESPACE

void QEglFSHooks::platformInit()
{
    Q_UNUSED(hooks);
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
    static QSize size;

    if (size.isEmpty()) {
        int width = qgetenv("QT_QPA_EGLFS_WIDTH").toInt();
        int height = qgetenv("QT_QPA_EGLFS_HEIGHT").toInt();

        if (width && height) {
            // no need to read fb0
            size.setWidth(width);
            size.setHeight(height);
            return size;
        }

        struct fb_var_screeninfo vinfo;
        int fd = open("/dev/fb0", O_RDONLY);

        if (fd != -1) {
            if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == -1)
                qWarning("Could not query variable screen info.");
            else
                size = QSize(vinfo.xres, vinfo.yres);

            close(fd);
        } else {
            qWarning("Failed to open /dev/fb0 to detect screen resolution.");
        }

        // override fb0 from environment var setting
        if (width)
            size.setWidth(width);
        if (height)
            size.setHeight(height);
    }

    return size;
}

int QEglFSHooks::screenDepth() const
{
    static int depth = qgetenv("QT_QPA_EGLFS_DEPTH").toInt();

    if (depth == 0) {
        struct fb_var_screeninfo vinfo;
        int fd = open("/dev/fb0", O_RDONLY);

        if (fd != -1) {
            if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == -1)
                qWarning("Could not query variable screen info.");
            else
                depth = vinfo.bits_per_pixel;

            close(fd);
        } else {
            qWarning("Failed to open /dev/fb0 to detect screen depth.");
        }
    }

    return depth == 0 ? 32 : depth;
}

QImage::Format QEglFSHooks::screenFormat() const
{
    return screenDepth() == 16 ? QImage::Format_RGB16 : QImage::Format_RGB32;
}

QSurfaceFormat QEglFSHooks::surfaceFormatFor(const QSurfaceFormat &inputFormat) const
{
    return inputFormat;
}

EGLNativeWindowType QEglFSHooks::createNativeWindow(const QSize &size, const QSurfaceFormat &format)
{
    Q_UNUSED(size);
    Q_UNUSED(format);
    return 0;
}

void QEglFSHooks::destroyNativeWindow(EGLNativeWindowType window)
{
    Q_UNUSED(window);
}

bool QEglFSHooks::hasCapability(QPlatformIntegration::Capability cap) const
{
    Q_UNUSED(cap);
    return false;
}

QEglFSCursor *QEglFSHooks::createCursor(QEglFSScreen *screen) const
{
    Q_UNUSED(screen);
    return 0;
}

#ifndef EGLFS_PLATFORM_HOOKS
QEglFSHooks stubHooks;
#endif

QT_END_NAMESPACE
