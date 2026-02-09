// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandbrcmeglwindow.h"

#include <QtWaylandClient/private/qwaylandbuffer_p.h>
#include <QtWaylandClient/private/qwaylandscreen_p.h>
#include "qwaylandbrcmglcontext.h"

#include <QtGui/private/qeglconvenience_p.h>

#include <QtGui/QWindow>
#include <qpa/qwindowsysteminterface.h>

#include <EGL/eglext_brcm.h>

#include "wayland-brcm-client-protocol.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandBrcmBuffer : public QWaylandBuffer
{
public:
    QWaylandBrcmBuffer(QWaylandDisplay *display,
                       struct qt_brcm *brcm,
                       const QSize &size,
                       EGLint *data,
                       int count,
                       struct wl_event_queue *eventQueue)
        : m_size(size)
        , m_display(display)
        , m_eventQueue(eventQueue)
    {
        wl_array_init(&m_array);
        m_data = static_cast<EGLint *>(wl_array_add(&m_array, count * sizeof(EGLint)));

        for (int i = 0; i < count; ++i)
            m_data[i] = data[i];

        mBuffer = qt_brcm_create_buffer(brcm, size.width(), size.height(), &m_array);
        wl_proxy_set_queue(reinterpret_cast<struct wl_proxy*>(mBuffer), m_eventQueue);

        static const struct wl_buffer_listener buffer_listener = {
            QWaylandBrcmBuffer::buffer_release
        };

        wl_buffer_add_listener(mBuffer, &buffer_listener, this);
    }

    ~QWaylandBrcmBuffer()
    {
        wl_array_release(&m_array);
        wl_buffer_destroy(mBuffer);
        mBuffer = nullptr;
    }

    QSize size() const { return m_size; }

    void bind()
    {
        m_released = false;
    }

    void waitForRelease()
    {
        if (m_released)
            return;
        while (!m_released) {
            wl_display_dispatch_queue(m_display->wl_display(), m_eventQueue);
        }
    }

    static void buffer_release(void *data, wl_buffer *buffer)
    {
        Q_UNUSED(buffer);
        static_cast<QWaylandBrcmBuffer *>(data)->m_released = true;
    }

private:

    QSize m_size;
    bool m_released = true;
    wl_array m_array;
    EGLint *m_data = nullptr;
    QWaylandDisplay *m_display = nullptr;
    struct wl_event_queue *m_eventQueue = nullptr;
};

QWaylandBrcmEglWindow::QWaylandBrcmEglWindow(QWindow *window, QWaylandDisplay *display)
    : QWaylandWindow(window, display)
    , m_eglIntegration(static_cast<QWaylandBrcmEglIntegration *>(mDisplay->clientBufferIntegration()))
    , m_format(window->format())
    , m_eventQueue(wl_display_create_queue(mDisplay->wl_display()))
{
}

QWaylandBrcmEglWindow::~QWaylandBrcmEglWindow()
{
    destroyEglSurfaces();
}

QWaylandWindow::WindowType QWaylandBrcmEglWindow::windowType() const
{
    return QWaylandWindow::Egl;
}

void QWaylandBrcmEglWindow::setGeometry(const QRect &rect)
{
    destroyEglSurfaces();
    QWaylandWindow::setGeometry(rect);
}

QSurfaceFormat QWaylandBrcmEglWindow::format() const
{
    return m_format;
}

void QWaylandBrcmEglWindow::destroyEglSurfaces()
{
    for (int i = 0; i < m_count; ++i) {
        if (m_eglSurfaces[i]) {
            eglDestroySurface(m_eglIntegration->eglDisplay(), m_eglSurfaces[i]);
            m_eglSurfaces[i] = 0;
            // the server does this
            //m_eglIntegration->eglDestroyGlobalImageBRCM(&m_globalImages[5*i]);
            delete m_buffers[i];
        }
    }

    m_count = 0;
    m_current = 0;
}

QSurfaceFormat brcmFixFormat(const QSurfaceFormat &f)
{
    QSurfaceFormat format = f;
    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);
    format.setAlphaBufferSize(8);
    return format;
}

