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

#include "qeglfsintegration_p.h"
#include "qlinuxmediadevice.h"
#include <qeglfskmshelpers.h>

#include <QtCore/QLoggingCategory>
#include <QtCore/QSize>
#include <QtCore/QRect>

#include <sys/ioctl.h>
#include <fcntl.h>

#include <cstdlib> //this needs to go before mediactl/mediactl.h because it uses size_t without including it
extern "C" {
#include <mediactl/mediactl.h>
#include <mediactl/v4l2subdev.h>
}

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEglfsKmsDebug)

static QString mediaBusFmtToString(uint code)
{
    switch (code) {
    case MEDIA_BUS_FMT_FIXED: return "FIXED";
    case MEDIA_BUS_FMT_RGB444_1X12: return "RGB444_1X12";
//    case MEDIA_BUS_FMT_RGB444_2X8_PADHI_B: return "RGB444_2X8_PADHI_B";
//    case MEDIA_BUS_FMT_RGB444_2X8_PADHI_L: return "RGB444_2X8_PADHI_L";
//    case MEDIA_BUS_FMT_RGB555_2X8_PADHI_B: return "RGB555_2X8_PADHI_B";
//    case MEDIA_BUS_FMT_RGB555_2X8_PADHI_L: return "RGB555_2X8_PADHI_L";
    case MEDIA_BUS_FMT_RGB565_1X16: return "RGB565_1X16";
    case MEDIA_BUS_FMT_BGR565_2X8_BE: return "BGR565_2X8_BE";
    case MEDIA_BUS_FMT_BGR565_2X8_LE: return "BGR565_2X8_LE";
    case MEDIA_BUS_FMT_RGB565_2X8_BE: return "RGB565_2X8_BE";
    case MEDIA_BUS_FMT_RGB565_2X8_LE: return "RGB565_2X8_LE";
    case MEDIA_BUS_FMT_RGB666_1X18: return "RGB666_1X18";
    case MEDIA_BUS_FMT_RBG888_1X24: return "RBG888_1X24";
//    case MEDIA_BUS_FMT_RGB666_1X24_CPADH: return "RGB666_1X24_CPADH";
    case MEDIA_BUS_FMT_RGB666_1X7X3_SPWG: return "RGB666_1X7X3_SPWG";
    case MEDIA_BUS_FMT_BGR888_1X24: return "BGR888_1X24";
    case MEDIA_BUS_FMT_GBR888_1X24: return "GBR888_1X24";
    case MEDIA_BUS_FMT_RGB888_1X24: return "RGB888_1X24";
    case MEDIA_BUS_FMT_RGB888_2X12_BE: return "RGB888_2X12_BE";
    case MEDIA_BUS_FMT_RGB888_2X12_LE: return "RGB888_2X12_LE";
    case MEDIA_BUS_FMT_RGB888_1X7X4_SPWG: return "RGB888_1X7X4_SPWG";
//    case MEDIA_BUS_FMT_RGB888_1X7X4_JEID: return "RGB888_1X7X4_JEID";
    case MEDIA_BUS_FMT_ARGB8888_1X32: return "ARGB8888_1X32";
    case MEDIA_BUS_FMT_RGB888_1X32_PADHI: return "RGB888_1X32_PADHI";
//    case MEDIA_BUS_FMT_RGB101010_1X30: return "RGB101010_1X30";
//    case MEDIA_BUS_FMT_RGB121212_1X36: return "RGB121212_1X36";
//    case MEDIA_BUS_FMT_RGB161616_1X48: return "RGB161616_1X48";
    case MEDIA_BUS_FMT_Y8_1X8: return "Y8_1X8";
    case MEDIA_BUS_FMT_UV8_1X8: return "UV8_1X8";
    case MEDIA_BUS_FMT_UYVY8_1_5X8: return "UYVY8_1_5X8";
    case MEDIA_BUS_FMT_VYUY8_1_5X8: return "VYUY8_1_5X8";
    case MEDIA_BUS_FMT_YUYV8_1_5X8: return "YUYV8_1_5X8";
    case MEDIA_BUS_FMT_YVYU8_1_5X8: return "YVYU8_1_5X8";
    case MEDIA_BUS_FMT_UYVY8_2X8: return "UYVY8_2X8";
    case MEDIA_BUS_FMT_VYUY8_2X8: return "VYUY8_2X8";
    case MEDIA_BUS_FMT_YUYV8_2X8: return "YUYV8_2X8";
    case MEDIA_BUS_FMT_YVYU8_2X8: return "YVYU8_2X8";
    case MEDIA_BUS_FMT_Y10_1X10: return "Y10_1X10";
    case MEDIA_BUS_FMT_UYVY10_2X10: return "UYVY10_2X10";
    case MEDIA_BUS_FMT_VYUY10_2X10: return "VYUY10_2X10";
    case MEDIA_BUS_FMT_YUYV10_2X10: return "YUYV10_2X10";
    case MEDIA_BUS_FMT_YVYU10_2X10: return "YVYU10_2X10";
    case MEDIA_BUS_FMT_Y12_1X12: return "Y12_1X12";
    case MEDIA_BUS_FMT_UYVY12_2X12: return "UYVY12_2X12";
    case MEDIA_BUS_FMT_VYUY12_2X12: return "VYUY12_2X12";
    case MEDIA_BUS_FMT_YUYV12_2X12: return "YUYV12_2X12";
    case MEDIA_BUS_FMT_YVYU12_2X12: return "YVYU12_2X12";
    case MEDIA_BUS_FMT_UYVY8_1X16: return "UYVY8_1X16";
    case MEDIA_BUS_FMT_VYUY8_1X16: return "VYUY8_1X16";
    case MEDIA_BUS_FMT_YUYV8_1X16: return "YUYV8_1X16";
    case MEDIA_BUS_FMT_YVYU8_1X16: return "YVYU8_1X16";
    case MEDIA_BUS_FMT_YDYUYDYV8_1X16: return "YDYUYDYV8_1X16";
    case MEDIA_BUS_FMT_UYVY10_1X20: return "UYVY10_1X20";
    case MEDIA_BUS_FMT_VYUY10_1X20: return "VYUY10_1X20";
    case MEDIA_BUS_FMT_YUYV10_1X20: return "YUYV10_1X20";
    case MEDIA_BUS_FMT_YVYU10_1X20: return "YVYU10_1X20";
    case MEDIA_BUS_FMT_VUY8_1X24: return "VUY8_1X24";
    case MEDIA_BUS_FMT_YUV8_1X24: return "YUV8_1X24";
//    case MEDIA_BUS_FMT_UYYVYY8_0_5X24: return "UYYVYY8_0_5X24";
    case MEDIA_BUS_FMT_UYVY12_1X24: return "UYVY12_1X24";
    case MEDIA_BUS_FMT_VYUY12_1X24: return "VYUY12_1X24";
    case MEDIA_BUS_FMT_YUYV12_1X24: return "YUYV12_1X24";
    case MEDIA_BUS_FMT_YVYU12_1X24: return "YVYU12_1X24";
    case MEDIA_BUS_FMT_YUV10_1X30: return "YUV10_1X30";
//    case MEDIA_BUS_FMT_UYYVYY10_0_5X30: return "UYYVYY10_0_5X30";
    case MEDIA_BUS_FMT_AYUV8_1X32: return "AYUV8_1X32";
//    case MEDIA_BUS_FMT_UYYVYY12_0_5X36: return "UYYVYY12_0_5X36";
//    case MEDIA_BUS_FMT_YUV12_1X36: return "YUV12_1X36";
//    case MEDIA_BUS_FMT_YUV16_1X48: return "YUV16_1X48";
//    case MEDIA_BUS_FMT_UYYVYY16_0_5X48: return "UYYVYY16_0_5X48";
    case MEDIA_BUS_FMT_SBGGR8_1X8: return "SBGGR8_1X8";
    case MEDIA_BUS_FMT_SGBRG8_1X8: return "SGBRG8_1X8";
    case MEDIA_BUS_FMT_SGRBG8_1X8: return "SGRBG8_1X8";
    case MEDIA_BUS_FMT_SRGGB8_1X8: return "SRGGB8_1X8";
    case MEDIA_BUS_FMT_SBGGR10_ALAW8_1X8: return "SBGGR10_ALAW8_1X8";
    case MEDIA_BUS_FMT_SGBRG10_ALAW8_1X8: return "SGBRG10_ALAW8_1X8";
    case MEDIA_BUS_FMT_SGRBG10_ALAW8_1X8: return "SGRBG10_ALAW8_1X8";
    case MEDIA_BUS_FMT_SRGGB10_ALAW8_1X8: return "SRGGB10_ALAW8_1X8";
    case MEDIA_BUS_FMT_SBGGR10_DPCM8_1X8: return "SBGGR10_DPCM8_1X8";
    case MEDIA_BUS_FMT_SGBRG10_DPCM8_1X8: return "SGBRG10_DPCM8_1X8";
    case MEDIA_BUS_FMT_SGRBG10_DPCM8_1X8: return "SGRBG10_DPCM8_1X8";
    case MEDIA_BUS_FMT_SRGGB10_DPCM8_1X8: return "SRGGB10_DPCM8_1X8";
//    case MEDIA_BUS_FMT_SBGGR10_2X8_PADHI_B: return "SBGGR10_2X8_PADHI_B";
//    case MEDIA_BUS_FMT_SBGGR10_2X8_PADHI_L: return "SBGGR10_2X8_PADHI_L";
//    case MEDIA_BUS_FMT_SBGGR10_2X8_PADLO_B: return "SBGGR10_2X8_PADLO_B";
//    case MEDIA_BUS_FMT_SBGGR10_2X8_PADLO_L: return "SBGGR10_2X8_PADLO_L";
    case MEDIA_BUS_FMT_SBGGR10_1X10: return "SBGGR10_1X10";
    case MEDIA_BUS_FMT_SGBRG10_1X10: return "SGBRG10_1X10";
    case MEDIA_BUS_FMT_SGRBG10_1X10: return "SGRBG10_1X10";
    case MEDIA_BUS_FMT_SRGGB10_1X10: return "SRGGB10_1X10";
    case MEDIA_BUS_FMT_SBGGR12_1X12: return "SBGGR12_1X12";
    case MEDIA_BUS_FMT_SGBRG12_1X12: return "SGBRG12_1X12";
    case MEDIA_BUS_FMT_SGRBG12_1X12: return "SGRBG12_1X12";
    case MEDIA_BUS_FMT_SRGGB12_1X12: return "SRGGB12_1X12";
    case MEDIA_BUS_FMT_SBGGR14_1X14: return "SBGGR14_1X14";
    case MEDIA_BUS_FMT_SGBRG14_1X14: return "SGBRG14_1X14";
    case MEDIA_BUS_FMT_SGRBG14_1X14: return "SGRBG14_1X14";
    case MEDIA_BUS_FMT_SRGGB14_1X14: return "SRGGB14_1X14";
    case MEDIA_BUS_FMT_SBGGR16_1X16: return "SBGGR16_1X16";
    case MEDIA_BUS_FMT_SGBRG16_1X16: return "SGBRG16_1X16";
    case MEDIA_BUS_FMT_SGRBG16_1X16: return "SGRBG16_1X16";
    case MEDIA_BUS_FMT_SRGGB16_1X16: return "SRGGB16_1X16";
    case MEDIA_BUS_FMT_JPEG_1X8: return "JPEG_1X8";
    case MEDIA_BUS_FMT_S5C_UYVY_JPEG_1X8: return "S5C_UYVY_JPEG_1X8";
    case MEDIA_BUS_FMT_AHSV8888_1X32: return "AHSV8888_1X32";
    default: return QString(code);
    }
}

