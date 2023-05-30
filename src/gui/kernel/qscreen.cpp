// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

    \note Both physical and logical DPI are expressed in device-independent dots.
    Multiply by QScreen::devicePixelRatio() to get device-dependent density.

    \inmodule QtGui
*/

QScreen::QScreen(QPlatformScreen *platformScreen)
    : QObject(*new QScreenPrivate(), nullptr)
{
    Q_D(QScreen);

    d->platformScreen = platformScreen;
    platformScreen->d_func()->screen = this;

    d->orientation = platformScreen->orientation();
    d->logicalDpi = QPlatformScreen::overrideDpi(platformScreen->logicalDpi());
    d->refreshRate = platformScreen->refreshRate();
    // safeguard ourselves against buggy platform behavior...
    if (d->refreshRate < 1.0)
        d->refreshRate = 60.0;

    d->updateGeometry();
    d->updatePrimaryOrientation(); // derived from the geometry
}

void QScreenPrivate::updateGeometry()
{
    qreal scaleFactor = QHighDpiScaling::factor(platformScreen);
    QRect nativeGeometry = platformScreen->geometry();
    geometry = QRect(nativeGeometry.topLeft(), QHighDpi::fromNative(nativeGeometry.size(), scaleFactor));
    availableGeometry = QHighDpi::fromNative(platformScreen->availableGeometry(), scaleFactor, geometry.topLeft());
}

/*!
    Destroys the screen.

    \internal
 */
QScreen::~QScreen()
{
    Q_ASSERT_X(!QGuiApplicationPrivate::screen_list.contains(this), "QScreen",
        "QScreens should be removed via QWindowSystemInterface::handleScreenRemoved()");
}

/*!
  Get the platform screen handle.

  \sa {Qt Platform Abstraction}{Qt Platform Abstraction (QPA)}
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

  \note The user presentable string is not guaranteed to match the
  result of any native APIs, and should not be used to uniquely identify
  a screen.
*/
QString QScreen::name() const
{
    Q_D(const QScreen);
    return d->platformScreen->name();
}

/*!
  \property QScreen::manufacturer
  \brief the manufacturer of the screen

  \since 5.9
*/
QString QScreen::manufacturer() const
{
    Q_D(const QScreen);
    return d->platformScreen->manufacturer();
}

/*!
  \property QScreen::model
  \brief the model of the screen

  \since 5.9
*/
QString QScreen::model() const
{
    Q_D(const QScreen);
    return d->platformScreen->model();
}

