/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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
#include "qmacmime.h"
#include <qpa/qplatformintegration.h>

#include <qpa/qwindowsysteminterface.h>
#include <QtGui/QTextFormat>
#include <QtCore/QDebug>
#include <private/qguiapplication_p.h>
#include "qcocoabackingstore.h"
#include "qcocoaglcontext.h"
#include "qcocoaintegration.h"

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
        m_backingStore = 0;
        m_maskImage = 0;
        m_maskData = 0;
        m_shouldInvalidateWindowShadow = false;
        m_window = 0;
        m_buttons = Qt::NoButton;
        m_sendKeyEvent = false;
        m_subscribesForGlobalFrameNotifications = false;
        m_glContext = 0;
        m_shouldSetGLContextinDrawRect = false;
        currentCustomDragTypes = 0;
        m_sendUpAsRightButton = false;

        if (!touchDevice) {
            touchDevice = new QTouchDevice;
            touchDevice->setType(QTouchDevice::TouchPad);
            touchDevice->setCapabilities(QTouchDevice::Position | QTouchDevice::NormalizedPosition);
            QWindowSystemInterface::registerTouchDevice(touchDevice);
        }
    }
    return self;
}

- (void)dealloc
{
    CGImageRelease(m_maskImage);
    m_maskImage = 0;
    m_maskData = 0;
    m_window = 0;
    if (m_subscribesForGlobalFrameNotifications) {
        m_subscribesForGlobalFrameNotifications = false;
        [[NSNotificationCenter defaultCenter] removeObserver:self
             name:NSViewGlobalFrameDidChangeNotification
             object:self];
}
    delete currentCustomDragTypes;

    [super dealloc];
}

- (id)initWithQWindow:(QWindow *)window platformWindow:(QCocoaWindow *) platformWindow
{
    self = [self init];
    if (!self)
        return 0;

    m_window = window;
    m_platformWindow = platformWindow;
    m_sendKeyEvent = false;

#ifdef QT_COCOA_ENABLE_ACCESSIBILITY_INSPECTOR
    // prevent rift in space-time continuum, disable
    // accessibility for the accessibility inspector's windows.
    static bool skipAccessibilityForInspectorWindows = false;
    if (!skipAccessibilityForInspectorWindows) {

        // m_accessibleRoot = window->accessibleRoot();

        AccessibilityInspector *inspector = new AccessibilityInspector(window);
        skipAccessibilityForInspectorWindows = true;
        inspector->inspectWindow(window);
        skipAccessibilityForInspectorWindows = false;
    }
#endif

    [self registerDragTypes];
    [self setPostsFrameChangedNotifications : YES];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                          selector:@selector(updateGeometry)
                                          name:NSViewFrameDidChangeNotification
                                          object:self];

    return self;
}

- (void) setQCocoaGLContext:(QCocoaGLContext *)context
{
    m_glContext = context;
    [m_glContext->nsOpenGLContext() setView:self];
    if (![m_glContext->nsOpenGLContext() view]) {
        //was unable to set view
        m_shouldSetGLContextinDrawRect = true;
    }

    if (!m_subscribesForGlobalFrameNotifications) {
        // NSOpenGLContext expects us to repaint (or update) the view when
        // it changes position on screen. Since this happens unnoticed for
        // the view when the parent view moves, we need to register a special
        // notification that lets us handle this case:
        m_subscribesForGlobalFrameNotifications = true;
        [[NSNotificationCenter defaultCenter] addObserver:self
            selector:@selector(globalFrameChanged:)
            name:NSViewGlobalFrameDidChangeNotification
            object:self];
    }
}

- (void) globalFrameChanged:(NSNotification*)notification
{
    Q_UNUSED(notification);
    QWindowSystemInterface::handleExposeEvent(m_window, m_window->geometry());
}

- (void)viewDidMoveToSuperview
{
    if (!(m_platformWindow->m_contentViewIsToBeEmbedded))
        return;

    if ([self superview]) {
        m_platformWindow->m_contentViewIsEmbedded = true;
        QWindowSystemInterface::handleGeometryChange(m_window, m_platformWindow->geometry());
        QWindowSystemInterface::handleExposeEvent(m_window, m_platformWindow->geometry());
        QWindowSystemInterface::flushWindowSystemEvents();
    } else {
        m_platformWindow->m_contentViewIsEmbedded = false;
    }
}

