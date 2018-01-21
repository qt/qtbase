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

#include "qxcbglxwindow.h"

#include "qxcbscreen.h"
#include <QtGlxSupport/private/qglxconvenience_p.h>
#include <QDebug>

QT_BEGIN_NAMESPACE

QXcbGlxWindow::QXcbGlxWindow(QWindow *window)
    : QXcbWindow(window)
{
}

QXcbGlxWindow::~QXcbGlxWindow()
{
}

const xcb_visualtype_t *QXcbGlxWindow::createVisual()
{
    QXcbScreen *scr = xcbScreen();
    if (!scr)
        return nullptr;

    qCDebug(lcQpaGl) << "Requested format before FBConfig/Visual selection:" << m_format;

    Display *dpy = static_cast<Display *>(scr->connection()->xlib_display());
    const char *glxExts = glXQueryExtensionsString(dpy, scr->screenNumber());
    int flags = 0;
    if (glxExts) {
        qCDebug(lcQpaGl, "Available GLX extensions: %s", glxExts);
        if (strstr(glxExts, "GLX_EXT_framebuffer_sRGB") || strstr(glxExts, "GLX_ARB_framebuffer_sRGB"))
            flags |= QGLX_SUPPORTS_SRGB;
    }

    XVisualInfo *visualInfo = qglx_findVisualInfo(dpy, scr->screenNumber(), &m_format, GLX_WINDOW_BIT, flags);
    if (!visualInfo) {
        qWarning() << "No XVisualInfo for format" << m_format;
        return nullptr;
    }
    const xcb_visualtype_t *xcb_visualtype = scr->visualForId(visualInfo->visualid);
    XFree(visualInfo);

    qCDebug(lcQpaGl) << "Got format:" << m_format;

    return xcb_visualtype;
}

QT_END_NAMESPACE
