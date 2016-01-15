/****************************************************************************
**
** Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QEGLFSKMSDEVICE_H
#define QEGLFSKMSDEVICE_H

#include "qeglfskmscursor.h"
#include "qeglfskmsintegration.h"

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

QT_BEGIN_NAMESPACE

class QEglFSKmsScreen;

class QEglFSKmsDevice
{
public:
    QEglFSKmsDevice(QEglFSKmsIntegration *integration, const QString &path);

    bool open();
    void close();

    void createScreens();

    gbm_device *device() const;
    int fd() const;

    QPlatformCursor *globalCursor() const;

    void handleDrmEvent();

private:
    Q_DISABLE_COPY(QEglFSKmsDevice)

    QEglFSKmsIntegration *m_integration;
    QString m_path;
    int m_dri_fd;
    gbm_device *m_gbm_device;

    quint32 m_crtc_allocator;
    quint32 m_connector_allocator;

    QEglFSKmsCursor *m_globalCursor;

    int crtcForConnector(drmModeResPtr resources, drmModeConnectorPtr connector);
    QEglFSKmsScreen *screenForConnector(drmModeResPtr resources, drmModeConnectorPtr connector, QPoint pos);
    drmModePropertyPtr connectorProperty(drmModeConnectorPtr connector, const QByteArray &name);

    static void pageFlipHandler(int fd,
                                unsigned int sequence,
                                unsigned int tv_sec,
                                unsigned int tv_usec,
                                void *user_data);
};

QT_END_NAMESPACE

#endif