- (void)viewWillMoveToWindow:(NSWindow *)newWindow
{
    // ### Merge "normal" window code path with this one for 5.1.
    if (!(m_window->type() & Qt::SubWindow))
        return;

    if (newWindow) {
        [[NSNotificationCenter defaultCenter] addObserver:self
                                              selector:@selector(windowNotification:)
                                              name:nil // Get all notifications
                                              object:newWindow];
    } else {
        [[NSNotificationCenter defaultCenter] removeObserver:self name:nil object:[self window]];
    }
}
- (void)updateGeometry
{
    QRect geometry;
    if (m_platformWindow->m_nsWindow) {
        // top level window, get window rect and flip y.
        NSRect rect = [self frame];
        NSRect windowRect = [[self window] frame];
        geometry = QRect(windowRect.origin.x, qt_mac_flipYCoordinate(windowRect.origin.y + rect.size.height), rect.size.width, rect.size.height);
    } else if (m_platformWindow->m_contentViewIsToBeEmbedded) {
        // embedded child window, use the frame rect ### merge with case below
        geometry = qt_mac_toQRect([self bounds]);
    } else {
        // child window, use the frame rect
        geometry = qt_mac_toQRect([self frame]);
    }

    if (m_platformWindow->m_nsWindow && geometry == m_platformWindow->geometry())
        return;

#ifdef QT_COCOA_ENABLE_WINDOW_DEBUG
    qDebug() << "QNSView::udpateGeometry" << m_platformWindow << geometry;
#endif

    // Call setGeometry on QPlatformWindow. (not on QCocoaWindow,
    // doing that will initiate a geometry change it and possibly create
    // an infinite loop when this notification is triggered again.)
    m_platformWindow->QPlatformWindow::setGeometry(geometry);

    // Don't send the geometry change if the QWindow is designated to be
    // embedded in a foreign view hiearchy but has not actually been
    // embedded yet - it's too early.
    if (m_platformWindow->m_contentViewIsToBeEmbedded && !m_platformWindow->m_contentViewIsEmbedded)
        return;

    // Send a geometry change event to Qt, if it's ready to handle events
    if (!m_platformWindow->m_inConstructor) {
        QWindowSystemInterface::handleGeometryChange(m_window, geometry);
        QWindowSystemInterface::handleExposeEvent(m_window, geometry);
        QWindowSystemInterface::flushWindowSystemEvents();
    }
}

- (void)notifyWindowStateChanged:(Qt::WindowState)newState
{
    QWindowSystemInterface::handleWindowStateChanged(m_window, newState);
    // We want to read the window state back from the window,
    // but the event we just sent may be asynchronous.
    QWindowSystemInterface::flushWindowSystemEvents();
    m_platformWindow->setSynchedWindowStateFromWindow();
}

- (void)windowNotification : (NSNotification *) windowNotification
{
    //qDebug() << "windowNotification" << QCFString::toQString([windowNotification name]);

    NSString *notificationName = [windowNotification name];
    if (notificationName == NSWindowDidBecomeKeyNotification) {
        if (!m_platformWindow->windowIsPopupType())
            QWindowSystemInterface::handleWindowActivated(m_window);
    } else if (notificationName == NSWindowDidResignKeyNotification) {
        // key window will be non-nil if another window became key... do not
        // set the active window to zero here, the new key window's
        // NSWindowDidBecomeKeyNotification hander will change the active window
        NSWindow *keyWindow = [NSApp keyWindow];
        if (!keyWindow) {
            // no new key window, go ahead and set the active window to zero
            if (!m_platformWindow->windowIsPopupType())
                QWindowSystemInterface::handleWindowActivated(0);
        }
    } else if (notificationName == NSWindowDidMiniaturizeNotification
               || notificationName == NSWindowDidDeminiaturizeNotification) {
        Qt::WindowState newState = notificationName == NSWindowDidMiniaturizeNotification ?
                    Qt::WindowMinimized : Qt::WindowNoState;
        [self notifyWindowStateChanged:newState];
    } else if ([notificationName isEqualToString: @"NSWindowDidOrderOffScreenNotification"]) {
        m_platformWindow->obscureWindow();
    } else if ([notificationName isEqualToString: @"NSWindowDidOrderOnScreenAndFinishAnimatingNotification"]) {
        m_platformWindow->exposeWindow();
    } else if (notificationName == NSWindowDidChangeScreenNotification) {
        if (m_window) {
            QCocoaIntegration *ci = static_cast<QCocoaIntegration *>(QGuiApplicationPrivate::platformIntegration());
            NSUInteger screenIndex = [[NSScreen screens] indexOfObject:self.window.screen];
            if (screenIndex != NSNotFound) {
                QCocoaScreen *cocoaScreen = ci->screenAtIndex(screenIndex);
                QWindowSystemInterface::handleWindowScreenChanged(m_window, cocoaScreen->screen());
            }
        }
    } else {

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
    if (QSysInfo::QSysInfo::MacintoshVersion >= QSysInfo::MV_10_7) {
        if (notificationName == NSWindowDidEnterFullScreenNotification
            || notificationName == NSWindowDidExitFullScreenNotification) {
            Qt::WindowState newState = notificationName == NSWindowDidEnterFullScreenNotification ?
                        Qt::WindowFullScreen : Qt::WindowNoState;
            [self notifyWindowStateChanged:newState];
        }
    }
#endif

    }
}

- (void)notifyWindowWillZoom:(BOOL)willZoom
{
    Qt::WindowState newState = willZoom ? Qt::WindowMaximized : Qt::WindowNoState;
    [self notifyWindowStateChanged:newState];
}

- (void)viewDidHide
{
    m_platformWindow->obscureWindow();
}

- (void)viewDidUnhide
{
    m_platformWindow->exposeWindow();
}

- (void) flushBackingStore:(QCocoaBackingStore *)backingStore region:(const QRegion &)region offset:(QPoint)offset
{
    m_backingStore = backingStore;
    m_backingStoreOffset = offset * m_backingStore->getBackingStoreDevicePixelRatio();
    QRect br = region.boundingRect();
    [self setNeedsDisplayInRect:NSMakeRect(br.x(), br.y(), br.width(), br.height())];
}

- (BOOL) hasMask
{
    return m_maskData != 0;
}

- (BOOL) isOpaque
{
    return m_platformWindow->isOpaque();
}

