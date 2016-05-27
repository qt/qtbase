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
    mNativeDisplay = fbGetDisplay(mWaylandDisplay);
    fbGetDisplayGeometry(mNativeDisplay, &width, &height);
    mScreenSize.setHeight(height);
    mScreenSize.setWidth(width);
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
    Q_UNUSED(window)
    Q_UNUSED(format)

    EGLNativeWindowType eglWindow = fbCreateWindow(mNativeDisplay, 0, 0, size.width(), size.height());
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
