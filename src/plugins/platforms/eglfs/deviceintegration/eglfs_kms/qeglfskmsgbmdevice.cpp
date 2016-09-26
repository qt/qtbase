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
#include <QtGui/private/qguiapplication_p.h>

#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEglfsKmsDebug)

void QEglFSKmsGbmDevice::pageFlipHandler(int fd, unsigned int sequence, unsigned int tv_sec, unsigned int tv_usec, void *user_data)
{
    Q_UNUSED(fd);
    Q_UNUSED(sequence);
    Q_UNUSED(tv_sec);
    Q_UNUSED(tv_usec);

    QEglFSKmsScreen *screen = static_cast<QEglFSKmsScreen *>(user_data);
    screen->flipFinished();
}

QEglFSKmsGbmDevice::QEglFSKmsGbmDevice(QEglFSKmsIntegration *integration, const QString &path)
    : QEglFSKmsDevice(integration, path)
    , m_gbm_device(Q_NULLPTR)
    , m_globalCursor(Q_NULLPTR)
{
}

bool QEglFSKmsGbmDevice::open()
{
    Q_ASSERT(fd() == -1);
    Q_ASSERT(m_gbm_device == Q_NULLPTR);

    qCDebug(qLcEglfsKmsDebug) << "Opening device" << devicePath();
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
    if (m_gbm_device) {
        gbm_device_destroy(m_gbm_device);
        m_gbm_device = Q_NULLPTR;
    }

    if (fd() != -1) {
        qt_safe_close(fd());
        setFd(-1);
    }

    if (m_globalCursor)
        m_globalCursor->deleteLater();
    m_globalCursor = Q_NULLPTR;
}

EGLNativeDisplayType QEglFSKmsGbmDevice::nativeDisplay() const
{
    return reinterpret_cast<EGLNativeDisplayType>(m_gbm_device);
}

gbm_device * QEglFSKmsGbmDevice::gbmDevice() const
{
    return m_gbm_device;
}

QPlatformCursor *QEglFSKmsGbmDevice::globalCursor() const
{
    return m_globalCursor;
}

void QEglFSKmsGbmDevice::handleDrmEvent()
{
    drmEventContext drmEvent = {
        DRM_EVENT_CONTEXT_VERSION,
        Q_NULLPTR,      // vblank handler
        pageFlipHandler // page flip handler
    };

    drmHandleEvent(fd(), &drmEvent);
}

QEglFSKmsScreen *QEglFSKmsGbmDevice::createScreen(QEglFSKmsIntegration *integration, QEglFSKmsDevice *device, QEglFSKmsOutput output)
{
    static bool firstScreen = true;
    QEglFSKmsGbmScreen *screen = new QEglFSKmsGbmScreen(integration, device, output);

    if (firstScreen && integration->hwCursor()) {
        m_globalCursor = new QEglFSKmsGbmCursor(screen);
        firstScreen = false;
    }

    return screen;
}

QT_END_NAMESPACE
