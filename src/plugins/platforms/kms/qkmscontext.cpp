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

#include "qkmsscreen.h"
#include "qkmsdevice.h"
#include "qkmscontext.h"
#include "qkmswindow.h"


QT_BEGIN_NAMESPACE

QKmsContext::QKmsContext(QKmsDevice *device)
    : QPlatformOpenGLContext(),
      m_device(device)
{
}

bool QKmsContext::makeCurrent(QPlatformSurface *surface)
{
    EGLDisplay display = m_device->eglDisplay();
    EGLContext context = m_device->eglContext();

    bool ok = eglMakeCurrent(display, EGL_NO_SURFACE,
                             EGL_NO_SURFACE, context);
    if (!ok)
        qWarning("QKmsContext::makeCurrent(): eglError: %d, this: %p",
                 eglGetError(), this);

    QPlatformWindow *window = static_cast<QPlatformWindow *>(surface);
    QKmsScreen *screen = static_cast<QKmsScreen *> (QPlatformScreen::platformScreenForWindow(window->window()));
    screen->bindFramebuffer();
    return true;
}

void QKmsContext::doneCurrent()
{
    bool ok = eglMakeCurrent(m_device->eglDisplay(), EGL_NO_SURFACE, EGL_NO_SURFACE,
                             EGL_NO_CONTEXT);
    if (!ok)
        qWarning("QKmsContext::doneCurrent(): eglError: %d, this: %p",
                 eglGetError(), this);

}

void QKmsContext::swapBuffers(QPlatformSurface *surface)
{
    //After flush, the current render target should be moved to
    //latest complete
    glFlush();

    //Cast context to a window surface and get the screen the context
    //is on and call swapBuffers on that screen.
    QPlatformWindow *window = static_cast<QPlatformWindow *>(surface);
    QKmsScreen *screen = static_cast<QKmsScreen *> (QPlatformScreen::platformScreenForWindow(window->window()));
    screen->swapBuffers();
}

void (*QKmsContext::getProcAddress(const QByteArray &procName)) ()
{
    return eglGetProcAddress(procName.data());
}


EGLContext QKmsContext::eglContext() const
{
    return m_device->eglContext();
}

QSurfaceFormat QKmsContext::format() const
{
    return QSurfaceFormat();
}

GLuint QKmsContext::defaultFramebufferObject(QPlatformSurface *surface) const
{
    QPlatformWindow *window = static_cast<QPlatformWindow *>(surface);
    QKmsScreen *screen = static_cast<QKmsScreen *> (QPlatformScreen::platformScreenForWindow(window->window()));
    return screen->framebufferObject();
}

QT_END_NAMESPACE
