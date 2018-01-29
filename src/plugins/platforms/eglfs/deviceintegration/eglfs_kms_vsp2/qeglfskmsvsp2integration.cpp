/****************************************************************************
**
** Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2016 Pelagicore AG
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

#include "qeglfskmsvsp2integration.h"
#include "qeglfskmsvsp2device.h"
#include "qeglfskmsvsp2screen.h"
#include "private/qeglfswindow_p.h"

#include <QtDeviceDiscoverySupport/private/qdevicediscovery_p.h>
#include <QtEglSupport/private/qeglconvenience_p.h>
#include <QtCore/QLoggingCategory>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/QScreen>
#include <QtPlatformHeaders/qeglfsfunctions.h>

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

QFunctionPointer QEglFSKmsVsp2Integration::platformFunction(const QByteArray &function) const
{
    if (function == QEglFSFunctions::vsp2AddLayerTypeIdentifier())
        return QFunctionPointer(addLayerStatic);
    if (function == QEglFSFunctions::vsp2RemoveLayerTypeIdentifier())
        return QFunctionPointer(removeLayerStatic);
    if (function == QEglFSFunctions::vsp2SetLayerBufferTypeIdentifier())
        return QFunctionPointer(setLayerBufferStatic);
    if (function == QEglFSFunctions::vsp2SetLayerPositionTypeIdentifier())
        return QFunctionPointer(setLayerPositionStatic);
    if (function == QEglFSFunctions::vsp2SetLayerAlphaTypeIdentifier())
        return QFunctionPointer(setLayerAlphaStatic);
    if (function == QEglFSFunctions::vsp2AddBlendListenerTypeIdentifier())
        return QFunctionPointer(addBlendListenerStatic);

    return nullptr;
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

int QEglFSKmsVsp2Integration::addLayerStatic(const QScreen *screen, int dmabufFd, const QSize &size, const QPoint &position, uint pixelFormat, uint bytesPerLine)
{
    auto vsp2Screen = static_cast<QEglFSKmsVsp2Screen *>(screen->handle());
    return vsp2Screen->addLayer(dmabufFd, size, position, pixelFormat, bytesPerLine);
}

bool QEglFSKmsVsp2Integration::removeLayerStatic(const QScreen *screen, int id)
{
    auto vsp2Screen = static_cast<QEglFSKmsVsp2Screen *>(screen->handle());
    return vsp2Screen->removeLayer(id);
}

void QEglFSKmsVsp2Integration::setLayerBufferStatic(const QScreen *screen, int id, int dmabufFd)
{
    auto vsp2Screen = static_cast<QEglFSKmsVsp2Screen *>(screen->handle());
    vsp2Screen->setLayerBuffer(id, dmabufFd);
}

void QEglFSKmsVsp2Integration::setLayerPositionStatic(const QScreen *screen, int id, const QPoint &position)
{
    auto vsp2Screen = static_cast<QEglFSKmsVsp2Screen *>(screen->handle());
    vsp2Screen->setLayerPosition(id, position);
}

void QEglFSKmsVsp2Integration::setLayerAlphaStatic(const QScreen *screen, int id, qreal alpha)
{
    auto vsp2Screen = static_cast<QEglFSKmsVsp2Screen *>(screen->handle());
    vsp2Screen->setLayerAlpha(id, alpha);
}

void QEglFSKmsVsp2Integration::addBlendListenerStatic(const QScreen *screen, void(*callback)())
{
    auto vsp2Screen = static_cast<QEglFSKmsVsp2Screen *>(screen->handle());
    vsp2Screen->addBlendListener(callback);
}

class QEglFSKmsVsp2Window : public QEglFSWindow
{
public:
    QEglFSKmsVsp2Window(QWindow *w, const QEglFSKmsVsp2Integration *integration)
        : QEglFSWindow(w)
        , m_integration(integration)
    {}
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
