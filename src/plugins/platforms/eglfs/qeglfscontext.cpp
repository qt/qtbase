/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
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
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/QSurface>
#include <QtPlatformSupport/private/qeglconvenience_p.h>
#include <QtPlatformSupport/private/qeglpbuffer_p.h>

#include "qeglfscontext.h"
#include "qeglfswindow.h"
#include "qeglfshooks.h"
#include "qeglfscursor.h"

QT_BEGIN_NAMESPACE

QEglFSContext::QEglFSContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display,
                             EGLConfig *config, const QVariant &nativeHandle)
    : QEGLPlatformContext(format, share, display, config, nativeHandle,
                          qt_egl_device_integration()->supportsSurfacelessContexts() ? Flags(0) : QEGLPlatformContext::NoSurfaceless),
      m_tempWindow(0)
{
}

EGLSurface QEglFSContext::eglSurfaceForPlatformSurface(QPlatformSurface *surface)
{
    if (surface->surface()->surfaceClass() == QSurface::Window)
        return static_cast<QEglFSWindow *>(surface)->surface();
    else
        return static_cast<QEGLPbuffer *>(surface)->pbuffer();
}

EGLSurface QEglFSContext::createTemporaryOffscreenSurface()
{
    if (qt_egl_device_integration()->supportsPBuffers())
        return QEGLPlatformContext::createTemporaryOffscreenSurface();

    if (!m_tempWindow) {
        m_tempWindow = qt_egl_device_integration()->createNativeOffscreenWindow(format());
        if (!m_tempWindow) {
            qWarning("QEglFSContext: Failed to create temporary native window");
            return EGL_NO_SURFACE;
        }
    }
    EGLConfig config = q_configFromGLFormat(eglDisplay(), format());
    return eglCreateWindowSurface(eglDisplay(), config, m_tempWindow, 0);
}

void QEglFSContext::destroyTemporaryOffscreenSurface(EGLSurface surface)
{
    if (qt_egl_device_integration()->supportsPBuffers()) {
        QEGLPlatformContext::destroyTemporaryOffscreenSurface(surface);
    } else {
        eglDestroySurface(eglDisplay(), surface);
        qt_egl_device_integration()->destroyNativeWindow(m_tempWindow);
        m_tempWindow = 0;
    }
}

void QEglFSContext::runGLChecks()
{
    // Note that even though there is an EGL context current here,
    // QOpenGLContext and QOpenGLFunctions are not yet usable at this stage.
    const char *renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    // Be nice and warn about a common source of confusion.
    if (renderer && strstr(renderer, "llvmpipe"))
        qWarning("Running on a software rasterizer (LLVMpipe), expect limited performance.");
}

void QEglFSContext::swapBuffers(QPlatformSurface *surface)
{
    // draw the cursor
    if (surface->surface()->surfaceClass() == QSurface::Window) {
        QPlatformWindow *window = static_cast<QPlatformWindow *>(surface);
        if (QEglFSCursor *cursor = qobject_cast<QEglFSCursor *>(window->screen()->cursor()))
            cursor->paintOnScreen();
    }

    qt_egl_device_integration()->waitForVSync(surface);
    QEGLPlatformContext::swapBuffers(surface);
    qt_egl_device_integration()->presentBuffer(surface);
}

QT_END_NAMESPACE
