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

#include "qeglfsoffscreenwindow_p.h"
#include "qeglfshooks_p.h"
#include <QtGui/QOffscreenSurface>
#include <QtEglSupport/private/qeglconvenience_p.h>

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