/*!
  \property QScreen::serialNumber
  \brief the serial number of the screen

  \since 5.9
*/
QString QScreen::serialNumber() const
{
    Q_D(const QScreen);
    return d->platformScreen->serialNumber();
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

  \note Physical DPI is expressed in device-independent dots. Multiply by QScreen::devicePixelRatio()
  to get device-dependent density.

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

  \note Physical DPI is expressed in device-independent dots. Multiply by QScreen::devicePixelRatio()
  to get device-dependent density.

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

  \note Physical DPI is expressed in device-independent dots. Multiply by QScreen::devicePixelRatio()
  to get device-dependent density.

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
        return QHighDpiScaling::logicalDpi(this).first;
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
        return QHighDpiScaling::logicalDpi(this).second;
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
    QDpi dpi = QHighDpiScaling::isActive() ? QHighDpiScaling::logicalDpi(this) : d->logicalDpi;
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

  Note, on X11 this will return the true available geometry only on systems with one monitor and
  if window manager has set _NET_WORKAREA atom. In all other cases this is equal to geometry().
  This is a limitation in X11 window manager specification.
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
    screens.reserve(platformScreens.size());
    for (QPlatformScreen *platformScreen : platformScreens) {
        // Only consider platform screens that have been added
        if (auto *knownScreen = platformScreen->screen())
            screens << knownScreen;
    }
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
    \property QScreen::orientation
    \brief the screen orientation

    The \c orientation property tells the orientation of the screen from the
    window system perspective.

    Most mobile devices and tablet computers contain accelerometer sensors.
    The Qt Sensors module provides the ability to read this sensor directly.
    However, the windowing system may rotate the entire screen automatically
    based on how it is being held; in that case, this \c orientation property
    will change.

    \sa primaryOrientation(), QWindow::contentOrientation()
*/
Qt::ScreenOrientation QScreen::orientation() const
{
    Q_D(const QScreen);
    return d->orientation;
}

/*!
  \property QScreen::refreshRate
  \brief the approximate vertical refresh rate of the screen in Hz

  \warning Avoid using the screen's refresh rate to drive animations
  via a timer such as QTimer. Instead use QWindow::requestUpdate().

  \sa QWindow::requestUpdate()
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
    Returns the screen at \a point within the set of \l QScreen::virtualSiblings(),
    or \c nullptr if outside of any screen.

    The \a point is in relation to the virtualGeometry() of each set of virtual
    siblings.

    \since 5.15
*/
QScreen *QScreen::virtualSiblingAt(QPoint point)
{
    const auto &siblings = virtualSiblings();
    for (QScreen *sibling : siblings) {
        if (sibling->geometry().contains(point))
            return sibling;
    }
    return nullptr;
}

/*!
    Creates and returns a pixmap constructed by grabbing the contents
    of the given \a window restricted by QRect(\a x, \a y, \a width,
    \a height). If \a window is 0, then the entire screen will be
    grabbed.

    The arguments (\a{x}, \a{y}) specify the offset in the window,
    whereas (\a{width}, \a{height}) specify the area to be copied. If
    \a width is negative, the function copies everything to the right
    border of the window. If \a height is negative, the function
    copies everything to the bottom of the window.

    The offset and size arguments are specified in device independent
    pixels. The returned pixmap may be larger than the requested size
    when grabbing from a high-DPI screen. Call QPixmap::devicePixelRatio()
    to determine if this is the case.

    The window system identifier (\c WId) can be retrieved using the
    QWidget::winId() function. The rationale for using a window
    identifier and not a QWidget, is to enable grabbing of windows
    that are not part of the application, window system frames, and so
    on.

    \warning Grabbing windows that are not part of the application is
    not supported on systems such as iOS, where sandboxing/security
    prevents reading pixels of windows not owned by the application.

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
    result.setDevicePixelRatio(result.devicePixelRatio() * factor);
    return result;
}
void *QScreen::resolveInterface(const char *name, int revision) const
{
    using namespace QNativeInterface::Private;

    auto *platformScreen = handle();
    Q_UNUSED(platformScreen);
    Q_UNUSED(name);
    Q_UNUSED(revision);

#if QT_CONFIG(xcb)
    QT_NATIVE_INTERFACE_RETURN_IF(QXcbScreen, platformScreen);
#endif

#if QT_CONFIG(vsp2)
    QT_NATIVE_INTERFACE_RETURN_IF(QVsp2Screen, platformScreen);
#endif

#if defined(Q_OS_WEBOS)
    QT_NATIVE_INTERFACE_RETURN_IF(QWebOSScreen, platformScreen);
#endif

#if defined(Q_OS_WIN32)
    QT_NATIVE_INTERFACE_RETURN_IF(QWindowsScreen, platformScreen);
#endif

#if defined(Q_OS_ANDROID)
    QT_NATIVE_INTERFACE_RETURN_IF(QAndroidScreen, platformScreen);
#endif

#if defined(Q_OS_UNIX)
    QT_NATIVE_INTERFACE_RETURN_IF(QWaylandScreen, platformScreen);
#endif

    return nullptr;
}

#ifndef QT_NO_DEBUG_STREAM
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
            debug << ", geometry=" << screen->geometry();
            debug << ", available=" << screen->availableGeometry();
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

QScreenPrivate::UpdateEmitter::UpdateEmitter(QScreen *screen)
{
    initialState.platformScreen = screen->handle();

    // Use public APIs to read out current state, rather
    // than accessing the QScreenPrivate members, so that
    // we detect any changes to the high-DPI scale factors
    // that may be applied in the getters.

    initialState.logicalDpi = QDpi{
        screen->logicalDotsPerInchX(),
        screen->logicalDotsPerInchY()
    };
    initialState.geometry = screen->geometry();
    initialState.availableGeometry = screen->availableGeometry();
    initialState.primaryOrientation = screen->primaryOrientation();
}

QScreenPrivate::UpdateEmitter::~UpdateEmitter()
{
    QScreen *screen = initialState.platformScreen->screen();

    const auto logicalDotsPerInch = QDpi{
        screen->logicalDotsPerInchX(),
        screen->logicalDotsPerInchY()
    };
    if (logicalDotsPerInch != initialState.logicalDpi)
        emit screen->logicalDotsPerInchChanged(screen->logicalDotsPerInch());

    const auto geometry = screen->geometry();
    const auto geometryChanged = geometry != initialState.geometry;
    if (geometryChanged)
        emit screen->geometryChanged(geometry);

    const auto availableGeometry = screen->availableGeometry();
    const auto availableGeometryChanged = availableGeometry != initialState.availableGeometry;
    if (availableGeometryChanged)
        emit screen->availableGeometryChanged(availableGeometry);

    if (geometryChanged || availableGeometryChanged) {
        const auto siblings = screen->virtualSiblings();
        for (QScreen* sibling : siblings)
            emit sibling->virtualGeometryChanged(sibling->virtualGeometry());
    }

    if (geometryChanged) {
        emit screen->physicalDotsPerInchChanged(screen->physicalDotsPerInch());

        const auto primaryOrientation = screen->primaryOrientation();
        if (primaryOrientation != initialState.primaryOrientation)
            emit screen->primaryOrientationChanged(primaryOrientation);
    }
}

QT_END_NAMESPACE

#include "moc_qscreen.cpp"
