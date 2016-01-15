/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglfsmaliintegration.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include <private/qcore_unix_p.h>

QT_BEGIN_NAMESPACE

struct fbdev_window {
    unsigned short width;
    unsigned short height;
};

void QEglFSMaliIntegration::platformInit()
{
    // Keep the non-overridden base class functions based on fb0 working.
    QEGLDeviceIntegration::platformInit();

    int fd = qt_safe_open("/dev/fb0", O_RDWR, 0);
    if (fd == -1)
        qWarning("Failed to open fb to detect screen resolution!");

    struct fb_var_screeninfo vinfo;
    memset(&vinfo, 0, sizeof(vinfo));
    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == -1)
        qWarning("Could not get variable screen info");

    vinfo.bits_per_pixel   = 32;
    vinfo.red.length       = 8;
    vinfo.green.length     = 8;
    vinfo.blue.length      = 8;
    vinfo.transp.length    = 8;
    vinfo.blue.offset      = 0;
    vinfo.green.offset     = 8;
    vinfo.red.offset       = 16;
    vinfo.transp.offset    = 24;
#if 0
    vinfo.yres_virtual     = 2 * vinfo.yres;
#endif

    if (ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo) == -1)
        qErrnoWarning(errno, "Unable to set double buffer mode!");

    qt_safe_close(fd);
}

EGLNativeWindowType QEglFSMaliIntegration::createNativeWindow(QPlatformWindow *window, const QSize &size, const QSurfaceFormat &format)
{
    Q_UNUSED(window);
    Q_UNUSED(format);

    fbdev_window *fbwin = reinterpret_cast<fbdev_window *>(malloc(sizeof(fbdev_window)));
    if (NULL == fbwin)
        return 0;

    fbwin->width = size.width();
    fbwin->height = size.height();
    return (EGLNativeWindowType)fbwin;
}

void QEglFSMaliIntegration::destroyNativeWindow(EGLNativeWindowType window)
{
    free((void*)window);
}

QT_END_NAMESPACE