static QDebug operator<<(QDebug debug, const struct v4l2_mbus_framefmt &format)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "v4l2_mbus_framefmt("
                    << "code: " << mediaBusFmtToString(format.code) << ", "
                    << "size: " << format.width << "x" << format.height << ")";
    return debug;
}

static QDebug operator<<(QDebug debug, const struct v4l2_pix_format_mplane &format)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "v4l2_pix_format_mplane("
                    << "pixel format: " << q_fourccToString(format.pixelformat) << ", "
                    << "size: " << format.width << "x" << format.height << ", "
                    << "planes: " << format.num_planes << ")";
    return debug;
}

QLinuxMediaDevice::QLinuxMediaDevice(const QString &devicePath)
    : m_mediaDevice(media_device_new(devicePath.toStdString().c_str()))
{
    if (!m_mediaDevice)
        qFatal("Couldn't get media device");

    if (media_device_enumerate(m_mediaDevice))
        qFatal("Couldn't enumerate media device");

    m_info = media_get_info(m_mediaDevice);

    qCDebug(qLcEglfsKmsDebug) << "Opened linux media device:"
                              << "\n\t Path:" << devicePath
                              << "\n\t Model:" << model()
                              << "\n\t Device name:" << deviceName();

    resetLinks();
}

QLinuxMediaDevice::~QLinuxMediaDevice()
{
    if (m_mediaDevice)
        media_device_unref(m_mediaDevice);
}

