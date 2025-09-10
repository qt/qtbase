// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEGLFSKMSEGLDEVICEINTEGRATION_H
#define QEGLFSKMSEGLDEVICEINTEGRATION_H

#include <private/qeglfskmsintegration_p.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <QtGui/private/qeglstreamconvenience_p.h>

QT_BEGIN_NAMESPACE

class QEglFSKmsEglDeviceIntegration : public QEglFSKmsIntegration
{
public:
    QEglFSKmsEglDeviceIntegration();

    QSurfaceFormat surfaceFormatFor(const QSurfaceFormat &inputFormat) const override;
    EGLint surfaceType() const override;
    EGLDisplay createDisplay(EGLNativeDisplayType nativeDisplay) override;
    bool supportsSurfacelessContexts() const override;
    bool supportsPBuffers() const override;
    QEglFSWindow *createWindow(QWindow *window) const override;

    EGLDeviceEXT eglDevice() const { return m_egl_device; }

protected:
    QKmsDevice *createDevice() override;
    QPlatformCursor *createCursor(QPlatformScreen *screen) const override;

private:
    bool setup_kms();
    bool query_egl_device();

    EGLDeviceEXT m_egl_device;
    QEGLStreamConvenience *m_funcs;

    friend class QEglFSKmsEglDeviceWindow;
};

QT_END_NAMESPACE

#endif
