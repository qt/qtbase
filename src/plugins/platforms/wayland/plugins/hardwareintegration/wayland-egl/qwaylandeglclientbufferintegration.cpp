// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandeglclientbufferintegration_p.h"

#include "qwaylandeglwindow_p.h"
#include "qwaylandglcontext_p.h"

#include <wayland-client-core.h>

#include <QtCore/QDebug>
#include <private/qeglconvenience_p.h>
#include <private/qeglpbuffer_p.h>

#ifndef EGL_EXT_platform_base
typedef EGLDisplay (*PFNEGLGETPLATFORMDISPLAYEXTPROC) (EGLenum platform, void *native_display, const EGLint *attrib_list);
#endif

#ifndef EGL_PLATFORM_WAYLAND_KHR
#define EGL_PLATFORM_WAYLAND_KHR 0x31D8
#endif

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

static const char *qwaylandegl_threadedgl_blacklist_vendor[] = {
    0
};

QWaylandEglClientBufferIntegration::QWaylandEglClientBufferIntegration()
{
    qCDebug(lcQpaWayland) << "Using Wayland-EGL";
}


QWaylandEglClientBufferIntegration::~QWaylandEglClientBufferIntegration()
{
    eglTerminate(m_eglDisplay);
}

void QWaylandEglClientBufferIntegration::initialize(QWaylandDisplay *display)
{
#if QT_CONFIG(egl_extension_platform_wayland)
    m_eglDisplay = eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_EXT, display->wl_display(), nullptr);
#else
    if (q_hasEglExtension(EGL_NO_DISPLAY, "EGL_EXT_platform_base")) {
        if (q_hasEglExtension(EGL_NO_DISPLAY, "EGL_KHR_platform_wayland") ||
            q_hasEglExtension(EGL_NO_DISPLAY, "EGL_EXT_platform_wayland") ||
            q_hasEglExtension(EGL_NO_DISPLAY, "EGL_MESA_platform_wayland")) {

            static PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplay = nullptr;
            if (!eglGetPlatformDisplay)
                eglGetPlatformDisplay = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");

            m_eglDisplay = eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_KHR, display->wl_display(), nullptr);
        } else {
            qCWarning(lcQpaWayland) << "The EGL implementation does not support the Wayland platform";
            return;
        }
    } else {
        QByteArray eglPlatform = qgetenv("EGL_PLATFORM");
        if (eglPlatform.isEmpty()) {
            setenv("EGL_PLATFORM","wayland",true);
        }

        m_eglDisplay = eglGetDisplay((EGLNativeDisplayType) display->wl_display());
    }
#endif

    m_display = display;

    if (m_eglDisplay == EGL_NO_DISPLAY) {
        qCWarning(lcQpaWayland) << "EGL not available";
        return;
    }

    EGLint major,minor;
    if (!eglInitialize(m_eglDisplay, &major, &minor)) {
        qCWarning(lcQpaWayland) <<  "Failed to initialize EGL display" << Qt::hex << eglGetError();
        m_eglDisplay = EGL_NO_DISPLAY;
        return;
    }

    m_supportsThreading = true;
    if (qEnvironmentVariableIsSet("QT_OPENGL_NO_SANITY_CHECK"))
        return;

    const char *vendor = eglQueryString(m_eglDisplay, EGL_VENDOR);
    for (int i = 0; qwaylandegl_threadedgl_blacklist_vendor[i]; ++i) {
        if (strstr(vendor, qwaylandegl_threadedgl_blacklist_vendor[i]) != 0) {
            m_supportsThreading = false;
            break;
        }
    }

    // On desktop NVIDIA resizing QtQuick freezes them when using threaded rendering QTBUG-95817
    // In order to support threaded rendering on embedded platforms where resizing is not needed
    // we check if XDG_CURRENT_DESKTOP is set which desktop environments should set
    if (qstrcmp(vendor, "NVIDIA") == 0 && qEnvironmentVariableIsSet("XDG_CURRENT_DESKTOP")) {
        m_supportsThreading = false;
    }
}

bool QWaylandEglClientBufferIntegration::isValid() const
{
    return m_eglDisplay != EGL_NO_DISPLAY;
}

bool QWaylandEglClientBufferIntegration::supportsThreadedOpenGL() const
{
    return m_supportsThreading;
}

bool QWaylandEglClientBufferIntegration::supportsWindowDecoration() const
{
    return true;
}

QWaylandWindow *QWaylandEglClientBufferIntegration::createEglWindow(QWindow *window)
{
    return new QWaylandEglWindow(window, m_display);
}

QPlatformOpenGLContext *QWaylandEglClientBufferIntegration::createPlatformOpenGLContext(const QSurfaceFormat &glFormat, QPlatformOpenGLContext *share) const
{
    QSurfaceFormat fmt = glFormat;
    if (m_display->supportsWindowDecoration())
        fmt.setAlphaBufferSize(8);
    return new QWaylandGLContext(m_eglDisplay, m_display, fmt, share);
}

QOpenGLContext *QWaylandEglClientBufferIntegration::createOpenGLContext(EGLContext context, EGLDisplay contextDisplay, QOpenGLContext *shareContext) const
{
    return QEGLPlatformContext::createFrom<QWaylandGLContext>(context, contextDisplay, m_eglDisplay, shareContext);
}

bool QWaylandEglClientBufferIntegration::canCreatePlatformOffscreenSurface() const
{
    return true;
}

QPlatformOffscreenSurface *QWaylandEglClientBufferIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    return new QEGLPbuffer(m_eglDisplay, surface->requestedFormat(), surface);
}

void *QWaylandEglClientBufferIntegration::nativeResource(NativeResource resource)
{
    switch (resource) {
    case EglDisplay:
        return m_eglDisplay;
    default:
        break;
    }
    return nullptr;
}

void *QWaylandEglClientBufferIntegration::nativeResourceForContext(NativeResource resource, QPlatformOpenGLContext *context)
{
    Q_ASSERT(context);
    switch (resource) {
    case EglConfig:
        return static_cast<QWaylandGLContext *>(context)->eglConfig();
    case EglContext:
        return static_cast<QWaylandGLContext *>(context)->eglContext();
    case EglDisplay:
        return m_eglDisplay;
    default:
        break;
    }
    return nullptr;
}

EGLDisplay QWaylandEglClientBufferIntegration::eglDisplay() const
{
    return m_eglDisplay;
}

}

QT_END_NAMESPACE
