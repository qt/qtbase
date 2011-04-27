/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenVG module of the Qt Toolkit.
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

#include "qpixmapdata_vg_p.h"
#include "qpaintengine_vg_p.h"
#include <QtGui/private/qdrawhelper_p.h>
#if !defined(QT_NO_EGL)
#include <QtGui/private/qegl_p.h>
#endif
#include "qvg_p.h"
#include "qvgimagepool_p.h"
#include <QBuffer>
#include <QImageReader>
#include <QtGui/private/qimage_p.h>
#include <QtGui/private/qnativeimagehandleprovider_p.h>
#include <QtGui/private/qfont_p.h>

QT_BEGIN_NAMESPACE

static int qt_vg_pixmap_serial = 0;

QVGPixmapData::QVGPixmapData(PixelType type)
    : QPixmapData(type, OpenVGClass)
{
    Q_ASSERT(type == QPixmapData::PixmapType);
    vgImage = VG_INVALID_HANDLE;
    vgImageOpacity = VG_INVALID_HANDLE;
    cachedOpacity = 1.0f;
    recreate = true;
    inImagePool = false;
    inLRU = false;
    failedToAlloc = false;
#if defined(Q_OS_SYMBIAN)
    nativeImageHandleProvider = 0;
    nativeImageHandle = 0;
#endif
#if !defined(QT_NO_EGL)
    context = 0;
    qt_vg_register_pixmap(this);
#endif
    updateSerial();
}

QVGPixmapData::~QVGPixmapData()
{
    destroyImageAndContext();
#if !defined(QT_NO_EGL)
    qt_vg_unregister_pixmap(this);
#endif
}

void QVGPixmapData::destroyImages()
{
    if (inImagePool) {
        QVGImagePool *pool = QVGImagePool::instance();
        if (vgImage != VG_INVALID_HANDLE)
            pool->releaseImage(this, vgImage);
        if (vgImageOpacity != VG_INVALID_HANDLE)
            pool->releaseImage(this, vgImageOpacity);
    } else {
        if (vgImage != VG_INVALID_HANDLE)
            vgDestroyImage(vgImage);
        if (vgImageOpacity != VG_INVALID_HANDLE)
            vgDestroyImage(vgImageOpacity);
    }
    vgImage = VG_INVALID_HANDLE;
    vgImageOpacity = VG_INVALID_HANDLE;
    inImagePool = false;

#if defined(Q_OS_SYMBIAN)
    releaseNativeImageHandle();
#endif
}

void QVGPixmapData::destroyImageAndContext()
{
    if (vgImage != VG_INVALID_HANDLE) {
        // We need to have a context current to destroy the image.
#if !defined(QT_NO_EGL)
        if (!context)
            context = qt_vg_create_context(0, QInternal::Pixmap);
        if (context->isCurrent()) {
            destroyImages();
        } else {
            // We don't currently have a widget surface active, but we
            // need a surface to make the context current.  So use the
            // shared pbuffer surface instead.
            context->makeCurrent(qt_vg_shared_surface());
            destroyImages();
            context->lazyDoneCurrent();
        }
#else
        destroyImages();
#endif
    } else {
#if defined(Q_OS_SYMBIAN)
        releaseNativeImageHandle();
#endif
    }
#if !defined(QT_NO_EGL)
    if (context) {
        qt_vg_destroy_context(context, QInternal::Pixmap);
        context = 0;
    }
#endif
    recreate = true;
}

QPixmapData *QVGPixmapData::createCompatiblePixmapData() const
{
    return new QVGPixmapData(pixelType());
}

bool QVGPixmapData::isValid() const
{
    return (w > 0 && h > 0);
}

void QVGPixmapData::updateSerial()
{
    setSerialNumber(++qt_vg_pixmap_serial);
}

void QVGPixmapData::resize(int wid, int ht)
{
    if (w == wid && h == ht) {
        updateSerial();
        return;
    }

    w = wid;
    h = ht;
    d = 32; // We always use ARGB_Premultiplied for VG pixmaps.
    is_null = (w <= 0 || h <= 0);
    source = QVolatileImage();
    recreate = true;

    updateSerial();
}

