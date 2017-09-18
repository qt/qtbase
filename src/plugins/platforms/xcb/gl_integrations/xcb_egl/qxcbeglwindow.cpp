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

#include "qxcbeglwindow.h"

#include "qxcbeglintegration.h"

#include <QtEglSupport/private/qeglconvenience_p.h>
#include <QtEglSupport/private/qxlibeglintegration_p.h>

QT_BEGIN_NAMESPACE

QXcbEglWindow::QXcbEglWindow(QWindow *window, QXcbEglIntegration *glIntegration)
    : QXcbWindow(window)
    , m_glIntegration(glIntegration)
    , m_config(nullptr)
    , m_surface(EGL_NO_SURFACE)
{
}

QXcbEglWindow::~QXcbEglWindow()
{
    eglDestroySurface(m_glIntegration->eglDisplay(), m_surface);
}

void QXcbEglWindow::resolveFormat(const QSurfaceFormat &format)
{
    m_config = q_configFromGLFormat(m_glIntegration->eglDisplay(), format);
    m_format = q_glFormatFromConfig(m_glIntegration->eglDisplay(), m_config, format);
}

#if QT_CONFIG(xcb_xlib)
const xcb_visualtype_t *QXcbEglWindow::createVisual()
{
    QXcbScreen *scr = xcbScreen();
    if (!scr)
        return QXcbWindow::createVisual();

    Display *xdpy = static_cast<Display *>(m_glIntegration->xlib_display());
    VisualID id = QXlibEglIntegration::getCompatibleVisualId(xdpy, m_glIntegration->eglDisplay(), m_config);

    XVisualInfo visualInfoTemplate;
    memset(&visualInfoTemplate, 0, sizeof(XVisualInfo));
    visualInfoTemplate.visualid = id;

    XVisualInfo *visualInfo;
    int matchingCount = 0;
    visualInfo = XGetVisualInfo(xdpy, VisualIDMask, &visualInfoTemplate, &matchingCount);
    const xcb_visualtype_t *xcb_visualtype = scr->visualForId(visualInfo->visualid);
    XFree(visualInfo);

    return xcb_visualtype;
}
#endif

void QXcbEglWindow::create()
{
    QXcbWindow::create();

    m_surface = eglCreateWindowSurface(m_glIntegration->eglDisplay(), m_config, m_window, 0);
}

QT_END_NAMESPACE
