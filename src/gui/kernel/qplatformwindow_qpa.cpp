/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qplatformwindow_qpa.h"

#include <QtGui/qwindowsysteminterface_qpa.h>
#include <QtGui/qwindow.h>
#include <QtGui/qscreen.h>

QT_BEGIN_NAMESPACE

class QPlatformWindowPrivate
{
    QWindow *window;
    QRect rect;
    friend class QPlatformWindow;
};

/*!
    Constructs a platform window with the given top level window.
*/

QPlatformWindow::QPlatformWindow(QWindow *window)
    : QPlatformSurface(QSurface::Window)
    , d_ptr(new QPlatformWindowPrivate)
{
    Q_D(QPlatformWindow);
    d->window = window;
    d->rect = window->geometry();
}

/*!
    Virtual destructor does not delete its top level window.
*/
QPlatformWindow::~QPlatformWindow()
{
}

/*!
    Returns the window which belongs to the QPlatformWindow
*/
QWindow *QPlatformWindow::window() const
{
    Q_D(const QPlatformWindow);
    return d->window;
}

/*!
    Returns the parent platform window (or 0 if orphan).
*/
QPlatformWindow *QPlatformWindow::parent() const
{
    Q_D(const QPlatformWindow);
    return d->window->parent() ? d->window->parent()->handle() : 0;
}

/*!
    Returns the platform screen handle corresponding to this platform window.
*/
QPlatformScreen *QPlatformWindow::screen() const
{
    Q_D(const QPlatformWindow);
    return d->window->screen()->handle();
}

/*!
    Returns the actual surface format of the window.
*/
QSurfaceFormat QPlatformWindow::format() const
{
    return QSurfaceFormat();
}

/*!
    This function is called by Qt whenever a window is moved or the window is resized. The resize
    can happen programatically(from ie. user application) or by the window manager. This means that
    there is no need to call this function specifically from the window manager callback, instead
    call QWindowSystemInterface::handleGeometryChange(QWindow *w, const QRect &newRect);

    The position(x, y) part of the rect might be inclusive or exclusive of the window frame
    as returned by frameMargins(). You can detect this in the plugin by checking
    qt_window_private(window())->positionPolicy.
*/
void QPlatformWindow::setGeometry(const QRect &rect)
{
    Q_D(QPlatformWindow);
    d->rect = rect;
}

/*!
    Returnes the current geometry of a window
*/
QRect QPlatformWindow::geometry() const
{
    Q_D(const QPlatformWindow);
    return d->rect;
}

QMargins QPlatformWindow::frameMargins() const
{
    return QMargins();
}

/*!
    Reimplemented in subclasses to show the surface
    if \a visible is \c true, and hide it if \a visible is \c false.

    The default implementation sends a synchronous expose event.
*/
void QPlatformWindow::setVisible(bool visible)
{
    QRect rect(QPoint(), geometry().size());
    QWindowSystemInterface::handleSynchronousExposeEvent(window(), rect);
}
/*!
    Requests setting the window flags of this surface
    to \a type. Returns the actual flags set.
*/
Qt::WindowFlags QPlatformWindow::setWindowFlags(Qt::WindowFlags flags)
{
    return flags;
}



/*!
    Returns if this window is exposed in the windowing system.

    An exposeEvent() is sent every time this value changes.
 */

bool QPlatformWindow::isExposed() const
{
    Q_D(const QPlatformWindow);
    return d->window->isVisible();
}

/*!
    Requests setting the window state of this surface
    to \a type. Returns the actual state set.

    Qt::WindowActive can be ignored.
*/
Qt::WindowState QPlatformWindow::setWindowState(Qt::WindowState)
{
    return Qt::WindowNoState;
}

/*!
  Reimplement in subclasses to return a handle to the native window
*/
WId QPlatformWindow::winId() const
{
    // Return anything but 0. Returning 0 would cause havoc with QWidgets on
    // very basic platform plugins that do not reimplement this function,
    // because the top-level widget's internalWinId() would always be 0 which
    // would mean top-levels are never treated as native.
    return WId(1);
}

/*!
    This function is called to enable native child window in QPA. It is common not to support this
    feature in Window systems, but can be faked. When this function is called all geometry of this
    platform window will be relative to the parent.
*/
//jl: It would be useful to have a property on the platform window which indicated if the sub-class
// supported the setParent. If not, then geometry would be in screen coordinates.
void QPlatformWindow::setParent(const QPlatformWindow *parent)
{
    Q_UNUSED(parent);
    qWarning("This plugin does not support setParent!");
}

