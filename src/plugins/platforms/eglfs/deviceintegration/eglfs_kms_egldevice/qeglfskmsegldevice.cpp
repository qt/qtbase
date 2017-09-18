/****************************************************************************
**
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

    int fd = drmOpen(devicePath().toLocal8Bit().constData(), nullptr);
    if (Q_UNLIKELY(fd < 0))
        qFatal("Could not open DRM (NV) device");

    setFd(fd);

    return true;
}

void QEglFSKmsEglDevice::close()
{
    // Note: screens are gone at this stage.

    if (qt_safe_close(fd()) == -1)
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
