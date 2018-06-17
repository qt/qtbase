/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <QtGui/qtguiglobal.h>

#include "qnsview.h"
#include "qcocoawindow.h"
#include "qcocoahelpers.h"
#include "qcocoascreen.h"
#include "qmultitouch_mac_p.h"
#include "qcocoadrag.h"
#include "qcocoainputcontext.h"
#include <qpa/qplatformintegration.h>

#include <qpa/qwindowsysteminterface.h>
#include <QtGui/QTextFormat>
#include <QtCore/QDebug>
#include <QtCore/qsysinfo.h>
#include <private/qguiapplication_p.h>
#include <private/qcoregraphics_p.h>
#include <private/qwindow_p.h>
#include "qcocoabackingstore.h"
#ifndef QT_NO_OPENGL
#include "qcocoaglcontext.h"
#endif
#include "qcocoaintegration.h"

#ifdef QT_COCOA_ENABLE_ACCESSIBILITY_INSPECTOR
#include <accessibilityinspector.h>
#endif

Q_LOGGING_CATEGORY(lcQpaTouch, "qt.qpa.input.touch")
#ifndef QT_NO_GESTURES
Q_LOGGING_CATEGORY(lcQpaGestures, "qt.qpa.input.gestures")
#endif
Q_LOGGING_CATEGORY(lcQpaTablet, "qt.qpa.input.tablet")

@interface QT_MANGLE_NAMESPACE(QNSViewMouseMoveHelper) : NSObject
{
    QNSView *view;
}

- (id)initWithView:(QNSView *)theView;

- (void)mouseMoved:(NSEvent *)theEvent;
- (void)mouseEntered:(NSEvent *)theEvent;
- (void)mouseExited:(NSEvent *)theEvent;
- (void)cursorUpdate:(NSEvent *)theEvent;

@end

@implementation QT_MANGLE_NAMESPACE(QNSViewMouseMoveHelper)

- (id)initWithView:(QNSView *)theView
{
    self = [super init];
    if (self) {
        view = theView;
    }
    return self;
}

- (void)mouseMoved:(NSEvent *)theEvent
{
    [view mouseMovedImpl:theEvent];
}

- (void)mouseEntered:(NSEvent *)theEvent
{
    [view mouseEnteredImpl:theEvent];
}

- (void)mouseExited:(NSEvent *)theEvent
{
    [view mouseExitedImpl:theEvent];
}

- (void)cursorUpdate:(NSEvent *)theEvent
{
    [view cursorUpdate:theEvent];
}

@end

// Private interface
@interface QT_MANGLE_NAMESPACE(QNSView) ()
- (BOOL)isTransparentForUserInput;
@end

@implementation QT_MANGLE_NAMESPACE(QNSView)

- (id) init
{
    if (self = [super initWithFrame:NSZeroRect]) {
        m_buttons = Qt::NoButton;
        m_acceptedMouseDowns = Qt::NoButton;
        m_frameStrutButtons = Qt::NoButton;
        m_sendKeyEvent = false;
#ifndef QT_NO_OPENGL
        m_glContext = 0;
        m_shouldSetGLContextinDrawRect = false;
#endif
        currentCustomDragTypes = 0;
        m_dontOverrideCtrlLMB = false;
        m_sendUpAsRightButton = false;
        m_inputSource = 0;
        m_mouseMoveHelper = [[QT_MANGLE_NAMESPACE(QNSViewMouseMoveHelper) alloc] initWithView:self];
        m_resendKeyEvent = false;
        m_scrolling = false;
        m_updatingDrag = false;
        m_currentlyInterpretedKeyEvent = 0;
        m_isMenuView = false;
        self.focusRingType = NSFocusRingTypeNone;
        self.cursor = nil;
        m_updateRequested = false;
    }
    return self;
}

- (void)dealloc
{
    if (m_trackingArea) {
        [self removeTrackingArea:m_trackingArea];
        [m_trackingArea release];
    }
    [m_inputSource release];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [m_mouseMoveHelper release];

    delete currentCustomDragTypes;

    [super dealloc];
}

- (id)initWithCocoaWindow:(QCocoaWindow *)platformWindow
{
    self = [self init];
    if (!self)
        return 0;

    m_platformWindow = platformWindow;
    m_sendKeyEvent = false;
    m_dontOverrideCtrlLMB = qt_mac_resolveOption(false, platformWindow->window(), "_q_platform_MacDontOverrideCtrlLMB", "QT_MAC_DONT_OVERRIDE_CTRL_LMB");
    m_trackingArea = nil;

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

    [[NSNotificationCenter defaultCenter] addObserver:self
                                          selector:@selector(textInputContextKeyboardSelectionDidChangeNotification:)
                                          name:NSTextInputContextKeyboardSelectionDidChangeNotification
                                          object:nil];

    return self;
}

- (NSString *)description
{
    NSMutableString *description = [NSMutableString stringWithString:[super description]];

#ifndef QT_NO_DEBUG_STREAM
    QString platformWindowDescription;
    QDebug debug(&platformWindowDescription);
    debug.nospace() << "; " << m_platformWindow << ">";

    NSRange lastCharacter = [description rangeOfComposedCharacterSequenceAtIndex:description.length - 1];
    [description replaceCharactersInRange:lastCharacter withString:platformWindowDescription.toNSString()];
#endif

    return description;
}

#ifndef QT_NO_OPENGL
- (void) setQCocoaGLContext:(QCocoaGLContext *)context
{
    m_glContext = context;
    [m_glContext->nsOpenGLContext() setView:self];
    if (![m_glContext->nsOpenGLContext() view]) {
        //was unable to set view
        m_shouldSetGLContextinDrawRect = true;
    }
}
#endif

- (void)viewDidMoveToSuperview
{
    if (!m_platformWindow)
        return;

    if (!(m_platformWindow->m_viewIsToBeEmbedded))
        return;

    if ([self superview]) {
        m_platformWindow->m_viewIsEmbedded = true;
        QWindowSystemInterface::handleGeometryChange(m_platformWindow->window(), m_platformWindow->geometry());
        [self setNeedsDisplay:YES];
        QWindowSystemInterface::flushWindowSystemEvents();
    } else {
        m_platformWindow->m_viewIsEmbedded = false;
    }
}

- (QWindow *)topLevelWindow
{
    if (!m_platformWindow)
        return nullptr;

    QWindow *focusWindow = m_platformWindow->window();

    // For widgets we need to do a bit of trickery as the window
    // to activate is the window of the top-level widget.
    if (qstrcmp(focusWindow->metaObject()->className(), "QWidgetWindow") == 0) {
        while (focusWindow->parent()) {
            focusWindow = focusWindow->parent();
        }
    }

    return focusWindow;
}

- (void)textInputContextKeyboardSelectionDidChangeNotification : (NSNotification *) textInputContextKeyboardSelectionDidChangeNotification
{
    Q_UNUSED(textInputContextKeyboardSelectionDidChangeNotification)
    if (([NSApp keyWindow] == [self window]) && [[self window] firstResponder] == self) {
        QCocoaInputContext *ic = qobject_cast<QCocoaInputContext *>(QCocoaIntegration::instance()->inputContext());
        ic->updateLocale();
    }
}

- (void)viewDidHide
{
    if (!m_platformWindow->isExposed())
        return;

    m_platformWindow->handleExposeEvent(QRegion());

    // Note: setNeedsDisplay is automatically called for
    // viewDidUnhide so no reason to override it here.
}

- (void)removeFromSuperview
{
    QMacAutoReleasePool pool;
    [super removeFromSuperview];
}

- (BOOL) isOpaque
{
    if (!m_platformWindow)
        return true;
    return m_platformWindow->isOpaque();
}

- (void)requestUpdate
{
    if (self.needsDisplay) {
        // If the view already has needsDisplay set it means that there may be code waiting for
        // a real expose event, so we can't issue setNeedsDisplay now as a way to trigger an
        // update request. We will re-trigger requestUpdate from drawRect.
        return;
    }

    [self setNeedsDisplay:YES];
    m_updateRequested = true;
}

- (void)setNeedsDisplayInRect:(NSRect)rect
{
    [super setNeedsDisplayInRect:rect];
    m_updateRequested = false;
}

- (void)drawRect:(NSRect)dirtyRect
{
    Q_UNUSED(dirtyRect);

    if (!m_platformWindow)
        return;

    QRegion exposedRegion;
    const NSRect *dirtyRects;
    NSInteger numDirtyRects;
    [self getRectsBeingDrawn:&dirtyRects count:&numDirtyRects];
    for (int i = 0; i < numDirtyRects; ++i)
        exposedRegion += QRectF::fromCGRect(dirtyRects[i]).toRect();

    qCDebug(lcQpaCocoaDrawing) << "[QNSView drawRect:]" << m_platformWindow->window() << exposedRegion;
    [self updateRegion:exposedRegion];
}

