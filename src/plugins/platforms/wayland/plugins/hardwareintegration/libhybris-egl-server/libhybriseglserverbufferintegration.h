// Copyright (C) 2016 Jolla Ltd, author: <giulio.camuffo@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef LIBHYBRISEGLSERVERBUFFERINTEGRATION_H
#define LIBHYBRISEGLSERVERBUFFERINTEGRATION_H

#include <QtWaylandClient/private/qwayland-wayland.h>
#include "qwayland-libhybris-egl-server-buffer.h"
#include <QtWaylandClient/private/qwaylandserverbufferintegration_p.h>
#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtCore/QList>
#include <QtCore/QTextStream>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#ifndef EGL_KHR_image
typedef void *EGLImageKHR;
typedef EGLImageKHR (EGLAPIENTRYP PFNEGLCREATEIMAGEKHRPROC) (EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLDESTROYIMAGEKHRPROC) (EGLDisplay dpy, EGLImageKHR image);
#endif

#ifndef GL_OES_EGL_image
typedef void (GL_APIENTRYP PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) (GLenum target, GLeglImageOES image);
#endif

#ifndef EGL_HYBRIS_native_buffer
typedef EGLBoolean (EGLAPIENTRYP PFNEGLHYBRISCREATEREMOTEBUFFERPROC)(EGLint width, EGLint height, EGLint usage, EGLint format, EGLint stride,
                                                                     int num_ints, int *ints, int num_fds, int *fds, EGLClientBuffer *buffer);
#endif

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class LibHybrisEglServerBufferIntegration;

class LibHybrisServerBuffer : public QWaylandServerBuffer, public QtWayland::qt_libhybris_buffer
{
public:
    LibHybrisServerBuffer(LibHybrisEglServerBufferIntegration *integration, int32_t numFds, wl_array *ints, int32_t name, int32_t width, int32_t height, int32_t stride, int32_t format);
    ~LibHybrisServerBuffer();
    QOpenGLTexture* toOpenGlTexture() override;

protected:
    void libhybris_buffer_add_fd(int32_t fd) override;

private:
    LibHybrisEglServerBufferIntegration *m_integration = nullptr;
    EGLImageKHR m_image;
    QOpenGLTexture *m_texture = nullptr;
    int m_numFds;
    QList<int32_t> m_ints;
    QList<int32_t> m_fds;
    int32_t m_stride;
    int32_t m_hybrisFormat;
};

class LibHybrisEglServerBufferIntegration
    : public QWaylandServerBufferIntegration
    , public QtWayland::qt_libhybris_egl_server_buffer
{
public:
    void initialize(QWaylandDisplay *display) override;

    virtual QWaylandServerBuffer *serverBuffer(struct qt_server_buffer *buffer) override;

    inline EGLImageKHR eglCreateImageKHR(EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list);
    inline EGLBoolean eglDestroyImageKHR (EGLImageKHR image);
    inline void glEGLImageTargetTexture2DOES (GLenum target, GLeglImageOES image);
    inline EGLBoolean eglHybrisCreateRemoteBuffer(EGLint width, EGLint height, EGLint usage, EGLint format, EGLint stride, int num_ints, int *ints, int num_fds, int *fds, EGLClientBuffer *buffer);

protected:
    void libhybris_egl_server_buffer_server_buffer_created(struct ::qt_libhybris_buffer *id, struct ::qt_server_buffer *qid,
                                                           int32_t numFds, wl_array *ints, int32_t name, int32_t width, int32_t height, int32_t stride, int32_t format) override;
private:
    static void wlDisplayHandleGlobal(void *data, struct ::wl_registry *registry, uint32_t id,
                                      const QString &interface, uint32_t version);
    void initializeEgl();

    PFNEGLCREATEIMAGEKHRPROC m_egl_create_image;
    PFNEGLDESTROYIMAGEKHRPROC m_egl_destroy_image;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC m_gl_egl_image_target_texture;
    PFNEGLHYBRISCREATEREMOTEBUFFERPROC m_egl_create_buffer;
    QWaylandDisplay *m_display = nullptr;
    EGLDisplay m_egl_display = EGL_NO_DISPLAY;
    bool m_egl_initialized = false;
};

EGLImageKHR LibHybrisEglServerBufferIntegration::eglCreateImageKHR(EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list)
{
    if (!m_egl_initialized)
        initializeEgl();

    if (!m_egl_create_image) {
        qWarning("LibHybrisEglServerBufferIntegration: Trying to used unresolved function eglCreateImageKHR");
        return EGL_NO_IMAGE_KHR;
    }
    return m_egl_create_image(m_egl_display, ctx, target, buffer,attrib_list);
}

EGLBoolean LibHybrisEglServerBufferIntegration::eglDestroyImageKHR (EGLImageKHR image)
{
    if (!m_egl_destroy_image) {
        qWarning("LibHybrisEglServerBufferIntegration: Trying to use unresolved function eglDestroyImageKHR");
        return false;
    }
    return m_egl_destroy_image(m_egl_display, image);
}

void LibHybrisEglServerBufferIntegration::glEGLImageTargetTexture2DOES (GLenum target, GLeglImageOES image)
{
    if (!m_gl_egl_image_target_texture) {
        qWarning("LibHybrisEglServerBufferIntegration: Trying to use unresolved function glEGLImageTargetRenderbufferStorageOES");
        return;
    }
    m_gl_egl_image_target_texture(target,image);
}

EGLBoolean LibHybrisEglServerBufferIntegration::eglHybrisCreateRemoteBuffer(EGLint width, EGLint height, EGLint usage, EGLint format, EGLint stride,
                                                                            int num_ints, int *ints, int num_fds, int *fds, EGLClientBuffer *buffer)
{
    if (!m_egl_create_buffer) {
        qWarning("LibHybrisEglServerBufferIntegration: Trying to use unresolved function eglHybrisCreateRemoteBuffer");
        return false;
    }
    return m_egl_create_buffer(width, height, usage, format, stride, num_ints, ints, num_fds, fds, buffer);
}

}

QT_END_NAMESPACE

#endif
