// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qopenglcontext.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformopenglcontext.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformwindow.h>
#include <qpa/qplatformwindow_p.h>
#include <qpa/qplatformscreen_p.h>

QT_BEGIN_NAMESPACE

using namespace QNativeInterface::Private;

#ifndef QT_NO_OPENGL

/*!
    \class QNativeInterface::QWGLContext
    \inheaderfile QOpenGLContext
    \since 6.0
    \brief Native interface to a WGL context on Windows.

    Accessed through QOpenGLContext::nativeInterface().

    \inmodule QtGui
    \inheaderfile QOpenGLContext
    \ingroup native-interfaces
    \ingroup native-interfaces-qopenglcontext
*/

/*!
    \fn QOpenGLContext *QNativeInterface::QWGLContext::fromNative(HGLRC context, HWND window, QOpenGLContext *shareContext = nullptr)

    \brief Adopts an WGL \a context handle.

    The \a window is needed because the its pixel format will be queried. When the
    adoption is successful, QOpenGLContext::format() will return a QSurfaceFormat
    describing this pixel format.

    \note The window specified by \a window must have its pixel format set to a
    format compatible with the context's. If no SetPixelFormat() call was made on
    any device context belonging to the window, adopting the context will fail.

    Ownership of the created QOpenGLContext \a shareContext is transferred to the
    caller.
*/

/*!
    \fn HGLRC QNativeInterface::QWGLContext::nativeContext() const

    \return the underlying context handle.
*/

/*!
    \fn HMODULE QNativeInterface::QWGLContext::openGLModuleHandle()

    \return the handle for the OpenGL implementation that is currently in use.

    \note This function requires that the QGuiApplication instance is already created.
*/

QT_DEFINE_NATIVE_INTERFACE(QWGLContext);
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QWindowsGLIntegration);

HMODULE QNativeInterface::QWGLContext::openGLModuleHandle()
{
    return QGuiApplicationPrivate::platformIntegration()->call<
        &QWindowsGLIntegration::openGLModuleHandle>();
}

QOpenGLContext *QNativeInterface::QWGLContext::fromNative(HGLRC context, HWND window, QOpenGLContext *shareContext)
{
    return QGuiApplicationPrivate::platformIntegration()->call<
        &QWindowsGLIntegration::createOpenGLContext>(context, window, shareContext);
}

#endif // QT_NO_OPENGL

/*!
    \class QNativeInterface::Private::QWindowsApplication
    \since 6.0
    \internal
    \brief Native interface to QGuiApplication, to be retrieved from QPlatformIntegration.
    \inmodule QtGui
    \ingroup native-interfaces
*/

QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QWindowsApplication);

/*!
    \class QNativeInterface::QWindowsScreen
    \since 6.7
    \brief Native interface to a screen.

    Accessed through QScreen::nativeInterface().
    \inmodule QtGui
    \ingroup native-interfaces
    \ingroup native-interfaces-qscreen
*/
/*!
 * \fn HWMONITOR QNativeInterface::QWindowsScreen::handle() const;
 * \return The underlying HWMONITOR of the screen.
 */
QT_DEFINE_NATIVE_INTERFACE(QWindowsScreen);
/*!
    \enum QNativeInterface::Private::QWindowsApplication::TouchWindowTouchType

    This enum represents the supported TouchWindow touch flags for registerTouchWindow().

    \value NormalTouch
    \value FineTouch
    \value WantPalmTouch
*/

/*!
    \fn void QNativeInterface::Private::QWindowsApplication::setTouchWindowTouchType(QNativeInterface::Private::QWindowsApplication::TouchWindowTouchTypes type)
    \internal

    Sets the touch window type for all windows to \a type.
*/

/*!
    \fn QNativeInterface::Private::QWindowsApplication::TouchWindowTouchTypes QNativeInterface::Private::QWindowsApplication::touchWindowTouchType() const
    \internal

    Returns the currently set the touch window type.
*/

/*!
    \enum QNativeInterface::Private::QWindowsApplication::WindowActivationBehavior

    This enum specifies the behavior of QWidget::activateWindow() and
    QWindow::requestActivate().

    \value DefaultActivateWindow The window is activated according to the default
        behavior of the Windows operating system. This means the window will not
        be activated in some circumstances (most notably when the calling process
        is not the active process); only the taskbar entry will be flashed.
    \value AlwaysActivateWindow  The window is always activated, even when the
        calling process is not the active process.

    \sa QWidget::activateWindow(), QWindow::requestActivate()
*/

/*!
    \fn void QNativeInterface::Private::QWindowsApplication::setWindowActivationBehavior(QNativeInterface::Private::QWindowsApplication::WindowActivationBehavior behavior)
    \internal

    Sets the window activation behavior to \a behavior.

    \sa QWidget::activateWindow(), QWindow::requestActivate()
*/