- (void)updateRegion:(QRegion)dirtyRegion
{
#ifndef QT_NO_OPENGL
    if (m_glContext && m_shouldSetGLContextinDrawRect) {
        [m_glContext->nsOpenGLContext() setView:self];
        m_shouldSetGLContextinDrawRect = false;
    }
#endif

    QWindowPrivate *windowPrivate = qt_window_private(m_platformWindow->window());

    if (m_updateRequested) {
        Q_ASSERT(windowPrivate->updateRequestPending);
        qCDebug(lcQpaCocoaWindow) << "Delivering update request to" << m_platformWindow->window();
        windowPrivate->deliverUpdateRequest();
        m_updateRequested = false;
    } else {
        m_platformWindow->handleExposeEvent(dirtyRegion);
    }

    if (m_updateRequested && windowPrivate->updateRequestPending) {
        // A call to QWindow::requestUpdate was issued during event delivery above,
        // but AppKit will reset the needsDisplay state of the view after completing
        // the current display cycle, so we need to defer the request to redisplay.
        // FIXME: Perhaps this should be a trigger to enable CADisplayLink?
        qCDebug(lcQpaCocoaDrawing) << "[QNSView drawRect:] issuing deferred setNeedsDisplay due to pending update request";
        dispatch_async(dispatch_get_main_queue (), ^{ [self requestUpdate]; });
    }
}

- (BOOL)wantsUpdateLayer
{
    return YES;
}

- (void)updateLayer
{
    if (!m_platformWindow)
        return;

    qCDebug(lcQpaCocoaDrawing) << "[QNSView updateLayer]" << m_platformWindow->window();

    // FIXME: Find out if there's a way to resolve the dirty rect like in drawRect:
    [self updateRegion:QRectF::fromCGRect(self.bounds).toRect()];
}

- (void)viewDidChangeBackingProperties
{
    if (self.layer)
        self.layer.contentsScale = self.window.backingScaleFactor;
}

- (BOOL)isFlipped
{
    return YES;
}

- (BOOL)isTransparentForUserInput
{
    return m_platformWindow->window() &&
        m_platformWindow->window()->flags() & Qt::WindowTransparentForInput;
}

- (BOOL)becomeFirstResponder
{
    if (!m_platformWindow)
        return NO;
    if ([self isTransparentForUserInput])
        return NO;
    if (!m_platformWindow->windowIsPopupType() && !m_isMenuView)
        QWindowSystemInterface::handleWindowActivated([self topLevelWindow]);
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    if (!m_platformWindow)
        return NO;
    if (m_isMenuView)
        return NO;
    if (m_platformWindow->shouldRefuseKeyWindowAndFirstResponder())
        return NO;
    if ([self isTransparentForUserInput])
        return NO;
    if ((m_platformWindow->window()->flags() & Qt::ToolTip) == Qt::ToolTip)
        return NO;
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
    Q_UNUSED(theEvent)
    if (!m_platformWindow)
        return NO;
    if ([self isTransparentForUserInput])
        return NO;
    return YES;
}

- (NSView *)hitTest:(NSPoint)aPoint
{
    NSView *candidate = [super hitTest:aPoint];
    if (candidate == self) {
        if ([self isTransparentForUserInput])
            return nil;
    }
    return candidate;
}

- (void)convertFromScreen:(NSPoint)mouseLocation toWindowPoint:(QPointF *)qtWindowPoint andScreenPoint:(QPointF *)qtScreenPoint
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

    NSWindow *window = [self window];
    NSPoint nsWindowPoint;
    NSRect windowRect = [window convertRectFromScreen:NSMakeRect(mouseLocation.x, mouseLocation.y, 1, 1)];
    nsWindowPoint = windowRect.origin;                    // NSWindow coordinates
    NSPoint nsViewPoint = [self convertPoint: nsWindowPoint fromView: nil]; // NSView/QWindow coordinates
    *qtWindowPoint = QPointF(nsViewPoint.x, nsViewPoint.y);                     // NSView/QWindow coordinates
    *qtScreenPoint = QCocoaScreen::mapFromNative(mouseLocation);
}

- (void)resetMouseButtons
{
    m_buttons = Qt::NoButton;
    m_frameStrutButtons = Qt::NoButton;
}

- (NSPoint) screenMousePoint:(NSEvent *)theEvent
{
    NSPoint screenPoint;
    if (theEvent) {
        NSPoint windowPoint = [theEvent locationInWindow];
        if (qIsNaN(windowPoint.x) || qIsNaN(windowPoint.y)) {
            screenPoint = [NSEvent mouseLocation];
        } else {
            NSRect screenRect = [[theEvent window] convertRectToScreen:NSMakeRect(windowPoint.x, windowPoint.y, 1, 1)];
            screenPoint = screenRect.origin;
        }
    } else {
        screenPoint = [NSEvent mouseLocation];
    }
    return screenPoint;
}

- (void)handleMouseEvent:(NSEvent *)theEvent
{
    if (!m_platformWindow)
        return;

#ifndef QT_NO_TABLETEVENT
    // Tablet events may come in via the mouse event handlers,
    // check if this is a valid tablet event first.
    if ([self handleTabletEvent: theEvent])
        return;
#endif

    QPointF qtWindowPoint;
    QPointF qtScreenPoint;
    QNSView *targetView = self;
    if (!targetView.platformWindow)
        return;

    // Popups implicitly grap mouse events; forward to the active popup if there is one
    if (QCocoaWindow *popup = QCocoaIntegration::instance()->activePopupWindow()) {
        // Tooltips must be transparent for mouse events
        // The bug reference is QTBUG-46379
        if (!popup->m_windowFlags.testFlag(Qt::ToolTip)) {
            if (QNSView *popupView = qnsview_cast(popup->view()))
                targetView = popupView;
        }
    }

    [targetView convertFromScreen:[self screenMousePoint:theEvent] toWindowPoint:&qtWindowPoint andScreenPoint:&qtScreenPoint];
    ulong timestamp = [theEvent timestamp] * 1000;

    QCocoaDrag* nativeDrag = QCocoaIntegration::instance()->drag();
    nativeDrag->setLastMouseEvent(theEvent, self);

    Qt::KeyboardModifiers keyboardModifiers = [QNSView convertKeyModifiers:[theEvent modifierFlags]];
    QWindowSystemInterface::handleMouseEvent(targetView->m_platformWindow->window(), timestamp, qtWindowPoint, qtScreenPoint,
                                             m_buttons, keyboardModifiers, Qt::MouseEventNotSynthesized);
}

- (void)handleFrameStrutMouseEvent:(NSEvent *)theEvent
{
    if (!m_platformWindow)
        return;

    // get m_buttons in sync
    // Don't send frme strut events if we are in the middle of a mouse drag.
    if (m_buttons != Qt::NoButton)
        return;

    NSEventType ty = [theEvent type];
    switch (ty) {
    case NSLeftMouseDown:
        m_frameStrutButtons |= Qt::LeftButton;
        break;
    case NSLeftMouseUp:
         m_frameStrutButtons &= ~Qt::LeftButton;
         break;
    case NSRightMouseDown:
        m_frameStrutButtons |= Qt::RightButton;
        break;
    case NSLeftMouseDragged:
        m_frameStrutButtons |= Qt::LeftButton;
        break;
    case NSRightMouseDragged:
        m_frameStrutButtons |= Qt::RightButton;
        break;
    case NSRightMouseUp:
        m_frameStrutButtons &= ~Qt::RightButton;
        break;
    case NSOtherMouseDown:
        m_frameStrutButtons |= cocoaButton2QtButton([theEvent buttonNumber]);
        break;
    case NSOtherMouseUp:
        m_frameStrutButtons &= ~cocoaButton2QtButton([theEvent buttonNumber]);
    default:
        break;
    }

    NSWindow *window = [self window];
    NSPoint windowPoint = [theEvent locationInWindow];

    int windowScreenY = [window frame].origin.y + [window frame].size.height;
    NSPoint windowCoord = [self convertPoint:[self frame].origin toView:nil];
    int viewScreenY = [window convertRectToScreen:NSMakeRect(windowCoord.x, windowCoord.y, 0, 0)].origin.y;
    int titleBarHeight = windowScreenY - viewScreenY;

    NSPoint nsViewPoint = [self convertPoint: windowPoint fromView: nil];
    QPoint qtWindowPoint = QPoint(nsViewPoint.x, titleBarHeight + nsViewPoint.y);
    NSPoint screenPoint = [window convertRectToScreen:NSMakeRect(windowPoint.x, windowPoint.y, 0, 0)].origin;
    QPoint qtScreenPoint = QCocoaScreen::mapFromNative(screenPoint).toPoint();

    ulong timestamp = [theEvent timestamp] * 1000;
    QWindowSystemInterface::handleFrameStrutMouseEvent(m_platformWindow->window(), timestamp, qtWindowPoint, qtScreenPoint, m_frameStrutButtons);
}

