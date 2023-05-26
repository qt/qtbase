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

QT_USE_NAMESPACE

QT_BEGIN_NAMESPACE

// ---------------------- Images ----------------------

CGBitmapInfo qt_mac_bitmapInfoForImage(const QImage &image)
{
    CGBitmapInfo bitmapInfo = kCGImageAlphaNone;
    switch (image.format()) {
    case QImage::Format_ARGB32:
        bitmapInfo = CGBitmapInfo(kCGImageAlphaFirst) | kCGBitmapByteOrder32Host;
        break;
    case QImage::Format_RGB32:
        bitmapInfo = CGBitmapInfo(kCGImageAlphaNoneSkipFirst) | kCGBitmapByteOrder32Host;
        break;
    case QImage::Format_RGBA8888_Premultiplied:
        bitmapInfo = CGBitmapInfo(kCGImageAlphaPremultipliedLast) | kCGBitmapByteOrder32Big;
        break;
    case QImage::Format_RGBA8888:
        bitmapInfo = CGBitmapInfo(kCGImageAlphaLast) | kCGBitmapByteOrder32Big;
        break;
    case QImage::Format_RGBX8888:
        bitmapInfo = CGBitmapInfo(kCGImageAlphaNoneSkipLast) | kCGBitmapByteOrder32Big;
        break;
    case QImage::Format_ARGB32_Premultiplied:
        bitmapInfo = CGBitmapInfo(kCGImageAlphaPremultipliedFirst) | kCGBitmapByteOrder32Host;
        break;
    default: break;
    }
    return bitmapInfo;
}

CGImageRef qt_mac_toCGImage(const QImage &inImage)
{
    CGImageRef cgImage = inImage.toCGImage();
    if (cgImage)
        return cgImage;

    // Convert image data to a known-good format if the fast conversion fails.
    return inImage.convertToFormat(QImage::Format_ARGB32_Premultiplied).toCGImage();
}

CGImageRef qt_mac_toCGImageMask(const QImage &image)
{
    static const auto deleter = [](void *image, const void *, size_t) { delete static_cast<QImage *>(image); };
    QCFType<CGDataProviderRef> dataProvider =
            CGDataProviderCreateWithData(new QImage(image), image.bits(),
                                                    image.sizeInBytes(), deleter);

    return CGImageMaskCreate(image.width(), image.height(), 8, image.depth(),
                              image.bytesPerLine(), dataProvider, NULL, false);
}

void qt_mac_drawCGImage(CGContextRef inContext, const CGRect *inBounds, CGImageRef inImage)
{
    CGContextSaveGState( inContext );
    CGContextTranslateCTM (inContext, 0, inBounds->origin.y + CGRectGetMaxY(*inBounds));
    CGContextScaleCTM(inContext, 1, -1);

    CGContextDrawImage(inContext, *inBounds, inImage);

    CGContextRestoreGState(inContext);
}

QImage qt_mac_toQImage(CGImageRef image)
{
    const size_t w = CGImageGetWidth(image),
                 h = CGImageGetHeight(image);
    QImage ret(w, h, QImage::Format_ARGB32_Premultiplied);
    ret.fill(Qt::transparent);
    CGRect rect = CGRectMake(0, 0, w, h);
    QMacCGContext ctx(&ret);
    qt_mac_drawCGImage(ctx, &rect, image);
    return ret;
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
    return [NSImage imageFromQIcon:icon withSize:0];
}

+ (instancetype)imageFromQIcon:(const QIcon &)icon withSize:(int)size
{
    if (icon.isNull())
        return nil;

    auto availableSizes = icon.availableSizes();
    if (availableSizes.isEmpty() && size > 0)
        availableSizes << QSize(size, size);

    auto nsImage = [[[NSImage alloc] initWithSize:NSZeroSize] autorelease];

    for (QSize size : std::as_const(availableSizes)) {
        QImage image = icon.pixmap(size).toImage();
        if (image.isNull())
            continue;

        QCFType<CGImageRef> cgImage = image.toCGImage();
        if (!cgImage)
            continue;

        auto *imageRep = [[NSBitmapImageRep alloc] initWithCGImage:cgImage];
        imageRep.size = image.deviceIndependentSize().toCGSize();
        [nsImage addRepresentation:[imageRep autorelease]];
    }

    if (!nsImage.representations.count)
        return nil;

    [nsImage setTemplate:icon.isMask()];

    if (size)
        nsImage.size = CGSizeMake(size, size);

    return nsImage;
}
@end

QT_BEGIN_NAMESPACE

QPixmap qt_mac_toQPixmap(const NSImage *image, const QSizeF &size)
{
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
    QCFType<CGColorSpaceRef> colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    context = CGBitmapContextCreate((void *)image->bits(), image->width(), image->height(), 8,
                                    image->bytesPerLine(), colorSpace, qt_mac_bitmapInfoForImage(*image));

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