QString QLinuxMediaDevice::model()
{
    return QString(m_info->model);
}

QString QLinuxMediaDevice::deviceName()
{
    return QString(m_info->bus_info).split(":").last();
}

bool QLinuxMediaDevice::resetLinks()
{
    if (media_reset_links(m_mediaDevice)) {
        qWarning() << "Could not reset media controller links.";
        return false;
    }
    qCDebug(qLcEglfsKmsDebug) << "Reset media links";
    return true;
}

struct media_link *QLinuxMediaDevice::parseLink(const QString &link)
{
    char *endp = nullptr;;
    struct media_link *mediaLink = media_parse_link(m_mediaDevice, link.toStdString().c_str(), &endp);

    if (!mediaLink)
        qWarning() << "Failed to parse media link:" << link;

    return mediaLink;
}

struct media_pad *QLinuxMediaDevice::parsePad(const QString &pad)
{
    struct media_pad *mediaPad = media_parse_pad(m_mediaDevice, pad.toStdString().c_str(), nullptr);

    if (!mediaPad)
        qWarning() << "Failed to parse media pad:" << pad;

    return mediaPad;
}

bool QLinuxMediaDevice::enableLink(struct media_link *link)
{
    if (media_setup_link(m_mediaDevice, link->source, link->sink, 1)) {
        qWarning() << "Failed to enable media link.";
        return false;
    }
    return true;
}