- (bool)handleMouseDownEvent:(NSEvent *)theEvent withButton:(int)buttonNumber
{
    if ([self isTransparentForUserInput])
        return false;

    Qt::MouseButton button = cocoaButton2QtButton(buttonNumber);

    QPointF qtWindowPoint;
    QPointF qtScreenPoint;
    [self convertFromScreen:[self screenMousePoint:theEvent] toWindowPoint:&qtWindowPoint andScreenPoint:&qtScreenPoint];
    Q_UNUSED(qtScreenPoint);

    // Maintain masked state for the button for use by MouseDragged and MouseUp.
    QRegion mask = m_platformWindow->window()->mask();
    const bool masked = !mask.isEmpty() && !mask.contains(qtWindowPoint.toPoint());
    if (masked)
        m_acceptedMouseDowns &= ~button;
    else
        m_acceptedMouseDowns |= button;

    // Forward masked out events to the next responder
    if (masked)
        return false;

    m_buttons |= button;

    [self handleMouseEvent:theEvent];
    return true;
}

- (bool)handleMouseDraggedEvent:(NSEvent *)theEvent withButton:(int)buttonNumber
{
    if ([self isTransparentForUserInput])
        return false;

    Qt::MouseButton button = cocoaButton2QtButton(buttonNumber);

    // Forward the event to the next responder if Qt did not accept the
    // corresponding mouse down for this button
    if (!(m_acceptedMouseDowns & button) == button)
        return false;

    [self handleMouseEvent:theEvent];
    return true;
}

- (bool)handleMouseUpEvent:(NSEvent *)theEvent withButton:(int)buttonNumber
{
    if ([self isTransparentForUserInput])
        return false;

    Qt::MouseButton button = cocoaButton2QtButton(buttonNumber);

    // Forward the event to the next responder if Qt did not accept the
    // corresponding mouse down for this button
    if (!(m_acceptedMouseDowns & button) == button)
        return false;

    if (m_sendUpAsRightButton && button == Qt::LeftButton)
        button = Qt::RightButton;
    if (button == Qt::RightButton)
        m_sendUpAsRightButton = false;

    m_buttons &= ~button;

    [self handleMouseEvent:theEvent];
    return true;
}

