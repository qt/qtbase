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

#include "qscreen.h"
#include "qscreen_p.h"
#include "qpixmap.h"
#include "qguiapplication_p.h"
#include <qpa/qplatformscreen.h>
#include <qpa/qplatformscreen_p.h>

#include <QtCore/QDebug>
#include <QtCore/private/qobject_p.h>
#include "qhighdpiscaling_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QScreen
    \since 5.0
    \brief The QScreen class is used to query screen properties.
    \inmodule QtGui

    A note on logical vs physical dots per inch: physical DPI is based on the
    actual physical pixel sizes when available, and is useful for print preview
    and other cases where it's desirable to know the exact physical dimensions
    of screen displayed contents.

    Logical dots per inch are used to convert font and user interface elements
    from point sizes to pixel sizes, and might be different from the physical
    dots per inch. The logical dots per inch are sometimes user-settable in the
    desktop environment's settings panel, to let the user globally control UI
    and font sizes in different applications.

    \inmodule QtGui
*/

QScreen::QScreen(QPlatformScreen *screen)
    : QObject(*new QScreenPrivate(), 0)
{
    Q_D(QScreen);
    d->setPlatformScreen(screen);
}

void QScreenPrivate::setPlatformScreen(QPlatformScreen *screen)
{
    Q_Q(QScreen);
    platformScreen = screen;
    platformScreen->d_func()->screen = q;
    orientation = platformScreen->orientation();
    geometry = platformScreen->deviceIndependentGeometry();
    availableGeometry = QHighDpi::fromNative(platformScreen->availableGeometry(), QHighDpiScaling::factor(platformScreen), geometry.topLeft());
    logicalDpi = platformScreen->logicalDpi();
    refreshRate = platformScreen->refreshRate();
    // safeguard ourselves against buggy platform behavior...
    if (refreshRate < 1.0)
        refreshRate = 60.0;

    updatePrimaryOrientation();

    filteredOrientation = orientation;
    if (filteredOrientation == Qt::PrimaryOrientation)
        filteredOrientation = primaryOrientation;

    updateHighDpi();
}


/*!
    Destroys the screen.
 */
QScreen::~QScreen()
{
    if (!qApp)
        return;

    // Allow clients to manage windows that are affected by the screen going
    // away, before we fall back to moving them to the primary screen.
    emit qApp->screenRemoved(this);

    if (QGuiApplication::closingDown())
        return;

    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    if (this == primaryScreen)
        return;

    bool movingFromVirtualSibling = primaryScreen && primaryScreen->handle()->virtualSiblings().contains(handle());

    // Move any leftover windows to the primary screen
    const auto allWindows = QGuiApplication::allWindows();
    for (QWindow *window : allWindows) {
        if (!window->isTopLevel() || window->screen() != this)
            continue;

        const bool wasVisible = window->isVisible();
        window->setScreen(primaryScreen);

        // Re-show window if moved from a virtual sibling screen. Otherwise
        // leave it up to the application developer to show the window.
        if (movingFromVirtualSibling)
            window->setVisible(wasVisible);
    }
}

/*!
  Get the platform screen handle.
*/
QPlatformScreen *QScreen::handle() const
{
    Q_D(const QScreen);
    return d->platformScreen;
}

/*!
  \property QScreen::name
  \brief a user presentable string representing the screen

  For example, on X11 these correspond to the XRandr screen names,
  typically "VGA1", "HDMI1", etc.
*/
QString QScreen::name() const
{
    Q_D(const QScreen);
    return d->platformScreen->name();
}

/*!
  \property QScreen::depth
  \brief the color depth of the screen
*/
int QScreen::depth() const
{
    Q_D(const QScreen);
    return d->platformScreen->depth();
}

/*!
  \property QScreen::size
  \brief the pixel resolution of the screen
*/
QSize QScreen::size() const
{
    Q_D(const QScreen);
    return d->geometry.size();
}

/*!
  \property QScreen::physicalDotsPerInchX
  \brief the number of physical dots or pixels per inch in the horizontal direction

  This value represents the actual horizontal pixel density on the screen's display.
  Depending on what information the underlying system provides the value might not be
  entirely accurate.

  \sa physicalDotsPerInchY()
*/
qreal QScreen::physicalDotsPerInchX() const
{
    return size().width() / physicalSize().width() * qreal(25.4);
}