bool QLinuxMediaDevice::disableLink(struct media_link *link)
{
    if (media_setup_link(m_mediaDevice, link->source, link->sink, 0)) {
        qWarning() << "Failed to disable media link.";
        return false;
    }
    return true;
}

// Between the v4l-utils 1.10 and 1.12 releases, media_get_entity_by_name changed signature,
// i.e. breaking source compatibility. Unfortunately the v4l-utils version is not exposed
// through anything we can use through a regular ifdef so here we do a hack and create two
// overloads for a function based on the signature of the function pointer argument. This
// means that by calling safeGetEntity with media_get_entity_by_name as an argument, the
// compiler will pick the correct version.
//
// v4l-utils v1.12 and later
static struct media_entity *safeGetEntity(struct media_entity *(get_entity_by_name_fn)(struct media_device *, const char *),
                                          struct media_device *device, const QString &name)
{
    return get_entity_by_name_fn(device, name.toStdString().c_str());
}
// v4l-utils v1.10 and earlier
static struct media_entity *safeGetEntity(struct media_entity *(get_entity_by_name_fn)(struct media_device *, const char *, size_t),
                                          struct media_device *device,
                                          const QString &name)
{
    return get_entity_by_name_fn(device, name.toStdString().c_str(), name.length());
}

struct media_entity *QLinuxMediaDevice::getEntity(const QString &name)
{
    struct media_entity *entity = safeGetEntity(media_get_entity_by_name, m_mediaDevice, name);

    if (!entity)
        qWarning() << "Failed to get media entity:" << name;

    return entity;
}

int QLinuxMediaDevice::openVideoDevice(const QString &name)
{
    struct media_entity *entity = getEntity(name);
    const char *deviceName = media_entity_get_devname(entity);
    int fd = open(deviceName, O_RDWR);
    qCDebug(qLcEglfsKmsDebug) << "Opened video device:" << deviceName << "with fd" << fd;
    return fd;
}

QLinuxMediaDevice::CaptureSubDevice::CaptureSubDevice(QLinuxMediaDevice *mediaDevice, const QString &name)
    : m_subdevFd(mediaDevice->openVideoDevice(name))
{
}

