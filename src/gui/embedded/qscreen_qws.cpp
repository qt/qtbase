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

#include "qplatformdefs.h"
#include "qscreen_qws.h"

#include "qcolormap.h"
#include "qscreendriverfactory_qws.h"
#include "qwindowsystem_qws.h"
#include "qwidget.h"
#include "qcolor.h"
#include "qpixmap.h"
#include "qvarlengtharray.h"
#include "qwsdisplay_qws.h"
#include "qpainter.h"
#include <private/qdrawhelper_p.h>
#include <private/qpaintengine_raster_p.h>
#include <private/qpixmap_raster_p.h>
#include <private/qwindowsurface_qws_p.h>
#include <private/qpainter_p.h>
#include <private/qwidget_p.h>
#include <private/qgraphicssystem_qws_p.h>

QT_BEGIN_NAMESPACE

// #define QT_USE_MEMCPY_DUFF

#ifndef QT_NO_QWS_CURSOR
Q_GUI_EXPORT QScreenCursor * qt_screencursor = 0;
#endif
Q_GUI_EXPORT QScreen * qt_screen = 0;

ClearCacheFunc QScreen::clearCacheFunc = 0;

#ifndef QT_NO_QWS_CURSOR
/*!
    \class QScreenCursor
    \ingroup qws

    \brief The QScreenCursor class is a base class for screen cursors
    in Qt for Embedded Linux.

    Note that this class is non-portable, and that it is only
    available in \l{Qt for Embedded Linux}.

    QScreenCursor implements a software cursor, but can be subclassed
    to support hardware cursors as well. When deriving from the
    QScreenCursor class it is important to maintain the cursor's
    image, position, hot spot (the point within the cursor's image
    that will be the position of the associated mouse events) and
    visibility as well as informing whether it is hardware accelerated
    or not.

    Note that there may only be one screen cursor at a time. Use the
    static instance() function to retrieve a pointer to the current
    screen cursor. Typically, the cursor is constructed by the QScreen
    class or one of its descendants when it is initializing the
    device; the QScreenCursor class should never be instantiated
    explicitly.

    Use the move() function to change the position of the cursor, and
    the set() function to alter its image or its hot spot. In
    addition, you can find out whether the cursor is accelerated or
    not, using the isAccelerated() function, and the boundingRect()
    function returns the cursor's bounding rectangle.

    The cursor's appearance can be controlled using the isVisible(),
    hide() and show() functions; alternatively the QWSServer class
    provides some means of controlling the cursor's appearance using
    the QWSServer::isCursorVisible() and QWSServer::setCursorVisible()
    functions.

    \sa QScreen, QWSServer
*/

/*!
    \fn static QScreenCursor* QScreenCursor::instance()
    \since 4.2

    Returns a pointer to the application's unique screen cursor.
*/

/*!
    Constructs a screen cursor
*/
QScreenCursor::QScreenCursor()
{
    pos = QPoint(qt_screen->deviceWidth()/2, qt_screen->deviceHeight()/2);
    size = QSize(0,0);
    enable = true;
    hwaccel = false;
    supportsAlpha = true;
}

/*!
    Destroys the screen cursor.
*/
QScreenCursor::~QScreenCursor()
{
}

/*!
    Hides the cursor from the screen.

    \sa show()
*/
void QScreenCursor::hide()
{
    if (enable) {
        enable = false;
        if (!hwaccel)
            qt_screen->exposeRegion(boundingRect(), 0);
    }
}

/*!
    Shows the mouse cursor.

    \sa hide()
*/
void QScreenCursor::show()
{
    if (!enable) {
        enable = true;
        if (!hwaccel)
            qt_screen->exposeRegion(boundingRect(), 0);
    }
}

/*!
    Sets the cursor's image to be the given \a image.

    The \a hotx and \a hoty parameters define the cursor's hot spot,
    i.e., the point within the cursor's image that will be the
    position of the associated mouse events.

    \sa move()
*/
void QScreenCursor::set(const QImage &image, int hotx, int hoty)
{
    const QRect r = boundingRect();

    hotspot = QPoint(hotx, hoty);
    // These are in almost all cases the fastest formats to blend
    QImage::Format f;
    switch (qt_screen->depth()) {
    case 12:
        f = QImage::Format_ARGB4444_Premultiplied;
        break;
    case 15:
        f =  QImage::Format_ARGB8555_Premultiplied;
        break;
    case 16:
        f = QImage::Format_ARGB8565_Premultiplied;
        break;
    case 18:
        f = QImage::Format_ARGB6666_Premultiplied;
        break;
    default:
        f =  QImage::Format_ARGB32_Premultiplied;
    }

    cursor = image.convertToFormat(f);

    size = image.size();

    if (enable && !hwaccel)
        qt_screen->exposeRegion(r | boundingRect(), 0);
}

/*!
    Moves the mouse cursor to the given position, i.e., (\a x, \a y).

    Note that the given position defines the top-left corner of the
    cursor's image, i.e., not the cursor's hot spot (the position of
    the associated mouse events).

    \sa set()
*/
void QScreenCursor::move(int x, int y)
{
    QRegion r = boundingRect();
    pos = QPoint(x,y);
    if (enable && !hwaccel) {
        r |= boundingRect();
        qt_screen->exposeRegion(r, 0);
    }
}


/*!
    \fn void QScreenCursor::initSoftwareCursor ()

    Initializes the screen cursor.

    This function is typically called from the screen driver when
    initializing the device. Alternatively, the cursor can be set
    directly using the pointer returned by the static instance()
    function.

    \sa QScreen::initDevice()
*/
void QScreenCursor::initSoftwareCursor()
{
    qt_screencursor = new QScreenCursor;
}


#endif // QT_NO_QWS_CURSOR


/*!
    \fn QRect QScreenCursor::boundingRect () const

    Returns the cursor's bounding rectangle.
*/

/*!
    \internal
    \fn bool QScreenCursor::enabled ()
*/

/*!
    \fn QImage QScreenCursor::image () const

    Returns the cursor's image.
*/


/*!
    \fn bool QScreenCursor::isAccelerated () const

    Returns true if the cursor is accelerated; otherwise false.
*/

/*!
    \fn bool QScreenCursor::isVisible () const

    Returns true if the cursor is visible; otherwise false.
*/

/*!
    \internal
    \fn bool QScreenCursor::supportsAlphaCursor () const
*/

/*
    \variable QScreenCursor::cursor

    \brief the cursor's image.

    \sa image()
*/

/*
    \variable QScreenCursor::size

    \brief the cursor's size
*/

/*
    \variable QScreenCursor::pos

    \brief the cursor's position, i.e., the position of the top-left
    corner of the crsor's image

    \sa set(), move()
*/

/*
    \variable QScreenCursor::hotspot

    \brief the cursor's hotspot, i.e., the point within the cursor's
    image that will be the position of the associated mouse events.

    \sa set(), move()
*/

/*
    \variable QScreenCursor::enable

    \brief whether the cursor is visible or not

    \sa isVisible()
*/

/*
    \variable QScreenCursor::hwaccel

    \brief holds whether the cursor is accelerated or not

    If the cursor is not accelerated, its image will be included by
    the screen when it composites the window surfaces.

    \sa isAccelerated()

*/

/*
    \variable QScreenCursor::supportsAlpha
*/

/*!
    \internal
    \macro qt_screencursor
    \relates QScreenCursor

    A global pointer referring to the unique screen cursor. It is
    equivalent to the pointer returned by the
    QScreenCursor::instance() function.
*/



class QScreenPrivate
{
public:
    QScreenPrivate(QScreen *parent, QScreen::ClassId id = QScreen::CustomClass);
    ~QScreenPrivate();

    inline QImage::Format preferredImageFormat() const;

    typedef void (*SolidFillFunc)(QScreen*, const QColor&, const QRegion&);
    typedef void (*BlitFunc)(QScreen*, const QImage&, const QPoint&, const QRegion&);

    SolidFillFunc solidFill;
    BlitFunc blit;

    QPoint offset;
    QList<QScreen*> subScreens;
    QPixmapDataFactory* pixmapFactory;
    QGraphicsSystem* graphicsSystem;
    QWSGraphicsSystem defaultGraphicsSystem; //###
    QImage::Format pixelFormat;
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    bool fb_is_littleEndian;
#endif
#ifdef QT_QWS_CLIENTBLIT
    bool supportsBlitInClients;
#endif
    int classId;
    QScreen *q_ptr;
};

template <typename T>
static void solidFill_template(QScreen *screen, const QColor &color,
                               const QRegion &region)
{
    T *dest = reinterpret_cast<T*>(screen->base());
    const T c = qt_colorConvert<T, quint32>(color.rgba(), 0);
    const int stride = screen->linestep();
    const QVector<QRect> rects = region.rects();

    for (int i = 0; i < rects.size(); ++i) {
        const QRect r = rects.at(i);
        qt_rectfill(dest, c, r.x(), r.y(), r.width(), r.height(), stride);
    }
}

#ifdef QT_QWS_DEPTH_GENERIC
static void solidFill_rgb_32bpp(QScreen *screen, const QColor &color,
                                const QRegion &region)
{
    quint32 *dest = reinterpret_cast<quint32*>(screen->base());
    const quint32 c = qt_convertToRgb<quint32>(color.rgba());

    const int stride = screen->linestep();
    const QVector<QRect> rects = region.rects();

    for (int i = 0; i < rects.size(); ++i) {
        const QRect r = rects.at(i);
        qt_rectfill(dest, c, r.x(), r.y(), r.width(), r.height(), stride);
    }
}

static void solidFill_rgb_16bpp(QScreen *screen, const QColor &color,
                                const QRegion &region)
{
    quint16 *dest = reinterpret_cast<quint16*>(screen->base());
    const quint16 c = qt_convertToRgb<quint32>(color.rgba());

    const int stride = screen->linestep();
    const QVector<QRect> rects = region.rects();

    for (int i = 0; i < rects.size(); ++i) {
        const QRect r = rects.at(i);
        qt_rectfill(dest, c, r.x(), r.y(), r.width(), r.height(), stride);
    }
}
#endif // QT_QWS_DEPTH_GENERIC

#ifdef QT_QWS_DEPTH_4
static inline void qt_rectfill_gray4(quint8 *dest, quint8 value,
                                     int x, int y, int width, int height,
                                     int stride)
{
    const int pixelsPerByte = 2;
    dest += y * stride + x / pixelsPerByte;
    const int doAlign = x & 1;
    const int doTail = (width - doAlign) & 1;
    const int width8 = (width - doAlign) / pixelsPerByte;

    for (int j = 0; j < height; ++j) {
        if (doAlign)
            *dest = (*dest & 0xf0) | (value & 0x0f);
        if (width8)
            qt_memfill<quint8>(dest + doAlign, value, width8);
        if (doTail) {
            quint8 *d = dest + doAlign + width8;
            *d = (*d & 0x0f) | (value & 0xf0);
        }
        dest += stride;
    }
}

static void solidFill_gray4(QScreen *screen, const QColor &color,
                            const QRegion &region)
{
    quint8 *dest = reinterpret_cast<quint8*>(screen->base());
    const quint8 c = qGray(color.rgba()) >> 4;
    const quint8 c8 = (c << 4) | c;

    const int stride = screen->linestep();
    const QVector<QRect> rects = region.rects();

    for (int i = 0; i < rects.size(); ++i) {
        const QRect r = rects.at(i);
        qt_rectfill_gray4(dest, c8, r.x(), r.y(), r.width(), r.height(),
                          stride);
    }
}
#endif // QT_QWS_DEPTH_4