/*!
  \property QScreen::physicalDotsPerInchY
  \brief the number of physical dots or pixels per inch in the vertical direction

  This value represents the actual vertical pixel density on the screen's display.
  Depending on what information the underlying system provides the value might not be
  entirely accurate.

  \sa physicalDotsPerInchX()
*/
qreal QScreen::physicalDotsPerInchY() const
{
    return size().height() / physicalSize().height() * qreal(25.4);
}

/*!
  \property QScreen::physicalDotsPerInch
  \brief the number of physical dots or pixels per inch

  This value represents the pixel density on the screen's display.
  Depending on what information the underlying system provides the value might not be
  entirely accurate.

  This is a convenience property that's simply the average of the physicalDotsPerInchX
  and physicalDotsPerInchY properties.

  \sa physicalDotsPerInchX()
  \sa physicalDotsPerInchY()
*/
qreal QScreen::physicalDotsPerInch() const
{
    QSize sz = size();
    QSizeF psz = physicalSize();
    return ((sz.height() / psz.height()) + (sz.width() / psz.width())) * qreal(25.4 * 0.5);
}

/*!
  \property QScreen::logicalDotsPerInchX
  \brief the number of logical dots or pixels per inch in the horizontal direction

  This value is used to convert font point sizes to pixel sizes.

  \sa logicalDotsPerInchY()
*/
qreal QScreen::logicalDotsPerInchX() const
{
    Q_D(const QScreen);
    if (QHighDpiScaling::isActive())
        return QHighDpiScaling::logicalDpi().first;
    return d->logicalDpi.first;
}

/*!
  \property QScreen::logicalDotsPerInchY
  \brief the number of logical dots or pixels per inch in the vertical direction

  This value is used to convert font point sizes to pixel sizes.

  \sa logicalDotsPerInchX()
*/
qreal QScreen::logicalDotsPerInchY() const
{
    Q_D(const QScreen);
    if (QHighDpiScaling::isActive())
        return QHighDpiScaling::logicalDpi().second;
    return d->logicalDpi.second;
}

/*!
  \property QScreen::logicalDotsPerInch
  \brief the number of logical dots or pixels per inch

  This value can be used to convert font point sizes to pixel sizes.

  This is a convenience property that's simply the average of the logicalDotsPerInchX
  and logicalDotsPerInchY properties.

  \sa logicalDotsPerInchX()
  \sa logicalDotsPerInchY()
*/
qreal QScreen::logicalDotsPerInch() const
{
    Q_D(const QScreen);
    QDpi dpi = QHighDpiScaling::isActive() ? QHighDpiScaling::logicalDpi() : d->logicalDpi;
    return (dpi.first + dpi.second) * qreal(0.5);
}

/*!
    \property QScreen::devicePixelRatio
    \brief the screen's ratio between physical pixels and device-independent pixels
    \since 5.5

    Returns the ratio between physical pixels and device-independent pixels for the screen.

    Common values are 1.0 on normal displays and 2.0 on "retina" displays.
    Higher values are also possible.

    \sa QWindow::devicePixelRatio(), QGuiApplication::devicePixelRatio()
*/
qreal QScreen::devicePixelRatio() const
{
    Q_D(const QScreen);
    return d->platformScreen->devicePixelRatio() * QHighDpiScaling::factor(this);
}

/*!
  \property QScreen::physicalSize
  \brief the screen's physical size (in millimeters)

  The physical size represents the actual physical dimensions of the
  screen's display.

  Depending on what information the underlying system provides the value
  might not be entirely accurate.
*/
QSizeF QScreen::physicalSize() const
{
    Q_D(const QScreen);
    return d->platformScreen->physicalSize();
}

/*!
  \property QScreen::availableSize
  \brief the screen's available size in pixels

  The available size is the size excluding window manager reserved areas
  such as task bars and system menus.
*/
QSize QScreen::availableSize() const
{
    Q_D(const QScreen);
    return d->availableGeometry.size();
}

/*!
  \property QScreen::geometry
  \brief the screen's geometry in pixels

  As an example this might return QRect(0, 0, 1280, 1024), or in a
  virtual desktop setting QRect(1280, 0, 1280, 1024).
*/
QRect QScreen::geometry() const
{
    Q_D(const QScreen);
    return d->geometry;
}

