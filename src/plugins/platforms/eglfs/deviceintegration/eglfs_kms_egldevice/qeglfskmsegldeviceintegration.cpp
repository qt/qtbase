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

#include "qeglfskmsegldeviceintegration.h"
#include <QLoggingCategory>
#include <private/qmath_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcEglfsKmsDebug, "qt.qpa.eglfs.kms")

QEglFSKmsEglDeviceIntegration::QEglFSKmsEglDeviceIntegration()
    : m_dri_fd(-1)
    , m_egl_device(EGL_NO_DEVICE_EXT)
    , m_egl_display(EGL_NO_DISPLAY)
    , m_drm_connector(Q_NULLPTR)
    , m_drm_encoder(Q_NULLPTR)
    , m_drm_crtc(0)
    , m_funcs(Q_NULLPTR)
{
    qCDebug(qLcEglfsKmsDebug, "New DRM/KMS on EGLDevice integration created");
}

void QEglFSKmsEglDeviceIntegration::platformInit()
{
    if (!query_egl_device())
        qFatal("Could not set up EGL device!");

    const char *deviceName = m_funcs->query_device_string(m_egl_device, EGL_DRM_DEVICE_FILE_EXT);
    if (!deviceName)
        qFatal("Failed to query device name from EGLDevice");

    qCDebug(qLcEglfsKmsDebug, "Opening %s", deviceName);

    m_dri_fd = drmOpen(deviceName, Q_NULLPTR);
    if (m_dri_fd < 0)
        qFatal("Could not open DRM device");

    if (!setup_kms())
        qFatal("Could not set up KMS on device %s!", m_device.constData());

    qCDebug(qLcEglfsKmsDebug, "DRM/KMS initialized");
}

void QEglFSKmsEglDeviceIntegration::platformDestroy()
{
    if (qt_safe_close(m_dri_fd) == -1)
        qErrnoWarning("Could not close DRM device");

    m_dri_fd = -1;

    delete m_funcs;
    m_funcs = Q_NULLPTR;
}

EGLNativeDisplayType QEglFSKmsEglDeviceIntegration::platformDisplay() const
{
    return static_cast<EGLNativeDisplayType>(m_egl_device);
}

EGLDisplay QEglFSKmsEglDeviceIntegration::createDisplay(EGLNativeDisplayType nativeDisplay)
{
    qCDebug(qLcEglfsKmsDebug, "Creating display");

    EGLDisplay display;

    if (m_funcs->has_egl_platform_device) {
        display = m_funcs->get_platform_display(EGL_PLATFORM_DEVICE_EXT, nativeDisplay, Q_NULLPTR);
    } else {
        qWarning("EGL_EXT_platform_device not available, falling back to legacy path!");
        display = eglGetDisplay(nativeDisplay);
    }

    if (display == EGL_NO_DISPLAY)
        qFatal("Could not get EGL display");

    EGLint major, minor;
    if (!eglInitialize(display, &major, &minor))
        qFatal("Could not initialize egl display");

    if (!eglBindAPI(EGL_OPENGL_ES_API))
        qFatal("Failed to bind EGL_OPENGL_ES_API\n");

    return display;
}

QSizeF QEglFSKmsEglDeviceIntegration::physicalScreenSize() const
{
    const int defaultPhysicalDpi = 100;
    static const int width = qEnvironmentVariableIntValue("QT_QPA_EGLFS_PHYSICAL_WIDTH");
    static const int height = qEnvironmentVariableIntValue("QT_QPA_EGLFS_PHYSICAL_HEIGHT");
    QSizeF size(width, height);
    if (size.isEmpty()) {
        size = QSizeF(m_drm_connector->mmWidth, m_drm_connector->mmHeight);
        if (size.isEmpty()) {
            const float pixelsPerMm = Q_MM_PER_INCH / defaultPhysicalDpi;
            size = QSizeF(screenSize().width() * pixelsPerMm, screenSize().height() * pixelsPerMm);
        }
    }
    return size;
}

QSize QEglFSKmsEglDeviceIntegration::screenSize() const
{
    return QSize(m_drm_mode.hdisplay, m_drm_mode.vdisplay);
}

int QEglFSKmsEglDeviceIntegration::screenDepth() const
{
    return 32;
}

QSurfaceFormat QEglFSKmsEglDeviceIntegration::surfaceFormatFor(const QSurfaceFormat &inputFormat) const
{
    QSurfaceFormat format(inputFormat);
    format.setRenderableType(QSurfaceFormat::OpenGLES);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);
    return format;
}

EGLint QEglFSKmsEglDeviceIntegration::surfaceType() const
{
    return EGL_STREAM_BIT_KHR;
}