- (void)mouseDown:(NSEvent *)theEvent
{
    if ([self isTransparentForUserInput])
        return [super mouseDown:theEvent];
    m_sendUpAsRightButton = false;

    // Handle any active poup windows; clicking outisde them should close them
    // all. Don't do anything or clicks inside one of the menus, let Cocoa
    // handle that case. Note that in practice many windows of the Qt::Popup type
    // will actually close themselves in this case using logic implemented in
    // that particular poup type (for example context menus). However, Qt expects
    // that plain popup QWindows will also be closed, so we implement the logic
    // here as well.
    QList<QCocoaWindow *> *popups = QCocoaIntegration::instance()->popupWindowStack();
    if (!popups->isEmpty()) {
        // Check if the click is outside all popups.
        bool inside = false;
        QPointF qtScreenPoint = QCocoaScreen::mapFromNative([self screenMousePoint:theEvent]);
        for (QList<QCocoaWindow *>::const_iterator it = popups->begin(); it != popups->end(); ++it) {
            if ((*it)->geometry().contains(qtScreenPoint.toPoint())) {
                inside = true;
                break;
            }
        }
        // Close the popups if the click was outside.
        if (!inside) {
            Qt::WindowType type = QCocoaIntegration::instance()->activePopupWindow()->window()->type();
            while (QCocoaWindow *popup = QCocoaIntegration::instance()->popPopupWindow()) {
                QWindowSystemInterface::handleCloseEvent(popup->window());
                QWindowSystemInterface::flushWindowSystemEvents();
            }
            // Consume the mouse event when closing the popup, except for tool tips
            // were it's expected that the event is processed normally.
            if (type != Qt::ToolTip)
                 return;
        }
    }

    QPointF qtWindowPoint;
    QPointF qtScreenPoint;
    [self convertFromScreen:[self screenMousePoint:theEvent] toWindowPoint:&qtWindowPoint andScreenPoint:&qtScreenPoint];
    Q_UNUSED(qtScreenPoint);

    QRegion mask = m_platformWindow->window()->mask();
    const bool masked = !mask.isEmpty() && !mask.contains(qtWindowPoint.toPoint());
    // Maintain masked state for the button for use by MouseDragged and Up.
    if (masked)
        m_acceptedMouseDowns &= ~Qt::LeftButton;
    else
        m_acceptedMouseDowns |= Qt::LeftButton;

    // Forward masked out events to the next responder
    if (masked) {
        [super mouseDown:theEvent];
        return;
    }

    if ([self hasMarkedText]) {
        [[NSTextInputContext currentInputContext] handleEvent:theEvent];
    } else {
        auto ctrlOrMetaModifier = qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta) ? Qt::ControlModifier : Qt::MetaModifier;
        if (!m_dontOverrideCtrlLMB && [QNSView convertKeyModifiers:[theEvent modifierFlags]] & ctrlOrMetaModifier) {
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
    const bool accepted = [self handleMouseDraggedEvent:theEvent withButton:[theEvent buttonNumber]];
    if (!accepted)
        [super mouseDragged:theEvent];
}

- (void)mouseUp:(NSEvent *)theEvent
{
    const bool accepted = [self handleMouseUpEvent:theEvent withButton:[theEvent buttonNumber]];
    if (!accepted)
        [super mouseUp:theEvent];
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
    // Wacom tablet might not return the correct button number for NSEvent buttonNumber
    // on right clicks. Decide here that the button is the "right" button and forward
    // the button number to the mouse (and tablet) handler.
    const bool accepted = [self handleMouseDownEvent:theEvent withButton:1];
    if (!accepted)
        [super rightMouseDown:theEvent];
}

- (void)rightMouseDragged:(NSEvent *)theEvent
{
    const bool accepted = [self handleMouseDraggedEvent:theEvent withButton:1];
    if (!accepted)
        [super rightMouseDragged:theEvent];
}

- (void)rightMouseUp:(NSEvent *)theEvent
{
    const bool accepted = [self handleMouseUpEvent:theEvent withButton:1];
    if (!accepted)
        [super rightMouseUp:theEvent];
}

- (void)otherMouseDown:(NSEvent *)theEvent
{
    const bool accepted = [self handleMouseDownEvent:theEvent withButton:[theEvent buttonNumber]];
    if (!accepted)
        [super otherMouseDown:theEvent];
}

- (void)otherMouseDragged:(NSEvent *)theEvent
{
    const bool accepted = [self handleMouseDraggedEvent:theEvent withButton:[theEvent buttonNumber]];
    if (!accepted)
        [super otherMouseDragged:theEvent];
}

- (void)otherMouseUp:(NSEvent *)theEvent
{
    const bool accepted = [self handleMouseUpEvent:theEvent withButton:[theEvent buttonNumber]];
    if (!accepted)
        [super otherMouseUp:theEvent];
}

- (void)updateTrackingAreas
{
    [super updateTrackingAreas];

    QMacAutoReleasePool pool;

    // NSTrackingInVisibleRect keeps care of updating once the tracking is set up, so bail out early
    if (m_trackingArea && [[self trackingAreas] containsObject:m_trackingArea])
        return;

    // Ideally, we shouldn't have NSTrackingMouseMoved events included below, it should
    // only be turned on if mouseTracking, hover is on or a tool tip is set.
    // Unfortunately, Qt will send "tooltip" events on mouse moves, so we need to
    // turn it on in ALL case. That means EVERY QWindow gets to pay the cost of
    // mouse moves delivered to it (Apple recommends keeping it OFF because there
    // is a performance hit). So it goes.
    NSUInteger trackingOptions = NSTrackingMouseEnteredAndExited | NSTrackingActiveInActiveApp
                                 | NSTrackingInVisibleRect | NSTrackingMouseMoved | NSTrackingCursorUpdate;
    [m_trackingArea release];
    m_trackingArea = [[NSTrackingArea alloc] initWithRect:[self frame]
                                                  options:trackingOptions
                                                    owner:m_mouseMoveHelper
                                                 userInfo:nil];
    [self addTrackingArea:m_trackingArea];
}

- (void)cursorUpdate:(NSEvent *)theEvent
{
    qCDebug(lcQpaCocoaMouse) << "[QNSView cursorUpdate:]" << self.cursor;

    // Note: We do not get this callback when moving from a subview that
    // uses the legacy cursorRect API, so the cursor is reset to the arrow
    // cursor. See rdar://34183708

    if (self.cursor)
        [self.cursor set];
    else
        [super cursorUpdate:theEvent];
}

- (void)mouseMovedImpl:(NSEvent *)theEvent
{
    if (!m_platformWindow)
        return;

    if ([self isTransparentForUserInput])
        return;

    QPointF windowPoint;
    QPointF screenPoint;
    [self convertFromScreen:[self screenMousePoint:theEvent] toWindowPoint:&windowPoint andScreenPoint:&screenPoint];
    QWindow *childWindow = m_platformWindow->childWindowAt(windowPoint.toPoint());

    // Top-level windows generate enter-leave events for sub-windows.
    // Qt wants to know which window (if any) will be entered at the
    // the time of the leave. This is dificult to accomplish by
    // handling mouseEnter and mouseLeave envents, since they are sent
    // individually to different views.
    if (m_platformWindow->isContentView() && childWindow) {
        if (childWindow != m_platformWindow->m_enterLeaveTargetWindow) {
            QWindowSystemInterface::handleEnterLeaveEvent(childWindow, m_platformWindow->m_enterLeaveTargetWindow, windowPoint, screenPoint);
            m_platformWindow->m_enterLeaveTargetWindow = childWindow;
        }
    }

    // Cocoa keeps firing mouse move events for obscured parent views. Qt should not
    // send those events so filter them out here.
    if (childWindow != m_platformWindow->window())
        return;

    [self handleMouseEvent: theEvent];
}

- (void)mouseEnteredImpl:(NSEvent *)theEvent
{
    Q_UNUSED(theEvent)
    if (!m_platformWindow)
        return;

    m_platformWindow->m_windowUnderMouse = true;

    if ([self isTransparentForUserInput])
        return;

    // Top-level windows generate enter events for sub-windows.
    if (!m_platformWindow->isContentView())
        return;

    QPointF windowPoint;
    QPointF screenPoint;
    [self convertFromScreen:[self screenMousePoint:theEvent] toWindowPoint:&windowPoint andScreenPoint:&screenPoint];
    m_platformWindow->m_enterLeaveTargetWindow = m_platformWindow->childWindowAt(windowPoint.toPoint());
    QWindowSystemInterface::handleEnterEvent(m_platformWindow->m_enterLeaveTargetWindow, windowPoint, screenPoint);
}

- (void)mouseExitedImpl:(NSEvent *)theEvent
{
    Q_UNUSED(theEvent);
    if (!m_platformWindow)
        return;

    m_platformWindow->m_windowUnderMouse = false;

    if ([self isTransparentForUserInput])
        return;

    // Top-level windows generate leave events for sub-windows.
    if (!m_platformWindow->isContentView())
        return;

    QWindowSystemInterface::handleLeaveEvent(m_platformWindow->m_enterLeaveTargetWindow);
    m_platformWindow->m_enterLeaveTargetWindow = 0;
}

#ifndef QT_NO_TABLETEVENT
struct QCocoaTabletDeviceData
{
    QTabletEvent::TabletDevice device;
    QTabletEvent::PointerType pointerType;
    uint capabilityMask;
    qint64 uid;
};

typedef QHash<uint, QCocoaTabletDeviceData> QCocoaTabletDeviceDataHash;
Q_GLOBAL_STATIC(QCocoaTabletDeviceDataHash, tabletDeviceDataHash)

- (bool)handleTabletEvent: (NSEvent *)theEvent
{
    static bool ignoreButtonMapping = qEnvironmentVariableIsSet("QT_MAC_TABLET_IGNORE_BUTTON_MAPPING");

    if (!m_platformWindow)
        return false;

    NSEventType eventType = [theEvent type];
    if (eventType != NSTabletPoint && [theEvent subtype] != NSTabletPointEventSubtype)
        return false; // Not a tablet event.

    ulong timestamp = [theEvent timestamp] * 1000;

    QPointF windowPoint;
    QPointF screenPoint;
    [self convertFromScreen:[self screenMousePoint:theEvent] toWindowPoint: &windowPoint andScreenPoint: &screenPoint];

    uint deviceId = [theEvent deviceID];
    if (!tabletDeviceDataHash->contains(deviceId)) {
        // Error: Unknown tablet device. Qt also gets into this state
        // when running on a VM. This appears to be harmless; don't
        // print a warning.
        return false;
    }
    const QCocoaTabletDeviceData &deviceData = tabletDeviceDataHash->value(deviceId);

    bool down = (eventType != NSMouseMoved);

    qreal pressure;
    if (down) {
        pressure = [theEvent pressure];
    } else {
        pressure = 0.0;
    }

    NSPoint tilt = [theEvent tilt];
    int xTilt = qRound(tilt.x * 60.0);
    int yTilt = qRound(tilt.y * -60.0);
    qreal tangentialPressure = 0;
    qreal rotation = 0;
    int z = 0;
    if (deviceData.capabilityMask & 0x0200)
        z = [theEvent absoluteZ];

    if (deviceData.capabilityMask & 0x0800)
        tangentialPressure = ([theEvent tangentialPressure] * 2.0) - 1.0;

    rotation = 360.0 - [theEvent rotation];
    if (rotation > 180.0)
        rotation -= 360.0;

    Qt::KeyboardModifiers keyboardModifiers = [QNSView convertKeyModifiers:[theEvent modifierFlags]];
    Qt::MouseButtons buttons = ignoreButtonMapping ? static_cast<Qt::MouseButtons>(static_cast<uint>([theEvent buttonMask])) : m_buttons;

    qCDebug(lcQpaTablet, "event on tablet %d with tool %d type %d unique ID %lld pos %6.1f, %6.1f root pos %6.1f, %6.1f buttons 0x%x pressure %4.2lf tilt %d, %d rotation %6.2lf",
        deviceId, deviceData.device, deviceData.pointerType, deviceData.uid,
        windowPoint.x(), windowPoint.y(), screenPoint.x(), screenPoint.y(),
        static_cast<uint>(buttons), pressure, xTilt, yTilt, rotation);

    QWindowSystemInterface::handleTabletEvent(m_platformWindow->window(), timestamp, windowPoint, screenPoint,
                                              deviceData.device, deviceData.pointerType, buttons, pressure, xTilt, yTilt,
                                              tangentialPressure, rotation, z, deviceData.uid,
                                              keyboardModifiers);
    return true;
}

- (void)tabletPoint:(NSEvent *)theEvent
{
    if ([self isTransparentForUserInput])
        return [super tabletPoint:theEvent];

    [self handleTabletEvent: theEvent];
}

static QTabletEvent::TabletDevice wacomTabletDevice(NSEvent *theEvent)
{
    qint64 uid = [theEvent uniqueID];
    uint bits = [theEvent vendorPointingDeviceType];
    if (bits == 0 && uid != 0) {
        // Fallback. It seems that the driver doesn't always include all the information.
        // High-End Wacom devices store their "type" in the uper bits of the Unique ID.
        // I'm not sure how to handle it for consumer devices, but I'll test that in a bit.
        bits = uid >> 32;
    }

    QTabletEvent::TabletDevice device;
    // Defined in the "EN0056-NxtGenImpGuideX"
    // on Wacom's Developer Website (www.wacomeng.com)
    if (((bits & 0x0006) == 0x0002) && ((bits & 0x0F06) != 0x0902)) {
        device = QTabletEvent::Stylus;
    } else {
        switch (bits & 0x0F06) {
            case 0x0802:
                device = QTabletEvent::Stylus;
                break;
            case 0x0902:
                device = QTabletEvent::Airbrush;
                break;
            case 0x0004:
                device = QTabletEvent::FourDMouse;
                break;
            case 0x0006:
                device = QTabletEvent::Puck;
                break;
            case 0x0804:
                device = QTabletEvent::RotationStylus;
                break;
            default:
                device = QTabletEvent::NoDevice;
        }
    }
    return device;
}

- (void)tabletProximity:(NSEvent *)theEvent
{
    if ([self isTransparentForUserInput])
        return [super tabletProximity:theEvent];

    ulong timestamp = [theEvent timestamp] * 1000;

    QCocoaTabletDeviceData deviceData;
    deviceData.uid = [theEvent uniqueID];
    deviceData.capabilityMask = [theEvent capabilityMask];

    switch ([theEvent pointingDeviceType]) {
        case NSUnknownPointingDevice:
        default:
            deviceData.pointerType = QTabletEvent::UnknownPointer;
            break;
        case NSPenPointingDevice:
            deviceData.pointerType = QTabletEvent::Pen;
            break;
        case NSCursorPointingDevice:
            deviceData.pointerType = QTabletEvent::Cursor;
            break;
        case NSEraserPointingDevice:
            deviceData.pointerType = QTabletEvent::Eraser;
            break;
    }

    deviceData.device = wacomTabletDevice(theEvent);

    // The deviceID is "unique" while in the proximity, it's a key that we can use for
    // linking up QCocoaTabletDeviceData to an event (especially if there are two devices in action).
    bool entering = [theEvent isEnteringProximity];
    uint deviceId = [theEvent deviceID];
    if (entering) {
        tabletDeviceDataHash->insert(deviceId, deviceData);
    } else {
        tabletDeviceDataHash->remove(deviceId);
    }

    qCDebug(lcQpaTablet, "proximity change on tablet %d: current tool %d type %d unique ID %lld",
        deviceId, deviceData.device, deviceData.pointerType, deviceData.uid);

    if (entering) {
        QWindowSystemInterface::handleTabletEnterProximityEvent(timestamp, deviceData.device, deviceData.pointerType, deviceData.uid);
    } else {
        QWindowSystemInterface::handleTabletLeaveProximityEvent(timestamp, deviceData.device, deviceData.pointerType, deviceData.uid);
    }
}
#endif

- (bool)shouldSendSingleTouch
{
    if (!m_platformWindow)
        return true;

    // QtWidgets expects single-point touch events, QtDeclarative does not.
    // Until there is an API we solve this by looking at the window class type.
    return m_platformWindow->window()->inherits("QWidgetWindow");
}

- (void)touchesBeganWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    const NSTimeInterval timestamp = [event timestamp];
    const QList<QWindowSystemInterface::TouchPoint> points = QCocoaTouch::getCurrentTouchPointList(event, [self shouldSendSingleTouch]);
    qCDebug(lcQpaTouch) << "touchesBeganWithEvent" << points << "from device" << hex << [event deviceID];
    QWindowSystemInterface::handleTouchEvent(m_platformWindow->window(), timestamp * 1000, QCocoaTouch::getTouchDevice(QTouchDevice::TouchPad, [event deviceID]), points);
}

