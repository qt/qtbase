/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwaylandeglwindow.h"

#include "qwaylandscreen.h"
#include "qwaylandglcontext.h"

#include <QtPlatformSupport/private/qeglconvenience_p.h>

#include <QtGui/QWindow>

QWaylandEglWindow::QWaylandEglWindow(QWindow *window)
    : QWaylandWindow(window)
    , m_waylandEglWindow(0)
    , m_eglSurface(0)
    , m_eglConfig(0)
    , m_format(window->format())
{
    m_eglIntegration = static_cast<QWaylandEglIntegration *>(mDisplay->eglIntegration());

    //super creates a new surface
    newSurfaceCreated();
}

QWaylandEglWindow::~QWaylandEglWindow()
{
    if (m_eglSurface) {
        eglDestroySurface(m_eglIntegration->eglDisplay(), m_eglSurface);
        m_eglSurface = 0;
    }
}

QWaylandWindow::WindowType QWaylandEglWindow::windowType() const
{
    return QWaylandWindow::Egl;
}

void QWaylandEglWindow::setGeometry(const QRect &rect)
{
    QWaylandWindow::setGeometry(rect);
    if (m_waylandEglWindow)
        wl_egl_window_resize(m_waylandEglWindow, rect.width(), rect.height(), 0, 0);
}

void QWaylandEglWindow::newSurfaceCreated()
{
    if (m_waylandEglWindow)
        wl_egl_window_destroy(m_waylandEglWindow);

    wl_visual *visual = QWaylandScreen::waylandScreenFromWindow(window())->visual();
    QSize size = geometry().size();
    if (!size.isValid())
        size = QSize(0,0);

    if (m_eglSurface) {
        eglDestroySurface(m_eglIntegration->eglDisplay(), m_eglSurface);
        m_eglSurface = 0;
    }

    m_waylandEglWindow = wl_egl_window_create(mSurface, size.width(), size.height(), visual);
}

QSurfaceFormat QWaylandEglWindow::format() const
{
    return m_format;
}

EGLSurface QWaylandEglWindow::eglSurface() const
{
    if (!m_waylandEglWindow)
        return 0;

    if (!m_eglSurface) {
        if (!m_eglConfig)
            m_eglConfig = q_configFromGLFormat(m_eglIntegration->eglDisplay(), window()->format(), true);

        EGLNativeWindowType window = m_waylandEglWindow;
        m_eglSurface = eglCreateWindowSurface(m_eglIntegration->eglDisplay(), m_eglConfig, window, 0);
    }

    return m_eglSurface;
}

