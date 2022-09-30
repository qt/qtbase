// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    const int remainingScreenCount = qGuiApp->screens().size();
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
