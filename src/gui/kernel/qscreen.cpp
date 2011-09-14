/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
  Get the platform dependent screen name.

  For example, on an X11 platform this should typically be
  the DISPLAY environment variable corresponding to the screen.
*/
QString QScreen::name() const
{
    Q_D(const QScreen);
    return d->platformScreen->name();
}

/*!
  Get the screen's color depth.
*/
int QScreen::depth() const
{
    Q_D(const QScreen);
    return d->platformScreen->depth();
}

/*!
  Get the screen's size.
*/
QSize QScreen::size() const
{
    Q_D(const QScreen);
    return d->platformScreen->geometry().size();
}

/*!
  Get the screen's available size.

  The available size is the size excluding window manager reserved areas
  such as task bars and system menus.
*/
QSize QScreen::availableSize() const
{
    Q_D(const QScreen);
    return d->platformScreen->availableGeometry().size();
}

/*!
  Get the screen's geometry.
*/
QRect QScreen::geometry() const
{
    Q_D(const QScreen);
    return d->platformScreen->geometry();
}

/*!
  Get the screen's available geometry.

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
  Get the size of the virtual desktop corresponding to this screen.

  This is the combined size of the virtual siblings' individual geometries.

  \sa virtualSiblings()
*/
QSize QScreen::virtualSize() const
{
    return virtualGeometry().size();
}

/*!
  Get the geometry of the virtual desktop corresponding to this screen.

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
  Get the available size of the virtual desktop corresponding to this screen.

  This is the combined size of the virtual siblings' individual available geometries.

  \sa availableSize()
  \sa virtualSiblings()
*/
QSize QScreen::availableVirtualSize() const
{
    return availableVirtualGeometry().size();
}

/*!
  Get the available size of the virtual desktop corresponding to this screen.

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

QT_END_NAMESPACE
