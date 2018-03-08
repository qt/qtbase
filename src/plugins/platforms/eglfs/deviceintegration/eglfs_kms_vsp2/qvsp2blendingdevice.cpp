/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qvsp2blendingdevice.h"
#include <qeglfskmshelpers.h>

#include <QDebug>
#include <QtCore/QLoggingCategory>

#include <drm_fourcc.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEglfsKmsDebug)

//TODO: is this the right place for these conversion functions?
static inline uint drmFormatToV4l2PixelFormat(uint drmFormat) {
    //ARGB8888 == ABGR32 because linux media list stuff in the opposite order, but the fourcc is the same
    Q_ASSERT(DRM_FORMAT_ARGB8888 == V4L2_PIX_FMT_ABGR32);
    return drmFormat;
}

static uint drmFormatToMediaBusFormat(uint drmFormat)
{
    switch (drmFormat) {
    case DRM_FORMAT_RGB888:
        return MEDIA_BUS_FMT_RGB888_1X24;
    case DRM_FORMAT_BGR888:
        return MEDIA_BUS_FMT_BGR888_1X24;
    case DRM_FORMAT_XRGB8888:
    case DRM_FORMAT_XBGR8888:
//        return MEDIA_BUS_FMT_RGB888_1X32_PADHI; // doesn't work on renesas m3, just use fallthrough to argb for now
    case DRM_FORMAT_RGBX8888:
    case DRM_FORMAT_BGRX8888:
    case DRM_FORMAT_ARGB8888:
    case DRM_FORMAT_ABGR8888:
    case DRM_FORMAT_RGBA8888:
    case DRM_FORMAT_BGRA8888:
        return MEDIA_BUS_FMT_ARGB8888_1X32;
    default:
        qWarning() << "Unknown drm format" << q_fourccToString(drmFormat) << "defaulting to argb8888";
        return MEDIA_BUS_FMT_ARGB8888_1X32;
    }
}

QVsp2BlendingDevice::QVsp2BlendingDevice(const QSize &screenSize)
    : m_mediaDevice("/dev/media0")
    , m_screenSize(screenSize)
{
    QLinuxMediaDevice &md = m_mediaDevice;
    QString deviceName = md.deviceName();

    if (md.model() != QString("VSP2"))
        qWarning() << "Unsupported media device model:" << md.model();

    if (deviceName != "fe960000.vsp")
        qWarning() << "Unknown media device name:" << deviceName;

    const int numInputs = 5;

    for (int i = 0; i < numInputs; ++i) {
        Input input;
        input.linkToBru = md.parseLink(QString("'%1 rpf.%2':1 -> '%1 bru':%2").arg(deviceName).arg(i));
        input.inputFormatPad = md.parsePad(QString("'%1 rpf.%2':0").arg(deviceName).arg(i));
        input.outputFormatPad = md.parsePad(QString("'%1 rpf.%2':1").arg(deviceName).arg(i));
        input.outputFormatFd = QLinuxMediaDevice::openVideoDevice(input.outputFormatPad);
        input.bruInputFormatPad = md.parsePad(QString("'%1 bru':%2").arg(deviceName).arg(i));
        input.rpfInput = new QLinuxMediaDevice::OutputSubDevice(&md, QString("%1 rpf.%2 input").arg(deviceName).arg(i));
        m_inputs.append(input);
    }

    m_wpfOutput = new QLinuxMediaDevice::CaptureSubDevice(&md, QString("%1 wpf.0 output").arg(deviceName));

    // Setup links for output
    md.enableLink(md.parseLink(QString("'%1 bru':5 -> '%1 wpf.0':0").arg(deviceName)));
    md.enableLink(md.parseLink(QString("'%1 wpf.0':1 -> '%1 wpf.0 output':0").arg(deviceName)));

    // Output pads
    auto bruOutputFormatPad = md.parsePad(QString("'%1 bru':5").arg(deviceName));
    auto wpfInputFormatPad = md.parsePad(QString("'%1 wpf.0':0").arg(deviceName));
    auto wpfOutputFormatPad = md.parsePad(QString("'%1 wpf.0':1").arg(deviceName));

    m_wpfOutput->setFormat(screenSize);
    QLinuxMediaDevice::setSubdevFormat(bruOutputFormatPad, screenSize);
    QLinuxMediaDevice::setSubdevFormat(wpfInputFormatPad, screenSize);
    QLinuxMediaDevice::setSubdevFormat(wpfOutputFormatPad, screenSize);

    m_wpfOutput->requestBuffer();
}

