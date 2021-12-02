/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <QtGui/private/qtguiglobal_p.h>

#include <QtGui/private/qopenglcontext_p.h>
#include <QtGui/private/qguiapplication_p.h>

#include <qpa/qplatformopenglcontext.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformscreen_p.h>
#include <qpa/qplatformwindow_p.h>

#include <QtGui/private/qkeymapper_p.h>

QT_BEGIN_NAMESPACE

using namespace QNativeInterface::Private;

#ifndef QT_NO_OPENGL

#if QT_CONFIG(xcb_glx_plugin)

/*!
    \class QNativeInterface::QGLXContext
    \since 6.0
    \brief Native interface to a GLX context.

    Accessed through QOpenGLContext::nativeInterface().

    \inmodule QtGui
    \inheaderfile QOpenGLContext
    \ingroup native-interfaces
    \ingroup native-interfaces-qopenglcontext
*/

/*!
    \fn QOpenGLContext *QNativeInterface::QGLXContext::fromNative(GLXContext configBasedContext, QOpenGLContext *shareContext = nullptr)

    \brief Adopts a GLXContext \a configBasedContext created from an FBConfig.

    The context must be created from a framebuffer configuration, using the \c glXCreateNewContext function.

    Ownership of the created QOpenGLContext \a shareContext is transferred to the caller.
*/

/*!
    \fn QOpenGLContext *QNativeInterface::QGLXContext::fromNative(GLXContext visualBasedContext, void *visualInfo, QOpenGLContext *shareContext = nullptr)

    \brief Adopts a GLXContext created from an X visual.

    The context must be created from a visual, using the \c glXCreateContext function.
    The same visual must be passed as a pointer to an \c XVisualInfo struct, in the \a visualInfo argument.

    Ownership of the created QOpenGLContext is transferred to the caller.
*/

/*!
    \fn GLXContext QNativeInterface::QGLXContext::nativeContext() const

    \return the underlying GLXContext.
*/

QT_DEFINE_NATIVE_INTERFACE(QGLXContext);
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QGLXIntegration);

QOpenGLContext *QNativeInterface::QGLXContext::fromNative(GLXContext configBasedContext, QOpenGLContext *shareContext)
{
    return QGuiApplicationPrivate::platformIntegration()->call<
        &QGLXIntegration::createOpenGLContext>(configBasedContext, nullptr, shareContext);
}

QOpenGLContext *QNativeInterface::QGLXContext::fromNative(GLXContext visualBasedContext, void *visualInfo, QOpenGLContext *shareContext)
{
    return QGuiApplicationPrivate::platformIntegration()->call<
        &QGLXIntegration::createOpenGLContext>(visualBasedContext, visualInfo, shareContext);
}
#endif // QT_CONFIG(xcb_glx_plugin)

#if QT_CONFIG(egl)

/*!
    \class QNativeInterface::QEGLContext
    \since 6.0
    \brief Native interface to an EGL context.

    Accessed through QOpenGLContext::nativeInterface().

    \inmodule QtGui
    \inheaderfile QOpenGLContext
    \ingroup native-interfaces
    \ingroup native-interfaces-qopenglcontext
*/

/*!
    \fn QOpenGLContext *QNativeInterface::QEGLContext::fromNative(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext = nullptr)

    \brief Adopts an EGLContext \a context.

    The same \c EGLDisplay passed to \c eglCreateContext must be passed as the \a display argument.

    Ownership of the created QOpenGLContext \a shareContext is transferred
    to the caller.
*/

/*!
    \fn EGLContext QNativeInterface::QEGLContext::nativeContext() const

    \return the underlying EGLContext.
*/

/*!
    \fn EGLConfig QNativeInterface::QEGLContext::config() const
    \since 6.3
    \return the EGLConfig associated with the underlying EGLContext.
*/

/*!
    \fn EGLDisplay QNativeInterface::QEGLContext::display() const
    \since 6.3
    \return the EGLDisplay associated with the underlying EGLContext.
*/

QT_DEFINE_NATIVE_INTERFACE(QEGLContext);
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QEGLIntegration);

QOpenGLContext *QNativeInterface::QEGLContext::fromNative(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext)
{
    return QGuiApplicationPrivate::platformIntegration()->call<
        &QEGLIntegration::createOpenGLContext>(context, display, shareContext);
}
#endif // QT_CONFIG(egl)

#endif // QT_NO_OPENGL

#if QT_CONFIG(xcb)

/*!
    \class QNativeInterface::Private::QXcbScreen
    \since 6.0
    \internal
    \brief Native interface to QPlatformScreen.
    \inmodule QtGui
    \ingroup native-interfaces
*/

QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QXcbScreen);

/*!
    \class QNativeInterface::Private::QXcbWindow
    \since 6.0
    \internal
    \brief Native interface to QPlatformWindow.
    \inmodule QtGui
    \ingroup native-interfaces
*/

QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QXcbWindow);

/*!
    \class QNativeInterface::QX11Application
    \since 6.2
    \brief Native interface to an X11 application.

    Accessed through QGuiApplication::nativeInterface().

    \inmodule QtGui
    \inheaderfile QGuiApplication
    \ingroup native-interfaces
    \ingroup native-interfaces-qguiapplication
*/

/*!
    \fn Display *QNativeInterface::QX11Application::display() const

    \return the X display of the application, for use with Xlib.

    \sa connection()
*/

/*!
    \fn xcb_connection_t *QNativeInterface::QX11Application::connection() const

    \return the X connection of the application, for use with XCB.

    \sa display()
*/

QT_DEFINE_NATIVE_INTERFACE(QX11Application);

#endif // QT_CONFIG(xcb)

#if QT_CONFIG(vsp2)
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QVsp2Screen);
#endif

#ifdef Q_OS_WEBOS
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QWebOSScreen);
#endif

#if QT_CONFIG(evdev)

/*!
    \class QNativeInterface::Private::QEvdevKeyMapper
    \since 6.0
    \internal
    \brief Native interface to QKeyMapper.
    \inmodule QtGui
    \ingroup native-interfaces
*/

QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QEvdevKeyMapper);

#endif // QT_CONFIG(evdev)

QT_END_NAMESPACE
