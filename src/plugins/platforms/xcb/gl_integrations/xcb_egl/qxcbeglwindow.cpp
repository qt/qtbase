// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qxcbeglwindow.h"

#include "qxcbeglintegration.h"

#include <QtGui/private/qeglconvenience_p.h>

#ifndef EGL_EXT_platform_base
typedef EGLSurface (EGLAPIENTRYP PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC) (EGLDisplay dpy, EGLConfig config, void *native_window, const EGLint *attrib_list);
#endif

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

const xcb_visualtype_t *QXcbEglWindow::createVisual()
{
    QXcbScreen *scr = xcbScreen();
    if (!scr)
        return QXcbWindow::createVisual();

    xcb_visualid_t id = m_glIntegration->getCompatibleVisualId(scr->screen(), m_config);
    return scr->visualForId(id);
}

void QXcbEglWindow::create()
{
    QXcbWindow::create();

    // this is always true without egl_x11
    if (m_glIntegration->usingPlatformDisplay()) {
        auto createPlatformWindowSurface = reinterpret_cast<PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC>(
            eglGetProcAddress("eglCreatePlatformWindowSurfaceEXT"));
        m_surface = createPlatformWindowSurface(m_glIntegration->eglDisplay(),
                                                m_config,
                                                reinterpret_cast<void *>(&m_window),
                                                nullptr);
        return;
    }

#if QT_CONFIG(egl_x11)
    m_surface = eglCreateWindowSurface(m_glIntegration->eglDisplay(), m_config, m_window, nullptr);
#endif
}

QT_END_NAMESPACE
