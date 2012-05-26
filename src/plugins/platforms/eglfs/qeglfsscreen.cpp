/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglfscursor.h"
#include "qeglfsscreen.h"
#include "qeglfswindow.h"
#include "qeglfshooks.h"

#include <QtPlatformSupport/private/qeglconvenience_p.h>
#include <QtPlatformSupport/private/qeglplatformcontext_p.h>

QT_BEGIN_NAMESPACE

// #define QEGL_EXTRA_DEBUG

class QEglFSContext : public QEGLPlatformContext
{
public:
    QEglFSContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display,
                  EGLenum eglApi = EGL_OPENGL_ES_API)
        : QEGLPlatformContext(format, share, display, eglApi)
    {
    }

    EGLSurface eglSurfaceForPlatformSurface(QPlatformSurface *surface)
    {
        QEglFSWindow *window = static_cast<QEglFSWindow *>(surface);
        QEglFSScreen *screen = static_cast<QEglFSScreen *>(window->screen());
        return screen->surface();
    }

    void swapBuffers(QPlatformSurface *surface)
    {
        QEglFSWindow *window = static_cast<QEglFSWindow *>(surface);
        QEglFSScreen *screen = static_cast<QEglFSScreen *>(window->screen());
        if (QEglFSCursor *cursor = static_cast<QEglFSCursor *>(screen->cursor()))
            cursor->paintOnScreen();

        QEGLPlatformContext::swapBuffers(surface);
    }
};

QEglFSScreen::QEglFSScreen()
    : m_depth(32)
    , m_format(QImage::Format_Invalid)
    , m_platformContext(0)
    , m_surface(0)
    , m_window(0)
    , m_cursor(0)
{
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglScreen %p\n", this);
#endif

    hooks->platformInit();

    EGLint major, minor;

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        qWarning("Could not bind GL_ES API\n");
        qFatal("EGL error");
    }

    m_dpy = eglGetDisplay(hooks ? hooks->platformDisplay() : EGL_DEFAULT_DISPLAY);
    if (m_dpy == EGL_NO_DISPLAY) {
        qWarning("Could not open egl display\n");
        qFatal("EGL error");
    }
    qWarning("Opened display %p\n", m_dpy);

    if (!eglInitialize(m_dpy, &major, &minor)) {
        qWarning("Could not initialize egl display\n");
        qFatal("EGL error");
    }

    qWarning("Initialized display %d %d\n", major, minor);

    int swapInterval = 1;
    QByteArray swapIntervalString = qgetenv("QT_QPA_EGLFS_SWAPINTERVAL");
    if (!swapIntervalString.isEmpty()) {
        bool ok;
        swapInterval = swapIntervalString.toInt(&ok);
        if (!ok)
            swapInterval = 1;
    }
    eglSwapInterval(m_dpy, swapInterval);

    static int hideCursor = qgetenv("QT_QPA_EGLFS_HIDECURSOR").toInt();
    if (!hideCursor) {
        if (QEglFSCursor *customCursor = hooks->createCursor(this))
            m_cursor = customCursor;
        else
            m_cursor = new QEglFSCursor(this);
    }
}

QEglFSScreen::~QEglFSScreen()
{
    delete m_cursor;

    if (m_surface)
        eglDestroySurface(m_dpy, m_surface);

    hooks->destroyNativeWindow(m_window);

    eglTerminate(m_dpy);

    hooks->platformDestroy();
}

void QEglFSScreen::createAndSetPlatformContext() const {
    const_cast<QEglFSScreen *>(this)->createAndSetPlatformContext();
}

void QEglFSScreen::createAndSetPlatformContext()
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

    if (!qgetenv("QT_QPA_EGLFS_MULTISAMPLE").isEmpty())
        platformFormat.setSamples(4);

    EGLConfig config = q_configFromGLFormat(m_dpy, platformFormat);

    m_window = hooks->createNativeWindow(hooks->screenSize());

#ifdef QEGL_EXTRA_DEBUG
    q_printEglConfig(m_dpy, config);
#endif

    m_surface = eglCreateWindowSurface(m_dpy, config, m_window, NULL);
    if (m_surface == EGL_NO_SURFACE) {
        qWarning("Could not create the egl surface: error = 0x%x\n", eglGetError());
        eglTerminate(m_dpy);
        qFatal("EGL error");
    }
    //    qWarning("Created surface %dx%d\n", w, h);

    QEGLPlatformContext *platformContext = new QEglFSContext(platformFormat, 0, m_dpy);
    m_platformContext = platformContext;

    EGLint w,h;                    // screen size detection
    eglQuerySurface(m_dpy, m_surface, EGL_WIDTH, &w);
    eglQuerySurface(m_dpy, m_surface, EGL_HEIGHT, &h);

    m_geometry = QRect(0,0,w,h);

}

QRect QEglFSScreen::geometry() const
{
    if (m_geometry.isNull()) {
        createAndSetPlatformContext();
    }
    return m_geometry;
}

int QEglFSScreen::depth() const
{
    return m_depth;
}

QImage::Format QEglFSScreen::format() const
{
    if (m_format == QImage::Format_Invalid)
        createAndSetPlatformContext();
    return m_format;
}

QPlatformCursor *QEglFSScreen::cursor() const
{
    return m_cursor;
}

QPlatformOpenGLContext *QEglFSScreen::platformContext() const
{
    if (!m_platformContext) {
        QEglFSScreen *that = const_cast<QEglFSScreen *>(this);
        that->createAndSetPlatformContext();
    }
    return m_platformContext;
}

QT_END_NAMESPACE