#ifdef QT_QWS_DEPTH_1
static inline void qt_rectfill_mono(quint8 *dest, quint8 value,
                                    int x, int y, int width, int height,
                                    int stride)
{
    const int pixelsPerByte = 8;
    const int alignWidth = qMin(width, (8 - (x & 7)) & 7);
    const int doAlign = (alignWidth > 0 ? 1 : 0);
    const int alignStart = pixelsPerByte - 1 - (x & 7);
    const int alignStop = alignStart - (alignWidth - 1);
    const quint8 alignMask = ((1 << alignWidth) - 1) << alignStop;
    const int tailWidth = (width - alignWidth) & 7;
    const int doTail = (tailWidth > 0 ? 1 : 0);
    const quint8 tailMask = (1 << (pixelsPerByte - tailWidth)) - 1;
    const int width8 = (width - alignWidth) / pixelsPerByte;

    dest += y * stride + x / pixelsPerByte;
    stride -= (doAlign + width8);

    for (int j = 0; j < height; ++j) {
        if (doAlign) {
            *dest = (*dest & ~alignMask) | (value & alignMask);
            ++dest;
        }
        if (width8) {
            qt_memfill<quint8>(dest, value, width8);
            dest += width8;
        }
        if (doTail)
            *dest = (*dest & tailMask) | (value & ~tailMask);
        dest += stride;
    }
}

static void solidFill_mono(QScreen *screen, const QColor &color,
                           const QRegion &region)
{
    quint8 *dest = reinterpret_cast<quint8*>(screen->base());
    const quint8 c8 = (qGray(color.rgba()) >> 7) * 0xff;

    const int stride = screen->linestep();
    const QVector<QRect> rects = region.rects();

    for (int i = 0; i < rects.size(); ++i) {
        const QRect r = rects.at(i);
        qt_rectfill_mono(dest, c8, r.x(), r.y(), r.width(), r.height(),
                         stride);
    }
}
#endif // QT_QWS_DEPTH_1

void qt_solidFill_setup(QScreen *screen, const QColor &color,
                        const QRegion &region)
{
    switch (screen->depth()) {
#ifdef QT_QWS_DEPTH_32
    case 32:
        if (screen->pixelType() == QScreen::NormalPixel)
            screen->d_ptr->solidFill = solidFill_template<quint32>;
        else
            screen->d_ptr->solidFill = solidFill_template<qabgr8888>;
        break;
#endif
#ifdef QT_QWS_DEPTH_24
    case 24:
        if (screen->pixelType() == QScreen::NormalPixel)
            screen->d_ptr->solidFill = solidFill_template<qrgb888>;
        else
            screen->d_ptr->solidFill = solidFill_template<quint24>;
        break;
#endif
#ifdef QT_QWS_DEPTH_18
    case 18:
        screen->d_ptr->solidFill = solidFill_template<quint18>;
        break;
#endif
#ifdef QT_QWS_DEPTH_16
    case 16:
        if (screen->pixelType() == QScreen::NormalPixel)
            screen->d_ptr->solidFill = solidFill_template<quint16>;
        else
            screen->d_ptr->solidFill = solidFill_template<qbgr565>;
        break;
#endif
#ifdef QT_QWS_DEPTH_15
    case 15:
        if (screen->pixelType() == QScreen::NormalPixel)
            screen->d_ptr->solidFill = solidFill_template<qrgb555>;
        else
            screen->d_ptr->solidFill = solidFill_template<qbgr555>;
        break;
#endif
#ifdef QT_QWS_DEPTH_12
    case 12:
        screen->d_ptr->solidFill = solidFill_template<qrgb444>;
        break;
#endif
#ifdef QT_QWS_DEPTH_8
    case 8:
        screen->d_ptr->solidFill = solidFill_template<quint8>;
        break;
#endif
#ifdef QT_QWS_DEPTH_4
    case 4:
        screen->d_ptr->solidFill = solidFill_gray4;
        break;
#endif
#ifdef QT_QWS_DEPTH_1
    case 1:
        screen->d_ptr->solidFill = solidFill_mono;
        break;
#endif
     default:
        qFatal("solidFill_setup(): Screen depth %d not supported!",
               screen->depth());
        screen->d_ptr->solidFill = 0;
        break;
    }
    screen->d_ptr->solidFill(screen, color, region);
}

template <typename DST, typename SRC>
static void blit_template(QScreen *screen, const QImage &image,
                          const QPoint &topLeft, const QRegion &region)
{
    DST *dest = reinterpret_cast<DST*>(screen->base());
    const int screenStride = screen->linestep();
    const int imageStride = image.bytesPerLine();

    if (region.rectCount() == 1) {
        const QRect r = region.boundingRect();
        const SRC *src = reinterpret_cast<const SRC*>(image.scanLine(r.y()))
                         + r.x();
        qt_rectconvert<DST, SRC>(dest, src,
                                 r.x() + topLeft.x(), r.y() + topLeft.y(),
                                 r.width(), r.height(),
                                 screenStride, imageStride);
    } else {
        const QVector<QRect> rects = region.rects();

        for (int i = 0; i < rects.size(); ++i) {
            const QRect r = rects.at(i);
            const SRC *src = reinterpret_cast<const SRC*>(image.scanLine(r.y()))
                             + r.x();
            qt_rectconvert<DST, SRC>(dest, src,
                                     r.x() + topLeft.x(), r.y() + topLeft.y(),
                                     r.width(), r.height(),
                                     screenStride, imageStride);
        }
    }
}

#ifdef QT_QWS_DEPTH_32
static void blit_32(QScreen *screen, const QImage &image,
                    const QPoint &topLeft, const QRegion &region)
{
    switch (image.format()) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        blit_template<quint32, quint32>(screen, image, topLeft, region);
        return;
#ifdef QT_QWS_DEPTH_16
    case QImage::Format_RGB16:
        blit_template<quint32, quint16>(screen, image, topLeft, region);
        return;
#endif
    default:
        qCritical("blit_32(): Image format %d not supported!", image.format());
    }
}
#endif // QT_QWS_DEPTH_32

#ifdef QT_QWS_DEPTH_24
static void blit_24(QScreen *screen, const QImage &image,
                    const QPoint &topLeft, const QRegion &region)
{
    switch (image.format()) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        blit_template<quint24, quint32>(screen, image, topLeft, region);
        return;
    case QImage::Format_RGB888:
        blit_template<quint24, qrgb888>(screen, image, topLeft, region);
        return;
#ifdef QT_QWS_DEPTH_16
    case QImage::Format_RGB16:
        blit_template<quint24, quint16>(screen, image, topLeft, region);
        return;
#endif
    default:
        qCritical("blit_24(): Image format %d not supported!", image.format());
    }
}

static void blit_qrgb888(QScreen *screen, const QImage &image,
                         const QPoint &topLeft, const QRegion &region)
{
    switch (image.format()) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        blit_template<qrgb888, quint32>(screen, image, topLeft, region);
        return;
    case QImage::Format_RGB888:
        blit_template<qrgb888, qrgb888>(screen, image, topLeft, region);
        return;
#ifdef QT_QWS_DEPTH_16
    case QImage::Format_RGB16:
        blit_template<qrgb888, quint16>(screen, image, topLeft, region);
        return;
#endif
    default:
        qCritical("blit_24(): Image format %d not supported!", image.format());
        break;
    }
}
#endif // QT_QWS_DEPTH_24

#ifdef QT_QWS_DEPTH_18
static void blit_18(QScreen *screen, const QImage &image,
                    const QPoint &topLeft, const QRegion &region)
{
    switch (image.format()) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        blit_template<qrgb666, quint32>(screen, image, topLeft, region);
        return;
    case QImage::Format_RGB666:
        blit_template<qrgb666, qrgb666>(screen, image, topLeft, region);
        return;
#ifdef QT_QWS_DEPTH_16
    case QImage::Format_RGB16:
        blit_template<qrgb666, quint16>(screen, image, topLeft, region);
        return;
#endif
    default:
        qCritical("blit_18(): Image format %d not supported!", image.format());
    }
}
#endif // QT_QWS_DEPTH_18

#if (Q_BYTE_ORDER == Q_BIG_ENDIAN) && (defined(QT_QWS_DEPTH_16) || defined(QT_QWS_DEPTH_15))
class quint16LE
{
public:
    inline quint16LE(quint32 v) {
        data = ((v & 0xff00) >> 8) | ((v & 0x00ff) << 8);
    }

    inline quint16LE(int v) {
        data = ((v & 0xff00) >> 8) | ((v & 0x00ff) << 8);
    }

    inline quint16LE(quint16 v) {
        data = ((v & 0xff00) >> 8) | ((v & 0x00ff) << 8);
    }

    inline quint16LE(qrgb555 v) {
        data = (( (quint16)v & 0xff00) >> 8) |
               (( (quint16)v & 0x00ff) << 8);
    }

    inline bool operator==(const quint16LE &v) const
    {
        return data == v.data;
    }

private:
    quint16 data;
};
#endif

#ifdef QT_QWS_DEPTH_16
static void blit_16(QScreen *screen, const QImage &image,
                    const QPoint &topLeft, const QRegion &region)
{
    switch (image.format()) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        // ### This probably doesn't work but it's a case which should never happen
        blit_template<quint16, quint32>(screen, image, topLeft, region);
        return;
    case QImage::Format_RGB16:
        blit_template<quint16, quint16>(screen, image, topLeft, region);
        return;
    default:
        qCritical("blit_16(): Image format %d not supported!", image.format());
    }
}

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
static void blit_16_bigToLittleEndian(QScreen *screen, const QImage &image,
                                      const QPoint &topLeft,
                                      const QRegion &region)
{
    switch (image.format()) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        blit_template<quint16LE, quint32>(screen, image, topLeft, region);
        return;
    case QImage::Format_RGB16:
        blit_template<quint16LE, quint16>(screen, image, topLeft, region);
        return;
    default:
        qCritical("blit_16_bigToLittleEndian(): Image format %d not supported!", image.format());
    }
}

#endif // Q_BIG_ENDIAN
#endif // QT_QWS_DEPTH_16

#ifdef QT_QWS_DEPTH_15
static void blit_15(QScreen *screen, const QImage &image,
                    const QPoint &topLeft, const QRegion &region)
{
    switch (image.format()) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        blit_template<qrgb555, quint32>(screen, image, topLeft, region);
        return;
    case QImage::Format_RGB555:
        blit_template<qrgb555, qrgb555>(screen, image, topLeft, region);
        return;
    case QImage::Format_RGB16:
        blit_template<qrgb555, quint16>(screen, image, topLeft, region);
        return;
    default:
        qCritical("blit_15(): Image format %d not supported!", image.format());
    }
}

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
static void blit_15_bigToLittleEndian(QScreen *screen, const QImage &image,
                                      const QPoint &topLeft,
                                      const QRegion &region)
{
    switch (image.format()) {
    case QImage::Format_RGB555:
        blit_template<quint16LE, qrgb555>(screen, image, topLeft, region);
        return;
    default:
        qCritical("blit_15_bigToLittleEndian(): Image format %d not supported!", image.format());
    }
}
#endif // Q_BIG_ENDIAN
#endif // QT_QWS_DEPTH_15


#ifdef QT_QWS_DEPTH_12
static void blit_12(QScreen *screen, const QImage &image,
                    const QPoint &topLeft, const QRegion &region)
{
    switch (image.format()) {
    case QImage::Format_ARGB4444_Premultiplied:
        blit_template<qrgb444, qargb4444>(screen, image, topLeft, region);
        return;
    case QImage::Format_RGB444:
        blit_template<qrgb444, qrgb444>(screen, image, topLeft, region);
        return;
    default:
        qCritical("blit_12(): Image format %d not supported!", image.format());
    }
}
#endif // QT_QWS_DEPTH_12

