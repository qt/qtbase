/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#include "qandroidplatformoffscreensurface.h"

#include <QtGui/private/qeglconvenience_p.h>

QT_BEGIN_NAMESPACE

QAndroidPlatformOffscreenSurface::QAndroidPlatformOffscreenSurface(
        ANativeWindow *nativeSurface, EGLDisplay display, QOffscreenSurface *offscreenSurface)
    : QPlatformOffscreenSurface(offscreenSurface), m_display(display), m_surface(EGL_NO_SURFACE)
{
    // FIXME: Read surface format properties from native surface using ANativeWindow_getFormat
    m_format.setAlphaBufferSize(8);
    m_format.setRedBufferSize(8);
    m_format.setGreenBufferSize(8);
    m_format.setBlueBufferSize(8);

    if (EGLConfig config = q_configFromGLFormat(m_display, m_format, false)) {
        const EGLint attributes[] = { EGL_NONE };
        m_surface = eglCreateWindowSurface(m_display, config, nativeSurface, attributes);
    }
}

QAndroidPlatformOffscreenSurface::~QAndroidPlatformOffscreenSurface()
{
    eglDestroySurface(m_display, m_surface);
}

QT_END_NAMESPACE

