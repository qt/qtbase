/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qplatformwindow.h"
#include "qplatformwindow_p.h"
#include "qplatformscreen.h"

#include <private/qguiapplication_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtGui/qwindow.h>
#include <QtGui/qscreen.h>
#include <private/qhighdpiscaling_p.h>
#include <private/qwindow_p.h>


QT_BEGIN_NAMESPACE

/*!
    Constructs a platform window with the given top level window.
*/

QPlatformWindow::QPlatformWindow(QWindow *window)
    : QPlatformSurface(window)
    , d_ptr(new QPlatformWindowPrivate)
{
    Q_D(QPlatformWindow);
    d->rect = QHighDpi::toNativePixels(window->geometry(), window);
}

/*!
    Virtual destructor does not delete its top level window.
*/
QPlatformWindow::~QPlatformWindow()
{
}

/*!
    Called as part of QWindow::create(), after constructing
    the window. Platforms should prefer to do initialization
    here instead of in the constructor, as the platform window
    object will be fully constructed, and associated to the
    corresponding QWindow, allowing synchronous event delivery.
*/
void QPlatformWindow::initialize()
{
}

/*!
    Returns the window which belongs to the QPlatformWindow
*/
QWindow *QPlatformWindow::window() const
{
    return static_cast<QWindow *>(m_surface);
}

/*!
    Returns the parent platform window (or \nullptr if orphan).
*/
QPlatformWindow *QPlatformWindow::parent() const
{
    return window()->parent() ? window()->parent()->handle() : nullptr;
}

/*!
    Returns the platform screen handle corresponding to this platform window,
    or null if the window is not associated with a screen.
*/
QPlatformScreen *QPlatformWindow::screen() const
{
    QScreen *scr = window()->screen();
    return scr ? scr->handle() : nullptr;
}

/*!
    Returns the actual surface format of the window.
*/
QSurfaceFormat QPlatformWindow::format() const
{
    return QSurfaceFormat();
}

/*!
    This function is called by Qt whenever a window is moved or resized using the QWindow API.

    Unless you also override QPlatformWindow::geometry(), you need to call the baseclass
    implementation of this function in any override of QPlatformWindow::setGeometry(), as
    QWindow::geometry() is expected to report back the set geometry until a confirmation
    (or rejection) of the new geometry comes back from the window manager and is reported
    via QWindowSystemInterface::handleGeometryChange().

    Window move/resizes can also be triggered spontaneously by the window manager, or as a
    response to an earlier requested move/resize via the Qt APIs. There is no need to call
    this function from the window manager callback, instead call
    QWindowSystemInterface::handleGeometryChange().

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
    Returns the current geometry of a window
*/
QRect QPlatformWindow::geometry() const
{
    Q_D(const QPlatformWindow);
    return d->rect;
}

/*!
    Returns the geometry of a window in 'normal' state
    (neither maximized, fullscreen nor minimized) for saving geometries to
    application settings.

    \since 5.3
*/
QRect QPlatformWindow::normalGeometry() const
{
    return QRect();
}

QMargins QPlatformWindow::frameMargins() const
{
    return QMargins();
}

/*!
    The safe area margins of a window represent the area that is safe to
    place content within, without intersecting areas of the screen where
    system UI is placed, or where a screen bezel may cover the content.
*/
QMargins QPlatformWindow::safeAreaMargins() const
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
    Q_UNUSED(visible);
    QRect rect(QPoint(), geometry().size());
    QWindowSystemInterface::handleExposeEvent(window(), rect);
    QWindowSystemInterface::flushWindowSystemEvents();
}

/*!
    Requests setting the window flags of this surface
    to \a flags.
*/
void QPlatformWindow::setWindowFlags(Qt::WindowFlags flags)
{
    Q_UNUSED(flags);
}

/*!
    Returns if this window is exposed in the windowing system.

    An exposeEvent() is sent every time this value changes.
 */

bool QPlatformWindow::isExposed() const
{
    return window()->isVisible();
}

