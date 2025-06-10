// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <AppKit/AppKit.h>

#include "qcocoacursor.h"
#include "qcocoawindow.h"
#include "qcocoascreen.h"
#include "qcocoahelpers.h"
#include <QtGui/private/qcoregraphics_p.h>

#include <QtGui/QBitmap>

#if !defined(QT_APPLE_NO_PRIVATE_APIS)
@interface NSCursor()
+ (id)busyButClickableCursor;
+ (id)_helpCursor;
+ (id)_windowResizeNorthWestSouthEastCursor;
+ (id)_windowResizeNorthEastSouthWestCursor;
+ (id)_windowResizeNorthSouthCursor;
+ (id)_windowResizeEastWestCursor;
@end
#endif // QT_APPLE_NO_PRIVATE_APIS

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QCocoaCursor::QCocoaCursor()
{
}

QCocoaCursor::~QCocoaCursor()
{
    // release cursors
    QHash<Qt::CursorShape, NSCursor *>::const_iterator i = m_cursors.constBegin();
    while (i != m_cursors.constEnd()) {
        [*i release];
        ++i;
    }
}

void QCocoaCursor::changeCursor(QCursor *cursor, QWindow *window)
{
    NSCursor *cocoaCursor = convertCursor(cursor);

    if (QPlatformWindow * platformWindow = window->handle())
        static_cast<QCocoaWindow *>(platformWindow)->setWindowCursor(cocoaCursor);
}

QPoint QCocoaCursor::pos() const
{
    return QCocoaScreen::mapFromNative([NSEvent mouseLocation]).toPoint();
}

void QCocoaCursor::setPos(const QPoint &position)
{
    CGPoint pos;
    pos.x = position.x();
    pos.y = position.y();

    CGEventRef e = CGEventCreateMouseEvent(nullptr, kCGEventMouseMoved, pos, kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, e);
    CFRelease(e);
}


QSize QCocoaCursor::size() const
{
    NSCursor *cocoaCursor = NSCursor.currentSystemCursor;
    if (!cocoaCursor)
        return QPlatformCursor::size();
    NSImage *cursorImage = cocoaCursor.image;
    if (!cursorImage)
        return QPlatformCursor::size();

    QSizeF size = QSizeF::fromCGSize(cursorImage.size);
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    NSDictionary *accessSettings = [defaults persistentDomainForName:@"com.apple.universalaccess"];
    if (accessSettings == nil)
        return size.toSize();

    float sizeScale = [accessSettings[@"mouseDriverCursorSize"] floatValue];
    if (sizeScale > 0) {
        size.rwidth() *= sizeScale;
        size.rheight() *= sizeScale;
    }

    return size.toSize();
}