class QEglJetsonTK1Window : public QEglFSWindow
{
public:
    QEglJetsonTK1Window(QWindow *w, const QEglFSKmsEglDeviceIntegration *integration)
        : QEglFSWindow(w)
        , m_integration(integration)
        , m_egl_stream(EGL_NO_STREAM_KHR)
    { }

    void invalidateSurface() Q_DECL_OVERRIDE;
    void resetSurface() Q_DECL_OVERRIDE;

    const QEglFSKmsEglDeviceIntegration *m_integration;
    EGLStreamKHR m_egl_stream;
    EGLint m_latency;
};

void QEglJetsonTK1Window::invalidateSurface()
{
    QEglFSWindow::invalidateSurface();
    m_integration->m_funcs->destroy_stream(screen()->display(), m_egl_stream);
}

void QEglJetsonTK1Window::resetSurface()
{
    qCDebug(qLcEglfsKmsDebug, "Creating stream");

    EGLDisplay display = screen()->display();
    EGLOutputLayerEXT layer = EGL_NO_OUTPUT_LAYER_EXT;
    EGLint count;

    m_egl_stream = m_integration->m_funcs->create_stream(display, Q_NULLPTR);
    if (m_egl_stream == EGL_NO_STREAM_KHR) {
        qWarning("resetSurface: Couldn't create EGLStream for native window");
        return;
    }

    qCDebug(qLcEglfsKmsDebug, "Created stream %p on display %p", m_egl_stream, display);

    if (!m_integration->m_funcs->get_output_layers(display, Q_NULLPTR, Q_NULLPTR, 0, &count) || count == 0) {
        qWarning("No output layers found");
        return;
    }

    qCDebug(qLcEglfsKmsDebug, "Output has %d layers", count);

    QVector<EGLOutputLayerEXT> layers;
    layers.resize(count);
    EGLint actualCount;
    if (!m_integration->m_funcs->get_output_layers(display, Q_NULLPTR, layers.data(), count, &actualCount)) {
        qWarning("Failed to get layers");
        return;
    }

    for (int i = 0; i < actualCount; ++i) {
        EGLAttrib id;
        if (m_integration->m_funcs->query_output_layer_attrib(display, layers[i], EGL_DRM_CRTC_EXT, &id)) {
            qCDebug(qLcEglfsKmsDebug, "  [%d] layer %p - crtc %d", i, layers[i], (int) id);
            if (id == EGLAttrib(m_integration->m_drm_crtc))
                layer = layers[i];
        } else if (m_integration->m_funcs->query_output_layer_attrib(display, layers[i], EGL_DRM_PLANE_EXT, &id)) {
            // Not used yet, just for debugging.
            qCDebug(qLcEglfsKmsDebug, "  [%d] layer %p - plane %d", i, layers[i], (int) id);
        } else {
            qCDebug(qLcEglfsKmsDebug, "  [%d] layer %p - unknown", i, layers[i]);
        }
    }

    QByteArray reqLayerIndex = qgetenv("QT_QPA_EGLFS_LAYER_INDEX");
    if (!reqLayerIndex.isEmpty()) {
        int idx = reqLayerIndex.toInt();
        if (idx >= 0 && idx < layers.count())
            layer = layers[idx];
    }

    if (layer == EGL_NO_OUTPUT_LAYER_EXT) {
        qWarning("resetSurface: Couldn't get EGLOutputLayer for native window");
        return;
    }

    qCDebug(qLcEglfsKmsDebug, "Using layer %p", layer);

    if (!m_integration->m_funcs->stream_consumer_output(display, m_egl_stream, layer))
        qWarning("resetSurface: Unable to connect stream");

    m_config = QEglFSIntegration::chooseConfig(display, m_integration->surfaceFormatFor(window()->requestedFormat()));
    m_format = q_glFormatFromConfig(display, m_config);
    qCDebug(qLcEglfsKmsDebug) << "Stream producer format is" << m_format;

    const int w = m_integration->screenSize().width();
    const int h = m_integration->screenSize().height();
    qCDebug(qLcEglfsKmsDebug, "Creating stream producer surface of size %dx%d", w, h);

    const EGLint stream_producer_attribs[] = {
        EGL_WIDTH,  w,
        EGL_HEIGHT, h,
        EGL_NONE
    };

    m_surface = m_integration->m_funcs->create_stream_producer_surface(display, m_config, m_egl_stream, stream_producer_attribs);
    if (m_surface == EGL_NO_SURFACE)
        return;

    qCDebug(qLcEglfsKmsDebug, "Created stream producer surface %p", m_surface);
}

QEglFSWindow *QEglFSKmsEglDeviceIntegration::createWindow(QWindow *window) const
{
    QEglJetsonTK1Window *eglWindow = new QEglJetsonTK1Window(window, this);

    m_funcs->initialize(eglWindow->screen()->display());
    if (!(m_funcs->has_egl_output_base && m_funcs->has_egl_output_drm && m_funcs->has_egl_stream
          && m_funcs->has_egl_stream_producer_eglsurface && m_funcs->has_egl_stream_consumer_egloutput))
        qFatal("Required extensions missing!");

    return eglWindow;
}

bool QEglFSKmsEglDeviceIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case QPlatformIntegration::ThreadedPixmaps:
    case QPlatformIntegration::OpenGL:
    case QPlatformIntegration::ThreadedOpenGL:
    case QPlatformIntegration::BufferQueueingOpenGL:
        return true;
    default:
        return false;
    }
}

void QEglFSKmsEglDeviceIntegration::waitForVSync(QPlatformSurface *) const
{
    static bool mode_set = false;

    if (!mode_set) {
        mode_set = true;

        drmModeCrtcPtr currentMode = drmModeGetCrtc(m_dri_fd, m_drm_crtc);
        const bool alreadySet = currentMode
            && currentMode->width == m_drm_mode.hdisplay
            && currentMode->height == m_drm_mode.vdisplay;
        if (currentMode)
            drmModeFreeCrtc(currentMode);
        if (alreadySet) {
            qCDebug(qLcEglfsKmsDebug, "Mode already set");
            return;
        }

        qCDebug(qLcEglfsKmsDebug, "Setting mode");
        int ret = drmModeSetCrtc(m_dri_fd, m_drm_crtc,
                                 -1, 0, 0,
                                 &m_drm_connector->connector_id, 1,
                                 const_cast<const drmModeModeInfoPtr>(&m_drm_mode));
        if (ret)
            qFatal("drmModeSetCrtc failed");
    }
}

qreal QEglFSKmsEglDeviceIntegration::refreshRate() const
{
    quint32 refresh = m_drm_mode.vrefresh;
    return refresh > 0 ? refresh : 60;
}

bool QEglFSKmsEglDeviceIntegration::supportsSurfacelessContexts() const
{
    // Returning false disables the usage of EGL_KHR_surfaceless_context even when the
    // extension is available. This is just what we need since, at least with NVIDIA
    // 352.00 making a null surface current with a context breaks.
    return false;
}

bool QEglFSKmsEglDeviceIntegration::setup_kms()
{
    drmModeRes *resources;
    drmModeConnector *connector;
    drmModeEncoder *encoder;
    quint32 crtc = 0;
    int i;

    resources = drmModeGetResources(m_dri_fd);
    if (!resources) {
        qWarning("drmModeGetResources failed");
        return false;
    }

    for (i = 0; i < resources->count_connectors; i++) {
        connector = drmModeGetConnector(m_dri_fd, resources->connectors[i]);
        if (!connector)
            continue;

        if (connector->connection == DRM_MODE_CONNECTED && connector->count_modes > 0)
            break;

        drmModeFreeConnector(connector);
    }

    if (i == resources->count_connectors) {
        qWarning("No currently active connector found.");
        return false;
    }

    qCDebug(qLcEglfsKmsDebug, "Using connector with type %d", connector->connector_type);

    for (i = 0; i < resources->count_encoders; i++) {
        encoder = drmModeGetEncoder(m_dri_fd, resources->encoders[i]);
        if (!encoder)
            continue;

        if (encoder->encoder_id == connector->encoder_id)
            break;

        drmModeFreeEncoder(encoder);
    }

    for (int j = 0; j < resources->count_crtcs; j++) {
        if ((encoder->possible_crtcs & (1 << j))) {
            crtc = resources->crtcs[j];
            break;
        }
    }

    if (crtc == 0)
        qFatal("No suitable CRTC available");

    m_drm_connector = connector;
    m_drm_encoder = encoder;
    m_drm_mode = connector->modes[0];
    m_drm_crtc = crtc;

    qCDebug(qLcEglfsKmsDebug).noquote() << "Using crtc" << m_drm_crtc
                                        << "with mode" << m_drm_mode.hdisplay << "x" << m_drm_mode.vdisplay
                                        << "@" << m_drm_mode.vrefresh;

    drmModeFreeResources(resources);

    return true;
}

bool QEglFSKmsEglDeviceIntegration::query_egl_device()
{
    m_funcs = new QEGLStreamConvenience;
    if (!m_funcs->has_egl_device_base)
        qFatal("EGL_EXT_device_base missing");

    EGLint num_devices = 0;
    if (m_funcs->query_devices(1, &m_egl_device, &num_devices) != EGL_TRUE) {
        qWarning("eglQueryDevicesEXT failed: eglError: %x", eglGetError());
        return false;
    }

    qCDebug(qLcEglfsKmsDebug, "Found %d EGL devices", num_devices);

    if (num_devices < 1 || m_egl_device == EGL_NO_DEVICE_EXT) {
        qWarning("eglQueryDevicesEXT could not find any EGL devices");
        return false;
    }

    return true;
}

QT_END_NAMESPACE
