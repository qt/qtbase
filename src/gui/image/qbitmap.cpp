// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qbitmap.h"
#include <qpa/qplatformpixmap.h>
#include <qpa/qplatformintegration.h>
#include "qimage.h"
#include "qscreen.h"
#include "qvariant.h"
#include <qpainter.h>
#include <private/qguiapplication_p.h>

#include <memory>

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
    \deprecated [6.0] Use fromPixmap instead.

    Constructs a bitmap with the given \a size.  The pixels in the
    bitmap are uninitialized.

    \sa clear()
*/
QBitmap::QBitmap(const QSize &size)
    : QPixmap(size, QPlatformPixmap::BitmapType)
{
}

/*!
    \internal
    This dtor must stay empty until Qt 7 (was inline until 6.2).
*/
QBitmap::~QBitmap() = default;

/*!
    \fn QBitmap::clear()

    Clears the bitmap, setting all its bits to Qt::color0.
*/

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
    return QVariant::fromValue(*this);
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

    std::unique_ptr<QPlatformPixmap> data(QGuiApplicationPrivate::platformIntegration()->createPlatformPixmap(QPlatformPixmap::BitmapType));

    data->fromImageInPlace(image, flags | Qt::MonoOnly);
    return QBitmap::fromPixmap(QPixmap(data.release()));
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

    The bitmap data has to be byte aligned and provided in the bit
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

    // Need to memcpy each line separately since QImage is 32bit aligned and
    // this data is only byte aligned...
    int bytesPerLine = (size.width() + 7) / 8;
    for (int y = 0; y < size.height(); ++y)
        memcpy(image.scanLine(y), bits + bytesPerLine * y, bytesPerLine);
    return QBitmap::fromImage(std::move(image));
}

/*!
    Returns a copy of the given \a pixmap converted to a bitmap.

    If the pixmap has a depth greater than 1, the resulting bitmap
    will be dithered automatically.

    \since 6.0

    \sa QPixmap::depth()
*/

QBitmap QBitmap::fromPixmap(const QPixmap &pixmap)
{
    if (pixmap.isNull()) {                        // a null pixmap
        return QBitmap(0, 0);
    } else if (pixmap.depth() == 1) {             // 1-bit pixmap
        QBitmap bm;
        if (pixmap.paintingActive()) {            // make a deep copy
            pixmap.copy().swap(bm);
        } else {
            bm.data = pixmap.data;                // shallow assignment
        }
        return bm;
    }
    // n-bit depth pixmap, will dither image
    return fromImage(pixmap.toImage());
}

#if QT_DEPRECATED_SINCE(6, 0)
/*!
    \deprecated [6.0] Use fromPixmap instead.
    Constructs a bitmap that is a copy of the given \a pixmap.

    If the pixmap has a depth greater than 1, the resulting bitmap
    will be dithered automatically.

    \sa QPixmap::depth(), fromImage(), fromData()
*/
QBitmap::QBitmap(const QPixmap &pixmap)
{
    *this = QBitmap::fromPixmap(pixmap);
}

/*!
    \deprecated [6.0] Use fromPixmap instead.
    \overload

    Assigns the given \a pixmap to this bitmap and returns a reference
    to this bitmap.

    If the pixmap has a depth greater than 1, the resulting bitmap
    will be dithered automatically.

    \sa QPixmap::depth()
 */
QBitmap &QBitmap::operator=(const QPixmap &pixmap)
{
    *this = QBitmap::fromPixmap(pixmap);
    return *this;
}
#endif

/*!
    Returns a copy of this bitmap, transformed according to the given
    \a matrix.

    \sa QPixmap::transformed()
 */
QBitmap QBitmap::transformed(const QTransform &matrix) const
{
    return QBitmap::fromPixmap(QPixmap::transformed(matrix));
}

QT_END_NAMESPACE
