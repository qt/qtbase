/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qglobal.h>

#include <Carbon/Carbon.h>

#include "qnsview.h"
#include "qcocoawindow.h"
#include "qcocoahelpers.h"
#include "qcocoaautoreleasepool.h"
#include "qmultitouch_mac_p.h"
#include "qcocoadrag.h"

#include <QtGui/QWindowSystemInterface>
#include <QtGui/QTextFormat>
#include <QtCore/QDebug>

#ifdef QT_COCOA_ENABLE_ACCESSIBILITY_INSPECTOR
#include <accessibilityinspector.h>
#endif

static QTouchDevice *touchDevice = 0;

@interface NSEvent (Qt_Compile_Leopard_DeviceDelta)
  - (CGFloat)deviceDeltaX;
  - (CGFloat)deviceDeltaY;
  - (CGFloat)deviceDeltaZ;
@end

@implementation QNSView

- (id) init
{
    self = [super initWithFrame : NSMakeRect(0,0, 300,300)];
    if (self) {
        m_cgImage = 0;
        m_window = 0;
        m_buttons = Qt::NoButton;
        currentCustomDragTypes = 0;
        if (!touchDevice) {
            touchDevice = new QTouchDevice;
            touchDevice->setType(QTouchDevice::TouchPad);
            touchDevice->setCapabilities(QTouchDevice::Position | QTouchDevice::NormalizedPosition);
            QWindowSystemInterface::registerTouchDevice(touchDevice);
        }
    }
    return self;
}

- (id)initWithQWindow:(QWindow *)window platformWindow:(QCocoaWindow *) platformWindow
{
    self = [self init];
    if (!self)
        return 0;

    m_window = window;
    m_platformWindow = platformWindow;
    m_accessibleRoot = 0;
    m_keyEventsAccepted = false;

#ifdef QT_COCOA_ENABLE_ACCESSIBILITY_INSPECTOR
    // prevent rift in space-time continuum, disable
    // accessibility for the accessibility inspector's windows.
    static bool skipAccessibilityForInspectorWindows = false;
    if (!skipAccessibilityForInspectorWindows) {

        m_accessibleRoot = window->accessibleRoot();

        AccessibilityInspector *inspector = new AccessibilityInspector(window);
        skipAccessibilityForInspectorWindows = true;
        inspector->inspectWindow(window);
        skipAccessibilityForInspectorWindows = false;
    }
#else
    m_accessibleRoot = window->accessibleRoot();
#endif

    [self registerDragTypes];
    [self setPostsFrameChangedNotifications : YES];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                          selector:@selector(updateGeometry)
                                          name:NSViewFrameDidChangeNotification
                                          object:self];

    return self;
}

- (void)updateGeometry
{
    NSRect rect = [self frame];
    NSRect windowRect = [[self window] frame];
    QRect geo(windowRect.origin.x, qt_mac_flipYCoordinate(windowRect.origin.y + rect.size.height), rect.size.width, rect.size.height);

#ifdef QT_COCOA_ENABLE_WINDOW_DEBUG
    qDebug() << "QNSView::udpateGeometry" << geo;
#endif

    // Call setGeometry on QPlatformWindow. (not on QCocoaWindow,
    // doing that will initiate a geometry change it and possibly create
    // an infinite loop when this notification is triggered again.)
    m_platformWindow->QPlatformWindow::setGeometry(geo);

    // Send a geometry change event to Qt, if it's ready to handle events
    if (!m_platformWindow->m_inConstructor)
        QWindowSystemInterface::handleSynchronousGeometryChange(m_window, geo);
}

- (void)windowDidBecomeKey
{
//    QWindowSystemInterface::handleWindowActivated(m_window);
}

- (void)windowDidResignKey
{
//    QWindowSystemInterface::handleWindowActivated(0);
}

- (void)windowDidBecomeMain
{
//    qDebug() << "window did become main" << m_window;
    QWindowSystemInterface::handleWindowActivated(m_window);
}

- (void)windowDidResignMain
{
//    qDebug() << "window did resign main" << m_window;
    QWindowSystemInterface::handleWindowActivated(0);
}


