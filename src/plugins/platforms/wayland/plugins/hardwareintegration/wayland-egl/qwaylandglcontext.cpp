// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandglcontext_p.h"

#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>
#include <QtWaylandClient/private/qwaylandsubsurface_p.h>
#include <QtWaylandClient/private/qwaylandabstractdecoration_p.h>
#include <QtWaylandClient/private/qwaylandintegration_p.h>
#include "qwaylandeglwindow_p.h"

#include <QDebug>
#include <QtGui/private/qeglconvenience_p.h>
#include <QtGui/private/qopenglcontext_p.h>
#include <QtOpenGL/private/qopengltexturecache_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qeglpbuffer_p.h>


#include <qpa/qplatformopenglcontext.h>
#include <QtGui/QSurfaceFormat>
#include <QtOpenGL/QOpenGLShaderProgram>
#include <QtGui/QOpenGLFunctions>
#include <QOpenGLBuffer>

#include <QtCore/qmutex.h>

#include <dlfcn.h>

// Constants from EGL_KHR_create_context
#ifndef EGL_CONTEXT_MINOR_VERSION_KHR
#define EGL_CONTEXT_MINOR_VERSION_KHR 0x30FB
#endif
#ifndef EGL_CONTEXT_FLAGS_KHR
#define EGL_CONTEXT_FLAGS_KHR 0x30FC
#endif
#ifndef EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR
#define EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR 0x30FD
#endif
#ifndef EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR
#define EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR 0x00000001
#endif
#ifndef EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR
#define EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR 0x00000002
#endif
#ifndef EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR
#define EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR 0x00000001
#endif
#ifndef EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR
#define EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR 0x00000002
#endif
#ifndef EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_KHR
#define EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_KHR 0x31BD
#endif
#ifndef EGL_LOSE_CONTEXT_ON_RESET_KHR
#define EGL_LOSE_CONTEXT_ON_RESET_KHR 0x31BF
#endif
#ifndef EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR
#define EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR 0x00000004
#endif

// Constants from EGL_EXT_create_context_robustness
#ifndef EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT
#define EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT 0x30BF
#endif
#ifndef EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT
#define EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT 0x3138
#endif
#ifndef EGL_LOSE_CONTEXT_ON_RESET_EXT
#define EGL_LOSE_CONTEXT_ON_RESET_EXT 0x31BF
#endif

// Constants for OpenGL which are not available in the ES headers.
#ifndef GL_CONTEXT_FLAGS
#define GL_CONTEXT_FLAGS 0x821E
#endif
#ifndef GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT
#define GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT 0x0001
#endif
#ifndef GL_CONTEXT_FLAG_DEBUG_BIT
#define GL_CONTEXT_FLAG_DEBUG_BIT 0x00000002
#endif
#ifndef GL_CONTEXT_PROFILE_MASK
#define GL_CONTEXT_PROFILE_MASK 0x9126
#endif
#ifndef GL_CONTEXT_CORE_PROFILE_BIT
#define GL_CONTEXT_CORE_PROFILE_BIT 0x00000001
#endif
#ifndef GL_CONTEXT_COMPATIBILITY_PROFILE_BIT
#define GL_CONTEXT_COMPATIBILITY_PROFILE_BIT 0x00000002
#endif

// Constants from EGL_NV_robustness_video_memory_purge
#ifndef EGL_GENERATE_RESET_ON_VIDEO_MEMORY_PURGE_NV
#define EGL_GENERATE_RESET_ON_VIDEO_MEMORY_PURGE_NV 0x334C
#endif

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandGLContext::QWaylandGLContext()
    : QEGLPlatformContext(),
      m_api(EGL_OPENGL_ES_API)
{
}

QWaylandGLContext::QWaylandGLContext(EGLDisplay eglDisplay, QWaylandDisplay *display,
                                     const QSurfaceFormat &fmt, QPlatformOpenGLContext *share)
    : QEGLPlatformContext(fmt, share, eglDisplay)
    , m_display(display)
{
    m_reconnectionWatcher = QObject::connect(m_display, &QWaylandDisplay::connected,
                                             m_display, [this] { invalidateContext(); });

    switch (format().renderableType()) {
    case QSurfaceFormat::OpenVG:
        m_api = EGL_OPENVG_API;
        break;
#ifdef EGL_VERSION_1_4
    case QSurfaceFormat::OpenGL:
        m_api = EGL_OPENGL_API;
        break;
#endif // EGL_VERSION_1_4
    default:
        m_api = EGL_OPENGL_ES_API;
        break;
    }

    EGLint a = EGL_MIN_SWAP_INTERVAL;
    EGLint b = EGL_MAX_SWAP_INTERVAL;
    if (!eglGetConfigAttrib(eglDisplay, eglConfig(), a, &a)
        || !eglGetConfigAttrib(eglDisplay, eglConfig(), b, &b) || a > 0) {
        m_supportNonBlockingSwap = false;
    }
    {
        bool ok;
        int supportNonBlockingSwap = qEnvironmentVariableIntValue("QT_WAYLAND_FORCE_NONBLOCKING_SWAP_SUPPORT", &ok);
        if (ok)
            m_supportNonBlockingSwap = supportNonBlockingSwap != 0;
    }
    if (!m_supportNonBlockingSwap) {
        qCWarning(lcQpaWayland) << "Non-blocking swap buffers not supported."
                               << "Subsurface rendering can be affected."
                               << "It may also cause the event loop to freeze in some situations";
    }
}

