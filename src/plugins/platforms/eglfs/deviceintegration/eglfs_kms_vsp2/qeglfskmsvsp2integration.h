// Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEGLFSKMSVSP2INTEGRATION_H
#define QEGLFSKMSVSP2INTEGRATION_H

#include "qeglfskmsintegration_p.h"
#include <QtCore/QMap>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE

class QEglFSKmsDevice;

class QEglFSKmsVsp2Integration : public QEglFSKmsIntegration
{
public:
    QEglFSKmsVsp2Integration();

    EGLDisplay createDisplay(EGLNativeDisplayType nativeDisplay) override;
    EGLNativeWindowType createNativeOffscreenWindow(const QSurfaceFormat &format) override;
    void destroyNativeWindow(EGLNativeWindowType window) override;

    void presentBuffer(QPlatformSurface *surface) override;
    QEglFSWindow *createWindow(QWindow *window) const override;

protected:
    QKmsDevice *createDevice() override;
};

QT_END_NAMESPACE

#endif // QEGLFSKMSVSP2INTEGRATION_H