NSCursor *QCocoaCursor::convertCursor(QCursor *cursor)
{
    if (!cursor)
        return nil;

    const Qt::CursorShape newShape = cursor->shape();
    NSCursor *cocoaCursor = nil;

    // Check for a suitable built-in NSCursor first:
    switch (newShape) {
    case Qt::ArrowCursor:
        cocoaCursor= [NSCursor arrowCursor];
        break;
    case Qt::ForbiddenCursor:
        cocoaCursor = [NSCursor operationNotAllowedCursor];
        break;
    case Qt::CrossCursor:
        cocoaCursor = [NSCursor crosshairCursor];
        break;
    case Qt::IBeamCursor:
        cocoaCursor = [NSCursor IBeamCursor];
        break;
    case Qt::WhatsThisCursor:
#if !defined(QT_APPLE_NO_PRIVATE_APIS)
        if ([NSCursor respondsToSelector:@selector(_helpCursor)]) {
            cocoaCursor = [NSCursor performSelector:@selector(_helpCursor)];
            break;
        }
#endif
         // For now use the pointing hand as a fallback
        Q_FALLTHROUGH();
    case Qt::PointingHandCursor:
        cocoaCursor = [NSCursor pointingHandCursor];
        break;
    case Qt::SplitVCursor:
        cocoaCursor = [NSCursor resizeUpDownCursor];
        break;
    case Qt::SplitHCursor:
        cocoaCursor = [NSCursor resizeLeftRightCursor];
        break;
    case Qt::OpenHandCursor:
        cocoaCursor = [NSCursor openHandCursor];
        break;
    case Qt::ClosedHandCursor:
        cocoaCursor = [NSCursor closedHandCursor];
        break;
    case Qt::DragMoveCursor:
        cocoaCursor = [NSCursor crosshairCursor];
        break;
    case Qt::DragCopyCursor:
        cocoaCursor = [NSCursor dragCopyCursor];
        break;
    case Qt::DragLinkCursor:
        cocoaCursor = [NSCursor dragLinkCursor];
        break;
    case Qt::BusyCursor:
#if !defined(QT_APPLE_NO_PRIVATE_APIS)
        if ([NSCursor respondsToSelector:@selector(busyButClickableCursor)])
            cocoaCursor = [NSCursor performSelector:@selector(busyButClickableCursor)];
#endif
        break;
    case Qt::SizeVerCursor:
    case Qt::SizeHorCursor:
    case Qt::SizeBDiagCursor:
    case Qt::SizeFDiagCursor: {
#if QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(150000)
        if (@available(macOS 15, *)) {
            auto position = [newShape]{
                switch (newShape) {
                case Qt::SizeVerCursor: return NSCursorFrameResizePositionTop;
                case Qt::SizeHorCursor: return NSCursorFrameResizePositionLeft;
                case Qt::SizeBDiagCursor: return NSCursorFrameResizePositionTopRight;
                case Qt::SizeFDiagCursor: return NSCursorFrameResizePositionTopLeft;
                default: Q_UNREACHABLE();
                }
            }();
            cocoaCursor = [NSCursor frameResizeCursorFromPosition:position
                                    inDirections:NSCursorFrameResizeDirectionsAll];
            break;
        }
#endif // macOS 15 SDK
#if !defined(QT_APPLE_NO_PRIVATE_APIS)
        auto selector = [newShape]{
            switch (newShape) {
            case Qt::SizeVerCursor: return @selector(_windowResizeNorthSouthCursor);
            case Qt::SizeHorCursor: return @selector(_windowResizeEastWestCursor);
            case Qt::SizeBDiagCursor: return @selector(_windowResizeNorthEastSouthWestCursor);
            case Qt::SizeFDiagCursor: return @selector(_windowResizeNorthWestSouthEastCursor);
            default: Q_UNREACHABLE();
            }
        }();

        if ([NSCursor respondsToSelector:selector])
            cocoaCursor = [NSCursor performSelector:selector];
#endif // QT_APPLE_NO_PRIVATE_APIS
        break;
    }
    default:
        break;
    }

    if (!cocoaCursor) {
        // No suitable OS cursor exist, use cursors provided
        // by Qt for the rest. Check for a cached cursor:
        cocoaCursor = m_cursors.value(newShape);
        if (cocoaCursor && cursor->shape() == Qt::BitmapCursor) {
            [cocoaCursor release];
            cocoaCursor = nil;
        }
        if (!cocoaCursor) {
            cocoaCursor = createCursorData(cursor);
            if (!cocoaCursor)
                return [NSCursor arrowCursor];

            m_cursors.insert(newShape, cocoaCursor);
        }
    }
    return cocoaCursor;
}


