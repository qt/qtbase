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

#include "qxcbeglintegration.h"

#include "qxcbeglcontext.h"

#include <QtGui/QOffscreenSurface>

#include "qxcbeglnativeinterfacehandler.h"

QT_BEGIN_NAMESPACE

QXcbEglIntegration::QXcbEglIntegration()
    : m_connection(Q_NULLPTR)
    , m_egl_display(EGL_NO_DISPLAY)
{
    qCDebug(QT_XCB_GLINTEGRATION) << "Xcb EGL gl-integration created";
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
        qCDebug(QT_XCB_GLINTEGRATION) << "Xcb EGL gl-integration retrying with display" << m_egl_display;
        success = eglInitialize(m_egl_display, &major, &minor);
    }

    m_native_interface_handler.reset(new QXcbEglNativeInterfaceHandler(connection->nativeInterface()));

    qCDebug(QT_XCB_GLINTEGRATION) << "Xcb EGL gl-integration successfully initialized";
    return success;
}

QXcbWindow *QXcbEglIntegration::createWindow(QWindow *window) const
{
    return new QXcbEglWindow(window, const_cast<QXcbEglIntegration *>(this));
}

QPlatformOpenGLContext *QXcbEglIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QXcbScreen *screen = static_cast<QXcbScreen *>(context->screen()->handle());
    QXcbEglContext *platformContext = new QXcbEglContext(context->format(),
                                                         context->shareHandle(),
                                                         eglDisplay(),
                                                         screen->connection(),
                                                         context->nativeHandle());
    context->setNativeHandle(platformContext->nativeHandle());
    return platformContext;
}

QPlatformOffscreenSurface *QXcbEglIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    return new QEGLPbuffer(eglDisplay(), surface->requestedFormat(), surface);
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
