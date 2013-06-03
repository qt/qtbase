/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqnxglcontext.h"
#include "qqnxrootwindow.h"
#include "qqnxscreen.h"
#include "qqnxwindow.h"

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

static EGLenum checkEGLError(const char *msg)
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

QQnxGLContext::QQnxGLContext(QOpenGLContext *glContext)
    : QPlatformOpenGLContext(),
      m_glContext(glContext),
      m_eglSurface(EGL_NO_SURFACE),
      m_newSurfaceRequested(true)   // Create a surface the first time makeCurrent() is called
{
    qGLContextDebug() << Q_FUNC_INFO;
    QSurfaceFormat format = m_glContext->format();

    // Set current rendering API
    EGLBoolean eglResult = eglBindAPI(EGL_OPENGL_ES_API);
    if (eglResult != EGL_TRUE)
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
    if (m_eglConfig == 0)
        qFatal("QQnxGLContext: failed to find EGL config");

    m_eglContext = eglCreateContext(ms_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, contextAttrs());
    if (m_eglContext == EGL_NO_CONTEXT) {
        checkEGLError("eglCreateContext");
        qFatal("QQnxGLContext: failed to create EGL context, err=%d", eglGetError());
    }

    // Query/cache window format of selected EGL config
    m_windowFormat = q_glFormatFromConfig(ms_eglDisplay, m_eglConfig);
}

QQnxGLContext::~QQnxGLContext()
{
    qGLContextDebug() << Q_FUNC_INFO;

    // Cleanup EGL context if it exists
    if (m_eglContext != EGL_NO_CONTEXT)
        eglDestroyContext(ms_eglDisplay, m_eglContext);

    // Cleanup EGL surface if it exists
    destroySurface();
}

void QQnxGLContext::initialize()
{
    qGLContextDebug() << Q_FUNC_INFO;

    // Initialize connection to EGL
    ms_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (ms_eglDisplay == EGL_NO_DISPLAY) {
        checkEGLError("eglGetDisplay");
        qFatal("QQnxGLContext: failed to obtain EGL display");
    }

    EGLBoolean eglResult = eglInitialize(ms_eglDisplay, 0, 0);
    if (eglResult != EGL_TRUE) {
        checkEGLError("eglInitialize");
        qFatal("QQnxGLContext: failed to initialize EGL display, err=%d", eglGetError());
    }
}

void QQnxGLContext::shutdown()
{
    qGLContextDebug() << Q_FUNC_INFO;

    // Close connection to EGL
    eglTerminate(ms_eglDisplay);
}

void QQnxGLContext::requestSurfaceChange()
{
    qGLContextDebug() << Q_FUNC_INFO;
    m_newSurfaceRequested.testAndSetRelease(false, true);
}

bool QQnxGLContext::makeCurrent(QPlatformSurface *surface)
{
    qGLContextDebug() << Q_FUNC_INFO;

    Q_ASSERT(surface->surface()->surfaceType() == QSurface::OpenGLSurface);

    // Set current rendering API
    EGLBoolean eglResult = eglBindAPI(EGL_OPENGL_ES_API);
    if (eglResult != EGL_TRUE)
        qFatal("QQnxGLContext: failed to set EGL API, err=%d", eglGetError());

    if (m_newSurfaceRequested.testAndSetOrdered(true, false)) {
        qGLContextDebug() << "New EGL surface requested";
        doneCurrent();
        destroySurface();
        createSurface(surface);
    }

    eglResult = eglMakeCurrent(ms_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);
    if (eglResult != EGL_TRUE) {
        checkEGLError("eglMakeCurrent");
        qFatal("QQNX: failed to set current EGL context, err=%d", eglGetError());
    }
    return (eglResult == EGL_TRUE);
}