#ifdef QT_QWS_DEPTH_8
static void blit_8(QScreen *screen, const QImage &image,
                   const QPoint &topLeft, const QRegion &region)
{
    switch (image.format()) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        blit_template<quint8, quint32>(screen, image, topLeft, region);
        return;
    case QImage::Format_RGB16:
        blit_template<quint8, quint16>(screen, image, topLeft, region);
        return;
    case QImage::Format_ARGB4444_Premultiplied:
        blit_template<quint8, qargb4444>(screen, image, topLeft, region);
        return;
    case QImage::Format_RGB444:
        blit_template<quint8, qrgb444>(screen, image, topLeft, region);
        return;
    default:
        qCritical("blit_8(): Image format %d not supported!", image.format());
    }
}
#endif // QT_QWS_DEPTH_8

#ifdef QT_QWS_DEPTH_4

struct qgray4 { quint8 dummy; } Q_PACKED;

template <typename SRC>
Q_STATIC_TEMPLATE_FUNCTION inline quint8 qt_convertToGray4(SRC color);

template <>
inline quint8 qt_convertToGray4(quint32 color)
{
    return qGray(color) >> 4;
}

template <>
inline quint8 qt_convertToGray4(quint16 color)
{
    const int r = (color & 0xf800) >> 11;
    const int g = (color & 0x07e0) >> 6; // only keep 5 bit
    const int b = (color & 0x001f);
    return (r * 11 + g * 16 + b * 5) >> 6;
}

template <>
inline quint8 qt_convertToGray4(qrgb444 color)
{
    return qt_convertToGray4(quint32(color));
}

template <>
inline quint8 qt_convertToGray4(qargb4444 color)
{
    return qt_convertToGray4(quint32(color));
}

template <typename SRC>
Q_STATIC_TEMPLATE_FUNCTION inline void qt_rectconvert_gray4(qgray4 *dest4, const SRC *src,
                                        int x, int y, int width, int height,
                                        int dstStride, int srcStride)
{
    const int pixelsPerByte = 2;
    quint8 *dest8 = reinterpret_cast<quint8*>(dest4)
                    + y * dstStride + x / pixelsPerByte;
    const int doAlign = x & 1;
    const int doTail = (width - doAlign) & 1;
    const int width8 = (width - doAlign) / pixelsPerByte;
    const int count8 = (width8 + 3) / 4;

    srcStride = srcStride / sizeof(SRC) - width;
    dstStride -= (width8 + doAlign);

    for (int i = 0; i < height; ++i) {
        if (doAlign) {
            *dest8 = (*dest8 & 0xf0) | qt_convertToGray4<SRC>(*src++);
            ++dest8;
        }
        if (count8) {
            int n = count8;
            switch (width8 & 0x03) // duff's device
            {
            case 0: do { *dest8++ = qt_convertToGray4<SRC>(src[0]) << 4
                                    | qt_convertToGray4<SRC>(src[1]);
                         src += 2;
            case 3:      *dest8++ = qt_convertToGray4<SRC>(src[0]) << 4
                                    | qt_convertToGray4<SRC>(src[1]);
                         src += 2;
            case 2:      *dest8++ = qt_convertToGray4<SRC>(src[0]) << 4
                                    | qt_convertToGray4<SRC>(src[1]);
                         src += 2;
            case 1:      *dest8++ = qt_convertToGray4<SRC>(src[0]) << 4
                                    | qt_convertToGray4<SRC>(src[1]);
                         src += 2;
            } while (--n > 0);
            }
        }

        if (doTail)
            *dest8 = qt_convertToGray4<SRC>(*src++) << 4 | (*dest8 & 0x0f);

        dest8 += dstStride;
        src += srcStride;
    }
}

template <>
void qt_rectconvert(qgray4 *dest, const quint32 *src,
                    int x, int y, int width, int height,
                    int dstStride, int srcStride)
{
    qt_rectconvert_gray4<quint32>(dest, src, x, y, width, height,
                                  dstStride, srcStride);
}

template <>
void qt_rectconvert(qgray4 *dest, const quint16 *src,
                    int x, int y, int width, int height,
                    int dstStride, int srcStride)
{
    qt_rectconvert_gray4<quint16>(dest, src, x, y, width, height,
                                  dstStride, srcStride);
}

template <>
void qt_rectconvert(qgray4 *dest, const qrgb444 *src,
                    int x, int y, int width, int height,
                    int dstStride, int srcStride)
{
    qt_rectconvert_gray4<qrgb444>(dest, src, x, y, width, height,
                                  dstStride, srcStride);
}

template <>
void qt_rectconvert(qgray4 *dest, const qargb4444 *src,
                    int x, int y, int width, int height,
                    int dstStride, int srcStride)
{
    qt_rectconvert_gray4<qargb4444>(dest, src, x, y, width, height,
                                    dstStride, srcStride);
}

static void blit_4(QScreen *screen, const QImage &image,
                   const QPoint &topLeft, const QRegion &region)
{
    switch (image.format()) {
    case QImage::Format_ARGB32_Premultiplied:
        blit_template<qgray4, quint32>(screen, image, topLeft, region);
        return;
    case QImage::Format_RGB16:
        blit_template<qgray4, quint16>(screen, image, topLeft, region);
        return;
    case QImage::Format_RGB444:
        blit_template<qgray4, qrgb444>(screen, image, topLeft, region);
        return;
    case QImage::Format_ARGB4444_Premultiplied:
        blit_template<qgray4, qargb4444>(screen, image, topLeft, region);
        return;
    default:
        qCritical("blit_4(): Image format %d not supported!", image.format());
    }
}
#endif // QT_QWS_DEPTH_4

#ifdef QT_QWS_DEPTH_1

struct qmono { quint8 dummy; } Q_PACKED;

template <typename SRC>
Q_STATIC_TEMPLATE_FUNCTION inline quint8 qt_convertToMono(SRC color);

template <>
inline quint8 qt_convertToMono(quint32 color)
{
    return qGray(color) >> 7;
}

template <>
inline quint8 qt_convertToMono(quint16 color)
{
    return (qGray(qt_colorConvert<quint32, quint16>(color, 0)) >> 7);
}

template <>
inline quint8 qt_convertToMono(qargb4444 color)
{
    return (qGray(quint32(color)) >> 7);
}

template <>
inline quint8 qt_convertToMono(qrgb444 color)
{
    return (qGray(quint32(color)) >> 7);
}

template <typename SRC>
inline void qt_rectconvert_mono(qmono *dest, const SRC *src,
                                       int x, int y, int width, int height,
                                       int dstStride, int srcStride)
{
    const int pixelsPerByte = 8;
    quint8 *dest8 = reinterpret_cast<quint8*>(dest)
                    + y * dstStride + x / pixelsPerByte;
    const int alignWidth = qMin(width, (8 - (x & 7)) & 7);
    const int doAlign = (alignWidth > 0 ? 1 : 0);
    const int alignStart = pixelsPerByte - 1 - (x & 7);
    const int alignStop = alignStart - (alignWidth - 1);
    const quint8 alignMask = ((1 << alignWidth) - 1) << alignStop;
    const int tailWidth = (width - alignWidth) & 7;
    const int doTail = (tailWidth > 0 ? 1 : 0);
    const quint8 tailMask = (1 << (pixelsPerByte - tailWidth)) - 1;
    const int width8 = (width - alignWidth) / pixelsPerByte;

    srcStride = srcStride / sizeof(SRC) - (width8 * 8 + alignWidth);
    dstStride -= (width8 + doAlign);

    for (int j = 0;  j < height; ++j) {
        if (doAlign) {
            quint8 d = *dest8 & ~alignMask;
            for (int i = alignStart; i >= alignStop; --i)
                d |= qt_convertToMono<SRC>(*src++) << i;
            *dest8++ = d;
        }
        for (int i = 0; i < width8; ++i) {
            *dest8 = (qt_convertToMono<SRC>(src[0]) << 7)
                     | (qt_convertToMono<SRC>(src[1]) << 6)
                     | (qt_convertToMono<SRC>(src[2]) << 5)
                     | (qt_convertToMono<SRC>(src[3]) << 4)
                     | (qt_convertToMono<SRC>(src[4]) << 3)
                     | (qt_convertToMono<SRC>(src[5]) << 2)
                     | (qt_convertToMono<SRC>(src[6]) << 1)
                     | (qt_convertToMono<SRC>(src[7]));
            src += 8;
            ++dest8;
        }
        if (doTail) {
            quint8 d = *dest8 & tailMask;
            switch (tailWidth) {
            case 7: d |= qt_convertToMono<SRC>(src[6]) << 1;
            case 6: d |= qt_convertToMono<SRC>(src[5]) << 2;
            case 5: d |= qt_convertToMono<SRC>(src[4]) << 3;
            case 4: d |= qt_convertToMono<SRC>(src[3]) << 4;
            case 3: d |= qt_convertToMono<SRC>(src[2]) << 5;
            case 2: d |= qt_convertToMono<SRC>(src[1]) << 6;
            case 1: d |= qt_convertToMono<SRC>(src[0]) << 7;
            }
            *dest8 = d;
        }

        dest8 += dstStride;
        src += srcStride;
    }
}

template <>
void qt_rectconvert(qmono *dest, const quint32 *src,
                    int x, int y, int width, int height,
                    int dstStride, int srcStride)
{
    qt_rectconvert_mono<quint32>(dest, src, x, y, width, height,
                                 dstStride, srcStride);
}

template <>
void qt_rectconvert(qmono *dest, const quint16 *src,
                    int x, int y, int width, int height,
                    int dstStride, int srcStride)
{
    qt_rectconvert_mono<quint16>(dest, src, x, y, width, height,
                                 dstStride, srcStride);
}

template <>
void qt_rectconvert(qmono *dest, const qrgb444 *src,
                    int x, int y, int width, int height,
                    int dstStride, int srcStride)
{
    qt_rectconvert_mono<qrgb444>(dest, src, x, y, width, height,
                                 dstStride, srcStride);
}

template <>
void qt_rectconvert(qmono *dest, const qargb4444 *src,
                    int x, int y, int width, int height,
                    int dstStride, int srcStride)
{
    qt_rectconvert_mono<qargb4444>(dest, src, x, y, width, height,
                                   dstStride, srcStride);
}

static void blit_1(QScreen *screen, const QImage &image,
                   const QPoint &topLeft, const QRegion &region)
{
    switch (image.format()) {
    case QImage::Format_ARGB32_Premultiplied:
        blit_template<qmono, quint32>(screen, image, topLeft, region);
        return;
    case QImage::Format_RGB16:
        blit_template<qmono, quint16>(screen, image, topLeft, region);
        return;
    case QImage::Format_RGB444:
        blit_template<qmono, qrgb444>(screen, image, topLeft, region);
        return;
    case QImage::Format_ARGB4444_Premultiplied:
        blit_template<qmono, qargb4444>(screen, image, topLeft, region);
        return;
    default:
        qCritical("blit_1(): Image format %d not supported!", image.format());
    }
}
#endif // QT_QWS_DEPTH_1

#ifdef QT_QWS_DEPTH_GENERIC

static void blit_rgb(QScreen *screen, const QImage &image,
                     const QPoint &topLeft, const QRegion &region)
{
    switch (image.format()) {
    case QImage::Format_ARGB32_Premultiplied:
        blit_template<qrgb, quint32>(screen, image, topLeft, region);
        return;
    case QImage::Format_RGB16:
        blit_template<qrgb, quint16>(screen, image, topLeft, region);
        return;
    default:
        qCritical("blit_rgb(): Image format %d not supported!", image.format());
    }
}

void qt_set_generic_blit(QScreen *screen, int bpp,
                         int len_red, int len_green, int len_blue, int len_alpha,
                         int off_red, int off_green, int off_blue, int off_alpha)
{
    qrgb::bpp = bpp / 8;
    qrgb::len_red = len_red;
    qrgb::len_green = len_green;
    qrgb::len_blue = len_blue;
    qrgb::len_alpha = len_alpha;
    qrgb::off_red = off_red;
    qrgb::off_green = off_green;
    qrgb::off_blue = off_blue;
    qrgb::off_alpha = off_alpha;
    screen->d_ptr->blit = blit_rgb;
    if (bpp == 16)
        screen->d_ptr->solidFill = solidFill_rgb_16bpp;
    else if (bpp == 32)
        screen->d_ptr->solidFill = solidFill_rgb_32bpp;
}

