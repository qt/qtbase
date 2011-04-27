/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdirectpainter_qws.h"

#include "qscreen_qws.h"
#include "private/qobject_p.h"
#include "private/qapplication_p.h"
#include "qwsdisplay_qws.h"
#include "qwidget.h"
#include "qimage.h"
#include <qwsevent_qws.h>
#include <private/qwindowsurface_qws_p.h>
#include <private/qwsdisplay_qws_p.h>

QT_BEGIN_NAMESPACE

#ifdef Q_WS_QWS
#ifndef QT_NO_DIRECTPAINTER

/*!
    \class QDirectPainter
    \ingroup painting
    \ingroup qws

    \brief The QDirectPainter class provides direct access to the
    underlying hardware in Qt for Embedded Linux.

    Note that this class is only available in \l{Qt for Embedded Linux}.

    QDirectPainter allows a client application to reserve a region of
    the framebuffer and render directly onto the screen. There are two
    ways of using the QDirectPainter class: You can either reserve a
    region using the provided static functions, or you can instantiate
    an object and make use of its more dynamic API.

    \tableofcontents

    \section1 Dynamic Allocation

    By instantiating a QDirectPainter object using the default
    QDirectPainter::NonReserved surface flag, the client application
    only gets some control over the reserved region, i.e., it can
    still render directly onto the screen but the allocated region may
    change (for example, if a window with a higher focus requests
    parts of the same region). The currently allocated region can be
    retrieved using the allocatedRegion() function, while the
    requestedRegion() function returns the originally reserved
    region.


    \section1 Static Allocation


    Using the static approach, the client application gets complete
    control over the reserved region, i.e., the affected region will
    never be modified by the screen driver.

    To create a static region, pass the QDirectPainter::Reserved
    surface flag to the constructor. After the reserved region is
    reported through regionChanged(), the allocated region will not
    change, unless setRegion() is called.

    If QDirectPainter::ReservedSynchronous is passed to the
    constructor, calls to setRegion() will block until the region is
    reserved, meaning that allocatedRegion() will be available immediately.
    Note that in the current version setRegion() will cause the application
    event loop to be entered, potentially causing reentrancy issues.

    \section1 Rendering

    To draw on a given region, the application must first get hold of
    a pointer to the framebuffer. In most cases, this pointer can be
    retrieved using the QDirectPainter::frameBuffer() function. But
    note that if the current screen has subscreens, you must query the
    screen driver instead to identify the correct subscreen. A pointer
    to the current screen driver can always be retrieved using the
    static QScreen::instance() function. Then use QScreen's \l
    {QScreen::}{subScreenIndexAt()} and \l {QScreen::}{subScreens()}
    functions to access the correct subscreen, and the subscreen's \l
    {QScreen::}{base()} function to retrieve a pointer to the
    framebuffer.

    Depending on the hardware, it might be necessary to lock the
    framebuffer for exclusive use while writing to it. This is
    possible using the lock() and unlock() functions. Note that
    calling lock() will prevent all other applications from working
    until unlock() is called.

    In addition, QDirectPainter provides several functions returning
    information about the framebuffer: the linestep() function returns
    the length (in bytes) of each scanline of the framebuffer while
    the screenDepth(), screenWidth() and screenHeight() function
    return the screen metrics.

    \sa QScreen, QWSEmbedWidget, {Qt for Embedded Linux Architecture}
*/

/*!
    \enum QDirectPainter::SurfaceFlag

    This enum describes the behavior of the region reserved by this
    QDirectPainter object.

    \value NonReserved The allocated region may change, e.g., if a
    window with a higher focus requests parts of the same region. See
    also \l {Dynamic Allocation}.

    \value Reserved The allocated region will never change. See also
    \l {Static Allocation}.

    \value ReservedSynchronous The allocated region will never change and
    each function that changes the allocated region will be blocking.

    \sa allocatedRegion()
*/

/*!
    \fn QRegion QDirectPainter::region()
    \obsolete

    Use QDirectPainter::allocatedRegion() instead.
*/

static inline QScreen *getPrimaryScreen()
{
    QScreen *screen = QScreen::instance();
    if (!screen->base()) {
        QList<QScreen*> subScreens = screen->subScreens();
        if (subScreens.size() < 1)
            return 0;
        screen = subScreens.at(0);
    }
    return screen;
}

static inline QSize screenS()
{
    QScreen *screen = getPrimaryScreen();
    if (!screen)
        return QSize();
    return QSize(screen->width(), screen->height());
}

