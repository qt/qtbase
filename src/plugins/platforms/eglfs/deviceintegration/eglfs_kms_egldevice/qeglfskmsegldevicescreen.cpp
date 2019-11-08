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

#include "qeglfskmsegldevicescreen.h"
#include "qeglfskmsegldevice.h"
#include <QGuiApplication>
#include <QLoggingCategory>
#include <errno.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEglfsKmsDebug)

QEglFSKmsEglDeviceScreen::QEglFSKmsEglDeviceScreen(QEglFSKmsDevice *device, const QKmsOutput &output)
    : QEglFSKmsScreen(device, output)
{
}

QEglFSKmsEglDeviceScreen::~QEglFSKmsEglDeviceScreen()
{
    const int remainingScreenCount = qGuiApp->screens().count();
    qCDebug(qLcEglfsKmsDebug, "Screen dtor. Remaining screens: %d", remainingScreenCount);
    if (!remainingScreenCount && !device()->screenConfig()->separateScreens())
        static_cast<QEglFSKmsEglDevice *>(device())->destroyGlobalCursor();
}

QPlatformCursor *QEglFSKmsEglDeviceScreen::cursor() const
{
    // The base class creates a cursor via integration->createCursor()
    // in its ctor. With separateScreens just use that. Otherwise
    // there's a virtual desktop and the device has a global cursor
    // and the base class has no dedicated cursor at all.
    // config->hwCursor() is ignored for now, just use the standard OpenGL cursor.
    return device()->screenConfig()->separateScreens()
        ? QEglFSScreen::cursor()
        : static_cast<QEglFSKmsEglDevice *>(device())->globalCursor();
}

void QEglFSKmsEglDeviceScreen::waitForFlip()
{
    QKmsOutput &op(output());
    const int fd = device()->fd();
    const uint32_t w = op.modes[op.mode].hdisplay;
    const uint32_t h = op.modes[op.mode].vdisplay;

    if (!op.mode_set) {
        op.mode_set = true;

        drmModeCrtcPtr currentMode = drmModeGetCrtc(fd, op.crtc_id);
        const bool alreadySet = currentMode && currentMode->width == w && currentMode->height == h;
        if (currentMode)
            drmModeFreeCrtc(currentMode);
        if (alreadySet) {
            // Maybe detecting the DPMS mode could help here, but there are no properties
            // exposed on the connector apparently. So rely on an env var for now.
            static bool alwaysDoSet = qEnvironmentVariableIntValue("QT_QPA_EGLFS_ALWAYS_SET_MODE");
            if (!alwaysDoSet) {
                qCDebug(qLcEglfsKmsDebug, "Mode already set");
                return;
            }
        }

        qCDebug(qLcEglfsKmsDebug, "Setting mode");
        int ret = drmModeSetCrtc(fd, op.crtc_id,
                                 uint32_t(-1), 0, 0,
                                 &op.connector_id, 1,
                                 &op.modes[op.mode]);
        if (ret)
            qErrnoWarning(errno, "drmModeSetCrtc failed");
    }

    if (!op.forced_plane_set) {
        op.forced_plane_set = true;

        if (op.wants_forced_plane) {
            qCDebug(qLcEglfsKmsDebug, "Setting plane %u", op.forced_plane_id);
            int ret = drmModeSetPlane(fd, op.forced_plane_id, op.crtc_id, uint32_t(-1), 0,
                                      0, 0, w, h,
                                      0 << 16, 0 << 16, op.size.width() << 16, op.size.height() << 16);
            if (ret == -1)
                qErrnoWarning(errno, "drmModeSetPlane failed");
        }
    }
}

QT_END_NAMESPACE
