// Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeglfskmsvsp2device.h"
#include "qeglfskmsvsp2screen.h"

#include "qeglfsintegration_p.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/private/qcore_unix_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEglfsKmsDebug)

QEglFSKmsVsp2Device::QEglFSKmsVsp2Device(QKmsScreenConfig *screenConfig, const QString &path)
    : QEglFSKmsDevice(screenConfig, path)
{
}

bool QEglFSKmsVsp2Device::open()
{
    Q_ASSERT(fd() == -1);
    Q_ASSERT(m_gbm_device == nullptr);

    int fd = qt_safe_open(devicePath().toLocal8Bit().constData(), O_RDWR | O_CLOEXEC);
    if (fd == -1) {
        qErrnoWarning("Could not open DRM device %s", qPrintable(devicePath()));
        return false;
    }

    qCDebug(qLcEglfsKmsDebug) << "Creating GBM device for file descriptor" << fd
                              << "obtained from" << devicePath();
    m_gbm_device = gbm_create_device(fd);
    if (!m_gbm_device) {
        qErrnoWarning("Could not create GBM device");
        qt_safe_close(fd);
        fd = -1;
        return false;
    }

    setFd(fd);

    return true;
}

void QEglFSKmsVsp2Device::close()
{
    // Note: screens are gone at this stage.

    if (m_gbm_device) {
        gbm_device_destroy(m_gbm_device);
        m_gbm_device = nullptr;
    }

    if (fd() != -1) {
        qt_safe_close(fd());
        setFd(-1);
    }
}

void *QEglFSKmsVsp2Device::nativeDisplay() const
{
    return m_gbm_device;
}

gbm_device * QEglFSKmsVsp2Device::gbmDevice() const
{
    return m_gbm_device;
}

QPlatformScreen *QEglFSKmsVsp2Device::createScreen(const QKmsOutput &output)
{
    auto *screen = new QEglFSKmsVsp2Screen(this, output);

    return screen;
}

QPlatformScreen *QEglFSKmsVsp2Device::createHeadlessScreen()
{
    qWarning() << Q_FUNC_INFO << "Not implemented yet";
    return nullptr;
}

void QEglFSKmsVsp2Device::registerScreenCloning(QPlatformScreen *screen,
                                               QPlatformScreen *screenThisScreenClones,
                                               const QList<QPlatformScreen *> &screensCloningThisScreen)
{
    Q_UNUSED(screen);
    qWarning() << Q_FUNC_INFO << "Not implemented yet";
    if (!screenThisScreenClones && screensCloningThisScreen.isEmpty())
        return;

//    auto *vsp2Screen = static_cast<QEglFSKmsVsp2Screen *>(screen);
//    vsp2Screen->initCloning(screenThisScreenClones, screensCloningThisScreen);
}

QT_END_NAMESPACE