bool QLinuxMediaDevice::CaptureSubDevice::setFormat(const QSize &size, uint32_t pixelFormat)
{
    Q_ASSERT(size.isValid());
    struct v4l2_format format;
    memset(&format, 0, sizeof(struct v4l2_format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(m_subdevFd, VIDIOC_G_FMT, &format) == -1) {
        qErrnoWarning("VIDIOC_G_FMT for capture device failed");
        return false;
    }

    format.fmt.pix_mp.width = static_cast<uint>(size.width());
    format.fmt.pix_mp.height = static_cast<uint>(size.height());
    format.fmt.pix_mp.field = V4L2_FIELD_NONE;
    format.fmt.pix_mp.pixelformat = pixelFormat;
    format.fmt.pix_mp.num_planes = 1;
    format.fmt.pix_mp.flags = 0;
    format.fmt.pix_mp.plane_fmt[0].bytesperline = 0;
    format.fmt.pix_mp.plane_fmt[0].sizeimage = 0;

    if (ioctl(m_subdevFd, VIDIOC_S_FMT, &format) == -1) {
        qWarning() << "Capture device" << m_subdevFd << "VIDIOC_S_FMT with format" << format.fmt.pix_mp
                   << "failed:" << strerror(errno);
        return false;
    }

    qCDebug(qLcEglfsKmsDebug) << "Capture device" << m_subdevFd << "format set:" << format.fmt.pix_mp;
    return true;
}

bool QLinuxMediaDevice::CaptureSubDevice::clearBuffers()
{
    struct v4l2_requestbuffers requestBuffers;
    memset(&requestBuffers, 0, sizeof(requestBuffers));
    requestBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    requestBuffers.memory = V4L2_MEMORY_DMABUF;
    requestBuffers.count = 0;
    if (ioctl(m_subdevFd, VIDIOC_REQBUFS, &requestBuffers) == -1) {
        qWarning("Capture device %d: VIDIOC_REQBUFS clear failed: %s", m_subdevFd, strerror(errno));
        return false;
    }
    qCDebug(qLcEglfsKmsDebug, "Capture device %d: Deallocced buffers with REQBUF, now has %d buffers", m_subdevFd, requestBuffers.count);
    Q_ASSERT(requestBuffers.count == 0);
    return true;
}

bool QLinuxMediaDevice::CaptureSubDevice::requestBuffer()
{
    struct v4l2_requestbuffers requestBuffers;
    memset(&requestBuffers, 0, sizeof(requestBuffers));
    requestBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    requestBuffers.memory = V4L2_MEMORY_DMABUF;
    requestBuffers.count = 1;
    if (ioctl(m_subdevFd, VIDIOC_REQBUFS, &requestBuffers) == -1) {
        if (errno == EINVAL)
            qWarning("Capture device %d: Multi-planar capture or dma buffers not supported", m_subdevFd);
        qWarning("Capture device %d: VIDIOC_REQBUFS failed: %s", m_subdevFd, strerror(errno));
        return false;
    }
    Q_ASSERT(requestBuffers.count == 1);
    return true;
}

bool QLinuxMediaDevice::CaptureSubDevice::queueBuffer(int dmabufFd, const QSize &bufferSize)
{
    const uint numPlanes = 1;
    struct v4l2_buffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buffer.memory = V4L2_MEMORY_DMABUF;
    buffer.index = 0;

    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    buffer.m.planes = planes;
    buffer.length = numPlanes;
    memset(planes, 0, sizeof(planes));
    for (uint i = 0; i < numPlanes; i++) {
        buffer.m.planes[i].m.fd = dmabufFd;
        buffer.m.planes[i].length = static_cast<uint>(bufferSize.width() * bufferSize.height() * 4); //TODO: don't harcode bpp
        buffer.m.planes[i].bytesused = static_cast<uint>(bufferSize.width() * bufferSize.height() * 4); //TODO: don't harcode bpp
    }

    if (ioctl(m_subdevFd, VIDIOC_QBUF, &buffer) == -1) {
        qWarning("Capture device %d: VIDIOC_QBUF failed for dma buffer with fd %d: %s",
                 m_subdevFd, dmabufFd, strerror(errno));
        return false;
    }

    return true;
}

bool QLinuxMediaDevice::CaptureSubDevice::dequeueBuffer()
{
    const int numPlanes = 1;
    struct v4l2_buffer buffer;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];

    memset(&buffer, 0, sizeof(buffer));
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buffer.memory = V4L2_MEMORY_DMABUF;
    buffer.index = 0;
    buffer.m.planes = planes;
    buffer.length = numPlanes;
    memset(planes, 0, sizeof(planes));

    if (ioctl(m_subdevFd, VIDIOC_DQBUF, &buffer) == -1) {
        qWarning("Capture device %d: VIDIOC_DQBUF failed: %s", m_subdevFd, strerror(errno));
        return false;
    }

    return true;
}