/*!
  \property QScreen::availableGeometry
  \brief the screen's available geometry in pixels

  The available geometry is the geometry excluding window manager reserved areas
  such as task bars and system menus.
*/
QRect QScreen::availableGeometry() const
{
    Q_D(const QScreen);
    return d->availableGeometry;
}

/*!
  Get the screen's virtual siblings.

  The virtual siblings are the screen instances sharing the same virtual desktop.
  They share a common coordinate system, and windows can freely be moved or
  positioned across them without having to be re-created.
*/
QList<QScreen *> QScreen::virtualSiblings() const
{
    Q_D(const QScreen);
    const QList<QPlatformScreen *> platformScreens = d->platformScreen->virtualSiblings();
    QList<QScreen *> screens;
    screens.reserve(platformScreens.count());
    for (QPlatformScreen *platformScreen : platformScreens)
        screens << platformScreen->screen();
    return screens;
}

/*!
    \property QScreen::virtualSize
    \brief the pixel size of the virtual desktop to which this screen belongs

  Returns the pixel size of the virtual desktop corresponding to this screen.

  This is the combined size of the virtual siblings' individual geometries.

  \sa virtualSiblings()
*/
QSize QScreen::virtualSize() const
{
    return virtualGeometry().size();
}

/*!
    \property QScreen::virtualGeometry
    \brief the pixel geometry of the virtual desktop to which this screen belongs

  Returns the pixel geometry of the virtual desktop corresponding to this screen.

  This is the union of the virtual siblings' individual geometries.

  \sa virtualSiblings()
*/
QRect QScreen::virtualGeometry() const
{
    QRect result;
    const auto screens = virtualSiblings();
    for (QScreen *screen : screens)
        result |= screen->geometry();
    return result;
}

/*!
    \property QScreen::availableVirtualSize
    \brief the available size of the virtual desktop to which this screen belongs

  Returns the available pixel size of the virtual desktop corresponding to this screen.

  This is the combined size of the virtual siblings' individual available geometries.

  \sa availableSize(), virtualSiblings()
*/
QSize QScreen::availableVirtualSize() const
{
    return availableVirtualGeometry().size();
}

/*!
    \property QScreen::availableVirtualGeometry
    \brief the available geometry of the virtual desktop to which this screen belongs

  Returns the available geometry of the virtual desktop corresponding to this screen.

  This is the union of the virtual siblings' individual available geometries.

  \sa availableGeometry(), virtualSiblings()
*/
QRect QScreen::availableVirtualGeometry() const
{
    QRect result;
    const auto screens = virtualSiblings();
    for (QScreen *screen : screens)
        result |= screen->availableGeometry();
    return result;
}

/*!
    Sets the orientations that the application is interested in receiving
    updates for in conjunction with this screen.

    For example, to receive orientation() updates and thus have
    orientationChanged() signals being emitted for LandscapeOrientation and
    InvertedLandscapeOrientation, call setOrientationUpdateMask() with
    \a{mask} set to Qt::LandscapeOrientation | Qt::InvertedLandscapeOrientation.

    The default, 0, means no orientationChanged() signals are fired.
*/
void QScreen::setOrientationUpdateMask(Qt::ScreenOrientations mask)
{
    Q_D(QScreen);
    d->orientationUpdateMask = mask;
    d->platformScreen->setOrientationUpdateMask(mask);
    QGuiApplicationPrivate::updateFilteredScreenOrientation(this);
}

/*!
    Returns the currently set orientation update mask.

    \sa setOrientationUpdateMask()
*/
Qt::ScreenOrientations QScreen::orientationUpdateMask() const
{
    Q_D(const QScreen);
    return d->orientationUpdateMask;
}

/*!
    \property QScreen::orientation
    \brief the screen orientation

    The screen orientation represents the physical orientation
    of the display. For example, the screen orientation of a mobile device
    will change based on how it is being held. A change to the orientation
    might or might not trigger a change to the primary orientation of the screen.

    Changes to this property will be filtered by orientationUpdateMask(),
    so in order to receive orientation updates the application must first
    call setOrientationUpdateMask() with a mask of the orientations it wants
    to receive.

    Qt::PrimaryOrientation is never returned.

    \sa primaryOrientation()
*/
Qt::ScreenOrientation QScreen::orientation() const
{
    Q_D(const QScreen);
    return d->filteredOrientation;
}

/*!
  \property QScreen::refreshRate
  \brief the approximate vertical refresh rate of the screen in Hz
*/
qreal QScreen::refreshRate() const
{
    Q_D(const QScreen);
    return d->refreshRate;
}

