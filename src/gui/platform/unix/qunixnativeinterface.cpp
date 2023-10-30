// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/private/qtguiglobal_p.h>

#if QT_CONFIG(opengl)
#  include <QtGui/private/qopenglcontext_p.h>
#endif
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


/*!
    \fn void QNativeInterface::QEGLContext::invalidateContext()
    \since 6.5
    \brief Marks the context as invalid

    If this context is used by the Qt Quick scenegraph, this will trigger the
    SceneGraph to destroy this context and create a new one.

    Similarly to QPlatformWindow::invalidateSurface(),
    this function can only be expected to have an effect on certain platforms,
    such as eglfs.

    \sa QOpenGLContext::isValid(), QPlatformWindow::invalidateSurface()
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

#if defined(Q_OS_UNIX)

/*!
    \class QNativeInterface::QWaylandApplication
    \inheaderfile QGuiApplication
    \since 6.5
    \brief Native interface to a Wayland application.

    Accessed through QGuiApplication::nativeInterface().
    \inmodule QtGui
    \ingroup native-interfaces
    \ingroup native-interfaces-qguiapplication
*/
/*!
    \fn wl_display *QNativeInterface::QWaylandApplication::display() const
    \return the wl_display that the application is using.
*/
/*!
    \fn wl_compositor *QNativeInterface::QWaylandApplication::compositor() const
    \return the wl_compositor that the application is using.
*/
/*!
    \fn wl_keyboard *QNativeInterface::QWaylandApplication::keyboard() const
    \return the wl_keyboard belonging to seat() if available.
*/
/*!
    \fn wl_pointer *QNativeInterface::QWaylandApplication::pointer() const
    \return the wl_pointer belonging to seat() if available.
*/
/*!
    \fn wl_touch *QNativeInterface::QWaylandApplication::touch() const
    \return the wl_touch belonging to seat() if available.
*/
/*!
    \fn uint *QNativeInterface::QWaylandApplication::lastInputSerial() const
    \return the serial of the last input event on any seat.
*/
/*!
    \fn wl_seat *QNativeInterface::QWaylandApplication::lastInputSeat() const
    \return the seat on which the last input event happened.
*/
/*!
    \fn wl_seat *QNativeInterface::QWaylandApplication::seat() const
    \return the seat associated with the default input device.
*/

QT_DEFINE_NATIVE_INTERFACE(QWaylandApplication);

/*!
    \class QNativeInterface::Private::QWaylandScreen
    \since 6.5
    \internal
    \brief Native interface to QPlatformScreen.
    \inmodule QtGui
    \ingroup native-interfaces
*/

QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QWaylandScreen);

/*!
    \class QNativeInterface::QWaylandWindow
    \since 6.5
    \internal
    \brief Native interface to a Wayland window.
    \inmodule QtGui
    \ingroup native-interfaces
*/

QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QWaylandWindow);

#endif // Q_OS_UNIX

QT_END_NAMESPACE
