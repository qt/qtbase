/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
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