- (void) setImage:(QImage *)image
{
    CGImageRelease(m_cgImage);

    const uchar *imageData = image->bits();
    int bitDepth = image->depth();
    int colorBufferSize = 8;
    int bytesPrLine = image->bytesPerLine();
    int width = image->width();
    int height = image->height();

    CGColorSpaceRef cgColourSpaceRef = CGColorSpaceCreateDeviceRGB();

    CGDataProviderRef cgDataProviderRef = CGDataProviderCreateWithData(
                NULL,
                imageData,
                image->byteCount(),
                NULL);

    m_cgImage = CGImageCreate(width,
                              height,
                              colorBufferSize,
                              bitDepth,
                              bytesPrLine,
                              cgColourSpaceRef,
                              kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst,
                              cgDataProviderRef,
                              NULL,
                              false,
                              kCGRenderingIntentDefault);

    CGColorSpaceRelease(cgColourSpaceRef);

}

- (void) drawRect:(NSRect)dirtyRect
{
    if (!m_cgImage)
        return;

    CGRect dirtyCGRect = NSRectToCGRect(dirtyRect);

    NSGraphicsContext *nsGraphicsContext = [NSGraphicsContext currentContext];
    CGContextRef cgContext = (CGContextRef) [nsGraphicsContext graphicsPort];

    CGContextSaveGState( cgContext );
    int dy = dirtyCGRect.origin.y + CGRectGetMaxY(dirtyCGRect);
    CGContextTranslateCTM(cgContext, 0, dy);
    CGContextScaleCTM(cgContext, 1, -1);

    CGImageRef subImage = CGImageCreateWithImageInRect(m_cgImage, dirtyCGRect);
    CGContextDrawImage(cgContext,dirtyCGRect,subImage);

    CGContextRestoreGState(cgContext);

    CGImageRelease(subImage);

}

- (BOOL) isFlipped
{
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
    return YES;
}

- (void)handleMouseEvent:(NSEvent *)theEvent
{
    // Calculate the mouse position in the QWindow and Qt screen coordinate system,
    // starting from coordinates in the NSWindow coordinate system.
    //
    // This involves translating according to the window location on screen,
    // as well as inverting the y coordinate due to the origin change.
    //
    // Coordinate system overview, outer to innermost:
    //
    // Name             Origin
    //
    // OS X screen      bottom-left
    // Qt screen        top-left
    // NSWindow         bottom-left
    // NSView/QWindow   top-left
    //
    // NSView and QWindow are equal coordinate systems: the QWindow covers the
    // entire NSView, and we've set the NSView's isFlipped property to true.

    NSPoint nsWindowPoint = [theEvent locationInWindow];                    // NSWindow coordinates

    NSPoint nsViewPoint = [self convertPoint: nsWindowPoint fromView: nil]; // NSView/QWindow coordinates
    QPoint qtWindowPoint(nsViewPoint.x, nsViewPoint.y);                     // NSView/QWindow coordinates

    QPoint qtScreenPoint;

    NSWindow *window = [self window];
    // Use convertRectToScreen if available (added in 10.7).
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
    if ([window respondsToSelector:@selector(convertRectToScreen:)]) {
        NSRect screenRect = [window convertRectToScreen : NSMakeRect(nsWindowPoint.x, nsWindowPoint.y, 0, 0)]; // OS X screen coordinates
        qtScreenPoint = QPoint(screenRect.origin.x, qt_mac_flipYCoordinate(screenRect.origin.y));              // Qt screen coordinates
    } else
#endif
    {
        NSPoint screenPoint = [window convertBaseToScreen : NSMakePoint(nsWindowPoint.x, nsWindowPoint.y)];
        qtScreenPoint = QPoint(screenPoint.x, qt_mac_flipYCoordinate(screenPoint.y));
    }
    ulong timestamp = [theEvent timestamp] * 1000;
    QWindowSystemInterface::handleMouseEvent(m_window, timestamp, qtWindowPoint, qtScreenPoint, m_buttons);
}

- (void)mouseDown:(NSEvent *)theEvent
{
    if (m_platformWindow->m_activePopupWindow) {
        QWindowSystemInterface::handleSynchronousCloseEvent(m_platformWindow->m_activePopupWindow);
        m_platformWindow->m_activePopupWindow = 0;
    }
    if ([self hasMarkedText]) {
        NSInputManager* inputManager = [NSInputManager currentInputManager];
        if ([inputManager wantsToHandleMouseEvents]) {
            [inputManager handleMouseEvent:theEvent];
        }
    } else {
        m_buttons |= Qt::LeftButton;
        [self handleMouseEvent:theEvent];
    }
}

