// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeglfskmsgbmscreen_p.h"
#include "qeglfskmsgbmdevice_p.h"
#include "qeglfskmsgbmcursor_p.h"

#include <private/qeglfsintegration_p.h>
#include <private/qeglfskmsintegration_p.h>

#include <QtCore/QLoggingCategory>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qtguiglobal_p.h>
#include <QtFbSupport/private/qfbvthandler_p.h>

#include <errno.h>

QT_BEGIN_NAMESPACE

QMutex QEglFSKmsGbmScreen::m_nonThreadedFlipMutex;

static inline uint32_t drmFormatToGbmFormat(uint32_t drmFormat)
{
    Q_ASSERT(DRM_FORMAT_XRGB8888 == GBM_FORMAT_XRGB8888);
    return drmFormat;
}

static inline uint32_t gbmFormatToDrmFormat(uint32_t gbmFormat)
{
    Q_ASSERT(DRM_FORMAT_XRGB8888 == GBM_FORMAT_XRGB8888);
    return gbmFormat;
}

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
    uint32_t handles[4] = { gbm_bo_get_handle(bo).u32 };
    uint32_t strides[4] = { gbm_bo_get_stride(bo) };
    uint32_t offsets[4] = { 0 };
    uint32_t pixelFormat = gbmFormatToDrmFormat(gbm_bo_get_format(bo));

    auto fb = std::make_unique<FrameBuffer>();
    qCDebug(qLcEglfsKmsDebug, "Adding FB, size %ux%u, DRM format 0x%x, stride %u, handle %u",
            width, height, pixelFormat, strides[0], handles[0]);

    int ret = drmModeAddFB2(device()->fd(), width, height, pixelFormat,
                            handles, strides, offsets, &fb->fb, 0);

    if (ret) {
        qWarning("Failed to create KMS FB!");
        return nullptr;
    }

    auto res = fb.get();
    gbm_bo_set_user_data(bo, fb.release(), bufferDestroyedHandler);
    return res;
}

QEglFSKmsGbmScreen::QEglFSKmsGbmScreen(QEglFSKmsDevice *device, const QKmsOutput &output, bool headless)
    : QEglFSKmsScreen(device, output, headless)
    , m_gbm_surface(nullptr)
    , m_gbm_bo_current(nullptr)
    , m_gbm_bo_next(nullptr)
    , m_flipPending(false)
    , m_cursor(nullptr)
    , m_cloneSource(nullptr)
{
}

QEglFSKmsGbmScreen::~QEglFSKmsGbmScreen()
{
    const int remainingScreenCount = qGuiApp->screens().count();
    qCDebug(qLcEglfsKmsDebug, "Screen dtor. Remaining screens: %d", remainingScreenCount);
    if (!remainingScreenCount && !device()->screenConfig()->separateScreens())
        static_cast<QEglFSKmsGbmDevice *>(device())->destroyGlobalCursor();
}