- (void) setMaskRegion:(const QRegion *)region
{
    m_shouldInvalidateWindowShadow = true;
    if (m_maskImage)
        CGImageRelease(m_maskImage);
    if (region->isEmpty()) {
        m_maskImage = 0;
        return;
    }

    const QRect &rect = region->boundingRect();
    QImage tmp(rect.size(), QImage::Format_RGB32);
    tmp.fill(Qt::white);
    QPainter p(&tmp);
    p.setClipRegion(*region);
    p.fillRect(rect, Qt::black);
    p.end();
    QImage maskImage = QImage(rect.size(), QImage::Format_Indexed8);
    for (int y=0; y<rect.height(); ++y) {
        const uint *src = (const uint *) tmp.constScanLine(y);
        uchar *dst = maskImage.scanLine(y);
        for (int x=0; x<rect.width(); ++x) {
            dst[x] = src[x] & 0xff;
        }
    }
    m_maskImage = qt_mac_toCGImage(maskImage, true, &m_maskData);
}

- (void)invalidateWindowShadowIfNeeded
{
    if (m_shouldInvalidateWindowShadow && m_platformWindow->m_nsWindow) {
        [m_platformWindow->m_nsWindow invalidateShadow];
        m_shouldInvalidateWindowShadow = false;
    }
}

- (void) drawRect:(NSRect)dirtyRect
{
    if (m_glContext && m_shouldSetGLContextinDrawRect) {
        [m_glContext->nsOpenGLContext() setView:self];
        m_shouldSetGLContextinDrawRect = false;
    }

    if (!m_backingStore)
        return;

    // Calculate source and target rects. The target rect is the dirtyRect:
    CGRect dirtyWindowRect = NSRectToCGRect(dirtyRect);

    // The backing store source rect will be larger on retina displays.
    // Scale dirtyRect by the device pixel ratio:
    const qreal devicePixelRatio = m_backingStore->getBackingStoreDevicePixelRatio();
    CGRect dirtyBackingRect = CGRectMake(dirtyRect.origin.x * devicePixelRatio,
                                         dirtyRect.origin.y * devicePixelRatio,
                                         dirtyRect.size.width * devicePixelRatio,
                                         dirtyRect.size.height * devicePixelRatio);

    NSGraphicsContext *nsGraphicsContext = [NSGraphicsContext currentContext];
    CGContextRef cgContext = (CGContextRef) [nsGraphicsContext graphicsPort];

    // Translate coordiate system from CoreGraphics (bottom-left) to NSView (top-left):
    CGContextSaveGState(cgContext);
    int dy = dirtyWindowRect.origin.y + CGRectGetMaxY(dirtyWindowRect);

    CGContextTranslateCTM(cgContext, 0, dy);
    CGContextScaleCTM(cgContext, 1, -1);

    // If a mask is set, modify the sub image accordingly:
    CGImageRef subMask = 0;
    if (m_maskImage) {
        subMask = CGImageCreateWithImageInRect(m_maskImage, dirtyWindowRect);
        CGContextClipToMask(cgContext, dirtyWindowRect, subMask);
    }

    // Clip out and draw the correct sub image from the (shared) backingstore:
    CGRect backingStoreRect = CGRectMake(
        dirtyBackingRect.origin.x + m_backingStoreOffset.x(),
        dirtyBackingRect.origin.y + m_backingStoreOffset.y(),
        dirtyBackingRect.size.width,
        dirtyBackingRect.size.height
    );
    CGImageRef bsCGImage = m_backingStore->getBackingStoreCGImage();
    CGImageRef cleanImg = CGImageCreateWithImageInRect(bsCGImage, backingStoreRect);

    // Optimization: Copy frame buffer content instead of blending for
    // top-level windows where Qt fills the entire window content area.
    if (m_platformWindow->m_nsWindow)
        CGContextSetBlendMode(cgContext, kCGBlendModeCopy);

    CGContextDrawImage(cgContext, dirtyWindowRect, cleanImg);

    // Clean-up:
    CGContextRestoreGState(cgContext);
    CGImageRelease(cleanImg);
    CGImageRelease(subMask);

    [self invalidateWindowShadowIfNeeded];
}

- (BOOL) isFlipped
{
    return YES;
}

- (BOOL)becomeFirstResponder
{
    if (m_window->flags() & Qt::WindowTransparentForInput)
        return NO;
    QWindow *focusWindow = m_window;

    // For widgets we need to do a bit of trickery as the window
    // to activate is the window of the top-level widget.
    if (m_window->metaObject()->className() == QStringLiteral("QWidgetWindow")) {
        while (focusWindow->parent()) {
            focusWindow = focusWindow->parent();
        }
    }
    QWindowSystemInterface::handleWindowActivated(focusWindow);
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    if (m_window->flags() & Qt::WindowDoesNotAcceptFocus)
        return NO;
    if (m_window->flags() & Qt::WindowTransparentForInput)
        return NO;
    if ((m_window->flags() & Qt::ToolTip) == Qt::ToolTip)
        return NO;
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
    Q_UNUSED(theEvent)
    if (m_window->flags() & Qt::WindowTransparentForInput)
        return NO;
    return YES;
}

- (void)convertFromEvent:(NSEvent *)event toWindowPoint:(QPoint *)qtWindowPoint andScreenPoint:(QPoint *)qtScreenPoint
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

    NSPoint nsWindowPoint = [event locationInWindow];                    // NSWindow coordinates

    NSPoint nsViewPoint = [self convertPoint: nsWindowPoint fromView: nil]; // NSView/QWindow coordinates
    *qtWindowPoint = QPoint(nsViewPoint.x, nsViewPoint.y);                     // NSView/QWindow coordinates

    NSWindow *window = [self window];
    // Use convertRectToScreen if available (added in 10.7).
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
    if ([window respondsToSelector:@selector(convertRectToScreen:)]) {
        NSRect screenRect = [window convertRectToScreen : NSMakeRect(nsWindowPoint.x, nsWindowPoint.y, 0, 0)]; // OS X screen coordinates
        *qtScreenPoint = QPoint(screenRect.origin.x, qt_mac_flipYCoordinate(screenRect.origin.y));              // Qt screen coordinates
    } else