void QVGPixmapData::fromImage(const QImage &image, Qt::ImageConversionFlags flags)
{
    if (image.isNull())
        return;

    QImage img = image;
    createPixmapForImage(img, flags, false);
}

void QVGPixmapData::fromImageReader(QImageReader *imageReader,
                                 Qt::ImageConversionFlags flags)
{
    QImage image = imageReader->read();
    if (image.isNull())
        return;

    createPixmapForImage(image, flags, true);
}

bool QVGPixmapData::fromFile(const QString &filename, const char *format,
                          Qt::ImageConversionFlags flags)
{
    QImage image = QImageReader(filename, format).read();
    if (image.isNull())
        return false;

    createPixmapForImage(image, flags, true);

    return !isNull();
}

bool QVGPixmapData::fromData(const uchar *buffer, uint len, const char *format,
                      Qt::ImageConversionFlags flags)
{
    QByteArray a = QByteArray::fromRawData(reinterpret_cast<const char *>(buffer), len);
    QBuffer b(&a);
    b.open(QIODevice::ReadOnly);
    QImage image = QImageReader(&b, format).read();
    if (image.isNull())
        return false;

    createPixmapForImage(image, flags, true);

    return !isNull();
}

QImage::Format QVGPixmapData::idealFormat(QImage *image, Qt::ImageConversionFlags flags) const
{
    QImage::Format format = sourceFormat();
    int d = image->depth();
    if (d == 1 || d == 16 || d == 24 || (d == 32 && !image->hasAlphaChannel()))
        format = QImage::Format_RGB32;
    else if (!(flags & Qt::NoOpaqueDetection) && image->data_ptr()->checkForAlphaPixels())
        format = sourceFormat();
    else
        format = image->hasAlphaChannel() ? sourceFormat() : QImage::Format_RGB32;
    return format;
}

void QVGPixmapData::createPixmapForImage(QImage &image, Qt::ImageConversionFlags flags, bool inPlace)
{
    resize(image.width(), image.height());

    QImage::Format format = idealFormat(&image, flags);

    if (inPlace && image.data_ptr()->convertInPlace(format, flags)) {
        source = QVolatileImage(image);
    } else {
        QImage convertedImage = image.convertToFormat(format);
        // convertToFormat won't detach the image if format stays the
        // same. Detaching is needed to prevent issues with painting
        // onto this QPixmap later on.
        convertedImage.detach();
        source = QVolatileImage(convertedImage);
    }

    recreate = true;
}

void QVGPixmapData::fill(const QColor &color)
{
    if (!isValid())
        return;
    forceToImage();
    if (source.depth() == 1) {
        // Pick the best approximate color in the image's colortable.
        int gray = qGray(color.rgba());
        if (qAbs(qGray(source.imageRef().color(0)) - gray)
            < qAbs(qGray(source.imageRef().color(1)) - gray))
            source.fill(0);
        else
            source.fill(1);
    } else {
        source.fill(PREMUL(color.rgba()));
    }
}

bool QVGPixmapData::hasAlphaChannel() const
{
    ensureReadback(true);
    if (!source.isNull())
        return source.hasAlphaChannel();
    else
        return isValid();
}

void QVGPixmapData::setAlphaChannel(const QPixmap &alphaChannel)
{
    if (!isValid())
        return;
    forceToImage();
    source.setAlphaChannel(alphaChannel);
}

QImage QVGPixmapData::toImage() const
{
    if (!isValid())
        return QImage();
    ensureReadback(true);
    if (source.isNull()) {
        source = QVolatileImage(w, h, sourceFormat());
        recreate = true;
    }
    return source.toImage();
}

void QVGPixmapData::copy(const QPixmapData *data, const QRect &rect)
{
    // toImage() is potentially expensive with QVolatileImage so provide a
    // more efficient implementation of copy() that does not rely on it.
    if (!data) {
        return;
    }
    if (data->classId() != OpenVGClass) {
        fromImage(data->toImage(rect), Qt::NoOpaqueDetection);
        return;
    }
    const QVGPixmapData *pd = static_cast<const QVGPixmapData *>(data);
    QRect r = rect;
    if (r.isNull() || r.contains(QRect(0, 0, pd->w, pd->h))) {
        r = QRect(0, 0, pd->w, pd->h);
    }
    resize(r.width(), r.height());
    recreate = true;
    if (!pd->source.isNull()) {
        source = QVolatileImage(r.width(), r.height(), pd->source.format());
        source.copyFrom(&pd->source, r);
    }
}

