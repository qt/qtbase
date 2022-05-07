/****************************************************************************
**
** Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Copyright (C) 2022 The Qt Company Ltd.
** Copyright (C) 2016 Pelagicore AG
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
******************************************************************************/

#ifndef QEGLFSKMSGBMINTEGRATION_H
#define QEGLFSKMSGBMINTEGRATION_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qeglfskmsintegration_p.h>
#include <QtCore/QMap>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE

class QEglFSKmsDevice;

class Q_EGLFS_EXPORT QEglFSKmsGbmIntegration : public QEglFSKmsIntegration
{
public:
    QEglFSKmsGbmIntegration();

    EGLDisplay createDisplay(EGLNativeDisplayType nativeDisplay) override;
    EGLNativeWindowType createNativeOffscreenWindow(const QSurfaceFormat &format) override;
    void destroyNativeWindow(EGLNativeWindowType window) override;

    QPlatformCursor *createCursor(QPlatformScreen *screen) const override;
    void presentBuffer(QPlatformSurface *surface) override;
    QEglFSWindow *createWindow(QWindow *window) const override;

protected:
    QKmsDevice *createDevice() override;

private:
};

QT_END_NAMESPACE

#endif // QEGLFSKMSGBMINTEGRATION_H
