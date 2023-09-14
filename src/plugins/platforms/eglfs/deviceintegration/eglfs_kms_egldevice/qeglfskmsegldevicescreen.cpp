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
    , m_default_fb_handle(uint32_t(-1))
    , m_default_fb_id(uint32_t(-1))
{
    const int fd = device->fd();

    struct drm_mode_create_dumb createRequest;
    createRequest.width = output.size.width();
    createRequest.height = output.size.height();
    createRequest.bpp = 32;
    createRequest.flags = 0;

    qCDebug(qLcEglfsKmsDebug, "Creating dumb fb %dx%d", createRequest.width, createRequest.height);

    int ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &createRequest);
    if (ret < 0)
        qFatal("Unable to create dumb buffer.\n");

    m_default_fb_handle = createRequest.handle;

    uint32_t handles[4] = { 0, 0, 0, 0 };
    uint32_t pitches[4] = { 0, 0, 0, 0 };
    uint32_t offsets[4] = { 0, 0, 0, 0 };

    handles[0] = createRequest.handle;
    pitches[0] = createRequest.pitch;
    offsets[0] = 0;

    ret = drmModeAddFB2(fd, createRequest.width, createRequest.height, DRM_FORMAT_ARGB8888, handles,
                        pitches, offsets, &m_default_fb_id, 0);
    if (ret)
        qFatal("Unable to add fb\n");

    qCDebug(qLcEglfsKmsDebug, "Added dumb fb %dx%d handle:%u pitch:%d id:%u", createRequest.width, createRequest.height,
        createRequest.handle, createRequest.pitch, m_default_fb_id);
}

QEglFSKmsEglDeviceScreen::~QEglFSKmsEglDeviceScreen()
{
    int ret;
    const int fd = device()->fd();

    if (m_default_fb_id != uint32_t(-1)) {
        ret = drmModeRmFB(fd, m_default_fb_id);
        if (ret)
            qErrnoWarning("drmModeRmFB failed");
    }

    if (m_default_fb_handle != uint32_t(-1)) {
        struct drm_mode_destroy_dumb destroyRequest;
        destroyRequest.handle = m_default_fb_handle;

        ret = drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroyRequest);
        if (ret)
            qErrnoWarning("DRM_IOCTL_MODE_DESTROY_DUMB failed");
    }

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
            // Note that typically, we need to set crtc with the default fb even if the
            // mode did not change, unless QT_QPA_EGLFS_ALWAYS_SET_MODE is explicitly specified.
            static bool envVarSet = false;
            static bool alwaysDoSet = qEnvironmentVariableIntValue("QT_QPA_EGLFS_ALWAYS_SET_MODE", &envVarSet);
            if (envVarSet && !alwaysDoSet) {
                qCDebug(qLcEglfsKmsDebug, "Mode already set");
                return;
            }
        }

        qCDebug(qLcEglfsKmsDebug, "Setting mode");
        int ret = drmModeSetCrtc(fd, op.crtc_id,
                                 m_default_fb_id, 0, 0,
                                 &op.connector_id, 1,
                                 &op.modes[op.mode]);
        if (ret)
            qErrnoWarning(errno, "drmModeSetCrtc failed");
    }

    if (!op.forced_plane_set) {
        op.forced_plane_set = true;

        if (op.wants_forced_plane) {
            qCDebug(qLcEglfsKmsDebug, "Setting plane %u", op.forced_plane_id);
            int ret = drmModeSetPlane(fd, op.forced_plane_id, op.crtc_id, m_default_fb_id, 0,
                                      0, 0, w, h,
                                      0 << 16, 0 << 16, op.size.width() << 16, op.size.height() << 16);
            if (ret == -1)
                qErrnoWarning(errno, "drmModeSetPlane failed");
        }
    }
}

QT_END_NAMESPACE
