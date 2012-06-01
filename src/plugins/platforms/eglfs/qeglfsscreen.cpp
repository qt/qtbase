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

QEglFSScreen::QEglFSScreen(EGLDisplay dpy)
    : m_dpy(dpy)
    , m_platformContext(0)
    , m_surface(0)
    , m_window(0)
    , m_cursor(0)
{
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglScreen %p\n", this);
#endif

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
}

void QEglFSScreen::createAndSetPlatformContext() const {
    const_cast<QEglFSScreen *>(this)->createAndSetPlatformContext();
}

void QEglFSScreen::createAndSetPlatformContext()
{
    QSurfaceFormat platformFormat = hooks->defaultSurfaceFormat();

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
}

QRect QEglFSScreen::geometry() const
{
    return QRect(QPoint(0, 0), hooks->screenSize());
}

int QEglFSScreen::depth() const
{
    return hooks->screenDepth();
}

QImage::Format QEglFSScreen::format() const
{
    return hooks->screenFormat();
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
