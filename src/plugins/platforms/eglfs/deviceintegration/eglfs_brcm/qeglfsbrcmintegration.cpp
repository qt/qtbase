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

#include "qeglfsbrcmintegration.h"
#include <bcm_host.h>

QT_BEGIN_NAMESPACE

static DISPMANX_DISPLAY_HANDLE_T dispman_display = 0;

static EGLNativeWindowType createDispmanxLayer(const QPoint &pos, const QSize &size, int z, DISPMANX_FLAGS_ALPHA_T flags)
{
    VC_RECT_T dst_rect;
    dst_rect.x = pos.x();
    dst_rect.y = pos.y();
    dst_rect.width = size.width();
    dst_rect.height = size.height();

    VC_RECT_T src_rect;
    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = size.width() << 16;
    src_rect.height = size.height() << 16;

    DISPMANX_UPDATE_HANDLE_T dispman_update = vc_dispmanx_update_start(0);

    VC_DISPMANX_ALPHA_T alpha;
    alpha.flags = flags;
    alpha.opacity = 0xFF;
    alpha.mask = 0;

    DISPMANX_ELEMENT_HANDLE_T dispman_element = vc_dispmanx_element_add(
            dispman_update, dispman_display, z, &dst_rect, 0, &src_rect,
            DISPMANX_PROTECTION_NONE, &alpha, (DISPMANX_CLAMP_T *)NULL, (DISPMANX_TRANSFORM_T)0);

    vc_dispmanx_update_submit_sync(dispman_update);

    EGL_DISPMANX_WINDOW_T *eglWindow = new EGL_DISPMANX_WINDOW_T;
    eglWindow->element = dispman_element;
    eglWindow->width = size.width();
    eglWindow->height = size.height();

    return eglWindow;
}

static void destroyDispmanxLayer(EGLNativeWindowType window)
{
    EGL_DISPMANX_WINDOW_T *eglWindow = static_cast<EGL_DISPMANX_WINDOW_T *>(window);
    DISPMANX_UPDATE_HANDLE_T dispman_update = vc_dispmanx_update_start(0);
    vc_dispmanx_element_remove(dispman_update, eglWindow->element);
    vc_dispmanx_update_submit_sync(dispman_update);
    delete eglWindow;
}

void QEglFSBrcmIntegration::platformInit()
{
    bcm_host_init();
}

EGLNativeDisplayType QEglFSBrcmIntegration::platformDisplay() const
{
    dispman_display = vc_dispmanx_display_open(0/* LCD */);
    return EGL_DEFAULT_DISPLAY;
}

void QEglFSBrcmIntegration::platformDestroy()
{
    vc_dispmanx_display_close(dispman_display);
}

QSize QEglFSBrcmIntegration::screenSize() const
{
    uint32_t width, height;
    graphics_get_display_size(0 /* LCD */, &width, &height);
    return QSize(width, height);
}

EGLNativeWindowType QEglFSBrcmIntegration::createNativeWindow(QPlatformWindow *window, const QSize &size, const QSurfaceFormat &format)
{
    Q_UNUSED(window)
    return createDispmanxLayer(QPoint(0, 0), size, 1, format.hasAlpha() ? DISPMANX_FLAGS_ALPHA_FROM_SOURCE : DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS);
}

void QEglFSBrcmIntegration::destroyNativeWindow(EGLNativeWindowType window)
{
    destroyDispmanxLayer(window);
}

bool QEglFSBrcmIntegration::hasCapability(QPlatformIntegration::Capability cap) const
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

QT_END_NAMESPACE