#endif // QT_QWS_DEPTH_GENERIC

void qt_blit_setup(QScreen *screen, const QImage &image,
                   const QPoint &topLeft, const QRegion &region)
{
    switch (screen->depth()) {
#ifdef QT_QWS_DEPTH_32
    case 32:
        if (screen->pixelType() == QScreen::NormalPixel)
            screen->d_ptr->blit = blit_32;
        else
            screen->d_ptr->blit = blit_template<qabgr8888, quint32>;
        break;
#endif
#ifdef QT_QWS_DEPTH_24
    case 24:
        if (screen->pixelType() == QScreen::NormalPixel)
            screen->d_ptr->blit = blit_qrgb888;
        else
            screen->d_ptr->blit = blit_24;
        break;
#endif
#ifdef QT_QWS_DEPTH_18
    case 18:
        screen->d_ptr->blit = blit_18;
        break;
#endif
#ifdef QT_QWS_DEPTH_16
    case 16:
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        if (screen->d_ptr->fb_is_littleEndian)
            screen->d_ptr->blit = blit_16_bigToLittleEndian;
        else
#endif
        if (screen->pixelType() == QScreen::NormalPixel)
            screen->d_ptr->blit = blit_16;
        else
            screen->d_ptr->blit = blit_template<qbgr565, quint16>;
        break;
#endif
#ifdef QT_QWS_DEPTH_15
    case 15:
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        if (screen->d_ptr->fb_is_littleEndian)
            screen->d_ptr->blit = blit_15_bigToLittleEndian;
        else
#endif // Q_BIG_ENDIAN
        if (screen->pixelType() == QScreen::NormalPixel)
            screen->d_ptr->blit = blit_15;
        else
            screen->d_ptr->blit = blit_template<qbgr555, qrgb555>;
        break;
#endif
#ifdef QT_QWS_DEPTH_12
    case 12:
        screen->d_ptr->blit = blit_12;
        break;
#endif
#ifdef QT_QWS_DEPTH_8
    case 8:
        screen->d_ptr->blit = blit_8;
        break;
#endif
#ifdef QT_QWS_DEPTH_4
    case 4:
        screen->d_ptr->blit = blit_4;
        break;
#endif
#ifdef QT_QWS_DEPTH_1
    case 1:
        screen->d_ptr->blit = blit_1;
        break;
#endif
    default:
        qFatal("blit_setup(): Screen depth %d not supported!",
               screen->depth());
        screen->d_ptr->blit = 0;
        break;
    }
    screen->d_ptr->blit(screen, image, topLeft, region);
}

QScreenPrivate::QScreenPrivate(QScreen *parent, QScreen::ClassId id)
    : defaultGraphicsSystem(QWSGraphicsSystem(parent)),
      pixelFormat(QImage::Format_Invalid),
#ifdef QT_QWS_CLIENTBLIT
      supportsBlitInClients(false),
#endif
      classId(id), q_ptr(parent)
{
    solidFill = qt_solidFill_setup;
    blit = qt_blit_setup;
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    fb_is_littleEndian = false;
#endif
    pixmapFactory = 0;
    graphicsSystem = &defaultGraphicsSystem;
}

QScreenPrivate::~QScreenPrivate()
{
}

QImage::Format QScreenPrivate::preferredImageFormat() const
{
    if (pixelFormat > QImage::Format_Indexed8)
        return pixelFormat;

    if (q_ptr->depth() <= 16)
        return QImage::Format_RGB16;
    else
        return QImage::Format_ARGB32_Premultiplied;
}

/*!
    \class QScreen
    \ingroup qws

    \brief The QScreen class is a base class for screen drivers in
    Qt for Embedded Linux.

    Note that this class is only available in \l{Qt for Embedded Linux}.

    \l{Qt for Embedded Linux} provides ready-made drivers for several screen
    protocols, see the \l{Qt for Embedded Linux Display Management}{display
    management} documentation for details. Custom screen drivers can
    be implemented by subclassing the QScreen class and creating a
    screen driver plugin (derived from QScreenDriverPlugin). The
    default implementation of the QScreenDriverFactory class
    will automatically detect the plugin, and load the driver into the
    server application at run-time using Qt's \l {How to Create Qt
    Plugins}{plugin system}.

    When rendering, the default behavior is for each
    client to render its widgets as well as its decorations into
    memory, while the server copies the memory content to the device's
    framebuffer using the screen driver. See the \l{Qt for Embedded Linux
    Architecture} overview for details (note that it is possible for
    the clients to manipulate and control the underlying hardware
    directly as well).

    Starting with Qt 4.2, it is also possible to add an
    accelerated graphics driver to take advantage of available
    hardware resources. See the \l{Adding an Accelerated Graphics
    Driver to Qt for Embedded Linux} documentation for details.

    \tableofcontents

    \section1 Framebuffer Management

    When a \l{Qt for Embedded Linux} application starts running, it
    calls the screen driver's connect() function to map the
    framebuffer and the accelerated drivers that the graphics card
    control registers. The connect() function should then read out the
    parameters of the framebuffer and use them as required to set this
    class's protected variables.

    The initDevice() function can be reimplemented to initialize the
    graphics card. Note, however, that connect() is called \e before
    the initDevice() function, so, for some hardware configurations,
    some of the initialization that would normally be done in the
    initDevice() function might have to be done in the connect()
    function.

    Likewise, just before a \l{Qt for Embedded Linux} application
    exits, it calls the screen driver's disconnect() function. The
    server application will in addition call the shutdownDevice()
    function before it calls disconnect(). Note that the default
    implementation of the shutdownDevice() function only hides the
    mouse cursor.

    QScreen also provides the save() and restore() functions, making
    it possible to save and restore the state of the graphics
    card. Note that the default implementations do nothing. Hardware
    screen drivers should reimplement these functions to save (and
    restore) its registers, enabling switching between virtual
    consoles.

    In addition, you can use the base() function to retrieve a pointer
    to the beginning of the framebuffer, and the region() function to
    retrieve the framebuffer's region. Use the onCard() function to
    determine whether the framebuffer is within the graphics card's
    memory, and the totalSize() function to determine the size of the
    available graphics card memory (including the screen). Finally,
    you can use the offset() function to retrieve the offset between
    the framebuffer's coordinates and the application's coordinate
    system.

    \section1 Palette Management

    QScreen provides several functions to retrieve information about
    the color palette: The clut() function returns a pointer to the
    color lookup table (i.e. its color palette). Use the colorCount()
    function to determine the number of entries in this table, and the
    alloc() function to retrieve the palette index of the color that
    is the closest match to a given RGB value.

    To determine if the screen driver supports a given color depth,
    use the supportsDepth() function that returns true of the
    specified depth is supported.

    \section1 Drawing on Screen

    When a screen update is required, the \l{Qt for Embedded Linux} server runs
    through all the top-level windows that intersect with the region
    that is about to be updated, and ensures that the associated
    clients have updated their memory buffer. Then the server calls
    the exposeRegion() function that composes the window surfaces and
    copies the content of memory to screen by calling the blit() and
    solidFill() functions.

    The blit() function copies a given region in a given image to a
    specified point using device coordinates, while the solidFill()
    function fills the given region of the screen with the specified
    color. Note that normally there is no need to call either of these
    functions explicitly.

    In addition, QScreen provides the blank() function that can be
    reimplemented to prevent any contents from being displayed on the
    screen, and the setDirty() function that can be reimplemented to
    indicate that a given rectangle of the screen has been
    altered. Note that the default implementations of these functions
    do nothing.

    Reimplement the mapFromDevice() and mapToDevice() functions to
    map objects from the framebuffer coordinate system to the
    coordinate space used by the application, and vice versa. Be aware
    that the default implementations simply return the given objects
    as they are.

    \section1 Properties

    \table
    \header \o Property \o Functions
    \row
    \o Size
    \o

    The size of the screen can be retrieved using the screenSize()
    function. The size is returned in bytes.

    The framebuffer's logical width and height can be retrieved using
    width() and height(), respectively. These functions return values
    are given in pixels. Alternatively, the physicalWidth() and
    physicalHeight() function returns the same metrics in
    millimeters. QScreen also provides the deviceWidth() and
    deviceHeight() functions returning the physical width and height
    of the device in pixels. Note that the latter metrics can differ
    from the ones used if the display is centered within the
    framebuffer.

    \row
    \o Resolution
    \o

    Reimplement the setMode() function to be able to set the
    framebuffer to a new resolution (width and height) and bit depth.

    The current depth of the framebuffer can be always be retrieved
    using the depth() function. Use the pixmapDepth() function to
    obtain the preferred depth for pixmaps.

    \row
    \o Pixmap Alignment
    \o

    Use the pixmapOffsetAlignment() function to retrieve the value to
    which the start address of pixmaps held in the graphics card's
    memory, should be aligned.

    Use the pixmapLinestepAlignment() to retrieve the value to which
    the \e {individual scanlines} of pixmaps should be aligned.

    \row
    \o Image Display
    \o

    The isInterlaced() function tells whether the screen is displaying
    images progressively, and the isTransformed() function whether it
    is rotated. The transformOrientation() function can be
    reimplemented to return the current rotation.

    \row
    \o Scanlines
    \o

    Use the linestep() function to retrieve the length of each
    scanline of the framebuffer.

    \row
    \o Pixel Type
    \o

    The pixelType() function returns the screen's pixel storage format as
    described by the PixelType enum.

    \endtable

    \section1 Subclassing and Initial Values

    You need to set the following members when implementing a subclass of QScreen:

    \table
    \header \o Member \o Initial Value
    \row \o \l{QScreen::}{data} \o A pointer to the framebuffer if possible;
    0 otherwise.
    \row \o \l{QScreen::}{lstep} \o The number of bytes between each scanline
    in the framebuffer.
    \row \o \l{QScreen::}{w} \o The logical screen width in pixels.
    \row \o \l{QScreen::}{h} \o The logical screen height in pixels.
    \row \o \l{QScreen::}{dw} \o The real screen width in pixels.
    \row \o \l{QScreen::}{dh} \o The real screen height in pixels.
    \row \o \l{QScreen::}{d} \o The number of bits per pixel.
    \row \o \l{QScreen::}{physWidth} \o The screen width in millimeters.
    \row \o \l{QScreen::}{physHeight} \o The screen height in millimeters.
    \endtable

    The logical screen values are the same as the real screen values unless the
    screen is transformed in some way; e.g., rotated.

    See also the \l{Accelerated Graphics Driver Example} for an example that
    shows how to initialize these values.

    \sa QScreenDriverPlugin, QScreenDriverFactory, {Qt for Embedded Linux Display
    Management}
*/

/*!
    \enum QScreen::PixelType

    This enum describes the pixel storage format of the screen,
    i.e. the order of the red (R), green (G) and blue (B) components
    of a pixel.

    \value NormalPixel Red-green-blue (RGB)
    \value BGRPixel Blue-green-red (BGR)

    \sa pixelType()
*/

/*!
    \enum QScreen::ClassId

    This enum defines the class identifiers for the known screen subclasses.

    \value LinuxFBClass QLinuxFBScreen
    \value TransformedClass QTransformedScreen
    \value VNCClass QVNCScreen
    \value MultiClass QMultiScreen
    \value VFbClass QVFbScreen
    \value DirectFBClass QDirectFBScreen
    \value SvgalibClass QSvgalibScreen
    \value ProxyClass QProxyScreen
    \value GLClass QGLScreen
    \value CustomClass Unknown QScreen subclass

    \sa classId()
*/

