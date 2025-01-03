// Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeglfskmsvsp2integration.h"
#include "qeglfskmsvsp2device.h"
#include "qeglfskmsvsp2screen.h"
#include "private/qeglfswindow_p.h"

#include <QtDeviceDiscoverySupport/private/qdevicediscovery_p.h>
#include <QtGui/private/qeglconvenience_p.h>
#include <QtCore/QLoggingCategory>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/QScreen>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

QT_BEGIN_NAMESPACE

QEglFSKmsVsp2Integration::QEglFSKmsVsp2Integration()
{
    qCDebug(qLcEglfsKmsDebug, "New DRM/KMS via Vsp2 integration created");
}

#ifndef EGL_EXT_platform_base
typedef EGLDisplay (EGLAPIENTRYP PFNEGLGETPLATFORMDISPLAYEXTPROC) (EGLenum platform, void *native_display, const EGLint *attrib_list);
typedef EGLSurface (EGLAPIENTRYP PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC) (EGLDisplay dpy, EGLConfig config, void *native_window, const EGLint *attrib_list);
#endif

#ifndef EGL_PLATFORM_GBM_KHR
#define EGL_PLATFORM_GBM_KHR 0x31D7
#endif

EGLDisplay QEglFSKmsVsp2Integration::createDisplay(EGLNativeDisplayType nativeDisplay)
{
    qCDebug(qLcEglfsKmsDebug, "Querying EGLDisplay");
    EGLDisplay display;

    PFNEGLGETPLATFORMDISPLAYEXTPROC getPlatformDisplay = nullptr;
    const char *extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if (extensions && (strstr(extensions, "EGL_KHR_platform_gbm") || strstr(extensions, "EGL_MESA_platform_gbm"))) {
        getPlatformDisplay = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
            eglGetProcAddress("eglGetPlatformDisplayEXT"));
    }

    if (getPlatformDisplay) {
        display = getPlatformDisplay(EGL_PLATFORM_GBM_KHR, nativeDisplay, nullptr);
    } else {
        qCDebug(qLcEglfsKmsDebug, "No eglGetPlatformDisplay for GBM, falling back to eglGetDisplay");
        display = eglGetDisplay(nativeDisplay);
    }

    return display;
}

EGLNativeWindowType QEglFSKmsVsp2Integration::createNativeOffscreenWindow(const QSurfaceFormat &format)
{
    Q_UNUSED(format);
    Q_ASSERT(device());

    gbm_surface *surface = gbm_surface_create(static_cast<QEglFSKmsVsp2Device *>(device())->gbmDevice(),
                                              1, 1,
                                              GBM_FORMAT_XRGB8888,
                                              GBM_BO_USE_RENDERING);

    return reinterpret_cast<EGLNativeWindowType>(surface);
}

void QEglFSKmsVsp2Integration::destroyNativeWindow(EGLNativeWindowType window)
{
    gbm_surface *surface = reinterpret_cast<gbm_surface *>(window);
    //TODO call screen destroysurface instead
    gbm_surface_destroy(surface);
}

void QEglFSKmsVsp2Integration::presentBuffer(QPlatformSurface *surface)
{
    QWindow *window = static_cast<QWindow *>(surface->surface());
    auto *screen = static_cast<QEglFSKmsVsp2Screen *>(window->screen()->handle());
    screen->flip();
}

QKmsDevice *QEglFSKmsVsp2Integration::createDevice()
{
    QString path = screenConfig()->devicePath();
    if (!path.isEmpty()) {
        qCDebug(qLcEglfsKmsDebug) << "VSP2: Using DRM device" << path << "specified in config file";
    } else {
        QDeviceDiscovery *d = QDeviceDiscovery::create(QDeviceDiscovery::Device_VideoMask);
        const QStringList devices = d->scanConnectedDevices();
        qCDebug(qLcEglfsKmsDebug) << "Found the following video devices:" << devices;
        d->deleteLater();

        if (Q_UNLIKELY(devices.isEmpty()))
            qFatal("Could not find DRM device!");

        path = devices.first();
        qCDebug(qLcEglfsKmsDebug) << "Using" << path;
    }

    return new QEglFSKmsVsp2Device(screenConfig(), path);
}

class QEglFSKmsVsp2Window : public QEglFSWindow
{
public:
    QEglFSKmsVsp2Window(QWindow *w, const QEglFSKmsVsp2Integration *integration)
        : QEglFSWindow(w)
        , m_integration(integration)
    {}

    ~QEglFSKmsVsp2Window() { destroy(); }

    void resetSurface() override;
    void invalidateSurface() override;
    const QEglFSKmsVsp2Integration *m_integration;
};

void QEglFSKmsVsp2Window::resetSurface()
{
    auto *vsp2Screen = static_cast<QEglFSKmsVsp2Screen *>(screen());
    EGLDisplay display = vsp2Screen->display();
    QSurfaceFormat platformFormat = m_integration->surfaceFormatFor(window()->requestedFormat());
    m_config = QEglFSDeviceIntegration::chooseConfig(display, platformFormat);
    m_format = q_glFormatFromConfig(display, m_config, platformFormat);
    // One fullscreen window per screen -> the native window is simply the gbm_surface the screen created.
    m_window = reinterpret_cast<EGLNativeWindowType>(vsp2Screen->createSurface());

    PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC createPlatformWindowSurface = nullptr;
    const char *extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if (extensions && (strstr(extensions, "EGL_KHR_platform_gbm") || strstr(extensions, "EGL_MESA_platform_gbm"))) {
        createPlatformWindowSurface = reinterpret_cast<PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC>(
            eglGetProcAddress("eglCreatePlatformWindowSurfaceEXT"));
    }

    if (createPlatformWindowSurface) {
        m_surface = createPlatformWindowSurface(display, m_config, reinterpret_cast<void *>(m_window), nullptr);
    } else {
        qCDebug(qLcEglfsKmsDebug, "No eglCreatePlatformWindowSurface for GBM, falling back to eglCreateWindowSurface");
        m_surface = eglCreateWindowSurface(display, m_config, m_window, nullptr);
    }
}

void QEglFSKmsVsp2Window::invalidateSurface()
{
    auto *vsp2Screen = static_cast<QEglFSKmsVsp2Screen *>(screen());
    QEglFSWindow::invalidateSurface();
    vsp2Screen->resetSurface();
}

QEglFSWindow *QEglFSKmsVsp2Integration::createWindow(QWindow *window) const
{
    return new QEglFSKmsVsp2Window(window, this);
}

QT_END_NAMESPACE
