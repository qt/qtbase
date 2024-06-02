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

/*
    The reason for using this helper is to ensure that QNSView doesn't implement
    the NSResponder callbacks for mouseEntered, mouseExited, and mouseMoved.

    If it did, we would get mouse events though the responder chain as well,
    for example if a subview has a tracking area of its own and calls super
    in the handler, which results in forwarding the event though the responder
    chain. The same applies if NSWindow.acceptsMouseMovedEvents is YES.

    By having a helper as the target for our tracking areas, we know for sure
    that the events we are getting stem from our own tracking areas.

    FIXME: Ideally we wouldn't need this workaround, and would correctly
    interact with the responder chain by e.g. calling super if Qt does not
    accept the mouse event
*/
@implementation QNSViewMouseMoveHelper {
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

@implementation QNSView (MouseAPI)

- (void)resetMouseButtons
{
    qCDebug(lcQpaMouse) << "Resetting mouse buttons";
    m_buttons = Qt::NoButton;
    m_frameStrutButtons = Qt::NoButton;
}

- (void)handleFrameStrutMouseEvent:(NSEvent *)theEvent
{
    if (!m_platformWindow)
        return;

    switch (theEvent.type) {
    case NSEventTypeLeftMouseDown:
        m_frameStrutButtons |= Qt::LeftButton;
        break;
    case NSEventTypeLeftMouseUp:
         m_frameStrutButtons &= ~Qt::LeftButton;
         break;
    case NSEventTypeRightMouseDown:
        m_frameStrutButtons |= Qt::RightButton;
        break;
    case NSEventTypeRightMouseUp:
        m_frameStrutButtons &= ~Qt::RightButton;
        break;
    case NSEventTypeOtherMouseDown:
        m_frameStrutButtons |= cocoaButton2QtButton(theEvent.buttonNumber);
        break;
    case NSEventTypeOtherMouseUp:
        m_frameStrutButtons &= ~cocoaButton2QtButton(theEvent.buttonNumber);
    default:
        break;
    }

    // m_buttons can sometimes get out of sync with the button state in AppKit
    // E.g if the QNSView where a drag starts is reparented to another window
    // while the drag is ongoing, it will not get the corresponding mouseUp
    // call. This will result in m_buttons to be stuck on Qt::LeftButton.
    // Since we know which buttons was pressed/released directly on the frame
    // strut, we can rectify m_buttons here so that we at least don't return early
    // from the drag test underneath because of the faulty m_buttons state.
    // FIXME: get m_buttons in sync with AppKit/NSEvent all over in QNSView.
    m_buttons &= ~m_frameStrutButtons;

    if (m_buttons != Qt::NoButton) {
        // Don't send frame strut events if we are in the middle of
        // a mouse drag that didn't start on the frame strut.
        return;
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

    const auto button = cocoaButton2QtButton(theEvent);
    auto eventType = [&]() {
        switch (theEvent.type) {
        case NSEventTypeLeftMouseDown:
        case NSEventTypeRightMouseDown:
        case NSEventTypeOtherMouseDown:
            return QEvent::NonClientAreaMouseButtonPress;

        case NSEventTypeLeftMouseUp:
        case NSEventTypeRightMouseUp:
        case NSEventTypeOtherMouseUp:
            return QEvent::NonClientAreaMouseButtonRelease;

        case NSEventTypeMouseMoved:
        case NSEventTypeLeftMouseDragged:
        case NSEventTypeRightMouseDragged:
        case NSEventTypeOtherMouseDragged:
            return QEvent::NonClientAreaMouseMove;

        default:
            Q_UNREACHABLE();
        }
    }();

    qCInfo(lcQpaMouse) << eventType << "at" << qtWindowPoint << "with" << m_frameStrutButtons << "in" << self.window;
    QWindowSystemInterface::handleFrameStrutMouseEvent(m_platformWindow->window(),
        timestamp, qtWindowPoint, qtScreenPoint, m_frameStrutButtons, button, eventType);
}

- (bool)closePopups:(NSEvent *)theEvent
{
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
            bool selfClosed = false;
            Qt::WindowType type = QCocoaIntegration::instance()->activePopupWindow()->window()->type();
            while (QCocoaWindow *popup = QCocoaIntegration::instance()->popPopupWindow()) {
                selfClosed = self == popup->view();
                QWindowSystemInterface::handleCloseEvent<QWindowSystemInterface::SynchronousDelivery>(popup->window());
                if (!m_platformWindow)
                    return true; // Bail out if window was destroyed
            }
            // Consume the mouse event when closing the popup, except for tool tips
            // were it's expected that the event is processed normally.
            if (type != Qt::ToolTip || selfClosed)
                 return true;
        }
    }
    return false;
}
@end