- (void)mouseDragged:(NSEvent *)theEvent
{
    if (!(m_buttons & Qt::LeftButton))
        qWarning("Internal Mousebutton tracking invalid(missing Qt::LeftButton");
    [self handleMouseEvent:theEvent];
}

- (void)mouseUp:(NSEvent *)theEvent
{
    m_buttons &= QFlag(~int(Qt::LeftButton));
    [self handleMouseEvent:theEvent];
}

- (void)mouseMoved:(NSEvent *)theEvent
{
    [self handleMouseEvent:theEvent];
}

- (void)mouseEntered:(NSEvent *)theEvent
{
    Q_UNUSED(theEvent);
    QWindowSystemInterface::handleEnterEvent(m_window);
}

- (void)mouseExited:(NSEvent *)theEvent
{
    Q_UNUSED(theEvent);
    QWindowSystemInterface::handleLeaveEvent(m_window);
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
    m_buttons |= Qt::RightButton;
    [self handleMouseEvent:theEvent];
}

- (void)rightMouseDragged:(NSEvent *)theEvent
{
    if (!(m_buttons & Qt::LeftButton))
        qWarning("Internal Mousebutton tracking invalid(missing Qt::LeftButton");
    [self handleMouseEvent:theEvent];
}

- (void)rightMouseUp:(NSEvent *)theEvent
{
    m_buttons &= QFlag(~int(Qt::RightButton));
    [self handleMouseEvent:theEvent];
}

- (void)otherMouseDown:(NSEvent *)theEvent
{
    switch ([theEvent buttonNumber]) {
        case 3:
            m_buttons |= Qt::MiddleButton;
            break;
        case 4:
            m_buttons |= Qt::ExtraButton1;  // AKA Qt::BackButton
            break;
        case 5:
            m_buttons |= Qt::ExtraButton2;  // AKA Qt::ForwardButton
            break;
        case 6:
            m_buttons |= Qt::ExtraButton3;
            break;
        case 7:
            m_buttons |= Qt::ExtraButton4;
            break;
        case 8:
            m_buttons |= Qt::ExtraButton5;
            break;
        case 9:
            m_buttons |= Qt::ExtraButton6;
            break;
        case 10:
            m_buttons |= Qt::ExtraButton7;
            break;
        case 11:
            m_buttons |= Qt::ExtraButton8;
            break;
        case 12:
            m_buttons |= Qt::ExtraButton9;
            break;
        case 13:
            m_buttons |= Qt::ExtraButton10;
            break;
        case 14:
            m_buttons |= Qt::ExtraButton11;
            break;
        case 15:
            m_buttons |= Qt::ExtraButton12;
            break;
        case 16:
            m_buttons |= Qt::ExtraButton13;
            break;
        default:
            m_buttons |= Qt::MiddleButton;
            break;
    }
    [self handleMouseEvent:theEvent];
}

- (void)otherMouseDragged:(NSEvent *)theEvent
{
    if (!(m_buttons & Qt::LeftButton))
        qWarning("Internal Mousebutton tracking invalid(missing Qt::LeftButton");
    [self handleMouseEvent:theEvent];
}

- (void)otherMouseUp:(NSEvent *)theEvent
{
    switch ([theEvent buttonNumber]) {
        case 3:
            m_buttons &= QFlag(~int(Qt::MiddleButton));
            break;
        case 4:
            m_buttons &= QFlag(~int(Qt::ExtraButton1));  // AKA Qt::BackButton
            break;
        case 5:
            m_buttons &= QFlag(~int(Qt::ExtraButton2));  // AKA Qt::ForwardButton
            break;
        case 6:
            m_buttons &= QFlag(~int(Qt::ExtraButton3));
            break;
        case 7:
            m_buttons &= QFlag(~int(Qt::ExtraButton4));
            break;
        case 8:
            m_buttons &= QFlag(~int(Qt::ExtraButton5));
            break;
        case 9:
            m_buttons &= QFlag(~int(Qt::ExtraButton6));
            break;
        case 10:
            m_buttons &= QFlag(~int(Qt::ExtraButton7));
            break;
        case 11:
            m_buttons &= QFlag(~int(Qt::ExtraButton8));
            break;
        case 12:
            m_buttons &= QFlag(~int(Qt::ExtraButton9));
            break;
        case 13:
            m_buttons &= QFlag(~int(Qt::ExtraButton10));
            break;
        case 14:
            m_buttons &= QFlag(~int(Qt::ExtraButton11));
            break;
        case 15:
            m_buttons &= QFlag(~int(Qt::ExtraButton12));
            break;
        case 16:
            m_buttons &= QFlag(~int(Qt::ExtraButton13));
            break;
        default:
            m_buttons &= QFlag(~int(Qt::MiddleButton));
            break;
    }
    [self handleMouseEvent:theEvent];
}

