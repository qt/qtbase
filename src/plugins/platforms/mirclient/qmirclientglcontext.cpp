/****************************************************************************
**
** Copyright (C) 2014 Canonical, Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qmirclientglcontext.h"
#include "qmirclientwindow.h"
#include "qmirclientlogging.h"
#include <QtPlatformSupport/private/qeglconvenience_p.h>

#if !defined(QT_NO_DEBUG)
static void printOpenGLESConfig() {
  static bool once = true;
  if (once) {
    const char* string = (const char*) glGetString(GL_VENDOR);
    LOG("OpenGL ES vendor: %s", string);
    string = (const char*) glGetString(GL_RENDERER);
    LOG("OpenGL ES renderer: %s", string);
    string = (const char*) glGetString(GL_VERSION);
    LOG("OpenGL ES version: %s", string);
    string = (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION);
    LOG("OpenGL ES Shading Language version: %s", string);
    string = (const char*) glGetString(GL_EXTENSIONS);
    LOG("OpenGL ES extensions: %s", string);
    once = false;
  }
}
#endif

static EGLenum api_in_use()
{
#ifdef QTUBUNTU_USE_OPENGL
    return EGL_OPENGL_API;
#else
    return EGL_OPENGL_ES_API;
#endif
}

QMirClientOpenGLContext::QMirClientOpenGLContext(QMirClientScreen* screen, QMirClientOpenGLContext* share)
{
    ASSERT(screen != NULL);
    mEglDisplay = screen->eglDisplay();
    mScreen = screen;

    // Create an OpenGL ES 2 context.
    QVector<EGLint> attribs;
    attribs.append(EGL_CONTEXT_CLIENT_VERSION);
    attribs.append(2);
    attribs.append(EGL_NONE);
    ASSERT(eglBindAPI(api_in_use()) == EGL_TRUE);

    mEglContext = eglCreateContext(mEglDisplay, screen->eglConfig(), share ? share->eglContext() : EGL_NO_CONTEXT,
                                   attribs.constData());
    DASSERT(mEglContext != EGL_NO_CONTEXT);
}

QMirClientOpenGLContext::~QMirClientOpenGLContext()
{
    ASSERT(eglDestroyContext(mEglDisplay, mEglContext) == EGL_TRUE);
}

bool QMirClientOpenGLContext::makeCurrent(QPlatformSurface* surface)
{
    DASSERT(surface->surface()->surfaceType() == QSurface::OpenGLSurface);
    EGLSurface eglSurface = static_cast<QMirClientWindow*>(surface)->eglSurface();
#if defined(QT_NO_DEBUG)
    eglBindAPI(api_in_use());
    eglMakeCurrent(mEglDisplay, eglSurface, eglSurface, mEglContext);
#else
    ASSERT(eglBindAPI(api_in_use()) == EGL_TRUE);
    ASSERT(eglMakeCurrent(mEglDisplay, eglSurface, eglSurface, mEglContext) == EGL_TRUE);
    printOpenGLESConfig();
#endif
    return true;
}

void QMirClientOpenGLContext::doneCurrent()
{
#if defined(QT_NO_DEBUG)
    eglBindAPI(api_in_use());
    eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
#else
    ASSERT(eglBindAPI(api_in_use()) == EGL_TRUE);
    ASSERT(eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_TRUE);
#endif
}

void QMirClientOpenGLContext::swapBuffers(QPlatformSurface* surface)
{
    QMirClientWindow *ubuntuWindow = static_cast<QMirClientWindow*>(surface);

    EGLSurface eglSurface = ubuntuWindow->eglSurface();
#if defined(QT_NO_DEBUG)
    eglBindAPI(api_in_use());
    eglSwapBuffers(mEglDisplay, eglSurface);
#else
    ASSERT(eglBindAPI(api_in_use()) == EGL_TRUE);
    ASSERT(eglSwapBuffers(mEglDisplay, eglSurface) == EGL_TRUE);
#endif

    ubuntuWindow->onSwapBuffersDone();
}

void (*QMirClientOpenGLContext::getProcAddress(const QByteArray& procName)) ()
{
#if defined(QT_NO_DEBUG)
    eglBindAPI(api_in_use());
#else
    ASSERT(eglBindAPI(api_in_use()) == EGL_TRUE);
#endif
    return eglGetProcAddress(procName.constData());
}