/*!
  \variable QScreen::screenclut
  \brief the color table

  Initialize this variable in a subclass using a paletted screen mode,
  and initialize its partner, QScreen::screencols.

  \sa screencols
*/

/*!
  \variable QScreen::screencols
  \brief the number of entries in the color table

  Initialize this variable in a subclass using a paletted screen mode,
  and initialize its partner, QScreen::screenclut.

  \sa screenclut
*/

/*!
  \variable QScreen::data
  \brief points to the first visible pixel in the frame buffer.

  You must initialize this variable if you are using the default
  implementation of non-buffered painting Qt::WA_PaintOnScreen,
  QPixmap::grabWindow() or QDirectPainter::frameBuffer(). If you
  initialize this variable, you must also initialize QScreen::size and
  QScreen::mapsize.

  \sa QScreen::size, QScreen::mapsize
*/

/*!
  \variable QScreen::w
  \brief the logical width of the screen.

  This variable \e{must} be initialized by a subclass.
*/

/*!
  \variable QScreen::lstep
  \brief the number of bytes representing a line in the frame buffer.

  i.e., \e{line step}. \c {data[lstep * 2]} is the address of the
  first visible pixel in the third line of the frame buffer.

  \sa data
*/

/*!
  \variable QScreen::h
  \brief the logical height of the screen.

  This variable \e{must} be initialized by a subclass.
*/

/*!
  \variable QScreen::d
  \brief the pixel depth

  This is the number of significant bits used to set a pixel
  color. This variable \e{must} be initialized by a subclass.
*/

/*!
  \variable QScreen::pixeltype
  \brief set to BGRPixel

  Set this variable to BGRPixel in a subclass, if the screen pixel
  format is a BGR type and you have used setPixelFormat() to set the
  pixel format to the corresponding RGB format. e.g., you have set the
  pixel format to QImage::Format_RGB555, but your screen really uses
  BGR, not RGB.
*/

/*!
  \variable QScreen::grayscale
  \brief the gray scale screen mode flag

  Set this variable to true in a subclass, if you are using a
  grayscale screen mode. e.g., in an 8-bit mode where you don't want
  to use the palette, but you want to use the grayscales.
*/

/*!
  \variable QScreen::dw
  \brief the device width

  This is the number of pixels in a row of the physical screen.  It
  \e{must} be initialized by a subclass. Normally, it should be set to
  the logical width QScreen::w, but it might be different, e.g., if
  you are doing rotations in software.

  \sa QScreen::w
*/

/*!
  \variable QScreen::dh
  \brief the device height

  This is the number of pixels in a column of the physical screen.  It
  \e{must} be initialized by a subclass. Normally, it should be set to
  the logical height QScreen::h, but it might be different, e.g., if
  you are doing rotations in software.

  \sa QScreen::h
*/

/*!
  \variable QScreen::size
  \brief the number of bytes in the visible region of the frame buffer

  This is the number of bytes in the visible part of the block pointed
  to by the QScreen::data pointer. You must initialize this variable
  if you initialize the QScreen::data pointer.

  \sa QScreen::data, QScreen::mapsize
*/

/*!
  \variable QScreen::mapsize
  \brief the total number of bytes in the frame buffer

  This is the total number of bytes in the block pointed to by the
  QScreen::data pointer. You must initialize this variable if you
  initialize the QScreen::data pointer.

  \sa QScreen::data, QScreen::size
*/

/*!
  \variable QScreen::physWidth
  \brief the physical width of the screen in millimeters.

  Currently, this variable is used when calculating the screen DPI,
  which in turn is used when deciding the actual font size Qt is
  using.
*/

/*!
  \variable QScreen::physHeight
  \brief the physical height of the screen in millimeters.

  Currently, this variable is used when calculating the screen DPI,
  which in turn is used when deciding the actual font size Qt is
  using.
*/

/*!
    \fn static QScreen* QScreen::instance()

    Returns a pointer to the application's QScreen instance.

    If this screen consists of several subscreens, operations to the
    returned instance will affect all its subscreens. Use the
    subscreens() function to retrieve access to a particular
    subscreen.

    \sa subScreens(), subScreenIndexAt()
*/

/*!
    \fn QList<QScreen*> QScreen::subScreens() const
    \since 4.2

    Returns a list of this screen's subscreens. Use the
    subScreenIndexAt() function to retrieve the index of a screen at a
    given position.

    Note that if \e this screen consists of several subscreens,
    operations to \e this instance will affect all subscreens by
    default.

    \sa instance(), subScreenIndexAt()
*/

/*!
    \fn int QScreen::physicalWidth() const
    \since 4.2

    Returns the physical width of the screen in millimeters.

    \sa width(), deviceWidth(), physicalHeight()
*/

/*!
    \fn int QScreen::physicalHeight() const
    \since 4.2

    Returns the physical height of the screen in millimeters.

    \sa height(), deviceHeight(), physicalWidth()
*/

/*!
    \fn virtual bool QScreen::initDevice() = 0

    This function is called by the \l{Qt for Embedded Linux} server to
    initialize the framebuffer. Note that a server application will call the
    connect() function prior to this function.

    Implement this function to make accelerated drivers set up the
    graphics card. Return true to indicate success and false to indicate
    failure.

    \sa shutdownDevice(), connect()
*/

/*!
    \fn virtual bool QScreen::connect(const QString &displaySpec) = 0

    This function is called by every \l{Qt for Embedded Linux}
    application on startup, and must be implemented to map in the
    framebuffer and the accelerated drivers that the graphics card
    control registers.  Note that connect must be called \e before
    the initDevice() function.

    Ensure that true is returned if a connection to the screen device
    is made. Otherwise, return false. Upon making the connection, the
    function should read out the parameters of the framebuffer and use
    them as required to set this class's protected variables.

    The \a displaySpec argument is passed by the QWS_DISPLAY
    environment variable or the -display command line parameter, and
    has the following syntax:

    \snippet doc/src/snippets/code/src_gui_embedded_qscreen_qws.cpp 0

    For example, to use the mach64 driver on fb1 as display 2:

    \snippet doc/src/snippets/code/src_gui_embedded_qscreen_qws.cpp 1

    See \l{Qt for Embedded Linux Display Management} for more details.

    \sa disconnect(), initDevice(), {Running Qt for Embedded Linux Applications}
*/

/*!
    \fn QScreen::disconnect()

    This function is called by every \l{Qt for Embedded Linux} application
    before exiting, and must be implemented to unmap the
    framebuffer. Note that a server application will call the
    shutdownDevice() function prior to this function.

    \sa connect(), shutdownDevice(), {Running Qt for Embedded Linux
    Applications}
*/

/*!
    \fn QScreen::setMode(int width, int height, int depth)

    Implement this function to reset the framebuffer's resolution (\a
    width and \a height) and bit \a depth.

    After the resolution has been set, existing paint engines will be
    invalid and the framebuffer should be completely redrawn. In a
    multiple-process situation, all other applications must be
    notified to reset their mode and update themselves accordingly.
*/

/*!
    \fn QScreen::blank(bool on)

    Prevents the screen driver form displaying any content on the
    screen.

    Note that the default implementation does nothing.

    Reimplement this function to prevent the screen driver from
    displaying any contents on the screen if \a on is true; otherwise
    the contents is expected to be shown.

    \sa blit()
*/

/*!
    \fn int QScreen::pixmapOffsetAlignment()

    Returns the value (in bits) to which the start address of pixmaps
    held in the graphics card's memory, should be aligned.

    Note that the default implementation returns 64; reimplement this
    function to override the return value, e.g., when implementing an
    accelerated driver (see the \l {Adding an Accelerated Graphics
    Driver to Qt for Embedded Linux}{Adding an Accelerated Graphics Driver}
    documentation for details).

    \sa pixmapLinestepAlignment()
*/

/*!
    \fn int QScreen::pixmapLinestepAlignment()

    Returns the value (in bits) to which individual scanlines of
    pixmaps held in the graphics card's memory, should be
    aligned.

    Note that the default implementation returns 64; reimplement this
    function to override the return value, e.g., when implementing an
    accelerated driver (see the \l {Adding an Accelerated Graphics
    Driver to Qt for Embedded Linux}{Adding an Accelerated Graphics Driver}
    documentation for details).

    \sa pixmapOffsetAlignment()
*/

/*!
    \fn QScreen::width() const

    Returns the logical width of the framebuffer in pixels.

    \sa deviceWidth(), physicalWidth(), height()
*/

/*!
    \fn int QScreen::height() const

    Returns the logical height of the framebuffer in pixels.

    \sa deviceHeight(), physicalHeight(), width()
*/

/*!
    \fn QScreen::depth() const

    Returns the depth of the framebuffer, in bits per pixel.

    Note that the returned depth is the number of bits each pixel
    fills rather than the number of significant bits, so 24bpp and
    32bpp express the same range of colors (8 bits of red, green and
    blue).

    \sa clut(), pixmapDepth()
*/

/*!
    \fn int QScreen::pixmapDepth() const

    Returns the preferred depth for pixmaps, in bits per pixel.

    \sa depth()
*/

/*!
    \fn QScreen::linestep() const

    Returns the length of each scanline of the framebuffer in bytes.

    \sa isInterlaced()
*/

/*!
    \fn QScreen::deviceWidth() const

    Returns the physical width of the framebuffer device in pixels.

    Note that the returned width can differ from the width which
    \l{Qt for Embedded Linux} will actually use, that is if the display is
    centered within the framebuffer.

    \sa width(), physicalWidth(), deviceHeight()
*/

/*!
    \fn QScreen::deviceHeight() const

    Returns the full height of the framebuffer device in pixels.

    Note that the returned height can differ from the height which
    \l{Qt for Embedded Linux} will actually use, that is if the display is
    centered within the framebuffer.

    \sa height(), physicalHeight(), deviceWidth()
*/

/*!
    \fn uchar *QScreen::base() const

    Returns a pointer to the beginning of the framebuffer.

    \sa onCard(), region(), totalSize()
*/

/*!
    \fn uchar *QScreen::cache(int)

    \internal

    This function is used to store pixmaps in graphics memory for the
    use of the accelerated drivers. See QLinuxFbScreen (where the
    caching is implemented) for more information.
*/

/*!
    \fn QScreen::uncache(uchar *)

    \internal

    This function is called on pixmap destruction to remove them from
    graphics card memory.
*/

/*!
    \fn QScreen::screenSize() const

    Returns the size of the screen in bytes.

    The screen size is always located at the beginning of framebuffer
    memory, i.e. it can also be retrieved using the base() function.

    \sa base(), region()
*/

/*!
    \fn QScreen::totalSize() const

    Returns the size of the available graphics card memory (including
    the screen) in bytes.

    \sa onCard()
*/

// Unaccelerated screen/driver setup. Can be overridden by accelerated
// drivers

/*!
    \fn QScreen::QScreen(int displayId)

    Constructs a new screen driver.

    The \a displayId identifies the \l{Qt for Embedded Linux} server to connect
    to.
*/

/*!
    \fn QScreen::clut()

    Returns a pointer to the screen's color lookup table (i.e. its
    color palette).

    Note that this function only apply in paletted modes like 8-bit,
    i.e. in modes where only the palette indexes (and not the actual
    color values) are stored in memory.

    \sa alloc(), depth(), colorCount()
*/

/*!
    \obsolete
    \fn int QScreen::numCols()

    \sa colorCount()
*/

/*!
    \since 4.6
    \fn int QScreen::colorCount()

    Returns the number of entries in the screen's color lookup table
    (i.e. its color palette). A pointer to the color table can be
    retrieved using the clut() function.

    \sa clut(), alloc()
*/