/*!
    Returns \c true if the window should appear active from a style perspective.

    This function can make platform-specific isActive checks, such as checking
    if the QWindow is embedded in an active native window.
*/
bool QPlatformWindow::isActive() const
{
    return false;
}

/*!
    Returns \c true if the window is an ancestor of the given \a child.

    Platform overrides should iterate the native window hierarchy of the child,
    to ensure that ancestary is reflected even with native windows in the window
    hierarchy.
*/
bool QPlatformWindow::isAncestorOf(const QPlatformWindow *child) const
{
    for (const QPlatformWindow *parent = child->parent(); parent; parent = child->parent()) {
        if (parent == this)
            return true;
    }

    return false;
}

/*!
    Returns \c true if the window is a child of a non-Qt window.

    A embedded window has no parent platform window as reflected
    though parent(), but will have a native parent window.
*/
bool QPlatformWindow::isEmbedded() const
{
    return false;
}

/*!
    Translates the window coordinate \a pos to global screen
    coordinates using native methods. This is required for embedded windows,
    where the topmost QWindow coordinates are not global screen coordinates.

    Returns \a pos if there is no platform specific implementation.
*/
QPoint QPlatformWindow::mapToGlobal(const QPoint &pos) const
{
    const QPlatformWindow *p = this;
    QPoint result = pos;
    while (p) {
        result += p->geometry().topLeft();
        p = p->parent();
    }
    return result;
}

/*!
    Translates the global screen coordinate \a pos to window
    coordinates using native methods. This is required for embedded windows,
    where the topmost QWindow coordinates are not global screen coordinates.

    Returns \a pos if there is no platform specific implementation.
*/
QPoint QPlatformWindow::mapFromGlobal(const QPoint &pos) const
{
    const QPlatformWindow *p = this;
    QPoint result = pos;
    while (p) {
        result -= p->geometry().topLeft();
        p = p->parent();
    }
    return result;
}