/*!
    \property QScreen::primaryOrientation
    \brief the primary screen orientation

    The primary screen orientation is Qt::LandscapeOrientation
    if the screen geometry's width is greater than or equal to its
    height, or Qt::PortraitOrientation otherwise. This property might
    change when the screen orientation was changed (i.e. when the
    display is rotated).
    The behavior is however platform dependent and can often be specified in
    an application manifest file.

*/
Qt::ScreenOrientation QScreen::primaryOrientation() const
{
    Q_D(const QScreen);
    return d->primaryOrientation;
}

/*!
    \property QScreen::nativeOrientation
    \brief the native screen orientation
    \since 5.2

    The native orientation of the screen is the orientation where the logo
    sticker of the device appears the right way up, or Qt::PrimaryOrientation
    if the platform does not support this functionality.

    The native orientation is a property of the hardware, and does not change.
*/
Qt::ScreenOrientation QScreen::nativeOrientation() const
{
    Q_D(const QScreen);
    return d->platformScreen->nativeOrientation();
}

/*!
    Convenience function to compute the angle of rotation to get from
    rotation \a a to rotation \a b.

    The result will be 0, 90, 180, or 270.

    Qt::PrimaryOrientation is interpreted as the screen's primaryOrientation().
*/
int QScreen::angleBetween(Qt::ScreenOrientation a, Qt::ScreenOrientation b) const
{
    if (a == Qt::PrimaryOrientation)
        a = primaryOrientation();

    if (b == Qt::PrimaryOrientation)
        b = primaryOrientation();

    return QPlatformScreen::angleBetween(a, b);
}

/*!
    Convenience function to compute a transform that maps from the coordinate system
    defined by orientation \a a into the coordinate system defined by orientation
    \a b and target dimensions \a target.

    Example, \a a is Qt::Landscape, \a b is Qt::Portrait, and \a target is QRect(0, 0, w, h)
    the resulting transform will be such that the point QPoint(0, 0) is mapped to QPoint(0, w),
    and QPoint(h, w) is mapped to QPoint(0, h). Thus, the landscape coordinate system QRect(0, 0, h, w)
    is mapped (with a 90 degree rotation) into the portrait coordinate system QRect(0, 0, w, h).

    Qt::PrimaryOrientation is interpreted as the screen's primaryOrientation().
*/
QTransform QScreen::transformBetween(Qt::ScreenOrientation a, Qt::ScreenOrientation b, const QRect &target) const
{
    if (a == Qt::PrimaryOrientation)
        a = primaryOrientation();

    if (b == Qt::PrimaryOrientation)
        b = primaryOrientation();

    return QPlatformScreen::transformBetween(a, b, target);
}

/*!
    Maps the rect between two screen orientations.

    This will flip the x and y dimensions of the rectangle \a{rect} if the orientation \a{a} is
    Qt::PortraitOrientation or Qt::InvertedPortraitOrientation and orientation \a{b} is
    Qt::LandscapeOrientation or Qt::InvertedLandscapeOrientation, or vice versa.

    Qt::PrimaryOrientation is interpreted as the screen's primaryOrientation().
*/
QRect QScreen::mapBetween(Qt::ScreenOrientation a, Qt::ScreenOrientation b, const QRect &rect) const
{
    if (a == Qt::PrimaryOrientation)
        a = primaryOrientation();

    if (b == Qt::PrimaryOrientation)
        b = primaryOrientation();

    return QPlatformScreen::mapBetween(a, b, rect);
}

/*!
    Convenience function that returns \c true if \a o is either portrait or inverted portrait;
    otherwise returns \c false.

    Qt::PrimaryOrientation is interpreted as the screen's primaryOrientation().
*/
bool QScreen::isPortrait(Qt::ScreenOrientation o) const
{
    return o == Qt::PortraitOrientation || o == Qt::InvertedPortraitOrientation
        || (o == Qt::PrimaryOrientation && primaryOrientation() == Qt::PortraitOrientation);
}

/*!
    Convenience function that returns \c true if \a o is either landscape or inverted landscape;
    otherwise returns \c false.

    Qt::PrimaryOrientation is interpreted as the screen's primaryOrientation().
*/
bool QScreen::isLandscape(Qt::ScreenOrientation o) const
{
    return o == Qt::LandscapeOrientation || o == Qt::InvertedLandscapeOrientation
        || (o == Qt::PrimaryOrientation && primaryOrientation() == Qt::LandscapeOrientation);
}