- (void)touchesBeganWithEvent:(NSEvent *)event
{
    const NSTimeInterval timestamp = [event timestamp];
    const QList<QWindowSystemInterface::TouchPoint> points = QCocoaTouch::getCurrentTouchPointList(event, /*acceptSingleTouch= ### true or false?*/false);
    QWindowSystemInterface::handleTouchEvent(m_window, timestamp * 1000, touchDevice, points);
}

- (void)touchesMovedWithEvent:(NSEvent *)event
{
    const NSTimeInterval timestamp = [event timestamp];
    const QList<QWindowSystemInterface::TouchPoint> points = QCocoaTouch::getCurrentTouchPointList(event, /*acceptSingleTouch= ### true or false?*/false);
    QWindowSystemInterface::handleTouchEvent(m_window, timestamp * 1000, touchDevice, points);
}

- (void)touchesEndedWithEvent:(NSEvent *)event
{
    const NSTimeInterval timestamp = [event timestamp];
    const QList<QWindowSystemInterface::TouchPoint> points = QCocoaTouch::getCurrentTouchPointList(event, /*acceptSingleTouch= ### true or false?*/false);
    QWindowSystemInterface::handleTouchEvent(m_window, timestamp * 1000, touchDevice, points);
}

- (void)touchesCancelledWithEvent:(NSEvent *)event
{
    const NSTimeInterval timestamp = [event timestamp];
    const QList<QWindowSystemInterface::TouchPoint> points = QCocoaTouch::getCurrentTouchPointList(event, /*acceptSingleTouch= ### true or false?*/false);
    QWindowSystemInterface::handleTouchEvent(m_window, timestamp * 1000, touchDevice, points);
}

#ifndef QT_NO_WHEELEVENT
- (void)scrollWheel:(NSEvent *)theEvent
{
    const EventRef carbonEvent = (EventRef)[theEvent eventRef];
    const UInt32 carbonEventKind = carbonEvent ? ::GetEventKind(carbonEvent) : 0;
    const bool scrollEvent = carbonEventKind == kEventMouseScroll;

    QPoint angleDelta;
    if (scrollEvent) {
        // The mouse device contains pixel scroll wheel support (Mighty Mouse, Trackpad).
        // Since deviceDelta is delivered as pixels rather than degrees, we need to
        // convert from pixels to degrees in a sensible manner.
        // It looks like 1/4 degrees per pixel behaves most native.
        // (NB: Qt expects the unit for delta to be 8 per degree):
        const int pixelsToDegrees = 2; // 8 * 1/4

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
        if ([theEvent respondsToSelector:@selector(scrollingDeltaX)]) {
            angleDelta.setX([theEvent scrollingDeltaX] * pixelsToDegrees);
            angleDelta.setY([theEvent scrollingDeltaY] * pixelsToDegrees);
        } else
#endif
        {
            angleDelta.setX([theEvent deviceDeltaX] * pixelsToDegrees);
            angleDelta.setY([theEvent deviceDeltaY] * pixelsToDegrees);
        }

    } else {
        // carbonEventKind == kEventMouseWheelMoved
        // Remove acceleration, and use either -120 or 120 as delta:
        angleDelta.setX(qBound(-120, int([theEvent deltaX] * 10000), 120));
        angleDelta.setY(qBound(-120, int([theEvent deltaY] * 10000), 120));
    }

    QPoint pixelDelta;
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
    if ([theEvent respondsToSelector:@selector(scrollingDeltaX)]) {
        if ([theEvent hasPreciseScrollingDeltas]) {
            pixelDelta.setX([theEvent scrollingDeltaX]);
            pixelDelta.setY([theEvent scrollingDeltaY]);
        } else {
            // docs: "In the case of !hasPreciseScrollingDeltas, multiply the delta with the line width."
            // scrollingDeltaX seems to return a minimum value of 0.1 in this case, map that to two pixels.
            const CGFloat lineWithEstimate = 20.0;
            pixelDelta.setX([theEvent scrollingDeltaX] * lineWithEstimate);
            pixelDelta.setY([theEvent scrollingDeltaY] * lineWithEstimate);
        }
    }
#endif


    NSPoint windowPoint = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    QPoint qt_windowPoint(windowPoint.x, windowPoint.y);
    NSTimeInterval timestamp = [theEvent timestamp];
    ulong qt_timestamp = timestamp * 1000;

    QWindowSystemInterface::handleWheelEvent(m_window, qt_timestamp, qt_windowPoint, qt_windowPoint, pixelDelta, angleDelta);
}
#endif //QT_NO_WHEELEVENT

