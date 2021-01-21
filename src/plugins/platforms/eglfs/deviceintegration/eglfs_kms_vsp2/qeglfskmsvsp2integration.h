/****************************************************************************
**
** Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QEGLFSKMSVSP2INTEGRATION_H
#define QEGLFSKMSVSP2INTEGRATION_H

#include "qeglfskmsintegration.h"
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

    QFunctionPointer platformFunction(const QByteArray &function) const override;

protected:
    QKmsDevice *createDevice() override;

private:
    static int addLayerStatic(const QScreen *screen, int dmabufFd, const QSize &size, const QPoint &position, uint pixelFormat, uint bytesPerLine);
    static bool removeLayerStatic(const QScreen *screen, int id);
    static void setLayerBufferStatic(const QScreen *screen, int id, int dmabufFd);
    static void setLayerPositionStatic(const QScreen *screen, int id, const QPoint &position);
    static void setLayerAlphaStatic(const QScreen *screen, int id, qreal alpha);
    static void addBlendListenerStatic(const QScreen *screen, void(*callback)());
};

QT_END_NAMESPACE

#endif // QEGLFSKMSVSP2INTEGRATION_H