QPlatformCursor *QEglFSKmsGbmScreen::cursor() const
{
    QKmsScreenConfig *config = device()->screenConfig();
    if (config->headless())
        return nullptr;
    if (config->hwCursor()) {
        if (!config->separateScreens())
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

gbm_surface *QEglFSKmsGbmScreen::createSurface(EGLConfig eglConfig)
{
    if (!m_gbm_surface) {
        qCDebug(qLcEglfsKmsDebug, "Creating gbm_surface for screen %s", qPrintable(name()));

        const auto gbmDevice = static_cast<QEglFSKmsGbmDevice *>(device())->gbmDevice();
        // If there was no format override given in the config file,
        // query the native (here, gbm) format from the EGL config.
        const bool queryFromEgl = !m_output.drm_format_requested_by_user;
        if (queryFromEgl) {
            EGLint native_format = -1;
            EGLBoolean success = eglGetConfigAttrib(display(), eglConfig, EGL_NATIVE_VISUAL_ID, &native_format);
            qCDebug(qLcEglfsKmsDebug) << "Got native format" << Qt::hex << native_format << Qt::dec
                                      << "from eglGetConfigAttrib() with return code" << bool(success);

            if (success) {
                m_gbm_surface = gbm_surface_create(gbmDevice,
                                                   rawGeometry().width(),
                                                   rawGeometry().height(),
                                                   native_format,
                                                   gbmFlags());
                if (m_gbm_surface)
                    m_output.drm_format = gbmFormatToDrmFormat(native_format);
            }
        }

        const uint32_t gbmFormat = drmFormatToGbmFormat(m_output.drm_format);

        // Fallback for older drivers, and when "format" is explicitly specified
        // in the output config. (not guaranteed that the requested format works
        // of course, but do what we are told to)
        if (!m_gbm_surface) {
            if (queryFromEgl)
                qCDebug(qLcEglfsKmsDebug, "Could not create surface with EGL_NATIVE_VISUAL_ID, falling back to format %x", gbmFormat);
            m_gbm_surface = gbm_surface_create(gbmDevice,
                                           rawGeometry().width(),
                                           rawGeometry().height(),
                                           gbmFormat,
                                           gbmFlags());
        }

#ifndef Q_OS_VXWORKS
        // Fallback for some drivers, its required to request with modifiers
        if (!m_gbm_surface) {
            uint64_t modifier = DRM_FORMAT_MOD_LINEAR;

            m_gbm_surface = gbm_surface_create_with_modifiers(gbmDevice,
                                    rawGeometry().width(),
                                    rawGeometry().height(),
                                    gbmFormat,
                                    &modifier, 1);
        }
#endif
        // Fail here, as it would fail with the next usage of the GBM surface, which is very unexpected
        if (!m_gbm_surface)
            qFatal("Could not create GBM surface!");
    }
    return m_gbm_surface; // not owned, gets destroyed in QEglFSKmsGbmIntegration::destroyNativeWindow() via QEglFSKmsGbmWindow::invalidateSurface()
}

void QEglFSKmsGbmScreen::resetSurface()
{
    m_flipPending = false; // not necessarily true but enough to keep bo_next
    m_gbm_bo_current = nullptr;
    m_gbm_surface = nullptr;

    // Leave m_gbm_bo_next untouched. waitForFlip() should
    // still do its work, when called. Otherwise we end up
    // in device-is-busy errors if there is a new QWindow
    // created afterwards. (QTBUG-122663)

    // If not using atomic, will need a new drmModeSetCrtc if a new window
    // gets created later on (and so there's a new fb).
    if (!device()->hasAtomicSupport())
        needsNewModeSetForNextFb = true;
}

void QEglFSKmsGbmScreen::initCloning(QPlatformScreen *screenThisScreenClones,
                                     const QList<QPlatformScreen *> &screensCloningThisScreen)
{
    // clone destinations need to know the clone source
    const bool clonesAnother = screenThisScreenClones != nullptr;
    if (clonesAnother && !screensCloningThisScreen.isEmpty()) {
        qWarning("QEglFSKmsGbmScreen %s cannot be clone source and destination at the same time", qPrintable(name()));
        return;
    }
    if (clonesAnother) {
        m_cloneSource = static_cast<QEglFSKmsGbmScreen *>(screenThisScreenClones);
        qCDebug(qLcEglfsKmsDebug, "Screen %s clones %s", qPrintable(name()), qPrintable(m_cloneSource->name()));
    }

    // clone sources need to know their additional destinations
    for (QPlatformScreen *s : screensCloningThisScreen) {
        CloneDestination d;
        d.screen = static_cast<QEglFSKmsGbmScreen *>(s);
        m_cloneDests.append(d);
    }
}

void QEglFSKmsGbmScreen::ensureModeSet(uint32_t fb)
{
    QKmsOutput &op(output());
    const int fd = device()->fd();

    if (!op.mode_set || needsNewModeSetForNextFb) {
        op.mode_set = true;
        needsNewModeSetForNextFb = false;

        bool doModeSet = true;
        drmModeCrtcPtr currentMode = drmModeGetCrtc(fd, op.crtc_id);
        const bool alreadySet = currentMode && currentMode->buffer_id == fb && !memcmp(&currentMode->mode, &op.modes[op.mode], sizeof(drmModeModeInfo));
        if (currentMode)
            drmModeFreeCrtc(currentMode);
        if (alreadySet)
            doModeSet = false;

        if (doModeSet) {
            qCDebug(qLcEglfsKmsDebug, "Setting mode for screen %s", qPrintable(name()));

            if (device()->hasAtomicSupport()) {
#if QT_CONFIG(drm_atomic)
                drmModeAtomicReq *request = device()->threadLocalAtomicRequest();
                if (request) {
                    drmModeAtomicAddProperty(request, op.connector_id, op.crtcIdPropertyId, op.crtc_id);
                    drmModeAtomicAddProperty(request, op.crtc_id, op.modeIdPropertyId, op.mode_blob_id);
                    drmModeAtomicAddProperty(request, op.crtc_id, op.activePropertyId, 1);
                }
#endif
            } else {
                int ret = drmModeSetCrtc(fd,
                                         op.crtc_id,
                                         fb,
                                         0, 0,
                                         &op.connector_id, 1,
                                         &op.modes[op.mode]);

                if (ret == 0)
                    setPowerState(PowerStateOn);
                else
                    qErrnoWarning(errno, "Could not set DRM mode for screen %s", qPrintable(name()));
            }
        }
    }
}

void QEglFSKmsGbmScreen::nonThreadedPageFlipHandler(int fd,
                                                    unsigned int sequence,
                                                    unsigned int tv_sec,
                                                    unsigned int tv_usec,
                                                    void *user_data)
{
    // note that with cloning involved this callback is called also for screens that clone another one
    Q_UNUSED(fd);
    QEglFSKmsGbmScreen *screen = static_cast<QEglFSKmsGbmScreen *>(user_data);
    screen->flipFinished();
    screen->pageFlipped(sequence, tv_sec, tv_usec);
}

void QEglFSKmsGbmScreen::waitForFlipWithEventReader(QEglFSKmsGbmScreen *screen)
{
    m_flipMutex.lock();
    QEglFSKmsGbmDevice *dev = static_cast<QEglFSKmsGbmDevice *>(device());
    dev->eventReader()->startWaitFlip(screen, &m_flipMutex, &m_flipCond);
    m_flipCond.wait(&m_flipMutex);
    m_flipMutex.unlock();
    screen->flipFinished();
}

void QEglFSKmsGbmScreen::waitForFlip()
{
    if (m_headless || m_cloneSource)
        return;

    // Don't lock the mutex unless we actually need to
    if (!m_gbm_bo_next)
        return;

    QEglFSKmsGbmDevice *dev = static_cast<QEglFSKmsGbmDevice *>(device());
    if (dev->usesEventReader()) {
        waitForFlipWithEventReader(this);
        // Now, unlike on the other code path, we need to ensure the
        // flips have completed for the screens that just scan out
        // this one's content, because the eventReader's wait is
        // per-output.
        for (CloneDestination &d : m_cloneDests) {
            if (d.screen != this)
                waitForFlipWithEventReader(d.screen);
        }
    } else {
        QMutexLocker lock(&m_nonThreadedFlipMutex);
        while (m_gbm_bo_next) {
            drmEventContext drmEvent;
            memset(&drmEvent, 0, sizeof(drmEvent));
            drmEvent.version = 2;
            drmEvent.vblank_handler = nullptr;
            drmEvent.page_flip_handler = nonThreadedPageFlipHandler;
            drmHandleEvent(device()->fd(), &drmEvent);
        }
    }

#if QT_CONFIG(drm_atomic)
    device()->threadLocalAtomicReset();
#endif
}

#if QT_CONFIG(drm_atomic)
static void addAtomicFlip(drmModeAtomicReq *request, const QKmsOutput &output, uint32_t fb)
{
    drmModeAtomicAddProperty(request, output.eglfs_plane->id,
                             output.eglfs_plane->framebufferPropertyId, fb);

    drmModeAtomicAddProperty(request, output.eglfs_plane->id,
                             output.eglfs_plane->crtcPropertyId, output.crtc_id);

    drmModeAtomicAddProperty(request, output.eglfs_plane->id,
                             output.eglfs_plane->srcwidthPropertyId, output.size.width() << 16);

    drmModeAtomicAddProperty(request, output.eglfs_plane->id,
                             output.eglfs_plane->srcXPropertyId, 0);

    drmModeAtomicAddProperty(request, output.eglfs_plane->id,
                             output.eglfs_plane->srcYPropertyId, 0);

    drmModeAtomicAddProperty(request, output.eglfs_plane->id,
                             output.eglfs_plane->srcheightPropertyId, output.size.height() << 16);

    drmModeAtomicAddProperty(request, output.eglfs_plane->id,
                             output.eglfs_plane->crtcXPropertyId, 0);

    drmModeAtomicAddProperty(request, output.eglfs_plane->id,
                             output.eglfs_plane->crtcYPropertyId, 0);

    drmModeAtomicAddProperty(request, output.eglfs_plane->id,
                             output.eglfs_plane->crtcwidthPropertyId, output.modes[output.mode].hdisplay);

    drmModeAtomicAddProperty(request, output.eglfs_plane->id,
                             output.eglfs_plane->crtcheightPropertyId, output.modes[output.mode].vdisplay);
}
#endif

void QEglFSKmsGbmScreen::flip()
{
    // For headless screen just return silently. It is not necessarily an error
    // to end up here, so show no warnings.
    if (m_headless)
        return;

    if (m_cloneSource) {
        qWarning("Screen %s clones another screen. swapBuffers() not allowed.", qPrintable(name()));
        return;
    }

    if (!m_gbm_surface) {
        qWarning("Cannot sync before platform init!");
        return;
    }

    m_gbm_bo_next = gbm_surface_lock_front_buffer(m_gbm_surface);
    if (!m_gbm_bo_next) {
        qWarning("Could not lock GBM surface front buffer for screen %s", qPrintable(name()));
        return;
    }

    auto gbmRelease = qScopeGuard([this]{
        m_flipPending = false;
        gbm_surface_release_buffer(m_gbm_surface, m_gbm_bo_next);
        m_gbm_bo_next = nullptr;
    });

    FrameBuffer *fb = framebufferForBufferObject(m_gbm_bo_next);
    if (!fb) {
        qWarning("FrameBuffer not available. Cannot flip");
        return;
    }
    ensureModeSet(fb->fb);

    const QKmsOutput &thisOutput(output());
    const int fd = device()->fd();
    m_flipPending = true;

    if (device()->hasAtomicSupport()) {
#if QT_CONFIG(drm_atomic)
        drmModeAtomicReq *request = device()->threadLocalAtomicRequest();
        if (request) {
            addAtomicFlip(request, thisOutput, fb->fb);
            static int zpos = qEnvironmentVariableIntValue("QT_QPA_EGLFS_KMS_ZPOS");
            if (zpos) {
                drmModeAtomicAddProperty(request, thisOutput.eglfs_plane->id,
                                         thisOutput.eglfs_plane->zposPropertyId, zpos);
            }
            static uint blendOp = uint(qEnvironmentVariableIntValue("QT_QPA_EGLFS_KMS_BLEND_OP"));
            if (blendOp) {
                drmModeAtomicAddProperty(request, thisOutput.eglfs_plane->id,
                                         thisOutput.eglfs_plane->blendOpPropertyId, blendOp);
            }
        }
#endif
    } else {
        int ret = drmModePageFlip(fd,
                                  thisOutput.crtc_id,
                                  fb->fb,
                                  DRM_MODE_PAGE_FLIP_EVENT,
                                  this);
        if (ret) {
            qErrnoWarning("Could not queue DRM page flip on screen %s", qPrintable(name()));
            return;
        }
    }

    for (CloneDestination &d : m_cloneDests) {
        if (d.screen != this) {
            d.screen->ensureModeSet(fb->fb);
            d.cloneFlipPending = true;
            const QKmsOutput &destOutput(d.screen->output());

            if (device()->hasAtomicSupport()) {
#if QT_CONFIG(drm_atomic)
                drmModeAtomicReq *request = device()->threadLocalAtomicRequest();
                if (request)
                    addAtomicFlip(request, destOutput, fb->fb);

                // ### This path is broken. On the other branch we can easily
                // pass in d.screen as the user_data for drmModePageFlip, but
                // using one atomic request breaks down here since we get events
                // with the same user_data passed to drmModeAtomicCommit.  Until
                // this gets reworked (multiple requests?) screen cloning is not
                // compatible with atomic.
#endif
            } else {
                int ret = drmModePageFlip(fd,
                                          destOutput.crtc_id,
                                          fb->fb,
                                          DRM_MODE_PAGE_FLIP_EVENT,
                                          d.screen);
                if (ret) {
                    qErrnoWarning("Could not queue DRM page flip for screen %s (clones screen %s)",
                                  qPrintable(d.screen->name()),
                                  qPrintable(name()));
                    d.cloneFlipPending = false;
                }
            }
        }
    }

    if (device()->hasAtomicSupport()) {
#if QT_CONFIG(drm_atomic)
        if (!device()->threadLocalAtomicCommit(this)) {
            return;
        }
#endif
    }

    gbmRelease.dismiss();
}

void QEglFSKmsGbmScreen::flipFinished()
{
    if (m_cloneSource) {
        m_cloneSource->cloneDestFlipFinished(this);
        return;
    }

    m_flipPending = false;
    updateFlipStatus();
}

void QEglFSKmsGbmScreen::cloneDestFlipFinished(QEglFSKmsGbmScreen *cloneDestScreen)
{
    for (CloneDestination &d : m_cloneDests) {
        if (d.screen == cloneDestScreen) {
            d.cloneFlipPending = false;
            break;
        }
    }
    updateFlipStatus();
}

void QEglFSKmsGbmScreen::updateFlipStatus()
{
    // only for 'real' outputs that own the color buffer, i.e. that are not cloning another one
    if (m_cloneSource)
        return;

    // proceed only if flips for both this and all others that clone this have finished
    if (m_flipPending)
        return;

    for (const CloneDestination &d : std::as_const(m_cloneDests)) {
        if (d.cloneFlipPending)
            return;
    }

    if (m_gbm_bo_current) {
        gbm_surface_release_buffer(m_gbm_surface,
                                   m_gbm_bo_current);
    }

    m_gbm_bo_current = m_gbm_bo_next;
    m_gbm_bo_next = nullptr;
}

QT_END_NAMESPACE
