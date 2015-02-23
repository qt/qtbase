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

#include "qxcbeglwindow.h"

#include "qxcbeglintegration.h"

#include <QtPlatformSupport/private/qeglconvenience_p.h>
#include <QtPlatformSupport/private/qxlibeglintegration_p.h>

QT_BEGIN_NAMESPACE

QXcbEglWindow::QXcbEglWindow(QWindow *window, QXcbEglIntegration *glIntegration)
    : QXcbWindow(window)
    , m_glIntegration(glIntegration)
    , m_config(Q_NULLPTR)
    , m_surface(EGL_NO_SURFACE)
{
}

QXcbEglWindow::~QXcbEglWindow()
{
    eglDestroySurface(m_glIntegration->eglDisplay(), m_surface);
}

void QXcbEglWindow::resolveFormat()
{
    m_config = q_configFromGLFormat(m_glIntegration->eglDisplay(), window()->requestedFormat(), true);
    m_format = q_glFormatFromConfig(m_glIntegration->eglDisplay(), m_config, m_format);
}

void *QXcbEglWindow::createVisual()
{
#ifdef XCB_USE_XLIB
    Display *xdpy = static_cast<Display *>(m_glIntegration->xlib_display());
    VisualID id = QXlibEglIntegration::getCompatibleVisualId(xdpy, m_glIntegration->eglDisplay(), m_config);

    XVisualInfo visualInfoTemplate;
    memset(&visualInfoTemplate, 0, sizeof(XVisualInfo));
    visualInfoTemplate.visualid = id;

    XVisualInfo *visualInfo;
    int matchingCount = 0;
    visualInfo = XGetVisualInfo(xdpy, VisualIDMask, &visualInfoTemplate, &matchingCount);
    return visualInfo;
#else
    return QXcbWindow::createVisual();
#endif
}

void QXcbEglWindow::create()
{
    QXcbWindow::create();

    m_surface = eglCreateWindowSurface(m_glIntegration->eglDisplay(), m_config, m_window, 0);
}

QT_END_NAMESPACE
