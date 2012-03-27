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

#include "qwindow.h"

#include "qplatformwindow_qpa.h"
#include "qplatformintegration_qpa.h"
#include "qsurfaceformat.h"
#ifndef QT_NO_OPENGL
#include "qplatformopenglcontext_qpa.h"
#include "qopenglcontext.h"
#endif
#include "qscreen.h"

#include "qwindow_p.h"
#include "qguiapplication_p.h"
#include "qaccessible.h"

#include <private/qevent_p.h>

#include <QtCore/QDebug>

#include <QStyleHints>

QT_BEGIN_NAMESPACE

/*!
    \class QWindow
    \since 5.0
    \brief The QWindow class represents a window in the underlying windowing system.

    A window that is supplied a parent becomes a native child window of
    their parent window.

    \section1 Resource management

    Windows can potentially use a lot of memory. A usual measurement is
    width times height times color depth. A window might also include multiple
    buffers to support double and triple buffering, as well as depth and stencil
    buffers. To release a window's memory resources, the destroy() function.

    \section1 Window and content orientation

    QWindow has reportContentOrientationChange() and
    requestWindowOrientation() that can be used to specify the
    layout of the window contents in relation to the screen. The
    window orientation determines the actual buffer layout of the
    window, and the windowing system uses this value to rotate the
    window before it ends up on the display, and to ensure that input
    coordinates are in the correct coordinate space relative to the
    application.

    On the other hand, the content orientation is simply a hint to the
    windowing system about which orientation the window contents are in.
    It's useful when you wish to keep the same buffer layout, but rotate
    the contents instead, especially when doing rotation animations
    between different orientations. The windowing system might use this
    value to determine the layout of system popups or dialogs.

    \section1 Visibility and Windowing system exposure.

    By default, the window is not visible, and you must call setVisible(true),
    or show() or similar to make it visible. To make a window hidden again,
    call setVisible(false) or hide(). The visible property describes the state
    the application wants the window to be in. Depending on the underlying
    system, a visible window might still not be shown on the screen. It could,
    for instance, be covered by other opaque windows or moved outside the
    physical area of the screen. On windowing systems that have exposure
    notifications, the isExposed() accessor describes whether the window should
    be treated as directly visible on screen. The exposeEvent() function is
    called whenever the windows exposure in the windowing system changes.  On
    windowing systems that do not make this information visible to the
    application, isExposed() will simply return the same value as isVisible().

    \section1 Rendering

    There are two Qt APIs that can be used to render content into a window,
    QBackingStore for rendering with a QPainter and flushing the contents
    to a window with type QSurface::RasterSurface, and QOpenGLContext for
    rendering with OpenGL to a window with type QSurface::OpenGLSurface.

    The application can start rendering as soon as isExposed() returns true,
    and can keep rendering until it isExposed() returns false. To find out when
    isExposed() changes, reimplement exposeEvent(). The window will always get
    a resize event before the first expose event.
*/

/*!
    Creates a window as a top level on the given screen.

    The window is not shown until setVisible(true), show(), or similar is called.

    \sa setScreen()
*/
QWindow::QWindow(QScreen *targetScreen)
    : QObject(*new QWindowPrivate(), 0)
    , QSurface(QSurface::Window)
{
    Q_D(QWindow);
    d->screen = targetScreen;
    if (!d->screen)
        d->screen = QGuiApplication::primaryScreen();

    //if your applications aborts here, then chances are your creating a QWindow before the
    //screen list is populated.
    Q_ASSERT(d->screen);

    connect(d->screen, SIGNAL(destroyed(QObject *)), this, SLOT(screenDestroyed(QObject *)));
    QGuiApplicationPrivate::window_list.prepend(this);
}

/*!
    Creates a window as a child of the given \a parent window.

    The window will be embedded inside the parent window, its coordinates relative to the parent.

    The screen is inherited from the parent.

    \sa setParent()
*/
QWindow::QWindow(QWindow *parent)
    : QObject(*new QWindowPrivate(), parent)
    , QSurface(QSurface::Window)
{
    Q_D(QWindow);
    d->parentWindow = parent;
    if (parent)
        d->screen = parent->screen();
    if (!d->screen)
        d->screen = QGuiApplication::primaryScreen();
    QGuiApplicationPrivate::window_list.prepend(this);
}

QWindow::QWindow(QWindowPrivate &dd, QWindow *parent)
    : QObject(dd, parent)
    , QSurface(QSurface::Window)
{
    Q_D(QWindow);
    d->parentWindow = parent;
    if (parent)
        d->screen = parent->screen();
    if (!d->screen)
        d->screen = QGuiApplication::primaryScreen();
    QGuiApplicationPrivate::window_list.prepend(this);
}

/*!
    Destroys the window.
*/
QWindow::~QWindow()
{
    if (QGuiApplicationPrivate::focus_window == this)
        QGuiApplicationPrivate::focus_window = 0;
    QGuiApplicationPrivate::window_list.removeAll(this);
    destroy();
}

QSurface::~QSurface()
{
}

