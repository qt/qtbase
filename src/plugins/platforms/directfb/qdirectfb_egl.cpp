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

#include "qdirectfb_egl.h"
#include "qdirectfbwindow.h"
#include "qdirectfbscreen.h"
#include "qdirectfbeglhooks.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/QPlatformOpenGLContext>
#include <QtGui/QScreen>

#include <QtPlatformSupport/private/qeglplatformcontext_p.h>
#include <QtPlatformSupport/private/qeglconvenience_p.h>

#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

#ifdef DIRECTFB_PLATFORM_HOOKS
extern QDirectFBEGLHooks platform_hook;
static QDirectFBEGLHooks *hooks = &platform_hook;
#else
static QDirectFBEGLHooks *hooks = 0;
#endif

/**
 * This provides OpenGL ES 2.0 integration with DirectFB. It assumes that
 * one can adapt a DirectFBSurface as a EGLSurface. It might need some vendor
 * init code in the QDirectFbScreenEGL class to initialize EGL and one might
 * need to provide a custom QDirectFbWindowEGL::format() to return the
 * QSurfaceFormat used by the device.
 */

class QDirectFbScreenEGL : public QDirectFbScreen {
public:
    QDirectFbScreenEGL(int display);
    ~QDirectFbScreenEGL();

    // EGL helper
    EGLDisplay eglDisplay();

private:
    void initializeEGL();
    void platformInit();
    void platformDestroy();

private:
    EGLDisplay m_eglDisplay;
};

class QDirectFbWindowEGL : public QDirectFbWindow {
public:
    QDirectFbWindowEGL(QWindow *tlw, QDirectFbInput *inputhandler);
    ~QDirectFbWindowEGL();

    // EGL. Subclass it instead to have different GL integrations?
    EGLSurface eglSurface();

    QSurfaceFormat format() const;

private:
    EGLSurface m_eglSurface;
};

class QDirectFbEGLContext : public QEGLPlatformContext {
public:
    QDirectFbEGLContext(QDirectFbScreenEGL *screen, QOpenGLContext *context);

protected:
    EGLSurface eglSurfaceForPlatformSurface(QPlatformSurface *surface);

private:
    QDirectFbScreen *m_screen;
};

QDirectFbScreenEGL::QDirectFbScreenEGL(int display)
    : QDirectFbScreen(display)
    , m_eglDisplay(EGL_NO_DISPLAY)
{}

QDirectFbScreenEGL::~QDirectFbScreenEGL()
{
    platformDestroy();
}

EGLDisplay QDirectFbScreenEGL::eglDisplay()
{
    if (m_eglDisplay == EGL_NO_DISPLAY)
        initializeEGL();
    return m_eglDisplay;
}

void QDirectFbScreenEGL::initializeEGL()
{
    m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_eglDisplay == EGL_NO_DISPLAY)
        return;

    platformInit();

    EGLint major, minor;
    eglBindAPI(EGL_OPENGL_ES_API);
    eglInitialize(m_eglDisplay, &major, &minor);
    return;
}

void QDirectFbScreenEGL::platformInit()
{
    if (hooks)
        hooks->platformInit();
}

void QDirectFbScreenEGL::platformDestroy()
{
    if (hooks)
        hooks->platformDestroy();
}

QDirectFbWindowEGL::QDirectFbWindowEGL(QWindow *tlw, QDirectFbInput *input)
    : QDirectFbWindow(tlw, input)
    , m_eglSurface(EGL_NO_SURFACE)
{}

QDirectFbWindowEGL::~QDirectFbWindowEGL()
{
    if (m_eglSurface != EGL_NO_SURFACE) {
        QDirectFbScreenEGL *dfbScreen;
        dfbScreen = static_cast<QDirectFbScreenEGL*>(screen());
        eglDestroySurface(dfbScreen->eglDisplay(), m_eglSurface);
    }
}

EGLSurface QDirectFbWindowEGL::eglSurface()
{
    if (m_eglSurface == EGL_NO_SURFACE) {
        QDirectFbScreenEGL *dfbScreen = static_cast<QDirectFbScreenEGL *>(screen());
        EGLConfig config = q_configFromGLFormat(dfbScreen->eglDisplay(), format(), true);
        m_eglSurface = eglCreateWindowSurface(dfbScreen->eglDisplay(), config, dfbSurface(), NULL);

        if (m_eglSurface == EGL_NO_SURFACE)
            eglGetError();
    }

    return m_eglSurface;
}

QSurfaceFormat QDirectFbWindowEGL::format() const
{
    return window()->requestedFormat();
}


QDirectFbEGLContext::QDirectFbEGLContext(QDirectFbScreenEGL *screen, QOpenGLContext *context)
    : QEGLPlatformContext(context->format(), context->shareHandle(),
                          screen->eglDisplay(), EGL_OPENGL_ES_API)
    , m_screen(screen)
{}

EGLSurface QDirectFbEGLContext::eglSurfaceForPlatformSurface(QPlatformSurface *surface)
{
    QDirectFbWindowEGL *window = static_cast<QDirectFbWindowEGL*>(surface);
    return window->eglSurface();
}

QPlatformWindow *QDirectFbIntegrationEGL::createPlatformWindow(QWindow *window) const
{
    return new QDirectFbWindowEGL(window, m_input.data());
}

QPlatformOpenGLContext *QDirectFbIntegrationEGL::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QDirectFbScreenEGL *screen;
    screen = static_cast<QDirectFbScreenEGL*>(context->screen()->handle());
    return new QDirectFbEGLContext(screen, context);
}

void QDirectFbIntegrationEGL::initializeScreen()
{
    m_primaryScreen.reset(new QDirectFbScreenEGL(0));
    screenAdded(m_primaryScreen.data());
}

bool QDirectFbIntegrationEGL::hasCapability(QPlatformIntegration::Capability cap) const
{
    // We assume that devices will have more and not less capabilities
    if (hooks && hooks->hasCapability(cap))
        return true;
    return QDirectFbIntegration::hasCapability(cap);
}

QT_END_NAMESPACE
