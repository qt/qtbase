// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "drmeglserverbufferintegration.h"
#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QDebug>
#include <QtOpenGL/QOpenGLTexture>
#include <QtGui/QOpenGLContext>

#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

DrmServerBuffer::DrmServerBuffer(DrmEglServerBufferIntegration *integration
                                , int32_t name
                                , int32_t width
                                , int32_t height
                                , int32_t stride
                                , int32_t format)
    : m_integration(integration)
{
    m_size = QSize(width, height);
    EGLint egl_format;
    int32_t format_stride;
    switch (format) {
        case QtWayland::qt_drm_egl_server_buffer::format_RGBA32:
            m_format = QWaylandServerBuffer::RGBA32;
            egl_format = EGL_DRM_BUFFER_FORMAT_ARGB32_MESA;
            format_stride = stride / 4;
            break;
#ifdef EGL_DRM_BUFFER_FORMAT_A8_MESA
        case QtWayland::qt_drm_egl_server_buffer::format_A8:
            m_format = QWaylandServerBuffer::A8;
            egl_format = EGL_DRM_BUFFER_FORMAT_A8_MESA;
            format_stride = stride;
            break;
#endif
        default:
            qWarning("DrmServerBuffer: unknown format");
            m_format = QWaylandServerBuffer::RGBA32;
            egl_format = EGL_DRM_BUFFER_FORMAT_ARGB32_MESA;
            format_stride = stride / 4;
            break;
    }
    EGLint attribs[] = {
        EGL_WIDTH,                      width,
        EGL_HEIGHT,                     height,
        EGL_DRM_BUFFER_STRIDE_MESA,     format_stride,
        EGL_DRM_BUFFER_FORMAT_MESA,     egl_format,
        EGL_NONE
    };

    qintptr name_pointer = name;
    m_image = integration->eglCreateImageKHR(EGL_NO_CONTEXT, EGL_DRM_BUFFER_MESA, (EGLClientBuffer) name_pointer, attribs);

}

DrmServerBuffer::~DrmServerBuffer()
{
    m_integration->eglDestroyImageKHR(m_image);
}

QOpenGLTexture *DrmServerBuffer::toOpenGlTexture()
{
    if (!QOpenGLContext::currentContext())
        qWarning("DrmServerBuffer: creating texture with no current context");

    if (!m_texture) {
        m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        m_texture->create();
    }

    m_texture->bind();
    m_integration->glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_texture->release();
    m_texture->setSize(m_size.width(), m_size.height());
    return m_texture;
}

void DrmEglServerBufferIntegration::initializeEgl()
{
    if (m_egl_initialized)
        return;
    m_egl_initialized = true;

#if QT_CONFIG(egl_extension_platform_wayland)
    m_egl_display = eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_EXT, m_display->wl_display(), nullptr);
#else
    m_egl_display = eglGetDisplay((EGLNativeDisplayType) m_display->wl_display());
#endif
    if (m_egl_display == EGL_NO_DISPLAY) {
        qWarning("Failed to initialize drm egl server buffer integration. Could not get egl display from wl_display.");
        return;
    }

    const char *extensionString = eglQueryString(m_egl_display, EGL_EXTENSIONS);
    if (!extensionString || !strstr(extensionString, "EGL_KHR_image")) {
        qWarning("Failed to initialize drm egl server buffer integration. There is no EGL_KHR_image extension.\n");
        return;
    }
    m_egl_create_image = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
    m_egl_destroy_image = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
    if (!m_egl_create_image || !m_egl_destroy_image) {
        qWarning("Failed to initialize drm egl server buffer integration. Could not resolve eglCreateImageKHR or eglDestroyImageKHR");
        return;
    }

    m_gl_egl_image_target_texture = reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));
    if (!m_gl_egl_image_target_texture) {
        qWarning("Failed to initialize drm egl server buffer integration. Could not resolve glEGLImageTargetTexture2DOES");
        return;
    }
    m_egl_initialized = true;
}

void DrmEglServerBufferIntegration::initialize(QWaylandDisplay *display)
{
    m_display = display;
    display->addRegistryListener(&wlDisplayHandleGlobal, this);
}

QWaylandServerBuffer *DrmEglServerBufferIntegration::serverBuffer(struct qt_server_buffer *buffer)
{
    return static_cast<QWaylandServerBuffer *>(qt_server_buffer_get_user_data(buffer));
}

void DrmEglServerBufferIntegration::wlDisplayHandleGlobal(void *data, ::wl_registry *registry, uint32_t id, const QString &interface, uint32_t version)
{
    Q_UNUSED(version);
    if (interface == QStringLiteral("qt_drm_egl_server_buffer")) {
        auto *integration = static_cast<DrmEglServerBufferIntegration *>(data);
        integration->QtWayland::qt_drm_egl_server_buffer::init(registry, id, 1);
    }
}

void DrmEglServerBufferIntegration::drm_egl_server_buffer_server_buffer_created(struct ::qt_server_buffer *id
                                                                               , int32_t name
                                                                               , int32_t width
                                                                               , int32_t height
                                                                               , int32_t stride
                                                                               , int32_t format)
{
    DrmServerBuffer *server_buffer = new DrmServerBuffer(this, name, width, height, stride, format);
    qt_server_buffer_set_user_data(id, server_buffer);
}

}

QT_END_NAMESPACE
