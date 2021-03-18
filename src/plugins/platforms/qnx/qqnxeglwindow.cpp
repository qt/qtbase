/***************************************************************************
**
** Copyright (C) 2013 - 2014 BlackBerry Limited. All rights reserved.
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


#include "qqnxeglwindow.h"
#include "qqnxscreen.h"
#include "qqnxglcontext.h"

#include <QDebug>

#include <errno.h>

#if defined(QQNXEGLWINDOW_DEBUG)
#define qEglWindowDebug qDebug
#else
#define qEglWindowDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QQnxEglWindow::QQnxEglWindow(QWindow *window, screen_context_t context, bool needRootWindow) :
    QQnxWindow(window, context, needRootWindow),
    m_newSurfaceRequested(true),
    m_eglDisplay(EGL_NO_DISPLAY),
    m_eglSurface(EGL_NO_SURFACE)
{
    initWindow();

    m_requestedBufferSize = shouldMakeFullScreen() ? screen()->geometry().size() : window->geometry().size();
}

QQnxEglWindow::~QQnxEglWindow()
{
    // Cleanup EGL surface if it exists
    destroyEGLSurface();
}

bool QQnxEglWindow::isInitialized() const
{
    return m_eglSurface != EGL_NO_SURFACE;
}

void QQnxEglWindow::ensureInitialized(QQnxGLContext* context)
{
    if (m_newSurfaceRequested.testAndSetOrdered(true, false)) {
        const QMutexLocker locker(&m_mutex); // Set geomety must not reset the requestedBufferSize till
                                             // the surface is created

        if (m_requestedBufferSize != bufferSize() || m_eglSurface == EGL_NO_SURFACE) {
            if (m_eglSurface != EGL_NO_SURFACE) {
                context->doneCurrent();
                destroyEGLSurface();
            }
            createEGLSurface(context);
        } else {
            // Must've been a sequence of unprocessed changes returning us to the original size.
            resetBuffers();
        }
    }
}

void QQnxEglWindow::createEGLSurface(QQnxGLContext *context)
{
    if (context->format().renderableType() != QSurfaceFormat::OpenGLES) {
        qFatal("QQnxEglWindow: renderable type is not OpenGLES");
        return;
    }

    // Set window usage
    int usage = SCREEN_USAGE_OPENGL_ES2;
#if _SCREEN_VERSION >= _SCREEN_MAKE_VERSION(1, 0, 0)
    if (context->format().majorVersion() == 3)
        usage |= SCREEN_USAGE_OPENGL_ES3;
#endif

    const int result = screen_set_window_property_iv(nativeHandle(), SCREEN_PROPERTY_USAGE, &usage);
    if (Q_UNLIKELY(result != 0))
        qFatal("QQnxEglWindow: failed to set window usage, errno=%d", errno);

    if (!m_requestedBufferSize.isValid()) {
        qWarning("QQNX: Trying to create 0 size EGL surface. "
               "Please set a valid window size before calling QOpenGLContext::makeCurrent()");
        return;
    }

    m_eglDisplay = context->eglDisplay();
    m_eglConfig = context->eglConfig();
    m_format = context->format();

    // update the window's buffers before we create the EGL surface
    setBufferSize(m_requestedBufferSize);

    const EGLint eglSurfaceAttrs[] =
    {
        EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
        EGL_NONE
    };

    qEglWindowDebug() << "Creating EGL surface from" << this << context
        << window()->surfaceType() << window()->type();

    // Create EGL surface
    EGLSurface eglSurface = eglCreateWindowSurface(
        m_eglDisplay,
        m_eglConfig,
        (EGLNativeWindowType) nativeHandle(),
        eglSurfaceAttrs);

    if (eglSurface == EGL_NO_SURFACE)
        qWarning("QQNX: failed to create EGL surface, err=%d", eglGetError());

    m_eglSurface = eglSurface;
}

void QQnxEglWindow::destroyEGLSurface()
{
    // Destroy EGL surface if it exists
    if (m_eglSurface != EGL_NO_SURFACE) {
        EGLBoolean eglResult = eglDestroySurface(m_eglDisplay, m_eglSurface);
        if (Q_UNLIKELY(eglResult != EGL_TRUE))
            qFatal("QQNX: failed to destroy EGL surface, err=%d", eglGetError());
    }

    m_eglSurface = EGL_NO_SURFACE;
}

EGLSurface QQnxEglWindow::surface() const
{
    return m_eglSurface;
}

void QQnxEglWindow::setGeometry(const QRect &rect)
{
    //If this is the root window, it has to be shown fullscreen
    const QRect &newGeometry = shouldMakeFullScreen() ? screen()->geometry() : rect;

    //We need to request that the GL context updates
    // the EGLsurface on which it is rendering.
    {
        // We want the setting of the atomic bool in the GL context to be atomic with
        // setting m_requestedBufferSize and therefore extended the scope to include
        // that test.
        const QMutexLocker locker(&m_mutex);
        m_requestedBufferSize = newGeometry.size();
        if (isInitialized() && bufferSize() != newGeometry.size())
            m_newSurfaceRequested.testAndSetRelease(false, true);
    }
    QQnxWindow::setGeometry(newGeometry);
}

int QQnxEglWindow::pixelFormat() const
{
    // Extract size of color channels from window format
    const int redSize = m_format.redBufferSize();
    if (Q_UNLIKELY(redSize == -1))
        qFatal("QQnxWindow: red size not defined");

    const int greenSize = m_format.greenBufferSize();
    if (Q_UNLIKELY(greenSize == -1))
        qFatal("QQnxWindow: green size not defined");

    const int blueSize = m_format.blueBufferSize();
    if (Q_UNLIKELY(blueSize == -1))
        qFatal("QQnxWindow: blue size not defined");

    // select matching native format
    if (redSize == 5 && greenSize == 6 && blueSize == 5)
        return SCREEN_FORMAT_RGB565;
    else if (redSize == 8 && greenSize == 8 && blueSize == 8)
        return SCREEN_FORMAT_RGBA8888;

    qFatal("QQnxWindow: unsupported pixel format");
}

void QQnxEglWindow::resetBuffers()
{
    m_requestedBufferSize = QSize();
}

QT_END_NAMESPACE
