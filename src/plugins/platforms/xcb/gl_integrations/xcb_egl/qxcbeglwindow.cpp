// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qxcbeglwindow.h"

#include "qxcbeglintegration.h"

#include <QtGui/private/qeglconvenience_p.h>
#include <QtGui/private/qxlibeglintegration_p.h>

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

    m_surface = eglCreateWindowSurface(m_glIntegration->eglDisplay(), m_config, m_window, nullptr);
}

QT_END_NAMESPACE