/*!
    \fn QNativeInterface::Private::QWindowsApplication::WindowActivationBehavior QNativeInterface::Private::QWindowsApplication::windowActivationBehavior() const
    \internal

    Returns the currently set the window activation behavior.
*/

/*!
    \fn bool QNativeInterface::Private::QWindowsApplication::isTabletMode() const
    \internal

    Returns \c true if Windows 10 operates in \e{Tablet Mode}.
    In this mode, Windows forces all application main windows to open in maximized
    state. Applications should then avoid resizing windows or restoring geometries
    to non-maximized states.

    \sa QWidget::showMaximized(), QWidget::saveGeometry(), QWidget::restoreGeometry()
*/

/*!
    \enum QNativeInterface::Private::QWindowsApplication::DarkModeHandlingFlag

    This enum specifies the behavior of the application when Windows
    is configured to use dark mode for applications.

    \value DarkModeWindowFrames The window frames will be switched to dark.
    \value DarkModeStyle        The Windows Vista style will be turned off and
                                a simple dark style will be used.

    \sa setDarkModeHandling()
*/

/*!
    \fn void QNativeInterface::Private::QWindowsApplication::setDarkModeHandling(DarkModeHandling handling) = 0
    \internal

    Sets the dark mode handling to \a handling.
*/

/*!
    \fn QNativeInterface::Private::QWindowsApplication::DarkModeHandling QNativeInterface::Private::QWindowsApplication::darkModeHandling() const
    \internal

    Returns the currently set dark mode handling.
*/

/*!
    \fn bool QNativeInterface::Private::QWindowsApplication::isWinTabEnabled() const = 0
    \internal

    Returns whether the \e{Tablet WinTab Driver} (\c Wintab32.dll) is used.
*/

/*!
    \fn bool QNativeInterface::Private::QWindowsApplication::setWinTabEnabled(bool enabled)
    \internal

    Sets whether the \e{Tablet WinTab Driver} (\c Wintab32.dll) should be used to \a enabled.

    Returns \c true on success, \c false otherwise.
*/

/*!
    \fn bool QNativeInterface::Private::QWindowsApplication::registerMime(QWindowsMimeConverter *mime)
    \internal

    Registers the converter \a mime to the system.

    \sa QWindowsMimeConverter, unregisterMime()
*/

/*!
    \fn void QNativeInterface::Private::QWindowsApplication::unregisterMime(QWindowsMimeConverter *mime)
    \internal

    Unregisters the converter \a mime from the system.

    \sa QWindowsMimeConverter, registerMime()
*/

/*!
    \fn int QNativeInterface::Private::QWindowsApplication::registerMimeType(const QString &mime)
    \internal

    Registers the MIME type \a mime, and returns an ID number
    identifying the format on Windows.
*/

/*!
    \fn HWND QNativeInterface::Private::QWindowsApplication::createMessageWindow(const QString &, const QString &, QFunctionPointer) const
    \internal
*/

/*!
    \fn bool QNativeInterface::Private::QWindowsApplication::asyncExpose() const
    \internal
*/

/*!
    \fn void QNativeInterface::Private::QWindowsApplication::setAsyncExpose(bool)
    \internal
*/

/*!
    \fn QVariant QNativeInterface::Private::QWindowsApplication::gpu()
    \internal
*/

/*!
    \fn QVariant QNativeInterface::Private::QWindowsApplication::gpuList()
    \internal
*/

/*!
    \class QNativeInterface::Private::QWindowsWindow
    \since 6.0
    \internal
    \brief Native interface to QPlatformWindow.
    \inmodule QtGui
    \ingroup native-interfaces
*/

QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QWindowsWindow);

/*!
    \fn void QNativeInterface::Private::QWindowsWindow::setHasBorderInFullScreen(bool border)
    \internal

    Sets whether the WS_BORDER flag will be set for the window in full screen mode
    to \a border.

    See also \l [QtDoc] {Fullscreen OpenGL Based Windows}
*/

/*!
    \fn bool QNativeInterface::Private::QWindowsWindow::hasBorderInFullScreen() const
    \internal

    Returns whether the WS_BORDER flag will be set for the window in full screen
    mode.
*/

/*!
    \fn QMargins QNativeInterface::Private::QWindowsWindow::customMargins() const
    \internal

    Returns the margin to be used when handling the \c WM_NCCALCSIZE message.
*/

/*!
    \fn void QNativeInterface::Private::QWindowsWindow::setCustomMargins(const QMargins &margins)
    \internal

    Sets the\a  margins to be used when handling the \c WM_NCCALCSIZE message. It is
    possible to remove a frame border by specifying a negative value.
*/

QT_END_NAMESPACE