static inline QSize devS()
{
    QScreen *screen = getPrimaryScreen();
    if (!screen)
        return QSize();
    return QSize(screen->deviceWidth(), screen->deviceHeight());
}


class QDirectPainterPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDirectPainter);
public:

    QDirectPainterPrivate() : surface(0), seenRegion(false) {}

    ~QDirectPainterPrivate() {
        if (QPaintDevice::qwsDisplay()) { // make sure not in QApplication destructor
            qApp->d_func()->directPainters->remove(surface->windowId());
            surface->setGeometry(QRect());
        }
        delete surface;
    }

    QWSDirectPainterSurface *surface;
    QRegion requested_region;

    static QDirectPainter *staticPainter;
    bool seenRegion;
};

QDirectPainter *QDirectPainterPrivate::staticPainter = 0;

void qt_directpainter_region(QDirectPainter *dp, const QRegion &alloc, int type)
{
    QDirectPainterPrivate *d = dp->d_func();

    QRegion r = alloc;
    QScreen *screen = d->surface->screen();
    if (screen->isTransformed()) {
        const QSize screenSize(screen->width(), screen->height());
        r = screen->mapToDevice(r, screenSize);
    }
    if (type == QWSRegionEvent::Allocation) {
        d->surface->setClipRegion(alloc);
        d->seenRegion = true;
        if (dp != QDirectPainterPrivate::staticPainter) {
            if (!d->surface->flushingRegionEvents) // recursion guard
                dp->regionChanged(r);
        }
    }
}

#ifndef QT_NO_QWSEMBEDWIDGET
void qt_directpainter_embedevent(QDirectPainter *dp, const QWSEmbedEvent *event)
{
    if (event->type | QWSEmbedEvent::Region) {
        QScreen *screen = dp->d_func()->surface->screen();
        QRegion r = event->region;
        if (screen->isTransformed()) {
            const QSize screenSize(screen->width(), screen->height());
            r = screen->mapToDevice(r, screenSize);
        }
        dp->setRegion(r);
    }
}
#endif

/*!
    Constructs a QDirectPainter object with the given \a parent and
    surface \a flag.
*/
QDirectPainter::QDirectPainter(QObject *parent, SurfaceFlag flag)
    :QObject(*new QDirectPainterPrivate, parent)
{
    Q_D(QDirectPainter);
    d->surface = new QWSDirectPainterSurface(true, flag);

    if (flag != NonReserved)
        d->surface->setReserved();

    QApplicationPrivate *ad = qApp->d_func();
    if (!ad->directPainters)
        ad->directPainters = new QMap<WId, QDirectPainter*>;
    ad->directPainters->insert(d->surface->windowId(), this);
}

/*!
    Destroys this QDirectPainter object, releasing the reserved region.

    \sa allocatedRegion()
*/
QDirectPainter::~QDirectPainter()
{
    /* should not be necessary
    if (this == QDirectPainterPrivate::staticPainter)
        QDirectPainterPrivate::staticPainter = 0;
    */
}

/*!
    \fn void QDirectPainter::setGeometry(const QRect &rectangle)
    \since 4.2

    Request to reserve the given \a rectangle of the framebuffer.

    Note that the actually allocated region might differ from the
    requested one, e.g., if the given region overlaps with the
    region of another QDirectPainter object.

    \sa geometry(), allocatedRegion(), setRegion()
*/
void QDirectPainter::setGeometry(const QRect &rect)
{
    setRegion(rect);
}

/*!
    \since 4.2

    Returns the bounding rectangle of the requested region.

    \sa setGeometry(), requestedRegion()
*/
QRect QDirectPainter::geometry() const
{
    Q_D(const QDirectPainter);
    return d->requested_region.boundingRect();
}

/*!
    \since 4.2

    Requests to reserve the given \a region of the framebuffer.

    Note that the actually allocated region might differ from the
    requested one, e.g., if the given region overlaps with the region
    of another QDirectPainter object.

    \sa requestedRegion(), allocatedRegion(), {Dynamic Allocation}
*/
void QDirectPainter::setRegion(const QRegion &region)
{
    Q_D(QDirectPainter);
    d->requested_region = region;

    const QScreen *screen = d->surface->screen();
    if (screen->isTransformed()) {
        const QSize devSize(screen->deviceWidth(), screen->deviceHeight());
        const QRegion r = screen->mapFromDevice(region, devSize);
        d->surface->setRegion(r);
    } else {
        d->surface->setRegion(region);
    }
}

