/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
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

#include "qeglfskmsvsp2screen.h"
#include "qeglfskmsvsp2device.h"
#include <qeglfskmshelpers.h>

#include <QtCore/QLoggingCategory>
#include <QtGui/private/qguiapplication_p.h>

#include <drm_fourcc.h>
#include <xf86drm.h>
#include <fcntl.h>

//TODO move to qlinuxmediadevice?
#include <cstdlib> //this needs to go before mediactl/mediactl.h because it uses size_t without including it
extern "C" {
#include <mediactl/mediactl.h>
#include <mediactl/v4l2subdev.h> //needed in header for default arguments
}

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEglfsKmsDebug)

static inline uint32_t drmFormatToGbmFormat(uint32_t drmFormat)
{
    Q_ASSERT(DRM_FORMAT_XRGB8888 == GBM_FORMAT_XRGB8888);
    return drmFormat;
}

static inline uint32_t gbmFormatToDrmFormat(uint32_t gbmFormat) //TODO: is this needed?
{
    Q_ASSERT(DRM_FORMAT_XRGB8888 == GBM_FORMAT_XRGB8888);
    return gbmFormat;
}

void QEglFSKmsVsp2Screen::dmaBufferDestroyedHandler(gbm_bo *gbmBo, void *data)
{
    Q_UNUSED(gbmBo); //TODO: do we need to do something with it after all?
    auto dmabuf = static_cast<DmaBuffer *>(data);
    //TODO: need some extra cleanup here?
    delete dmabuf;
}

QEglFSKmsVsp2Screen::DmaBuffer *QEglFSKmsVsp2Screen::dmaBufferForGbmBuffer(gbm_bo *gbmBo)
{
    auto existingBuffer = static_cast<DmaBuffer *>(gbm_bo_get_user_data(gbmBo));
    if (existingBuffer)
        return existingBuffer;

    uint32_t handle = gbm_bo_get_handle(gbmBo).u32;
    QScopedPointer<DmaBuffer> fb(new DmaBuffer);
    int ret = drmPrimeHandleToFD(device()->fd(), handle, DRM_CLOEXEC, &fb->dmabufFd);
    if (ret) {
        qWarning("Failed to create dmabuf file descriptor for buffer with drmPrimeHandleToFd");
        return nullptr;
    }

    gbm_bo_set_user_data(gbmBo, fb.data(), dmaBufferDestroyedHandler);
    return fb.take();
}

QEglFSKmsVsp2Screen::QEglFSKmsVsp2Screen(QEglFSKmsDevice *device, const QKmsOutput &output)
    : QEglFSKmsScreen(device, output)
    , m_blender(new Blender(this))
{
}

gbm_surface *QEglFSKmsVsp2Screen::createSurface()
{
    if (!m_gbmSurface) {
        uint32_t gbmFormat = drmFormatToGbmFormat(m_output.drm_format);
        qCDebug(qLcEglfsKmsDebug, "Creating gbm_surface for screen %s with format 0x%x", qPrintable(name()), gbmFormat);
        Q_ASSERT(rawGeometry().isValid());
        m_gbmSurface = gbm_surface_create(static_cast<QEglFSKmsVsp2Device *>(device())->gbmDevice(),
                                           uint(rawGeometry().width()),
                                           uint(rawGeometry().height()),
                                           gbmFormat,
                                           GBM_BO_USE_RENDERING);
    }

    if (!m_blendDevice)
        initVsp2();

    if (m_frameBuffers[m_backFb].dmabufFd == -1)
        initDumbFrameBuffers();

    return m_gbmSurface; // not owned, gets destroyed in QEglFSKmsGbmIntegration::destroyNativeWindow()
}

void QEglFSKmsVsp2Screen::resetSurface()
{
    m_gbmSurface = nullptr;
}

void QEglFSKmsVsp2Screen::initDumbFrameBuffers()
{
    for (auto &fb : m_frameBuffers)
        initDumbFrameBuffer(fb);
}

void QEglFSKmsVsp2Screen::initVsp2()
{
    qCDebug(qLcEglfsKmsDebug, "Initializing Vsp2 hardware");
    m_blendDevice.reset(new QVsp2BlendingDevice(rawGeometry().size()));

    // Enable input for main buffer drawn by the compositor (always on)
    initQtLayer();
}