/*!
    \since 4.4

    Constructs a new screen driver.

    The \a display_id identifies the \l{Qt for Embedded Linux}
    server to connect to. The \a classId specifies the class
    identifier.
*/
QScreen::QScreen(int display_id, ClassId classId)
    : screencols(0), data(0), entries(0), entryp(0), lowest(0),
      w(0), lstep(0), h(0), d(1), pixeltype(NormalPixel), grayscale(false),
      dw(0), dh(0), size(0), mapsize(0), displayId(display_id),
      physWidth(0), physHeight(0), d_ptr(new QScreenPrivate(this, classId))
{
    clearCacheFunc = 0;
}

QScreen::QScreen(int display_id)
    : screencols(0), data(0), entries(0), entryp(0), lowest(0),
      w(0), lstep(0), h(0), d(1), pixeltype(NormalPixel), grayscale(false),
      dw(0), dh(0), size(0), mapsize(0), displayId(display_id),
      physWidth(0), physHeight(0), d_ptr(new QScreenPrivate(this))
{
    clearCacheFunc = 0;
}

/*!
    Destroys this screen driver.
*/

QScreen::~QScreen()
{
    delete d_ptr;
}

/*!
    This function is called by the \l{Qt for Embedded Linux} server before it
    calls the disconnect() function when exiting.

    Note that the default implementation only hides the mouse cursor;
    reimplement this function to do the necessary graphics card
    specific cleanup.

    \sa initDevice(), disconnect()
*/

void QScreen::shutdownDevice()
{
#ifndef QT_NO_QWS_CURSOR
    if (qt_screencursor)
        qt_screencursor->hide();
#endif
}

extern bool qws_accel; //in qapplication_qws.cpp

/*!
    \fn PixelType QScreen::pixelType() const

    Returns the pixel storage format of the screen.
*/

/*!
  Returns the pixel format of the screen, or \c QImage::Format_Invalid
  if the pixel format is not a supported image format.

*/
QImage::Format QScreen::pixelFormat() const
{
    return d_ptr->pixelFormat;
}

/*!
  Sets the screen's pixel format to \a format.
 */
void QScreen::setPixelFormat(QImage::Format format)
{
    d_ptr->pixelFormat = format;
}


/*!
    \fn int QScreen::alloc(unsigned int red, unsigned int green, unsigned int blue)

    Returns the index in the screen's palette which is the closest
    match to the given RGB value (\a red, \a green, \a blue).

    Note that this function only apply in paletted modes like 8-bit,
    i.e. in modes where only the palette indexes (and not the actual
    color values) are stored in memory.

    \sa clut(), colorCount()
*/

int QScreen::alloc(unsigned int r,unsigned int g,unsigned int b)
{
    int ret = 0;
    if (d == 8) {
        if (grayscale)
            return qGray(r, g, b);

        // First we look to see if we match a default color
        const int pos = (r + 25) / 51 * 36 + (g + 25) / 51 * 6 + (b + 25) / 51;
        if (pos < screencols && screenclut[pos] == qRgb(r, g, b)) {
            return pos;
        }

        // search for nearest color
        unsigned int mindiff = 0xffffffff;
        unsigned int diff;
        int dr,dg,db;

        for (int loopc = 0; loopc < screencols; ++loopc) {
            dr = qRed(screenclut[loopc]) - r;
            dg = qGreen(screenclut[loopc]) - g;
            db = qBlue(screenclut[loopc]) - b;
            diff = dr*dr + dg*dg + db*db;

            if (diff < mindiff) {
                ret = loopc;
                if (!diff)
                    break;
                mindiff = diff;
            }
        }
    } else if (d == 4) {
        ret = qGray(r, g, b) >> 4;
    } else if (d == 1) {
        ret = qGray(r, g, b) >= 128;
    } else {
        qFatal("cannot alloc %dbpp color", d);
    }

    return ret;
}

/*!
    Saves the current state of the graphics card.

    For example, hardware screen drivers should reimplement the save()
    and restore() functions to save and restore its registers,
    enabling swintching between virtual consoles.

    Note that the default implementation does nothing.

    \sa restore()
*/

void QScreen::save()
{
}

/*!
    Restores the previously saved state of the graphics card.

    For example, hardware screen drivers should reimplement the save()
    and restore() functions to save and restore its registers,
    enabling swintching between virtual consoles.

    Note that the default implementation does nothing.

    \sa save()
*/

void QScreen::restore()
{
}

void QScreen::blank(bool)
{
}

/*!
    \internal
*/

void QScreen::set(unsigned int, unsigned int, unsigned int, unsigned int)
{
}

/*!
    \fn bool QScreen::supportsDepth(int depth) const

    Returns true if the screen supports the specified color \a depth;
    otherwise returns false.

    \sa clut()
*/

bool QScreen::supportsDepth(int d) const
{
    if (false) {
        //Just to simplify the ifdeffery
#ifdef QT_QWS_DEPTH_1
    } else if(d==1) {
        return true;
#endif
#ifdef QT_QWS_DEPTH_4
    } else if(d==4) {
        return true;
#endif
#ifdef QT_QWS_DEPTH_8
    } else if(d==8) {
        return true;
#endif
#ifdef QT_QWS_DEPTH_16
    } else if(d==16) {
        return true;
#endif
#ifdef QT_QWS_DEPTH_15
    } else if (d == 15) {
        return true;
#endif
#ifdef QT_QWS_DEPTH_18
    } else if(d==18 || d==19) {
        return true;
#endif
#ifdef QT_QWS_DEPTH_24
    } else if(d==24) {
        return true;
#endif
#ifdef QT_QWS_DEPTH_32
    } else if(d==32) {
        return true;
#endif
    }
    return false;
}

/*!
    \fn bool QScreen::onCard(const unsigned char *buffer) const

    Returns true if the specified \a buffer is within the graphics
    card's memory; otherwise returns false (i.e. if it's in main RAM).

    \sa base(), totalSize()
*/

bool QScreen::onCard(const unsigned char * p) const
{
    long t=(unsigned long)p;
    long bmin=(unsigned long)data;
    if (t < bmin)
        return false;
    if(t >= bmin+mapsize)
        return false;
    return true;
}

/*!
    \fn bool QScreen::onCard(const unsigned char * buffer, ulong& offset) const
    \overload

    If the specified \a buffer is within the graphics card's memory,
    this function stores the offset from the start of graphics card
    memory (in bytes), in the location specified by the \a offset
    parameter.
*/

bool QScreen::onCard(const unsigned char * p, ulong& offset) const
{
    long t=(unsigned long)p;
    long bmin=(unsigned long)data;
    if (t < bmin)
        return false;
    long o = t - bmin;
    if (o >= mapsize)
        return false;
    offset = o;
    return true;
}

/*
#if !defined(QT_NO_QWS_REPEATER)
    { "Repeater", qt_get_screen_repeater, 0 },
#endif
#if defined(QT_QWS_EE)
    { "EE", qt_get_screen_ee, 0 },
#endif

*/

/*
Given a display_id (number of the \l{Qt for Embedded Linux} server to connect to)
and a spec (e.g. Mach64:/dev/fb0) return a QScreen-descendant.
The QScreenDriverFactory is queried for a suitable driver and, if found,
asked to create a driver.
People writing new graphics drivers should either hook their own
QScreen-descendant into QScreenDriverFactory or use the QScreenDriverPlugin
to make a dynamically loadable driver.
*/

Q_GUI_EXPORT QScreen* qt_get_screen(int display_id, const char *spec)
{
    QString displaySpec = QString::fromAscii(spec);
    QString driver = displaySpec;
    int colon = displaySpec.indexOf(QLatin1Char(':'));
    if (colon >= 0)
        driver.truncate(colon);
    driver = driver.trimmed();

    bool foundDriver = false;
    QString driverName = driver;

    QStringList driverList;
    if (!driver.isEmpty())
        driverList << driver;
    else
        driverList = QScreenDriverFactory::keys();

    for (int i = 0; i < driverList.size(); ++i) {
        const QString driverName = driverList.at(i);
        qt_screen = QScreenDriverFactory::create(driverName, display_id);
        if (qt_screen) {
            foundDriver = true;
            if (qt_screen->connect(displaySpec)) {
                return qt_screen;
            } else {
                delete qt_screen;
                qt_screen = 0;
            }
        }
    }

    if (driver.isNull())
        qFatal("No suitable driver found");
    else if (foundDriver)
        qFatal("%s: driver cannot connect", driver.toLatin1().constData());
    else
        qFatal("%s: driver not found", driver.toLatin1().constData());

    return 0;
}

#ifndef QT_NO_QWS_CURSOR
static void blendCursor(QImage *dest, const QImage &cursor, const QPoint &offset)
{
    QRasterBuffer rb;
    rb.prepare(dest);

    QSpanData spanData;
    spanData.init(&rb, 0);
    spanData.type = QSpanData::Texture;
    spanData.initTexture(&cursor, 256);
    spanData.dx = -offset.x();
    spanData.dy = -offset.y();
    if (!spanData.blend)
        return;

    const QRect rect = QRect(offset, cursor.size())
                       & QRect(QPoint(0, 0), dest->size());
    const int w = rect.width();
    const int h = rect.height();

    QVarLengthArray<QT_FT_Span, 32> spans(h);
    for (int i = 0; i < h; ++i) {
        spans[i].x = rect.x();
        spans[i].len = w;
        spans[i].y = rect.y() + i;
        spans[i].coverage = 255;
    }
    spanData.blend(h, spans.constData(), &spanData);
}
#endif // QT_NO_QWS_CURSOR

/*!
    \fn void QScreen::exposeRegion(QRegion region, int windowIndex)

    This function is called by the \l{Qt for Embedded Linux} server whenever a
    screen update is required. \a region is the area on the screen
    that must be updated, and \a windowIndex is the index into
    QWSServer::clientWindows() of the window that required the
    update. QWSWindow::state() gives more information about the cause.

    The default implementation composes the
    affected windows and paints the given \a region on screen by
    calling the blit() and solidFill() functions

    This function can be reimplemented to perform composition in
    hardware, or to perform transition effects.
    For simpler hardware acceleration, or to interface with
    this is typically done by reimplementing the blit() and
    solidFill() functions instead.

    Note that there is no need to call this function explicitly.

    \sa blit(), solidFill(), blank()
*/
void QScreen::exposeRegion(QRegion r, int windowIndex)
{
    r &= region();
    if (r.isEmpty())
        return;

    int changing = windowIndex;
    // when we have just lowered a window, we have to expose all the windows below where the
    // window used to be.
    if (changing && qwsServer->clientWindows().at(changing)->state() == QWSWindow::Lowering)
        changing = 0;
#ifdef QTOPIA_PERFTEST
    static enum { PerfTestUnknown, PerfTestOn, PerfTestOff } perfTestState = PerfTestUnknown;
    if(PerfTestUnknown == perfTestState) {
        if(::getenv("QTOPIA_PERFTEST"))
            perfTestState = PerfTestOn;
        else
            perfTestState = PerfTestOff;
    }
    if(PerfTestOn == perfTestState) {
        QWSWindow *changed = qwsServer->clientWindows().at(changing);
        if(!changed->client()->identity().isEmpty())
            qDebug() << "Performance  :  expose_region  :"
                     << changed->client()->identity()
                     << r.boundingRect() << ": "
                     << qPrintable( QTime::currentTime().toString( "h:mm:ss.zzz" ) );
    }
#endif

    const QRect bounds = r.boundingRect();
    QRegion blendRegion;
    QImage *blendBuffer = 0;

#ifndef QT_NO_QWS_CURSOR
    if (qt_screencursor && !qt_screencursor->isAccelerated()) {
        blendRegion = r & qt_screencursor->boundingRect();
    }
#endif
    compose(0, r, blendRegion, &blendBuffer, changing);

    if (blendBuffer && !blendBuffer->isNull()) {
        const QPoint offset = blendRegion.boundingRect().topLeft();
#ifndef QT_NO_QWS_CURSOR
        if (qt_screencursor && !qt_screencursor->isAccelerated()) {
            const QRect cursorRect = qt_screencursor->boundingRect();
            if (blendRegion.intersects(cursorRect)) {
                blendCursor(blendBuffer, qt_screencursor->image(),
                            cursorRect.topLeft() - offset);
            }
        }
#endif // QT_NO_QWS_CURSOR
        blit(*blendBuffer, offset, blendRegion);
        delete blendBuffer;
    }

    if (r.rectCount() == 1) {
        setDirty(r.boundingRect());
    } else {
        const QVector<QRect> rects = r.rects();
        for (int i = 0; i < rects.size(); ++i)
            setDirty(rects.at(i));
    }
}