/*!
    \fn void QScreen::orientationChanged(Qt::ScreenOrientation orientation)

    This signal is emitted when the orientation of the screen
    changes with \a orientation as an argument.

    \sa orientation()
*/

/*!
    \fn void QScreen::primaryOrientationChanged(Qt::ScreenOrientation orientation)

    This signal is emitted when the primary orientation of the screen
    changes with \a orientation as an argument.

    \sa primaryOrientation()
*/

void QScreenPrivate::updatePrimaryOrientation()
{
    primaryOrientation = geometry.width() >= geometry.height() ? Qt::LandscapeOrientation : Qt::PortraitOrientation;
}

/*!
    Creates and returns a pixmap constructed by grabbing the contents
    of the given \a window restricted by QRect(\a x, \a y, \a width,
    \a height).

    The arguments (\a{x}, \a{y}) specify the offset in the window,
    whereas (\a{width}, \a{height}) specify the area to be copied.  If
    \a width is negative, the function copies everything to the right
    border of the window. If \a height is negative, the function
    copies everything to the bottom of the window.

    The window system identifier (\c WId) can be retrieved using the
    QWidget::winId() function. The rationale for using a window
    identifier and not a QWidget, is to enable grabbing of windows
    that are not part of the application, window system frames, and so
    on.

    The grabWindow() function grabs pixels from the screen, not from
    the window, i.e. if there is another window partially or entirely
    over the one you grab, you get pixels from the overlying window,
    too. The mouse cursor is generally not grabbed.

    Note on X11 that if the given \a window doesn't have the same depth
    as the root window, and another window partially or entirely
    obscures the one you grab, you will \e not get pixels from the
    overlying window.  The contents of the obscured areas in the
    pixmap will be undefined and uninitialized.

    On Windows Vista and above grabbing a layered window, which is
    created by setting the Qt::WA_TranslucentBackground attribute, will
    not work. Instead grabbing the desktop widget should work.

    \warning In general, grabbing an area outside the screen is not
    safe. This depends on the underlying window system.
*/

QPixmap QScreen::grabWindow(WId window, int x, int y, int width, int height)
{
    const QPlatformScreen *platformScreen = handle();
    if (!platformScreen) {
        qWarning("invoked with handle==0");
        return QPixmap();
    }
    const qreal factor = QHighDpiScaling::factor(this);
    if (qFuzzyCompare(factor, 1))
        return platformScreen->grabWindow(window, x, y, width, height);

    const QPoint nativePos = QHighDpi::toNative(QPoint(x, y), factor);
    QSize nativeSize(width, height);
    if (nativeSize.isValid())
        nativeSize = QHighDpi::toNative(nativeSize, factor);
    QPixmap result =
        platformScreen->grabWindow(window, nativePos.x(), nativePos.y(),
                                   nativeSize.width(), nativeSize.height());
    result.setDevicePixelRatio(factor);
    return result;
}

#ifndef QT_NO_DEBUG_STREAM

static inline void formatRect(QDebug &debug, const QRect r)
{
    debug << r.width() << 'x' << r.height()
        << forcesign << r.x() << r.y() << noforcesign;
}

Q_GUI_EXPORT QDebug operator<<(QDebug debug, const QScreen *screen)
{
    const QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "QScreen(" << (const void *)screen;
    if (screen) {
        debug << ", name=" << screen->name();
        if (debug.verbosity() > 2) {
            if (screen == QGuiApplication::primaryScreen())
                debug << ", primary";
            debug << ", geometry=";
            formatRect(debug, screen->geometry());
            debug << ", available=";
            formatRect(debug, screen->availableGeometry());
            debug << ", logical DPI=" << screen->logicalDotsPerInchX()
                << ',' << screen->logicalDotsPerInchY()
                << ", physical DPI=" << screen->physicalDotsPerInchX()
                << ',' << screen->physicalDotsPerInchY()
                << ", devicePixelRatio=" << screen->devicePixelRatio()
                << ", orientation=" << screen->orientation()
                << ", physical size=" << screen->physicalSize().width()
                << 'x' << screen->physicalSize().height() << "mm";
        }
    }
    debug << ')';
    return debug;
}
#endif // !QT_NO_DEBUG_STREAM

QT_END_NAMESPACE
