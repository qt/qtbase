/****************************************************************************
**
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

// Experimental DRM dumb buffer backend.
//
// TODO:
// Multiscreen: QWindow-QScreen(-output) association. Needs some reorg (device cannot be owned by screen)
// Find card via devicediscovery like in eglfs_kms.
// Mode restore like QEglFSKmsInterruptHandler.
// grabWindow

#include "qlinuxfbdrmscreen.h"
#include <QLoggingCategory>
#include <QGuiApplication>
#include <QPainter>
#include <QtFbSupport/private/qfbcursor_p.h>
#include <QtFbSupport/private/qfbwindow_p.h>
#include <QtKmsSupport/private/qkmsdevice_p.h>
#include <QtCore/private/qcore_unix_p.h>
#include <sys/mman.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcFbDrm, "qt.qpa.fb")

static const int BUFFER_COUNT = 2;

class QLinuxFbDevice : public QKmsDevice
{
public:
    struct Framebuffer {
        Framebuffer() : handle(0), pitch(0), size(0), fb(0), p(MAP_FAILED) { }
        uint32_t handle;
        uint32_t pitch;
        uint64_t size;
        uint32_t fb;
        void *p;
        QImage wrapper;
    };

    struct Output {
        Output() : backFb(0), flipped(false) { }
        QKmsOutput kmsOutput;
        Framebuffer fb[BUFFER_COUNT];
        QRegion dirty[BUFFER_COUNT];
        int backFb;
        bool flipped;
        QSize currentRes() const {
            const drmModeModeInfo &modeInfo(kmsOutput.modes[kmsOutput.mode]);
            return QSize(modeInfo.hdisplay, modeInfo.vdisplay);
        }
    };

    QLinuxFbDevice(QKmsScreenConfig *screenConfig);

    bool open() override;
    void close() override;

    void createFramebuffers();
    void destroyFramebuffers();
    void setMode();

    void swapBuffers(Output *output);

    int outputCount() const { return m_outputs.count(); }
    Output *output(int idx) { return &m_outputs[idx]; }

private:
    void *nativeDisplay() const override;
    QPlatformScreen *createScreen(const QKmsOutput &output) override;
    void registerScreen(QPlatformScreen *screen,
                        bool isPrimary,
                        const QPoint &virtualPos,
                        const QList<QPlatformScreen *> &virtualSiblings) override;

    bool createFramebuffer(Output *output, int bufferIdx);
    void destroyFramebuffer(Output *output, int bufferIdx);

    static void pageFlipHandler(int fd, unsigned int sequence,
                                unsigned int tv_sec, unsigned int tv_usec, void *user_data);

    QVector<Output> m_outputs;
};

QLinuxFbDevice::QLinuxFbDevice(QKmsScreenConfig *screenConfig)
    : QKmsDevice(screenConfig, QStringLiteral("/dev/dri/card0"))
{
}

bool QLinuxFbDevice::open()
{
    int fd = qt_safe_open(devicePath().toLocal8Bit().constData(), O_RDWR | O_CLOEXEC);
    if (fd == -1) {
        qErrnoWarning("Could not open DRM device %s", qPrintable(devicePath()));
        return false;
    }

    uint64_t hasDumbBuf = 0;
    if (drmGetCap(fd, DRM_CAP_DUMB_BUFFER, &hasDumbBuf) == -1 || !hasDumbBuf) {
        qWarning("Dumb buffers not supported");
        qt_safe_close(fd);
        return false;
    }

    setFd(fd);

    qCDebug(qLcFbDrm, "DRM device %s opened", qPrintable(devicePath()));

    return true;
}

void QLinuxFbDevice::close()
{
    for (Output &output : m_outputs)
        output.kmsOutput.cleanup(this); // restore mode

    m_outputs.clear();

    if (fd() != -1) {
        qCDebug(qLcFbDrm, "Closing DRM device");
        qt_safe_close(fd());
        setFd(-1);
    }
}

void *QLinuxFbDevice::nativeDisplay() const
{
    Q_UNREACHABLE();
    return nullptr;
}

QPlatformScreen *QLinuxFbDevice::createScreen(const QKmsOutput &output)
{
    qCDebug(qLcFbDrm, "Got a new output: %s", qPrintable(output.name));
    Output o;
    o.kmsOutput = output;
    m_outputs.append(o);
    return nullptr; // no platformscreen, we are not a platform plugin
}

void QLinuxFbDevice::registerScreen(QPlatformScreen *screen,
                                    bool isPrimary,
                                    const QPoint &virtualPos,
                                    const QList<QPlatformScreen *> &virtualSiblings)
{
    Q_UNUSED(screen);
    Q_UNUSED(isPrimary);
    Q_UNUSED(virtualPos);
    Q_UNUSED(virtualSiblings);
    Q_UNREACHABLE();
}

static uint32_t bppForDrmFormat(uint32_t drmFormat)
{
    switch (drmFormat) {
    case DRM_FORMAT_RGB565:
    case DRM_FORMAT_BGR565:
        return 16;
    default:
        return 32;
    }
}

static int depthForDrmFormat(uint32_t drmFormat)
{
    switch (drmFormat) {
    case DRM_FORMAT_RGB565:
    case DRM_FORMAT_BGR565:
        return 16;
    case DRM_FORMAT_XRGB8888:
    case DRM_FORMAT_XBGR8888:
        return 24;
    case DRM_FORMAT_XRGB2101010:
    case DRM_FORMAT_XBGR2101010:
        return 30;
    default:
        return 32;
    }
}

static QImage::Format formatForDrmFormat(uint32_t drmFormat)
{
    switch (drmFormat) {
    case DRM_FORMAT_XRGB8888:
    case DRM_FORMAT_XBGR8888:
        return QImage::Format_RGB32;
    case DRM_FORMAT_ARGB8888:
    case DRM_FORMAT_ABGR8888:
        return QImage::Format_ARGB32;
    case DRM_FORMAT_RGB565:
    case DRM_FORMAT_BGR565:
        return QImage::Format_RGB16;
    case DRM_FORMAT_XRGB2101010:
    case DRM_FORMAT_XBGR2101010:
        return QImage::Format_RGB30;
    case DRM_FORMAT_ARGB2101010:
    case DRM_FORMAT_ABGR2101010:
        return QImage::Format_A2RGB30_Premultiplied;
    default:
        return QImage::Format_ARGB32;
    }
}

bool QLinuxFbDevice::createFramebuffer(QLinuxFbDevice::Output *output, int bufferIdx)
{
    const QSize size = output->currentRes();
    const uint32_t w = size.width();
    const uint32_t h = size.height();
    const uint32_t bpp = bppForDrmFormat(output->kmsOutput.drm_format);
    drm_mode_create_dumb creq = {
        h,
        w,
        bpp,
        0, 0, 0, 0
    };
    if (drmIoctl(fd(), DRM_IOCTL_MODE_CREATE_DUMB, &creq) == -1) {
        qErrnoWarning(errno, "Failed to create dumb buffer");
        return false;
    }

    Framebuffer &fb(output->fb[bufferIdx]);
    fb.handle = creq.handle;
    fb.pitch = creq.pitch;
    fb.size = creq.size;
    qCDebug(qLcFbDrm, "Got a dumb buffer for size %dx%d and bpp %u: handle %u, pitch %u, size %u",
            w, h, bpp, fb.handle, fb.pitch, (uint) fb.size);

    uint32_t handles[4] = { fb.handle };
    uint32_t strides[4] = { fb.pitch };
    uint32_t offsets[4] = { 0 };

    if (drmModeAddFB2(fd(), w, h, output->kmsOutput.drm_format,
                      handles, strides, offsets, &fb.fb, 0) == -1) {
        qErrnoWarning(errno, "Failed to add FB");
        return false;
    }

    drm_mode_map_dumb mreq = {
        fb.handle,
        0, 0
    };
    if (drmIoctl(fd(), DRM_IOCTL_MODE_MAP_DUMB, &mreq) == -1) {
        qErrnoWarning(errno, "Failed to map dumb buffer");
        return false;
    }
    fb.p = mmap(0, fb.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd(), mreq.offset);
    if (fb.p == MAP_FAILED) {
        qErrnoWarning(errno, "Failed to mmap dumb buffer");
        return false;
    }

    qCDebug(qLcFbDrm, "FB is %u (DRM format 0x%x), mapped at %p", fb.fb, output->kmsOutput.drm_format, fb.p);
    memset(fb.p, 0, fb.size);

    fb.wrapper = QImage(static_cast<uchar *>(fb.p), w, h, fb.pitch, formatForDrmFormat(output->kmsOutput.drm_format));

    return true;
}

void QLinuxFbDevice::createFramebuffers()
{
    for (Output &output : m_outputs) {
        for (int i = 0; i < BUFFER_COUNT; ++i) {
            if (!createFramebuffer(&output, i))
                return;
        }
        output.backFb = 0;
        output.flipped = false;
    }
}

void QLinuxFbDevice::destroyFramebuffer(QLinuxFbDevice::Output *output, int bufferIdx)
{
    Framebuffer &fb(output->fb[bufferIdx]);
    if (fb.p != MAP_FAILED)
        munmap(fb.p, fb.size);
    if (fb.fb) {
        if (drmModeRmFB(fd(), fb.fb) == -1)
            qErrnoWarning("Failed to remove fb");
    }
    if (fb.handle) {
        drm_mode_destroy_dumb dreq = { fb.handle };
        if (drmIoctl(fd(), DRM_IOCTL_MODE_DESTROY_DUMB, &dreq) == -1)
            qErrnoWarning(errno, "Failed to destroy dumb buffer %u", fb.handle);
    }
    fb = Framebuffer();
}

void QLinuxFbDevice::destroyFramebuffers()
{
    for (Output &output : m_outputs) {
        for (int i = 0; i < BUFFER_COUNT; ++i)
            destroyFramebuffer(&output, i);
    }
}

void QLinuxFbDevice::setMode()
{
    for (Output &output : m_outputs) {
        drmModeModeInfo &modeInfo(output.kmsOutput.modes[output.kmsOutput.mode]);
        if (drmModeSetCrtc(fd(), output.kmsOutput.crtc_id, output.fb[0].fb, 0, 0,
                           &output.kmsOutput.connector_id, 1, &modeInfo) == -1) {
            qErrnoWarning(errno, "Failed to set mode");
            return;
        }

        output.kmsOutput.mode_set = true; // have cleanup() to restore the mode
        output.kmsOutput.setPowerState(this, QPlatformScreen::PowerStateOn);
    }
}

void QLinuxFbDevice::pageFlipHandler(int fd, unsigned int sequence,
                                     unsigned int tv_sec, unsigned int tv_usec,
                                     void *user_data)
{
    Q_UNUSED(fd);
    Q_UNUSED(sequence);
    Q_UNUSED(tv_sec);
    Q_UNUSED(tv_usec);

    Output *output = static_cast<Output *>(user_data);
    output->backFb = (output->backFb + 1) % BUFFER_COUNT;
}

void QLinuxFbDevice::swapBuffers(Output *output)
{
    Framebuffer &fb(output->fb[output->backFb]);
    if (drmModePageFlip(fd(), output->kmsOutput.crtc_id, fb.fb, DRM_MODE_PAGE_FLIP_EVENT, output) == -1) {
        qErrnoWarning(errno, "Page flip failed");
        return;
    }

    const int fbIdx = output->backFb;
    while (output->backFb == fbIdx) {
        drmEventContext drmEvent;
        memset(&drmEvent, 0, sizeof(drmEvent));
        drmEvent.version = 2;
        drmEvent.vblank_handler = nullptr;
        drmEvent.page_flip_handler = pageFlipHandler;
        // Blocks until there is something to read on the drm fd
        // and calls back pageFlipHandler once the flip completes.
        drmHandleEvent(fd(), &drmEvent);
    }
}

QLinuxFbDrmScreen::QLinuxFbDrmScreen(const QStringList &args)
    : m_screenConfig(nullptr),
      m_device(nullptr)
{
    Q_UNUSED(args);
}

QLinuxFbDrmScreen::~QLinuxFbDrmScreen()
{
    if (m_device) {
        m_device->destroyFramebuffers();
        m_device->close();
        delete m_device;
    }
    delete m_screenConfig;
}

bool QLinuxFbDrmScreen::initialize()
{
    m_screenConfig = new QKmsScreenConfig;
    m_device = new QLinuxFbDevice(m_screenConfig);
    if (!m_device->open())
        return false;

    // Discover outputs. Calls back Device::createScreen().
    m_device->createScreens();
    // Now off to dumb buffer specifics.
    m_device->createFramebuffers();
    // Do the modesetting.
    m_device->setMode();

    QLinuxFbDevice::Output *output(m_device->output(0));

    mGeometry = QRect(QPoint(0, 0), output->currentRes());
    mDepth = depthForDrmFormat(output->kmsOutput.drm_format);
    mFormat = formatForDrmFormat(output->kmsOutput.drm_format);
    mPhysicalSize = output->kmsOutput.physical_size;
    qCDebug(qLcFbDrm) << mGeometry << mPhysicalSize << mDepth << mFormat;

    QFbScreen::initializeCompositor();

    mCursor = new QFbCursor(this);

    return true;
}

QRegion QLinuxFbDrmScreen::doRedraw()
{
    const QRegion dirty = QFbScreen::doRedraw();
    if (dirty.isEmpty())
        return dirty;

    QLinuxFbDevice::Output *output(m_device->output(0));

    for (int i = 0; i < BUFFER_COUNT; ++i)
        output->dirty[i] += dirty;

    if (output->fb[output->backFb].wrapper.isNull())
        return dirty;

    QPainter pntr(&output->fb[output->backFb].wrapper);
    // Image has alpha but no need for blending at this stage.
    // Do not waste time with the default SourceOver.
    pntr.setCompositionMode(QPainter::CompositionMode_Source);
    for (const QRect &rect : qAsConst(output->dirty[output->backFb]))
        pntr.drawImage(rect, mScreenImage, rect);
    pntr.end();

    output->dirty[output->backFb] = QRegion();

    m_device->swapBuffers(output);

    return dirty;
}

QPixmap QLinuxFbDrmScreen::grabWindow(WId wid, int x, int y, int width, int height) const
{
    Q_UNUSED(wid);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(width);
    Q_UNUSED(height);

    return QPixmap();
}

QT_END_NAMESPACE
