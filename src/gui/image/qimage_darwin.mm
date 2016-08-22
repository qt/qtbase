/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qimage.h"

#include <private/qcore_mac_p.h>

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>

QT_BEGIN_NAMESPACE

/*!
    Creates a \c CGImage equivalent to the QImage \a image. Returns a
    \c CGImageRef handle.

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
    convert the QImage to a supported formate first, for example
    Format_ARGB32_Premultiplied.

    The CGImageRef color space is set to the sRGB color space.

    \sa toNSImage()
*/
CGImageRef QImage::toCGImage() const
{
    if (isNull())
        return nil;

    // Determine the target native format
    uint cgflags = kCGImageAlphaNone;
    switch (format()) {
    case QImage::Format_ARGB32:
        cgflags = kCGImageAlphaFirst | kCGBitmapByteOrder32Host;
        break;
    case QImage::Format_RGB32:
        cgflags = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host;
        break;
    case QImage::Format_RGBA8888_Premultiplied:
        cgflags = kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big;
        break;
    case QImage::Format_RGBA8888:
        cgflags = kCGImageAlphaLast | kCGBitmapByteOrder32Big;
        break;
    case QImage::Format_RGBX8888:
        cgflags = kCGImageAlphaNoneSkipLast | kCGBitmapByteOrder32Big;
        break;
    case QImage::Format_ARGB32_Premultiplied:
        cgflags = kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host;
        break;
    default: break;
    }

    // Format not supported: return nil CGImageRef
    if (cgflags == kCGImageAlphaNone)
        return nil;

    // Create a data provider that owns a copy of the QImage and references the image data.
    auto deleter = [](void *image, const void *, size_t)
                   { delete static_cast<QImage *>(image); };
    QCFType<CGDataProviderRef> dataProvider =
        CGDataProviderCreateWithData(new QImage(*this), bits(), byteCount(), deleter);

    QCFType<CGColorSpaceRef> colorSpace =  CGColorSpaceCreateWithName(kCGColorSpaceSRGB);

    const size_t bitsPerComponent = 8;
    const size_t bitsPerPixel = 32;
    const CGFloat *decode = nullptr;
    const bool shouldInterpolate = false;

    return CGImageCreate(width(), height(), bitsPerComponent, bitsPerPixel,
                         this->bytesPerLine(), colorSpace, cgflags, dataProvider,
                         decode, shouldInterpolate, kCGRenderingIntentDefault);
}

QT_END_NAMESPACE