static const QPointingDevice *pointingDeviceFor(qint64 deviceID)
{
    // macOS will in many cases not report a deviceID (0 value).
    // We can't pass this on directly, as the QInputDevicePrivate
    // constructor will treat this as a request to assign a new Id.
    // Instead we use the default Id of the primary pointing device.
    static const int kDefaultPrimaryPointingDeviceId = 1;
    if (!deviceID)
        deviceID = kDefaultPrimaryPointingDeviceId;

    if (const auto *device = QPointingDevicePrivate::pointingDeviceById(deviceID))
        return device; // All good, already have the device registered

    const auto *primaryDevice = QPointingDevice::primaryPointingDevice();
    if (primaryDevice->systemId() == kDefaultPrimaryPointingDeviceId) {
        // Adopt existing primary device instead of creating a new one
        QPointingDevicePrivate::get(const_cast<QPointingDevice *>(primaryDevice))->systemId = deviceID;
        qCDebug(lcInputDevices) << "primaryPointingDevice is now" << primaryDevice;
        return primaryDevice;
    } else {
        // Register a new device. Name and capabilities may need updating later.
        const auto *device = new QPointingDevice(QLatin1String("mouse"), deviceID,
            QInputDevice::DeviceType::Mouse, QPointingDevice::PointerType::Generic,
            QInputDevice::Capability::Scroll | QInputDevice::Capability::Position,
            1, 3, QString(), QPointingDeviceUniqueId(), QCocoaIntegration::instance());
        QWindowSystemInterface::registerInputDevice(device);
        return device;
    }
}

@implementation QNSView (Mouse)

- (void)initMouse
{
    m_buttons = Qt::NoButton;
    m_acceptedMouseDowns = Qt::NoButton;
    m_frameStrutButtons = Qt::NoButton;

    m_scrolling = false;
    self.cursor = nil;

    m_sendUpAsRightButton = false;
    m_dontOverrideCtrlLMB = qt_mac_resolveOption(false, m_platformWindow->window(),
            "_q_platform_MacDontOverrideCtrlLMB", "QT_MAC_DONT_OVERRIDE_CTRL_LMB");

    m_mouseMoveHelper = [[QNSViewMouseMoveHelper alloc] initWithView:self];

    NSUInteger trackingOptions = NSTrackingActiveInActiveApp
        | NSTrackingMouseEnteredAndExited | NSTrackingCursorUpdate;

    // Ideally, NSTrackingMouseMoved should be turned on only if QWidget::mouseTracking
    // is enabled, hover is on, or a tool tip is set. Unfortunately, Qt will send "tooltip"
    // events on mouse moves, so we need to turn it on in ALL case. That means EVERY QWindow
    // gets to pay the cost of mouse moves delivered to it (Apple recommends keeping it OFF
    // because there is a performance hit).
    trackingOptions |= NSTrackingMouseMoved;

    // Using NSTrackingInVisibleRect means AppKit will automatically synchronize the
    // tracking rect with changes in the view's visible area, so leave it undefined.
    trackingOptions |= NSTrackingInVisibleRect;
    static const NSRect trackingRect = NSZeroRect;

    QMacAutoReleasePool pool;
    [self addTrackingArea:[[[NSTrackingArea alloc] initWithRect:trackingRect
        options:trackingOptions owner:m_mouseMoveHelper userInfo:nil] autorelease]];
}

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
    Q_UNUSED(theEvent);
    if (!m_platformWindow)
        return NO;
    if ([self isTransparentForUserInput])
        return NO;
    QPointF windowPoint;
    QPointF screenPoint;
    [self convertFromScreen:[NSEvent mouseLocation] toWindowPoint: &windowPoint andScreenPoint: &screenPoint];
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

    const auto modifiers = QAppleKeyMapper::fromCocoaModifiers(theEvent.modifierFlags);
    auto button = cocoaButton2QtButton(theEvent);
    if (button == Qt::LeftButton && m_sendUpAsRightButton)
        button = Qt::RightButton;
    const auto eventType = cocoaEvent2QtMouseEvent(theEvent);

    const QPointingDevice *device = pointingDeviceFor(theEvent.deviceID);
    Q_ASSERT(device);

    if (eventType == QEvent::MouseMove)
        qCDebug(lcQpaMouse) << eventType << "at" << qtWindowPoint << "with" << m_buttons;
    else
        qCInfo(lcQpaMouse) << eventType << "of" << button << "at" << qtWindowPoint << "with" << m_buttons;

    QWindowSystemInterface::handleMouseEvent(targetView->m_platformWindow->window(),
                                             timestamp, qtWindowPoint, qtScreenPoint,
                                             m_buttons, button, eventType, modifiers);
}

