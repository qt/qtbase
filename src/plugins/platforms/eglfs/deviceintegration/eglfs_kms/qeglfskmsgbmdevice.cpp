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

#include "qeglfskmsgbmdevice.h"
#include "qeglfskmsgbmscreen.h"

#include "qeglfsintegration_p.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/private/qcore_unix_p.h>

#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEglfsKmsDebug)

QEglFSKmsGbmDevice::QEglFSKmsGbmDevice(QKmsScreenConfig *screenConfig, const QString &path)
    : QEglFSKmsDevice(screenConfig, path)
    , m_gbm_device(nullptr)
    , m_globalCursor(nullptr)
{
}

bool QEglFSKmsGbmDevice::open()
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

void QEglFSKmsGbmDevice::close()
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

void *QEglFSKmsGbmDevice::nativeDisplay() const
{
    return m_gbm_device;
}

gbm_device * QEglFSKmsGbmDevice::gbmDevice() const
{
    return m_gbm_device;
}

QPlatformCursor *QEglFSKmsGbmDevice::globalCursor() const
{
    return m_globalCursor;
}

// Cannot do this from close(), it may be too late.
// Call this from the last screen dtor instead.
void QEglFSKmsGbmDevice::destroyGlobalCursor()
{
    if (m_globalCursor) {
        qCDebug(qLcEglfsKmsDebug, "Destroying global GBM mouse cursor");
        delete m_globalCursor;
        m_globalCursor = nullptr;
    }
}

QPlatformScreen *QEglFSKmsGbmDevice::createScreen(const QKmsOutput &output)
{
    QEglFSKmsGbmScreen *screen = new QEglFSKmsGbmScreen(this, output, false);

    if (!m_globalCursor && screenConfig()->hwCursor()) {
        qCDebug(qLcEglfsKmsDebug, "Creating new global GBM mouse cursor");
        m_globalCursor = new QEglFSKmsGbmCursor(screen);
    }

    return screen;
}

QPlatformScreen *QEglFSKmsGbmDevice::createHeadlessScreen()
{
    return new QEglFSKmsGbmScreen(this, QKmsOutput(), true);
}

void QEglFSKmsGbmDevice::registerScreenCloning(QPlatformScreen *screen,
                                               QPlatformScreen *screenThisScreenClones,
                                               const QVector<QPlatformScreen *> &screensCloningThisScreen)
{
    if (!screenThisScreenClones && screensCloningThisScreen.isEmpty())
        return;

    QEglFSKmsGbmScreen *gbmScreen = static_cast<QEglFSKmsGbmScreen *>(screen);
    gbmScreen->initCloning(screenThisScreenClones, screensCloningThisScreen);
}

QT_END_NAMESPACE
