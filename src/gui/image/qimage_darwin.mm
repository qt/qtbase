// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qimage.h"

#include <private/qcore_mac_p.h>
#include <private/qcoregraphics_p.h>

#include <QtGui/qcolorspace.h>

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>

QT_BEGIN_NAMESPACE

/*!
    \fn CGImageRef QImage::toCGImage() const

    Creates a \c CGImage equivalent to this QImage, and returns a \c CGImageRef
    handle.

    The returned CGImageRef partakes in the QImage implicit sharing,
    and holds a reference to the QImage data. CGImage is immutable
    and will never detach the QImage. Writing to the QImage will detach
    as usual.

    This function is fast, and does not copy or convert image data.

    The following image formats are supported, and will be mapped to
    a corresponding native image type:

    \table
    \header
        \li Qt
        \li CoreGraphics
    \row
        \li Format_ARGB32
        \li kCGImageAlphaFirst | kCGBitmapByteOrder32Host
    \row
        \li Format_RGB32
        \li kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host
    \row
        \li Format_RGBA8888_Premultiplied
        \li kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big
    \row
        \li Format_RGBA8888
        \li kCGImageAlphaLast | kCGBitmapByteOrder32Big
    \row
        \li Format_RGBX8888
        \li kCGImageAlphaNoneSkipLast | kCGBitmapByteOrder32Big
    \row
        \li Format_ARGB32_Premultiplied
        \li kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host
    \endtable

    Other formats are not supported; this function returns a null
    CGImageRef for those cases. Users of this function may then
    convert the QImage to a supported format first, for example
    Format_ARGB32_Premultiplied.

    If the image does not have a color space set the resulting
    CGImageRef color space is set to the sRGB color space.

    \ingroup platform-type-conversions
*/
CGImageRef QImage::toCGImage() const
{
    if (isNull())
        return nil;

    CGBitmapInfo bitmapInfo = qt_mac_bitmapInfoForImage(*this);

    // Format not supported: return nil CGImageRef
    if (bitmapInfo == kCGImageAlphaNone)
        return nil;

    // Create a data provider that owns a copy of the QImage and references the image data.
    auto deleter = [](void *image, const void *, size_t)
                   { delete static_cast<QImage *>(image); };
    QCFType<CGDataProviderRef> dataProvider =
        CGDataProviderCreateWithData(new QImage(*this), bits(), sizeInBytes(), deleter);

    QCFType<CGColorSpaceRef> cgColorSpace = [&]{
        if (colorSpace().isValid()) {
            QCFType<CFDataRef> iccData = colorSpace().iccProfile().toCFData();
            return CGColorSpaceCreateWithICCData(iccData);
        } else {
            return CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        }
    }();

    const size_t bitsPerComponent = 8;
    const size_t bitsPerPixel = 32;
    const CGFloat *decode = nullptr;
    const bool shouldInterpolate = false;

    return CGImageCreate(width(), height(), bitsPerComponent, bitsPerPixel,
                         this->bytesPerLine(), cgColorSpace, bitmapInfo, dataProvider,
                         decode, shouldInterpolate, kCGRenderingIntentDefault);
}

QT_END_NAMESPACE