- (int) convertKeyCode : (QChar)keyChar
{
    return qt_mac_cocoaKey2QtKey(keyChar);
}

- (Qt::KeyboardModifiers) convertKeyModifiers : (ulong)modifierFlags
{
    Qt::KeyboardModifiers qtMods =Qt::NoModifier;
    if (modifierFlags &  NSShiftKeyMask)
        qtMods |= Qt::ShiftModifier;
    if (modifierFlags & NSControlKeyMask)
        qtMods |= Qt::MetaModifier;
    if (modifierFlags & NSAlternateKeyMask)
        qtMods |= Qt::AltModifier;
    if (modifierFlags & NSCommandKeyMask)
        qtMods |= Qt::ControlModifier;
    if (modifierFlags & NSNumericPadKeyMask)
        qtMods |= Qt::KeypadModifier;
    return qtMods;
}

- (void)handleKeyEvent:(NSEvent *)theEvent eventType:(int)eventType
{
    NSTimeInterval timestamp = [theEvent timestamp];
    ulong qt_timestamp = timestamp * 1000;
    QString characters = QString::fromUtf8([[theEvent characters] UTF8String]);
    Qt::KeyboardModifiers modifiers = [self convertKeyModifiers : [theEvent modifierFlags]];
    QChar ch([[theEvent charactersIgnoringModifiers] characterAtIndex:0]);
    int keyCode = [self convertKeyCode : ch];

    QWindowSystemInterface::handleKeyEvent(m_window, qt_timestamp, QEvent::Type(eventType), keyCode, modifiers, characters);
}

- (void)keyDown:(NSEvent *)theEvent
{
    QObject *fo = QGuiApplication::focusObject();
    m_keyEventsAccepted = false;
    if (fo) {
        QInputMethodQueryEvent queryEvent(Qt::ImHints);
        if (QCoreApplication::sendEvent(fo, &queryEvent)) {
            Qt::InputMethodHints hints = static_cast<Qt::InputMethodHints>(queryEvent.value(Qt::ImHints).toUInt());
            if (!(hints & Qt::ImhDigitsOnly || hints & Qt::ImhFormattedNumbersOnly || hints & Qt::ImhHiddenText)) {
                [self interpretKeyEvents:[NSArray arrayWithObject: theEvent]];
            }
        }
    }

    if (!m_keyEventsAccepted && m_composingText.isEmpty()) {
        [self handleKeyEvent : theEvent eventType :int(QEvent::KeyPress)];
    }
}

- (void)keyUp:(NSEvent *)theEvent
{
    if (!m_keyEventsAccepted && m_composingText.isEmpty()) {
        [self handleKeyEvent : theEvent eventType :int(QEvent::KeyRelease)];
    }
}

- (void) doCommandBySelector:(SEL)aSelector
{
    [self tryToPerform:aSelector with:self];
}

- (void) insertText:(id)aString
{
    QString commitString;
    if ([aString length]) {
        if ([aString isKindOfClass:[NSAttributedString class]]) {
            commitString = QCFString::toQString(reinterpret_cast<CFStringRef>([aString string]));
        } else {
            commitString = QCFString::toQString(reinterpret_cast<CFStringRef>(aString));
        };
    }
    QObject *fo = QGuiApplication::focusObject();
    if (fo) {
        QInputMethodEvent e;
        e.setCommitString(commitString);
        QCoreApplication::sendEvent(fo, &e);
        m_keyEventsAccepted = true;
    }

    m_composingText.clear();
 }