void QEglFSKmsVsp2Screen::initQtLayer()
{
    const QSize screenSize = rawGeometry().size();
    const uint bytesPerLine = uint(screenSize.width()) * 4; //TODO: is this ok?
    bool formatSet = m_blendDevice->enableInput(m_qtLayer, QRect(QPoint(), screenSize), m_output.drm_format, bytesPerLine);
    if (!formatSet) {
        const uint32_t fallbackFormat = DRM_FORMAT_ARGB8888;
        qWarning() << "Failed to set format" << q_fourccToString(m_output.drm_format)
                   << "falling back to" << q_fourccToString(fallbackFormat);
        formatSet = m_blendDevice->enableInput(m_qtLayer, QRect(QPoint(), screenSize), fallbackFormat, bytesPerLine);
        if (!formatSet)
            qFatal("Failed to set vsp2 blending format");
    }
}

int QEglFSKmsVsp2Screen::addLayer(int dmabufFd, const QSize &size, const QPoint &position, uint drmPixelFormat, uint bytesPerLine)
{
    int index = m_blendDevice->enableInput(QRect(position, size), drmPixelFormat, bytesPerLine);
    if (index != -1) {
        m_blendDevice->setInputBuffer(index, dmabufFd);
        int id = index; //TODO: maybe make id something independent of layer index?
        qCDebug(qLcEglfsKmsDebug) << "Enabled extra layer for vsp input" << index;
        return id;
    }
    qWarning() << "Failed to add layer";
    return -1;
}

void QEglFSKmsVsp2Screen::setLayerBuffer(int id, int dmabufFd)
{
    int layerIndex = id;
    m_blendDevice->setInputBuffer(layerIndex, dmabufFd);
    if (!m_blendScheduled) {
        m_blendScheduled = true;
        QCoreApplication::postEvent(m_blender.data(), new QEvent(QEvent::User));
    }
}

void QEglFSKmsVsp2Screen::setLayerPosition(int id, const QPoint &position)
{
    int layerIndex = id;
    m_blendDevice->setInputPosition(layerIndex, position);
}

void QEglFSKmsVsp2Screen::setLayerAlpha(int id, qreal alpha)
{
    int layerIndex = id;
    m_blendDevice->setInputAlpha(layerIndex, alpha);
}

bool QEglFSKmsVsp2Screen::removeLayer(int id)
{
    int layerIndex = id;
    m_blendDevice->disableInput(layerIndex);
    return true;
}

void QEglFSKmsVsp2Screen::addBlendListener(void (*callback)())
{
    m_blendFinishedCallbacks.append(callback);
}

void QEglFSKmsVsp2Screen::flip()
{
    if (!m_gbmSurface) {
        qWarning("Cannot sync before platform init!");
        return;
    }

    if (!m_blendScheduled && !m_nextGbmBo) {
        m_nextGbmBo = gbm_surface_lock_front_buffer(m_gbmSurface);

        if (!m_nextGbmBo) {
            qWarning("Could not lock GBM surface front buffer!");
            return;
        }

        m_blendScheduled = true;
        QCoreApplication::postEvent(m_blender.data(), new QEvent(QEvent::User));
    }
}

void QEglFSKmsVsp2Screen::ensureModeSet()
{
    const int driFd = device()->fd();
    QKmsOutput &op(output());
    if (!op.mode_set) {
        int ret = drmModeSetCrtc(driFd,
                                 op.crtc_id,
                                 m_frameBuffers[m_backFb].drmBufferId,
                                 0, 0,
                                 &op.connector_id, 1,
                                 &op.modes[op.mode]);

        if (ret == -1) {
            qErrnoWarning(errno, "Could not set DRM mode!");
        } else {
            op.mode_set = true;
            setPowerState(PowerStateOn);
        }
    }
}

void QEglFSKmsVsp2Screen::doDrmFlip()
{
    QKmsOutput &op(output());
    const int driFd = device()->fd();

    int ret = drmModePageFlip(driFd,
                              op.crtc_id,
                              m_frameBuffers[m_backFb].drmBufferId,
                              0,
                              this);

    if (ret)
        qErrnoWarning("Could not queue DRM page flip on screen %s", qPrintable(name()));

    m_backFb = (m_backFb + 1) % 2;
}

