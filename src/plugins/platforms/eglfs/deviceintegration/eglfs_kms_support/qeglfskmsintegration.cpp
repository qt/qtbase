// Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeglfskmsintegration_p.h"
#include "qeglfskmsscreen_p.h"

#include <QtKmsSupport/private/qkmsdevice_p.h>

#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/QScreen>

#include <xf86drm.h>
#include <xf86drmMode.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcEglfsKmsDebug, "qt.qpa.eglfs.kms")

QEglFSKmsIntegration::QEglFSKmsIntegration()
    : m_device(nullptr)
{
}

QEglFSKmsIntegration::~QEglFSKmsIntegration()
{
}

void QEglFSKmsIntegration::platformInit()
{
    qCDebug(qLcEglfsKmsDebug, "platformInit: Load Screen Config");
    m_screenConfig = createScreenConfig();

    qCDebug(qLcEglfsKmsDebug, "platformInit: Opening DRM device");
    m_device = createDevice();
    if (Q_UNLIKELY(!m_device->open()))
        qFatal("Could not open DRM device");
}

void QEglFSKmsIntegration::platformDestroy()
{
    qCDebug(qLcEglfsKmsDebug, "platformDestroy: Closing DRM device");
    m_device->close();
    delete m_device;
    m_device = nullptr;
    delete m_screenConfig;
    m_screenConfig = nullptr;
}

EGLNativeDisplayType QEglFSKmsIntegration::platformDisplay() const
{
    Q_ASSERT(m_device);
    return (EGLNativeDisplayType) m_device->nativeDisplay();
}

bool QEglFSKmsIntegration::usesDefaultScreen()
{
    return false;
}

void QEglFSKmsIntegration::screenInit()
{
    m_device->createScreens();
}

QSurfaceFormat QEglFSKmsIntegration::surfaceFormatFor(const QSurfaceFormat &inputFormat) const
{
    QSurfaceFormat format(inputFormat);
    format.setRenderableType(QSurfaceFormat::OpenGLES);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);
    return format;
}

bool QEglFSKmsIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case QPlatformIntegration::ThreadedPixmaps:
    case QPlatformIntegration::OpenGL:
    case QPlatformIntegration::ThreadedOpenGL:
        return true;
    default:
        return false;
    }
}

void QEglFSKmsIntegration::waitForVSync(QPlatformSurface *surface) const
{
    QWindow *window = static_cast<QWindow *>(surface->surface());
    QEglFSKmsScreen *screen = static_cast<QEglFSKmsScreen *>(window->screen()->handle());

    screen->waitForFlip();
}

bool QEglFSKmsIntegration::supportsPBuffers() const
{
    return m_screenConfig->supportsPBuffers();
}

void *QEglFSKmsIntegration::nativeResourceForIntegration(const QByteArray &name)
{
    if (name == QByteArrayLiteral("dri_fd") && m_device)
        return (void *) (qintptr) m_device->fd();

#if QT_CONFIG(drm_atomic)
    if (name == QByteArrayLiteral("dri_atomic_request") && m_device)
        return (void *) (qintptr) m_device->threadLocalAtomicRequest();
#endif
    return nullptr;
}

void *QEglFSKmsIntegration::nativeResourceForScreen(const QByteArray &resource, QScreen *screen)
{
    QEglFSKmsScreen *s = static_cast<QEglFSKmsScreen *>(screen->handle());
    if (s) {
        if (resource == QByteArrayLiteral("dri_crtcid"))
            return (void *) (qintptr) s->output().crtc_id;
        if (resource == QByteArrayLiteral("dri_connectorid"))
            return (void *) (qintptr) s->output().connector_id;
    }
    return nullptr;
}

QKmsDevice *QEglFSKmsIntegration::device() const
{
    return m_device;
}

QKmsScreenConfig *QEglFSKmsIntegration::screenConfig() const
{
    return m_screenConfig;
}

QKmsScreenConfig *QEglFSKmsIntegration::createScreenConfig()
{
    QKmsScreenConfig *screenConfig = new QKmsScreenConfig;
    screenConfig->loadConfig();

    return screenConfig;
}

QT_END_NAMESPACE
