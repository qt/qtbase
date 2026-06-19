// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qimage.h"

#include <private/qcore_mac_p.h>
#include <private/qcoregraphics_p.h>

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

    If the image format can not be converted a null CGImageRef will
    be returned. Users of this function may then convert the QImage
    to a supported format first, for example Format_ARGB32_Premultiplied.

    If the image does not have a color space set the resulting
    CGImageRef color space is set to the sRGB color space.

    \ingroup platform-type-conversions
*/
CGImageRef QImage::toCGImage() const
{
    if (isNull())
        return nil;

    auto cgImageFormat = qt_mac_cgImageFormatForImage(*this);
    if (!cgImageFormat)
        return nil;

    // Create a data provider that owns a copy of the QImage and references the image data.
    auto deleter = [](void *image, const void *, size_t)
                   { delete static_cast<QImage *>(image); };
    QCFType<CGDataProviderRef> dataProvider =
        CGDataProviderCreateWithData(new QImage(*this), bits(), sizeInBytes(), deleter);

    static constexpr bool shouldInterpolate = false;
    static constexpr CGFloat *decodeArray = nullptr;

    return CGImageCreate(width(), height(),
        cgImageFormat->bitsPerComponent, cgImageFormat->bitsPerPixel,
        this->bytesPerLine(), cgImageFormat->colorSpace,
        cgImageFormat->bitmapInfo, dataProvider, decodeArray,
        shouldInterpolate, kCGRenderingIntentDefault
    );
}

QT_END_NAMESPACE
