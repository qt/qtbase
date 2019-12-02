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

#include "qbitmap.h"
#include <qpa/qplatformpixmap.h>
#include <qpa/qplatformintegration.h>
#include "qimage.h"
#include "qscreen.h"
#include "qvariant.h"
#include <qpainter.h>
#include <private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QBitmap
    \inmodule QtGui
    \brief The QBitmap class provides monochrome (1-bit depth) pixmaps.

    \ingroup painting
    \ingroup shared

    The QBitmap class is a monochrome off-screen paint device used
    mainly for creating custom QCursor and QBrush objects,
    constructing QRegion objects, and for setting masks for pixmaps
    and widgets.

    QBitmap is a QPixmap subclass ensuring a depth of 1, except for
    null objects which have a depth of 0. If a pixmap with a depth
    greater than 1 is assigned to a bitmap, the bitmap will be
    dithered automatically.

    Use the QColor objects Qt::color0 and Qt::color1 when drawing on a
    QBitmap object (or a QPixmap object with depth 1).

    Painting with Qt::color0 sets the bitmap bits to 0, and painting
    with Qt::color1 sets the bits to 1. For a bitmap, 0-bits indicate
    background (or transparent pixels) and 1-bits indicate foreground
    (or opaque pixels). Use the clear() function to set all the bits
    to Qt::color0. Note that using the Qt::black and Qt::white colors
    make no sense because the QColor::pixel() value is not necessarily
    0 for black and 1 for white.

    The QBitmap class provides the transformed() function returning a
    transformed copy of the bitmap; use the QTransform argument to
    translate, scale, shear, and rotate the bitmap. In addition,
    QBitmap provides the static fromData() function which returns a
    bitmap constructed from the given \c uchar data, and the static
    fromImage() function returning a converted copy of a QImage
    object.

    Just like the QPixmap class, QBitmap is optimized by the use of
    implicit data sharing. For more information, see the \l {Implicit
    Data Sharing} documentation.

    \sa QPixmap, QImage, QImageReader, QImageWriter
*/

/*! \typedef QBitmap::DataPtr
  \internal
 */

/*!
    Constructs a null bitmap.

    \sa QPixmap::isNull()
*/
QBitmap::QBitmap()
    : QPixmap(QSize(0, 0), QPlatformPixmap::BitmapType)
{
}

/*!
    \fn QBitmap::QBitmap(int width, int height)

    Constructs a bitmap with the given \a width and \a height. The pixels
    inside are uninitialized.

    \sa clear()
*/

QBitmap::QBitmap(int w, int h)
    : QPixmap(QSize(w, h), QPlatformPixmap::BitmapType)
{
}

/*!
    Constructs a bitmap with the given \a size.  The pixels in the
    bitmap are uninitialized.

    \sa clear()
*/

QBitmap::QBitmap(const QSize &size)
    : QPixmap(size, QPlatformPixmap::BitmapType)
{
}

/*!
    \fn QBitmap::clear()

    Clears the bitmap, setting all its bits to Qt::color0.
*/

/*!
    Constructs a bitmap that is a copy of the given \a pixmap.

    If the pixmap has a depth greater than 1, the resulting bitmap
    will be dithered automatically.

    \sa QPixmap::depth(), fromImage(), fromData()
*/

QBitmap::QBitmap(const QPixmap &pixmap)
{
    QBitmap::operator=(pixmap);
}

/*!
    Constructs a bitmap from the file specified by the given \a
    fileName. If the file does not exist, or has an unknown format,
    the bitmap becomes a null bitmap.

    The \a fileName and \a format parameters are passed on to the
    QPixmap::load() function. If the file format uses more than 1 bit
    per pixel, the resulting bitmap will be dithered automatically.

    \sa QPixmap::isNull(), QImageReader::imageFormat()
*/

QBitmap::QBitmap(const QString& fileName, const char *format)
    : QPixmap(QSize(0, 0), QPlatformPixmap::BitmapType)
{
    load(fileName, format, Qt::MonoOnly);
}

/*!
    \overload

    Assigns the given \a pixmap to this bitmap and returns a reference
    to this bitmap.

    If the pixmap has a depth greater than 1, the resulting bitmap
    will be dithered automatically.

    \sa QPixmap::depth()
 */

