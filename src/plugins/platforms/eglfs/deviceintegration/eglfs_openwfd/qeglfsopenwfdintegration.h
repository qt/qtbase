/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
};

QT_END_NAMESPACE

#endif
