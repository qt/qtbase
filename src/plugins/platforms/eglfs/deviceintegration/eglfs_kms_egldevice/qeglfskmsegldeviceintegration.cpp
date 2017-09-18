/****************************************************************************
**
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

#include "qeglfskmsegldeviceintegration.h"
#include "qeglfskmsegldevice.h"
#include "qeglfskmsegldevicescreen.h"
#include <QtEglSupport/private/qeglconvenience_p.h>
#include "private/qeglfswindow_p.h"
#include "private/qeglfscursor_p.h"
#include <QLoggingCategory>
#include <private/qmath_p.h>

QT_BEGIN_NAMESPACE

QEglFSKmsEglDeviceIntegration::QEglFSKmsEglDeviceIntegration()
    : m_egl_device(EGL_NO_DEVICE_EXT)
    , m_funcs(nullptr)
{
    qCDebug(qLcEglfsKmsDebug, "New DRM/KMS on EGLDevice integration created");
}

QSurfaceFormat QEglFSKmsEglDeviceIntegration::surfaceFormatFor(const QSurfaceFormat &inputFormat) const
{
    QSurfaceFormat format = QEglFSKmsIntegration::surfaceFormatFor(inputFormat);
    format.setAlphaBufferSize(8);
    return format;
}

EGLint QEglFSKmsEglDeviceIntegration::surfaceType() const
{
    return EGL_STREAM_BIT_KHR;
}

EGLDisplay QEglFSKmsEglDeviceIntegration::createDisplay(EGLNativeDisplayType nativeDisplay)
{
    qCDebug(qLcEglfsKmsDebug, "Creating display");

    EGLDisplay display;

    if (m_funcs->has_egl_platform_device) {
        display = m_funcs->get_platform_display(EGL_PLATFORM_DEVICE_EXT, nativeDisplay, nullptr);
    } else {
        qWarning("EGL_EXT_platform_device not available, falling back to legacy path!");
        display = eglGetDisplay(nativeDisplay);
    }

    if (Q_UNLIKELY(display == EGL_NO_DISPLAY))
        qFatal("Could not get EGL display");

    EGLint major, minor;
    if (Q_UNLIKELY(!eglInitialize(display, &major, &minor)))
        qFatal("Could not initialize egl display");

    if (Q_UNLIKELY(!eglBindAPI(EGL_OPENGL_ES_API)))
        qFatal("Failed to bind EGL_OPENGL_ES_API\n");

    return display;
}

bool QEglFSKmsEglDeviceIntegration::supportsSurfacelessContexts() const
{
    // Returning false disables the usage of EGL_KHR_surfaceless_context even when the
    // extension is available. This is just what we need since, at least with NVIDIA
    // 352.00 making a null surface current with a context breaks.
    return false;
}

bool QEglFSKmsEglDeviceIntegration::supportsPBuffers() const
{
    return true;
}

class QEglFSKmsEglDeviceWindow : public QEglFSWindow
{
public:
    QEglFSKmsEglDeviceWindow(QWindow *w, const QEglFSKmsEglDeviceIntegration *integration)
        : QEglFSWindow(w)
        , m_integration(integration)
        , m_egl_stream(EGL_NO_STREAM_KHR)
    { }

    void invalidateSurface() override;
    void resetSurface() override;

    const QEglFSKmsEglDeviceIntegration *m_integration;
    EGLStreamKHR m_egl_stream;
    EGLint m_latency;
};

void QEglFSKmsEglDeviceWindow::invalidateSurface()
{
    QEglFSWindow::invalidateSurface();
    m_integration->m_funcs->destroy_stream(screen()->display(), m_egl_stream);
}

void QEglFSKmsEglDeviceWindow::resetSurface()
{
    qCDebug(qLcEglfsKmsDebug, "Creating stream");

    EGLDisplay display = screen()->display();
    EGLint streamAttribs[3];
    int streamAttribCount = 0;
    int fifoLength = qEnvironmentVariableIntValue("QT_QPA_EGLFS_STREAM_FIFO_LENGTH");
    if (fifoLength > 0) {
        streamAttribs[streamAttribCount++] = EGL_STREAM_FIFO_LENGTH_KHR;
        streamAttribs[streamAttribCount++] = fifoLength;
    }
    streamAttribs[streamAttribCount++] = EGL_NONE;

    m_egl_stream = m_integration->m_funcs->create_stream(display, streamAttribs);
    if (m_egl_stream == EGL_NO_STREAM_KHR) {
        qWarning("resetSurface: Couldn't create EGLStream for native window");
        return;
    }

    qCDebug(qLcEglfsKmsDebug, "Created stream %p on display %p", m_egl_stream, display);

    EGLint count;
    if (m_integration->m_funcs->query_stream(display, m_egl_stream, EGL_STREAM_FIFO_LENGTH_KHR, &count)) {
        if (count > 0)
            qCDebug(qLcEglfsKmsDebug, "Using EGLStream FIFO mode with %d frames", count);
        else
            qCDebug(qLcEglfsKmsDebug, "Using EGLStream mailbox mode");
    } else {
        qCDebug(qLcEglfsKmsDebug, "Could not query number of EGLStream FIFO frames");
    }

    if (!m_integration->m_funcs->get_output_layers(display, nullptr, nullptr, 0, &count) || count == 0) {
        qWarning("No output layers found");
        return;
    }

    qCDebug(qLcEglfsKmsDebug, "Output has %d layers", count);

    QVector<EGLOutputLayerEXT> layers;
    layers.resize(count);
    EGLint actualCount;
    if (!m_integration->m_funcs->get_output_layers(display, nullptr, layers.data(), count, &actualCount)) {
        qWarning("Failed to get layers");
        return;
    }

    QEglFSKmsEglDeviceScreen *cur_screen = static_cast<QEglFSKmsEglDeviceScreen *>(screen());
    Q_ASSERT(cur_screen);
    QKmsOutput &output(cur_screen->output());
    const uint32_t wantedId = !output.wants_forced_plane ? output.crtc_id : output.forced_plane_id;
    qCDebug(qLcEglfsKmsDebug, "Searching for id: %d", wantedId);

    EGLOutputLayerEXT layer = EGL_NO_OUTPUT_LAYER_EXT;
    for (int i = 0; i < actualCount; ++i) {
        EGLAttrib id;
        if (m_integration->m_funcs->query_output_layer_attrib(display, layers[i], EGL_DRM_CRTC_EXT, &id)) {
            qCDebug(qLcEglfsKmsDebug, "  [%d] layer %p - crtc %d", i, layers[i], (int) id);
            if (id == EGLAttrib(wantedId))
                layer = layers[i];
        } else if (m_integration->m_funcs->query_output_layer_attrib(display, layers[i], EGL_DRM_PLANE_EXT, &id)) {
            qCDebug(qLcEglfsKmsDebug, "  [%d] layer %p - plane %d", i, layers[i], (int) id);
            if (id == EGLAttrib(wantedId))
                layer = layers[i];
        } else {
            qCDebug(qLcEglfsKmsDebug, "  [%d] layer %p - unknown", i, layers[i]);
        }
    }

    QByteArray reqLayerIndex = qgetenv("QT_QPA_EGLFS_LAYER_INDEX");
    if (!reqLayerIndex.isEmpty()) {
        int idx = reqLayerIndex.toInt();
        if (idx >= 0 && idx < layers.count()) {
            qCDebug(qLcEglfsKmsDebug, "EGLOutput layer index override = %d", idx);
            layer = layers[idx];
        }
    }

    if (layer == EGL_NO_OUTPUT_LAYER_EXT) {
        qWarning("resetSurface: Couldn't get EGLOutputLayer for native window");
        return;
    }

    qCDebug(qLcEglfsKmsDebug, "Using layer %p", layer);

    if (!m_integration->m_funcs->stream_consumer_output(display, m_egl_stream, layer))
        qWarning("resetSurface: Unable to connect stream");

    m_config = QEglFSDeviceIntegration::chooseConfig(display, m_integration->surfaceFormatFor(window()->requestedFormat()));
    m_format = q_glFormatFromConfig(display, m_config);
    qCDebug(qLcEglfsKmsDebug) << "Stream producer format is" << m_format;

    const int w = cur_screen->rawGeometry().width();
    const int h = cur_screen->rawGeometry().height();
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
    QEglFSKmsEglDeviceWindow *eglWindow = new QEglFSKmsEglDeviceWindow(window, this);

    m_funcs->initialize(eglWindow->screen()->display());
    if (Q_UNLIKELY(!(m_funcs->has_egl_output_base && m_funcs->has_egl_output_drm && m_funcs->has_egl_stream &&
                     m_funcs->has_egl_stream_producer_eglsurface && m_funcs->has_egl_stream_consumer_egloutput)))
        qFatal("Required extensions missing!");

    return eglWindow;
}

QKmsDevice *QEglFSKmsEglDeviceIntegration::createDevice()
{
    if (Q_UNLIKELY(!query_egl_device()))
        qFatal("Could not set up EGL device!");

    const char *deviceName = m_funcs->query_device_string(m_egl_device, EGL_DRM_DEVICE_FILE_EXT);
    if (Q_UNLIKELY(!deviceName))
        qFatal("Failed to query device name from EGLDevice");

    return new QEglFSKmsEglDevice(this, screenConfig(), deviceName);
}

bool QEglFSKmsEglDeviceIntegration::query_egl_device()
{
    m_funcs = new QEGLStreamConvenience;
    if (Q_UNLIKELY(!m_funcs->has_egl_device_base))
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

QPlatformCursor *QEglFSKmsEglDeviceIntegration::createCursor(QPlatformScreen *screen) const
{
#if QT_CONFIG(opengl)
    if (screenConfig()->separateScreens())
        return  new QEglFSCursor(screen);
#endif
    return nullptr;
}

QT_END_NAMESPACE