- (bool)handleMouseDownEvent:(NSEvent *)theEvent
{
    if ([self isTransparentForUserInput])
        return false;

    const auto button = cocoaButton2QtButton(theEvent);

    QPointF qtWindowPoint;
    QPointF qtScreenPoint;
    [self convertFromScreen:[self screenMousePoint:theEvent] toWindowPoint:&qtWindowPoint andScreenPoint:&qtScreenPoint];
    Q_UNUSED(qtScreenPoint);

    // Maintain masked state for the button for use by MouseDragged and MouseUp.
    QRegion mask = QHighDpi::toNativeLocalPosition(m_platformWindow->window()->mask(), m_platformWindow->window());
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

- (bool)handleMouseDraggedEvent:(NSEvent *)theEvent
{
    if ([self isTransparentForUserInput])
        return false;

    const auto button = cocoaButton2QtButton(theEvent);

    // Forward the event to the next responder if Qt did not accept the
    // corresponding mouse down for this button
    if (!(m_acceptedMouseDowns & button) == button)
        return false;

    [self handleMouseEvent:theEvent];
    return true;
}

- (bool)handleMouseUpEvent:(NSEvent *)theEvent
{
    if ([self isTransparentForUserInput])
        return false;

    auto button = cocoaButton2QtButton(theEvent);

    // Forward the event to the next responder if Qt did not accept the
    // corresponding mouse down for this button
    if (!(m_acceptedMouseDowns & button) == button)
        return false;

    if (m_sendUpAsRightButton && button == Qt::LeftButton)
        button = Qt::RightButton;

    m_buttons &= ~button;

    [self handleMouseEvent:theEvent];

    if (button == Qt::RightButton)
        m_sendUpAsRightButton = false;

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
    if ([self closePopups:theEvent])
        return;

    QPointF qtWindowPoint;
    QPointF qtScreenPoint;
    [self convertFromScreen:[self screenMousePoint:theEvent] toWindowPoint:&qtWindowPoint andScreenPoint:&qtScreenPoint];
    Q_UNUSED(qtScreenPoint);

    QRegion mask = QHighDpi::toNativeLocalPosition(m_platformWindow->window()->mask(), m_platformWindow->window());
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

    // FIXME: AppKit transfers first responder to the view before calling mouseDown,
    // whereas we only transfer focus once the mouse press is delivered, which means
    // on first click the focus item won't be the correct one when transferring focus.
    auto *focusObject = m_platformWindow->window()->focusObject();
    if (queryInputMethod(focusObject)) {
        // Input method is enabled. Pass on to the input context if we
        // are hitting the input item.
        if (QPlatformInputContext::inputItemClipRectangle().contains(qtWindowPoint)) {
            qCDebug(lcQpaInputMethods) << "Asking input context to handle mouse press"
                << "for focus object" << focusObject;
            if ([NSTextInputContext.currentInputContext handleEvent:theEvent]) {
                // NSTextView bails out if the input context handled the event,
                // which is e.g. the case for 2-Set Korean input. We follow suit,
                // even if that means having to click twice to move the cursor
                // for these input methods when they are composing.
                qCDebug(lcQpaInputMethods) << "Input context handled event; bailing out.";
                return;
            }
        }
    }

    if (!m_dontOverrideCtrlLMB && (theEvent.modifierFlags & NSEventModifierFlagControl)) {
        m_buttons |= Qt::RightButton;
        m_sendUpAsRightButton = true;
    } else {
        m_buttons |= Qt::LeftButton;
    }

    [self handleMouseEvent:theEvent];
}

- (void)mouseDragged:(NSEvent *)theEvent
{
    const bool accepted = [self handleMouseDraggedEvent:theEvent];
    if (!accepted)
        [super mouseDragged:theEvent];
}

- (void)mouseUp:(NSEvent *)theEvent
{
    const bool accepted = [self handleMouseUpEvent:theEvent];
    if (!accepted)
        [super mouseUp:theEvent];
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
    const bool accepted = [self handleMouseDownEvent:theEvent];
    if (!accepted)
        [super rightMouseDown:theEvent];
}

- (void)rightMouseDragged:(NSEvent *)theEvent
{
    const bool accepted = [self handleMouseDraggedEvent:theEvent];
    if (!accepted)
        [super rightMouseDragged:theEvent];
}

- (void)rightMouseUp:(NSEvent *)theEvent
{
    const bool accepted = [self handleMouseUpEvent:theEvent];
    if (!accepted)
        [super rightMouseUp:theEvent];
}

- (void)otherMouseDown:(NSEvent *)theEvent
{
    const bool accepted = [self handleMouseDownEvent:theEvent];
    if (!accepted)
        [super otherMouseDown:theEvent];
}

- (void)otherMouseDragged:(NSEvent *)theEvent
{
    const bool accepted = [self handleMouseDraggedEvent:theEvent];
    if (!accepted)
        [super otherMouseDragged:theEvent];
}

- (void)otherMouseUp:(NSEvent *)theEvent
{
    const bool accepted = [self handleMouseUpEvent:theEvent];
    if (!accepted)
        [super otherMouseUp:theEvent];
}

- (void)cursorUpdate:(NSEvent *)theEvent
{
    // Note: We do not get this callback when moving from a subview that
    // uses the legacy cursorRect API, so the cursor is reset to the arrow
    // cursor. See rdar://34183708

    auto previousCursor = NSCursor.currentCursor;

    if (self.cursor)
        [self.cursor set];
    else
        [super cursorUpdate:theEvent];

    if (NSCursor.currentCursor != previousCursor)
        qCInfo(lcQpaMouse) << "Cursor update for" << self << "resulted in new cursor" << NSCursor.currentCursor;
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
    Q_UNUSED(theEvent);
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

    qCInfo(lcQpaMouse) << QEvent::Enter << self << "at" << windowPoint << "with" << currentlyPressedMouseButtons();
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

    qCInfo(lcQpaMouse) << QEvent::Leave << self;
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

    Qt::ScrollPhase phase = Qt::NoScrollPhase;
    if (theEvent.phase == NSEventPhaseMayBegin || theEvent.phase == NSEventPhaseBegan) {
        // MayBegin is likely to happen. We treat it the same as an actual begin,
        // and follow it with an update when the actual begin is delivered.
        phase = m_scrolling ? Qt::ScrollUpdate : Qt::ScrollBegin;
        m_scrolling = true;
    } else if (theEvent.phase == NSEventPhaseStationary || theEvent.phase == NSEventPhaseChanged) {
        phase = Qt::ScrollUpdate;
    } else if (theEvent.phase == NSEventPhaseEnded) {
        // A scroll event phase may be followed by a momentum phase after the user releases
        // the finger, and in that case we don't want to send a Qt::ScrollEnd until after
        // the momentum phase has ended. Unfortunately there isn't any guaranteed way of
        // knowing whether or not a NSEventPhaseEnded will be followed by a momentum phase.
        // The best we can do is to look at the event queue and hope that the system has
        // had time to emit a momentum phase event.
        if ([NSApp nextEventMatchingMask:NSEventMaskScrollWheel untilDate:[NSDate distantPast]
                inMode:@"QtMomementumEventSearchMode" dequeue:NO].momentumPhase == NSEventPhaseBegan) {
            return; // Ignore, even if it has delta
        } else {
            phase = Qt::ScrollEnd;
            m_scrolling = false;
        }
    } else if (theEvent.momentumPhase == NSEventPhaseBegan) {
        Q_ASSERT(!pixelDelta.isNull() && !angleDelta.isNull());
        // If we missed finding a momentum NSEventPhaseBegan when the non-momentum
        // phase ended we need to treat this as a scroll begin, to not confuse client
        // code. Otherwise we treat it as a continuation of the existing scroll.
        phase = m_scrolling ? Qt::ScrollUpdate : Qt::ScrollBegin;
        m_scrolling = true;
    } else if (theEvent.momentumPhase == NSEventPhaseChanged) {
        phase = Qt::ScrollMomentum;
    } else if (theEvent.phase == NSEventPhaseCancelled
            || theEvent.momentumPhase == NSEventPhaseEnded
            || theEvent.momentumPhase == NSEventPhaseCancelled) {
        phase = Qt::ScrollEnd;
        m_scrolling = false;
    } else {
        Q_ASSERT(theEvent.momentumPhase != NSEventPhaseStationary);
    }

    // Sanitize deltas for events that should not result in scrolling.
    // On macOS 12.1 this phase has been observed to report deltas.
    if (theEvent.phase == NSEventPhaseCancelled) {
        if (!pixelDelta.isNull() || !angleDelta.isNull()) {
            qCInfo(lcQpaMouse) << "Ignoring unexpected delta for" << theEvent;
            pixelDelta = QPoint();
            angleDelta = QPoint();
        }
    }

    // Prevent keyboard modifier state from changing during scroll event streams.
    // A two-finger trackpad flick generates a stream of scroll events. We want
    // the keyboard modifier state to be the state at the beginning of the
    // flick in order to avoid changing the interpretation of the events
    // mid-stream. One example of this happening would be when pressing cmd
    // after scrolling in Qt Creator: not taking the phase into account causes
    // the end of the event stream to be interpreted as font size changes.
    if (theEvent.momentumPhase == NSEventPhaseNone)
        m_currentWheelModifiers = QAppleKeyMapper::fromCocoaModifiers(theEvent.modifierFlags);

    // "isInverted": natural OS X scrolling, inverted from the Qt/other platform/Jens perspective.
    bool isInverted  = [theEvent isDirectionInvertedFromDevice];

    qCInfo(lcQpaMouse).nospace() << phase << " at " << qt_windowPoint
        << " pixelDelta=" << pixelDelta << " angleDelta=" << angleDelta
        << (isInverted ? " inverted=true" : "");

    const QPointingDevice *device = pointingDeviceFor(theEvent.deviceID);
    Q_ASSERT(device);

    if (theEvent.hasPreciseScrollingDeltas) {
        auto *devicePriv = QPointingDevicePrivate::get(const_cast<QPointingDevice *>(device));
        if (!devicePriv->capabilities.testFlag(QInputDevice::Capability::PixelScroll)) {
            devicePriv->name = QLatin1String("trackpad or magic mouse");
            devicePriv->deviceType = QInputDevice::DeviceType::TouchPad;
            devicePriv->capabilities |= QInputDevice::Capability::PixelScroll;
            qCDebug(lcInputDevices) << "mouse scrolling: updated capabilities" << device;
        }
    }

    QWindowSystemInterface::handleWheelEvent(m_platformWindow->window(), qt_timestamp,
        device, qt_windowPoint, qt_screenPoint, pixelDelta, angleDelta,
        m_currentWheelModifiers, phase, source, isInverted);
}
#endif // QT_CONFIG(wheelevent)

@end
