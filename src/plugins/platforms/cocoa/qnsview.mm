/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qglobal.h>

#include <Carbon/Carbon.h>

#include "qnsview.h"
#include "qcocoawindow.h"
#include "qcocoahelpers.h"
#include "qmultitouch_mac_p.h"

#include <QtGui/QWindowSystemInterface>
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

    [self setPostsFrameChangedNotifications : YES];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                          selector:@selector(frameDidChangeNotification:)
                                          name:NSViewFrameDidChangeNotification
                                          object:self];

    return self;
}

- (void) frameDidChangeNotification: (NSNotification *) notification
{
    NSRect rect = [self frame];
    NSRect windowRect = [[self window] frame];
    QRect geo(windowRect.origin.x, qt_mac_flipYCoordinate(windowRect.origin.y + rect.size.height), rect.size.width, rect.size.height);

    // Call setGeometry on QPlatformWindow. (not on QCocoaWindow,
    // doing that will initiate a geometry change it and possibly create
    // an infinite loop when this notification is triggered again.)
    m_platformWindow->QPlatformWindow::setGeometry(geo);

    // Send a geometry change event to Qt, if it's ready to handle events
    if (!m_platformWindow->m_inConstructor)
        QWindowSystemInterface::handleSynchronousGeometryChange(m_window, geo);
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
#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_6
    NSRect screenRect = [[self window] convertRectToScreen : NSMakeRect(nsWindowPoint.x, nsWindowPoint.y, 0, 0)]; // OS X screen coordinates
    qtScreenPoint = QPoint(screenRect.origin.x, qt_mac_flipYCoordinate(screenRect.origin.y));                     // Qt screen coordinates
#else
    NSPoint screenPoint = [[self window] convertBaseToScreen : NSMakePoint(nsWindowPoint.x, nsWindowPoint.y)];
    qtScreenPoint = QPoint(screenPoint.x, qt_mac_flipYCoordinate(screenPoint.y));
#endif

    ulong timestamp = [theEvent timestamp] * 1000;

    QWindowSystemInterface::handleMouseEvent(m_window, timestamp, qtWindowPoint, qtScreenPoint, m_buttons);
}

- (void)mouseDown:(NSEvent *)theEvent
{
    m_buttons |= Qt::LeftButton;
    [self handleMouseEvent:theEvent];
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
    m_buttons |= Qt::RightButton;
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
    m_buttons &= QFlag(~int(Qt::MiddleButton));
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
    int deltaX = 0;
    int deltaY = 0;
    int deltaZ = 0;

    const EventRef carbonEvent = (EventRef)[theEvent eventRef];
    const UInt32 carbonEventKind = carbonEvent ? ::GetEventKind(carbonEvent) : 0;
    const bool scrollEvent = carbonEventKind == kEventMouseScroll;

    if (scrollEvent) {
        // The mouse device contains pixel scroll wheel support (Mighty Mouse, Trackpad).
        // Since deviceDelta is delivered as pixels rather than degrees, we need to
        // convert from pixels to degrees in a sensible manner.
        // It looks like 1/4 degrees per pixel behaves most native.
        // (NB: Qt expects the unit for delta to be 8 per degree):
        const int pixelsToDegrees = 2; // 8 * 1/4

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
        if ([theEvent respondsToSelector:@selector(scrollingDeltaX)]) {
            deltaX = [theEvent scrollingDeltaX] * pixelsToDegrees;
            deltaY = [theEvent scrollingDeltaY] * pixelsToDegrees;
            //  scrollingDeltaZ API is missing.
        } else
#endif
        {
            deltaX = [theEvent deviceDeltaX] * pixelsToDegrees;
            deltaY = [theEvent deviceDeltaY] * pixelsToDegrees;
            deltaZ = [theEvent deviceDeltaZ] * pixelsToDegrees;
        }

    } else {
        // carbonEventKind == kEventMouseWheelMoved
        // Remove acceleration, and use either -120 or 120 as delta:
        deltaX = qBound(-120, int([theEvent deltaX] * 10000), 120);
        deltaY = qBound(-120, int([theEvent deltaY] * 10000), 120);
        deltaZ = qBound(-120, int([theEvent deltaZ] * 10000), 120);
    }

    NSPoint windowPoint = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    QPoint qt_windowPoint(windowPoint.x, windowPoint.y);
    NSTimeInterval timestamp = [theEvent timestamp];
    ulong qt_timestamp = timestamp * 1000;

    if (deltaX != 0)
        QWindowSystemInterface::handleWheelEvent(m_window, qt_timestamp, qt_windowPoint, qt_windowPoint, deltaX, Qt::Horizontal);

    if (deltaY != 0)
        QWindowSystemInterface::handleWheelEvent(m_window, qt_timestamp, qt_windowPoint, qt_windowPoint, deltaY, Qt::Vertical);

    if (deltaZ != 0)
        // Qt doesn't explicitly support wheels with a Z component. In a misguided attempt to
        // try to be ahead of the pack, I'm adding this extra value.
        QWindowSystemInterface::handleWheelEvent(m_window, qt_timestamp, qt_windowPoint, qt_windowPoint, deltaY, (Qt::Orientation)3);
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
    [self handleKeyEvent : theEvent eventType :int(QEvent::KeyPress)];
}

- (void)keyUp:(NSEvent *)theEvent
{
    [self handleKeyEvent : theEvent eventType :int(QEvent::KeyRelease)];
}

@end