void QQnxGLContext::doneCurrent()
{
    qGLContextDebug() << Q_FUNC_INFO;

    // set current rendering API
    EGLBoolean eglResult = eglBindAPI(EGL_OPENGL_ES_API);
    if (eglResult != EGL_TRUE)
        qFatal("QQNX: failed to set EGL API, err=%d", eglGetError());

    // clear curent EGL context and unbind EGL surface
    eglResult = eglMakeCurrent(ms_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (eglResult != EGL_TRUE)
        qFatal("QQNX: failed to clear current EGL context, err=%d", eglGetError());
}

void QQnxGLContext::swapBuffers(QPlatformSurface *surface)
{
    Q_UNUSED(surface);
    qGLContextDebug() << Q_FUNC_INFO;

    // Set current rendering API
    EGLBoolean eglResult = eglBindAPI(EGL_OPENGL_ES_API);
    if (eglResult != EGL_TRUE)
        qFatal("QQNX: failed to set EGL API, err=%d", eglGetError());

    // Post EGL surface to window
    eglResult = eglSwapBuffers(ms_eglDisplay, m_eglSurface);
    if (eglResult != EGL_TRUE)
        qFatal("QQNX: failed to swap EGL buffers, err=%d", eglGetError());
}

QFunctionPointer QQnxGLContext::getProcAddress(const QByteArray &procName)
{
    qGLContextDebug() << Q_FUNC_INFO;

    // Set current rendering API
    EGLBoolean eglResult = eglBindAPI(EGL_OPENGL_ES_API);
    if (eglResult != EGL_TRUE)
        qFatal("QQNX: failed to set EGL API, err=%d", eglGetError());

    // Lookup EGL extension function pointer
    return static_cast<QFunctionPointer>(eglGetProcAddress(procName.constData()));
}

EGLint *QQnxGLContext::contextAttrs()
{
    qGLContextDebug() << Q_FUNC_INFO;

    // Choose EGL settings based on OpenGL version
#if defined(QT_OPENGL_ES_2)
    static EGLint attrs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    return attrs;
#else
    return 0;
#endif
}

bool QQnxGLContext::isCurrent() const
{
    qGLContextDebug() << Q_FUNC_INFO;
    return (eglGetCurrentContext() == m_eglContext);
}

void QQnxGLContext::createSurface(QPlatformSurface *surface)
{
    qGLContextDebug() << Q_FUNC_INFO;

    // Get a pointer to the corresponding platform window
    QQnxWindow *platformWindow = dynamic_cast<QQnxWindow*>(surface);
    if (!platformWindow)
        qFatal("QQNX: unable to create EGLSurface without a QQnxWindow");

    // Link the window and context
    platformWindow->setPlatformOpenGLContext(this);

    // Fetch the surface size from the window and update
    // the window's buffers before we create the EGL surface
    const QSize surfaceSize = platformWindow->requestedBufferSize();
    if (!surfaceSize.isValid()) {
        qFatal("QQNX: Trying to create 0 size EGL surface. "
               "Please set a valid window size before calling QOpenGLContext::makeCurrent()");
    }
    platformWindow->setBufferSize(surfaceSize);

    // Post root window, in case it hasn't been posted yet, to make it appear.
    platformWindow->screen()->onWindowPost(platformWindow);

    // Obtain the native handle for our window
    screen_window_t handle = platformWindow->nativeHandle();

    const EGLint eglSurfaceAttrs[] =
    {
        EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
        EGL_NONE
    };

    // Create EGL surface
    m_eglSurface = eglCreateWindowSurface(ms_eglDisplay, m_eglConfig, (EGLNativeWindowType) handle, eglSurfaceAttrs);
    if (m_eglSurface == EGL_NO_SURFACE) {
        checkEGLError("eglCreateWindowSurface");
        qFatal("QQNX: failed to create EGL surface, err=%d", eglGetError());
    }
}

void QQnxGLContext::destroySurface()
{
    qGLContextDebug() << Q_FUNC_INFO;

    // Destroy EGL surface if it exists
    if (m_eglSurface != EGL_NO_SURFACE) {
        EGLBoolean eglResult = eglDestroySurface(ms_eglDisplay, m_eglSurface);
        if (eglResult != EGL_TRUE) {
            qFatal("QQNX: failed to destroy EGL surface, err=%d", eglGetError());
        }
    }

    m_eglSurface = EGL_NO_SURFACE;
}

QT_END_NAMESPACE