// Creates an NSCursor for the given QCursor.
NSCursor *QCocoaCursor::createCursorData(QCursor *cursor)
{
    /* Note to self... ***
     * mask x data
     * 0xFF x 0x00 == fully opaque white
     * 0x00 x 0xFF == xor'd black
     * 0xFF x 0xFF == fully opaque black
     * 0x00 x 0x00 == fully transparent
     */
#define QT_USE_APPROXIMATE_CURSORS
#ifdef QT_USE_APPROXIMATE_CURSORS
    static const uchar cur_ver_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x03, 0xc0, 0x07, 0xe0, 0x0f, 0xf0,
        0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x0f, 0xf0,
        0x07, 0xe0, 0x03, 0xc0, 0x01, 0x80, 0x00, 0x00 };
    static const uchar mcur_ver_bits[] = {
        0x00, 0x00, 0x03, 0x80, 0x07, 0xc0, 0x0f, 0xe0, 0x1f, 0xf0, 0x3f, 0xf8,
        0x7f, 0xfc, 0x07, 0xc0, 0x07, 0xc0, 0x07, 0xc0, 0x7f, 0xfc, 0x3f, 0xf8,
        0x1f, 0xf0, 0x0f, 0xe0, 0x07, 0xc0, 0x03, 0x80 };

    static const uchar cur_hor_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x20, 0x18, 0x30,
        0x38, 0x38, 0x7f, 0xfc, 0x7f, 0xfc, 0x38, 0x38, 0x18, 0x30, 0x08, 0x20,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const uchar mcur_hor_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x04, 0x40, 0x0c, 0x60, 0x1c, 0x70, 0x3c, 0x78,
        0x7f, 0xfc, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0x7f, 0xfc, 0x3c, 0x78,
        0x1c, 0x70, 0x0c, 0x60, 0x04, 0x40, 0x00, 0x00 };

    static const uchar cur_fdiag_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xf8, 0x00, 0xf8, 0x00, 0x78,
        0x00, 0xf8, 0x01, 0xd8, 0x23, 0x88, 0x37, 0x00, 0x3e, 0x00, 0x3c, 0x00,
        0x3e, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const uchar mcur_fdiag_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x07, 0xfc, 0x03, 0xfc, 0x01, 0xfc, 0x00, 0xfc,
        0x41, 0xfc, 0x63, 0xfc, 0x77, 0xdc, 0x7f, 0x8c, 0x7f, 0x04, 0x7e, 0x00,
        0x7f, 0x00, 0x7f, 0x80, 0x7f, 0xc0, 0x00, 0x00 };

    static const uchar cur_bdiag_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x3e, 0x00, 0x3c, 0x00, 0x3e, 0x00,
        0x37, 0x00, 0x23, 0x88, 0x01, 0xd8, 0x00, 0xf8, 0x00, 0x78, 0x00, 0xf8,
        0x01, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const uchar mcur_bdiag_bits[] = {
        0x00, 0x00, 0x7f, 0xc0, 0x7f, 0x80, 0x7f, 0x00, 0x7e, 0x00, 0x7f, 0x04,
        0x7f, 0x8c, 0x77, 0xdc, 0x63, 0xfc, 0x41, 0xfc, 0x00, 0xfc, 0x01, 0xfc,
        0x03, 0xfc, 0x07, 0xfc, 0x00, 0x00, 0x00, 0x00 };

    static const unsigned char cur_up_arrow_bits[] = {
        0x00, 0x80, 0x01, 0x40, 0x01, 0x40, 0x02, 0x20, 0x02, 0x20, 0x04, 0x10,
        0x04, 0x10, 0x08, 0x08, 0x0f, 0x78, 0x01, 0x40, 0x01, 0x40, 0x01, 0x40,
        0x01, 0x40, 0x01, 0x40, 0x01, 0x40, 0x01, 0xc0 };
    static const unsigned char mcur_up_arrow_bits[] = {
        0x00, 0x80, 0x01, 0xc0, 0x01, 0xc0, 0x03, 0xe0, 0x03, 0xe0, 0x07, 0xf0,
        0x07, 0xf0, 0x0f, 0xf8, 0x0f, 0xf8, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0,
        0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0 };
