// Copyright (C) 2016 Jolla Ltd, author: <giulio.camuffo@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "libhybriseglserverbufferintegration.h"
#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QDebug>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLTexture>
#include <hybris/eglplatformcommon/hybris_nativebufferext.h>

#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

LibHybrisServerBuffer::LibHybrisServerBuffer(LibHybrisEglServerBufferIntegration *integration
                                            , int32_t numFds
                                            , wl_array *ints
                                            , int32_t name
                                            , int32_t width
                                            , int32_t height
                                            , int32_t stride
                                            , int32_t format)
    : QWaylandServerBuffer()
    , m_integration(integration)
    , m_stride(stride)
    , m_hybrisFormat(format)
{
    m_numFds = numFds;
    m_fds.reserve(numFds);
    m_ints.resize(ints->size / sizeof(int32_t));
    memcpy(m_ints.data(), ints->data, ints->size);
    m_image = 0;

    m_size = QSize(width, height);
}

LibHybrisServerBuffer::~LibHybrisServerBuffer()
{
    m_integration->eglDestroyImageKHR(m_image);
}

QOpenGLTexture * LibHybrisServerBuffer::toOpenGlTexture()
{
    if (!QOpenGLContext::currentContext()) {
        qWarning("LibHybrisServerBuffer: creating texture with no current context");
    }

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

void LibHybrisServerBuffer::libhybris_buffer_add_fd(int32_t fd)
{
    m_fds << fd;

    if (m_fds.size() == m_numFds) {
        EGLint egl_format;
        switch (m_hybrisFormat) {
            case QtWayland::qt_libhybris_egl_server_buffer::format_RGBA32:
                m_format = QWaylandServerBuffer::RGBA32;
                egl_format = HYBRIS_PIXEL_FORMAT_RGBA_8888;
                break;
            default:
                qWarning("LibHybrisServerBuffer: unknown format");
                m_format = QWaylandServerBuffer::RGBA32;
                egl_format = HYBRIS_PIXEL_FORMAT_RGBA_8888;
                break;
        }

        EGLClientBuffer buf;
        m_integration->eglHybrisCreateRemoteBuffer(m_size.width(), m_size.height(), HYBRIS_USAGE_HW_TEXTURE, egl_format, m_stride, m_ints.size(), m_ints.data(), m_fds.size(), m_fds.data(), &buf);
        m_image = m_integration->eglCreateImageKHR(EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, buf, 0);
    }
}

void LibHybrisEglServerBufferIntegration::initializeEgl()
{
    if (m_egl_initialized)
        return;
    m_egl_initialized = true;

    m_egl_display = eglGetDisplay((EGLNativeDisplayType) m_display->wl_display());
    if (m_egl_display == EGL_NO_DISPLAY) {
        qWarning("Failed to initialize libhybris egl server buffer integration. Could not get egl display from wl_display.");
        return;
    }

    const char *extensionString = eglQueryString(m_egl_display, EGL_EXTENSIONS);
    if (!extensionString || !strstr(extensionString, "EGL_KHR_image")) {
        qWarning("Failed to initialize libhybris egl server buffer integration. There is no EGL_KHR_image extension.\n");
        return;
    }
    m_egl_create_image = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
    m_egl_destroy_image = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
    if (!m_egl_create_image || !m_egl_destroy_image) {
        qWarning("Failed to initialize libhybris egl server buffer integration. Could not resolve eglCreateImageKHR or eglDestroyImageKHR");
        return;
    }

    m_gl_egl_image_target_texture = reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));
    if (!m_gl_egl_image_target_texture) {
        qWarning("Failed to initialize libhybris egl server buffer integration. Could not resolve glEGLImageTargetTexture2DOES");
        return;
    }

    m_egl_create_buffer = reinterpret_cast<PFNEGLHYBRISCREATEREMOTEBUFFERPROC>(eglGetProcAddress("eglHybrisCreateRemoteBuffer"));
    if (!m_egl_create_buffer) {
        qWarning("Failed to initialize libhybris egl server buffer integration. Could not resolve eglHybrisCreateRemoteBuffer");
        return;
    }
    m_egl_initialized = true;
}

void LibHybrisEglServerBufferIntegration::initialize(QWaylandDisplay *display)
{
    m_display = display;
    display->addRegistryListener(&wlDisplayHandleGlobal, this);
}

QWaylandServerBuffer *LibHybrisEglServerBufferIntegration::serverBuffer(struct qt_server_buffer *buffer)
{
    return static_cast<QWaylandServerBuffer *>(qt_server_buffer_get_user_data(buffer));
}

void LibHybrisEglServerBufferIntegration::wlDisplayHandleGlobal(void *data, ::wl_registry *registry, uint32_t id, const QString &interface, uint32_t version)
{
    Q_UNUSED(version);
    if (interface == QStringLiteral("qt_libhybris_egl_server_buffer")) {
        auto *integration = static_cast<LibHybrisEglServerBufferIntegration *>(data);
        integration->QtWayland::qt_libhybris_egl_server_buffer::init(registry, id, 1);
    }
}

void LibHybrisEglServerBufferIntegration::libhybris_egl_server_buffer_server_buffer_created(struct ::qt_libhybris_buffer *id
                                                                                            , struct ::qt_server_buffer *qid
                                                                                            , int32_t numFds
                                                                                            , wl_array *ints
                                                                                            , int32_t name
                                                                                            , int32_t width
                                                                                            , int32_t height
                                                                                            , int32_t stride
                                                                                            , int32_t format)
{
    LibHybrisServerBuffer *server_buffer = new LibHybrisServerBuffer(this, numFds, ints, name, width, height, stride, format);
    server_buffer->QtWayland::qt_libhybris_buffer::init(id);
    qt_server_buffer_set_user_data(qid, server_buffer);
}

}

QT_END_NAMESPACE