#endif
    {
        NSPoint screenPoint = [window convertBaseToScreen : NSMakePoint(nsWindowPoint.x, nsWindowPoint.y)];
        *qtScreenPoint = QPoint(screenPoint.x, qt_mac_flipYCoordinate(screenPoint.y));
    }
}

- (void)resetMouseButtons
{
    m_buttons = Qt::NoButton;
}

- (void)handleMouseEvent:(NSEvent *)theEvent
{
    QPoint qtWindowPoint, qtScreenPoint;
    [self convertFromEvent:theEvent toWindowPoint:&qtWindowPoint andScreenPoint:&qtScreenPoint];
    ulong timestamp = [theEvent timestamp] * 1000;

    QCocoaDrag* nativeDrag = static_cast<QCocoaDrag *>(QGuiApplicationPrivate::platformIntegration()->drag());
    nativeDrag->setLastMouseEvent(theEvent, self);

    Qt::KeyboardModifiers keyboardModifiers = [QNSView convertKeyModifiers:[theEvent modifierFlags]];
    QWindowSystemInterface::handleMouseEvent(m_window, timestamp, qtWindowPoint, qtScreenPoint, m_buttons, keyboardModifiers);
}

- (void)handleFrameStrutMouseEvent:(NSEvent *)theEvent
{
    // get m_buttons in sync
    NSEventType ty = [theEvent type];
    switch (ty) {
    case NSLeftMouseDown:
        m_buttons |= Qt::LeftButton;
        break;
    case NSLeftMouseUp:
         m_buttons &= ~Qt::LeftButton;
         break;
    case NSRightMouseDown:
        m_buttons |= Qt::RightButton;
        break;
    case NSRightMouseUp:
        m_buttons &= ~Qt::RightButton;
        break;
    case NSOtherMouseDown:
        m_buttons |= cocoaButton2QtButton([theEvent buttonNumber]);
        break;
    case NSOtherMouseUp:
        m_buttons &= ~cocoaButton2QtButton([theEvent buttonNumber]);
    default:
        break;
    }

    NSWindow *window = [self window];
    NSPoint windowPoint = [theEvent locationInWindow];

    int windowScreenY = [window frame].origin.y + [window frame].size.height;
    int viewScreenY = [window convertBaseToScreen:[self convertPoint:[self frame].origin toView:nil]].y;
    int titleBarHeight = windowScreenY - viewScreenY;

    NSPoint nsViewPoint = [self convertPoint: windowPoint fromView: nil];
    QPoint qtWindowPoint = QPoint(nsViewPoint.x, titleBarHeight + nsViewPoint.y);
    NSPoint screenPoint = [window convertBaseToScreen:windowPoint];
    QPoint qtScreenPoint = QPoint(screenPoint.x, qt_mac_flipYCoordinate(screenPoint.y));

    ulong timestamp = [theEvent timestamp] * 1000;
    QWindowSystemInterface::handleFrameStrutMouseEvent(m_window, timestamp, qtWindowPoint, qtScreenPoint, m_buttons);
}

- (void)mouseDown:(NSEvent *)theEvent
{
    if (m_window->flags() & Qt::WindowTransparentForInput)
        return [super mouseDown:theEvent];
    m_sendUpAsRightButton = false;
    if (m_platformWindow->m_activePopupWindow) {
        QWindowSystemInterface::handleCloseEvent(m_platformWindow->m_activePopupWindow);
        QWindowSystemInterface::flushWindowSystemEvents();
        m_platformWindow->m_activePopupWindow = 0;
    }
    if ([self hasMarkedText]) {
        NSInputManager* inputManager = [NSInputManager currentInputManager];
        if ([inputManager wantsToHandleMouseEvents]) {
            [inputManager handleMouseEvent:theEvent];
        }
    } else {
        if ([QNSView convertKeyModifiers:[theEvent modifierFlags]] & Qt::MetaModifier) {
            m_buttons |= Qt::RightButton;
            m_sendUpAsRightButton = true;
        } else {
            m_buttons |= Qt::LeftButton;
        }
        [self handleMouseEvent:theEvent];
    }
}

- (void)mouseDragged:(NSEvent *)theEvent
{
    if (m_window->flags() & Qt::WindowTransparentForInput)
        return [super mouseDragged:theEvent];
    if (!(m_buttons & Qt::LeftButton))
        qWarning("QNSView mouseDragged: Internal mouse button tracking invalid (missing Qt::LeftButton)");
    [self handleMouseEvent:theEvent];
}

- (void)mouseUp:(NSEvent *)theEvent
{
    if (m_window->flags() & Qt::WindowTransparentForInput)
        return [super mouseUp:theEvent];
    if (m_sendUpAsRightButton) {
        m_buttons &= ~Qt::RightButton;
        m_sendUpAsRightButton = false;
    } else {
        m_buttons &= ~Qt::LeftButton;
    }
    [self handleMouseEvent:theEvent];
}

