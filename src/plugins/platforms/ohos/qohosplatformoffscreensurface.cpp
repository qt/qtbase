// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosplatformoffscreensurface.h"

#include <QtGui/QOffscreenSurface>
#include <QtGui/private/qeglconvenience_p.h>


QT_BEGIN_NAMESPACE

QOhosPlatformOffscreenSurface::QOhosPlatformOffscreenSurface(EGLDisplay display, const QSurfaceFormat &format, QOffscreenSurface *offscreenSurface)
    : QPlatformOffscreenSurface(offscreenSurface)
    , m_format(format)
    , m_display(display)
    , m_surface(EGL_NO_SURFACE)
{
    // Get native handle
    // HACK
    // ANativeWindow *surfaceTexture = (ANativeWindow*)offscreenSurface->nativeHandle();

    EGLConfig config = q_configFromGLFormat(m_display, m_format, false);
    if (config != nullptr) {
        const EGLint attributes[] = {
            EGL_NONE
        };
        Q_UNUSED(attributes);
        //HACK
        //m_surface = eglCreateWindowSurface(m_display, config, surfaceTexture, attributes);
    }
}

QOhosPlatformOffscreenSurface::~QOhosPlatformOffscreenSurface()
{
    eglDestroySurface(m_display, m_surface);
}

QT_END_NAMESPACE

