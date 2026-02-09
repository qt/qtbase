// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandbrcmeglintegration.h"

#include <QtWaylandClient/private/qwaylandclientbufferintegration_p.h>

#include "qwaylandbrcmeglwindow.h"
#include "qwaylandbrcmglcontext.h"

#include <QtCore/QDebug>

#include "wayland-brcm-client-protocol.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandBrcmEglIntegration::QWaylandBrcmEglIntegration()
{
    qDebug() << "Using Brcm-EGL";
}

void QWaylandBrcmEglIntegration::wlDisplayHandleGlobal(void *data, struct ::wl_registry *registry, uint32_t id, const QString &interface, uint32_t version)
{
    Q_UNUSED(version);
    if (interface == "qt_brcm") {
        QWaylandBrcmEglIntegration *integration = static_cast<QWaylandBrcmEglIntegration *>(data);
        integration->m_waylandBrcm = static_cast<struct qt_brcm *>(wl_registry_bind(registry, id, &qt_brcm_interface, 1));
    }
}

qt_brcm *QWaylandBrcmEglIntegration::waylandBrcm() const
{
    return m_waylandBrcm;
}

QWaylandBrcmEglIntegration::~QWaylandBrcmEglIntegration()
{
    eglTerminate(m_eglDisplay);
}

void QWaylandBrcmEglIntegration::initialize(QWaylandDisplay *waylandDisplay)
{
    m_display = waylandDisplay;
    m_waylandDisplay = waylandDisplay->wl_display();
    waylandDisplay->addRegistryListener(wlDisplayHandleGlobal, this);
    EGLint major,minor;
    m_eglDisplay = eglGetDisplay((EGLNativeDisplayType)EGL_DEFAULT_DISPLAY);
    if (m_eglDisplay == NULL) {
        qWarning("EGL not available");
    } else {
        if (!eglInitialize(m_eglDisplay, &major, &minor)) {
            qWarning("failed to initialize EGL display");
            return;
        }

        eglFlushBRCM = (PFNEGLFLUSHBRCMPROC)eglGetProcAddress("eglFlushBRCM");
        if (!eglFlushBRCM) {
            qWarning("failed to resolve eglFlushBRCM, performance will suffer");
        }

        eglCreateGlobalImageBRCM = (PFNEGLCREATEGLOBALIMAGEBRCMPROC)eglGetProcAddress("eglCreateGlobalImageBRCM");
        if (!eglCreateGlobalImageBRCM) {
            qWarning("failed to resolve eglCreateGlobalImageBRCM");
            return;
        }

        eglDestroyGlobalImageBRCM = (PFNEGLDESTROYGLOBALIMAGEBRCMPROC)eglGetProcAddress("eglDestroyGlobalImageBRCM");
        if (!eglDestroyGlobalImageBRCM) {
            qWarning("failed to resolve eglDestroyGlobalImageBRCM");
            return;
        }
    }
}

QWaylandWindow *QWaylandBrcmEglIntegration::createEglWindow(QWindow *window)
{
    return new QWaylandBrcmEglWindow(window, m_display);
}

QPlatformOpenGLContext *QWaylandBrcmEglIntegration::createPlatformOpenGLContext(const QSurfaceFormat &glFormat, QPlatformOpenGLContext *share) const
{
    return new QWaylandBrcmGLContext(m_eglDisplay, glFormat, share);
}

QOpenGLContext *QWaylandBrcmEglIntegration::createOpenGLContext(EGLContext context, EGLDisplay contextDisplay, QOpenGLContext *shareContext) const
{
    return QEGLPlatformContext::createFrom<QWaylandBrcmGLContext>(context, contextDisplay, m_eglDisplay, shareContext);
}

EGLDisplay QWaylandBrcmEglIntegration::eglDisplay() const
{
    return m_eglDisplay;
}

void *QWaylandBrcmEglIntegration::nativeResource(NativeResource resource)
{
    switch (resource) {
    case EglDisplay:
        return m_eglDisplay;
    default:
        break;
    }
    return nullptr;
}

void *QWaylandBrcmEglIntegration::nativeResourceForContext(NativeResource resource, QPlatformOpenGLContext *context)
{
    Q_ASSERT(context);
    switch (resource) {
    case EglConfig:
        return static_cast<QWaylandBrcmGLContext *>(context)->eglConfig();
    case EglContext:
        return static_cast<QWaylandBrcmGLContext *>(context)->eglContext();
    case EglDisplay:
        return m_eglDisplay;
    default:
        break;
    }
    return nullptr;
}

}

QT_END_NAMESPACE
