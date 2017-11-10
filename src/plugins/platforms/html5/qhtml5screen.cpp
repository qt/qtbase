/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qhtml5screen.h"
#include "qhtml5window.h"
#include "qhtml5compositor.h"

#include <QtEglSupport/private/qeglconvenience_p.h>
#ifndef QT_NO_OPENGL
# include <QtEglSupport/private/qeglplatformcontext_p.h>
#endif
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <private/qhighdpiscaling_p.h>

#ifdef Q_OPENKODE
#include <KD/kd.h>
#include <KD/NV_initialize.h>
#endif //Q_OPENKODE

QT_BEGIN_NAMESPACE

// #define QEGL_EXTRA_DEBUG

#ifndef QT_NO_OPENGL

class QHtml5Context : public QEGLPlatformContext
{
public:
    QHtml5Context(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display)
        : QEGLPlatformContext(format, share, display, 0, QVariant(), QEGLPlatformContext::NoSurfaceless)
    {
    }

    EGLSurface eglSurfaceForPlatformSurface(QPlatformSurface *surface) Q_DECL_OVERRIDE
    {
        QHtml5Window *window = static_cast<QHtml5Window *>(surface);
        QHTML5Screen *screen = static_cast<QHTML5Screen *>(window->screen());
        return screen->surface();
    }
};

#endif

QHTML5Screen::QHTML5Screen(EGLNativeDisplayType display, QHtml5Compositor *compositor)
    : mCompositor(compositor)
    , m_depth(32)
    , m_format(QImage::Format_Invalid)
    , m_platformContext(0)
    , m_context(0)
    , m_surface(0)
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

    mCompositor->setScreen(this);
}

QHTML5Screen::~QHTML5Screen()
{
    if (m_surface)
        eglDestroySurface(m_dpy, m_surface);

    eglTerminate(m_dpy);
}

void QHTML5Screen::createAndSetPlatformContext() const {
    const_cast<QHTML5Screen *>(this)->createAndSetPlatformContext();
}

void QHTML5Screen::createAndSetPlatformContext()
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

//#ifdef QEGL_EXTRA_DEBUG
    q_printEglConfig(m_dpy, config);
//#endif

    m_surface = eglCreateWindowSurface(m_dpy, config, eglWindow, NULL);
    if (Q_UNLIKELY(m_surface == EGL_NO_SURFACE)) {
        qWarning("Could not create the egl surface: error = 0x%x\n", eglGetError());
        eglTerminate(m_dpy);
        qFatal("EGL error");
    }

    QEGLPlatformContext *platformContext = new QHtml5Context(platformFormat, 0, m_dpy);
    m_platformContext = platformContext;

    EGLint w,h;                    // screen size detection
    eglQuerySurface(m_dpy, m_surface, EGL_WIDTH, &w);
    eglQuerySurface(m_dpy, m_surface, EGL_HEIGHT, &h);

    m_geometry = QRect(0,0,w,h);

    //m_context.reset(new QOpenGLContext);
    //m_context->setFormat(platformFormat);
    //m_context->setScreen(screen);
    //m_context->create();
}

QRect QHTML5Screen::geometry() const
{
    if (m_geometry.isNull()) {
        createAndSetPlatformContext();
    }
    return m_geometry;
}

int QHTML5Screen::depth() const
{
    return m_depth;
}

QImage::Format QHTML5Screen::format() const
{
    if (m_format == QImage::Format_Invalid)
        createAndSetPlatformContext();
    return m_format;
}

#ifndef QT_NO_OPENGL
QPlatformOpenGLContext *QHTML5Screen::platformContext() const
{
    if (!m_platformContext) {
        QHTML5Screen *that = const_cast<QHTML5Screen *>(this);
        that->createAndSetPlatformContext();
    }
    return m_platformContext;
}
#endif

void QHTML5Screen::resizeMaximizedWindows()
{
    QList<QWindow*> windows = QGuiApplication::allWindows();
    // 'screen()' still has the old geometry info while 'this' has the new geometry info
    const QRect oldGeometry = screen()->geometry();
    const QRect oldAvailableGeometry = screen()->availableGeometry();

    const QRect newGeometry = deviceIndependentGeometry();
    const QRect newAvailableGeometry = QHighDpi::fromNative(availableGeometry(), QHighDpiScaling::factor(this), newGeometry.topLeft());

    // make sure maximized and fullscreen windows are updated
    for (int i = 0; i < windows.size(); ++i) {
        QWindow *w = windows.at(i);

        // Skip non-platform windows, e.g., offscreen windows.
        if (!w->handle())
            continue;

        if (platformScreenForWindow(w) != this)
            continue;

        if (w->windowState() & Qt::WindowMaximized || w->geometry() == oldAvailableGeometry)
            w->setGeometry(newAvailableGeometry);

        else if (w->windowState() & Qt::WindowFullScreen || w->geometry() == oldGeometry)
            w->setGeometry(newGeometry);
    }
}

QWindow *QHTML5Screen::topWindow() const
{
    return mCompositor->keyWindow();
}

QWindow *QHTML5Screen::topLevelAt(const QPoint & p) const
{
    return mCompositor->windowAt(p);
}

void QHTML5Screen::invalidateSize()
{
    m_geometry = QRect();
}

void QHTML5Screen::setGeometry(const QRect &rect)
{
    m_geometry = rect;
    QWindowSystemInterface::handleScreenGeometryChange(QPlatformScreen::screen(), geometry(), availableGeometry());
    resizeMaximizedWindows();
}

QT_END_NAMESPACE