- (void) setMarkedText:(id)aString selectedRange:(NSRange)selRange
{
    QString preeditString;

    QList<QInputMethodEvent::Attribute> attrs;
    attrs<<QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, selRange.location + selRange.length, 1, QVariant());

    if ([aString isKindOfClass:[NSAttributedString class]]) {
        // Preedit string has attribution
        preeditString = QCFString::toQString(reinterpret_cast<CFStringRef>([aString string]));
        int composingLength = preeditString.length();
        int index = 0;
        // Create attributes for individual sections of preedit text
        while (index < composingLength) {
            NSRange effectiveRange;
            NSRange range = NSMakeRange(index, composingLength-index);
            NSDictionary *attributes = [aString attributesAtIndex:index
                                            longestEffectiveRange:&effectiveRange
                                                          inRange:range];
            NSNumber *underlineStyle = [attributes objectForKey:NSUnderlineStyleAttributeName];
            if (underlineStyle) {
                QColor clr (Qt::black);
                NSColor *color = [attributes objectForKey:NSUnderlineColorAttributeName];
                if (color) {
                    clr = qt_mac_toQColor(color);
                }
                QTextCharFormat format;
                format.setFontUnderline(true);
                format.setUnderlineColor(clr);
                attrs<<QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat,
                                                    effectiveRange.location,
                                                    effectiveRange.length,
                                                    format);
            }
            index = effectiveRange.location + effectiveRange.length;
        }
    } else {
        // No attributes specified, take only the preedit text.
        preeditString = QCFString::toQString(reinterpret_cast<CFStringRef>(aString));
    }

    if (attrs.isEmpty()) {
        QTextCharFormat format;
        format.setFontUnderline(true);
        attrs<<QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat,
                                            0, preeditString.length(), format);
    }

    m_composingText = preeditString;

    QObject *fo = QGuiApplication::focusObject();
    if (fo) {
        QInputMethodEvent e(preeditString, attrs);
        QCoreApplication::sendEvent(fo, &e);
        m_keyEventsAccepted = true;
    }
}

- (void) unmarkText
{
    if (!m_composingText.isEmpty()) {
        QObject *fo = QGuiApplication::focusObject();
        if (fo) {
            QInputMethodEvent e;
            e.setCommitString(m_composingText);
            QCoreApplication::sendEvent(fo, &e);
        }
    }
    m_composingText.clear();
}

- (BOOL) hasMarkedText
{
    return (m_composingText.isEmpty() ? NO: YES);
}

- (NSInteger) conversationIdentifier
{
    return (NSInteger)self;
}

- (NSAttributedString *) attributedSubstringFromRange:(NSRange)theRange
{
    QObject *fo = QGuiApplication::focusObject();
    if (!fo)
        return nil;
    QInputMethodQueryEvent queryEvent(Qt::ImCurrentSelection);
    if (!QCoreApplication::sendEvent(fo, &queryEvent))
        return nil;

    QString selectedText = queryEvent.value(Qt::ImCurrentSelection).toString();
    if (selectedText.isEmpty())
        return nil;

    QCFString string(selectedText.mid(theRange.location, theRange.length));
    const NSString *tmpString = reinterpret_cast<const NSString *>((CFStringRef)string);
    return [[[NSAttributedString alloc]  initWithString:const_cast<NSString *>(tmpString)] autorelease];
}

- (NSRange) markedRange
{
    NSRange range;
    if (!m_composingText.isEmpty()) {
        range.location = 0;
        range.length = m_composingText.length();
    } else {
        range.location = NSNotFound;
        range.length = 0;
    }
    return range;
}


- (NSRange) selectedRange
{
    NSRange selRange = {NSNotFound, 0};
    selRange.location = NSNotFound;
    selRange.length = 0;

    QObject *fo = QGuiApplication::focusObject();
    if (!fo)
        return selRange;
    QInputMethodQueryEvent queryEvent(Qt::ImCurrentSelection);
    if (!QCoreApplication::sendEvent(fo, &queryEvent))
        return selRange;
    QString selectedText = queryEvent.value(Qt::ImCurrentSelection).toString();

    if (!selectedText.isEmpty()) {
        selRange.location = 0;
        selRange.length = selectedText.length();
    }
    return selRange;
}