QImage *QVGPixmapData::buffer()
{
    // Cannot be safely implemented and QVGPixmapData is not (must not be) RasterClass anyway.
    return 0;
}

QPaintEngine* QVGPixmapData::paintEngine() const
{
    // If the application wants to paint into the QPixmap, we first
    // force it to QImage format and then paint into that.
    // This is simpler than juggling multiple VG contexts.
    const_cast<QVGPixmapData *>(this)->forceToImage();
    return source.paintEngine();
}

VGImage QVGPixmapData::toVGImage()
{
    if (!isValid() || failedToAlloc)
        return VG_INVALID_HANDLE;

#if !defined(QT_NO_EGL)
    // Increase the reference count on the shared context.
    if (!context)
        context = qt_vg_create_context(0, QInternal::Pixmap);
#endif

    if (recreate && prevSize != QSize(w, h))
        destroyImages();
    else if (recreate)
        cachedOpacity = -1.0f;  // Force opacity image to be refreshed later.

#if defined(Q_OS_SYMBIAN)
    if (recreate && nativeImageHandleProvider && !nativeImageHandle) {
        createFromNativeImageHandleProvider();
    }
#endif

    if (vgImage == VG_INVALID_HANDLE) {
        vgImage = QVGImagePool::instance()->createImageForPixmap
            (qt_vg_image_to_vg_format(source.format()), w, h, VG_IMAGE_QUALITY_FASTER, this);

        // Bail out if we run out of GPU memory - try again next time.
        if (vgImage == VG_INVALID_HANDLE) {
            failedToAlloc = true;
            return VG_INVALID_HANDLE;
        }

        inImagePool = true;
    } else if (inImagePool) {
        QVGImagePool::instance()->useImage(this);
    }

    if (!source.isNull() && recreate) {
        source.beginDataAccess();
        vgImageSubData
            (vgImage,
             source.constBits(), source.bytesPerLine(),
             qt_vg_image_to_vg_format(source.format()), 0, 0, w, h);
        source.endDataAccess(true);
    }

    recreate = false;
    prevSize = QSize(w, h);

    return vgImage;
}

VGImage QVGPixmapData::toVGImage(qreal opacity)
{
#if !defined(QT_SHIVAVG)
    // Force the primary VG image to be recreated if necessary.
    if (toVGImage() == VG_INVALID_HANDLE)
        return VG_INVALID_HANDLE;

    if (opacity == 1.0f)
        return vgImage;

    // Create an alternative image for the selected opacity.
    if (vgImageOpacity == VG_INVALID_HANDLE || cachedOpacity != opacity) {
        if (vgImageOpacity == VG_INVALID_HANDLE) {
            if (inImagePool) {
                vgImageOpacity = QVGImagePool::instance()->createImageForPixmap
                    (VG_sARGB_8888_PRE, w, h, VG_IMAGE_QUALITY_FASTER, this);
            } else {
                vgImageOpacity = vgCreateImage
                    (VG_sARGB_8888_PRE, w, h, VG_IMAGE_QUALITY_FASTER);
            }

            // Bail out if we run out of GPU memory - try again next time.
            if (vgImageOpacity == VG_INVALID_HANDLE)
                return VG_INVALID_HANDLE;
        }
        VGfloat matrix[20] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, opacity,
            0.0f, 0.0f, 0.0f, 0.0f
        };
        vgColorMatrix(vgImageOpacity, vgImage, matrix);
        cachedOpacity = opacity;
    }

    return vgImageOpacity;
#else
    // vgColorMatrix() doesn't work with ShivaVG, so ignore the opacity.
    Q_UNUSED(opacity);
    return toVGImage();
#endif
}

void QVGPixmapData::detachImageFromPool()
{
    if (inImagePool) {
        QVGImagePool::instance()->detachImage(this);
        inImagePool = false;
    }
}