/*!
    Set the \a surfaceType of the window.

    Specifies whether the window is meant for raster rendering with
    QBackingStore, or OpenGL rendering with QOpenGLContext.

    \sa QBackingStore, QOpenGLContext
*/
void QWindow::setSurfaceType(SurfaceType surfaceType)
{
    Q_D(QWindow);
    d->surfaceType = surfaceType;
}

/*!
    Returns the surface type of the window.

    \sa setSurfaceType()
*/
QWindow::SurfaceType QWindow::surfaceType() const
{
    Q_D(const QWindow);
    return d->surfaceType;
}

/*!
    \property QWindow::visible
    \brief whether the window is visible or not

    This property controls the visibility of the window in the windowing system.

    By default, the window is not visible, you must call setVisible(true), or
    show() or similar to make it visible.

    \sa show()
*/
void QWindow::setVisible(bool visible)
{
    Q_D(QWindow);

    if (d->visible == visible)
        return;
    d->visible = visible;
    emit visibleChanged(visible);

    if (!d->platformWindow)
        create();

    if (visible) {
        // remove posted quit events when showing a new window
        QCoreApplication::removePostedEvents(qApp, QEvent::Quit);

        QShowEvent showEvent;
        QGuiApplication::sendEvent(this, &showEvent);
    }

    d->platformWindow->setVisible(visible);

    if (!visible) {
        QHideEvent hideEvent;
        QGuiApplication::sendEvent(this, &hideEvent);
    }
}

/*!
    Returns true if the window is set to visible.
    \obsolete
*/
bool QWindow::visible() const
{
    return isVisible();
}

bool QWindow::isVisible() const
{
    Q_D(const QWindow);

    return d->visible;
}

/*!
    Allocates the platform resources associated with the window.

    It is at this point that the surface format set using setFormat() gets resolved
    into an actual native surface. However, the window remains hidden until setVisible() is called.

    Note that it is not usually necessary to call this function directly, as it will be implicitly
    called by show(), setVisible(), and other functions that require access to the platform
    resources.

    Call destroy() to free the platform resources if necessary.

    \sa destroy()
*/
void QWindow::create()
{
    Q_D(QWindow);
    if (!d->platformWindow) {
        d->platformWindow = QGuiApplicationPrivate::platformIntegration()->createPlatformWindow(this);
        QObjectList childObjects = children();
        for (int i = 0; i < childObjects.size(); i ++) {
            QObject *object = childObjects.at(i);
            if(object->isWindowType()) {
                QWindow *window = static_cast<QWindow *>(object);
                if (window->d_func()->platformWindow)
                    window->d_func()->platformWindow->setParent(d->platformWindow);
            }
        }
    }
}

/*!
    Returns the window's platform id.

    For platforms where this id might be useful, the value returned
    will uniquely represent the window inside the corresponding screen.

    \sa screen()
*/
WId QWindow::winId() const
{
    Q_D(const QWindow);
    if(!d->platformWindow)
        const_cast<QWindow *>(this)->create();

    WId id = d->platformWindow->winId();
    // See the QPlatformWindow::winId() documentation
    Q_ASSERT(id != WId(0));
    return id;
}

/*!
    Returns the parent window, if any.

    A window without a parent is known as a top level window.
*/
QWindow *QWindow::parent() const
{
    Q_D(const QWindow);
    return d->parentWindow;
}

/*!
    Sets the parent Window. This will lead to the windowing system managing the clip of the window, so it will be clipped to the parent window.

    Setting parent to be 0 will make the window become a top level window.
*/

void QWindow::setParent(QWindow *parent)
{
    Q_D(QWindow);

    QObject::setParent(parent);

    if (d->platformWindow) {
        if (parent && parent->d_func()->platformWindow) {
            d->platformWindow->setParent(parent->d_func()->platformWindow);
        } else {
            d->platformWindow->setParent(0);
        }
    }

    d->parentWindow = parent;
}

/*!
    Returns whether the window is top level, i.e. has no parent window.
*/
bool QWindow::isTopLevel() const
{
    Q_D(const QWindow);
    return d->parentWindow == 0;
}

/*!
    Returns whether the window is modal.

    A modal window prevents other windows from getting any input.
*/
bool QWindow::isModal() const
{
    Q_D(const QWindow);
    return d->modality != Qt::NonModal;
}

/*!
    Returns the window's modality.

    \sa setWindowModality()
*/
Qt::WindowModality QWindow::windowModality() const
{
    Q_D(const QWindow);
    return d->modality;
}

/*!
    Sets the window's modality to \a windowModality.
*/
void QWindow::setWindowModality(Qt::WindowModality windowModality)
{
    Q_D(QWindow);
    d->modality = windowModality;
}

/*!
    Sets the window's surface \a format.

    The format determines properties such as color depth, alpha,
    depth and stencil buffer size, etc.
*/
void QWindow::setFormat(const QSurfaceFormat &format)
{
    Q_D(QWindow);
    d->requestedFormat = format;
}

/*!
    Returns the requested surfaceformat of this window.

    If the requested format was not supported by the platform implementation,
    the requestedFormat will differ from the actual window format.

    This is the value set with setFormat().

    \sa setFormat(), format()
 */