/*!
  Reimplement to set the window title to \a title
*/
void QPlatformWindow::setWindowTitle(const QString &title) { Q_UNUSED(title); }

/*!
  Reimplement to be able to let Qt raise windows to the top of the desktop
*/
void QPlatformWindow::raise() { qWarning("This plugin does not support raise()"); }

/*!
  Reimplement to be able to let Qt lower windows to the bottom of the desktop
*/
void QPlatformWindow::lower() { qWarning("This plugin does not support lower()"); }

/*!
  Reimplement to propagate the size hints of the QWindow.

  The size hints include QWindow::minimumSize(), QWindow::maximumSize(),
  QWindow::sizeIncrement(), and QWindow::baseSize().
*/
void QPlatformWindow::propagateSizeHints() {qWarning("This plugin does not support propagateSizeHints()"); }

/*!
  Reimplement to be able to let Qt set the opacity level of a window
*/
void QPlatformWindow::setOpacity(qreal level)
{
    Q_UNUSED(level);
    qWarning("This plugin does not support setting window opacity");
}

/*!
  Reimplement to let Qt be able to request activation/focus for a window

  Some window systems will probably not have callbacks for this functionality,
  and then calling QWindowSystemInterface::handleWindowActivated(QWindow *w)
  would be sufficient.

  If the window system has some event handling/callbacks then call
  QWindowSystemInterface::handleWindowActivated(QWindow *w) when the window system
  gives the notification.

  Default implementation calls QWindowSystem::handleWindowActivated(QWindow *w)
*/
void QPlatformWindow::requestActivateWindow()
{
    QWindowSystemInterface::handleWindowActivated(window());
}

/*!
  Handle changes to the orientation of the platform window's contents.

  This is a hint to the window manager in case it needs to display
  additional content like popups, dialogs, status bars, or similar
  in relation to the window.

  \sa QWindow::reportContentOrientationChange()
*/
void QPlatformWindow::handleContentOrientationChange(Qt::ScreenOrientation orientation)
{
    Q_UNUSED(orientation);
}

/*!
  Request a different orientation of the platform window.

  This tells the window manager how the window wants to be rotated in order
  to be displayed, and how input events should be translated.

  As an example, a portrait compositor might rotate the window by 90 degrees,
  if the window is in landscape. It will also rotate input coordinates from
  portrait to landscape such that top right in portrait gets mapped to top
  left in landscape.

  If the implementation doesn't support the requested orientation it should
  signal this by returning an actual supported orientation.

  If the implementation doesn't support rotating the window at all it should
  return Qt::PrimaryOrientation, this is also the default value.

  \sa QWindow::requestWindowOrientation()
*/
Qt::ScreenOrientation QPlatformWindow::requestWindowOrientation(Qt::ScreenOrientation orientation)
{
    Q_UNUSED(orientation);
    return Qt::PrimaryOrientation;
}

bool QPlatformWindow::setKeyboardGrabEnabled(bool grab)
{
    Q_UNUSED(grab);
    qWarning("This plugin does not support grabbing the keyboard");
    return false;
}

bool QPlatformWindow::setMouseGrabEnabled(bool grab)
{
    Q_UNUSED(grab);
    qWarning("This plugin does not support grabbing the mouse");
    return false;
}

/*!
    \class QPlatformWindow
    \since 4.8
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformWindow class provides an abstraction for top-level windows.

    The QPlatformWindow abstraction is used by QWindow for all its top level windows. It is being
    created by calling the createPlatformWindow function in the loaded QPlatformIntegration
    instance.

    QPlatformWindow is used to signal to the windowing system, how Qt perceives its frame.
    However, it is not concerned with how Qt renders into the window it represents.

    Visible QWindows will always have a QPlatformWindow. However, it is not necessary for
    all windows to have a QWindowSurface. This is the case for QOpenGLWidget. And could be the case for
    windows where some  3.party renders into it.

    The platform specific window handle can be retrieved by the winId function.

    QPlatformWindow is also the way QPA defines how native child windows should be supported
    through the setParent function.

    The only way to retrieve a QPlatformOpenGLContext in QPA is by calling the glContext() function
    on QPlatformWindow.

    \sa QWindowSurface, QWindow
*/

QT_END_NAMESPACE