- (void)touchesMovedWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    const NSTimeInterval timestamp = [event timestamp];
    const QList<QWindowSystemInterface::TouchPoint> points = QCocoaTouch::getCurrentTouchPointList(event, [self shouldSendSingleTouch]);
    qCDebug(lcQpaTouch) << "touchesMovedWithEvent" << points << "from device" << hex << [event deviceID];
    QWindowSystemInterface::handleTouchEvent(m_platformWindow->window(), timestamp * 1000, QCocoaTouch::getTouchDevice(QTouchDevice::TouchPad, [event deviceID]), points);
}

- (void)touchesEndedWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    const NSTimeInterval timestamp = [event timestamp];
    const QList<QWindowSystemInterface::TouchPoint> points = QCocoaTouch::getCurrentTouchPointList(event, [self shouldSendSingleTouch]);
    qCDebug(lcQpaTouch) << "touchesEndedWithEvent" << points << "from device" << hex << [event deviceID];
    QWindowSystemInterface::handleTouchEvent(m_platformWindow->window(), timestamp * 1000, QCocoaTouch::getTouchDevice(QTouchDevice::TouchPad, [event deviceID]), points);
}

- (void)touchesCancelledWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    const NSTimeInterval timestamp = [event timestamp];
    const QList<QWindowSystemInterface::TouchPoint> points = QCocoaTouch::getCurrentTouchPointList(event, [self shouldSendSingleTouch]);
    qCDebug(lcQpaTouch) << "touchesCancelledWithEvent" << points << "from device" << hex << [event deviceID];
    QWindowSystemInterface::handleTouchEvent(m_platformWindow->window(), timestamp * 1000, QCocoaTouch::getTouchDevice(QTouchDevice::TouchPad, [event deviceID]), points);
}

#ifndef QT_NO_GESTURES

- (bool)handleGestureAsBeginEnd:(NSEvent *)event
{
    if (QOperatingSystemVersion::current() < QOperatingSystemVersion::OSXElCapitan)
        return false;

    if ([event phase] == NSEventPhaseBegan) {
        [self beginGestureWithEvent:event];
        return true;
    }

    if ([event phase] == NSEventPhaseEnded) {
        [self endGestureWithEvent:event];
        return true;
    }

    return false;
}
- (void)magnifyWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    if ([self handleGestureAsBeginEnd:event])
        return;

    qCDebug(lcQpaGestures) << "magnifyWithEvent" << [event magnification] << "from device" << hex << [event deviceID];
    const NSTimeInterval timestamp = [event timestamp];
    QPointF windowPoint;
    QPointF screenPoint;
    [self convertFromScreen:[self screenMousePoint:event] toWindowPoint:&windowPoint andScreenPoint:&screenPoint];
    QWindowSystemInterface::handleGestureEventWithRealValue(m_platformWindow->window(), QCocoaTouch::getTouchDevice(QTouchDevice::TouchPad, [event deviceID]), timestamp, Qt::ZoomNativeGesture,
                                                            [event magnification], windowPoint, screenPoint);
}

- (void)smartMagnifyWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    static bool zoomIn = true;
    qCDebug(lcQpaGestures) << "smartMagnifyWithEvent" << zoomIn << "from device" << hex << [event deviceID];
    const NSTimeInterval timestamp = [event timestamp];
    QPointF windowPoint;
    QPointF screenPoint;
    [self convertFromScreen:[self screenMousePoint:event] toWindowPoint:&windowPoint andScreenPoint:&screenPoint];
    QWindowSystemInterface::handleGestureEventWithRealValue(m_platformWindow->window(), QCocoaTouch::getTouchDevice(QTouchDevice::TouchPad, [event deviceID]), timestamp, Qt::SmartZoomNativeGesture,
                                                            zoomIn ? 1.0f : 0.0f, windowPoint, screenPoint);
    zoomIn = !zoomIn;
}

- (void)rotateWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    if ([self handleGestureAsBeginEnd:event])
        return;

    const NSTimeInterval timestamp = [event timestamp];
    QPointF windowPoint;
    QPointF screenPoint;
    [self convertFromScreen:[self screenMousePoint:event] toWindowPoint:&windowPoint andScreenPoint:&screenPoint];
    QWindowSystemInterface::handleGestureEventWithRealValue(m_platformWindow->window(), QCocoaTouch::getTouchDevice(QTouchDevice::TouchPad, [event deviceID]), timestamp, Qt::RotateNativeGesture,
                                                            -[event rotation], windowPoint, screenPoint);
}

- (void)swipeWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    qCDebug(lcQpaGestures) << "swipeWithEvent" << [event deltaX] << [event deltaY] << "from device" << hex << [event deviceID];
    const NSTimeInterval timestamp = [event timestamp];
    QPointF windowPoint;
    QPointF screenPoint;
    [self convertFromScreen:[self screenMousePoint:event] toWindowPoint:&windowPoint andScreenPoint:&screenPoint];

    qreal angle = 0.0f;
    if ([event deltaX] == 1)
        angle = 180.0f;
    else if ([event deltaX] == -1)
        angle = 0.0f;
    else if ([event deltaY] == 1)
        angle = 90.0f;
    else if ([event deltaY] == -1)
        angle = 270.0f;

    QWindowSystemInterface::handleGestureEventWithRealValue(m_platformWindow->window(), QCocoaTouch::getTouchDevice(QTouchDevice::TouchPad, [event deviceID]),  timestamp, Qt::SwipeNativeGesture,
                                                            angle, windowPoint, screenPoint);
}

- (void)beginGestureWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    const NSTimeInterval timestamp = [event timestamp];
    QPointF windowPoint;
    QPointF screenPoint;
    [self convertFromScreen:[self screenMousePoint:event] toWindowPoint:&windowPoint andScreenPoint:&screenPoint];
    qCDebug(lcQpaGestures) << "beginGestureWithEvent @" << windowPoint << "from device" << hex << [event deviceID];
    QWindowSystemInterface::handleGestureEvent(m_platformWindow->window(), QCocoaTouch::getTouchDevice(QTouchDevice::TouchPad, [event deviceID]),  timestamp, Qt::BeginNativeGesture,
                                               windowPoint, screenPoint);
}

- (void)endGestureWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    qCDebug(lcQpaGestures) << "endGestureWithEvent" << "from device" << hex << [event deviceID];
    const NSTimeInterval timestamp = [event timestamp];
    QPointF windowPoint;
    QPointF screenPoint;
    [self convertFromScreen:[self screenMousePoint:event] toWindowPoint:&windowPoint andScreenPoint:&screenPoint];
    QWindowSystemInterface::handleGestureEvent(m_platformWindow->window(), QCocoaTouch::getTouchDevice(QTouchDevice::TouchPad, [event deviceID]),  timestamp, Qt::EndNativeGesture,
                                               windowPoint, screenPoint);
}
#endif // QT_NO_GESTURES