QSurfaceFormat QWindow::requestedFormat() const
{
    Q_D(const QWindow);
    return d->requestedFormat;
}

/*!
    Returns the actual format of this window.

    After the window has been created, this function will return the actual surface format
    of the window. It might differ from the requested format if the requested format could
    not be fulfilled by the platform.

    \sa create(), requestedFormat()
*/
QSurfaceFormat QWindow::format() const
{
    Q_D(const QWindow);
    if (d->platformWindow)
        return d->platformWindow->format();
    return d->requestedFormat;
}

/*!
    Sets the window flags of the window to \a flags.

    The window flags control the window's appearance in the windowing system,
    whether it's a dialog, popup, or a regular window, and whether it should
    have a title bar, etc.

    \sa windowFlags()
*/
void QWindow::setWindowFlags(Qt::WindowFlags flags)
{
    Q_D(QWindow);
    if (d->platformWindow)
        d->windowFlags = d->platformWindow->setWindowFlags(flags);
    else
        d->windowFlags = flags;
}

/*!
    Returns the window flags of the window.

    This might differ from the flags set with setWindowFlags() if the
    requested flags could not be fulfilled.

    \sa setWindowFlags()
*/
Qt::WindowFlags QWindow::windowFlags() const
{
    Q_D(const QWindow);
    return d->windowFlags;
}

/*!
    Returns the type of the window.

    This returns the part of the window flags that represents
    whether the window is a dialog, tooltip, popup, regular window, etc.

    \sa windowFlags(), setWindowFlags()
*/
Qt::WindowType QWindow::windowType() const
{
    Q_D(const QWindow);
    return static_cast<Qt::WindowType>(int(d->windowFlags & Qt::WindowType_Mask));
}

/*!
    \property QWindow::windowTitle
    \brief the window's title in the windowing system

    The window title might appear in the title area of the window decorations,
    depending on the windowing system and the window flags. It might also
    be used by the windowing system to identify the window in other contexts,
    such as in the task switcher.

    \sa windowFlags()
*/
void QWindow::setWindowTitle(const QString &title)
{
    Q_D(QWindow);
    d->windowTitle = title;
    if (d->platformWindow) {
        d->platformWindow->setWindowTitle(title);
    }
}

QString QWindow::windowTitle() const
{
    Q_D(const QWindow);
    return d->windowTitle;
}

/*!
    Raise the window in the windowing system.

    Requests that the window be raised to appear above other windows.
*/
void QWindow::raise()
{
    Q_D(QWindow);
    if (d->platformWindow) {
        d->platformWindow->raise();
    }
}

/*!
    Lower the window in the windowing system.

    Requests that the window be lowered to appear below other windows.
*/
void QWindow::lower()
{
    Q_D(QWindow);
    if (d->platformWindow) {
        d->platformWindow->lower();
    }
}

/*!
    Sets the window's opacity in the windowing system to \a level.

    If the windowing system supports window opacity, this can be used to fade the
    window in and out, or to make it semitransparent.

    A value of 1.0 or above is treated as fully opaque, whereas a value of 0.0 or below
    is treated as fully transparent. Values inbetween represent varying levels of
    translucency between the two extremes.
*/
void QWindow::setOpacity(qreal level)
{
    Q_D(QWindow);
    if (d->platformWindow) {
        d->platformWindow->setOpacity(level);
    }
}

/*!
    Requests the window to be activated, i.e. receive keyboard focus.

    \sa isActive(), QGuiApplication::focusWindow()
*/
void QWindow::requestActivateWindow()
{
    Q_D(QWindow);
    if (d->platformWindow)
        d->platformWindow->requestActivateWindow();
}

/*!
    Returns if this window is exposed in the windowing system.

    When the window is not exposed, it is shown by the application
    but it is still not showing in the windowing system, so the application
    should minimize rendering and other graphical activities.

    An exposeEvent() is sent every time this value changes.

    \sa exposeEvent()
*/
bool QWindow::isExposed() const
{
    Q_D(const QWindow);
    return d->exposed;
}

/*!
    Returns true if the window should appear active from a style perspective.

    This is the case for the window that has input focus as well as windows
    that are in the same parent / transient parent chain as the focus window.

    To get the window that currently has focus, use QGuiApplication::focusWindow().
*/
bool QWindow::isActive() const
{
    Q_D(const QWindow);
    if (!d->platformWindow)
        return false;

    QWindow *focus = QGuiApplication::focusWindow();

    // Means the whole application lost the focus
    if (!focus)
        return false;

    if (focus == this)
        return true;

    if (!parent() && !transientParent()) {
        return isAncestorOf(focus);
    } else {
        return (parent() && parent()->isActive()) || (transientParent() && transientParent()->isActive());
    }
}