- (NSRect) firstRectForCharacterRange:(NSRange)theRange
{
    Q_UNUSED(theRange);
    QObject *fo = QGuiApplication::focusObject();
    if (!fo)
        return NSZeroRect;

    if (!m_window)
        return NSZeroRect;

    // The returned rect is always based on the internal cursor.
    QRect mr = qApp->inputMethod()->cursorRectangle().toRect();
    QPoint mp = m_window->mapToGlobal(mr.bottomLeft());

    NSRect rect;
    rect.origin.x = mp.x();
    rect.origin.y = qt_mac_flipYCoordinate(mp.y());
    rect.size.width = mr.width();
    rect.size.height = mr.height();
    return rect;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)thePoint
{
    // We dont support cursor movements using mouse while composing.
    Q_UNUSED(thePoint);
    return NSNotFound;
}

- (NSArray*) validAttributesForMarkedText
{
    QObject *fo = QGuiApplication::focusObject();
    if (!fo)
        return nil;

    // Support only underline color/style.
    return [NSArray arrayWithObjects:NSUnderlineColorAttributeName,
                                     NSUnderlineStyleAttributeName, nil];
}

-(void)registerDragTypes
{
    QCocoaAutoReleasePool pool;
    // ### Custom types disabled.
    QStringList customTypes;  // = qEnabledDraggedTypes();
    if (currentCustomDragTypes == 0 || *currentCustomDragTypes != customTypes) {
        if (currentCustomDragTypes == 0)
            currentCustomDragTypes = new QStringList();
        *currentCustomDragTypes = customTypes;
        const NSString* mimeTypeGeneric = @"com.trolltech.qt.MimeTypeName";
        NSMutableArray *supportedTypes = [NSMutableArray arrayWithObjects:NSColorPboardType,
                       NSFilenamesPboardType, NSStringPboardType,
                       NSFilenamesPboardType, NSPostScriptPboardType, NSTIFFPboardType,
                       NSRTFPboardType, NSTabularTextPboardType, NSFontPboardType,
                       NSRulerPboardType, NSFileContentsPboardType, NSColorPboardType,
                       NSRTFDPboardType, NSHTMLPboardType, NSPICTPboardType,
                       NSURLPboardType, NSPDFPboardType, NSVCardPboardType,
                       NSFilesPromisePboardType, NSInkTextPboardType,
                       NSMultipleTextSelectionPboardType, mimeTypeGeneric, nil];
        // Add custom types supported by the application.
        for (int i = 0; i < customTypes.size(); i++) {
           [supportedTypes addObject:QCFString::toNSString(customTypes[i])];
        }
        [self registerForDraggedTypes:supportedTypes];
    }
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender
{
    return [self handleDrag : sender];
}

- (NSDragOperation)draggingUpdated:(id <NSDraggingInfo>)sender
{
    return [self handleDrag : sender];
}

// Sends drag update to Qt, return the action
- (NSDragOperation)handleDrag:(id <NSDraggingInfo>)sender
{
    NSPoint windowPoint = [self convertPoint: [sender draggingLocation] fromView: nil];
    QPoint qt_windowPoint(windowPoint.x, windowPoint.y);
    Qt::DropActions qtAllowed = qt_mac_mapNSDragOperations([sender draggingSourceOperationMask]);
    QCocoaDropData mimeData([sender draggingPasteboard]);

    QPlatformDragQtResponse response = QWindowSystemInterface::handleDrag(m_window, &mimeData, qt_windowPoint, qtAllowed);
    return qt_mac_mapDropAction(response.acceptedAction());
}

- (void)draggingExited:(id <NSDraggingInfo>)sender
{
    NSPoint windowPoint = [self convertPoint: [sender draggingLocation] fromView: nil];
    QPoint qt_windowPoint(windowPoint.x, windowPoint.y);

    // Send 0 mime data to indicate drag exit
    QWindowSystemInterface::handleDrag(m_window, 0 ,qt_windowPoint, Qt::IgnoreAction);
}

// called on drop, send the drop to Qt and return if it was accepted.
- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
    NSPoint windowPoint = [self convertPoint: [sender draggingLocation] fromView: nil];
    QPoint qt_windowPoint(windowPoint.x, windowPoint.y);
    Qt::DropActions qtAllowed = qt_mac_mapNSDragOperations([sender draggingSourceOperationMask]);
    QCocoaDropData mimeData([sender draggingPasteboard]);

    QPlatformDropQtResponse response = QWindowSystemInterface::handleDrop(m_window, &mimeData, qt_windowPoint, qtAllowed);
    return response.isAccepted();
}

@end
