// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeglfskmsegldevice.h"
#include "qeglfskmsegldevicescreen.h"
#include "qeglfskmsegldeviceintegration.h"
#include "private/qeglfsintegration_p.h"
#include "private/qeglfscursor_p.h"

#include <QtCore/private/qcore_unix_p.h>

QT_BEGIN_NAMESPACE

QEglFSKmsEglDevice::QEglFSKmsEglDevice(QEglFSKmsEglDeviceIntegration *devInt, QKmsScreenConfig *screenConfig, const QString &path)
    : QEglFSKmsDevice(screenConfig, path),
      m_devInt(devInt),
      m_globalCursor(nullptr)
{
}

bool QEglFSKmsEglDevice::open()
{
    Q_ASSERT(fd() == -1);

    int fd = -1;

    if (devicePath().compare("drm-nvdc") == 0)
        fd = drmOpen(devicePath().toLocal8Bit().constData(), nullptr);
    else
        fd = qt_safe_open(devicePath().toLocal8Bit().constData(), O_RDWR);
    if (Q_UNLIKELY(fd < 0))
        qFatal("Could not open DRM (NV) device");

    setFd(fd);

    return true;
}

void QEglFSKmsEglDevice::close()
{
    // Note: screens are gone at this stage.

    if (drmClose(fd()) == -1)
        qErrnoWarning("Could not close DRM (NV) device");

    setFd(-1);
}

void *QEglFSKmsEglDevice::nativeDisplay() const
{
    return m_devInt->eglDevice();
}

QPlatformScreen *QEglFSKmsEglDevice::createScreen(const QKmsOutput &output)
{
    QEglFSKmsScreen *screen = new QEglFSKmsEglDeviceScreen(this, output);
#if QT_CONFIG(opengl)
    if (!m_globalCursor && !screenConfig()->separateScreens()) {
        qCDebug(qLcEglfsKmsDebug, "Creating new global mouse cursor");
        m_globalCursor = new QEglFSCursor(screen);
    }
#endif
    return screen;
}

void QEglFSKmsEglDevice::destroyGlobalCursor()
{
    if (m_globalCursor) {
        qCDebug(qLcEglfsKmsDebug, "Destroying global mouse cursor");
        delete m_globalCursor;
        m_globalCursor = nullptr;
    }
}

QT_END_NAMESPACE