/*!
    \property QWindow::contentOrientation
    \brief the orientation of the window's contents

    This is a hint to the window manager in case it needs to display
    additional content like popups, dialogs, status bars, or similar
    in relation to the window.

    The recommended orientation is QScreen::orientation() but
    an application doesn't have to support all possible orientations,
    and thus can opt to ignore the current screen orientation.

    The difference between the window and the content orientation
    determines how much to rotate the content by. QScreen::angleBetween(),
    QScreen::transformBetween(), and QScreen::mapBetween() can be used
    to compute the necessary transform.

    The default value is Qt::PrimaryOrientation

    \sa requestWindowOrientation(), QScreen::orientation()
*/
void QWindow::reportContentOrientationChange(Qt::ScreenOrientation orientation)
{
    Q_D(QWindow);
    if (d->contentOrientation == orientation)
        return;
    if (!d->platformWindow)
        create();
    Q_ASSERT(d->platformWindow);
    d->contentOrientation = orientation;
    d->platformWindow->handleContentOrientationChange(orientation);
    emit contentOrientationChanged(orientation);
}

Qt::ScreenOrientation QWindow::contentOrientation() const
{
    Q_D(const QWindow);
    return d->contentOrientation;
}

/*!
  Requests the given window orientation.

  The window orientation specifies how the window should be rotated
  by the window manager in order to be displayed. Input events will
  be correctly mapped to the given orientation.

  The return value is false if the system doesn't support the given
  orientation (for example when requesting a portrait orientation
  on a device that only handles landscape buffers, typically a desktop
  system).

  If the return value is false, call windowOrientation() to get the actual
  supported orientation.

  \sa windowOrientation(), reportContentOrientationChange(), QScreen::orientation()
*/
bool QWindow::requestWindowOrientation(Qt::ScreenOrientation orientation)
{
    Q_D(QWindow);
    if (!d->platformWindow)
        create();
    Q_ASSERT(d->platformWindow);
    d->windowOrientation = d->platformWindow->requestWindowOrientation(orientation);
    return d->windowOrientation == orientation;
}

/*!
  Returns the actual window orientation.

  The default value is Qt::PrimaryOrientation.

  \sa requestWindowOrientation()
*/
Qt::ScreenOrientation QWindow::windowOrientation() const
{
    Q_D(const QWindow);
    return d->windowOrientation;
}

/*!
    Returns the window state.

    \sa setWindowState()
*/
Qt::WindowState QWindow::windowState() const
{
    Q_D(const QWindow);
    return d->windowState;
}

/*!
    Sets the desired window \a state.

    The window state represents whether the window appears in the
    windowing system as maximized, minimized, fullscreen, or normal.

    The enum value Qt::WindowActive is not an accepted parameter.

    \sa windowState(), showNormal(), showFullScreen(), showMinimized(), showMaximized()
*/
void QWindow::setWindowState(Qt::WindowState state)
{
    if (state == Qt::WindowActive) {
        qWarning() << "QWindow::setWindowState does not accept Qt::WindowActive";
        return;
    }

    Q_D(QWindow);
    if (d->platformWindow)
        d->windowState = d->platformWindow->setWindowState(state);
    else
        d->windowState = state;
}

/*!
    Sets the transient parent

    This is a hint to the window manager that this window is a dialog or pop-up on behalf of the given window.

    \sa transientParent(), parent()
*/
void QWindow::setTransientParent(QWindow *parent)
{
    Q_D(QWindow);
    d->transientParent = parent;
}

/*!
    Returns the transient parent of the window.

    \sa setTransientParent(), parent()
*/
QWindow *QWindow::transientParent() const
{
    Q_D(const QWindow);
    return d->transientParent.data();
}

/*!
    \enum QWindow::AncestorMode

    This enum is used to control whether or not transient parents
    should be considered ancestors.

    \value ExcludeTransients Transient parents are not considered ancestors.
    \value IncludeTransients Transient parents are considered ancestors.
*/

/*!
    Returns true if the window is an ancestor of the given child. If mode is
    IncludeTransients transient parents are also considered ancestors.
*/
bool QWindow::isAncestorOf(const QWindow *child, AncestorMode mode) const
{
    if (child->parent() == this || (mode == IncludeTransients && child->transientParent() == this))
        return true;

    return (child->parent() && isAncestorOf(child->parent(), mode))
        || (mode == IncludeTransients && child->transientParent() && isAncestorOf(child->transientParent(), mode));
}

/*!
    Returns the minimum size of the window.

    \sa setMinimumSize()
*/
QSize QWindow::minimumSize() const
{
    Q_D(const QWindow);
    return d->minimumSize;
}

/*!
    Returns the maximum size of the window.

    \sa setMaximumSize()
*/
QSize QWindow::maximumSize() const
{
    Q_D(const QWindow);
    return d->maximumSize;
}

/*!
    Returns the base size of the window.

    \sa setBaseSize()
*/
QSize QWindow::baseSize() const
{
    Q_D(const QWindow);
    return d->baseSize;
}

/*!
    Returns the size increment of the window.

    \sa setSizeIncrement()
*/
QSize QWindow::sizeIncrement() const
{
    Q_D(const QWindow);
    return d->sizeIncrement;
}

