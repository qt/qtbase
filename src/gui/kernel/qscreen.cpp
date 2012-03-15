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

#include "qscreen.h"
#include "qscreen_p.h"
#include "qpixmap.h"
#include "qplatformscreen_qpa.h"

#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QScreen
    \since 5.0
    \brief The QScreen class is used to query screen properties.

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
    : QObject(*new QScreenPrivate(screen), 0)
{
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
    QDpi dpi = d->logicalDpi;
    return (dpi.first + dpi.second) * qreal(0.5);
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
    QList<QPlatformScreen *> platformScreens = d->platformScreen->virtualSiblings();
    QList<QScreen *> screens;
    foreach (QPlatformScreen *platformScreen, platformScreens)
        screens << platformScreen->screen();
    return screens;
}

/*!
  \property QScreen::virtualSize
  \brief the pixel size of the virtual desktop corresponding to this screen

  This is the combined size of the virtual siblings' individual geometries.

  \sa virtualSiblings()
*/
QSize QScreen::virtualSize() const
{
    return virtualGeometry().size();
}

/*!
  \property QScreen::virtualGeometry
  \brief the pixel geometry of the virtual desktop corresponding to this screen

  This is the union of the virtual siblings' individual geometries.

  \sa virtualSiblings()
*/
QRect QScreen::virtualGeometry() const
{
    QRect result;
    foreach (QScreen *screen, virtualSiblings())
        result |= screen->geometry();
    return result;
}

/*!
  \property QScreen::availableVirtualSize
  \brief the available pixel size of the virtual desktop corresponding to this screen

  This is the combined size of the virtual siblings' individual available geometries.

  \sa availableSize()
  \sa virtualSiblings()
*/
QSize QScreen::availableVirtualSize() const
{
    return availableVirtualGeometry().size();
}

/*!
  \property QScreen::availableVirtualGeometry
  \brief the available size of the virtual desktop corresponding to this screen

  This is the union of the virtual siblings' individual available geometries.

  \sa availableGeometry()
  \sa virtualSiblings()
*/
QRect QScreen::availableVirtualGeometry() const
{
    QRect result;
    foreach (QScreen *screen, virtualSiblings())
        result |= screen->availableGeometry();
    return result;
}

/*!
    \property QScreen::orientation
    \brief the screen orientation

    The screen orientation represents the physical orientation
    of the display. For example, the screen orientation of a mobile device
    will change based on the device is being held, and a desktop display
    might be rotated so that it's in portrait mode.

    Qt::PrimaryOrientation is never returned.

    \sa primaryOrientation(), orientationChanged()
*/
Qt::ScreenOrientation QScreen::orientation() const
{
    Q_D(const QScreen);
    return d->orientation == Qt::PrimaryOrientation ? primaryOrientation() : d->orientation;
}

/*!
    \property QScreen::primaryOrientation
    \brief the primary screen orientation

    The primary screen orientation is Qt::LandscapeOrientation
    if the screen geometry's width is greater than or equal to its
    height, or Qt::PortraitOrientation otherwise.

    \sa primaryOrientationChanged()
*/
Qt::ScreenOrientation QScreen::primaryOrientation() const
{
    Q_D(const QScreen);
    return d->primaryOrientation;
}

// i must be power of two
static int log2(uint i)
{
    if (i == 0)
        return -1;

    int result = 0;
    while (!(i & 1)) {
        ++result;
        i >>= 1;
    }
    return result;
}

/*!
    Convenience function to compute the angle of rotation to get from
    rotation \a a to rotation \a b.

    The result will be 0, 90, 180, or 270.

    Qt::PrimaryOrientation is interpreted as the screen's primaryOrientation().
*/
int QScreen::angleBetween(Qt::ScreenOrientation a, Qt::ScreenOrientation b)
{
    if (a == Qt::PrimaryOrientation)
        a = primaryOrientation();

    if (b == Qt::PrimaryOrientation)
        b = primaryOrientation();

    if (a == b)
        return 0;

    int ia = log2(uint(a));
    int ib = log2(uint(b));

    int delta = ia - ib;

    if (delta < 0)
        delta = delta + 4;

    int angles[] = { 0, 90, 180, 270 };
    return angles[delta];
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
QTransform QScreen::transformBetween(Qt::ScreenOrientation a, Qt::ScreenOrientation b, const QRect &target)
{
    if (a == Qt::PrimaryOrientation)
        a = primaryOrientation();

    if (b == Qt::PrimaryOrientation)
        b = primaryOrientation();

    if (a == b)
        return QTransform();

    int angle = angleBetween(a, b);

    QTransform result;
    switch (angle) {
    case 90:
        result.translate(target.width(), 0);
        break;
    case 180:
        result.translate(target.width(), target.height());
        break;
    case 270:
        result.translate(0, target.height());
        break;
    default:
        Q_ASSERT(false);
    }
    result.rotate(angle);

    return result;
}

/*!
    Maps the rect between two screen orientations.

    This will flip the x and y dimensions of the rectangle if orientation \a is
    Qt::PortraitOrientation or Qt::InvertedPortraitOrientation and orientation \b is
    Qt::LandscapeOrientation or Qt::InvertedLandscapeOrientation, or vice versa.

    Qt::PrimaryOrientation is interpreted as the screen's primaryOrientation().
*/
QRect QScreen::mapBetween(Qt::ScreenOrientation a, Qt::ScreenOrientation b, const QRect &rect)
{
    if (a == Qt::PrimaryOrientation)
        a = primaryOrientation();

    if (b == Qt::PrimaryOrientation)
        b = primaryOrientation();

    if (a == b)
        return rect;

    if ((a == Qt::PortraitOrientation || a == Qt::InvertedPortraitOrientation)
        != (b == Qt::PortraitOrientation || b == Qt::InvertedPortraitOrientation))
    {
        return QRect(rect.y(), rect.x(), rect.height(), rect.width());
    }

    return rect;
}

/*!
    Convenience function to check if a screen orientation is either portrait
    or inverted portrait.

    Qt::PrimaryOrientation is interpreted as the screen's primaryOrientation().
*/
bool QScreen::isPortrait(Qt::ScreenOrientation o)
{
    return o == Qt::PortraitOrientation || o == Qt::InvertedPortraitOrientation
        || (o == Qt::PrimaryOrientation && primaryOrientation() == Qt::PortraitOrientation);
}

/*!
    Convenience function to check if a screen orientation is either landscape
    or inverted landscape.

    Qt::PrimaryOrientation is interpreted as the screen's primaryOrientation().
*/
bool QScreen::isLandscape(Qt::ScreenOrientation o)
{
    return o == Qt::LandscapeOrientation || o == Qt::InvertedLandscapeOrientation
        || (o == Qt::PrimaryOrientation && primaryOrientation() == Qt::LandscapeOrientation);
}

/*!
    \fn QScreen::orientationChanged(Qt::ScreenOrientation orientation)

    This signal is emitted when the orientation of the screen
    changes.

    \sa orientation()
*/

/*!
    \fn QScreen::primaryOrientationChanged(Qt::ScreenOrientation orientation)

    This signal is emitted when the primary orientation of the screen
    changes.

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
    \since 5.0
*/

QPixmap QScreen::grabWindow(WId window, int x, int y, int w, int h) const
{
    const QPlatformScreen *platformScreen = handle();
    if (!platformScreen) {
        qWarning("%s invoked with handle==0", Q_FUNC_INFO);
        return QPixmap();
    }
    return platformScreen->grabWindow(window, x, y, w, h);
}

QT_END_NAMESPACE