/*!
    \fn void QScreen::blit(const QImage &image, const QPoint &topLeft, const QRegion &region)

    Copies the given \a region in the given \a image to the point
    specified by \a topLeft using device coordinates.

    This function is called from the exposeRegion() function; it is
    not intended to be called explicitly.

    Reimplement this function to make use of \l{Adding an Accelerated
    Graphics Driver to Qt for Embedded Linux}{accelerated hardware}. Note that
    this function must be reimplemented if the framebuffer format is
    not supported by \l{Qt for Embedded Linux} (See the
    \l{Qt for Embedded Linux Display Management}{Display Management}
    documentation for more details).

    \sa exposeRegion(), solidFill(), blank()
*/
void QScreen::blit(const QImage &img, const QPoint &topLeft, const QRegion &reg)
{
    const QRect bound = (region() & QRect(topLeft, img.size())).boundingRect();
    QWSDisplay::grab();
    d_ptr->blit(this, img, topLeft - offset(),
            (reg & bound).translated(-topLeft));
    QWSDisplay::ungrab();
}

#ifdef QT_QWS_CLIENTBLIT
/*!
  Returns true if this screen driver supports calling QScreen::blit() and
  QScreen::setDirty() directly from non-server applications, otherwise returns
  false.

  If available, this is used to optimize the performance of non-occluded, opaque
  client windows by removing the server round trip when they are updated.

  \sa setSupportsBlitInClients()
 */
bool QScreen::supportsBlitInClients() const
{
    return d_ptr->supportsBlitInClients;
}

/*!
  If \a supported, the screen driver is marked as supporting blitting directly
  from non-server applications.

  \sa supportsBlitInClients()
 */
void QScreen::setSupportsBlitInClients(bool supported)
{
    d_ptr->supportsBlitInClients = supported;
}
#endif

/*!
    \internal
*/

void QScreen::blit(QWSWindow *win, const QRegion &clip)
{
    QWSWindowSurface *surface = win->windowSurface();
    if (!surface)
        return;

    const QImage &img = surface->image();
    if (img.isNull())
        return;

    const QRegion rgn = clip & win->paintedRegion();
    if (rgn.isEmpty())
        return;

    surface->lock();
    blit(img, win->requestedRegion().boundingRect().topLeft(), rgn);
    surface->unlock();
}

struct fill_data {
    quint32 color;
    uchar *data;
    int lineStep;
    int x;
    int y;
    int w;
    int h;
};

/*!
    Fills the given \a region of the screen with the specified \a
    color.

    This function is called from the exposeRegion() function; it is
    not intended to be called explicitly.

    Reimplement this function to make use of \l{Adding an Accelerated
    Graphics Driver to Qt for Embedded Linux}{accelerated hardware}. Note that
    this function must be reimplemented if the framebuffer format is
    not supported by \l{Qt for Embedded Linux} (See the
    \l{Qt for Embedded Linux Display Management}{Display Management}
    documentation for more details).

    \sa exposeRegion(), blit(), blank()
*/
// the base class implementation works in device coordinates, so that transformed drivers can use it
void QScreen::solidFill(const QColor &color, const QRegion &region)
{
    QWSDisplay::grab();
    d_ptr->solidFill(this, color,
                     region.translated(-offset()) & QRect(0, 0, dw, dh));
    QWSDisplay::ungrab();
}

/*!
    \since 4.2

    Creates and returns a new window surface matching the given \a
    key.

    The server application will call this function whenever it needs
    to create a server side representation of a window, e.g. when
    copying the content of memory to the screen using the screen
    driver.

    Note that this function must be reimplemented when adding an
    accelerated graphics driver. See the
    \l{Adding an Accelerated Graphics Driver to Qt for Embedded Linux}
    {Adding an Accelerated Graphics Driver} documentation for details.

    \sa {Qt for Embedded Linux Architecture}
*/
QWSWindowSurface* QScreen::createSurface(const QString &key) const
{
#ifndef QT_NO_PAINTONSCREEN
    if (key == QLatin1String("OnScreen"))
        return new QWSOnScreenSurface;
    else
#endif
    if (key == QLatin1String("mem"))
        return new QWSLocalMemSurface;
#ifndef QT_NO_QWS_MULTIPROCESS
    else if (key == QLatin1String("shm"))
        return new QWSSharedMemSurface;
#endif
#ifndef QT_NO_PAINT_DEBUG
    else if (key == QLatin1String("Yellow"))
        return new QWSYellowSurface;
#endif
#ifndef QT_NO_DIRECTPAINTER
    else if (key == QLatin1String("DirectPainter"))
        return new QWSDirectPainterSurface;
#endif

    return 0;
}

#ifndef QT_NO_PAINTONSCREEN
bool QScreen::isWidgetPaintOnScreen(const QWidget *w)
{
    static int doOnScreen = -1;
    if (doOnScreen == -1) {
        const QByteArray env = qgetenv("QT_ONSCREEN_PAINT");
        if (env == "force")
            doOnScreen = 2;
        else
            doOnScreen = (env.toInt() > 0 ? 1 : 0);
    }

    if (doOnScreen == 2) // force
        return true;

    if (doOnScreen == 0 && !w->testAttribute(Qt::WA_PaintOnScreen))
        return false;

    return w->d_func()->isOpaque;
}
#endif

/*!
    \overload

    Creates and returns a new window surface for the given \a widget.
*/
QWSWindowSurface* QScreen::createSurface(QWidget *widget) const
{
#ifndef QT_NO_PAINTONSCREEN
    if (isWidgetPaintOnScreen(widget) && base())
        return new QWSOnScreenSurface(widget);
    else
#endif
    if (QApplication::type() == QApplication::GuiServer)
        return new QWSLocalMemSurface(widget);
#ifndef QT_NO_QWS_MULTIPROCESS
    else
        return new QWSSharedMemSurface(widget);
#endif

    return 0;
}

void QScreen::compose(int level, const QRegion &exposed, QRegion &blend,
                      QImage **blendbuffer, int changing_level)
{
    QRect exposed_bounds = exposed.boundingRect();
    QWSWindow *win = 0;
    do {
        win = qwsServer->clientWindows().value(level); // null is background
        ++level;
    } while (win && !win->paintedRegion().boundingRect().intersects(exposed_bounds));

    QWSWindowSurface *surface = (win ? win->windowSurface() : 0);
    bool above_changing = level <= changing_level; // 0 is topmost

    QRegion exposedBelow = exposed;
    bool opaque = true;

    if (win) {
        opaque = win->isOpaque() || !surface->isBuffered();
        if (opaque) {
            exposedBelow -= win->paintedRegion();
            if (above_changing || !surface->isBuffered())
                blend -= exposed & win->paintedRegion();
        } else {
            blend += exposed & win->paintedRegion();
        }
    }
    if (win && !exposedBelow.isEmpty()) {
        compose(level, exposedBelow, blend, blendbuffer, changing_level);
    } else {
        QSize blendSize = blend.boundingRect().size();
        if (!blendSize.isNull()) {
            *blendbuffer = new QImage(blendSize, d_ptr->preferredImageFormat());
        }
    }

    const QRegion blitRegion = exposed - blend;
    if (!win)
        paintBackground(blitRegion);
    else if (!above_changing && surface->isBuffered())
        blit(win, blitRegion);

    QRegion blendRegion = exposed & blend;

    if (win)
        blendRegion &= win->paintedRegion();
    if (!blendRegion.isEmpty()) {

        QPoint off = blend.boundingRect().topLeft();

        QRasterBuffer rb;
        rb.prepare(*blendbuffer);
        QSpanData spanData;
        spanData.init(&rb, 0);
        if (!win) {
            const QImage::Format format = (*blendbuffer)->format();
            switch (format) {
            case QImage::Format_ARGB32_Premultiplied:
            case QImage::Format_ARGB32:
            case QImage::Format_ARGB8565_Premultiplied:
            case QImage::Format_ARGB8555_Premultiplied:
            case QImage::Format_ARGB6666_Premultiplied:
            case QImage::Format_ARGB4444_Premultiplied:
                spanData.rasterBuffer->compositionMode = QPainter::CompositionMode_Source;
                break;
            default:
                break;
            }
            spanData.setup(qwsServer->backgroundBrush(), 256, QPainter::CompositionMode_Source);
            spanData.dx = off.x();
            spanData.dy = off.y();
        } else if (!surface->isBuffered()) {
                return;
        } else {
            const QImage &img = surface->image();
            QPoint winoff = off - win->requestedRegion().boundingRect().topLeft();
            // convert win->opacity() from scale [0..255] to [0..256]
            int const_alpha = win->opacity();
            const_alpha += (const_alpha >> 7);
            spanData.type = QSpanData::Texture;
            spanData.initTexture(&img, const_alpha);
            spanData.dx = winoff.x();
            spanData.dy = winoff.y();
        }
        if (!spanData.blend)
            return;

        if (surface)
            surface->lock();
        const QVector<QRect> rects = blendRegion.rects();
        const int nspans = 256;
        QT_FT_Span spans[nspans];
        for (int i = 0; i < rects.size(); ++i) {
            int y = rects.at(i).y() - off.y();
            int ye = y + rects.at(i).height();
            int x = rects.at(i).x() - off.x();
            int len = rects.at(i).width();
            while (y < ye) {
                int n = qMin(nspans, ye - y);
                int i = 0;
                while (i < n) {
                    spans[i].x = x;
                    spans[i].len = len;
                    spans[i].y = y + i;
                    spans[i].coverage = 255;
                    ++i;
                }
                spanData.blend(n, spans, &spanData);
                y += n;
            }
        }
        if (surface)
            surface->unlock();
    }
}

void QScreen::paintBackground(const QRegion &r)
{
    const QBrush &bg = qwsServer->backgroundBrush();
    Qt::BrushStyle bs = bg.style();
    if (bs == Qt::NoBrush || r.isEmpty())
        return;

    if (bs == Qt::SolidPattern) {
        solidFill(bg.color(), r);
    } else {
        const QRect br = r.boundingRect();
        QImage img(br.size(), d_ptr->preferredImageFormat());
        QPoint off = br.topLeft();
        QRasterBuffer rb;
        rb.prepare(&img);
        QSpanData spanData;
        spanData.init(&rb, 0);
        spanData.setup(bg, 256, QPainter::CompositionMode_Source);
        spanData.dx = off.x();
        spanData.dy = off.y();
        Q_ASSERT(spanData.blend);

        const QVector<QRect> rects = r.rects();
        const int nspans = 256;
        QT_FT_Span spans[nspans];
        for (int i = 0; i < rects.size(); ++i) {
            int y = rects.at(i).y() - off.y();
            int ye = y + rects.at(i).height();
            int x = rects.at(i).x() - off.x();
            int len = rects.at(i).width();
            while (y < ye) {
                int n = qMin(nspans, ye - y);
                int i = 0;
                while (i < n) {
                    spans[i].x = x;
                    spans[i].len = len;
                    spans[i].y = y + i;
                    spans[i].coverage = 255;
                    ++i;
                }
                spanData.blend(n, spans, &spanData);
                y += n;
            }
        }
        blit(img, br.topLeft(), r);
    }
}