- (void)updateTrackingAreas
{
    [super updateTrackingAreas];

    // [NSView addTrackingArea] is slow, so bail out early if we can:
    if (NSIsEmptyRect([self visibleRect]))
        return;

    // Remove current trakcing areas:
    QCocoaAutoReleasePool pool;
    if (NSArray *trackingArray = [self trackingAreas]) {
        NSUInteger size = [trackingArray count];
        for (NSUInteger i = 0; i < size; ++i) {
            NSTrackingArea *t = [trackingArray objectAtIndex:i];
            [self removeTrackingArea:t];
        }
    }

    // Ideally, we shouldn't have NSTrackingMouseMoved events included below, it should
    // only be turned on if mouseTracking, hover is on or a tool tip is set.
    // Unfortunately, Qt will send "tooltip" events on mouse moves, so we need to
    // turn it on in ALL case. That means EVERY QWindow gets to pay the cost of
    // mouse moves delivered to it (Apple recommends keeping it OFF because there
    // is a performance hit). So it goes.
    NSUInteger trackingOptions = NSTrackingMouseEnteredAndExited | NSTrackingActiveInActiveApp
                                 | NSTrackingInVisibleRect | NSTrackingMouseMoved;
    NSTrackingArea *ta = [[[NSTrackingArea alloc] initWithRect:[self frame]
                                                      options:trackingOptions
                                                        owner:self
                                                     userInfo:nil]
                                                                autorelease];
    [self addTrackingArea:ta];
}

- (void)mouseMoved:(NSEvent *)theEvent
{
    if (m_window->flags() & Qt::WindowTransparentForInput)
        return [super mouseMoved:theEvent];

    QPoint windowPoint, screenPoint;
    [self convertFromEvent:theEvent toWindowPoint:&windowPoint andScreenPoint:&screenPoint];
    QWindow *childWindow = m_platformWindow->childWindowAt(windowPoint);

    // Top-level windows generate enter-leave events for sub-windows.
    // Qt wants to know which window (if any) will be entered at the
    // the time of the leave. This is dificult to accomplish by
    // handling mouseEnter and mouseLeave envents, since they are sent
    // individually to different views.
    if (m_platformWindow->m_nsWindow && childWindow) {
        if (childWindow != m_platformWindow->m_underMouseWindow) {
            QWindowSystemInterface::handleEnterLeaveEvent(childWindow, m_platformWindow->m_underMouseWindow, windowPoint, screenPoint);
            m_platformWindow->m_underMouseWindow = childWindow;
        }
    }

    // Cocoa keeps firing mouse move events for obscured parent views. Qt should not
    // send those events so filter them out here.
    if (childWindow != m_window)
        return;

    [self handleMouseEvent: theEvent];
}

- (void)mouseEntered:(NSEvent *)theEvent
{
    if (m_window->flags() & Qt::WindowTransparentForInput)
        return [super mouseEntered:theEvent];

    // Top-level windows generate enter events for sub-windows.
    if (!m_platformWindow->m_nsWindow)
        return;

    QPoint windowPoint, screenPoint;
    [self convertFromEvent:theEvent toWindowPoint:&windowPoint andScreenPoint:&screenPoint];
    m_platformWindow->m_underMouseWindow = m_platformWindow->childWindowAt(windowPoint);
    QWindowSystemInterface::handleEnterEvent(m_platformWindow->m_underMouseWindow, windowPoint, screenPoint);
}

- (void)mouseExited:(NSEvent *)theEvent
{
    if (m_window->flags() & Qt::WindowTransparentForInput)
        return [super mouseExited:theEvent];
    Q_UNUSED(theEvent);

    // Top-level windows generate leave events for sub-windows.
    if (!m_platformWindow->m_nsWindow)
        return;

    QWindowSystemInterface::handleLeaveEvent(m_platformWindow->m_underMouseWindow);
    m_platformWindow->m_underMouseWindow = 0;
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
    if (m_window->flags() & Qt::WindowTransparentForInput)
        return [super rightMouseDown:theEvent];
    m_buttons |= Qt::RightButton;
    [self handleMouseEvent:theEvent];
}

- (void)rightMouseDragged:(NSEvent *)theEvent
{
    if (m_window->flags() & Qt::WindowTransparentForInput)
        return [super rightMouseDragged:theEvent];
    if (!(m_buttons & Qt::RightButton))
        qWarning("QNSView rightMouseDragged: Internal mouse button tracking invalid (missing Qt::RightButton)");
    [self handleMouseEvent:theEvent];
}

- (void)rightMouseUp:(NSEvent *)theEvent
{
    if (m_window->flags() & Qt::WindowTransparentForInput)
        return [super rightMouseUp:theEvent];
    m_buttons &= ~Qt::RightButton;
    [self handleMouseEvent:theEvent];
}

- (void)otherMouseDown:(NSEvent *)theEvent
{
    if (m_window->flags() & Qt::WindowTransparentForInput)
        return [super otherMouseDown:theEvent];
    m_buttons |= cocoaButton2QtButton([theEvent buttonNumber]);
    [self handleMouseEvent:theEvent];
}

- (void)otherMouseDragged:(NSEvent *)theEvent
{
    if (m_window->flags() & Qt::WindowTransparentForInput)
        return [super otherMouseDragged:theEvent];
    if (!(m_buttons & ~(Qt::LeftButton | Qt::RightButton)))
        qWarning("QNSView otherMouseDragged: Internal mouse button tracking invalid (missing Qt::MiddleButton or Qt::ExtraButton*)");
    [self handleMouseEvent:theEvent];
}