#if QT_CONFIG(wheelevent)
- (void)scrollWheel:(NSEvent *)theEvent
{
    if (!m_platformWindow)
        return;

    if ([self isTransparentForUserInput])
        return [super scrollWheel:theEvent];

    QPoint angleDelta;
    Qt::MouseEventSource source = Qt::MouseEventNotSynthesized;
    if ([theEvent hasPreciseScrollingDeltas]) {
        // The mouse device contains pixel scroll wheel support (Mighty Mouse, Trackpad).
        // Since deviceDelta is delivered as pixels rather than degrees, we need to
        // convert from pixels to degrees in a sensible manner.
        // It looks like 1/4 degrees per pixel behaves most native.
        // (NB: Qt expects the unit for delta to be 8 per degree):
        const int pixelsToDegrees = 2; // 8 * 1/4
        angleDelta.setX([theEvent scrollingDeltaX] * pixelsToDegrees);
        angleDelta.setY([theEvent scrollingDeltaY] * pixelsToDegrees);
        source = Qt::MouseEventSynthesizedBySystem;
    } else {
        // Remove acceleration, and use either -120 or 120 as delta:
        angleDelta.setX(qBound(-120, int([theEvent deltaX] * 10000), 120));
        angleDelta.setY(qBound(-120, int([theEvent deltaY] * 10000), 120));
    }

    QPoint pixelDelta;
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

    QPointF qt_windowPoint;
    QPointF qt_screenPoint;
    [self convertFromScreen:[self screenMousePoint:theEvent] toWindowPoint:&qt_windowPoint andScreenPoint:&qt_screenPoint];
    NSTimeInterval timestamp = [theEvent timestamp];
    ulong qt_timestamp = timestamp * 1000;

    // Prevent keyboard modifier state from changing during scroll event streams.
    // A two-finger trackpad flick generates a stream of scroll events. We want
    // the keyboard modifier state to be the state at the beginning of the
    // flick in order to avoid changing the interpretation of the events
    // mid-stream. One example of this happening would be when pressing cmd
    // after scrolling in Qt Creator: not taking the phase into account causes
    // the end of the event stream to be interpreted as font size changes.
    NSEventPhase momentumPhase = [theEvent momentumPhase];
    if (momentumPhase == NSEventPhaseNone) {
        currentWheelModifiers = [QNSView convertKeyModifiers:[theEvent modifierFlags]];
    }

    NSEventPhase phase = [theEvent phase];
    Qt::ScrollPhase ph = Qt::ScrollUpdate;

    // MayBegin is likely to happen.  We treat it the same as an actual begin.
    if (phase == NSEventPhaseMayBegin) {
        m_scrolling = true;
        ph = Qt::ScrollBegin;
    } else if (phase == NSEventPhaseBegan) {
        // If MayBegin did not happen, Began is the actual beginning.
        if (!m_scrolling)
            ph = Qt::ScrollBegin;
        m_scrolling = true;
    } else if (phase == NSEventPhaseEnded || phase == NSEventPhaseCancelled ||
               momentumPhase == NSEventPhaseEnded || momentumPhase == NSEventPhaseCancelled) {
        ph = Qt::ScrollEnd;
        m_scrolling = false;
    } else if (phase == NSEventPhaseNone && momentumPhase == NSEventPhaseNone) {
        ph = Qt::NoScrollPhase;
    }
    // "isInverted": natural OS X scrolling, inverted from the Qt/other platform/Jens perspective.
    bool isInverted  = [theEvent isDirectionInvertedFromDevice];

    qCDebug(lcQpaCocoaMouse) << "scroll wheel @ window pos" << qt_windowPoint << "delta px" << pixelDelta << "angle" << angleDelta << "phase" << ph << (isInverted ? "inverted" : "");
    QWindowSystemInterface::handleWheelEvent(m_platformWindow->window(), qt_timestamp, qt_windowPoint, qt_screenPoint, pixelDelta, angleDelta, currentWheelModifiers, ph, source, isInverted);
}
#endif // QT_CONFIG(wheelevent)

- (int) convertKeyCode : (QChar)keyChar
{
    return qt_mac_cocoaKey2QtKey(keyChar);
}

+ (Qt::KeyboardModifiers) convertKeyModifiers : (ulong)modifierFlags
{
    const bool dontSwapCtrlAndMeta = qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta);
    Qt::KeyboardModifiers qtMods =Qt::NoModifier;
    if (modifierFlags &  NSShiftKeyMask)
        qtMods |= Qt::ShiftModifier;
    if (modifierFlags & NSControlKeyMask)
        qtMods |= dontSwapCtrlAndMeta ? Qt::ControlModifier : Qt::MetaModifier;
    if (modifierFlags & NSAlternateKeyMask)
        qtMods |= Qt::AltModifier;
    if (modifierFlags & NSCommandKeyMask)
        qtMods |= dontSwapCtrlAndMeta ? Qt::MetaModifier : Qt::ControlModifier;
    if (modifierFlags & NSNumericPadKeyMask)
        qtMods |= Qt::KeypadModifier;
    return qtMods;
}

- (bool)handleKeyEvent:(NSEvent *)nsevent eventType:(int)eventType
{
    ulong timestamp = [nsevent timestamp] * 1000;
    ulong nativeModifiers = [nsevent modifierFlags];
    Qt::KeyboardModifiers modifiers = [QNSView convertKeyModifiers: nativeModifiers];
    NSString *charactersIgnoringModifiers = [nsevent charactersIgnoringModifiers];
    NSString *characters = [nsevent characters];
    if (m_inputSource != characters) {
        [m_inputSource release];
        m_inputSource = [characters retain];
    }

    // There is no way to get the scan code from carbon/cocoa. But we cannot
    // use the value 0, since it indicates that the event originates from somewhere
    // else than the keyboard.
    quint32 nativeScanCode = 1;
    quint32 nativeVirtualKey = [nsevent keyCode];

    QChar ch = QChar::ReplacementCharacter;
    int keyCode = Qt::Key_unknown;

    // If a dead key occurs as a result of pressing a key combination then
    // characters will have 0 length, but charactersIgnoringModifiers will
    // have a valid character in it. This enables key combinations such as
    // ALT+E to be used as a shortcut with an English keyboard even though
    // pressing ALT+E will give a dead key while doing normal text input.
    if ([characters length] != 0 || [charactersIgnoringModifiers length] != 0) {
        auto ctrlOrMetaModifier = qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta) ? Qt::ControlModifier : Qt::MetaModifier;
        if (((modifiers & ctrlOrMetaModifier) || (modifiers & Qt::AltModifier)) && ([charactersIgnoringModifiers length] != 0))
            ch = QChar([charactersIgnoringModifiers characterAtIndex:0]);
        else if ([characters length] != 0)
            ch = QChar([characters characterAtIndex:0]);
        keyCode = [self convertKeyCode:ch];
    }

    // we will send a key event unless the input method sets m_sendKeyEvent to false
    m_sendKeyEvent = true;
    QString text;
    // ignore text for the U+F700-U+F8FF range. This is used by Cocoa when
    // delivering function keys (e.g. arrow keys, backspace, F1-F35, etc.)
    if (!(modifiers & (Qt::ControlModifier | Qt::MetaModifier)) && (ch.unicode() < 0xf700 || ch.unicode() > 0xf8ff))
        text = QString::fromNSString(characters);

    QWindow *window = [self topLevelWindow];

    // Popups implicitly grab key events; forward to the active popup if there is one.
    // This allows popups to e.g. intercept shortcuts and close the popup in response.
    if (QCocoaWindow *popup = QCocoaIntegration::instance()->activePopupWindow()) {
        if (!popup->m_windowFlags.testFlag(Qt::ToolTip))
            window = popup->window();
    }

    if (eventType == QEvent::KeyPress) {

        if (m_composingText.isEmpty()) {
            m_sendKeyEvent = !QWindowSystemInterface::handleShortcutEvent(window, timestamp, keyCode,
                modifiers, nativeScanCode, nativeVirtualKey, nativeModifiers, text, [nsevent isARepeat], 1);

            // Handling a shortcut may result in closing the window
            if (!m_platformWindow)
                return true;
        }

        QObject *fo = m_platformWindow->window()->focusObject();
        if (m_sendKeyEvent && fo) {
            QInputMethodQueryEvent queryEvent(Qt::ImEnabled | Qt::ImHints);
            if (QCoreApplication::sendEvent(fo, &queryEvent)) {
                bool imEnabled = queryEvent.value(Qt::ImEnabled).toBool();
                Qt::InputMethodHints hints = static_cast<Qt::InputMethodHints>(queryEvent.value(Qt::ImHints).toUInt());
                if (imEnabled && !(hints & Qt::ImhDigitsOnly || hints & Qt::ImhFormattedNumbersOnly || hints & Qt::ImhHiddenText)) {
                    // pass the key event to the input method. note that m_sendKeyEvent may be set to false during this call
                    m_currentlyInterpretedKeyEvent = nsevent;
                    [self interpretKeyEvents:[NSArray arrayWithObject:nsevent]];
                    m_currentlyInterpretedKeyEvent = 0;
                }
            }
        }
        if (m_resendKeyEvent)
            m_sendKeyEvent = true;
    }

    bool accepted = true;
    if (m_sendKeyEvent && m_composingText.isEmpty()) {
        QWindowSystemInterface::handleExtendedKeyEvent(window, timestamp, QEvent::Type(eventType), keyCode, modifiers,
                                                       nativeScanCode, nativeVirtualKey, nativeModifiers, text, [nsevent isARepeat], 1, false);
        accepted = QWindowSystemInterface::flushWindowSystemEvents();
    }
    m_sendKeyEvent = false;
    m_resendKeyEvent = false;
    return accepted;
}

