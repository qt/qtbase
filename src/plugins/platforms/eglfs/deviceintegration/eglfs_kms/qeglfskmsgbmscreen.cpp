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

#include "qeglfskmsgbmscreen.h"
#include "qeglfskmsgbmdevice.h"
#include "qeglfskmsgbmcursor.h"
#include "qeglfsintegration_p.h"

#include <QtCore/QLoggingCategory>

#include <QtGui/private/qguiapplication_p.h>
#include <QtFbSupport/private/qfbvthandler_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEglfsKmsDebug)

void QEglFSKmsGbmScreen::bufferDestroyedHandler(gbm_bo *bo, void *data)
{
    FrameBuffer *fb = static_cast<FrameBuffer *>(data);

    if (fb->fb) {
        gbm_device *device = gbm_bo_get_device(bo);
        drmModeRmFB(gbm_device_get_fd(device), fb->fb);
    }

    delete fb;
}

QEglFSKmsGbmScreen::FrameBuffer *QEglFSKmsGbmScreen::framebufferForBufferObject(gbm_bo *bo)
{
    {
        FrameBuffer *fb = static_cast<FrameBuffer *>(gbm_bo_get_user_data(bo));
        if (fb)
            return fb;
    }

    uint32_t width = gbm_bo_get_width(bo);
    uint32_t height = gbm_bo_get_height(bo);
    uint32_t stride = gbm_bo_get_stride(bo);
    uint32_t handle = gbm_bo_get_handle(bo).u32;

    QScopedPointer<FrameBuffer> fb(new FrameBuffer);

    int ret = drmModeAddFB(device()->fd(), width, height, 24, 32,
                           stride, handle, &fb->fb);

    if (ret) {
        qWarning("Failed to create KMS FB!");
        return Q_NULLPTR;
    }

    gbm_bo_set_user_data(bo, fb.data(), bufferDestroyedHandler);
    return fb.take();
}

QEglFSKmsGbmScreen::QEglFSKmsGbmScreen(QEglFSKmsIntegration *integration,
                                       QEglFSKmsDevice *device,
                                       QEglFSKmsOutput output)
    : QEglFSKmsScreen(integration, device, output)
    , m_gbm_surface(Q_NULLPTR)
    , m_gbm_bo_current(Q_NULLPTR)
    , m_gbm_bo_next(Q_NULLPTR)
    , m_cursor(Q_NULLPTR)
{
}

QEglFSKmsGbmScreen::~QEglFSKmsGbmScreen()
{
}

QPlatformCursor *QEglFSKmsGbmScreen::cursor() const
{
    if (integration()->hwCursor()) {
        if (!integration()->separateScreens())
            return static_cast<QEglFSKmsGbmDevice *>(device())->globalCursor();

        if (m_cursor.isNull()) {
            QEglFSKmsGbmScreen *that = const_cast<QEglFSKmsGbmScreen *>(this);
            that->m_cursor.reset(new QEglFSKmsGbmCursor(that));
        }

        return m_cursor.data();
    } else {
        return QEglFSScreen::cursor();
    }
}

gbm_surface *QEglFSKmsGbmScreen::createSurface()
{
    if (!m_gbm_surface) {
        qCDebug(qLcEglfsKmsDebug) << "Creating window for screen" << name();
        m_gbm_surface = gbm_surface_create(static_cast<QEglFSKmsGbmDevice *>(device())->gbmDevice(),
                                           rawGeometry().width(),
                                           rawGeometry().height(),
                                           GBM_FORMAT_XRGB8888,
                                           GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    }
    return m_gbm_surface;
}

void QEglFSKmsGbmScreen::destroySurface()
{
    if (m_gbm_bo_current) {
        gbm_bo_destroy(m_gbm_bo_current);
        m_gbm_bo_current = Q_NULLPTR;
    }

    if (m_gbm_bo_next) {
        gbm_bo_destroy(m_gbm_bo_next);
        m_gbm_bo_next = Q_NULLPTR;
    }

    if (m_gbm_surface) {
        gbm_surface_destroy(m_gbm_surface);
        m_gbm_surface = Q_NULLPTR;
    }
}

void QEglFSKmsGbmScreen::waitForFlip()
{
    // Don't lock the mutex unless we actually need to
    if (!m_gbm_bo_next)
        return;

    QMutexLocker lock(&m_waitForFlipMutex);
    while (m_gbm_bo_next)
        static_cast<QEglFSKmsGbmDevice *>(device())->handleDrmEvent();
}

void QEglFSKmsGbmScreen::flip()
{
    if (!m_gbm_surface) {
        qWarning("Cannot sync before platform init!");
        return;
    }

    m_gbm_bo_next = gbm_surface_lock_front_buffer(m_gbm_surface);
    if (!m_gbm_bo_next) {
        qWarning("Could not lock GBM surface front buffer!");
        return;
    }

    FrameBuffer *fb = framebufferForBufferObject(m_gbm_bo_next);

    if (!output().mode_set) {
        int ret = drmModeSetCrtc(device()->fd(),
                                 output().crtc_id,
                                 fb->fb,
                                 0, 0,
                                 &output().connector_id, 1,
                                 &output().modes[output().mode]);

        if (ret) {
            qErrnoWarning("Could not set DRM mode!");
        } else {
            output().mode_set = true;
            setPowerState(PowerStateOn);
        }
    }

    int ret = drmModePageFlip(device()->fd(),
                              output().crtc_id,
                              fb->fb,
                              DRM_MODE_PAGE_FLIP_EVENT,
                              this);
    if (ret) {
        qErrnoWarning("Could not queue DRM page flip!");
        gbm_surface_release_buffer(m_gbm_surface, m_gbm_bo_next);
        m_gbm_bo_next = Q_NULLPTR;
    }
}

void QEglFSKmsGbmScreen::flipFinished()
{
    if (m_gbm_bo_current)
        gbm_surface_release_buffer(m_gbm_surface,
                                   m_gbm_bo_current);

    m_gbm_bo_current = m_gbm_bo_next;
    m_gbm_bo_next = Q_NULLPTR;
}

QT_END_NAMESPACE