void QEglFSKmsVsp2Screen::blendAndFlipDrm()
{
    m_blendScheduled = false;
    if (!m_nextGbmBo && !m_blendDevice->isDirty())
        return;

    FrameBuffer &backBuffer = m_frameBuffers[m_backFb];
    if (backBuffer.dmabufFd == -1)
        initDumbFrameBuffers();

    if (m_nextGbmBo) {
        Q_ASSERT(m_nextGbmBo != m_currentGbmBo);
        int compositorBackBufferDmaFd = dmaBufferForGbmBuffer(m_nextGbmBo)->dmabufFd;
        m_blendDevice->setInputBuffer(m_qtLayer, compositorBackBufferDmaFd);

        if (m_currentGbmBo)
            gbm_surface_release_buffer(m_gbmSurface, m_currentGbmBo);
        m_currentGbmBo = m_nextGbmBo;
        m_nextGbmBo = nullptr;
    }

    ensureModeSet();

    if (!m_blendDevice)
        initVsp2();

    if (!m_blendDevice->isDirty())
        return;

    const int driFd = device()->fd();
    drmVBlank vBlank;
    vBlank.request.type = static_cast<drmVBlankSeqType>(DRM_VBLANK_RELATIVE | DRM_VBLANK_SECONDARY); //TODO: make secondary configurable (or automatic)
    vBlank.request.sequence = 1;
    vBlank.request.signal = 0;
    drmWaitVBlank(driFd, &vBlank);

    if (!m_blendDevice->blend(backBuffer.dmabufFd)) {
        qWarning() << "Vsp2: Blending failed";

        // For some reason, a failed blend may often mess up the qt layer, so reinitialize it here
        m_blendDevice->disableInput(m_qtLayer);
        initQtLayer();
    }

    for (auto cb : m_blendFinishedCallbacks)
        cb();

    doDrmFlip();
}

void QEglFSKmsVsp2Screen::initDumbFrameBuffer(FrameBuffer &fb)
{
    QKmsOutput &op(output());
    const uint32_t width = op.modes[op.mode].hdisplay;
    const uint32_t height = op.modes[op.mode].vdisplay;

    Q_ASSERT(fb.dmabufFd == -1);
    const uint32_t dumbBufferFlags = 0; //TODO: do we want some flags? What's possible?
    const uint32_t bpp = 32;

    drm_mode_create_dumb creq = {
        height,
        width,
        bpp,
        dumbBufferFlags,
        0, 0, 0 //return values
    };

    const int driFd = device()->fd();
    if (drmIoctl(driFd, DRM_IOCTL_MODE_CREATE_DUMB, &creq) == -1)
        qFatal("Failed to create dumb buffer: %s", strerror(errno));

//    uint32_t handles[4] = { gbm_bo_get_handle(bo).u32 };
//    uint32_t strides[4] = { gbm_bo_get_stride(bo) };
//    uint32_t offsets[4] = { 0 };
//    uint32_t pixelFormat = gbmFormatToDrmFormat(gbm_bo_get_format(bo));

    //TODO: support additional planes
    uint32_t gbmBoHandles[4] = { creq.handle, 0, 0, 0 };
    uint32_t strides[4] = { creq.pitch, 0, 0, 0 };
    uint32_t offsets[4] = { 0 };
    uint32_t pixelFormat = DRM_FORMAT_ARGB8888; //TODO: support other formats?
    uint32_t drmFlags = 0;

    qCDebug(qLcEglfsKmsDebug) << "Adding FB" << QSize(width, height)
                              << ", DRM format" << q_fourccToString(pixelFormat);
    int ret = drmModeAddFB2(driFd, width, height, pixelFormat,
                            gbmBoHandles, strides, offsets, &fb.drmBufferId, drmFlags);
    if (ret)
        qFatal("drmModeAddFB2 failed: %s", strerror(errno));

    drmPrimeHandleToFD(driFd, gbmBoHandles[0], DRM_CLOEXEC, &fb.dmabufFd);
}

bool QEglFSKmsVsp2Screen::Blender::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::User:
        m_screen->blendAndFlipDrm();
        return true;
    default:
        return QObject::event(event);
    }
}

QT_END_NAMESPACE