bool QLinuxMediaDevice::CaptureSubDevice::streamOn()
{
    return QLinuxMediaDevice::streamOn(m_subdevFd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
}

bool QLinuxMediaDevice::CaptureSubDevice::streamOff()
{
    return QLinuxMediaDevice::streamOff(m_subdevFd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
}

QLinuxMediaDevice::OutputSubDevice::OutputSubDevice(QLinuxMediaDevice *mediaDevice, const QString &name)
    : m_subdevFd(mediaDevice->openVideoDevice(name))
{
}

bool QLinuxMediaDevice::OutputSubDevice::setFormat(const QSize &size, uint32_t pixelFormat, uint32_t bytesPerLine)
{
    Q_ASSERT(size.isValid());
    struct v4l2_format format;
    memset(&format, 0, sizeof(struct v4l2_format));
    format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    if (ioctl(m_subdevFd, VIDIOC_G_FMT, &format) == -1) {
        qErrnoWarning("VIDIOC_G_FMT for output device failed");
        return false;
    }

    format.fmt.pix_mp.width = static_cast<uint>(size.width());
    format.fmt.pix_mp.height = static_cast<uint>(size.height());
    format.fmt.pix_mp.field = V4L2_FIELD_NONE;
    format.fmt.pix_mp.pixelformat = pixelFormat;
    format.fmt.pix_mp.num_planes = 1;
    format.fmt.pix_mp.flags = 0;
    format.fmt.pix_mp.plane_fmt[0].bytesperline = bytesPerLine;
    format.fmt.pix_mp.plane_fmt[0].sizeimage = 0;

    if (ioctl(m_subdevFd, VIDIOC_S_FMT, &format) == -1) {
        qWarning() << "Output device" << m_subdevFd << "VIDIOC_S_FMT with format" << format.fmt.pix_mp
                   << "failed:" << strerror(errno);
        return false;
    }

    qCDebug(qLcEglfsKmsDebug) << "Output device device" << m_subdevFd << "format set:" << format.fmt.pix_mp;
    return true;
}

bool QLinuxMediaDevice::OutputSubDevice::clearBuffers()
{
    struct v4l2_requestbuffers requestBuffers;
    memset(&requestBuffers, 0, sizeof(requestBuffers));
    requestBuffers.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    requestBuffers.memory = V4L2_MEMORY_DMABUF;
    requestBuffers.count = 0;
    if (ioctl(m_subdevFd, VIDIOC_REQBUFS, &requestBuffers) == -1) {
        qWarning("Output device %d: VIDIOC_REQBUFS clear failed: %s", m_subdevFd, strerror(errno));
        return false;
    }
    qCDebug(qLcEglfsKmsDebug, "Output device %d: Deallocced buffers with REQBUF, now has %d buffers", m_subdevFd, requestBuffers.count);
    Q_ASSERT(requestBuffers.count == 0);
    return true;
}

bool QLinuxMediaDevice::OutputSubDevice::requestBuffer()
{
    struct v4l2_requestbuffers requestBuffers;
    memset(&requestBuffers, 0, sizeof(requestBuffers));
    requestBuffers.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    requestBuffers.memory = V4L2_MEMORY_DMABUF;
    requestBuffers.count = 1;
    if (ioctl(m_subdevFd, VIDIOC_REQBUFS, &requestBuffers) == -1) {
        if (errno == EINVAL)
            qWarning("Output device %d: Multi-planar output or dma buffers not supported", m_subdevFd);
        qWarning("Output device %d: VIDIOC_REQBUFS failed: %s", m_subdevFd, strerror(errno));
        return false;
    }
    qCDebug(qLcEglfsKmsDebug) << "REQBUF returned" << requestBuffers.count << "buffers for output device" << m_subdevFd;
    return true;
}

bool QLinuxMediaDevice::OutputSubDevice::queueBuffer(int dmabufFd, uint bytesUsed, uint length)
{
    const int numPlanes = 1;
    struct v4l2_buffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    buffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    buffer.memory = V4L2_MEMORY_DMABUF;
    buffer.index = 0;
    buffer.length = numPlanes;
    buffer.bytesused = bytesUsed;
    buffer.field = V4L2_FIELD_NONE; //TODO: what is this?

    struct v4l2_plane planes[numPlanes];
    memset(planes, 0, sizeof(planes));
    buffer.m.planes = planes;

    for (int i = 0; i < numPlanes; i++) {
        buffer.m.planes[i].m.fd = dmabufFd;
        buffer.m.planes[i].length = length;
        buffer.m.planes[i].bytesused = bytesUsed;
    }

    if (ioctl(m_subdevFd, VIDIOC_QBUF, &buffer) == -1) {
        qWarning("Output device %d: VIDIOC_QBUF failed for dmabuf %d: %s", m_subdevFd, dmabufFd, strerror(errno));
        return false;
    }

    if (!(buffer.flags & V4L2_BUF_FLAG_QUEUED)) {
        qWarning() << "Queued flag not set on buffer for output device";
        return false;
    }

    return true;
}

bool QLinuxMediaDevice::OutputSubDevice::streamOn()
{
    return QLinuxMediaDevice::streamOn(m_subdevFd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
}

bool QLinuxMediaDevice::OutputSubDevice::streamOff()
{
    return QLinuxMediaDevice::streamOff(m_subdevFd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
}

int QLinuxMediaDevice::openVideoDevice(media_pad *pad)
{
    const char *deviceName = media_entity_get_devname(pad->entity);
    int fd = open(deviceName, O_RDWR);
    qCDebug(qLcEglfsKmsDebug) << "Opened video device:" << deviceName << "with fd" << fd;
    return fd;
}

bool QLinuxMediaDevice::setSubdevFormat(struct media_pad *pad, const QSize &size, uint32_t mbusFormat)
{
    Q_ASSERT(size.isValid());
    struct v4l2_mbus_framefmt format;
    format.width = static_cast<uint>(size.width());
    format.height = static_cast<uint>(size.height());
    format.code = mbusFormat;

    if (v4l2_subdev_set_format(pad->entity, &format, pad->index, V4L2_SUBDEV_FORMAT_ACTIVE)) {
        qWarning() << "Setting v4l2_subdev_set_format failed for format" << format;
        return false;
    }

    if (format.code != mbusFormat) {
        qWarning() << "Got" << mediaBusFmtToString(format.code) << "instead of"
                   << mediaBusFmtToString(mbusFormat) << "when setting subdevice format";
        return false;
    }

    qCDebug(qLcEglfsKmsDebug) << "Set format to" << format << "for entity" << pad->entity << "index" << pad->index;
    return true;
}

bool QLinuxMediaDevice::setSubdevAlpha(int subdevFd, qreal alpha)
{
    struct v4l2_control control;
    control.id = V4L2_CID_ALPHA_COMPONENT;
    control.value = static_cast<__s32>(alpha * 0xff);
    if (ioctl(subdevFd, VIDIOC_S_CTRL, &control) == -1) {
        qErrnoWarning("Setting alpha (%d) failed", control.value);
        return false;
    }
    return true;
}

bool QLinuxMediaDevice::setSubdevSelection(struct media_pad *pad, const QRect &geometry, uint target)
{
    Q_ASSERT(geometry.isValid());
    struct v4l2_rect rect;
    rect.left = geometry.left();
    rect.top = geometry.top();
    rect.width = static_cast<uint>(geometry.width());
    rect.height = static_cast<uint>(geometry.height());

    int ret = v4l2_subdev_set_selection(pad->entity, &rect, pad->index, target, V4L2_SUBDEV_FORMAT_ACTIVE);
    if (ret) {
        qWarning() << "Setting subdev selection failed.";
        return false;
    }

    return true;
}

bool QLinuxMediaDevice::setSubdevCrop(struct media_pad *pad, const QRect &geometry)
{
    return setSubdevSelection(pad, geometry, V4L2_SEL_TGT_CROP);
}

bool QLinuxMediaDevice::setSubdevCompose(struct media_pad *pad, const QRect &geometry)
{
    return setSubdevSelection(pad, geometry, V4L2_SEL_TGT_COMPOSE);
}

bool QLinuxMediaDevice::streamOn(int subDeviceFd, v4l2_buf_type bufferType)
{
    if (ioctl(subDeviceFd, VIDIOC_STREAMON, &bufferType) == -1) {
        qWarning("VIDIOC_STREAMON failed for subdevice %d: %s", subDeviceFd, strerror(errno));
        return false;
    }

    return true;
}

bool QLinuxMediaDevice::streamOff(int subDeviceFd, v4l2_buf_type bufferType)
{
    if (ioctl(subDeviceFd, VIDIOC_STREAMOFF, &bufferType) == -1) {
        qWarning("VIDIOC_STREAMOFF failed for subdevice %d: %s", subDeviceFd, strerror(errno));
        return false;
    }

    return true;
}

QT_END_NAMESPACE