void QVGPixmapData::hibernate()
{
    // If the image was imported (e.g, from an SgImage under Symbian), then
    // skip the hibernation, there is no sense in copying it back to main
    // memory because the data is most likely shared between several processes.
    bool skipHibernate = (vgImage != VG_INVALID_HANDLE && source.isNull());
#if defined(Q_OS_SYMBIAN)
    // However we have to proceed normally if the image was retrieved via
    // a handle provider.
    skipHibernate &= !nativeImageHandleProvider;
#endif
    if (skipHibernate)
        return;

    forceToImage(false); // no readback allowed here
    destroyImageAndContext();
}

void QVGPixmapData::reclaimImages()
{
    if (!inImagePool)
        return;
    forceToImage();
    destroyImages();
}

int QVGPixmapData::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    switch (metric) {
    case QPaintDevice::PdmWidth:
        return w;
    case QPaintDevice::PdmHeight:
        return h;
    case QPaintDevice::PdmNumColors:
        return 0;
    case QPaintDevice::PdmDepth:
        return d;
    case QPaintDevice::PdmWidthMM:
        return qRound(w * 25.4 / qt_defaultDpiX());
    case QPaintDevice::PdmHeightMM:
        return qRound(h * 25.4 / qt_defaultDpiY());
    case QPaintDevice::PdmDpiX:
    case QPaintDevice::PdmPhysicalDpiX:
        return qt_defaultDpiX();
    case QPaintDevice::PdmDpiY:
    case QPaintDevice::PdmPhysicalDpiY:
        return qt_defaultDpiY();
    default:
        qWarning("QVGPixmapData::metric(): Invalid metric");
        return 0;
    }
}

// Ensures that the pixmap is backed by some valid data and forces the data to
// be re-uploaded to the VGImage when toVGImage() is called next time.
void QVGPixmapData::forceToImage(bool allowReadback)
{
    if (!isValid())
        return;

    if (allowReadback)
        ensureReadback(false);

    if (source.isNull())
        source = QVolatileImage(w, h, sourceFormat());

    recreate = true;
}

void QVGPixmapData::ensureReadback(bool readOnly) const
{
    if (vgImage != VG_INVALID_HANDLE && source.isNull()) {
        source = QVolatileImage(w, h, sourceFormat());
        source.beginDataAccess();
        vgGetImageSubData(vgImage, source.bits(), source.bytesPerLine(),
                          qt_vg_image_to_vg_format(source.format()),
                          0, 0, w, h);
        source.endDataAccess();
        if (readOnly) {
            recreate = false;
        } else {
            // Once we did a readback, the original VGImage must be destroyed
            // because it may be shared (e.g. created via SgImage) and a subsequent
            // upload of the image data may produce unexpected results.
            const_cast<QVGPixmapData *>(this)->destroyImages();
#if defined(Q_OS_SYMBIAN)
            // There is now an own copy of the data so drop the handle provider,
            // otherwise toVGImage() would request the handle again, which is wrong.
            nativeImageHandleProvider = 0;
#endif
            recreate = true;
        }
    }
}

QImage::Format QVGPixmapData::sourceFormat() const
{
    return QImage::Format_ARGB32_Premultiplied;
}

/*
    \internal

    Returns the VGImage that is storing the contents of \a pixmap.
    Returns VG_INVALID_HANDLE if \a pixmap is not owned by the OpenVG
    graphics system or \a pixmap is invalid.

    This function is typically used to access the backing store
    for a pixmap when executing raw OpenVG calls.  It must only
    be used when a QPainter is active and the OpenVG paint engine
    is in use by the QPainter.

    \sa {QtOpenVG Module}
*/
Q_OPENVG_EXPORT VGImage qPixmapToVGImage(const QPixmap& pixmap)
{
    QPixmapData *pd = pixmap.pixmapData();
    if (!pd)
        return VG_INVALID_HANDLE; // null QPixmap
    if (pd->classId() == QPixmapData::OpenVGClass) {
        QVGPixmapData *vgpd = static_cast<QVGPixmapData *>(pd);
        if (vgpd->isValid())
            return vgpd->toVGImage();
    }
    return VG_INVALID_HANDLE;
}

QT_END_NAMESPACE
