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

#include "qopenwfdglcontext.h"

#include "qopenwfdwindow.h"
#include "qopenwfdscreen.h"
#include <dlfcn.h>

QOpenWFDGLContext::QOpenWFDGLContext(QOpenWFDDevice *device)
    : QPlatformOpenGLContext()
    , mWfdDevice(device)
{
}

QSurfaceFormat QOpenWFDGLContext::format() const
{
    return QSurfaceFormat();
}

bool QOpenWFDGLContext::makeCurrent(QPlatformSurface *surface)
{
    Q_ASSERT(surface->surface()->surfaceType() == QSurface::OpenGLSurface);

    EGLDisplay display = mWfdDevice->eglDisplay();
    EGLContext context = mWfdDevice->eglContext();
    if (!eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,context)) {
        qDebug("GLContext: eglMakeCurrent FAILED!");
    }

    QPlatformWindow *window = static_cast<QPlatformWindow *>(surface);
    QOpenWFDScreen *screen = static_cast<QOpenWFDScreen *>(QPlatformScreen::platformScreenForWindow(window->window()));
    screen->bindFramebuffer();
    return true;
}

void QOpenWFDGLContext::doneCurrent()
{
    //do nothing :)
}

void QOpenWFDGLContext::swapBuffers(QPlatformSurface *surface)
{
    glFlush();

    QPlatformWindow *window = static_cast<QPlatformWindow *>(surface);
    QOpenWFDScreen *screen = static_cast<QOpenWFDScreen *>(QPlatformScreen::platformScreenForWindow(window->window()));

    screen->swapBuffers();
}

QFunctionPointer QOpenWFDGLContext::getProcAddress(const char *procName)
{
    QFunctionPointer proc = (QFunctionPointer) eglGetProcAddress(procName);
    if (!proc)
        proc = (QFunctionPointer) dlsym(RTLD_DEFAULT, procName);
    return proc;
}

EGLContext QOpenWFDGLContext::eglContext() const
{
    return mWfdDevice->eglContext();
}


