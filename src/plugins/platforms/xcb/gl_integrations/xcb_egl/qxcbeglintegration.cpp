// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qxcbeglintegration.h"

#include "qxcbeglcontext.h"

#include <QtGui/QOffscreenSurface>
#include <QtGui/private/qeglstreamconvenience_p.h>

#include "qxcbeglnativeinterfacehandler.h"

QT_BEGIN_NAMESPACE

QXcbEglIntegration::QXcbEglIntegration()
    : m_connection(nullptr)
    , m_egl_display(EGL_NO_DISPLAY)
{
    qCDebug(lcQpaGl) << "Xcb EGL gl-integration created";
}

QXcbEglIntegration::~QXcbEglIntegration()
{
    if (m_egl_display != EGL_NO_DISPLAY)
        eglTerminate(m_egl_display);
}

bool QXcbEglIntegration::initialize(QXcbConnection *connection)
{
    m_connection = connection;

    const char *extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);

    if (extensions && strstr(extensions, "EGL_EXT_platform_x11")) {
        QEGLStreamConvenience streamFuncs;
        m_egl_display = streamFuncs.get_platform_display(EGL_PLATFORM_X11_KHR,
                                                         xlib_display(),
                                                         nullptr);
    }

    if (!m_egl_display)
        m_egl_display = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(xlib_display()));

    EGLint major, minor;
    bool success = eglInitialize(m_egl_display, &major, &minor);
    if (!success) {
        m_egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        qCDebug(lcQpaGl) << "Xcb EGL gl-integration retrying with display" << m_egl_display;
        success = eglInitialize(m_egl_display, &major, &minor);
    }

    m_native_interface_handler.reset(new QXcbEglNativeInterfaceHandler(connection->nativeInterface()));

    if (success)
        qCDebug(lcQpaGl) << "Xcb EGL gl-integration successfully initialized";
    else
        qCWarning(lcQpaGl) << "Xcb EGL gl-integration initialize failed";

    return success;
}

QXcbWindow *QXcbEglIntegration::createWindow(QWindow *window) const
{
    return new QXcbEglWindow(window, const_cast<QXcbEglIntegration *>(this));
}

QPlatformOpenGLContext *QXcbEglIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QXcbScreen *screen = static_cast<QXcbScreen *>(context->screen()->handle());
    QXcbEglContext *platformContext = new QXcbEglContext(screen->surfaceFormatFor(context->format()),
                                                         context->shareHandle(),
                                                         eglDisplay());
    return platformContext;
}

QOpenGLContext *QXcbEglIntegration::createOpenGLContext(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext) const
{
    return QEGLPlatformContext::createFrom<QXcbEglContext>(context, display, eglDisplay(), shareContext);
}

QPlatformOffscreenSurface *QXcbEglIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    QXcbScreen *screen = static_cast<QXcbScreen *>(surface->screen()->handle());
    return new QEGLPbuffer(eglDisplay(), screen->surfaceFormatFor(surface->requestedFormat()), surface);
}

void *QXcbEglIntegration::xlib_display() const
{
#if QT_CONFIG(xcb_xlib)
    return m_connection->xlib_display();
#else
    return EGL_DEFAULT_DISPLAY;
#endif
}

QT_END_NAMESPACE