/*!
    Sets the minimum size of the window.

    This is a hint to the window manager to prevent resizing below the specified \a size.

    \sa setMaximumSize(), minimumSize()
*/
void QWindow::setMinimumSize(const QSize &size)
{
    Q_D(QWindow);
    QSize adjustedSize = QSize(qBound(0, size.width(), QWINDOWSIZE_MAX), qBound(0, size.height(), QWINDOWSIZE_MAX));
    if (d->minimumSize == adjustedSize)
        return;
    d->minimumSize = adjustedSize;
    if (d->platformWindow && isTopLevel())
        d->platformWindow->propagateSizeHints();
}

/*!
    Sets the maximum size of the window.

    This is a hint to the window manager to prevent resizing above the specified \a size.

    \sa setMinimumSize(), maximumSize()
*/
void QWindow::setMaximumSize(const QSize &size)
{
    Q_D(QWindow);
    QSize adjustedSize = QSize(qBound(0, size.width(), QWINDOWSIZE_MAX), qBound(0, size.height(), QWINDOWSIZE_MAX));
    if (d->maximumSize == adjustedSize)
        return;
    d->maximumSize = adjustedSize;
    if (d->platformWindow && isTopLevel())
        d->platformWindow->propagateSizeHints();
}

/*!
    Sets the base size of the window.

    The base size is used to calculate a proper window size if the
    window defines sizeIncrement().

    \sa setMinimumSize(), setMaximumSize(), setSizeIncrement(), baseSize()
*/
void QWindow::setBaseSize(const QSize &size)
{
    Q_D(QWindow);
    if (d->baseSize == size)
        return;
    d->baseSize = size;
    if (d->platformWindow && isTopLevel())
        d->platformWindow->propagateSizeHints();
}

/*!
    Sets the size increment of the window.

    When the user resizes the window, the size will move in steps of
    sizeIncrement().width() pixels horizontally and
    sizeIncrement().height() pixels vertically, with baseSize() as the
    basis.

    By default, this property contains a size with zero width and height.

    The windowing system might not support size increments.

    \sa setBaseSize(), setMinimumSize(), setMaximumSize()
*/
void QWindow::setSizeIncrement(const QSize &size)
{
    Q_D(QWindow);
    if (d->sizeIncrement == size)
        return;
    d->sizeIncrement = size;
    if (d->platformWindow && isTopLevel())
        d->platformWindow->propagateSizeHints();
}

/*!
    Sets the geometry of the window, excluding its window frame, to \a rect.

    To make sure the window is visible, make sure the geometry is within
    the virtual geometry of its screen.

    \sa geometry(), screen(), QScreen::virtualGeometry()
*/
void QWindow::setGeometry(const QRect &rect)
{
    Q_D(QWindow);
    if (rect == geometry())
        return;
    QRect oldRect = geometry();

    d->positionPolicy = QWindowPrivate::WindowFrameExclusive;
    if (d->platformWindow) {
        d->platformWindow->setGeometry(rect);
    } else {
        d->geometry = rect;
    }

    if (rect.x() != oldRect.x())
        emit xChanged(rect.x());
    if (rect.y() != oldRect.y())
        emit yChanged(rect.y());
    if (rect.width() != oldRect.width())
        emit widthChanged(rect.width());
    if (rect.height() != oldRect.height())
        emit heightChanged(rect.height());
}

/*!
    \property QWindow::x
    \brief the x position of the window's geometry
*/

/*!
    \property QWindow::y
    \brief the y position of the window's geometry
*/

/*!
    \property QWindow::width
    \brief the width of the window's geometry
*/

/*!
    \property QWindow::height
    \brief the height of the window's geometry
*/

/*!
    Returns the geometry of the window, excluding its window frame.

    \sa frameMargins(), frameGeometry()
*/
QRect QWindow::geometry() const
{
    Q_D(const QWindow);
    if (d->platformWindow)
        return d->platformWindow->geometry();
    return d->geometry;
}

/*!
    Returns the window frame margins surrounding the window.

    \sa geometry(), frameGeometry()
*/
QMargins QWindow::frameMargins() const
{
    Q_D(const QWindow);
    if (d->platformWindow)
        return d->platformWindow->frameMargins();
    return QMargins();
}

/*!
    Returns the geometry of the window, including its window frame.

    \sa geometry(), frameMargins()
*/
QRect QWindow::frameGeometry() const
{
    Q_D(const QWindow);
    if (d->platformWindow) {
        QMargins m = frameMargins();
        return d->platformWindow->geometry().adjusted(-m.left(), -m.top(), m.right(), m.bottom());
    }
    return d->geometry;
}

/*!
    Returns the top left position of the window, including its window frame.

    This returns the same value as frameGeometry().topLeft().

    \sa geometry(), frameGeometry()
*/
QPoint QWindow::framePos() const
{
    Q_D(const QWindow);
    if (d->platformWindow) {
        QMargins margins = frameMargins();
        return d->platformWindow->geometry().topLeft() - QPoint(margins.left(), margins.top());
    }
    return d->geometry.topLeft();
}

