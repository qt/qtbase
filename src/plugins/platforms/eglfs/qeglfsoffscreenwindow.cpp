/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglfsoffscreenwindow.h"
#include "qeglfshooks.h"
#include <QtGui/QOffscreenSurface>
#include <QtPlatformSupport/private/qeglconvenience_p.h>

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
    m_surface = eglCreateWindowSurface(m_display, config, m_window, 0);
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
