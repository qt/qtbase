/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

// This file is included from qnsview.mm, and only used to organize the code

@implementation QT_MANGLE_NAMESPACE(QNSViewMouseMoveHelper) {
    QNSView *view;
}

- (instancetype)initWithView:(QNSView *)theView
{
    if ((self = [super init]))
        view = theView;

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

@implementation QT_MANGLE_NAMESPACE(QNSView) (MouseAPI)

- (void)resetMouseButtons
{
    m_buttons = Qt::NoButton;
    m_frameStrutButtons = Qt::NoButton;
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
@end

@implementation QT_MANGLE_NAMESPACE(QNSView) (Mouse)

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
    Q_UNUSED(theEvent)
    if (!m_platformWindow)
        return NO;
    if ([self isTransparentForUserInput])
        return NO;
    return YES;
}

- (NSPoint)screenMousePoint:(NSEvent *)theEvent
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
        if (!popup->window()->flags().testFlag(Qt::ToolTip)) {
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
    qCDebug(lcQpaMouse) << "[QNSView cursorUpdate:]" << self.cursor;

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

    qCDebug(lcQpaMouse) << "scroll wheel @ window pos" << qt_windowPoint << "delta px" << pixelDelta << "angle" << angleDelta << "phase" << ph << (isInverted ? "inverted" : "");
    QWindowSystemInterface::handleWheelEvent(m_platformWindow->window(), qt_timestamp, qt_windowPoint, qt_screenPoint, pixelDelta, angleDelta, currentWheelModifiers, ph, source, isInverted);
}
#endif // QT_CONFIG(wheelevent)

@end
