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

#include "qwaylandeglintegration.h"

#include "gl_integration/qwaylandglintegration.h"

#include "qwaylandeglwindow.h"
#include "qwaylandglcontext.h"

#include <QtCore/QDebug>

QWaylandEglIntegration::QWaylandEglIntegration(struct wl_display *waylandDisplay)
    : m_waylandDisplay(waylandDisplay)
{
    qDebug() << "Using Wayland-EGL";
}


QWaylandEglIntegration::~QWaylandEglIntegration()
{
    eglTerminate(m_eglDisplay);
}

void QWaylandEglIntegration::initialize()
{
    QByteArray eglPlatform = qgetenv("EGL_PLATFORM");
    if (eglPlatform.isEmpty()) {
        setenv("EGL_PLATFORM","wayland",true);
    }

    EGLint major,minor;
    m_eglDisplay = eglGetDisplay(m_waylandDisplay);
    if (m_eglDisplay == NULL) {
        qWarning("EGL not available");
    } else {
        if (!eglInitialize(m_eglDisplay, &major, &minor)) {
            qWarning("failed to initialize EGL display");
            return;
        }
    }
}

QWaylandWindow *QWaylandEglIntegration::createEglWindow(QWindow *window)
{
    return new QWaylandEglWindow(window);
}

QPlatformOpenGLContext *QWaylandEglIntegration::createPlatformOpenGLContext(const QSurfaceFormat &glFormat, QPlatformOpenGLContext *share) const
{
    return new QWaylandGLContext(m_eglDisplay, glFormat, share);
}

EGLDisplay QWaylandEglIntegration::eglDisplay() const
{
    return m_eglDisplay;
}

QWaylandGLIntegration *QWaylandGLIntegration::createGLIntegration(QWaylandDisplay *waylandDisplay)
{
    return new QWaylandEglIntegration(waylandDisplay->wl_display());
}