EGLSurface QWaylandGLContext::createTemporaryOffscreenSurface()
{
    m_wlSurface = m_display->createSurface(nullptr);
    m_eglWindow = wl_egl_window_create(m_wlSurface, 1, 1);
#if QT_CONFIG(egl_extension_platform_wayland)
    EGLSurface eglSurface =
            eglCreatePlatformWindowSurface(eglDisplay(), eglConfig(), m_eglWindow, nullptr);
#else
    EGLSurface eglSurface = eglCreateWindowSurface(eglDisplay(), eglConfig(), m_eglWindow, nullptr);
#endif
    return eglSurface;
}

void QWaylandGLContext::destroyTemporaryOffscreenSurface(EGLSurface eglSurface)
{
    eglDestroySurface(eglDisplay(), eglSurface);
    wl_egl_window_destroy(m_eglWindow);
    m_eglWindow = nullptr;
    wl_surface_destroy(m_wlSurface);
    m_wlSurface = nullptr;
}

void QWaylandGLContext::runGLChecks()
{
    bool ok;
    const int doneCurrentWorkAround = qEnvironmentVariableIntValue("QT_WAYLAND_ENABLE_DONECURRENT_WORKAROUND", &ok);
    if (ok) {
        m_doneCurrentWorkAround = doneCurrentWorkAround != 0;
        if (m_doneCurrentWorkAround)
            qCDebug(lcQpaWayland) << "Enabling doneCurrent() workaround on request.";
        else
            qCDebug(lcQpaWayland) << "Disabling doneCurrent() workaround on request.";

    } else {
        // Note that even though there is an EGL context current here,
        // QOpenGLContext and QOpenGLFunctions are not yet usable at this stage.
        const char *renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
        if (renderer && strstr(renderer, "Mali")) {
            qCDebug(lcQpaWayland) << "Enabling doneCurrent() workaround for Mali GPU."
                                  << "Set QT_WAYLAND_ENABLE_DONECURRENT_WORKAROUND=0 to disable.";
            m_doneCurrentWorkAround = true;
        }
    }

    QEGLPlatformContext::runGLChecks();
}

QWaylandGLContext::~QWaylandGLContext()
{
    QObject::disconnect(m_reconnectionWatcher);
}

void QWaylandGLContext::beginFrame()
{
    if (!m_currentWindow)
        return;

    if (m_supportNonBlockingSwap)
        m_currentWindow->beginFrame();
}

void QWaylandGLContext::endFrame()
{
    if (!m_currentWindow)
        return;

    QWaylandEglWindow *currentWindow = m_currentWindow;

    if (m_doneCurrentWorkAround) {
        doneCurrent();
        QOpenGLContextPrivate::setCurrentContext(nullptr);
    }

    if (m_supportNonBlockingSwap)
        currentWindow->endFrame();
}

bool QWaylandGLContext::makeCurrent(QPlatformSurface *surface)
{
    if (!isValid()) {
        return false;
    }

    if (surface->surface()->surfaceClass() == QSurface::Offscreen) {
        m_currentWindow = nullptr;
        return QEGLPlatformContext::makeCurrent(surface);
    }

    // in QWaylandGLContext() we called eglBindAPI with the correct value. However,
    // eglBindAPI's documentation says:
    // "eglBindAPI defines the current rendering API for EGL in the thread it is called from"
    // Since makeCurrent() can be called from a different thread than the one we created the
    // context in make sure to call eglBindAPI in the correct thread.
    if (eglQueryAPI() != m_api) {
        eglBindAPI(m_api);
    }

    m_currentWindow = static_cast<QWaylandEglWindow *>(surface);

    QMutexLocker lock(m_currentWindow->eglSurfaceLock());

    EGLSurface eglSurface = m_currentWindow->eglSurface();
    QSize size = m_currentWindow->bufferSize();

    if (eglSurface == EGL_NO_SURFACE) {
        m_currentWindow->createSurface(size);
        eglSurface = m_currentWindow->eglSurface();
    }

    if (!eglMakeCurrent(eglDisplay(), eglSurface, eglSurface, eglContext())) {
        qWarning("QWaylandGLContext::makeCurrent: eglError: %#x, this: %p \n", eglGetError(), this);
        return false;
    }
    if (!checkGraphicsReset())
        return false;

    // Nvidia requires that the surface is current and on the render thread at the time of doing the actual resizing
    m_currentWindow->updateSurfaceSize(size);
    return true;
}

void QWaylandGLContext::doneCurrent()
{
    m_currentWindow = nullptr;
    eglMakeCurrent(eglDisplay(), EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void QWaylandGLContext::swapBuffers(QPlatformSurface *surface)
{
    if (surface->surface()->surfaceClass() == QSurface::Offscreen) {
        return QEGLPlatformContext::swapBuffers(surface);
    }

    QWaylandEglWindow *window = static_cast<QWaylandEglWindow *>(surface);

    EGLSurface eglSurface = window->eglSurface();

    int swapInterval = m_supportNonBlockingSwap ? 0 : format().swapInterval();
    eglSwapInterval(eglDisplay(), swapInterval);
    if (swapInterval == 0 && format().swapInterval() > 0) {
        // Emulating a blocking swap
        glFlush(); // Flush before waiting so we can swap more quickly when the frame event arrives
        window->waitForFrameSync(100);
    }
    window->handleUpdate();
    if (!eglSwapBuffers(eglDisplay(), eglSurface))
        qCWarning(lcQpaWayland, "eglSwapBuffers failed with %#x, surface: %p", eglGetError(), eglSurface);
}

EGLSurface QWaylandGLContext::eglSurfaceForPlatformSurface(QPlatformSurface *surface)
{
    if (surface->surface()->surfaceClass() == QSurface::Window)
        return static_cast<QWaylandEglWindow *>(surface)->eglSurface();
    else
        return static_cast<QEGLPbuffer *>(surface)->pbuffer();
}

}

QT_END_NAMESPACE