- (void)otherMouseUp:(NSEvent *)theEvent
{
    if (m_window->flags() & Qt::WindowTransparentForInput)
        return [super otherMouseUp:theEvent];
    m_buttons &= ~cocoaButton2QtButton([theEvent buttonNumber]);
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
    if (m_window->flags() & Qt::WindowTransparentForInput)
        return [super scrollWheel:theEvent];
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

    QPoint qt_windowPoint, qt_screenPoint;
    [self convertFromEvent:theEvent toWindowPoint:&qt_windowPoint andScreenPoint:&qt_screenPoint];
    NSTimeInterval timestamp = [theEvent timestamp];
    ulong qt_timestamp = timestamp * 1000;

    // Set keyboard modifiers depending on event phase. A two-finger trackpad flick
    // generates a stream of scroll events. We want the keyboard modifier state to
    // be the state at the beginning of the flick in order to avoid changing the
    // interpretation of the events mid-stream. One example of this happening would
    // be when pressing cmd after scrolling in Qt Creator: not taking the phase into
    // account causes the end of the event stream to be interpreted as font size changes.

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
    if ([theEvent respondsToSelector:@selector(scrollingDeltaX)]) {
        NSEventPhase phase = [theEvent phase];
        if (phase == NSEventPhaseBegan) {
            currentWheelModifiers = [QNSView convertKeyModifiers:[theEvent modifierFlags]];
        }

        QWindowSystemInterface::handleWheelEvent(m_window, qt_timestamp, qt_windowPoint, qt_screenPoint, pixelDelta, angleDelta, currentWheelModifiers);

        if (phase == NSEventPhaseEnded || phase == NSEventPhaseCancelled) {
            currentWheelModifiers = Qt::NoModifier;
        }
    } else
#endif
    {
        QWindowSystemInterface::handleWheelEvent(m_window, qt_timestamp, qt_windowPoint, qt_screenPoint, pixelDelta, angleDelta,
                                                 [QNSView convertKeyModifiers:[theEvent modifierFlags]]);
    }
}
#endif //QT_NO_WHEELEVENT

- (int) convertKeyCode : (QChar)keyChar
{
    return qt_mac_cocoaKey2QtKey(keyChar);
}

+ (Qt::KeyboardModifiers) convertKeyModifiers : (ulong)modifierFlags
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

- (void)handleKeyEvent:(NSEvent *)nsevent eventType:(int)eventType
{
    ulong timestamp = [nsevent timestamp] * 1000;
    ulong nativeModifiers = [nsevent modifierFlags];
    Qt::KeyboardModifiers modifiers = [QNSView convertKeyModifiers: nativeModifiers];
    NSString *charactersIgnoringModifiers = [nsevent charactersIgnoringModifiers];
    NSString *characters = [nsevent characters];

    // [from Qt 4 impl] There is no way to get the scan code from carbon. But we cannot
    // use the value 0, since it indicates that the event originates from somewhere
    // else than the keyboard.
    quint32 nativeScanCode = 1;

    UInt32 nativeVirtualKey = 0;
    EventRef eventRef = EventRef([nsevent eventRef]);
    GetEventParameter(eventRef, kEventParamKeyCode, typeUInt32, 0, sizeof(nativeVirtualKey), 0, &nativeVirtualKey);

    QChar ch = QChar::ReplacementCharacter;
    int keyCode = Qt::Key_unknown;
    if ([characters length] != 0) {
        if ((modifiers & Qt::MetaModifier) && ([charactersIgnoringModifiers length] != 0))
            ch = QChar([charactersIgnoringModifiers characterAtIndex:0]);
        else
            ch = QChar([characters characterAtIndex:0]);
        keyCode = [self convertKeyCode:ch];
    }

    // we will send a key event unless the input method sets m_sendKeyEvent to false
    m_sendKeyEvent = true;
    QString text;
    // ignore text for the U+F700-U+F8FF range. This is used by Cocoa when
    // delivering function keys (e.g. arrow keys, backspace, F1-F35, etc.)
    if (ch.unicode() < 0xf700 || ch.unicode() > 0xf8ff)
        text = QCFString::toQString(characters);

    if (eventType == QEvent::KeyPress) {

        if (m_composingText.isEmpty())
            m_sendKeyEvent = !QWindowSystemInterface::tryHandleShortcutEvent(m_window, timestamp, keyCode, modifiers, text);

        QObject *fo = QGuiApplication::focusObject();
        if (m_sendKeyEvent && fo) {
            QInputMethodQueryEvent queryEvent(Qt::ImEnabled | Qt::ImHints);
            if (QCoreApplication::sendEvent(fo, &queryEvent)) {
                bool imEnabled = queryEvent.value(Qt::ImEnabled).toBool();
                Qt::InputMethodHints hints = static_cast<Qt::InputMethodHints>(queryEvent.value(Qt::ImHints).toUInt());
                if (imEnabled && !(hints & Qt::ImhDigitsOnly || hints & Qt::ImhFormattedNumbersOnly || hints & Qt::ImhHiddenText)) {
                    // pass the key event to the input method. note that m_sendKeyEvent may be set to false during this call
                    [self interpretKeyEvents:[NSArray arrayWithObject:nsevent]];
                }
            }
        }
    }

    if (m_sendKeyEvent && m_composingText.isEmpty())
        QWindowSystemInterface::handleExtendedKeyEvent(m_window, timestamp, QEvent::Type(eventType), keyCode, modifiers,
                                                       nativeScanCode, nativeVirtualKey, nativeModifiers, text);

    m_sendKeyEvent = false;
}

- (void)keyDown:(NSEvent *)nsevent
{
    if (m_window->flags() & Qt::WindowTransparentForInput)
        return [super keyDown:nsevent];
    [self handleKeyEvent:nsevent eventType:int(QEvent::KeyPress)];
}

