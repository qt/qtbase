// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcoregraphics_p.h"

#include <private/qcore_mac_p.h>
#include <qpa/qplatformpixmap.h>
#include <QtGui/qicon.h>
#include <QtGui/private/qpaintengine_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qoperatingsystemversion.h>
#include <QtGui/qcolorspace.h>
#include <QtGui/private/qicon_p.h>

#if defined(Q_OS_MACOS)
# include <AppKit/AppKit.h>
#elif defined(QT_PLATFORM_UIKIT)
# include <UIKit/UIKit.h>
#endif

QT_USE_NAMESPACE

QT_BEGIN_NAMESPACE

// ---------------------- Images ----------------------

std::optional<QCGImageFormat> qt_mac_cgImageFormatForImage(const QImage &image)
{
    const QPixelFormat format = image.pixelFormat();

    // FIXME: Support other color models, such as Grayscale and Alpha,
    // which would require the calling code to use a non-RGB color space.
    if (format.colorModel() != QPixelFormat::RGB)
        return {};

    const int alphaBits = format.alphaSize();

    CGBitmapInfo bitmapInfo = [&]{
        if (!alphaBits)
            return kCGImageAlphaNone;

        if (format.channelCount() == 1)
            return kCGImageAlphaOnly;

        return CGImageAlphaInfo(
            (format.alphaUsage() == QPixelFormat::IgnoresAlpha ?
                kCGImageAlphaNoneSkipLast
              : (format.premultiplied() == QPixelFormat::Premultiplied ?
                    kCGImageAlphaPremultipliedLast : kCGImageAlphaLast)
            ) // 'First' variants have a value one more than their 'Last'
            + (format.alphaPosition() == QPixelFormat::AtBeginning ? 1 : 0)
        );
    }();

    const std::tuple rgbBits{format.redSize(), format.greenSize(), format.blueSize() };

    const CGImageByteOrderInfo byteOrder16Bit =
        format.byteOrder() == QPixelFormat::LittleEndian ?
            kCGImageByteOrder16Little : kCGImageByteOrder16Big;

    const CGImageByteOrderInfo byteOrder32Bit =
        format.byteOrder() == QPixelFormat::LittleEndian ?
            kCGImageByteOrder32Little : kCGImageByteOrder32Big;

    static const auto isPacked = [](const QPixelFormat f) {
        return f.redSize() == f.greenSize()
            && f.greenSize() == f.blueSize()
            && (!f.alphaSize() || f.alphaSize() == f.blueSize());
    };

    switch (format.typeInterpretation()) {
    case QPixelFormat::UnsignedByte:
        // Qt always uses UnsignedByte for BigEndian formats, instead of
        // representing e.g. Format_RGBX8888 as UnsignedInteger+BigEndian,
        // so we need to look at the bits per pixel as well.
        if (format.bitsPerPixel() == 32)
            bitmapInfo |= kCGImageByteOrder32Big;
        else if (format.bitsPerPixel() == 16)
            bitmapInfo |= kCGImageByteOrder16Big;
        else
            bitmapInfo |= kCGImageByteOrderDefault;
        break;
    case QPixelFormat::UnsignedShort:
        bitmapInfo |= byteOrder16Bit;
        if (isPacked(format))
            bitmapInfo |= kCGImagePixelFormatPacked;
        else if (rgbBits == std::tuple{5,5,5} && alphaBits == 1)
            bitmapInfo |= kCGImagePixelFormatRGB555;
        else if (rgbBits == std::tuple{5,6,5} && !alphaBits)
            bitmapInfo |= kCGImagePixelFormatRGB565;
        else
            return {};
        break;
    case QPixelFormat::UnsignedInteger:
        bitmapInfo |= byteOrder32Bit;
        if (isPacked(format))
            bitmapInfo |= kCGImagePixelFormatPacked;
        else if (rgbBits == std::tuple{10,10,10} && alphaBits == 2)
            bitmapInfo |= kCGImagePixelFormatRGB101010;
        else
            return {};
        break;
    case QPixelFormat::FloatingPoint:
        bitmapInfo |= kCGBitmapFloatComponents;
        if (!isPacked(format))
            return {};
        if (format.bitsPerPixel() == 128)
            bitmapInfo |= byteOrder32Bit; // Full float
        else if (format.bitsPerPixel() == 64)
            bitmapInfo |= byteOrder16Bit; // Half float
        else
            return {};
    }

    // By trial and error the logic for the bits per component
    // seems to be the smallest of the color channels. This is
    // also somewhat corroborated by the vImage documentation.
    const uint32_t bitsPerComponent = std::min({
        format.redSize(), format.greenSize(), format.blueSize()
    });

    QCFType<CGColorSpaceRef> colorSpace = [&]{
        if (const auto colorSpace = image.colorSpace(); colorSpace.isValid()) {
            QCFType<CFDataRef> iccData = colorSpace.iccProfile().toCFData();
            return CGColorSpaceCreateWithICCData(iccData);
        } else {
            return CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        }
    }();

    return QCGImageFormat{
        bitsPerComponent, format.bitsPerPixel(),
        colorSpace, bitmapInfo
    };
}