#endif
    const uchar *cursorData = nullptr;
    const uchar *cursorMaskData = nullptr;
    QPoint hotspot = cursor->hotSpot();

    switch (cursor->shape()) {
    case Qt::BitmapCursor: {
        if (cursor->pixmap().isNull())
            return createCursorFromBitmap(cursor->bitmap(), cursor->mask(), hotspot);
        else
            return createCursorFromPixmap(cursor->pixmap(), hotspot);
        break; }
    case Qt::BlankCursor: {
        QPixmap pixmap = QPixmap(16, 16);
        pixmap.fill(Qt::transparent);
        return createCursorFromPixmap(pixmap);
        break; }
    case Qt::WaitCursor: {
        QPixmap pixmap = QPixmap(":/qt-project.org/mac/cursors/images/spincursor.png"_L1);
        return createCursorFromPixmap(pixmap, hotspot);
        break; }
    case Qt::SizeAllCursor: {
        QPixmap pixmap = QPixmap(":/qt-project.org/mac/cursors/images/sizeallcursor.png"_L1);
        return createCursorFromPixmap(pixmap, QPoint(8, 8));
        break; }
    case Qt::BusyCursor: {
        QPixmap pixmap = QPixmap(":/qt-project.org/mac/cursors/images/waitcursor.png"_L1);
        return createCursorFromPixmap(pixmap, hotspot);
        break; }
#define QT_USE_APPROXIMATE_CURSORS
#ifdef QT_USE_APPROXIMATE_CURSORS
    case Qt::SizeVerCursor:
        cursorData = cur_ver_bits;
        cursorMaskData = mcur_ver_bits;
        hotspot = QPoint(8, 8);
        break;
    case Qt::SizeHorCursor:
        cursorData = cur_hor_bits;
        cursorMaskData = mcur_hor_bits;
        hotspot = QPoint(8, 8);
        break;
    case Qt::SizeBDiagCursor:
        cursorData = cur_fdiag_bits;
        cursorMaskData = mcur_fdiag_bits;
        hotspot = QPoint(8, 8);
        break;
    case Qt::SizeFDiagCursor:
        cursorData = cur_bdiag_bits;
        cursorMaskData = mcur_bdiag_bits;
        hotspot = QPoint(8, 8);
        break;
    case Qt::UpArrowCursor:
        cursorData = cur_up_arrow_bits;
        cursorMaskData = mcur_up_arrow_bits;
        hotspot = QPoint(8, 0);
        break;
#endif
    default:
        qWarning("Qt: QCursor::update: Invalid cursor shape %d", cursor->shape());
        return nil;
    }

    // Create an NSCursor from image data if this a self-provided cursor.
    if (cursorData) {
        QBitmap bitmap(QBitmap::fromData(QSize(16, 16), cursorData, QImage::Format_Mono));
        QBitmap mask(QBitmap::fromData(QSize(16, 16), cursorMaskData, QImage::Format_Mono));
        return (createCursorFromBitmap(bitmap, mask, hotspot));
    }

    return nil; // should not happen, all cases covered above
}

NSCursor *QCocoaCursor::createCursorFromBitmap(const QBitmap &bitmap, const QBitmap &mask, const QPoint hotspot)
{
    QImage finalCursor(bitmap.size(), QImage::Format_ARGB32);
    QImage bmi = bitmap.toImage().convertToFormat(QImage::Format_RGB32);
    QImage bmmi = mask.toImage().convertToFormat(QImage::Format_RGB32);
    for (int row = 0; row < finalCursor.height(); ++row) {
        QRgb *bmData = reinterpret_cast<QRgb *>(bmi.scanLine(row));
        QRgb *bmmData = reinterpret_cast<QRgb *>(bmmi.scanLine(row));
        QRgb *finalData = reinterpret_cast<QRgb *>(finalCursor.scanLine(row));
        for (int col = 0; col < finalCursor.width(); ++col) {
            if (bmmData[col] == 0xff000000 && bmData[col] == 0xffffffff) {
                finalData[col] = 0xffffffff;
            } else if (bmmData[col] == 0xff000000 && bmData[col] == 0xffffffff) {
                finalData[col] = 0x7f000000;
            } else if (bmmData[col] == 0xffffffff && bmData[col] == 0xffffffff) {
                finalData[col] = 0x00000000;
            } else {
                finalData[col] = 0xff000000;
            }
        }
    }

    return createCursorFromPixmap(QPixmap::fromImage(finalCursor), hotspot);
}

NSCursor *QCocoaCursor::createCursorFromPixmap(const QPixmap &pixmap, const QPoint hotspot)
{
    NSPoint hotSpot = NSMakePoint(hotspot.x(), hotspot.y());
    auto *image = [NSImage imageFromQImage:pixmap.toImage()];
    return [[NSCursor alloc] initWithImage:image hotSpot:hotSpot];
}

QT_END_NAMESPACE
