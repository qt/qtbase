/****************************************************************************
**
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

#ifndef QEGLFSKMSEGLDEVICEINTEGRATION_H
#define QEGLFSKMSEGLDEVICEINTEGRATION_H

#include "qeglfsdeviceintegration.h"
#include "qeglfswindow.h"
#include "qeglfsintegration.h"

#include <QtPlatformSupport/private/qdevicediscovery_p.h>
#include <QtPlatformSupport/private/qeglconvenience_p.h>
#include <QtCore/private/qcore_unix_p.h>
#include <QtCore/QScopedPointer>
#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/qguiapplication.h>
#include <QDebug>

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <QtPlatformSupport/private/qeglstreamconvenience_p.h>

QT_BEGIN_NAMESPACE

class QEglFSKmsEglDeviceIntegration : public QEGLDeviceIntegration
{
public:
    QEglFSKmsEglDeviceIntegration();

    void platformInit() Q_DECL_OVERRIDE;
    void platformDestroy() Q_DECL_OVERRIDE;
    EGLNativeDisplayType platformDisplay() const Q_DECL_OVERRIDE;
    EGLDisplay createDisplay(EGLNativeDisplayType nativeDisplay) Q_DECL_OVERRIDE;
    QSizeF physicalScreenSize() const Q_DECL_OVERRIDE;
    QSize screenSize() const Q_DECL_OVERRIDE;
    int screenDepth() const Q_DECL_OVERRIDE;
    qreal refreshRate() const Q_DECL_OVERRIDE;
    QSurfaceFormat surfaceFormatFor(const QSurfaceFormat &inputFormat) const Q_DECL_OVERRIDE;
    EGLint surfaceType() const Q_DECL_OVERRIDE;
    QEglFSWindow *createWindow(QWindow *window) const Q_DECL_OVERRIDE;
    bool hasCapability(QPlatformIntegration::Capability cap) const Q_DECL_OVERRIDE;
    void waitForVSync(QPlatformSurface *surface) const Q_DECL_OVERRIDE;
    bool supportsSurfacelessContexts() const Q_DECL_OVERRIDE;

    bool setup_kms();
    bool query_egl_device();

    // device bits
    QByteArray m_device;
    int m_dri_fd;
    EGLDeviceEXT m_egl_device;
    EGLDisplay m_egl_display;

    // KMS bits
    drmModeConnector *m_drm_connector;
    drmModeEncoder *m_drm_encoder;
    drmModeModeInfo m_drm_mode;
    quint32 m_drm_crtc;

    // EGLStream infrastructure
    QEGLStreamConvenience *m_funcs;
};

QT_END_NAMESPACE

#endif
