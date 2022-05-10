// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeglfskmsgbmwindow_p.h"
#include "qeglfskmsgbmintegration_p.h"
#include "qeglfskmsgbmscreen_p.h"

#include <QtGui/private/qeglconvenience_p.h>

QT_BEGIN_NAMESPACE

#ifndef EGL_EXT_platform_base
typedef EGLSurface (EGLAPIENTRYP PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC) (EGLDisplay dpy, EGLConfig config, void *native_window, const EGLint *attrib_list);
#endif

void QEglFSKmsGbmWindow::resetSurface()
{
    QEglFSKmsGbmScreen *gbmScreen = static_cast<QEglFSKmsGbmScreen *>(screen());
    EGLDisplay display = gbmScreen->display();
    QSurfaceFormat platformFormat = m_integration->surfaceFormatFor(window()->requestedFormat());
    m_config = QEglFSDeviceIntegration::chooseConfig(display, platformFormat);
    m_format = q_glFormatFromConfig(display, m_config, platformFormat);
    // One fullscreen window per screen -> the native window is simply the gbm_surface the screen created.
    m_window = reinterpret_cast<EGLNativeWindowType>(gbmScreen->createSurface(m_config));

    PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC createPlatformWindowSurface = nullptr;
    const char *extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if (extensions && (strstr(extensions, "EGL_KHR_platform_gbm") || strstr(extensions, "EGL_MESA_platform_gbm"))) {
        createPlatformWindowSurface = reinterpret_cast<PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC>(
            eglGetProcAddress("eglCreatePlatformWindowSurfaceEXT"));
    }

    if (createPlatformWindowSurface) {
        QVector<EGLint> contextAttributes;
#ifdef EGL_EXT_protected_content
        if (platformFormat.testOption(QSurfaceFormat::ProtectedContent)) {
            if (q_hasEglExtension(display, "EGL_EXT_protected_content")) {
                contextAttributes.append(EGL_PROTECTED_CONTENT_EXT);
                contextAttributes.append(EGL_TRUE);
                qCDebug(qLcEglfsKmsDebug, "Enabled EGL_PROTECTED_CONTENT_EXT for eglCreatePlatformWindowSurfaceEXT");
            } else {
                m_format.setOption(QSurfaceFormat::ProtectedContent, false);
            }
        }
#endif
        contextAttributes.append(EGL_NONE);

        m_surface = createPlatformWindowSurface(display, m_config, reinterpret_cast<void *>(m_window), contextAttributes.constData());
    } else {
        qCDebug(qLcEglfsKmsDebug, "No eglCreatePlatformWindowSurface for GBM, falling back to eglCreateWindowSurface");
        m_surface = eglCreateWindowSurface(display, m_config, m_window, nullptr);
    }
}

void QEglFSKmsGbmWindow::invalidateSurface()
{
    QEglFSKmsGbmScreen *gbmScreen = static_cast<QEglFSKmsGbmScreen *>(screen());
    QEglFSWindow::invalidateSurface();
    gbmScreen->resetSurface();
}

QT_END_NAMESPACE
