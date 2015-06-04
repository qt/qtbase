/****************************************************************************
**
** Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QEGLFSKMSINTEGRATION_H
#define QEGLFSKMSINTEGRATION_H

#include "qeglfsdeviceintegration.h"
#include <QtCore/QMap>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE

class QEglFSKmsDevice;

class QEglFSKmsIntegration : public QEGLDeviceIntegration
{
public:
    QEglFSKmsIntegration();

    void platformInit() Q_DECL_OVERRIDE;
    void platformDestroy() Q_DECL_OVERRIDE;
    EGLNativeDisplayType platformDisplay() const Q_DECL_OVERRIDE;
    bool usesDefaultScreen() Q_DECL_OVERRIDE;
    void screenInit() Q_DECL_OVERRIDE;
    QSurfaceFormat surfaceFormatFor(const QSurfaceFormat &inputFormat) const Q_DECL_OVERRIDE;
    EGLNativeWindowType createNativeWindow(QPlatformWindow *platformWindow,
                                           const QSize &size,
                                           const QSurfaceFormat &format) Q_DECL_OVERRIDE;
    EGLNativeWindowType createNativeOffscreenWindow(const QSurfaceFormat &format) Q_DECL_OVERRIDE;
    void destroyNativeWindow(EGLNativeWindowType window) Q_DECL_OVERRIDE;
    bool hasCapability(QPlatformIntegration::Capability cap) const Q_DECL_OVERRIDE;
    QPlatformCursor *createCursor(QPlatformScreen *screen) const Q_DECL_OVERRIDE;
    void waitForVSync(QPlatformSurface *surface) const Q_DECL_OVERRIDE;
    void presentBuffer(QPlatformSurface *surface) Q_DECL_OVERRIDE;
    bool supportsPBuffers() const Q_DECL_OVERRIDE;

    bool hwCursor() const;
    bool separateScreens() const;
    QMap<QString, QVariantMap> outputSettings() const;

private:
    void loadConfig();

    QEglFSKmsDevice *m_device;
    bool m_hwCursor;
    bool m_pbuffers;
    bool m_separateScreens;
    QString m_devicePath;
    QMap<QString, QVariantMap> m_outputSettings;
};

QT_END_NAMESPACE

#endif
