/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglfswindow.h"
#include "qeglfshooks.h"
#include <qpa/qwindowsysteminterface.h>

#include <QtPlatformSupport/private/qeglconvenience_p.h>

#include <QtDebug>

QT_BEGIN_NAMESPACE

QEglFSWindow::QEglFSWindow(QWindow *w)
    : QPlatformWindow(w)
    , m_surface(0)
    , m_window(0)
{
    static int serialNo = 0;
    m_winid  = ++serialNo;
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglWindow %p: %p 0x%x\n", this, w, uint(m_winid));
#endif

    setWindowState(Qt::WindowFullScreen);

    create();
}

QEglFSWindow::~QEglFSWindow()
{
    destroy();
}

void QEglFSWindow::create()
{
    if (m_window)
        return;

    if (window()->type() == Qt::Desktop) {
        QRect rect(QPoint(), hooks->screenSize());
        QPlatformWindow::setGeometry(rect);
        QWindowSystemInterface::handleGeometryChange(window(), rect);
        return;
    }

    EGLDisplay display = (static_cast<QEglFSScreen *>(window()->screen()->handle()))->display();
    QSurfaceFormat platformFormat = hooks->surfaceFormatFor(window()->requestedFormat());
    EGLConfig config = q_configFromGLFormat(display, platformFormat);
    m_format = q_glFormatFromConfig(display, config);
    m_window = hooks->createNativeWindow(hooks->screenSize(), m_format);
    m_surface = eglCreateWindowSurface(display, config, m_window, NULL);
    if (m_surface == EGL_NO_SURFACE) {
        EGLint error = eglGetError();
        eglTerminate(display);
        qFatal("EGL Error : Could not create the egl surface: error = 0x%x\n", error);
    }
}

void QEglFSWindow::destroy()
{
    if (m_surface) {
        EGLDisplay display = (static_cast<QEglFSScreen *>(window()->screen()->handle()))->display();
        eglDestroySurface(display, m_surface);
        m_surface = 0;
    }

    if (m_window) {
        hooks->destroyNativeWindow(m_window);
        m_window = 0;
    }
}

void QEglFSWindow::setGeometry(const QRect &)
{
    // We only support full-screen windows
    QRect rect(screen()->availableGeometry());
    QWindowSystemInterface::handleGeometryChange(window(), rect);

    QPlatformWindow::setGeometry(rect);
}

void QEglFSWindow::setWindowState(Qt::WindowState)
{
    setGeometry(QRect());
}

WId QEglFSWindow::winId() const
{
    return m_winid;
}

QSurfaceFormat QEglFSWindow::format() const
{
    return m_format;
}

QT_END_NAMESPACE
