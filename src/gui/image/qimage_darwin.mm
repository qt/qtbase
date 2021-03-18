/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qimage.h"

#include <private/qcore_mac_p.h>
#include <private/qcoregraphics_p.h>

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>

QT_BEGIN_NAMESPACE

/*!
    Creates a \c CGImage equivalent to this QImage. Returns a
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

    \sa QtMac::toNSImage()
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

    QCFType<CGColorSpaceRef> colorSpace =  CGColorSpaceCreateWithName(kCGColorSpaceSRGB);

    const size_t bitsPerComponent = 8;
    const size_t bitsPerPixel = 32;
    const CGFloat *decode = nullptr;
    const bool shouldInterpolate = false;

    return CGImageCreate(width(), height(), bitsPerComponent, bitsPerPixel,
                         this->bytesPerLine(), colorSpace, bitmapInfo, dataProvider,
                         decode, shouldInterpolate, kCGRenderingIntentDefault);
}

QT_END_NAMESPACE
