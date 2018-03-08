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
                                               const QVector<QPlatformScreen *> &screensCloningThisScreen)
{
    Q_UNUSED(screen);
    qWarning() << Q_FUNC_INFO << "Not implemented yet";
    if (!screenThisScreenClones && screensCloningThisScreen.isEmpty())
        return;

//    auto *vsp2Screen = static_cast<QEglFSKmsVsp2Screen *>(screen);
//    vsp2Screen->initCloning(screenThisScreenClones, screensCloningThisScreen);
}

QT_END_NAMESPACE
