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

#include "qeglfskmsgbmintegration.h"
#include "qeglfskmsgbmdevice.h"
#include "qeglfskmsgbmscreen.h"
#include "qeglfskmsgbmcursor.h"
#include "private/qeglfscursor_p.h"

#include <QtDeviceDiscoverySupport/private/qdevicediscovery_p.h>
#include <QtCore/QLoggingCategory>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/qpa/qplatformcursor.h>
#include <QtGui/QScreen>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

QT_BEGIN_NAMESPACE

QMutex QEglFSKmsGbmScreen::m_waitForFlipMutex;

QEglFSKmsGbmIntegration::QEglFSKmsGbmIntegration()
    : QEglFSKmsIntegration()
{}

EGLNativeWindowType QEglFSKmsGbmIntegration::createNativeWindow(QPlatformWindow *platformWindow,
                                                     const QSize &size,
                                                     const QSurfaceFormat &format)
{
    Q_UNUSED(size);
    Q_UNUSED(format);

    QEglFSKmsGbmScreen *screen = static_cast<QEglFSKmsGbmScreen *>(platformWindow->screen());
    if (screen->surface()) {
        qWarning("Only single window per screen supported!");
        return 0;
    }

    return reinterpret_cast<EGLNativeWindowType>(screen->createSurface());
}

EGLNativeWindowType QEglFSKmsGbmIntegration::createNativeOffscreenWindow(const QSurfaceFormat &format)
{
    Q_UNUSED(format);
    Q_ASSERT(device());

    qCDebug(qLcEglfsKmsDebug) << "Creating native off screen window";
    gbm_surface *surface = gbm_surface_create(static_cast<QEglFSKmsGbmDevice *>(device())->gbmDevice(),
                                              1, 1,
                                              GBM_FORMAT_XRGB8888,
                                              GBM_BO_USE_RENDERING);

    return reinterpret_cast<EGLNativeWindowType>(surface);
}

void QEglFSKmsGbmIntegration::destroyNativeWindow(EGLNativeWindowType window)
{
    gbm_surface *surface = reinterpret_cast<gbm_surface *>(window);
    gbm_surface_destroy(surface);
}

QPlatformCursor *QEglFSKmsGbmIntegration::createCursor(QPlatformScreen *screen) const
{
    if (hwCursor())
        return Q_NULLPTR;
    else
        return new QEglFSCursor(screen);
}

void QEglFSKmsGbmIntegration::presentBuffer(QPlatformSurface *surface)
{
    QWindow *window = static_cast<QWindow *>(surface->surface());
    QEglFSKmsScreen *screen = static_cast<QEglFSKmsScreen *>(window->screen()->handle());

    screen->flip();
}

QEglFSKmsDevice *QEglFSKmsGbmIntegration::createDevice(const QString &devicePath)
{
    QString path = devicePath;
    if (!devicePath.isEmpty()) {
        qCDebug(qLcEglfsKmsDebug) << "Using DRM device" << path << "specified in config file";
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

    return new QEglFSKmsGbmDevice(this, path);
}

QT_END_NAMESPACE