bool QVsp2BlendingDevice::enableInput(int i, const QRect &bufferGeometry, uint drmFormat, uint bytesPerLine)
{
    qCDebug(qLcEglfsKmsDebug) << "Blend unit: Enabling input" << i;
    if (m_inputs[i].enabled) {
        qWarning("Vsp2: Input %d already enabled", i);
        return false;
    }

    if (!bufferGeometry.isValid()) { //TODO: bounds checking as well?
        qWarning() << "Vsp2: Invalid buffer geometry";
        return false;
    }

    Input &input = m_inputs[i];
    if (!m_mediaDevice.enableLink(input.linkToBru))
        return false;

    uint pixelFormat = drmFormatToV4l2PixelFormat(drmFormat);
    if (!setInputFormat(i, bufferGeometry, pixelFormat, bytesPerLine)) {
        disableInput(i);
        return false;
    }

    input.rpfInput->requestBuffer();
    return true;
}

int QVsp2BlendingDevice::enableInput(const QRect &bufferGeometry, uint drmFormat, uint bytesPerLine)
{
    for (int i = 0; i < m_inputs.size(); ++i) {
        if (!m_inputs[i].enabled)
            return enableInput(i, bufferGeometry, drmFormat, bytesPerLine) ? i : -1;
    }
    qWarning() << "Vsp2: No more inputs available in blend unit";
    return -1;
}

bool QVsp2BlendingDevice::disableInput(int i)
{
    qCDebug(qLcEglfsKmsDebug) << "Vsp2: disabling input" << i;
    if (!m_inputs[i].enabled) {
        qWarning("Vsp2: Input %d already disabled", i);
        return false;
    }
    m_mediaDevice.disableLink(m_inputs[i].linkToBru);
    m_inputs[i].rpfInput->clearBuffers();
    m_inputs[i].enabled = false;
    return true;
}

bool QVsp2BlendingDevice::setInputBuffer(int index, int dmabufFd)
{
    Input &input = m_inputs[index];

    if (!input.enabled) {
        qWarning() << "Vsp2: Can't queue on disabled input" << index;
        return false;
    }

    // Don't queue the buffer yet, store it here and wait until blending
    if (input.dmabuf.fd != dmabufFd) {
        m_dirty = true;
        input.dmabuf.fd = dmabufFd;
    }
    return true;
}

bool QVsp2BlendingDevice::setInputPosition(int index, const QPoint &position)
{
    Input &input = m_inputs[index];

    if (input.geometry.topLeft() == position)
        return true;

    m_dirty = true;
    input.geometry.moveTopLeft(position);
    return QLinuxMediaDevice::setSubdevCompose(input.bruInputFormatPad, input.geometry);
}

bool QVsp2BlendingDevice::setInputAlpha(int index, qreal alpha)
{
    Input &input = m_inputs[index];
    if (input.alpha == alpha)
        return true;

    m_dirty = true;
    input.alpha = alpha;
    return QLinuxMediaDevice::setSubdevAlpha(input.outputFormatFd, alpha);
}

