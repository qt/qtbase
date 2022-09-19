// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qminimaleglscreen.h"
#include "qminimaleglwindow.h"

#include <QtGui/private/qeglconvenience_p.h>
#ifndef QT_NO_OPENGL
# include <QtGui/private/qeglplatformcontext_p.h>
#endif

#ifdef Q_OPENKODE
#include <KD/kd.h>
#include <KD/NV_initialize.h>
#endif //Q_OPENKODE

QT_BEGIN_NAMESPACE

// #define QEGL_EXTRA_DEBUG

#ifndef QT_NO_OPENGL

class QMinimalEglContext : public QEGLPlatformContext
{
public:
    QMinimalEglContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display)
        : QEGLPlatformContext(format, share, display)
    {
    }

    EGLSurface eglSurfaceForPlatformSurface(QPlatformSurface *surface) override
    {
        QMinimalEglWindow *window = static_cast<QMinimalEglWindow *>(surface);
        QMinimalEglScreen *screen = static_cast<QMinimalEglScreen *>(window->screen());
        return screen->surface();
    }
};

#endif

QMinimalEglScreen::QMinimalEglScreen(EGLNativeDisplayType display)
    : m_depth(32)
    , m_format(QImage::Format_Invalid)
    , m_platformContext(nullptr)
    , m_surface(nullptr)
{
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglScreen %p\n", this);
#endif

    EGLint major, minor;

    if (Q_UNLIKELY(!eglBindAPI(EGL_OPENGL_ES_API))) {
        qWarning("Could not bind GL_ES API\n");
        qFatal("EGL error");
    }

    m_dpy = eglGetDisplay(display);
    if (Q_UNLIKELY(m_dpy == EGL_NO_DISPLAY)) {
        qWarning("Could not open egl display\n");
        qFatal("EGL error");
    }
    qWarning("Opened display %p\n", m_dpy);

    if (Q_UNLIKELY(!eglInitialize(m_dpy, &major, &minor))) {
        qWarning("Could not initialize egl display\n");
        qFatal("EGL error");
    }

    qWarning("Initialized display %d %d\n", major, minor);
}

QMinimalEglScreen::~QMinimalEglScreen()
{
    if (m_surface)
        eglDestroySurface(m_dpy, m_surface);

    eglTerminate(m_dpy);
}

void QMinimalEglScreen::createAndSetPlatformContext() const {
    const_cast<QMinimalEglScreen *>(this)->createAndSetPlatformContext();
}

void QMinimalEglScreen::createAndSetPlatformContext()
{
    QSurfaceFormat platformFormat;

    QByteArray depthString = qgetenv("QT_QPA_EGLFS_DEPTH");
    if (depthString.toInt() == 16) {
        platformFormat.setDepthBufferSize(16);
        platformFormat.setRedBufferSize(5);
        platformFormat.setGreenBufferSize(6);
        platformFormat.setBlueBufferSize(5);
        m_depth = 16;
        m_format = QImage::Format_RGB16;
    } else {
        platformFormat.setDepthBufferSize(24);
        platformFormat.setStencilBufferSize(8);
        platformFormat.setRedBufferSize(8);
        platformFormat.setGreenBufferSize(8);
        platformFormat.setBlueBufferSize(8);
        m_depth = 32;
        m_format = QImage::Format_RGB32;
    }

    if (!qEnvironmentVariableIsEmpty("QT_QPA_EGLFS_MULTISAMPLE"))
        platformFormat.setSamples(4);

    EGLConfig config = q_configFromGLFormat(m_dpy, platformFormat);

    EGLNativeWindowType eglWindow = 0;
#ifdef Q_OPENKODE
    if (Q_UNLIKELY(kdInitializeNV() == KD_ENOTINITIALIZED))
        qFatal("Did not manage to initialize openkode");

    KDWindow *window = kdCreateWindow(m_dpy,config,0);

    kdRealizeWindow(window,&eglWindow);
#endif

#ifdef QEGL_EXTRA_DEBUG
    q_printEglConfig(m_dpy, config);
#endif

    m_surface = eglCreateWindowSurface(m_dpy, config, eglWindow, nullptr);
    if (Q_UNLIKELY(m_surface == EGL_NO_SURFACE)) {
        qWarning("Could not create the egl surface: error = 0x%x\n", eglGetError());
        eglTerminate(m_dpy);
        qFatal("EGL error");
    }
    //    qWarning("Created surface %dx%d\n", w, h);

#ifndef QT_NO_OPENGL
    QEGLPlatformContext *platformContext = new QMinimalEglContext(platformFormat, nullptr, m_dpy);
    m_platformContext = platformContext;
#endif
    EGLint w,h;                    // screen size detection
    eglQuerySurface(m_dpy, m_surface, EGL_WIDTH, &w);
    eglQuerySurface(m_dpy, m_surface, EGL_HEIGHT, &h);

    m_geometry = QRect(0,0,w,h);

}

QRect QMinimalEglScreen::geometry() const
{
    if (m_geometry.isNull()) {
        createAndSetPlatformContext();
    }
    return m_geometry;
}

int QMinimalEglScreen::depth() const
{
    return m_depth;
}

QImage::Format QMinimalEglScreen::format() const
{
    if (m_format == QImage::Format_Invalid)
        createAndSetPlatformContext();
    return m_format;
}
#ifndef QT_NO_OPENGL
QPlatformOpenGLContext *QMinimalEglScreen::platformContext() const
{
    if (!m_platformContext) {
        QMinimalEglScreen *that = const_cast<QMinimalEglScreen *>(this);
        that->createAndSetPlatformContext();
    }
    return m_platformContext;
}
#endif
QT_END_NAMESPACE