- (void)keyDown:(NSEvent *)nsevent
{
    if ([self isTransparentForUserInput])
        return [super keyDown:nsevent];

    const bool accepted = [self handleKeyEvent:nsevent eventType:int(QEvent::KeyPress)];

    // When Qt is used to implement a plugin for a native application we
    // want to propagate unhandled events to other native views. However,
    // Qt does not always set the accepted state correctly (in particular
    // for return key events), so do this for plugin applications only
    // to prevent incorrect forwarding in the general case.
    const bool shouldPropagate = QCoreApplication::testAttribute(Qt::AA_PluginApplication) && !accepted;

    // Track keyDown acceptance/forward state for later acceptance of the keyUp.
    if (!shouldPropagate)
        m_acceptedKeyDowns.insert([nsevent keyCode]);

    if (shouldPropagate)
        [super keyDown:nsevent];
}

- (void)keyUp:(NSEvent *)nsevent
{
    if ([self isTransparentForUserInput])
        return [super keyUp:nsevent];

    const bool keyUpAccepted = [self handleKeyEvent:nsevent eventType:int(QEvent::KeyRelease)];

    // Propagate the keyUp if neither Qt accepted it nor the corresponding KeyDown was
    // accepted. Qt text controls wil often not use and ignore keyUp events, but we
    // want to avoid propagating unmatched keyUps.
    const bool keyDownAccepted = m_acceptedKeyDowns.remove([nsevent keyCode]);
    if (!keyUpAccepted && !keyDownAccepted)
        [super keyUp:nsevent];
}

- (void)cancelOperation:(id)sender
{
    Q_UNUSED(sender);

    NSEvent *currentEvent = [NSApp currentEvent];
    if (!currentEvent || currentEvent.type != NSKeyDown)
        return;

    // Handling the key event may recurse back here through interpretKeyEvents
    // (when IM is enabled), so we need to guard against that.
    if (currentEvent == m_currentlyInterpretedKeyEvent)
        return;

    // Send Command+Key_Period and Escape as normal keypresses so that
    // the key sequence is delivered through Qt. That way clients can
    // intercept the shortcut and override its effect.
    [self handleKeyEvent:currentEvent eventType:int(QEvent::KeyPress)];
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

        Qt::Key qtCode = modifier_key_symbols[i].qt_code;
        if (qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta)) {
            if (qtCode == Qt::Key_Meta)
                qtCode = Qt::Key_Control;
            else if (qtCode == Qt::Key_Control)
                qtCode = Qt::Key_Meta;
        }
        QWindowSystemInterface::handleKeyEvent(m_platformWindow->window(),
                                               timestamp,
                                               (lastKnownModifiers & mac_mask) ? QEvent::KeyRelease : QEvent::KeyPress,
                                               qtCode,
                                               qmodifiers ^ [QNSView convertKeyModifiers:mac_mask]);
    }
}

- (void) insertNewline:(id)sender
{
    Q_UNUSED(sender);
    m_resendKeyEvent = true;
}

- (void) doCommandBySelector:(SEL)aSelector
{
    [self tryToPerform:aSelector with:self];
}