CGImageRef qt_mac_toCGImage(const QImage &inImage)
{
    CGImageRef cgImage = inImage.toCGImage();
    if (cgImage)
        return cgImage;

    // Convert image data to a known-good format if the fast conversion fails.
    return inImage.convertToFormat(QImage::Format_ARGB32_Premultiplied).toCGImage();
}

void qt_mac_drawCGImage(CGContextRef inContext, const CGRect *inBounds, CGImageRef inImage)
{
    CGContextSaveGState( inContext );
    CGContextTranslateCTM (inContext, 0, inBounds->origin.y + CGRectGetMaxY(*inBounds));
    CGContextScaleCTM(inContext, 1, -1);

    CGContextDrawImage(inContext, *inBounds, inImage);

    CGContextRestoreGState(inContext);
}

QImage::Format qt_mac_imageFormatForCGImage(CGImageRef image)
{
    if (!image)
        return QImage::Format_Invalid;

    const CGColorSpaceRef colorSpace = CGImageGetColorSpace(image);
    if (CGColorSpaceGetModel(colorSpace) != kCGColorSpaceModelRGB)
        return QImage::Format_Invalid;

    const CGBitmapInfo bitmapInfo = CGImageGetBitmapInfo(image);
    const auto byteOrder = CGImageByteOrderInfo(bitmapInfo & kCGBitmapByteOrderMask);

    auto qtByteOrder = [&]() -> std::optional<QPixelFormat::ByteOrder> {
        switch (byteOrder) {
        case kCGImageByteOrder16Big:
        case kCGImageByteOrder32Big:
        case kCGImageByteOrderDefault:
            return QPixelFormat::BigEndian;
        case kCGImageByteOrder16Little:
        case kCGImageByteOrder32Little:
            return QPixelFormat::LittleEndian;
        default:
            return {};
        }
    }();
    if (!qtByteOrder)
        return QImage::Format_Invalid;

    auto typeInterpretation = [&]() -> std::optional<QPixelFormat::TypeInterpretation> {
        if (bitmapInfo & kCGBitmapFloatComponents)
            return QPixelFormat::FloatingPoint;
        else if (qtByteOrder == QPixelFormat::BigEndian)
            // Qt always uses UnsignedByte for BigEndian formats, instead of
            // representing e.g. Format_RGBX8888 as UnsignedInteger+BigEndian.
            return QPixelFormat::UnsignedByte;
        else if (byteOrder == kCGImageByteOrder16Little)
            return QPixelFormat::UnsignedShort;
        else if (byteOrder == kCGImageByteOrder32Little)
            return QPixelFormat::UnsignedInteger;
        else
            return {};
    }();
    if (!typeInterpretation)
        return QImage::Format_Invalid;

    const auto alphaInfo = CGImageAlphaInfo(bitmapInfo & kCGBitmapAlphaInfoMask);

    QPixelFormat::AlphaPosition alphaPosition = [&]{
        switch (alphaInfo) {
        case kCGImageAlphaNone:
        case kCGImageAlphaFirst:
        case kCGImageAlphaNoneSkipFirst:
        case kCGImageAlphaPremultipliedFirst:
            return QPixelFormat::AtBeginning;
        default:
            return QPixelFormat::AtEnd;
        }
    }();

    QPixelFormat::AlphaUsage alphaUsage = [&]{
        switch (alphaInfo) {
        case kCGImageAlphaNone:
        case kCGImageAlphaNoneSkipLast:
        case kCGImageAlphaNoneSkipFirst:
            return QPixelFormat::IgnoresAlpha;
        default:
            return QPixelFormat::UsesAlpha;
        }
    }();

    QPixelFormat::AlphaPremultiplied alphaPremultiplied = [&]{
        switch (alphaInfo) {
        case kCGImageAlphaPremultipliedFirst:
        case kCGImageAlphaPremultipliedLast:
            return QPixelFormat::Premultiplied;
        default:
            return QPixelFormat::NotPremultiplied;
        }
    }();

    auto [redSize, greenSize, blueSize, alphaSize] = [&]() -> std::tuple<uchar,uchar,uchar,uchar> {
        const auto pixelFormat = CGImagePixelFormatInfo(bitmapInfo & kCGImagePixelFormatMask);
        const size_t bpc = CGImageGetBitsPerComponent(image);
        if (pixelFormat == kCGImagePixelFormatPacked)
            return {bpc, bpc, bpc, alphaInfo != kCGImageAlphaNone ? bpc : 0};
        else if (pixelFormat == kCGImagePixelFormatRGB555)
            return {5, 5, 5, 1};
        else if (pixelFormat == kCGImagePixelFormatRGB565)
            return {5, 6, 5, 0};
        else if (pixelFormat == kCGImagePixelFormatRGB101010)
            return {10, 10, 10, 2};
        else
            return {0, 0, 0, 0};
    }();

    QPixelFormat pixelFormat(QPixelFormat::RGB, redSize, greenSize, blueSize, 0, 0,
        alphaSize, alphaUsage, alphaPosition, alphaPremultiplied,
        *typeInterpretation, *qtByteOrder);

    return QImage::toImageFormat(pixelFormat);
}