- (void)keyUp:(NSEvent *)nsevent
{
    if (m_window->flags() & Qt::WindowTransparentForInput)
        return [super keyUp:nsevent];
    [self handleKeyEvent:nsevent eventType:int(QEvent::KeyRelease)];
}

- (BOOL)performKeyEquivalent:(NSEvent *)nsevent
{
    NSString *chars = [nsevent charactersIgnoringModifiers];

    if ([nsevent type] == NSKeyDown && [chars length] > 0) {
        QChar ch = [chars characterAtIndex:0];
        Qt::Key qtKey = qt_mac_cocoaKey2QtKey(ch);
        // check for Command + Key_Period
        if ([nsevent modifierFlags] & NSCommandKeyMask
                && qtKey == Qt::Key_Period) {
            [self handleKeyEvent:nsevent eventType:int(QEvent::KeyPress)];
            return YES;
        }
    }
    return [super performKeyEquivalent:nsevent];
}

- (void)flagsChanged:(NSEvent *)nsevent
{
    ulong timestamp = [nsevent timestamp] * 1000;
    ulong modifiers = [nsevent modifierFlags];
    Qt::KeyboardModifiers qmodifiers = [QNSView convertKeyModifiers:modifiers];

    // calculate the delta and remember the current modifiers for next time
    static ulong m_lastKnownModifiers;
    ulong lastKnownModifiers = m_lastKnownModifiers;
    ulong delta = lastKnownModifiers ^ modifiers;
    m_lastKnownModifiers = modifiers;

    struct qt_mac_enum_mapper
    {
        ulong mac_mask;
        Qt::Key qt_code;
    };
    static qt_mac_enum_mapper modifier_key_symbols[] = {
        { NSShiftKeyMask, Qt::Key_Shift },
        { NSControlKeyMask, Qt::Key_Meta },
        { NSCommandKeyMask, Qt::Key_Control },
        { NSAlternateKeyMask, Qt::Key_Alt },
        { NSAlphaShiftKeyMask, Qt::Key_CapsLock },
        { 0ul, Qt::Key_unknown } };
    for (int i = 0; modifier_key_symbols[i].mac_mask != 0u; ++i) {
        uint mac_mask = modifier_key_symbols[i].mac_mask;
        if ((delta & mac_mask) == 0u)
            continue;

        QWindowSystemInterface::handleKeyEvent(m_window,
                                               timestamp,
                                               (lastKnownModifiers & mac_mask) ? QEvent::KeyRelease : QEvent::KeyPress,
                                               modifier_key_symbols[i].qt_code,
                                               qmodifiers);
    }
}

- (void) doCommandBySelector:(SEL)aSelector
{
    [self tryToPerform:aSelector with:self];
}

- (void) insertText:(id)aString replacementRange:(NSRange)replacementRange
{
    Q_UNUSED(replacementRange)

    if (m_sendKeyEvent && m_composingText.isEmpty()) {
        // don't send input method events for simple text input (let handleKeyEvent send key events instead)
        return;
    }

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
        QInputMethodQueryEvent queryEvent(Qt::ImEnabled);
        if (QCoreApplication::sendEvent(fo, &queryEvent)) {
            if (queryEvent.value(Qt::ImEnabled).toBool()) {
                QInputMethodEvent e;
                e.setCommitString(commitString);
                QCoreApplication::sendEvent(fo, &e);
                // prevent handleKeyEvent from sending a key event
                m_sendKeyEvent = false;
            }
        }
    }

    m_composingText.clear();
}

- (void) setMarkedText:(id)aString selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange
{
    Q_UNUSED(replacementRange)
    QString preeditString;

    QList<QInputMethodEvent::Attribute> attrs;
    attrs<<QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, selectedRange.location + selectedRange.length, 1, QVariant());

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
        QInputMethodQueryEvent queryEvent(Qt::ImEnabled);
        if (QCoreApplication::sendEvent(fo, &queryEvent)) {
            if (queryEvent.value(Qt::ImEnabled).toBool()) {
                QInputMethodEvent e(preeditString, attrs);
                QCoreApplication::sendEvent(fo, &e);
                // prevent handleKeyEvent from sending a key event
                m_sendKeyEvent = false;
            }
        }
    }
}

- (void) unmarkText
{
    if (!m_composingText.isEmpty()) {
        QObject *fo = QGuiApplication::focusObject();
        if (fo) {
            QInputMethodQueryEvent queryEvent(Qt::ImEnabled);
            if (QCoreApplication::sendEvent(fo, &queryEvent)) {
                if (queryEvent.value(Qt::ImEnabled).toBool()) {
                    QInputMethodEvent e;
                    e.setCommitString(m_composingText);
                    QCoreApplication::sendEvent(fo, &e);
                }
            }
        }
    }
    m_composingText.clear();
}

- (BOOL) hasMarkedText
{
    return (m_composingText.isEmpty() ? NO: YES);
}