/*!
    \since 4.2

    Returns the region requested by this QDirectPainter.

    Note that if the QDirectPainter::Reserved flag is set, the region
    returned by this function will always be equivalent to the region
    returned by the allocatedRegion() function. Otherwise they might
    differ (see \l {Dynamic Allocation} for details).

    \sa geometry(), setRegion(), allocatedRegion()
*/
QRegion QDirectPainter::requestedRegion() const
{
    Q_D(const QDirectPainter);
    return d->requested_region;
}

/*!
    \since 4.2

    Returns the currently reserved region.

    Note that if the QDirectPainter::Reserved flag is set, the region
    returned by this function will always be equivalent to the region
    returned by the requestedRegion() function. Otherwise they might
    differ (see \l {Dynamic Allocation} for details).

    \sa requestedRegion(), geometry()
*/
QRegion QDirectPainter::allocatedRegion() const
{
    Q_D(const QDirectPainter);
    const QScreen *screen = d->surface->screen();
    if (screen->isTransformed()) {
        const QSize screenSize(screen->width(), screen->height());
        return screen->mapToDevice(d->surface->region(), screenSize);
    } else {
        return d->surface->region();
    }
}

/*!
    \since 4.2

    Returns the window system identifier of the widget.
*/
WId QDirectPainter::winId() const
{
    Q_D(const QDirectPainter);
    return d->surface->windowId();
}

/*!
    \fn void QDirectPainter::regionChanged(const QRegion &newRegion)
    \since 4.2

    This function is called when the allocated region changes.

    This function is not called for region changes that happen while the
    startPainting() function is executing.

    Note that the given region, \a newRegion, is not guaranteed to be correct at the
    time you access the display. To prevent reentrancy problems you should
    always call startPainting() before updating the display and then use
    allocatedRegion() to retrieve the correct region.

    \sa allocatedRegion(), startPainting(), {Dynamic Allocation}
*/
void QDirectPainter::regionChanged(const QRegion &region)
{
    Q_UNUSED(region);
}

/*!
    \since 4.2

    Call this function before you start updating the pixels in the
    allocated region. The hardware will be notified, if necessary,
    that you are about to start painting operations.

    Set \a lockDisplay if you want startPainting() and endPainting()
    to lock() and unlock() the display automatically.

    Note that for a NonReserved direct painter, you must call
    allocatedRegion() after calling this function, since the allocated
    region is only guaranteed to be correct after this function has
    returned.

    The regionChanged() function will not be called between startPainting()
    and endPainting().

    \sa endPainting(), flush()
*/
void QDirectPainter::startPainting(bool lockDisplay)
{
    Q_D(QDirectPainter);
    d->surface->setLocking(lockDisplay);

    const QScreen *screen = d->surface->screen();
    if (screen->isTransformed()) {
        const QSize devSize(screen->deviceWidth(), screen->deviceHeight());
        const QRegion r = screen->mapFromDevice(d->surface->region(), devSize);
        d->surface->beginPaint(r);
    } else {
        d->surface->beginPaint(d->surface->region());
    }
}

/*!
    \since 4.2

    Call this function when you are done updating the screen. It will
    notify the hardware, if necessary, that your painting operations
    have ended.
*/
void QDirectPainter::endPainting()
{
    Q_D(QDirectPainter);

    const QScreen *screen = d->surface->screen();
    if (screen->isTransformed()) {
        const QSize devSize(screen->deviceWidth(), screen->deviceHeight());
        const QRegion r = screen->mapFromDevice(d->surface->region(), devSize);
        d->surface->endPaint(r);
    } else {
        d->surface->endPaint(d->surface->region());
    }
}

/*!
    \since 4.3
    \overload

    This function will automatically call flush() to flush the
    \a region to the display before notifying the hardware, if
    necessary, that painting operations have ended.
*/
void QDirectPainter::endPainting(const QRegion &region)
{
    endPainting();
    flush(region);
}

/*!
    \since 4.3

    Flushes the \a region onto the screen.
*/
void QDirectPainter::flush(const QRegion &region)
{
    Q_D(QDirectPainter);

    const QScreen *screen = d->surface->screen();
    if (screen->isTransformed()) {
        const QSize devSize(screen->deviceWidth(), screen->deviceHeight());
        const QRegion r = screen->mapFromDevice(region, devSize);
        d->surface->flush(0, r, QPoint());
    } else {
        d->surface->flush(0, region, QPoint());
    }
}

