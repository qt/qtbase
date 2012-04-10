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

#include <bcm_host.h>

#if 0 //fb size query
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>
#endif

static DISPMANX_DISPLAY_HANDLE_T dispman_display = 0;
static DISPMANX_UPDATE_HANDLE_T dispman_update = 0;

void QEglFSHooks::platformInit()
{
    bcm_host_init();
}

EGLNativeDisplayType QEglFSHooks::platformDisplay() const
{
    dispman_display = vc_dispmanx_display_open(0/* LCD */);
    return EGL_DEFAULT_DISPLAY;
}

void QEglFSHooks::platformDestroy()
{
    vc_dispmanx_display_close(dispman_display);
}

QSize QEglFSHooks::screenSize() const
{
    //both mechanisms work
#if 1
    uint32_t width, height;
    graphics_get_display_size(0 /* LCD */, &width, &height);
    return QSize(width, height);
#else
    int fd = open("/dev/fb0", O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Failed to open fb to detect screen resolution!\n");
        return QSize();
    }

    struct fb_var_screeninfo vinfo;
    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) = -1) {
        fprintf(stderr, "Could not query screen info variable\n");
        close(fd);
        return QSize();
    }

    close(fd);

    return QSize(vinfo.xres, vinfo.yres);
#endif
}

EGLNativeWindowType QEglFSHooks::createNativeWindow(const QSize &size)
{
    VC_RECT_T dst_rect;
    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = size.width();
    dst_rect.height = size.height();

    VC_RECT_T src_rect;
    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = size.width() << 16;
    src_rect.height = size.height() << 16;

    dispman_update = vc_dispmanx_update_start(0);

    VC_DISPMANX_ALPHA_T alpha;
    alpha.flags = DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS;
    alpha.opacity = 0xFF;
    alpha.mask = 0;

    DISPMANX_ELEMENT_HANDLE_T dispman_element = vc_dispmanx_element_add(
            dispman_update, dispman_display, 0, &dst_rect, 0, &src_rect,
            DISPMANX_PROTECTION_NONE, &alpha, (DISPMANX_CLAMP_T *)NULL, (DISPMANX_TRANSFORM_T)0);

    vc_dispmanx_update_submit_sync(dispman_update);

    EGL_DISPMANX_WINDOW_T *eglWindow = new EGL_DISPMANX_WINDOW_T;
    eglWindow->element = dispman_element;
    eglWindow->width = size.width();
    eglWindow->height = size.height();

    return eglWindow;
}

void QEglFSHooks::destroyNativeWindow(EGLNativeWindowType window)
{
    EGL_DISPMANX_WINDOW_T *eglWindow = static_cast<EGL_DISPMANX_WINDOW_T *>(window);
    vc_dispmanx_element_remove(dispman_update, eglWindow->element);
    delete eglWindow;
}

bool QEglFSHooks::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
        case QPlatformIntegration::ThreadedPixmaps:
        case QPlatformIntegration::OpenGL:
        case QPlatformIntegration::ThreadedOpenGL:
        case QPlatformIntegration::BufferQueueingOpenGL:
            return true;
        default:
            return false;
    }
}

QEglFSHooks platform_hooks;
