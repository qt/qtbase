/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qcoregraphics_p.h"

#include <private/qcore_mac_p.h>
#include <qpa/qplatformpixmap.h>
#include <QtGui/qicon.h>
#include <QtGui/private/qpaintengine_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

// ---------------------- Images ----------------------

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
                                                    image.byteCount(), deleter);

    return CGImageMaskCreate(image.width(), image.height(), 8, image.depth(),
                              image.bytesPerLine(), dataProvider, NULL, false);
}

OSStatus qt_mac_drawCGImage(CGContextRef inContext, const CGRect *inBounds, CGImageRef inImage)
{
    // Verbatim copy if HIViewDrawCGImage (as shown on Carbon-Dev)
    OSStatus err = noErr;

#ifdef Q_OS_MACOS
    require_action(inContext != NULL, InvalidContext, err = paramErr);
    require_action(inBounds != NULL, InvalidBounds, err = paramErr);
    require_action(inImage != NULL, InvalidImage, err = paramErr);
#endif

    CGContextSaveGState( inContext );
    CGContextTranslateCTM (inContext, 0, inBounds->origin.y + CGRectGetMaxY(*inBounds));
    CGContextScaleCTM(inContext, 1, -1);

    CGContextDrawImage(inContext, *inBounds, inImage);

    CGContextRestoreGState(inContext);

#ifdef Q_OS_MACOS
InvalidImage:
InvalidBounds:
InvalidContext:
#endif
    return err;
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

@interface NSGraphicsContext (QtAdditions)

+ (NSGraphicsContext *)qt_graphicsContextWithCGContext:(CGContextRef)graphicsPort flipped:(BOOL)initialFlippedState;

@end

@implementation NSGraphicsContext (QtAdditions)

+ (NSGraphicsContext *)qt_graphicsContextWithCGContext:(CGContextRef)graphicsPort flipped:(BOOL)initialFlippedState
{
#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_10, __IPHONE_NA)
    if (QT_PREPEND_NAMESPACE(QSysInfo::MacintoshVersion) >= QT_PREPEND_NAMESPACE(QSysInfo::MV_10_10))
        return [self graphicsContextWithCGContext:graphicsPort flipped:initialFlippedState];
#endif
    return [self graphicsContextWithGraphicsPort:graphicsPort flipped:initialFlippedState];
}

@end

QT_BEGIN_NAMESPACE

static NSImage *qt_mac_cgimage_to_nsimage(CGImageRef image)
{
    NSImage *newImage = [[NSImage alloc] initWithCGImage:image size:NSZeroSize];
    return newImage;
}

NSImage *qt_mac_create_nsimage(const QPixmap &pm)
{
    if (pm.isNull())
        return 0;
    QImage image = pm.toImage();
    CGImageRef cgImage = qt_mac_toCGImage(image);
    NSImage *nsImage = qt_mac_cgimage_to_nsimage(cgImage);
    CGImageRelease(cgImage);
    return nsImage;
}

NSImage *qt_mac_create_nsimage(const QIcon &icon, int defaultSize)
{
    if (icon.isNull())
        return nil;

    NSImage *nsImage = [[NSImage alloc] init];
    QList<QSize> availableSizes = icon.availableSizes();
    if (availableSizes.isEmpty() && defaultSize > 0)
        availableSizes << QSize(defaultSize, defaultSize);
    foreach (QSize size, availableSizes) {
        QPixmap pm = icon.pixmap(size);
        QImage image = pm.toImage();
        CGImageRef cgImage = qt_mac_toCGImage(image);
        NSBitmapImageRep *imageRep = [[NSBitmapImageRep alloc] initWithCGImage:cgImage];
        [nsImage addRepresentation:imageRep];
        [imageRep release];
        CGImageRelease(cgImage);
    }
    return nsImage;
}

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
    NSGraphicsContext *gc = [NSGraphicsContext qt_graphicsContextWithCGContext:ctx flipped:YES];
    if (!gc)
        return QPixmap();
    [NSGraphicsContext saveGraphicsState];
    [NSGraphicsContext setCurrentContext:gc];
    [image drawInRect:iconRect fromRect:iconRect operation:NSCompositeSourceOver fraction:1.0 respectFlipped:YES hints:nil];
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
    NSString *colorSpace = [color colorSpaceName];
    if (colorSpace == NSDeviceCMYKColorSpace) {
        CGFloat cyan, magenta, yellow, black, alpha;
        [color getCyan:&cyan magenta:&magenta yellow:&yellow black:&black alpha:&alpha];
        qtColor.setCmykF(cyan, magenta, yellow, black, alpha);
    } else {
        NSColor *tmpColor;
        tmpColor = [color colorUsingColorSpaceName:NSDeviceRGBColorSpace];
        CGFloat red, green, blue, alpha;
        [tmpColor getRed:&red green:&green blue:&blue alpha:&alpha];
        qtColor.setRgbF(red, green, blue, alpha);
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
    if ([color.catalogNameComponent isEqualToString:@"System"] &&
        [color.colorNameComponent isEqualToString:colorNameComponent] &&
        [color.colorSpaceName isEqualToString:NSNamedColorSpace])
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

    if (NSColor *patternColor = [color colorUsingColorSpaceName:NSPatternColorSpace]) {
        NSImage *patternImage = patternColor.patternImage;
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

// ---------------------- Color Management ----------------------

static CGColorSpaceRef m_genericColorSpace = 0;
static QHash<uint32_t, CGColorSpaceRef> m_displayColorSpaceHash;
static bool m_postRoutineRegistered = false;

static void qt_mac_cleanUpMacColorSpaces()
{
    if (m_genericColorSpace) {
        CFRelease(m_genericColorSpace);
        m_genericColorSpace = 0;
    }
    QHash<uint32_t, CGColorSpaceRef>::const_iterator it = m_displayColorSpaceHash.constBegin();
    while (it != m_displayColorSpaceHash.constEnd()) {
        if (it.value())
            CFRelease(it.value());
        ++it;
    }
    m_displayColorSpaceHash.clear();
}

static CGColorSpaceRef qt_mac_displayColorSpace(const QWindow *window)
{
    CGColorSpaceRef colorSpace = 0;
    uint32_t displayID = 0;

#ifdef Q_OS_MACOS
    if (window == 0) {
        displayID = CGMainDisplayID();
    } else {
        displayID = CGMainDisplayID();
        /*
        ### get correct display
        const QRect &qrect = window->geometry();
        CGRect rect = CGRectMake(qrect.x(), qrect.y(), qrect.width(), qrect.height());
        CGDisplayCount throwAway;
        CGDisplayErr dErr = CGGetDisplaysWithRect(rect, 1, &displayID, &throwAway);
        if (dErr != kCGErrorSuccess)
            return macDisplayColorSpace(0); // fall back on main display
        */
    }
    if ((colorSpace = m_displayColorSpaceHash.value(displayID)))
        return colorSpace;

    colorSpace = CGDisplayCopyColorSpace(displayID);
#else
    Q_UNUSED(window);
#endif

    if (colorSpace == 0)
        colorSpace = CGColorSpaceCreateDeviceRGB();

    m_displayColorSpaceHash.insert(displayID, colorSpace);
    if (!m_postRoutineRegistered) {
        m_postRoutineRegistered = true;
        qAddPostRoutine(qt_mac_cleanUpMacColorSpaces);
    }
    return colorSpace;
}

CGColorSpaceRef qt_mac_colorSpaceForDeviceType(const QPaintDevice *paintDevice)
{
    Q_UNUSED(paintDevice);

    // FIXME: Move logic into each paint device once Qt has support for color spaces
    return qt_mac_displayColorSpace(0);

    // The following code seems to take care of QWidget, but in reality doesn't, as
    // qt_mac_displayColorSpace ignores the argument and always uses the main display.
#if 0
    bool isWidget = (paintDevice->devType() == QInternal::Widget);
    return qt_mac_displayColorSpace(isWidget ? static_cast<const QWidget *>(paintDevice)->window() : 0);
#endif
}

CGColorSpaceRef qt_mac_genericColorSpace()
{
#if 0
    if (!m_genericColorSpace) {
        if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_4) {
            m_genericColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
        } else
        {
            m_genericColorSpace = CGColorSpaceCreateDeviceRGB();
        }
        if (!m_postRoutineRegistered) {
            m_postRoutineRegistered = true;
            qAddPostRoutine(QCoreGraphicsPaintEngine::cleanUpMacColorSpaces);
        }
    }
    return m_genericColorSpace;
#else
    // Just return the main display colorspace for the moment.
    return qt_mac_displayColorSpace(0);
#endif
}

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

QMacCGContext::QMacCGContext(QPaintDevice *paintDevice) : context(0)
{
    // In Qt 5, QWidget and QPixmap (and QImage) paint devices are all QImages under the hood.
    QImage *image = 0;
    if (paintDevice->devType() == QInternal::Image) {
        image = static_cast<QImage *>(paintDevice);
    } else if (paintDevice->devType() == QInternal::Pixmap) {

        const QPixmap *pm = static_cast<const QPixmap*>(paintDevice);
        QPlatformPixmap *data = const_cast<QPixmap *>(pm)->data_ptr().data();
        if (data && data->classId() == QPlatformPixmap::RasterClass) {
            image = data->buffer();
        } else {
            qDebug("QMacCGContext: Unsupported pixmap class");
        }
    } else if (paintDevice->devType() == QInternal::Widget) {
        // TODO test: image = static_cast<QImage *>(static_cast<const QWidget *>(paintDevice)->backingStore()->paintDevice());
        qDebug("QMacCGContext: not implemented: Widget class");
    }

    if (!image)
        return; // Context type not supported.

    CGColorSpaceRef colorspace = qt_mac_colorSpaceForDeviceType(paintDevice);
    uint flags = kCGImageAlphaPremultipliedFirst;
    flags |= kCGBitmapByteOrder32Host;

    context = CGBitmapContextCreate(image->bits(), image->width(), image->height(),
                                8, image->bytesPerLine(), colorspace, flags);
    CGContextTranslateCTM(context, 0, image->height());
    const qreal devicePixelRatio = paintDevice->devicePixelRatioF();
    CGContextScaleCTM(context, devicePixelRatio, devicePixelRatio);
    CGContextScaleCTM(context, 1, -1);
}

QMacCGContext::QMacCGContext(QPainter *painter) : context(0)
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

    int devType = painter->device()->devType();
    if (paintEngine->type() == QPaintEngine::Raster
            && (devType == QInternal::Widget ||
                devType == QInternal::Pixmap ||
                devType == QInternal::Image)) {

        CGColorSpaceRef colorspace = qt_mac_colorSpaceForDeviceType(paintEngine->paintDevice());
        uint flags = kCGImageAlphaPremultipliedFirst;
#ifdef kCGBitmapByteOrder32Host //only needed because CGImage.h added symbols in the minor version
        flags |= kCGBitmapByteOrder32Host;
#endif
        const QImage *image = static_cast<const QImage *>(paintEngine->paintDevice());

        context = CGBitmapContextCreate((void *)image->bits(), image->width(), image->height(),
                                        8, image->bytesPerLine(), colorspace, flags);

        // Invert y axis
        CGContextTranslateCTM(context, 0, image->height());
        CGContextScaleCTM(context, 1, -1);

        const qreal devicePixelRatio = image->devicePixelRatio();

        if (devType == QInternal::Widget) {
            // Set the clip rect which is an intersection of the system clip
            // and the painter clip. To make matters more interesting these
            // are in device pixels and device-independent pixels, respectively.
            QRegion clip = painter->paintEngine()->systemClip(); // get system clip in device pixels
            QTransform native = painter->deviceTransform();      // get device transform. dx/dy is in device pixels

            if (painter->hasClipping()) {
                QRegion r = painter->clipRegion();               // get painter clip, which is in device-independent pixels
                qt_mac_scale_region(&r, devicePixelRatio); // scale painter clip to device pixels
                r.translate(native.dx(), native.dy());
                if (clip.isEmpty())
                    clip = r;
                else
                    clip &= r;
            }
            qt_mac_clip_cg(context, clip, 0); // clip in device pixels

            // Scale the context so that painting happens in device-independent pixels
            CGContextScaleCTM(context, devicePixelRatio, devicePixelRatio);
            CGContextTranslateCTM(context, native.dx() / devicePixelRatio, native.dy() / devicePixelRatio);
        } else {
            // Scale to paint in device-independent pixels
            CGContextScaleCTM(context, devicePixelRatio, devicePixelRatio);
        }
    } else {
        qDebug() << "QMacCGContext:: Unsupported painter devtype type" << devType;
    }
}

QT_END_NAMESPACE
