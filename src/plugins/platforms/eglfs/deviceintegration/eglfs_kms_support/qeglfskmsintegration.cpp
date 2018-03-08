/****************************************************************************
**
** Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qeglfskmsintegration.h"
#include "qeglfskmsscreen.h"

#include <QtKmsSupport/private/qkmsdevice_p.h>

#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/QScreen>

#include <xf86drm.h>
#include <xf86drmMode.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcEglfsKmsDebug, "qt.qpa.eglfs.kms")

QEglFSKmsIntegration::QEglFSKmsIntegration()
    : m_device(nullptr),
      m_screenConfig(new QKmsScreenConfig)
{
}

QEglFSKmsIntegration::~QEglFSKmsIntegration()
{
    delete m_screenConfig;
}

void QEglFSKmsIntegration::platformInit()
{
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

    return nullptr;
}

void *QEglFSKmsIntegration::nativeResourceForScreen(const QByteArray &resource, QScreen *screen)
{
    QEglFSKmsScreen *s = static_cast<QEglFSKmsScreen *>(screen->handle());
    if (s) {
        if (resource == QByteArrayLiteral("dri_crtcid"))
            return (void *) (qintptr) s->output().crtc_id;
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

QT_END_NAMESPACE