/*!
    Sets the upper left position of the window including its window frame.

    \sa setGeometry(), frameGeometry()
*/
void QWindow::setFramePos(const QPoint &point)
{
    Q_D(QWindow);
    d->positionPolicy = QWindowPrivate::WindowFrameInclusive;
    if (d->platformWindow) {
        d->platformWindow->setGeometry(QRect(point, size()));
    } else {
        d->geometry.setTopLeft(point);
    }
}

/*!
    Sets the size of the window to be \a newSize.

    \sa setGeometry()
*/
void QWindow::resize(const QSize &newSize)
{
    Q_D(QWindow);
    if (d->platformWindow) {
        d->platformWindow->setGeometry(QRect(pos(), newSize));
    } else {
        d->geometry.setSize(newSize);
    }
}

/*!
    Sets the window icon to the given \a icon image.

    The window icon might be used by the windowing system for example to decorate the window,
    or in the task switcher.
*/
void QWindow::setWindowIcon(const QImage &icon) const
{
    Q_UNUSED(icon);
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
}

/*!
    Releases the native platform resources associated with this window.

    \sa create()
*/
void QWindow::destroy()
{
    Q_D(QWindow);
    QObjectList childrenWindows = children();
    for (int i = 0; i < childrenWindows.size(); i++) {
        QObject *object = childrenWindows.at(i);
        if (object->isWindowType()) {
            QWindow *w = static_cast<QWindow*>(object);
            QGuiApplicationPrivate::window_list.removeAll(w);
            w->destroy();
        }
    }
    setVisible(false);
    delete d->platformWindow;
    d->resizeEventPending = true;
    d->receivedExpose = false;
    d->exposed = false;
    d->platformWindow = 0;
}

/*!
    Returns the platform window corresponding to the window.

    \internal
*/
QPlatformWindow *QWindow::handle() const
{
    Q_D(const QWindow);
    return d->platformWindow;
}

/*!
    Returns the platform surface corresponding to the window.

    \internal
*/
QPlatformSurface *QWindow::surfaceHandle() const
{
    Q_D(const QWindow);
    return d->platformWindow;
}

/*!
    Set whether keyboard grab should be enabled or not.

    If the return value is true, the window receives all key events until setKeyboardGrabEnabled(false) is
    called; other windows get no key events at all. Mouse events are not affected.
    Use setMouseGrabEnabled() if you want to grab that.

    \sa setMouseGrabEnabled()
*/
bool QWindow::setKeyboardGrabEnabled(bool grab)
{
    Q_D(QWindow);
    if (d->platformWindow)
        return d->platformWindow->setKeyboardGrabEnabled(grab);
    return false;
}

/*!
    Sets whether mouse grab should be enabled or not.

    If the return value is true, the window receives all mouse events until setMouseGrabEnabled(false) is
    called; other windows get no mouse events at all. Keyboard events are not affected.
    Use setKeyboardGrabEnabled() if you want to grab that.

    \sa setKeyboardGrabEnabled()
*/
bool QWindow::setMouseGrabEnabled(bool grab)
{
    Q_D(QWindow);
    if (d->platformWindow)
        return d->platformWindow->setMouseGrabEnabled(grab);
    return false;
}

/*!
    Returns the screen on which the window is shown.

    The value returned will not change when the window is moved
    between virtual screens (as returned by QScreen::virtualSiblings()).

    \sa setScreen(), QScreen::virtualSiblings()
*/
QScreen *QWindow::screen() const
{
    Q_D(const QWindow);
    return d->screen;
}

/*!
    Sets the screen on which the window should be shown.

    If the window has been created, it will be recreated on the new screen.

    Note that if the screen is part of a virtual desktop of multiple screens,
    the window can appear on any of the screens returned by QScreen::virtualSiblings().

    \sa screen(), QScreen::virtualSiblings()
*/
void QWindow::setScreen(QScreen *newScreen)
{
    Q_D(QWindow);
    if (!newScreen)
        newScreen = QGuiApplication::primaryScreen();
    if (newScreen != screen()) {
        const bool wasCreated = d->platformWindow != 0;
        if (wasCreated)
            destroy();
        if (d->screen)
            disconnect(d->screen, SIGNAL(destroyed(QObject *)), this, SLOT(screenDestroyed(QObject *)));
        d->screen = newScreen;
        if (newScreen) {
            connect(d->screen, SIGNAL(destroyed(QObject *)), this, SLOT(screenDestroyed(QObject *)));
            if (wasCreated)
                create();
        }
        emit screenChanged(newScreen);
    }
}

void QWindow::screenDestroyed(QObject *object)
{
    Q_D(QWindow);
    if (object == static_cast<QObject *>(d->screen))
        setScreen(0);
}

/*!
    \fn QWindow::screenChanged(QScreen *screen)

    This signal is emitted when a window's screen changes, either
    by being set explicitly with setScreen(), or automatically when
    the window's screen is removed.
*/

/*!
  Returns the accessibility interface for the object that the window represents
  \internal
  \sa QAccessible
  */
QAccessibleInterface *QWindow::accessibleRoot() const
{
    return 0;
}