/*!
    \since 4.2

    Raises the reserved region to the top of the widget stack.

    After this call the reserved region will be visually in front of
    any overlapping widgets.

    \sa lower(), requestedRegion()
*/
void QDirectPainter::raise()
{
    QWidget::qwsDisplay()->setAltitude(winId(),QWSChangeAltitudeCommand::Raise);
}

/*!
    \since 4.2

    Lowers the reserved region to the bottom of the widget stack.

    After this call the reserved region will be visually behind (and
    therefore obscured by) any overlapping widgets.

    \sa raise(), requestedRegion()
*/
void QDirectPainter::lower()
{
    QWidget::qwsDisplay()->setAltitude(winId(),QWSChangeAltitudeCommand::Lower);
}


/*!
    \fn QRegion QDirectPainter::reserveRegion(const QRegion &region)

    Attempts to reserve the \a region and returns the region that is
    actually reserved.

    This function also releases the previously reserved region if
    any. If not released explicitly, the region will be released on
    application exit.

    \sa allocatedRegion(), {Static Allocation}

    \obsolete

    Construct a QDirectPainter using QDirectPainter::ReservedSynchronous instead.
*/
QRegion QDirectPainter::reserveRegion(const QRegion &reg)
{
    if (!QDirectPainterPrivate::staticPainter)
        QDirectPainterPrivate::staticPainter = new QDirectPainter(qApp, ReservedSynchronous);

    QDirectPainter *dp = QDirectPainterPrivate::staticPainter;
    dp->setRegion(reg);

    return dp->allocatedRegion();
}

/*!
    Returns a pointer to the beginning of the display memory.

    Note that it is the application's responsibility to limit itself
    to modifying only the reserved region.

    Do not use this pointer if the current screen has subscreens,
    query the screen driver instead: A pointer to the current screen
    driver can always be retrieved using the static
    QScreen::instance() function. Then use QScreen's \l
    {QScreen::}{subScreenIndexAt()} and \l {QScreen::}{subScreens()}
    functions to access the correct subscreen, and the subscreen's \l
    {QScreen::}{base()} function to retrieve a pointer to the
    framebuffer.

    \sa requestedRegion(), allocatedRegion(), linestep()
*/
uchar* QDirectPainter::frameBuffer()
{
    QScreen *screen = getPrimaryScreen();
    if (!screen)
        return 0;
    return screen->base();
}

/*!
    \since 4.2

    Returns the reserved region.

    \sa reserveRegion(), frameBuffer()

    \obsolete

    Use allocatedRegion() instead.
*/
QRegion QDirectPainter::reservedRegion()
{
    return QDirectPainterPrivate::staticPainter
        ? QDirectPainterPrivate::staticPainter->allocatedRegion() : QRegion();
}

/*!
    Returns the bit depth of the display.

    \sa screenHeight(), screenWidth()
*/
int QDirectPainter::screenDepth()
{
    QScreen *screen = getPrimaryScreen();
    if (!screen)
        return 0;
    return screen->depth();
}

/*!
    Returns the width of the display in pixels.

    \sa screenHeight(), screenDepth()
*/
int QDirectPainter::screenWidth()
{
    QScreen *screen = getPrimaryScreen();
    if (!screen)
        return 0;
    return screen->deviceWidth();
}

/*!
    Returns the height of the display in pixels.

    \sa screenWidth(), screenDepth()
*/
int QDirectPainter::screenHeight()
{
    QScreen *screen = getPrimaryScreen();
    if (!screen)
        return 0;
    return screen->deviceHeight();
}

/*!
    Returns the length (in bytes) of each scanline of the framebuffer.

    \sa frameBuffer()
*/
int QDirectPainter::linestep()
{
    QScreen *screen = getPrimaryScreen();
    if (!screen)
        return 0;
    return screen->linestep();
}


/*!
  Locks access to the framebuffer.

  Note that calling this function will prevent all other
  applications from updating the display until unlock() is called.

  \sa unlock()
*/
void QDirectPainter::lock()
{
    QWSDisplay::grab(true);
}
/*!
  Unlocks the lock on the framebuffer (set using the lock()
  function), allowing other applications to access the screen.

  \sa lock()
 */
void QDirectPainter::unlock()
{
    QWSDisplay::ungrab();
}

#endif //QT_NO_DIRECTPAINTER

#endif

QT_END_NAMESPACE
