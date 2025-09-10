// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEGLFSOPENWFDINTEGRATION_H
#define QEGLFSOPENWFDINTEGRATION_H

#include "private/qeglfsdeviceintegration_p.h"
#define WFD_WFDEXT_PROTOTYPES
#include "wfd.h"
#include "wfdext2.h"

QT_BEGIN_NAMESPACE

class QEglFSOpenWFDIntegration : public QEglFSDeviceIntegration
{
public:
    void platformInit() override;
    void platformDestroy() override;
    QSize screenSize() const override;
    EGLNativeWindowType createNativeWindow(QPlatformWindow *window, const QSize &size, const QSurfaceFormat &format) override;
    void destroyNativeWindow(EGLNativeWindowType window) override;
    EGLNativeDisplayType platformDisplay() const override;
    virtual QSurfaceFormat surfaceFormatFor(const QSurfaceFormat &inputFormat) const;

private:
    QSize mScreenSize;
    EGLNativeDisplayType mNativeDisplay;
    WFDDevice mDevice;
    WFDPort mPort;
    WFDPipeline mPipeline;
};

QT_END_NAMESPACE

#endif