- (NSAttributedString *) attributedSubstringForProposedRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange
{
    Q_UNUSED(actualRange)
    QObject *fo = QGuiApplication::focusObject();
    if (!fo)
        return nil;
    QInputMethodQueryEvent queryEvent(Qt::ImEnabled | Qt::ImCurrentSelection);
    if (!QCoreApplication::sendEvent(fo, &queryEvent))
        return nil;
    if (!queryEvent.value(Qt::ImEnabled).toBool())
        return nil;

    QString selectedText = queryEvent.value(Qt::ImCurrentSelection).toString();
    if (selectedText.isEmpty())
        return nil;

    QCFString string(selectedText.mid(aRange.location, aRange.length));
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
    NSRange selectedRange = {NSNotFound, 0};
    selectedRange.location = NSNotFound;
    selectedRange.length = 0;

    QObject *fo = QGuiApplication::focusObject();
    if (!fo)
        return selectedRange;
    QInputMethodQueryEvent queryEvent(Qt::ImEnabled | Qt::ImCurrentSelection);
    if (!QCoreApplication::sendEvent(fo, &queryEvent))
        return selectedRange;
    if (!queryEvent.value(Qt::ImEnabled).toBool())
        return selectedRange;

    QString selectedText = queryEvent.value(Qt::ImCurrentSelection).toString();

    if (!selectedText.isEmpty()) {
        selectedRange.location = 0;
        selectedRange.length = selectedText.length();
    }
    return selectedRange;
}

- (NSRect) firstRectForCharacterRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange
{
    Q_UNUSED(aRange)
    Q_UNUSED(actualRange)
    QObject *fo = QGuiApplication::focusObject();
    if (!fo)
        return NSZeroRect;

    QInputMethodQueryEvent queryEvent(Qt::ImEnabled);
    if (!QCoreApplication::sendEvent(fo, &queryEvent))
        return NSZeroRect;
    if (!queryEvent.value(Qt::ImEnabled).toBool())
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

- (NSUInteger)characterIndexForPoint:(NSPoint)aPoint
{
    // We don't support cursor movements using mouse while composing.
    Q_UNUSED(aPoint);
    return NSNotFound;
}

- (NSArray*) validAttributesForMarkedText
{
    if (m_window != QGuiApplication::focusWindow())
        return nil;

    QObject *fo = QGuiApplication::focusObject();
    if (!fo)
        return nil;

    QInputMethodQueryEvent queryEvent(Qt::ImEnabled);
    if (!QCoreApplication::sendEvent(fo, &queryEvent))
        return nil;
    if (!queryEvent.value(Qt::ImEnabled).toBool())
        return nil;

    // Support only underline color/style.
    return [NSArray arrayWithObjects:NSUnderlineColorAttributeName,
                                     NSUnderlineStyleAttributeName, nil];
}

-(void)registerDragTypes
{
    QCocoaAutoReleasePool pool;
    QStringList customTypes = qt_mac_enabledDraggedTypes();
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
                       NSRTFDPboardType, NSHTMLPboardType,
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

- (NSDragOperation) draggingSourceOperationMaskForLocal:(BOOL)isLocal
{
    Q_UNUSED(isLocal);
    QCocoaDrag* nativeDrag = static_cast<QCocoaDrag *>(QGuiApplicationPrivate::platformIntegration()->drag());
    return qt_mac_mapDropActions(nativeDrag->currentDrag()->supportedActions());
}

- (BOOL) ignoreModifierKeysWhileDragging
{
    return NO;
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

    // update these so selecting move/copy/link works
    QGuiApplicationPrivate::modifier_buttons = [QNSView convertKeyModifiers: [[NSApp currentEvent] modifierFlags]];

    QPlatformDragQtResponse response(false, Qt::IgnoreAction, QRect());
    if ([sender draggingSource] != nil) {
        QCocoaDrag* nativeDrag = static_cast<QCocoaDrag *>(QGuiApplicationPrivate::platformIntegration()->drag());
        response = QWindowSystemInterface::handleDrag(m_window, nativeDrag->platformDropData(), qt_windowPoint, qtAllowed);
    } else {
        QCocoaDropData mimeData([sender draggingPasteboard]);
        response = QWindowSystemInterface::handleDrag(m_window, &mimeData, qt_windowPoint, qtAllowed);
    }

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

    QPlatformDropQtResponse response(false, Qt::IgnoreAction);
    if ([sender draggingSource] != nil) {
        QCocoaDrag* nativeDrag = static_cast<QCocoaDrag *>(QGuiApplicationPrivate::platformIntegration()->drag());
        response = QWindowSystemInterface::handleDrop(m_window, nativeDrag->platformDropData(), qt_windowPoint, qtAllowed);
    } else {
        QCocoaDropData mimeData([sender draggingPasteboard]);
        response = QWindowSystemInterface::handleDrop(m_window, &mimeData, qt_windowPoint, qtAllowed);
    }
    if (response.isAccepted()) {
        QCocoaDrag* nativeDrag = static_cast<QCocoaDrag *>(QGuiApplicationPrivate::platformIntegration()->drag());
        nativeDrag->setAcceptedAction(response.acceptedAction());
    }
    return response.isAccepted();
}

- (void)draggedImage:(NSImage*) img endedAt:(NSPoint) point operation:(NSDragOperation) operation
{
    Q_UNUSED(img);
    Q_UNUSED(operation);

// keep our state, and QGuiApplication state (buttons member) in-sync,
// or future mouse events will be processed incorrectly
    m_buttons &= ~Qt::LeftButton;

    NSPoint windowPoint = [self convertPoint: point fromView: nil];
    QPoint qtWindowPoint(windowPoint.x, windowPoint.y);

    NSWindow *window = [self window];
    NSPoint screenPoint = [window convertBaseToScreen :point];
    QPoint qtScreenPoint = QPoint(screenPoint.x, qt_mac_flipYCoordinate(screenPoint.y));

    QWindowSystemInterface::handleMouseEvent(m_window, qtWindowPoint, qtScreenPoint, m_buttons);
}

@end
