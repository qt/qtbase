// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qxcbglxwindow.h"

#include "qxcbscreen.h"
#include <QtGui/private/qglxconvenience_p.h>
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
        return QXcbWindow::createVisual();

    qCDebug(lcQpaGl) << "Requested format before FBConfig/Visual selection:" << m_format;

    Display *dpy = static_cast<Display *>(scr->connection()->xlib_display());
    const char *glxExts = glXQueryExtensionsString(dpy, scr->screenNumber());
    int flags = 0;
    if (glxExts) {
        qCDebug(lcQpaGl, "Available GLX extensions: %s", glxExts);
        if (strstr(glxExts, "GLX_EXT_framebuffer_sRGB") || strstr(glxExts, "GLX_ARB_framebuffer_sRGB"))
            flags |= QGLX_SUPPORTS_SRGB;
    }

    const auto formatBackup = m_format;
    XVisualInfo *visualInfo = qglx_findVisualInfo(dpy, scr->screenNumber(), &m_format, GLX_WINDOW_BIT, flags);
    if (!visualInfo) {
        qCDebug(lcQpaGl) << "No XVisualInfo for format" << m_format;
        // restore initial format before requesting it again
        m_format = formatBackup;
        return QXcbWindow::createVisual();
    }
    const xcb_visualtype_t *xcb_visualtype = scr->visualForId(visualInfo->visualid);
    XFree(visualInfo);

    qCDebug(lcQpaGl) << "Got format:" << m_format;

    return xcb_visualtype;
}

QT_END_NAMESPACE
