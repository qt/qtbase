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

#include "qandroidplatformoffscreensurface.h"

#include <QtGui/QOffscreenSurface>
#include <QtEglSupport/private/qeglconvenience_p.h>

#include <android/native_window.h>

QT_BEGIN_NAMESPACE

QAndroidPlatformOffscreenSurface::QAndroidPlatformOffscreenSurface(EGLDisplay display, const QSurfaceFormat &format, QOffscreenSurface *offscreenSurface)
    : QPlatformOffscreenSurface(offscreenSurface)
    , m_format(format)
    , m_display(display)
    , m_surface(EGL_NO_SURFACE)
{
    // Get native handle
    ANativeWindow *surfaceTexture = (ANativeWindow*)offscreenSurface->nativeHandle();

    EGLConfig config = q_configFromGLFormat(m_display, m_format, false);
    if (config) {
        const EGLint attributes[] = {
            EGL_NONE
        };
        m_surface = eglCreateWindowSurface(m_display, config, surfaceTexture, attributes);
    }
}

QAndroidPlatformOffscreenSurface::~QAndroidPlatformOffscreenSurface()
{
    eglDestroySurface(m_display, m_surface);
}

QT_END_NAMESPACE

