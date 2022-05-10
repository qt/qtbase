// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeglfsoffscreenwindow_p.h"
#include "qeglfshooks_p.h"
#include <QtGui/QOffscreenSurface>
#include <QtGui/private/qeglconvenience_p.h>

QT_BEGIN_NAMESPACE

/*
    In some cases pbuffers are not available. Triggering QtGui's built-in
    fallback for a hidden QWindow is not suitable for eglfs since this would be
    treated as an attempt to create multiple top-level, native windows.

    Therefore this class is provided as an alternative to QEGLPbuffer.

    This class requires the hooks to implement createNativeOffscreenWindow().
*/

QEglFSOffscreenWindow::QEglFSOffscreenWindow(EGLDisplay display, const QSurfaceFormat &format, QOffscreenSurface *offscreenSurface)
    : QPlatformOffscreenSurface(offscreenSurface)
    , m_format(format)
    , m_display(display)
    , m_surface(EGL_NO_SURFACE)
    , m_window(0)
{
    m_window = qt_egl_device_integration()->createNativeOffscreenWindow(format);
    if (!m_window) {
        qWarning("QEglFSOffscreenWindow: Failed to create native window");
        return;
    }
    EGLConfig config = q_configFromGLFormat(m_display, m_format);
    m_surface = eglCreateWindowSurface(m_display, config, m_window, nullptr);
    if (m_surface != EGL_NO_SURFACE)
        m_format = q_glFormatFromConfig(m_display, config);
}

QEglFSOffscreenWindow::~QEglFSOffscreenWindow()
{
    if (m_surface != EGL_NO_SURFACE)
        eglDestroySurface(m_display, m_surface);
    if (m_window)
        qt_egl_device_integration()->destroyNativeWindow(m_window);
}

QT_END_NAMESPACE