QBitmap &QBitmap::operator=(const QPixmap &pixmap)
{
    if (pixmap.isNull()) {                        // a null pixmap
        QBitmap(0, 0).swap(*this);
    } else if (pixmap.depth() == 1) {                // 1-bit pixmap
        QPixmap::operator=(pixmap);                // shallow assignment
    } else {                                        // n-bit depth pixmap
        *this = fromImage(pixmap.toImage());        // will dither image
    }
    return *this;
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)

/*!
  Destroys the bitmap.
*/
QBitmap::~QBitmap()
{
}

#endif

/*!
    \fn void QBitmap::swap(QBitmap &other)
    \since 4.8

    Swaps bitmap \a other with this bitmap. This operation is very
    fast and never fails.
*/

/*!
   Returns the bitmap as a QVariant.
*/
QBitmap::operator QVariant() const
{
    return QVariant(QMetaType::QBitmap, this);
}

static QBitmap makeBitmap(QImage &&image, Qt::ImageConversionFlags flags)
{
    // make sure image.color(0) == Qt::color0 (white)
    // and image.color(1) == Qt::color1 (black)
    const QRgb c0 = QColor(Qt::black).rgb();
    const QRgb c1 = QColor(Qt::white).rgb();
    if (image.color(0) == c0 && image.color(1) == c1) {
        image.invertPixels();
        image.setColor(0, c1);
        image.setColor(1, c0);
    }

    QScopedPointer<QPlatformPixmap> data(QGuiApplicationPrivate::platformIntegration()->createPlatformPixmap(QPlatformPixmap::BitmapType));

    data->fromImageInPlace(image, flags | Qt::MonoOnly);
    return QPixmap(data.take());
}

/*!
    Returns a copy of the given \a image converted to a bitmap using
    the specified image conversion \a flags.

    \sa fromData()
*/
QBitmap QBitmap::fromImage(const QImage &image, Qt::ImageConversionFlags flags)
{
    if (image.isNull())
        return QBitmap();

    return makeBitmap(image.convertToFormat(QImage::Format_MonoLSB, flags), flags);
}

/*!
    \since 5.12
    \overload

    Returns a copy of the given \a image converted to a bitmap using
    the specified image conversion \a flags.

    \sa fromData()
*/
QBitmap QBitmap::fromImage(QImage &&image, Qt::ImageConversionFlags flags)
{
    if (image.isNull())
        return QBitmap();

    return makeBitmap(std::move(image).convertToFormat(QImage::Format_MonoLSB, flags), flags);
}

/*!
    Constructs a bitmap with the given \a size, and sets the contents to
    the \a bits supplied.

    The bitmap data has to be byte aligned and provided in in the bit
    order specified by \a monoFormat. The mono format must be either
    QImage::Format_Mono or QImage::Format_MonoLSB. Use
    QImage::Format_Mono to specify data on the XBM format.

    \sa fromImage()

*/
QBitmap QBitmap::fromData(const QSize &size, const uchar *bits, QImage::Format monoFormat)
{
    Q_ASSERT(monoFormat == QImage::Format_Mono || monoFormat == QImage::Format_MonoLSB);

    QImage image(size, monoFormat);
    image.setColor(0, QColor(Qt::color0).rgb());
    image.setColor(1, QColor(Qt::color1).rgb());

    // Need to memcpy each line separatly since QImage is 32bit aligned and
    // this data is only byte aligned...
    int bytesPerLine = (size.width() + 7) / 8;
    for (int y = 0; y < size.height(); ++y)
        memcpy(image.scanLine(y), bits + bytesPerLine * y, bytesPerLine);
    return QBitmap::fromImage(std::move(image));
}

/*!
    Returns a copy of this bitmap, transformed according to the given
    \a matrix.

    \sa QPixmap::transformed()
 */
QBitmap QBitmap::transformed(const QTransform &matrix) const
{
    QBitmap bm = QPixmap::transformed(matrix);
    return bm;
}

#if QT_DEPRECATED_SINCE(5, 13)
/*!
  \overload
  \obsolete

  This convenience function converts the \a matrix to a QTransform
  and calls the overloaded function.
*/
QBitmap QBitmap::transformed(const QMatrix &matrix) const
{
    return transformed(QTransform(matrix));
}
#endif

QT_END_NAMESPACE
