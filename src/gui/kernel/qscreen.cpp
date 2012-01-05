/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qscreen.h"
#include "qplatformscreen_qpa.h"

#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QScreenPrivate : public QObjectPrivate
{
public:
    QScreenPrivate(QPlatformScreen *screen)
        : platformScreen(screen)
    {
    }

    QPlatformScreen *platformScreen;
};

/*!
    \class QScreen
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
    return d->platformScreen->geometry().size();
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
    return d->platformScreen->logicalDpi().first;
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
    return d->platformScreen->logicalDpi().second;
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
    QDpi dpi = d->platformScreen->logicalDpi();
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
    return d->platformScreen->availableGeometry().size();
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
    return d->platformScreen->geometry();
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
    return d->platformScreen->availableGeometry();
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
    Q_D(const QScreen);
    QRect result;
    foreach (QPlatformScreen *platformScreen, d->platformScreen->virtualSiblings())
        result |= platformScreen->geometry();
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
    Q_D(const QScreen);
    QRect result;
    foreach (QPlatformScreen *platformScreen, d->platformScreen->virtualSiblings())
        result |= platformScreen->availableGeometry();
    return result;
}

/*!
    \property QScreen::primaryOrientation
    \brief the primary screen orientation

    The primary screen orientation is the orientation that corresponds
    to an un-rotated screen buffer. When the current orientation is equal
    to the primary orientation no rotation needs to be done by the
    application.
*/
Qt::ScreenOrientation QScreen::primaryOrientation() const
{
    Q_D(const QScreen);
    return d->platformScreen->primaryOrientation();
}

/*!
    \property QScreen::primaryOrientation
    \brief the current screen orientation

    The current orientation is a hint to the application saying
    what the preferred application orientation should be, based on the
    current orientation of the physical display and / or other factors.

    \sa primaryOrientation()
    \sa currentOrientationChanged()
*/
Qt::ScreenOrientation QScreen::currentOrientation() const
{
    Q_D(const QScreen);
    return d->platformScreen->currentOrientation();
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
*/
int QScreen::angleBetween(Qt::ScreenOrientation a, Qt::ScreenOrientation b)
{
    if (a == Qt::UnknownOrientation || b == Qt::UnknownOrientation || a == b)
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
*/
QTransform QScreen::transformBetween(Qt::ScreenOrientation a, Qt::ScreenOrientation b, const QRect &target)
{
    if (a == Qt::UnknownOrientation || b == Qt::UnknownOrientation || a == b)
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
*/
QRect QScreen::mapBetween(Qt::ScreenOrientation a, Qt::ScreenOrientation b, const QRect &rect)
{
    if (a == Qt::UnknownOrientation || b == Qt::UnknownOrientation || a == b)
        return rect;

    if ((a == Qt::PortraitOrientation || a == Qt::InvertedPortraitOrientation)
        != (b == Qt::PortraitOrientation || b == Qt::InvertedPortraitOrientation))
    {
        return QRect(rect.y(), rect.x(), rect.height(), rect.width());
    }

    return rect;
}

/*!
    \fn QScreen::currentOrientationChanged(Qt::ScreenOrientation orientation)

    This signal is emitted when the current orientation of the screen
    changes. The current orientation is a hint to the application saying
    what the preferred application orientation should be, based on the
    current orientation of the physical display and / or other factors.

    \sa currentOrientation()
*/

QT_END_NAMESPACE