/*!
    Requests setting the window state of this surface
    to \a type.

    Qt::WindowActive can be ignored.
*/
void QPlatformWindow::setWindowState(Qt::WindowStates)
{
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

//jl: It would be useful to have a property on the platform window which indicated if the sub-class
// supported the setParent. If not, then geometry would be in screen coordinates.
/*!
    This function is called to enable native child window in QPA. It is common not to support this
    feature in Window systems, but can be faked. When this function is called all geometry of this
    platform window will be relative to the parent.
*/
void QPlatformWindow::setParent(const QPlatformWindow *parent)
{
    Q_UNUSED(parent);
    qWarning("This plugin does not support setParent!");
}

/*!
  Reimplement to set the window title to \a title.

  The implementation might want to append the application display name to
  the window title, like Windows and Linux do.

  \sa QGuiApplication::applicationDisplayName()
*/
void QPlatformWindow::setWindowTitle(const QString &title) { Q_UNUSED(title); }

/*!
  Reimplement to set the window file path to \a filePath
*/
void QPlatformWindow::setWindowFilePath(const QString &filePath) { Q_UNUSED(filePath); }

/*!
  Reimplement to set the window icon to \a icon
*/
void QPlatformWindow::setWindowIcon(const QIcon &icon) { Q_UNUSED(icon); }

/*!
  Reimplement to let the platform handle non-spontaneous window close.

  When reimplementing make sure to call the base class implementation
  or QWindowSystemInterface::handleCloseEvent(), which will prompt the
  user to accept the window close (if needed) and then close the QWindow.
*/
bool QPlatformWindow::close()
{
    return QWindowSystemInterface::handleCloseEvent<QWindowSystemInterface::SynchronousDelivery>(window());
}

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
  Reimplement to  be able to let Qt set the mask of a window
*/

void QPlatformWindow::setMask(const QRegion &region)
{
    Q_UNUSED(region);
    qWarning("This plugin does not support setting window masks");
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
    Reimplement this function in subclass to return the device pixel ratio
    for the window. This is the ratio between physical pixels
    and device-independent pixels.

    \sa QPlatformWindow::devicePixelRatio();
*/
qreal QPlatformWindow::devicePixelRatio() const
{
    return 1.0;
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
    Reimplement to be able to let Qt indicate that the window has been
    modified. Return true if the native window supports setting the modified
    flag, false otherwise.
*/
bool QPlatformWindow::setWindowModified(bool modified)
{
    Q_UNUSED(modified);
    return false;
}

/*!
    Reimplement this method to be able to do any platform specific event
    handling. All non-synthetic events for window() are passed to this
    function before being sent to QWindow::event().

    Return true if the event should not be passed on to the QWindow.

    Subclasses should always call the base class implementation.
*/
bool QPlatformWindow::windowEvent(QEvent *event)
{
    Q_D(QPlatformWindow);

    if (event->type() == QEvent::Timer) {
        if (static_cast<QTimerEvent *>(event)->timerId() == d->updateTimer.timerId()) {
            d->updateTimer.stop();
            deliverUpdateRequest();
            return true;
        }
    }

    return false;
}

/*!
    Reimplement this method to start a system resize operation if
    the system supports it and return true to indicate success.

    The default implementation is empty and does nothing with \a edges.

    \since 5.15
*/

bool QPlatformWindow::startSystemResize(Qt::Edges edges)
{
    Q_UNUSED(edges)
    return false;
}

/*!
    Reimplement this method to start a system move operation if
    the system supports it and return true to indicate success.

    The default implementation is empty and does nothing.

    \since 5.15
*/

bool QPlatformWindow::startSystemMove()
{
    return false;
}

/*!
    Reimplement this method to set whether frame strut events
    should be sent to \a enabled.

    \sa frameStrutEventsEnabled
*/

void QPlatformWindow::setFrameStrutEventsEnabled(bool enabled)
{
    Q_UNUSED(enabled) // Do not warn as widgets enable it by default causing warnings with XCB.
}

/*!
    Reimplement this method to return whether
    frame strut events are enabled.
*/

bool QPlatformWindow::frameStrutEventsEnabled() const
{
    return false;
}

/*!
    Call this method to put together a window title composed of
    \a title
    \a separator
    the application display name

    If the display name isn't set, and the title is empty, the raw app name is used.
*/
QString QPlatformWindow::formatWindowTitle(const QString &title, const QString &separator)
{
    QString fullTitle = title;
    if (QGuiApplicationPrivate::displayName && !title.endsWith(*QGuiApplicationPrivate::displayName)) {
        // Append display name, if set.
        if (!fullTitle.isEmpty())
            fullTitle += separator;
        fullTitle += *QGuiApplicationPrivate::displayName;
    } else if (fullTitle.isEmpty()) {
        // Don't let the window title be completely empty, use the app name as fallback.
        fullTitle = QCoreApplication::applicationName();
    }
    return fullTitle;
}

/*!
    Helper function for finding the new screen for \a newGeometry in response to
    a geometry changed event. Returns the new screen if the window was moved to
    another virtual sibling. If the screen changes, the platform plugin should call
    QWindowSystemInterface::handleWindowScreenChanged().
    \note: The current screen will always be returned for child windows since
    they should never signal screen changes.

    \since 5.4
    \sa QWindowSystemInterface::handleWindowScreenChanged()
*/
QPlatformScreen *QPlatformWindow::screenForGeometry(const QRect &newGeometry) const
{
    QPlatformScreen *currentScreen = screen();
    QPlatformScreen *fallback = currentScreen;
    // QRect::center can return a value outside the rectangle if it's empty.
    // Apply mapToGlobal() in case it is a foreign/embedded window.
    QPoint center = newGeometry.isEmpty() ? newGeometry.topLeft() : newGeometry.center();
    if (isForeignWindow())
        center = mapToGlobal(center - newGeometry.topLeft());

    if (!parent() && currentScreen && !currentScreen->geometry().contains(center)) {
        const auto screens = currentScreen->virtualSiblings();
        for (QPlatformScreen *screen : screens) {
            const QRect screenGeometry = screen->geometry();
            if (screenGeometry.contains(center))
                return screen;
            if (screenGeometry.intersects(newGeometry))
                fallback = screen;
        }
    }
    return fallback;
}

/*!
    Returns a size with both dimensions bounded to [0, QWINDOWSIZE_MAX]
*/
QSize QPlatformWindow::constrainWindowSize(const QSize &size)
{
    return size.expandedTo(QSize(0, 0)).boundedTo(QSize(QWINDOWSIZE_MAX, QWINDOWSIZE_MAX));
}

/*!
    Reimplement this method to set whether the window demands attention
    (for example, by flashing the taskbar icon) depending on \a enabled.

    \sa isAlertState()
    \since 5.1
*/

void QPlatformWindow::setAlertState(bool enable)
{
    Q_UNUSED(enable)
}

/*!
    Reimplement this method return whether the window is in
    an alert state.

    \sa setAlertState()
    \since 5.1
*/

bool QPlatformWindow::isAlertState() const
{
    return false;
}

// Return the effective screen for the initial geometry of a window. In a
// multimonitor-setup, try to find the right screen by checking the transient
// parent or the mouse cursor for parentless windows (cf QTBUG-34204,
// QDialog::adjustPosition()), unless a non-primary screen has been set,
// in which case we try to respect that.
static inline const QScreen *effectiveScreen(const QWindow *window)
{
    if (!window)
        return QGuiApplication::primaryScreen();
    const QScreen *screen = window->screen();
    if (!screen)
        return QGuiApplication::primaryScreen();
    if (screen != QGuiApplication::primaryScreen())
        return screen;
#ifndef QT_NO_CURSOR
    const QList<QScreen *> siblings = screen->virtualSiblings();
    if (siblings.size() > 1) {
        const QPoint referencePoint = window->transientParent() ? window->transientParent()->geometry().center() : QCursor::pos();
        for (const QScreen *sibling : siblings) {
            if (sibling->geometry().contains(referencePoint))
                return sibling;
        }
    }
#endif
    return screen;
}

/*!
    Invalidates the window's surface by releasing its surface buffers.

    Many platforms do not support releasing the surface memory,
    and the default implementation does nothing.

    The platform window is expected to recreate the surface again if
    it is needed. For instance, if an OpenGL context is made current
    on this window.
 */
void QPlatformWindow::invalidateSurface()
{
}

static QSize fixInitialSize(QSize size, const QWindow *w,
                            int defaultWidth, int defaultHeight)
{
    if (size.width() == 0) {
        const int minWidth = w->minimumWidth();
        size.setWidth(minWidth > 0 ? minWidth : defaultWidth);
    }
    if (size.height() == 0) {
        const int minHeight = w->minimumHeight();
        size.setHeight(minHeight > 0 ? minHeight : defaultHeight);
    }
    return size;
}

/*!
    Helper function to get initial geometry on windowing systems which do not
    do smart positioning and also do not provide a means of centering a
    transient window w.r.t. its parent. For example this is useful on Windows
    and MacOS but not X11, because an X11 window manager typically tries to
    layout new windows to optimize usage of the available desktop space.
    However if the given window already has geometry which the application has
    initialized, it takes priority.
*/
QRect QPlatformWindow::initialGeometry(const QWindow *w, const QRect &initialGeometry,
                                       int defaultWidth, int defaultHeight,
                                       const QScreen **resultingScreenReturn)
{
    if (resultingScreenReturn)
        *resultingScreenReturn = w->screen();
    if (!w->isTopLevel()) {
        const qreal factor = QHighDpiScaling::factor(w);
        const QSize size = fixInitialSize(QHighDpi::fromNative(initialGeometry.size(), factor),
                                          w, defaultWidth, defaultHeight);
        return QRect(initialGeometry.topLeft(), QHighDpi::toNative(size, factor));
    }
    const auto *wp = qt_window_private(const_cast<QWindow*>(w));
    const bool position = wp->positionAutomatic && w->type() != Qt::Popup;
    if (!position && !wp->resizeAutomatic)
        return initialGeometry;
    const QScreen *screen = wp->positionAutomatic
        ? effectiveScreen(w)
        : QGuiApplication::screenAt(initialGeometry.center());
    if (!screen)
        return initialGeometry;
    if (resultingScreenReturn)
        *resultingScreenReturn = screen;
    // initialGeometry refers to window's screen
    QRect rect(QHighDpi::fromNativePixels(initialGeometry, w));
    if (wp->resizeAutomatic)
        rect.setSize(fixInitialSize(rect.size(), w, defaultWidth, defaultHeight));
    if (position) {
        const QRect availableGeometry = screen->availableGeometry();
        // Center unless the geometry ( + unknown window frame) is too large for the screen).
        if (rect.height() < (availableGeometry.height() * 8) / 9
                && rect.width() < (availableGeometry.width() * 8) / 9) {
            const QWindow *tp = w->transientParent();
            if (tp) {
                // A transient window should be centered w.r.t. its transient parent.
                rect.moveCenter(tp->geometry().center());
            } else {
                // Center the window on the screen.  (Only applicable on platforms
                // which do not provide a better way.)
                rect.moveCenter(availableGeometry.center());
            }
        }
    }
    return QHighDpi::toNativePixels(rect, screen);
}

/*!
    Requests an QEvent::UpdateRequest event. The event will be
    delivered to the QWindow.

    QPlatformWindow subclasses can re-implement this function to
    provide display refresh synchronized updates. The event
    should be delivered using QPlatformWindow::deliverUpdateRequest()
    to not get out of sync with the internal state of QWindow.

    The default implementation posts an UpdateRequest event to the
    window after 5 ms. The additional time is there to give the event
    loop a bit of idle time to gather system events.

*/
void QPlatformWindow::requestUpdate()
{
    Q_D(QPlatformWindow);

    static int updateInterval = []() {
        bool ok = false;
        int customUpdateInterval = qEnvironmentVariableIntValue("QT_QPA_UPDATE_IDLE_TIME", &ok);
        return ok ? customUpdateInterval : 5;
    }();

    Q_ASSERT(!d->updateTimer.isActive());
    d->updateTimer.start(updateInterval, Qt::PreciseTimer, window());
}

/*!
    Returns true if the window has a pending update request.

    \sa requestUpdate(), deliverUpdateRequest()
*/
bool QPlatformWindow::hasPendingUpdateRequest() const
{
    return qt_window_private(window())->updateRequestPending;
}

/*!
    Delivers an QEvent::UpdateRequest event to the window.

    QPlatformWindow subclasses can re-implement this function to
    provide e.g. logging or tracing of the delivery, but should
    always call the base class function.
*/
void QPlatformWindow::deliverUpdateRequest()
{
    Q_ASSERT(hasPendingUpdateRequest());

    QWindow *w = window();
    QWindowPrivate *wp = qt_window_private(w);
    wp->updateRequestPending = false;
    QEvent request(QEvent::UpdateRequest);
    QCoreApplication::sendEvent(w, &request);
}

/*!
    Returns the QWindow minimum size.
*/
QSize QPlatformWindow::windowMinimumSize() const
{
    return constrainWindowSize(QHighDpi::toNativePixels(window()->minimumSize(), window()));
}

/*!
    Returns the QWindow maximum size.
*/
QSize QPlatformWindow::windowMaximumSize() const
{
    return constrainWindowSize(QHighDpi::toNativePixels(window()->maximumSize(), window()));
}

/*!
    Returns the QWindow base size.
*/
QSize QPlatformWindow::windowBaseSize() const
{
    return QHighDpi::toNativePixels(window()->baseSize(), window());
}

/*!
    Returns the QWindow size increment.
*/
QSize QPlatformWindow::windowSizeIncrement() const
{
    QSize increment = window()->sizeIncrement();
    if (!QHighDpiScaling::isActive())
        return increment;

    // Normalize the increment. If not set the increment can be
    // (-1, -1) or (0, 0). Make that (1, 1) which is scalable.
    if (increment.isEmpty())
        increment = QSize(1, 1);

    return QHighDpi::toNativePixels(increment, window());
}

/*!
    Returns the QWindow geometry.
*/
QRect QPlatformWindow::windowGeometry() const
{
    return QHighDpi::toNativePixels(window()->geometry(), window());
}

/*!
    Returns the QWindow frame geometry.
*/
QRect QPlatformWindow::windowFrameGeometry() const
{
    return QHighDpi::toNativePixels(window()->frameGeometry(), window());
}

/*!
    Returns the closest acceptable geometry for a given geometry before
    a resize/move event for platforms that support it, for example to
    implement heightForWidth().
*/

QRectF QPlatformWindow::closestAcceptableGeometry(const QWindow *qWindow, const QRectF &nativeRect)
{
    const QRectF rectF = QHighDpi::fromNativePixels(nativeRect, qWindow);
    const QRectF correctedGeometryF = qt_window_private(const_cast<QWindow *>(qWindow))->closestAcceptableGeometry(rectF);
    return !correctedGeometryF.isEmpty() && rectF != correctedGeometryF
        ? QHighDpi::toNativePixels(correctedGeometryF, qWindow) : nativeRect;
}

QRectF QPlatformWindow::windowClosestAcceptableGeometry(const QRectF &nativeRect) const
{
    return QPlatformWindow::closestAcceptableGeometry(window(), nativeRect);
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
    all windows to have a QBackingStore. This is the case for QOpenGLWindow. And could be the case for
    windows where some third party renders into it.

    The platform specific window handle can be retrieved by the winId function.

    QPlatformWindow is also the way QPA defines how native child windows should be supported
    through the setParent function.

    \section1 Implementation Aspects

    \list 1
        \li Mouse grab: Qt expects windows to automatically grab the mouse if the user presses
            a button until the button is released.
            Automatic grab should be released if some window is explicitly grabbed.
        \li Enter/Leave events: If there is a window explicitly grabbing mouse events
            (\c{setMouseGrabEnabled()}), enter and leave events should only be sent to the
            grabbing window when mouse cursor passes over the grabbing window boundary.
            Other windows will not receive enter or leave events while the grab is active.
            While an automatic mouse grab caused by a mouse button press is active, no window
            will receive enter or leave events. When the last mouse button is released, the
            autograbbing window will receive leave event if mouse cursor is no longer within
            the window boundary.
            When any grab starts, the window under cursor will receive a leave event unless
            it is the grabbing window.
            When any grab ends, the window under cursor will receive an enter event unless it
            was the grabbing window.
        \li Window positioning: When calling \c{QWindow::setFramePosition()}, the flag
            \c{QWindowPrivate::positionPolicy} is set to \c{QWindowPrivate::WindowFrameInclusive}.
            This means the position includes the window frame, whose size is at this point
            unknown and the geometry's topleft point is the position of the window frame.
    \endlist

    Apart from the auto-tests (\c{tests/auto/gui/kernel/qwindow},
    \c{tests/auto/gui/kernel/qguiapplication} and \c{tests/auto/widgets/kernel/qwidget}),
    there are a number of manual tests and examples that can help testing a platform plugin:

    \list 1
        \li \c{examples/qpa/windows}: Basic \c{QWindow} creation.
        \li \c{examples/opengl/hellowindow}: Basic Open GL windows.
        \li \c{tests/manual/windowflags}: Tests setting the window flags.
        \li \c{tests/manual/windowgeometry} Tests setting the window geometry.
        \li \c{tests/manual/windowmodality} Tests setting the window modality.
        \li \c{tests/manual/widgetgrab} Tests mouse grab and dialogs.
    \endlist

    \sa QBackingStore, QWindow
*/

QT_END_NAMESPACE