QImage qt_mac_toQImage(CGImageRef cgImage)
{
    const size_t width = CGImageGetWidth(cgImage);
    const size_t height = CGImageGetHeight(cgImage);

    QImage image = [&]() -> QImage {
        QImage::Format imageFormat = qt_mac_imageFormatForCGImage(cgImage);
        if (imageFormat == QImage::Format_Invalid)
            return {};

        CGDataProviderRef dataProvider = CGImageGetDataProvider(cgImage);
        if (!dataProvider)
            return {};

        // Despite its name, this should not copy the actual image data
        CFDataRef data = CGDataProviderCopyData(dataProvider);
        if (!data)
            return {};

        // Adopt data for the lifetime of the QImage
        return QImage(CFDataGetBytePtr(data), width, height,
            CGImageGetBytesPerRow(cgImage), imageFormat,
            QImageCleanupFunction(CFRelease), (void*)data);
    }();

    if (image.isNull()) {
        // Fall back to drawing to a know good format
        image = QImage(width, height, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QMacCGContext context(&image);
        CGRect rect = CGRectMake(0, 0, width, height);
        qt_mac_drawCGImage(context, &rect, cgImage);
    }

    if (!image.isNull()) {
        CGColorSpaceRef colorSpace = CGImageGetColorSpace(cgImage);
        QCFType<CFDataRef> iccData = CGColorSpaceCopyICCData(colorSpace);
        image.setColorSpace(QColorSpace::fromIccProfile(QByteArray::fromCFData(iccData)));
    }

    return image;
}

QImage qt_mac_padToSquareImage(const QImage &image)
{
    if (image.width() == image.height())
        return image;

    const int size = std::max(image.width(), image.height());
    QImage squareImage(size, size, image.format());
    squareImage.setDevicePixelRatio(image.devicePixelRatio());
    squareImage.fill(Qt::transparent);

    QPoint pos((size - image.width()) / (2.0 * image.devicePixelRatio()),
               (size - image.height()) / (2.0 * image.devicePixelRatio()));

    QPainter painter(&squareImage);
    painter.drawImage(pos, image);
    painter.end();

    return squareImage;
}

#ifdef Q_OS_MACOS

QT_END_NAMESPACE

@implementation NSImage (QtExtras)
+ (instancetype)imageFromQImage:(const QImage &)image
{
    if (image.isNull())
        return nil;

    QCFType<CGImageRef> cgImage = image.toCGImage();
    if (!cgImage)
        return nil;

    // We set up the NSImage using an explicit NSBitmapImageRep, instead of
    // [NSImage initWithCGImage:size:], as the former allows us to correctly
    // set the size of the representation to account for the device pixel
    // ratio of the original image, which in turn will be reflected by the
    // NSImage.
    auto nsImage = [[NSImage alloc] initWithSize:NSZeroSize];
    auto *imageRep = [[NSBitmapImageRep alloc] initWithCGImage:cgImage];
    imageRep.size = image.deviceIndependentSize().toCGSize();
    [nsImage addRepresentation:[imageRep autorelease]];
    Q_ASSERT(CGSizeEqualToSize(nsImage.size, imageRep.size));

    return [nsImage autorelease];
}

+ (instancetype)imageFromQIcon:(const QIcon &)icon
{
    return [NSImage imageFromQIcon:icon withSize:QSize()];
}

+ (instancetype)imageFromQIcon:(const QIcon &)icon withSize:(const QSize &)size
{
    return [NSImage imageFromQIcon:icon withSize:size withMode:QIcon::Normal withState:QIcon::Off];
}

+ (instancetype)imageFromQIcon:(const QIcon &)icon withSize:(const QSize &)size
                    withMode:(QIcon::Mode)mode withState:(QIcon::State)state

{
    if (icon.isNull())
        return nil;

    auto availableSizes = icon.availableSizes();
    if (availableSizes.isEmpty() && !size.isNull())
        availableSizes << size;

    auto nsImage = [[[NSImage alloc] initWithSize:NSZeroSize] autorelease];

    for (QSize size : std::as_const(availableSizes)) {
        const QImage image = icon.pixmap(size, mode, state).toImage();
        if (image.isNull())
            continue;

        QCFType<CGImageRef> cgImage = image.toCGImage();
        if (!cgImage)
            continue;

        auto *imageRep = [[NSBitmapImageRep alloc] initWithCGImage:cgImage];
        imageRep.size = image.deviceIndependentSize().toCGSize();
        [nsImage addRepresentation:[imageRep autorelease]];
        // Match behavior of loading icns files, where the NSImage size
        // reflects the largest representation.
        nsImage.size = imageRep.size;
    }

    if (!nsImage.representations.count)
        return nil;

    [nsImage setTemplate:icon.isMask()];

    if (!size.isNull()) {
        auto imageSize = QSizeF::fromCGSize(nsImage.size);
        nsImage.size = imageSize.scaled(size, Qt::KeepAspectRatio).toCGSize();
    }

    return nsImage;
}

+ (instancetype)internalImageFromQIcon:(const QT_PREPEND_NAMESPACE(QIcon) &)icon
{
    if (icon.isNull())
        return nil;

    // Check if the icon is backed by an NSImage. If so, we can use that directly.
    auto *iconPrivate = QIconPrivate::get(&icon);
    NSImage *iconImage = nullptr;
    iconPrivate->engine->virtual_hook(QIconPrivate::PlatformIconHook, &iconImage);
    return iconImage;
}

@end

QT_BEGIN_NAMESPACE

QPixmap qt_mac_toQPixmap(const NSImage *image, const QSizeF &size)
{
    // ### TODO: add parameter so that we can decide whether to maintain the aspect
    // ratio of the image (positioning the image inside the pixmap of size \a size),
    // or whether we want to fill the resulting pixmap by stretching the image.
    const NSSize pixmapSize = NSMakeSize(size.width(), size.height());
    QPixmap pixmap(pixmapSize.width, pixmapSize.height);
    pixmap.fill(Qt::transparent);
    [image setSize:pixmapSize];
    const NSRect iconRect = NSMakeRect(0, 0, pixmapSize.width, pixmapSize.height);
    QMacCGContext ctx(&pixmap);
    if (!ctx)
        return QPixmap();
    NSGraphicsContext *gc = [NSGraphicsContext graphicsContextWithCGContext:ctx flipped:YES];
    if (!gc)
        return QPixmap();
    [NSGraphicsContext saveGraphicsState];
    [NSGraphicsContext setCurrentContext:gc];
    [image drawInRect:iconRect fromRect:iconRect operation:NSCompositingOperationSourceOver fraction:1.0 respectFlipped:YES hints:nil];
    [NSGraphicsContext restoreGraphicsState];
    return pixmap;
}

#endif // Q_OS_MACOS

#ifdef QT_PLATFORM_UIKIT

QImage qt_mac_toQImage(const UIImage *image, QSizeF size)
{
    // ### TODO: same as above
    QImage ret(size.width(), size.height(), QImage::Format_ARGB32_Premultiplied);
    ret.fill(Qt::transparent);
    QMacCGContext ctx(&ret);
    if (!ctx)
        return QImage();
    UIGraphicsPushContext(ctx);
    const CGRect rect = CGRectMake(0, 0, size.width(), size.height());
    [image drawInRect:rect];
    UIGraphicsPopContext();
    return ret;
}

#endif // QT_PLATFORM_UIKIT

// ---------------------- Colors and Brushes ----------------------

QColor qt_mac_toQColor(CGColorRef color)
{
    QColor qtColor;
    CGColorSpaceModel model = CGColorSpaceGetModel(CGColorGetColorSpace(color));
    const CGFloat *components = CGColorGetComponents(color);
    if (model == kCGColorSpaceModelRGB) {
        qtColor.setRgbF(components[0], components[1], components[2], components[3]);
    } else if (model == kCGColorSpaceModelCMYK) {
        qtColor.setCmykF(components[0], components[1], components[2], components[3]);
    } else if (model == kCGColorSpaceModelMonochrome) {
        qtColor.setRgbF(components[0], components[0], components[0], components[1]);
    } else {
        // Colorspace we can't deal with.
        qWarning("Qt: qt_mac_toQColor: cannot convert from colorspace model: %d", model);
        Q_ASSERT(false);
    }
    return qtColor;
}

#ifdef Q_OS_MACOS
QColor qt_mac_toQColor(const NSColor *color)
{
    if (!color)
        return QColor();

    QColor qtColor;
    switch (color.type) {
    case NSColorTypeComponentBased: {
        const NSColorSpace *colorSpace = [color colorSpace];
        if (colorSpace == NSColorSpace.genericRGBColorSpace
            && color.numberOfComponents == 4) { // rbga
            CGFloat components[4];
            [color getComponents:components];
            qtColor.setRgbF(components[0], components[1], components[2], components[3]);
            break;
        } else if (colorSpace == NSColorSpace.genericCMYKColorSpace
                   && color.numberOfComponents == 5) { // cmyk + alpha
            CGFloat components[5];
            [color getComponents:components];
            qtColor.setCmykF(components[0], components[1], components[2], components[3], components[4]);
            break;
        }
    }
        Q_FALLTHROUGH();
    default: {
        const NSColor *tmpColor = [color colorUsingColorSpace:NSColorSpace.genericRGBColorSpace];
        CGFloat red = 0, green = 0, blue = 0, alpha = 0;
        [tmpColor getRed:&red green:&green blue:&blue alpha:&alpha];
        qtColor.setRgbF(red, green, blue, alpha);
        break;
    }
    }

    return qtColor;
}
#endif

QBrush qt_mac_toQBrush(CGColorRef color)
{
    QBrush qtBrush;
    CGColorSpaceModel model = CGColorSpaceGetModel(CGColorGetColorSpace(color));
    if (model == kCGColorSpaceModelPattern) {
        // Colorspace we can't deal with; the color is drawn directly using a callback.
        qWarning("Qt: qt_mac_toQBrush: cannot convert from colorspace model: %d", model);
        Q_ASSERT(false);
    } else {
        qtBrush.setStyle(Qt::SolidPattern);
        qtBrush.setColor(qt_mac_toQColor(color));
    }
    return qtBrush;
}

#ifdef Q_OS_MACOS
static bool qt_mac_isSystemColorOrInstance(const NSColor *color, NSString *colorNameComponent, NSString *className)
{
    // We specifically do not want isKindOfClass: here
    if ([color.className isEqualToString:className]) // NSPatternColorSpace
        return true;
    if (color.type == NSColorTypeCatalog &&
        [color.catalogNameComponent isEqualToString:@"System"] &&
        [color.colorNameComponent isEqualToString:colorNameComponent])
        return true;
    return false;
}

QBrush qt_mac_toQBrush(const NSColor *color, QPalette::ColorGroup colorGroup)
{
    QBrush qtBrush;

    // QTBUG-49773: This calls NSDrawMenuItemBackground to render a 1 by n gradient; could use HITheme
    if ([color.className isEqualToString:@"NSMenuItemHighlightColor"]) {
        qWarning("Qt: qt_mac_toQBrush: cannot convert from NSMenuItemHighlightColor");
        return qtBrush;
    }

    // Not a catalog color or a manifestation of System.windowBackgroundColor;
    // only retrieved from NSWindow.backgroundColor directly
    if ([color.className isEqualToString:@"NSMetalPatternColor"]) {
        // NSTexturedBackgroundWindowMask, could theoretically handle this without private API by
        // creating a window with the appropriate properties and then calling NSWindow.backgroundColor.patternImage,
        // which returns a texture sized 1 by (window height, including frame), backed by a CGPattern
        // which follows the window key state... probably need to allow QBrush to store a function pointer
        // like CGPattern does
        qWarning("Qt: qt_mac_toQBrush: cannot convert from NSMetalPatternColor");
        return qtBrush;
    }

    // No public API to get these colors/stops;
    // both accurately obtained through runtime object inspection on OS X 10.11
    // (the NSColor object has NSGradient i-vars for both color groups)
    if (qt_mac_isSystemColorOrInstance(color, @"_sourceListBackgroundColor", @"NSSourceListBackgroundColor")) {
        QLinearGradient gradient;
        if (colorGroup == QPalette::Active) {
            gradient.setColorAt(0, QColor(233, 237, 242));
            gradient.setColorAt(0.5, QColor(225, 229, 235));
            gradient.setColorAt(1, QColor(209, 216, 224));
        } else {
            gradient.setColorAt(0, QColor(248, 248, 248));
            gradient.setColorAt(0.5, QColor(240, 240, 240));
            gradient.setColorAt(1, QColor(235, 235, 235));
        }
        return QBrush(gradient);
    }

    // A couple colors are special... they are actually instances of NSGradientPatternColor, which
    // override set/setFill/setStroke to instead initialize an internal color
    // ([NSColor colorWithCalibratedWhite:0.909804 alpha:1.000000]) while still returning the
    // ruled lines pattern image (from OS X 10.4) to the user from -[NSColor patternImage]
    // (and providing no public API to get the underlying color without this insanity)
    if (qt_mac_isSystemColorOrInstance(color, @"controlColor", @"NSGradientPatternColor") ||
        qt_mac_isSystemColorOrInstance(color, @"windowBackgroundColor", @"NSGradientPatternColor")) {
        qtBrush.setStyle(Qt::SolidPattern);
        qtBrush.setColor(qt_mac_toQColor(color.CGColor));
        return qtBrush;
    }

    if (color.type == NSColorTypePattern) {
        NSImage *patternImage = color.patternImage;
        const QSizeF sz(patternImage.size.width, patternImage.size.height);
        // FIXME: QBrush is not resolution independent (QTBUG-49774)
        qtBrush.setTexture(qt_mac_toQPixmap(patternImage, sz));
    } else {
        qtBrush.setStyle(Qt::SolidPattern);
        qtBrush.setColor(qt_mac_toQColor(color));
    }
    return qtBrush;
}
#endif

// ---------------------- Geometry Helpers ----------------------

void qt_mac_clip_cg(CGContextRef hd, const QRegion &rgn, CGAffineTransform *orig_xform)
{
    CGAffineTransform old_xform = CGAffineTransformIdentity;
    if (orig_xform) { //setup xforms
        old_xform = CGContextGetCTM(hd);
        CGContextConcatCTM(hd, CGAffineTransformInvert(old_xform));
        CGContextConcatCTM(hd, *orig_xform);
    }

    //do the clipping
    CGContextBeginPath(hd);
    if (rgn.isEmpty()) {
        CGContextAddRect(hd, CGRectMake(0, 0, 0, 0));
    } else {
        for (const QRect &r : rgn) {
            CGRect mac_r = CGRectMake(r.x(), r.y(), r.width(), r.height());
            CGContextAddRect(hd, mac_r);
        }
    }
    CGContextClip(hd);

    if (orig_xform) {//reset xforms
        CGContextConcatCTM(hd, CGAffineTransformInvert(CGContextGetCTM(hd)));
        CGContextConcatCTM(hd, old_xform);
    }
}

// move to QRegion?
void qt_mac_scale_region(QRegion *region, qreal scaleFactor)
{
    if (!region || !region->rectCount())
        return;

    QVector<QRect> scaledRects;
    scaledRects.reserve(region->rectCount());

    for (const QRect &rect : *region)
        scaledRects.append(QRect(rect.topLeft() * scaleFactor, rect.size() * scaleFactor));

    region->setRects(&scaledRects[0], scaledRects.count());
}

// ---------------------- QMacCGContext ----------------------

QMacCGContext::QMacCGContext(QPaintDevice *paintDevice)
{
    initialize(paintDevice);
}

void QMacCGContext::initialize(QPaintDevice *paintDevice)
{
    // Find the underlying QImage of the paint device
    switch (int deviceType = paintDevice->devType()) {
    case QInternal::Pixmap: {
        if (auto *platformPixmap = static_cast<QPixmap*>(paintDevice)->handle()) {
            if (platformPixmap->classId() == QPlatformPixmap::RasterClass)
                initialize(platformPixmap->buffer());
            else
                qWarning() << "QMacCGContext: Unsupported pixmap class" << platformPixmap->classId();
        } else {
            qWarning() << "QMacCGContext: Empty platformPixmap";
        }
        break;
    }
    case QInternal::Image:
        initialize(static_cast<const QImage *>(paintDevice));
        break;
    case QInternal::Widget:
        qWarning() << "QMacCGContext: not implemented: Widget class";
        break;
    default:
        qWarning() << "QMacCGContext:: Unsupported paint device type" << deviceType;
    }
}

QMacCGContext::QMacCGContext(QPainter *painter)
{
    QPaintEngine *paintEngine = painter->paintEngine();

    // Handle the case of QMacPrintEngine, which has an internal QCoreGraphicsPaintEngine
    while (QPaintEngine *aggregateEngine = QPaintEnginePrivate::get(paintEngine)->aggregateEngine())
        paintEngine = aggregateEngine;

    paintEngine->syncState();

    if (Qt::HANDLE handle = QPaintEnginePrivate::get(paintEngine)->nativeHandle()) {
        context = static_cast<CGContextRef>(handle);
        return;
    }

    if (paintEngine->type() != QPaintEngine::Raster) {
        qWarning() << "QMacCGContext:: Unsupported paint engine type" << paintEngine->type();
        return;
    }

    // The raster paint engine always operates on a QImage
    Q_ASSERT(paintEngine->paintDevice()->devType() == QInternal::Image);

    // On behalf of one of these supported painter devices
    switch (int painterDeviceType = painter->device()->devType()) {
    case QInternal::Pixmap:
    case QInternal::Image:
    case QInternal::Widget:
        break;
    default:
        qWarning() << "QMacCGContext:: Unsupported paint device type" << painterDeviceType;
        return;
    }

    // Applying the clip is so entangled with the rest of the context setup
    // that for simplicity we just pass in the painter.
    initialize(static_cast<const QImage *>(paintEngine->paintDevice()), painter);
}

void QMacCGContext::initialize(const QImage *image, QPainter *painter)
{
    auto cgImageFormat = qt_mac_cgImageFormatForImage(*image);
    if (!cgImageFormat) {
        qWarning() << "QMacCGContext:: Could not get bitmap info for" << image;
        return;
    }

    context = CGBitmapContextCreate((void *)image->bits(), image->width(), image->height(),
        cgImageFormat->bitsPerComponent, image->bytesPerLine(), cgImageFormat->colorSpace,
        cgImageFormat->bitmapInfo);

    // Invert y axis
    CGContextTranslateCTM(context, 0, image->height());
    CGContextScaleCTM(context, 1, -1);

    const qreal devicePixelRatio = image->devicePixelRatio();

    if (painter && painter->device()->devType() == QInternal::Widget) {
        // Set the clip rect which is an intersection of the system clip and the painter clip
        QRegion clip = painter->paintEngine()->systemClip();
        QTransform deviceTransform = painter->deviceTransform();

        if (painter->hasClipping()) {
            // To make matters more interesting the painter clip is in device-independent pixels,
            // so we need to scale it to match the device-pixels of the system clip.
            QRegion painterClip = painter->clipRegion();
            qt_mac_scale_region(&painterClip, devicePixelRatio);

            painterClip.translate(deviceTransform.dx(), deviceTransform.dy());

            if (clip.isEmpty())
                clip = painterClip;
            else
                clip &= painterClip;
        }

        qt_mac_clip_cg(context, clip, nullptr);

        CGContextTranslateCTM(context, deviceTransform.dx(), deviceTransform.dy());
    }

    // Scale the context so that painting happens in device-independent pixels
    CGContextScaleCTM(context, devicePixelRatio, devicePixelRatio);
}

QT_END_NAMESPACE