/*!
    \fn QWindow::focusObjectChanged(QObject *focusObject)

    This signal is emitted when final receiver of events tied to focus is changed.
    \sa focusObject()
*/

/*!
    Returns the QObject that will be the final receiver of events tied focus, such
    as key events.
*/
QObject *QWindow::focusObject() const
{
    return const_cast<QWindow *>(this);
}

/*!
    Shows the window.

    This equivalent to calling showFullScreen() or showNormal(), depending
    on whether the platform defaults to windows being fullscreen or not.

    \sa showFullScreen(), showNormal(), hide(), QStyleHints::showIsFullScreen()
*/
void QWindow::show()
{
    if (qApp->styleHints()->showIsFullScreen())
        showFullScreen();
    else
        showNormal();
}

/*!
    Hides the window.

    Equivalent to calling setVisible(false).

    \sa show(), setVisible()
*/
void QWindow::hide()
{
    setVisible(false);
}

/*!
    Shows the window as minimized.

    Equivalent to calling setWindowState(Qt::WindowMinimized) and then
    setVisible(true).

    \sa setWindowState(), setVisible()
*/
void QWindow::showMinimized()
{
    setWindowState(Qt::WindowMinimized);
    setVisible(true);
}

/*!
    Shows the window as maximized.

    Equivalent to calling setWindowState(Qt::WindowMaximized) and then
    setVisible(true).

    \sa setWindowState(), setVisible()
*/
void QWindow::showMaximized()
{
    setWindowState(Qt::WindowMaximized);
    setVisible(true);
}

/*!
    Shows the window as fullscreen.

    Equivalent to calling setWindowState(Qt::WindowFullScreen) and then
    setVisible(true).

    \sa setWindowState(), setVisible()
*/
void QWindow::showFullScreen()
{
    setWindowState(Qt::WindowFullScreen);
    setVisible(true);
    requestActivateWindow();
}

/*!
    Shows the window as normal, i.e. neither maximized, minimized, nor fullscreen.

    Equivalent to calling setWindowState(Qt::WindowNoState) and then
    setVisible(true).

    \sa setWindowState(), setVisible()
*/
void QWindow::showNormal()
{
    setWindowState(Qt::WindowNoState);
    setVisible(true);
}

/*!
    Close the window.

    This closes the window, effectively calling destroy(), and
    potentially quitting the application

    \sa destroy(), QGuiApplication::quitOnLastWindowClosed()
*/
bool QWindow::close()
{
    Q_D(QWindow);

    // Do not close non top level windows
    if (parent())
        return false;

    if (QGuiApplicationPrivate::focus_window == this)
        QGuiApplicationPrivate::focus_window = 0;

    QGuiApplicationPrivate::window_list.removeAll(this);
    destroy();
    d->maybeQuitOnLastWindowClosed();
    return true;
}

