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

#include "qxcbeglintegration.h"

#include "qxcbeglcontext.h"

#include <QtGui/QOffscreenSurface>

#include "qxcbeglnativeinterfacehandler.h"

QT_BEGIN_NAMESPACE

QXcbEglIntegration::QXcbEglIntegration()
    : m_connection(Q_NULLPTR)
    , m_egl_display(EGL_NO_DISPLAY)
{
    qCDebug(lcQpaGl) << "Xcb EGL gl-integration created";
}

QXcbEglIntegration::~QXcbEglIntegration()
{
    if (m_egl_display != EGL_NO_DISPLAY)
        eglTerminate(m_egl_display);
}

bool QXcbEglIntegration::initialize(QXcbConnection *connection)
{
    m_connection = connection;
    m_egl_display = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(xlib_display()));

    EGLint major, minor;
    bool success = eglInitialize(m_egl_display, &major, &minor);
    if (!success) {
        m_egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        qCDebug(lcQpaGl) << "Xcb EGL gl-integration retrying with display" << m_egl_display;
        success = eglInitialize(m_egl_display, &major, &minor);
    }

    m_native_interface_handler.reset(new QXcbEglNativeInterfaceHandler(connection->nativeInterface()));

    qCDebug(lcQpaGl) << "Xcb EGL gl-integration successfully initialized";
    return success;
}

QXcbWindow *QXcbEglIntegration::createWindow(QWindow *window) const
{
    return new QXcbEglWindow(window, const_cast<QXcbEglIntegration *>(this));
}

QPlatformOpenGLContext *QXcbEglIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QXcbScreen *screen = static_cast<QXcbScreen *>(context->screen()->handle());
    QXcbEglContext *platformContext = new QXcbEglContext(screen->surfaceFormatFor(context->format()),
                                                         context->shareHandle(),
                                                         eglDisplay(),
                                                         screen->connection(),
                                                         context->nativeHandle());
    context->setNativeHandle(platformContext->nativeHandle());
    return platformContext;
}

QPlatformOffscreenSurface *QXcbEglIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    QXcbScreen *screen = static_cast<QXcbScreen *>(surface->screen()->handle());
    return new QEGLPbuffer(eglDisplay(), screen->surfaceFormatFor(surface->requestedFormat()), surface);
}

void *QXcbEglIntegration::xlib_display() const
{
#ifdef XCB_USE_XLIB
    return m_connection->xlib_display();
#else
    return EGL_DEFAULT_DISPLAY;
#endif
}

QT_END_NAMESPACE