void QWaylandBrcmEglWindow::createEglSurfaces()
{
    QSize size(geometry().size());

    m_count = window()->format().swapBehavior() == QSurfaceFormat::TripleBuffer ? 3 : 2;

    EGLConfig eglConfig = q_configFromGLFormat(m_eglIntegration->eglDisplay(), brcmFixFormat(window()->format()), true, EGL_PIXMAP_BIT);

    m_format = q_glFormatFromConfig(m_eglIntegration->eglDisplay(), eglConfig);

    EGLint pixel_format = EGL_PIXEL_FORMAT_ARGB_8888_BRCM;

    EGLint rt;
    eglGetConfigAttrib(m_eglIntegration->eglDisplay(), eglConfig, EGL_RENDERABLE_TYPE, &rt);

    if (rt & EGL_OPENGL_ES_BIT) {
        pixel_format |= EGL_PIXEL_FORMAT_RENDER_GLES_BRCM;
        pixel_format |= EGL_PIXEL_FORMAT_GLES_TEXTURE_BRCM;
    }

    if (rt & EGL_OPENGL_ES2_BIT) {
        pixel_format |= EGL_PIXEL_FORMAT_RENDER_GLES2_BRCM;
        pixel_format |= EGL_PIXEL_FORMAT_GLES2_TEXTURE_BRCM;
    }

    if (rt & EGL_OPENVG_BIT) {
        pixel_format |= EGL_PIXEL_FORMAT_RENDER_VG_BRCM;
        pixel_format |= EGL_PIXEL_FORMAT_VG_IMAGE_BRCM;
    }

    if (rt & EGL_OPENGL_BIT) {
        pixel_format |= EGL_PIXEL_FORMAT_RENDER_GL_BRCM;
    }

    memset(m_globalImages, 0, 5 * m_count * sizeof(EGLint));
    for (int i = 0; i < m_count; ++i) {
        m_eglIntegration->eglCreateGlobalImageBRCM(size.width(), size.height(), pixel_format,
                0, size.width() * 4, &m_globalImages[5*i]);

        m_globalImages[5*i+2] = size.width();
        m_globalImages[5*i+3] = size.height();
        m_globalImages[5*i+4] = pixel_format;

        EGLint attrs[] = {
            EGL_VG_COLORSPACE, EGL_VG_COLORSPACE_sRGB,
            EGL_VG_ALPHA_FORMAT, pixel_format & EGL_PIXEL_FORMAT_ARGB_8888_PRE_BRCM ? EGL_VG_ALPHA_FORMAT_PRE : EGL_VG_ALPHA_FORMAT_NONPRE,
            EGL_NONE
        };

        m_eglSurfaces[i] = eglCreatePixmapSurface(m_eglIntegration->eglDisplay(), eglConfig, (EGLNativePixmapType)&m_globalImages[5*i], attrs);
        if (m_eglSurfaces[i] == EGL_NO_SURFACE)
            qFatal("eglCreatePixmapSurface failed: %x, global image id: %d %d\n", eglGetError(), m_globalImages[5*i], m_globalImages[5*i+1]);
        m_buffers[i] = new QWaylandBrcmBuffer(mDisplay, m_eglIntegration->waylandBrcm(), size, &m_globalImages[5*i], 5, m_eventQueue);
    }
}

void QWaylandBrcmEglWindow::swapBuffers()
{
    if (m_eglIntegration->eglFlushBRCM) {
        m_eglIntegration->eglFlushBRCM();
    } else {
        glFlush();
        glFinish();
    }

    if (!m_count)
        return;

    m_buffers[m_current]->bind();
    commit(m_buffers[m_current], QRegion(0, 0, geometry().size().width(), geometry().size().height()));

    m_current = (m_current + 1) % m_count;
    m_buffers[m_current]->waitForRelease();
}

bool QWaylandBrcmEglWindow::makeCurrent(EGLContext context)
{
    if (!m_count)
        const_cast<QWaylandBrcmEglWindow *>(this)->createEglSurfaces();
    return eglMakeCurrent(m_eglIntegration->eglDisplay(), m_eglSurfaces[m_current], m_eglSurfaces[m_current], context);
}

}

QT_END_NAMESPACE