/*!
    The expose event is sent by the window system whenever the window's
    exposure on screen changes.

    The application can start rendering into the window with QBackingStore
    and QOpenGLContext as soon as it gets an exposeEvent() such that
    isExposed() is true.

    If the window is moved off screen, is made totally obscured by another
    window, iconified or similar, this function might be called and the
    value of isExposed() might change to false. When this happens,
    an application should stop its rendering as it is no longer visible
    to the user.

    A resize event will always be sent before the expose event the first time
    a window is shown.

    \sa isExposed()
*/
void QWindow::exposeEvent(QExposeEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle mouse events.
*/
void QWindow::moveEvent(QMoveEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle resize events.

    The resize event is called whenever the window is resized in the windowing system,
    either directly through the windowing system acknowledging a setGeometry() or resize() request,
    or indirectly through the user resizing the window manually.
*/
void QWindow::resizeEvent(QResizeEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle show events.

    The show event is called when the window has requested becoming visible.

    If the window is successfully shown by the windowing system, this will
    be followed by a resize and an expose event.
*/
void QWindow::showEvent(QShowEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle hide evens.

    The hide event is called when the window has requested being hidden in the
    windowing system.
*/
void QWindow::hideEvent(QHideEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle any event sent to the window.

    Remember to call the base class version if you wish for mouse events,
    key events, resize events, etc to be dispatched as usual.
*/
bool QWindow::event(QEvent *ev)
{
    switch (ev->type()) {
    case QEvent::MouseMove:
        mouseMoveEvent(static_cast<QMouseEvent*>(ev));
        break;

    case QEvent::MouseButtonPress:
        mousePressEvent(static_cast<QMouseEvent*>(ev));
        break;

    case QEvent::MouseButtonRelease:
        mouseReleaseEvent(static_cast<QMouseEvent*>(ev));
        break;

    case QEvent::MouseButtonDblClick:
        mouseDoubleClickEvent(static_cast<QMouseEvent*>(ev));
        break;

    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        touchEvent(static_cast<QTouchEvent *>(ev));
        break;

    case QEvent::Move:
        moveEvent(static_cast<QMoveEvent*>(ev));
        break;

    case QEvent::Resize:
        resizeEvent(static_cast<QResizeEvent*>(ev));
        break;

    case QEvent::KeyPress:
        keyPressEvent(static_cast<QKeyEvent *>(ev));
        break;

    case QEvent::KeyRelease:
        keyReleaseEvent(static_cast<QKeyEvent *>(ev));
        break;

    case QEvent::FocusIn: {
        focusInEvent(static_cast<QFocusEvent *>(ev));
#ifndef QT_NO_ACCESSIBILITY
        QAccessible::State state;
        state.active = true;
        QAccessibleStateChangeEvent event(this, state);
        QAccessible::updateAccessibility(&event);
#endif
        break; }

    case QEvent::FocusOut: {
        focusOutEvent(static_cast<QFocusEvent *>(ev));
#ifndef QT_NO_ACCESSIBILITY
        QAccessible::State state;
        state.active = true;
        QAccessibleStateChangeEvent event(this, state);
        QAccessible::updateAccessibility(&event);
#endif
        break; }

#ifndef QT_NO_WHEELEVENT
    case QEvent::Wheel:
        wheelEvent(static_cast<QWheelEvent*>(ev));
        break;
#endif

    case QEvent::Close: {
        Q_D(QWindow);
        bool wasVisible = isVisible();
        destroy();
        if (wasVisible)
            d->maybeQuitOnLastWindowClosed();
        break; }

    case QEvent::Expose:
        exposeEvent(static_cast<QExposeEvent *>(ev));
        break;

    case QEvent::Show:
        showEvent(static_cast<QShowEvent *>(ev));
        break;

    case QEvent::Hide:
        hideEvent(static_cast<QHideEvent *>(ev));
        break;

    default:
        return QObject::event(ev);
    }
    return true;
}

/*!
    Override this to handle key press events.

    \sa keyReleaseEvent
*/
void QWindow::keyPressEvent(QKeyEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle key release events.

    \sa keyPressEvent
*/
void QWindow::keyReleaseEvent(QKeyEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle focus in events.

    Focus in events are sent when the window receives keyboard focus.

    \sa focusOutEvent
*/
void QWindow::focusInEvent(QFocusEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle focus out events.

    Focus out events are sent when the window loses keyboard focus.

    \sa focusInEvent
*/
void QWindow::focusOutEvent(QFocusEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle mouse press events.

    \sa mouseReleaseEvent()
*/
void QWindow::mousePressEvent(QMouseEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle mouse release events.

    \sa mousePressEvent()
*/
void QWindow::mouseReleaseEvent(QMouseEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle mouse double click events.

    \sa mousePressEvent(), QStyleHints::mouseDoubleClickInterval()
*/
void QWindow::mouseDoubleClickEvent(QMouseEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle mouse move events.
*/
void QWindow::mouseMoveEvent(QMouseEvent *ev)
{
    ev->ignore();
}

#ifndef QT_NO_WHEELEVENT
/*!
    Override this to handle mouse wheel or other wheel events.
*/
void QWindow::wheelEvent(QWheelEvent *ev)
{
    ev->ignore();
}
#endif //QT_NO_WHEELEVENT

/*!
    Override this to handle touch events.
*/
void QWindow::touchEvent(QTouchEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle platform dependent events.

    This might make your application non-portable.
*/
bool QWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
    return false;
}

/*!
    \fn QPoint QWindow::mapToGlobal(const QPoint &pos) const

    Translates the window coordinate \a pos to global screen
    coordinates. For example, \c{mapToGlobal(QPoint(0,0))} would give
    the global coordinates of the top-left pixel of the window.

    \sa mapFromGlobal()
*/
QPoint QWindow::mapToGlobal(const QPoint &pos) const
{
    return pos + d_func()->globalPosition();
}


/*!
    \fn QPoint QWindow::mapFromGlobal(const QPoint &pos) const

    Translates the global screen coordinate \a pos to window
    coordinates.

    \sa mapToGlobal()
*/
QPoint QWindow::mapFromGlobal(const QPoint &pos) const
{
    return pos - d_func()->globalPosition();
}


Q_GUI_EXPORT QWindowPrivate *qt_window_private(QWindow *window)
{
    return window->d_func();
}

void QWindowPrivate::maybeQuitOnLastWindowClosed()
{
    Q_Q(QWindow);

    // Attempt to close the application only if this has WA_QuitOnClose set and a non-visible parent
    bool quitOnClose = QGuiApplication::quitOnLastWindowClosed() && !q->parent();

    if (quitOnClose) {
        QWindowList list = QGuiApplication::topLevelWindows();
        bool lastWindowClosed = true;
        for (int i = 0; i < list.size(); ++i) {
            QWindow *w = list.at(i);
            if (!w->isVisible())
                continue;
            lastWindowClosed = false;
            break;
        }
        if (lastWindowClosed) {
            QGuiApplicationPrivate::emitLastWindowClosed();
            QCoreApplicationPrivate *applicationPrivate = static_cast<QCoreApplicationPrivate*>(QObjectPrivate::get(QCoreApplication::instance()));
            applicationPrivate->maybeQuit();
        }
    }

}

QT_END_NAMESPACE
