/***************************************************************************
**
** Copyright (C) 2011 - 2013 BlackBerry Limited. All rights reserved.
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

#include "qqnxglcontext.h"
#include "qqnxscreen.h"
#include "qqnxeglwindow.h"

#include "private/qeglconvenience_p.h"

#include <QtCore/QDebug>
#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>

#if defined(QQNXGLCONTEXT_DEBUG)
#define qGLContextDebug qDebug
#else
#define qGLContextDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

EGLDisplay QQnxGLContext::ms_eglDisplay = EGL_NO_DISPLAY;

QQnxGLContext::QQnxGLContext(QOpenGLContext *glContext)
    : QPlatformOpenGLContext(),
      m_glContext(glContext),
      m_currentEglSurface(EGL_NO_SURFACE)
{
    qGLContextDebug();
    QSurfaceFormat format = m_glContext->format();

    // Set current rendering API
    EGLBoolean eglResult = eglBindAPI(EGL_OPENGL_ES_API);
    if (Q_UNLIKELY(eglResult != EGL_TRUE))
        qFatal("QQNX: failed to set EGL API, err=%d", eglGetError());

    // Get colour channel sizes from window format
    int alphaSize = format.alphaBufferSize();
    int redSize = format.redBufferSize();
    int greenSize = format.greenBufferSize();
    int blueSize = format.blueBufferSize();

    // Check if all channels are don't care
    if (alphaSize == -1 && redSize == -1 && greenSize == -1 && blueSize == -1) {
        // Set colour channels based on depth of window's screen
        QQnxScreen *screen = static_cast<QQnxScreen*>(glContext->screen()->handle());
        int depth = screen->depth();
        if (depth == 32) {
            // SCREEN_FORMAT_RGBA8888
            alphaSize = 8;
            redSize = 8;
            greenSize = 8;
            blueSize = 8;
        } else {
            // SCREEN_FORMAT_RGB565
            alphaSize = 0;
            redSize = 5;
            greenSize = 6;
            blueSize = 5;
        }
    } else {
        // Choose best match based on supported pixel formats
        if (alphaSize <= 0 && redSize <= 5 && greenSize <= 6 && blueSize <= 5) {
            // SCREEN_FORMAT_RGB565
            alphaSize = 0;
            redSize = 5;
            greenSize = 6;
            blueSize = 5;
        } else {
            // SCREEN_FORMAT_RGBA8888
            alphaSize = 8;
            redSize = 8;
            greenSize = 8;
            blueSize = 8;
        }
    }

    // Update colour channel sizes in window format
    format.setAlphaBufferSize(alphaSize);
    format.setRedBufferSize(redSize);
    format.setGreenBufferSize(greenSize);
    format.setBlueBufferSize(blueSize);

    // Select EGL config based on requested window format
    m_eglConfig = q_configFromGLFormat(ms_eglDisplay, format);
    if (Q_UNLIKELY(m_eglConfig == 0))
        qFatal("QQnxGLContext: failed to find EGL config");

    QQnxGLContext *glShareContext = static_cast<QQnxGLContext*>(m_glContext->shareHandle());
    m_eglShareContext = glShareContext ? glShareContext->m_eglContext : EGL_NO_CONTEXT;

    m_eglContext = eglCreateContext(ms_eglDisplay, m_eglConfig, m_eglShareContext,
                                    contextAttrs(format));
    if (Q_UNLIKELY(m_eglContext == EGL_NO_CONTEXT)) {
        checkEGLError("eglCreateContext");
        qFatal("QQnxGLContext: failed to create EGL context, err=%d", eglGetError());
    }

    // Query/cache window format of selected EGL config
    m_windowFormat = q_glFormatFromConfig(ms_eglDisplay, m_eglConfig);
}

QQnxGLContext::~QQnxGLContext()
{
    qGLContextDebug();

    // Cleanup EGL context if it exists
    if (m_eglContext != EGL_NO_CONTEXT)
        eglDestroyContext(ms_eglDisplay, m_eglContext);
}

EGLenum QQnxGLContext::checkEGLError(const char *msg)
{
    static const char *errmsg[] =
    {
        "EGL function succeeded",
        "EGL is not initialized, or could not be initialized, for the specified display",
        "EGL cannot access a requested resource",
        "EGL failed to allocate resources for the requested operation",
        "EGL fail to access an unrecognized attribute or attribute value was passed in an attribute list",
        "EGLConfig argument does not name a valid EGLConfig",
        "EGLContext argument does not name a valid EGLContext",
        "EGL current surface of the calling thread is no longer valid",
        "EGLDisplay argument does not name a valid EGLDisplay",
        "EGL arguments are inconsistent",
        "EGLNativePixmapType argument does not refer to a valid native pixmap",
        "EGLNativeWindowType argument does not refer to a valid native window",
        "EGL one or more argument values are invalid",
        "EGLSurface argument does not name a valid surface configured for rendering",
        "EGL power management event has occurred",
    };
    EGLenum error = eglGetError();
    fprintf(stderr, "%s: %s\n", msg, errmsg[error - EGL_SUCCESS]);
    return error;
}

void QQnxGLContext::initializeContext()
{
    qGLContextDebug();

    // Initialize connection to EGL
    ms_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (Q_UNLIKELY(ms_eglDisplay == EGL_NO_DISPLAY)) {
        checkEGLError("eglGetDisplay");
        qFatal("QQnxGLContext: failed to obtain EGL display");
    }

    EGLBoolean eglResult = eglInitialize(ms_eglDisplay, 0, 0);
    if (Q_UNLIKELY(eglResult != EGL_TRUE)) {
        checkEGLError("eglInitialize");
        qFatal("QQnxGLContext: failed to initialize EGL display, err=%d", eglGetError());
    }
}

void QQnxGLContext::shutdownContext()
{
    qGLContextDebug();

    // Close connection to EGL
    eglTerminate(ms_eglDisplay);
}

bool QQnxGLContext::makeCurrent(QPlatformSurface *surface)
{
    qGLContextDebug();

    Q_ASSERT(surface->surface()->surfaceType() == QSurface::OpenGLSurface);

    // Set current rendering API
    EGLBoolean eglResult = eglBindAPI(EGL_OPENGL_ES_API);
    if (Q_UNLIKELY(eglResult != EGL_TRUE))
        qFatal("QQnxGLContext: failed to set EGL API, err=%d", eglGetError());

    QQnxEglWindow *platformWindow = dynamic_cast<QQnxEglWindow*>(surface);
    if (!platformWindow)
        return false;

    platformWindow->setPlatformOpenGLContext(this);

    if (m_currentEglSurface == EGL_NO_SURFACE || m_currentEglSurface != platformWindow->getSurface()) {
        m_currentEglSurface = platformWindow->getSurface();
        doneCurrent();
    }

    eglResult = eglMakeCurrent(ms_eglDisplay, m_currentEglSurface, m_currentEglSurface, m_eglContext);
    if (eglResult != EGL_TRUE) {
        checkEGLError("eglMakeCurrent");
        qWarning("QQNX: failed to set current EGL context, err=%d", eglGetError());
        return false;
    }
    return (eglResult == EGL_TRUE);
}

void QQnxGLContext::doneCurrent()
{
    qGLContextDebug();

    // set current rendering API
    EGLBoolean eglResult = eglBindAPI(EGL_OPENGL_ES_API);
    if (Q_UNLIKELY(eglResult != EGL_TRUE))
        qFatal("QQNX: failed to set EGL API, err=%d", eglGetError());

    // clear curent EGL context and unbind EGL surface
    eglResult = eglMakeCurrent(ms_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (Q_UNLIKELY(eglResult != EGL_TRUE))
        qFatal("QQNX: failed to clear current EGL context, err=%d", eglGetError());
}

void QQnxGLContext::swapBuffers(QPlatformSurface *surface)
{
    qGLContextDebug();
    QQnxEglWindow *platformWindow = dynamic_cast<QQnxEglWindow*>(surface);
    if (!platformWindow)
        return;

    platformWindow->swapEGLBuffers();
}

QFunctionPointer QQnxGLContext::getProcAddress(const char *procName)
{
    qGLContextDebug();

    // Set current rendering API
    EGLBoolean eglResult = eglBindAPI(EGL_OPENGL_ES_API);
    if (Q_UNLIKELY(eglResult != EGL_TRUE))
        qFatal("QQNX: failed to set EGL API, err=%d", eglGetError());

    // Lookup EGL extension function pointer
    return static_cast<QFunctionPointer>(eglGetProcAddress(procName));
}

bool QQnxGLContext::isSharing() const
{
    return m_eglShareContext != EGL_NO_CONTEXT;
}

EGLDisplay QQnxGLContext::getEglDisplay() {
    return ms_eglDisplay;
}

EGLint *QQnxGLContext::contextAttrs(const QSurfaceFormat &format)
{
    qGLContextDebug();

    // Choose EGL settings based on OpenGL version
#if defined(QT_OPENGL_ES_2)
    static EGLint attrs[] = { EGL_CONTEXT_CLIENT_VERSION, format.version().first, EGL_NONE };
    return attrs;
#else
    return 0;
#endif
}

QT_END_NAMESPACE