bool QVsp2BlendingDevice::blend(int outputDmabufFd)
{
    if (!m_dirty)
        qWarning("Blending without being dirty, should not be necessary");

    if (!hasContinuousLayers()) {
        qWarning("Vsp2: Can't blend when layers are not enabled in order from 0 without gaps.");
        return false;
    }

    bool queueingFailed = false;
    // Queue dma input buffers
    for (int i=0; i < m_inputs.size(); ++i) {
        auto &input = m_inputs[i];
        if (input.enabled) {
            if (!input.rpfInput->queueBuffer(input.dmabuf.fd, input.dmabuf.bytesUsed, input.dmabuf.length)) {
                qWarning() << "Vsp2: Failed to queue buffer for input" << i
                           << "with dmabuf" << input.dmabuf.fd
                           << "and size" << input.geometry.size();

                queueingFailed = true;
            }
        }
    }

    if (queueingFailed) {
        qWarning() << "Vsp2: Trying to clean up queued buffers";
        for (auto &input : qAsConst(m_inputs)) {
            if (input.enabled) {
                if (!input.rpfInput->clearBuffers())
                    qWarning() << "Vsp2: Failed to remove buffers after an aborted blend";
            }
        }
        return false;
    }

    if (!m_wpfOutput->queueBuffer(outputDmabufFd, m_screenSize)) {
        qWarning() << "Vsp2: Failed to queue blending output buffer" << outputDmabufFd << m_screenSize;
        return false;
    }

    if (!streamOn()) {
        qWarning() << "Vsp2: Failed to start streaming";
        return false;
    }

    if (!m_wpfOutput->dequeueBuffer()) {
        qWarning() << "Vsp2: Failed to dequeue blending output buffer";
        if (!streamOff())
            qWarning() << "Vsp2: Failed to stop streaming when recovering after a broken blend.";
        return false;
    }

    if (!streamOff()) {
        qWarning() << "Vsp2: Failed to stop streaming";
        return false;
    }

    m_dirty = false;
    return true;
}

int QVsp2BlendingDevice::numInputs() const
{
    return m_inputs.size();
}

bool QVsp2BlendingDevice::hasContinuousLayers() const
{
    bool seenDisabled = false;
    for (auto &input : qAsConst(m_inputs)) {
        if (seenDisabled && input.enabled)
            return false;
        seenDisabled |= !input.enabled;
    }
    return m_inputs[0].enabled;
}

bool QVsp2BlendingDevice::streamOn()
{
    for (auto &input : m_inputs) {
        if (input.enabled) {
            if (!input.rpfInput->streamOn()) {
                //TODO: perhaps it's better to try to continue with the other inputs?
                return false;
            }
        }
    }

    return m_wpfOutput->streamOn();
}

bool QVsp2BlendingDevice::streamOff()
{
    bool succeeded = m_wpfOutput->streamOff();
    for (auto &input : m_inputs) {
        if (input.enabled)
            succeeded &= input.rpfInput->streamOff();
    }
    return succeeded;
}

bool QVsp2BlendingDevice::setInputFormat(int i, const QRect &bufferGeometry, uint pixelFormat, uint bytesPerLine)
{
    Input &input = m_inputs[i];

    Q_ASSERT(bufferGeometry.isValid());

    const uint bpp = 4; //TODO: don't hardcode bpp, get it from pixelFormat?
    input.enabled = true;
    input.geometry = bufferGeometry;
    input.dmabuf.bytesUsed = bpp * static_cast<uint>(bufferGeometry.width()) * static_cast<uint>(bufferGeometry.height());
    input.dmabuf.length = static_cast<uint>(bufferGeometry.height()) * bytesPerLine;

    const QSize size = bufferGeometry.size();

    if (!input.rpfInput->setFormat(size, pixelFormat, bytesPerLine)) // rpf.x input
        return false;

    const uint mediaBusFormat = drmFormatToMediaBusFormat(pixelFormat);
    if (!QLinuxMediaDevice::setSubdevFormat(input.inputFormatPad, size, mediaBusFormat)) // rpf.x:0
        return false;

    if (!QLinuxMediaDevice::setSubdevFormat(input.outputFormatPad, size, mediaBusFormat)) // rpf.x:1
        return false;

    if (!QLinuxMediaDevice::setSubdevFormat(input.bruInputFormatPad, size, mediaBusFormat)) // bru:x
        return false;

    if (!QLinuxMediaDevice::setSubdevCrop(input.inputFormatPad, QRect(QPoint(0, 0), size)))
        return false;

    if (!QLinuxMediaDevice::setSubdevCompose(input.bruInputFormatPad, bufferGeometry))
        return false;

    return true;
}

QT_END_NAMESPACE