- (void) insertText:(id)aString replacementRange:(NSRange)replacementRange
{
    Q_UNUSED(replacementRange)

    if (m_sendKeyEvent && m_composingText.isEmpty() && [aString isEqualToString:m_inputSource]) {
        // don't send input method events for simple text input (let handleKeyEvent send key events instead)
        return;
    }

    QString commitString;
    if ([aString length]) {
        if ([aString isKindOfClass:[NSAttributedString class]]) {
            commitString = QString::fromCFString(reinterpret_cast<CFStringRef>([aString string]));
        } else {
            commitString = QString::fromCFString(reinterpret_cast<CFStringRef>(aString));
        };
    }
    if (QObject *fo = m_platformWindow->window()->focusObject()) {
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
    m_composingFocusObject = nullptr;
}

- (void) setMarkedText:(id)aString selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange
{
    Q_UNUSED(replacementRange)
    QString preeditString;

    QList<QInputMethodEvent::Attribute> attrs;
    attrs<<QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, selectedRange.location + selectedRange.length, 1, QVariant());

    if ([aString isKindOfClass:[NSAttributedString class]]) {
        // Preedit string has attribution
        preeditString = QString::fromCFString(reinterpret_cast<CFStringRef>([aString string]));
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
        preeditString = QString::fromCFString(reinterpret_cast<CFStringRef>(aString));
    }

    if (attrs.isEmpty()) {
        QTextCharFormat format;
        format.setFontUnderline(true);
        attrs<<QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat,
                                            0, preeditString.length(), format);
    }

    m_composingText = preeditString;

    if (QObject *fo = m_platformWindow->window()->focusObject()) {
        m_composingFocusObject = fo;
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

- (void)cancelComposingText
{
    if (m_composingText.isEmpty())
        return;

    if (m_composingFocusObject) {
        QInputMethodQueryEvent queryEvent(Qt::ImEnabled);
        if (QCoreApplication::sendEvent(m_composingFocusObject, &queryEvent)) {
            if (queryEvent.value(Qt::ImEnabled).toBool()) {
                QInputMethodEvent e;
                QCoreApplication::sendEvent(m_composingFocusObject, &e);
            }
        }
    }

    m_composingText.clear();
    m_composingFocusObject = nullptr;
}

- (void) unmarkText
{
    if (!m_composingText.isEmpty()) {
        if (QObject *fo = m_platformWindow->window()->focusObject()) {
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
    m_composingFocusObject = nullptr;
}

- (BOOL) hasMarkedText
{
    return (m_composingText.isEmpty() ? NO: YES);
}

- (NSAttributedString *) attributedSubstringForProposedRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange
{
    Q_UNUSED(actualRange)
    QObject *fo = m_platformWindow->window()->focusObject();
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
    NSRange selectedRange = {0, 0};

    QObject *fo = m_platformWindow->window()->focusObject();
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
    QObject *fo = m_platformWindow->window()->focusObject();
    if (!fo)
        return NSZeroRect;

    QInputMethodQueryEvent queryEvent(Qt::ImEnabled);
    if (!QCoreApplication::sendEvent(fo, &queryEvent))
        return NSZeroRect;
    if (!queryEvent.value(Qt::ImEnabled).toBool())
        return NSZeroRect;

    if (!m_platformWindow->window())
        return NSZeroRect;

    // The returned rect is always based on the internal cursor.
    QRect mr = qApp->inputMethod()->cursorRectangle().toRect();
    mr.moveBottomLeft(m_platformWindow->window()->mapToGlobal(mr.bottomLeft()));
    return QCocoaScreen::mapToNative(mr);
}

- (NSUInteger)characterIndexForPoint:(NSPoint)aPoint
{
    // We don't support cursor movements using mouse while composing.
    Q_UNUSED(aPoint);
    return NSNotFound;
}

- (NSArray*)validAttributesForMarkedText
{
    if (!m_platformWindow)
        return nil;

    if (m_platformWindow->window() != QGuiApplication::focusWindow())
        return nil;

    QObject *fo = m_platformWindow->window()->focusObject();
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
    QMacAutoReleasePool pool;
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
           [supportedTypes addObject:customTypes[i].toNSString()];
        }
        [self registerForDraggedTypes:supportedTypes];
    }
}

static QWindow *findEventTargetWindow(QWindow *candidate)
{
    while (candidate) {
        if (!(candidate->flags() & Qt::WindowTransparentForInput))
            return candidate;
        candidate = candidate->parent();
    }
    return candidate;
}

static QPoint mapWindowCoordinates(QWindow *source, QWindow *target, QPoint point)
{
    return target->mapFromGlobal(source->mapToGlobal(point));
}

- (NSDragOperation)draggingSession:(NSDraggingSession *)session
  sourceOperationMaskForDraggingContext:(NSDraggingContext)context
{
    Q_UNUSED(session);
    Q_UNUSED(context);
    QCocoaDrag* nativeDrag = QCocoaIntegration::instance()->drag();
    return qt_mac_mapDropActions(nativeDrag->currentDrag()->supportedActions());
}

- (BOOL)ignoreModifierKeysForDraggingSession:(NSDraggingSession *)session
{
    Q_UNUSED(session);
    // According to the "Dragging Sources" chapter on Cocoa DnD Programming
    // (https://developer.apple.com/library/mac/documentation/Cocoa/Conceptual/DragandDrop/Concepts/dragsource.html),
    // if the control, option, or command key is pressed, the source’s
    // operation mask is filtered to only contain a reduced set of operations.
    //
    // Since Qt already takes care of tracking the keyboard modifiers, we
    // don't need (or want) Cocoa to filter anything. Instead, we'll let
    // the application do the actual filtering.
    return YES;
}

- (BOOL)wantsPeriodicDraggingUpdates
{
    // From the documentation:
    //
    // "If the destination returns NO, these messages are sent only when the mouse moves
    //  or a modifier flag changes. Otherwise the destination gets the default behavior,
    //  where it receives periodic dragging-updated messages even if nothing changes."
    //
    // We do not want these constant drag update events while mouse is stationary,
    // since we do all animations (autoscroll) with timers.
    return NO;
}


- (BOOL)wantsPeriodicDraggingUpdates:(void *)dummy
{
    // This method never gets called. It's a workaround for Apple's
    // bug: they first respondsToSelector : @selector(wantsPeriodicDraggingUpdates:)
    // (note ':') and then call -wantsPeriodicDraggingUpdate (without colon).
    // So, let's make them happy.
    Q_UNUSED(dummy);

    return NO;
}

- (void)updateCursorFromDragResponse:(QPlatformDragQtResponse)response drag:(QCocoaDrag *)drag
{
    const QPixmap pixmapCursor = drag->currentDrag()->dragCursor(response.acceptedAction());
    NSCursor *nativeCursor = nil;

    if (pixmapCursor.isNull()) {
        switch (response.acceptedAction()) {
            case Qt::CopyAction:
                nativeCursor = [NSCursor dragCopyCursor];
                break;
            case Qt::LinkAction:
                nativeCursor = [NSCursor dragLinkCursor];
                break;
            case Qt::IgnoreAction:
                // Uncomment the next lines if forbiden cursor wanted on non droppable targets.
                /*nativeCursor = [NSCursor operationNotAllowedCursor];
                break;*/
            case Qt::MoveAction:
            default:
                nativeCursor = [NSCursor arrowCursor];
                break;
        }
    }
    else {
        NSImage *nsimage = qt_mac_create_nsimage(pixmapCursor);
        nsimage.size = NSSizeFromCGSize((pixmapCursor.size() / pixmapCursor.devicePixelRatioF()).toCGSize());
        nativeCursor = [[NSCursor alloc] initWithImage:nsimage hotSpot:NSZeroPoint];
        [nsimage release];
    }

    // change the cursor
    [nativeCursor set];

    // Make sure the cursor is updated correctly if the mouse does not move and window is under cursor
    // by creating a fake move event, unless on 10.14 and later where doing so will trigger a security
    // warning dialog. FIXME: Find a way to update the cursor without fake mouse events.
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSMojave)
        return;

    if (m_updatingDrag)
        return;

    const QPoint mousePos(QCursor::pos());
    CGEventRef moveEvent(CGEventCreateMouseEvent(
        NULL, kCGEventMouseMoved,
        CGPointMake(mousePos.x(), mousePos.y()),
        kCGMouseButtonLeft // ignored
    ));
    CGEventPost(kCGHIDEventTap, moveEvent);
    CFRelease(moveEvent);
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender
{
    return [self handleDrag : sender];
}

- (NSDragOperation)draggingUpdated:(id <NSDraggingInfo>)sender
{
    m_updatingDrag = true;
    const NSDragOperation ret([self handleDrag : sender]);
    m_updatingDrag = false;

    return ret;
}

// Sends drag update to Qt, return the action
- (NSDragOperation)handleDrag:(id <NSDraggingInfo>)sender
{
    if (!m_platformWindow)
        return NSDragOperationNone;

    NSPoint windowPoint = [self convertPoint: [sender draggingLocation] fromView: nil];
    QPoint qt_windowPoint(windowPoint.x, windowPoint.y);
    Qt::DropActions qtAllowed = qt_mac_mapNSDragOperations([sender draggingSourceOperationMask]);

    QWindow *target = findEventTargetWindow(m_platformWindow->window());
    if (!target)
        return NSDragOperationNone;

    // update these so selecting move/copy/link works
    QGuiApplicationPrivate::modifier_buttons = [QNSView convertKeyModifiers: [[NSApp currentEvent] modifierFlags]];

    QPlatformDragQtResponse response(false, Qt::IgnoreAction, QRect());
    QCocoaDrag* nativeDrag = QCocoaIntegration::instance()->drag();
    if (nativeDrag->currentDrag()) {
        // The drag was started from within the application
        response = QWindowSystemInterface::handleDrag(target, nativeDrag->dragMimeData(), mapWindowCoordinates(m_platformWindow->window(), target, qt_windowPoint), qtAllowed);
        [self updateCursorFromDragResponse:response drag:nativeDrag];
    } else {
        QCocoaDropData mimeData([sender draggingPasteboard]);
        response = QWindowSystemInterface::handleDrag(target, &mimeData, mapWindowCoordinates(m_platformWindow->window(), target, qt_windowPoint), qtAllowed);
    }

    return qt_mac_mapDropAction(response.acceptedAction());
}

- (void)draggingExited:(id <NSDraggingInfo>)sender
{
    if (!m_platformWindow)
        return;

    QWindow *target = findEventTargetWindow(m_platformWindow->window());
    if (!target)
        return;

    NSPoint windowPoint = [self convertPoint: [sender draggingLocation] fromView: nil];
    QPoint qt_windowPoint(windowPoint.x, windowPoint.y);

    // Send 0 mime data to indicate drag exit
    QWindowSystemInterface::handleDrag(target, 0, mapWindowCoordinates(m_platformWindow->window(), target, qt_windowPoint), Qt::IgnoreAction);
}

// called on drop, send the drop to Qt and return if it was accepted.
- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
    if (!m_platformWindow)
        return false;

    QWindow *target = findEventTargetWindow(m_platformWindow->window());
    if (!target)
        return false;

    NSPoint windowPoint = [self convertPoint: [sender draggingLocation] fromView: nil];
    QPoint qt_windowPoint(windowPoint.x, windowPoint.y);
    Qt::DropActions qtAllowed = qt_mac_mapNSDragOperations([sender draggingSourceOperationMask]);

    QPlatformDropQtResponse response(false, Qt::IgnoreAction);
    QCocoaDrag* nativeDrag = QCocoaIntegration::instance()->drag();
    if (nativeDrag->currentDrag()) {
        // The drag was started from within the application
        response = QWindowSystemInterface::handleDrop(target, nativeDrag->dragMimeData(), mapWindowCoordinates(m_platformWindow->window(), target, qt_windowPoint), qtAllowed);
    } else {
        QCocoaDropData mimeData([sender draggingPasteboard]);
        response = QWindowSystemInterface::handleDrop(target, &mimeData, mapWindowCoordinates(m_platformWindow->window(), target, qt_windowPoint), qtAllowed);
    }
    if (response.isAccepted()) {
        QCocoaDrag* nativeDrag = QCocoaIntegration::instance()->drag();
        nativeDrag->setAcceptedAction(response.acceptedAction());
    }
    return response.isAccepted();
}

- (void)draggingSession:(NSDraggingSession *)session
           endedAtPoint:(NSPoint)screenPoint
              operation:(NSDragOperation)operation
{
    Q_UNUSED(session);
    Q_UNUSED(operation);

    if (!m_platformWindow)
        return;

    QWindow *target = findEventTargetWindow(m_platformWindow->window());
    if (!target)
        return;

    // keep our state, and QGuiApplication state (buttons member) in-sync,
    // or future mouse events will be processed incorrectly
    NSUInteger pmb = [NSEvent pressedMouseButtons];
    for (int buttonNumber = 0; buttonNumber < 32; buttonNumber++) { // see cocoaButton2QtButton() for the 32 value
        if (!(pmb & (1 << buttonNumber)))
            m_buttons &= ~cocoaButton2QtButton(buttonNumber);
    }

    NSPoint windowPoint = [self.window convertRectFromScreen:NSMakeRect(screenPoint.x, screenPoint.y, 1, 1)].origin;
    NSPoint nsViewPoint = [self convertPoint: windowPoint fromView: nil]; // NSView/QWindow coordinates
    QPoint qtWindowPoint(nsViewPoint.x, nsViewPoint.y);
    QPoint qtScreenPoint = QCocoaScreen::mapFromNative(screenPoint).toPoint();

    QWindowSystemInterface::handleMouseEvent(target, mapWindowCoordinates(m_platformWindow->window(), target, qtWindowPoint), qtScreenPoint, m_buttons);
}

@end

@implementation QT_MANGLE_NAMESPACE(QNSView) (QtExtras)

- (QCocoaWindow*)platformWindow
{
    return m_platformWindow.data();;
}

- (BOOL)isMenuView
{
    return m_isMenuView;
}

@end