/*!
    \fn virtual int QScreen::sharedRamSize(void *)

    \internal
*/

/*!
    \fn QScreen::setDirty(const QRect& rectangle)

    Marks the given \a rectangle as dirty.

    Note that the default implementation does nothing; reimplement
    this function to indicate that the given \a rectangle has been
    altered.
*/

void QScreen::setDirty(const QRect&)
{
}

/*!
    \fn QScreen::isTransformed() const

    Returns true if the screen is transformed (for instance, rotated
    90 degrees); otherwise returns false.

    \sa transformOrientation(), isInterlaced()
*/

bool QScreen::isTransformed() const
{
    return false;
}

/*!
    \fn QScreen::isInterlaced() const

    Returns true if the display is interlaced (i.e. is displaying
    images progressively like a television screen); otherwise returns
    false.

    If the display is interlaced, the drawing is altered to look
    better.

    \sa isTransformed(), linestep()
*/

bool QScreen::isInterlaced() const
{
    return false;//qws_screen_is_interlaced;;
}

/*!
    \fn QScreen::mapToDevice(const QSize &size) const

    Maps the given \a size from the coordinate space used by the
    application to the framebuffer coordinate system. Note that the
    default implementation simply returns the given \a size as it is.

    Reimplement this function to use the given device's coordinate
    system when mapping.

    \sa mapFromDevice()
*/

QSize QScreen::mapToDevice(const QSize &s) const
{
    return s;
}

/*!
    \fn QScreen::mapFromDevice(const QSize &size) const

    Maps the given \a size from the framebuffer coordinate system to
    the coordinate space used by the application. Note that the
    default implementation simply returns the given \a size as it is.

    Reimplement this function to use the given device's coordinate
    system when mapping.

    \sa mapToDevice()
*/

QSize QScreen::mapFromDevice(const QSize &s) const
{
    return s;
}

/*!
    \fn QScreen::mapToDevice(const QPoint &point, const QSize &screenSize) const
    \overload

    Maps the given \a point from the coordinate space used by the
    application to the framebuffer coordinate system, passing the
    device's \a screenSize as argument. Note that the default
    implementation returns the given \a point as it is.
*/

QPoint QScreen::mapToDevice(const QPoint &p, const QSize &) const
{
    return p;
}

/*!
    \fn QScreen::mapFromDevice(const QPoint &point, const QSize &screenSize) const
    \overload

    Maps the given \a point from the framebuffer coordinate system to
    the coordinate space used by the application, passing the device's
    \a screenSize as argument. Note that the default implementation
    simply returns the given \a point as it is.
*/

QPoint QScreen::mapFromDevice(const QPoint &p, const QSize &) const
{
    return p;
}

/*!
    \fn QScreen::mapToDevice(const QRect &rectangle, const QSize &screenSize) const
    \overload

    Maps the given \a rectangle from the coordinate space used by the
    application to the framebuffer coordinate system, passing the
    device's \a screenSize as argument. Note that the default
    implementation returns the given \a rectangle as it is.
*/

QRect QScreen::mapToDevice(const QRect &r, const QSize &) const
{
    return r;
}

/*!
    \fn QScreen::mapFromDevice(const QRect &rectangle, const QSize &screenSize) const
    \overload

    Maps the given \a rectangle from the framebuffer coordinate system to
    the coordinate space used by the application, passing the device's
    \a screenSize as argument. Note that the default implementation
    simply returns the given \a rectangle as it is.
*/

QRect QScreen::mapFromDevice(const QRect &r, const QSize &) const
{
    return r;
}

/*!
    \fn QScreen::mapToDevice(const QImage &image) const
    \overload

    Maps the given \a image from the coordinate space used by the
    application to the framebuffer coordinate system. Note that the
    default implementation returns the given \a image as it is.
*/

QImage QScreen::mapToDevice(const QImage &i) const
{
    return i;
}

/*!
    \fn QScreen::mapFromDevice(const QImage &image) const
    \overload

    Maps the given \a image from the framebuffer coordinate system to
    the coordinate space used by the application. Note that the
    default implementation simply returns the given \a image as it is.
*/

QImage QScreen::mapFromDevice(const QImage &i) const
{
    return i;
}

/*!
    \fn QScreen::mapToDevice(const QRegion &region, const QSize &screenSize) const
    \overload

    Maps the given \a region from the coordinate space used by the
    application to the framebuffer coordinate system, passing the
    device's \a screenSize as argument. Note that the default
    implementation returns the given \a region as it is.
*/

QRegion QScreen::mapToDevice(const QRegion &r, const QSize &) const
{
    return r;
}

/*!
    \fn QScreen::mapFromDevice(const QRegion &region, const QSize &screenSize) const
    \overload

    Maps the given \a region from the framebuffer coordinate system to
    the coordinate space used by the application, passing the device's
    \a screenSize as argument. Note that the default implementation
    simply returns the given \a region as it is.
*/

QRegion QScreen::mapFromDevice(const QRegion &r, const QSize &) const
{
    return r;
}

/*!
    \fn QScreen::transformOrientation() const

    Returns the current rotation as an integer value.

    Note that the default implementation returns 0; reimplement this
    function to override this value.

    \sa isTransformed()
*/

int QScreen::transformOrientation() const
{
    return 0;
}

int QScreen::pixmapDepth() const
{
    return depth();
}

/*!
    \internal
*/
int QScreen::memoryNeeded(const QString&)
{
    return 0;
}

/*!
    \internal
*/
void QScreen::haltUpdates()
{
}

/*!
    \internal
*/
void QScreen::resumeUpdates()
{
}

/*!
    \fn QRegion QScreen::region() const
    \since 4.2

    Returns the region covered by this screen driver.

    \sa base(), screenSize()
*/

/*!
    \internal
*/
void QScreen::setOffset(const QPoint &p)
{
    d_ptr->offset = p;
}

/*!
    \since 4.2

    Returns the logical offset of the screen, i.e., the offset between
    (0,0) in screen coordinates and the application coordinate system.
*/
QPoint QScreen::offset() const
{
    return d_ptr->offset;
}

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
void QScreen::setFrameBufferLittleEndian(bool littleEndian)
{
    d_ptr->fb_is_littleEndian = littleEndian;
}

bool QScreen::frameBufferLittleEndian() const
{
    return d_ptr->fb_is_littleEndian;
}
#endif

/*!
    \fn int QScreen::subScreenIndexAt(const QPoint &position) const
    \since 4.2

    Returns the index of the subscreen at the given \a position;
    returns -1 if no screen is found.

    The index identifies the subscreen in the list of pointers
    returned by the subScreens() function.

    \sa instance(), subScreens()
*/
int QScreen::subScreenIndexAt(const QPoint &p) const
{
    const QList<QScreen*> screens = subScreens();
    const int n = screens.count();
    for (int i = 0; i < n; ++i) {
        if (screens.at(i)->region().contains(p))
            return i;
    }

    return -1;
}

#if 0
#ifdef QT_LOADABLE_MODULES
#include <dlfcn.h>

// ### needs update after driver init changes

static QScreen * qt_dodriver(char * driver,char * a,unsigned char * b)

{
    char buf[200];
    strcpy(buf,"/etc/qws/drivers/");
    qstrcpy(buf+17,driver);
    qDebug("Attempting driver %s",driver);

    void * handle;
    handle=dlopen(buf,RTLD_LAZY);
    if(handle==0) {
        qFatal("Module load error");
    }
    QScreen *(*qt_get_screen_func)(char *,unsigned char *);
    qt_get_screen_func=dlsym(handle,"qt_get_screen");
    if(qt_get_screen_func==0) {
        qFatal("Couldn't get symbol");
    }
    QScreen * ret=qt_get_screen_func(a,b);
    return ret;
}

static QScreen * qt_do_entry(char * entry)
{
    unsigned char config[256];

    FILE * f=fopen(entry,"r");
    if(!f) {
        return 0;
    }

    int r=fread(config,256,1,f);
    if(r<1)
        return 0;

    fclose(f);

    unsigned short vendorid=*((unsigned short int *)config);
    unsigned short deviceid=*(((unsigned short int *)config)+1);
    if(config[0xb]!=3)
        return 0;

    if(vendorid==0x1002) {
        if(deviceid==0x4c4d) {
            qDebug("Compaq Armada/IBM Thinkpad's Mach64 card");
            return qt_dodriver("mach64.so",entry,config);
        } else if(deviceid==0x4742) {
            qDebug("Desktop Rage Pro Mach64 card");
            return qt_dodriver("mach64.so",entry,config);
        } else {
            qDebug("Unrecognised ATI card id %x",deviceid);
            return 0;
        }
    } else {
        qDebug("Unrecognised vendor");
    }
    return 0;
}

extern bool qws_accel;

/// ** NOT SUPPPORTED **

QScreen * qt_probe_bus()
{
    if(!qws_accel) {
        return qt_dodriver("unaccel.so",0,0);
    }

    QT_DIR *dirptr = QT_OPENDIR("/proc/bus/pci");
    if(!dirptr)
        return qt_dodriver("unaccel.so",0,0);
    QT_DIR * dirptr2;
    QT_DIRENT *cards;

    QT_DIRENT *busses = QT_READDIR(dirptr);

    while(busses) {
        if(busses->d_name[0]!='.') {
            char buf[100];
            strcpy(buf,"/proc/bus/pci/");
            qstrcpy(buf+14,busses->d_name);
            int p=strlen(buf);
            dirptr2 = QT_OPENDIR(buf);
            if(dirptr2) {
                cards = QT_READDIR(dirptr2);
                while(cards) {
                    if(cards->d_name[0]!='.') {
                        buf[p]='/';
                        qstrcpy(buf+p+1,cards->d_name);
                        QScreen * ret=qt_do_entry(buf);
                        if(ret)
                            return ret;
                    }
                    cards = QT_READDIR(dirptr2);
                }
                QT_CLOSEDIR(dirptr2);
            }
        }
        busses = QT_READDIR(dirptr);
    }
    QT_CLOSEDIR(dirptr);

    return qt_dodriver("unaccel.so",0,0);
}

#else

char *qt_qws_hardcoded_slot = "/proc/bus/pci/01/00.0";

const unsigned char* qt_probe_bus()
{
    const char * slot;
    slot=::getenv("QWS_CARD_SLOT");
    if(!slot)
        slot=qt_qws_hardcoded_slot;
    if (slot) {
        static unsigned char config[256];
        FILE * f=fopen(slot,"r");
        if(!f) {
            qDebug("Open failure for %s",slot);
            slot=0;
        } else {
            int r=fread((char*)config,256,1,f);
            fclose(f);
            if(r<1) {
                qDebug("Read failure");
                return 0;
            } else {
                return config;
            }
        }
    }
    return 0;
}

#endif

#endif // 0

/*!
    \internal
    \since 4.4
*/
void QScreen::setPixmapDataFactory(QPixmapDataFactory *factory)
{
    static bool shownWarning = false;
    if (!shownWarning) {
        qWarning("QScreen::setPixmapDataFactory() is deprecated - use setGraphicsSystem() instead");
        shownWarning = true;
    }

    d_ptr->pixmapFactory = factory;
}

/*!
    \internal
    \since 4.4
*/
QPixmapDataFactory* QScreen::pixmapDataFactory() const
{
    return d_ptr->pixmapFactory;
}

/*!
    \internal
    \since 4.5
*/
void QScreen::setGraphicsSystem(QGraphicsSystem* system)
{
    d_ptr->graphicsSystem = system;
}

/*!
    \internal
    \since 4.5
*/
QGraphicsSystem* QScreen::graphicsSystem() const
{
    return d_ptr->graphicsSystem;
}

/*!
    \since 4.4

    Returns the class identifier for the screen object.
*/
QScreen::ClassId QScreen::classId() const
{
    return static_cast<ClassId>(d_ptr->classId);
}

QT_END_NAMESPACE
