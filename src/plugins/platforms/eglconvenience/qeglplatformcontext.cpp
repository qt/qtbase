/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglplatformcontext.h"


#include <QtGui/QPlatformWindow>

#include "qeglconvenience.h"

#include <EGL/egl.h>

QEGLPlatformContext::QEGLPlatformContext(EGLDisplay display, EGLConfig config, EGLint contextAttrs[], EGLSurface surface, EGLenum eglApi)
    : QPlatformGLContext()
    , m_eglDisplay(display)
    , m_eglSurface(surface)
    , m_eglApi(eglApi)
{
    if (m_eglSurface == EGL_NO_SURFACE) {
        qWarning("Createing QEGLPlatformContext with no surface");
    }

    eglBindAPI(m_eglApi);
    m_eglContext = eglCreateContext(m_eglDisplay,config, 0,contextAttrs);
    if (m_eglContext == EGL_NO_CONTEXT) {
        qWarning("Could not create the egl context\n");
        eglTerminate(m_eglDisplay);
        qFatal("EGL error");
    }

    m_windowFormat = qt_qPlatformWindowFormatFromConfig(display,config);
}

QEGLPlatformContext::~QEGLPlatformContext()
{
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglContext::~QEglContext(): %p\n",this);
#endif
    if (m_eglSurface != EGL_NO_SURFACE) {
        doneCurrent();
        eglDestroySurface(m_eglDisplay, m_eglSurface);
        m_eglSurface = EGL_NO_SURFACE;
    }

    if (m_eglContext != EGL_NO_CONTEXT) {
        eglDestroyContext(m_eglDisplay, m_eglContext);
        m_eglContext = EGL_NO_CONTEXT;
    }
}

void QEGLPlatformContext::makeCurrent()
{
    QPlatformGLContext::makeCurrent();
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglContext::makeCurrent: %p\n",this);
#endif
    eglBindAPI(m_eglApi);
    bool ok = eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);
    if (!ok)
        qWarning("QEGLPlatformContext::makeCurrent: eglError: %d, this: %p \n", eglGetError(), this);
#ifdef QEGL_EXTRA_DEBUG
    static bool showDebug = true;
    if (showDebug) {
        showDebug = false;
        const char *str = (const char*)glGetString(GL_VENDOR);
        qWarning("Vendor %s\n", str);
        str = (const char*)glGetString(GL_RENDERER);
        qWarning("Renderer %s\n", str);
        str = (const char*)glGetString(GL_VERSION);
        qWarning("Version %s\n", str);

        str = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
        qWarning("Extensions %s\n",str);

        str = (const char*)glGetString(GL_EXTENSIONS);
        qWarning("Extensions %s\n", str);

    }
#endif
}
void QEGLPlatformContext::doneCurrent()
{
    QPlatformGLContext::doneCurrent();
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglContext::doneCurrent:%p\n",this);
#endif
    eglBindAPI(m_eglApi);
    bool ok = eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (!ok)
        qWarning("QEGLPlatformContext::doneCurrent(): eglError: %d, this: %p \n", eglGetError(), this);
}
void QEGLPlatformContext::swapBuffers()
{
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglContext::swapBuffers:%p\n",this);
#endif
    eglBindAPI(m_eglApi);
    bool ok = eglSwapBuffers(m_eglDisplay, m_eglSurface);
    if (!ok)
        qWarning("QEGLPlatformContext::swapBuffers(): eglError: %d, this: %p \n", eglGetError(), this);
}
void* QEGLPlatformContext::getProcAddress(const QString& procName)
{
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglContext::getProcAddress%p\n",this);
#endif
    eglBindAPI(m_eglApi);
    return (void *)eglGetProcAddress(qPrintable(procName));
}

QPlatformWindowFormat QEGLPlatformContext::platformWindowFormat() const
{
    return m_windowFormat;
}

EGLContext QEGLPlatformContext::eglContext() const
{
    return m_eglContext;
}
