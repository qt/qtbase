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

#include <qplatformbackingstore_qpa.h>
#include <qwindow.h>
#include <qpixmap.h>
#include <private/qwindow_p.h>

QT_BEGIN_NAMESPACE

class QPlatformBackingStorePrivate
{
public:
    QPlatformBackingStorePrivate(QWindow *w)
        : window(w)
    {
    }

    QWindow *window;
    QSize size;
};

/*!
    \class QPlatformBackingStore
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformBackingStore class provides the drawing area for top-level
    windows.
*/

/*!
    \fn void QPlatformBackingStore::flush(QWindow *window, const QRegion &region,
                                  const QPoint &offset)

    Flushes the given \a region from the specified \a window onto the
    screen.

    Note that the \a offset parameter is currently unused.
*/

/*!
    \fn QPaintDevice* QPlatformBackingStore::paintDevice()

    Implement this function to return the appropriate paint device.
*/

/*!
    Constructs an empty surface for the given top-level \a window.
*/
QPlatformBackingStore::QPlatformBackingStore(QWindow *window)
    : d_ptr(new QPlatformBackingStorePrivate(window))
{
}

/*!
    Destroys this surface.
*/
QPlatformBackingStore::~QPlatformBackingStore()
{
    delete d_ptr;
}

/*!
    Returns a pointer to the top-level window associated with this
    surface.
*/
QWindow* QPlatformBackingStore::window() const
{
    return d_ptr->window;
}

/*!
    This function is called before painting onto the surface begins,
    with the \a region in which the painting will occur.

    \note A platform providing a backing store with an alpha channel
    needs to properly initialize the region to be painted.

    \sa endPaint(), paintDevice()
*/

void QPlatformBackingStore::beginPaint(const QRegion &)
{
}

/*!
    This function is called after painting onto the surface has ended.

    \sa beginPaint(), paintDevice()
*/

void QPlatformBackingStore::endPaint()
{
}

/*!
    Scrolls the given \a area \a dx pixels to the right and \a dy
    downward; both \a dx and \a dy may be negative.

    Returns true if the area was scrolled successfully; false otherwise.
*/
bool QPlatformBackingStore::scroll(const QRegion &area, int dx, int dy)
{
    Q_UNUSED(area);
    Q_UNUSED(dx);
    Q_UNUSED(dy);

    return false;
}

QT_END_NAMESPACE
