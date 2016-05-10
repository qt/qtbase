/****************************************************************************
**
** Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QEGLFSKMSINTEGRATION_H
#define QEGLFSKMSINTEGRATION_H

#include "qeglfsdeviceintegration.h"
#include <QtCore/QMap>
#include <QtCore/QVariant>
#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

class QEglFSKmsDevice;

Q_EGLFS_EXPORT Q_DECLARE_LOGGING_CATEGORY(qLcEglfsKmsDebug)

class Q_EGLFS_EXPORT QEglFSKmsIntegration : public QEGLDeviceIntegration
{
public:
    QEglFSKmsIntegration();

    void platformInit() Q_DECL_OVERRIDE;
    void platformDestroy() Q_DECL_OVERRIDE;
    EGLNativeDisplayType platformDisplay() const Q_DECL_OVERRIDE;
    bool usesDefaultScreen() Q_DECL_OVERRIDE;
    void screenInit() Q_DECL_OVERRIDE;
    QSurfaceFormat surfaceFormatFor(const QSurfaceFormat &inputFormat) const Q_DECL_OVERRIDE;
    bool hasCapability(QPlatformIntegration::Capability cap) const Q_DECL_OVERRIDE;
    void waitForVSync(QPlatformSurface *surface) const Q_DECL_OVERRIDE;
    bool supportsPBuffers() const Q_DECL_OVERRIDE;

    virtual bool hwCursor() const;
    virtual bool separateScreens() const;
    QMap<QString, QVariantMap> outputSettings() const;

    QEglFSKmsDevice *device() const;

protected:
    virtual QEglFSKmsDevice *createDevice(const QString &devicePath) = 0;

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
