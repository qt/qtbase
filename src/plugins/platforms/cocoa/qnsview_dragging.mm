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

@implementation QNSView (Dragging)

-(void)registerDragTypes
{
    QMacAutoReleasePool pool;

    NSString * const mimeTypeGeneric = @"com.trolltech.qt.MimeTypeName";
    NSMutableArray<NSString *> *supportedTypes = [NSMutableArray<NSString *> arrayWithArray:@[
                   NSColorPboardType,
                   NSFilenamesPboardType, NSStringPboardType,
                   NSFilenamesPboardType, NSPostScriptPboardType, NSTIFFPboardType,
                   NSRTFPboardType, NSTabularTextPboardType, NSFontPboardType,
                   NSRulerPboardType, NSFileContentsPboardType, NSColorPboardType,
                   NSRTFDPboardType, NSHTMLPboardType,
                   NSURLPboardType, NSPDFPboardType, NSVCardPboardType,
                   NSFilesPromisePboardType, NSInkTextPboardType,
                   NSMultipleTextSelectionPboardType, mimeTypeGeneric]];

    // Add custom types supported by the application
    for (const QString &customType : qt_mac_enabledDraggedTypes())
        [supportedTypes addObject:customType.toNSString()];

    [self registerForDraggedTypes:supportedTypes];
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

- (NSDragOperation)draggingSession:(NSDraggingSession *)session sourceOperationMaskForDraggingContext:(NSDraggingContext)context
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
            // Uncomment the next lines if forbidden cursor is wanted on undroppable targets.
            /*nativeCursor = [NSCursor operationNotAllowedCursor];
            break;*/
        case Qt::MoveAction:
        default:
            nativeCursor = [NSCursor arrowCursor];
            break;
        }
    } else {
        auto *nsimage = [NSImage imageFromQImage:pixmapCursor.toImage()];
        nativeCursor = [[NSCursor alloc] initWithImage:nsimage hotSpot:NSZeroPoint];
    }

    // Change the cursor
    [nativeCursor set];

    // Make sure the cursor is updated correctly if the mouse does not move and window is under cursor
    // by creating a fake move event, unless on 10.14 and later where doing so will trigger a security
    // warning dialog. FIXME: Find a way to update the cursor without fake mouse events.
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSMojave)
        return;

    if (m_updatingDrag)
        return;

    QCFType<CGEventRef> moveEvent = CGEventCreateMouseEvent(
        nullptr, kCGEventMouseMoved, QCursor::pos().toCGPoint(),
        kCGMouseButtonLeft // ignored
    );
    CGEventPost(kCGHIDEventTap, moveEvent);
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender
{
    return [self handleDrag:(QEvent::DragEnter) sender:sender];
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender
{
    QScopedValueRollback<bool> rollback(m_updatingDrag, true);
    return [self handleDrag:(QEvent::DragMove) sender:sender];
}

// Sends drag update to Qt, return the action
- (NSDragOperation)handleDrag:(QEvent::Type)dragType sender:(id<NSDraggingInfo>)sender
{
    if (!m_platformWindow)
        return NSDragOperationNone;

    QPoint windowPoint = QPointF::fromCGPoint([self convertPoint:sender.draggingLocation fromView:nil]).toPoint();

    Qt::DropActions qtAllowed = qt_mac_mapNSDragOperations(sender.draggingSourceOperationMask);

    QWindow *target = findEventTargetWindow(m_platformWindow->window());
    if (!target)
        return NSDragOperationNone;

    const auto modifiers = [QNSView convertKeyModifiers:NSApp.currentEvent.modifierFlags];
    const auto buttons = currentlyPressedMouseButtons();
    const auto point = mapWindowCoordinates(m_platformWindow->window(), target, windowPoint);

    if (dragType == QEvent::DragEnter)
        qCDebug(lcQpaMouse) << dragType << self << "at" << windowPoint;
    else
        qCDebug(lcQpaMouse) << dragType << "at" << windowPoint << "with" << buttons;

    QPlatformDragQtResponse response(false, Qt::IgnoreAction, QRect());
    QCocoaDrag* nativeDrag = QCocoaIntegration::instance()->drag();
    if (nativeDrag->currentDrag()) {
        // The drag was started from within the application
        response = QWindowSystemInterface::handleDrag(target, nativeDrag->dragMimeData(),
                                                      point, qtAllowed, buttons, modifiers);
        [self updateCursorFromDragResponse:response drag:nativeDrag];
    } else {
        QCocoaDropData mimeData(sender.draggingPasteboard);
        response = QWindowSystemInterface::handleDrag(target, &mimeData,
                                                      point, qtAllowed, buttons, modifiers);
    }

    return qt_mac_mapDropAction(response.acceptedAction());
}

- (void)draggingExited:(id<NSDraggingInfo>)sender
{
    if (!m_platformWindow)
        return;

    QWindow *target = findEventTargetWindow(m_platformWindow->window());
    if (!target)
        return;

    auto *nativeDrag = QCocoaIntegration::instance()->drag();
    Q_ASSERT(nativeDrag);
    nativeDrag->exitDragLoop();

    QPoint windowPoint = QPointF::fromCGPoint([self convertPoint:sender.draggingLocation fromView:nil]).toPoint();

    qCDebug(lcQpaMouse) << QEvent::DragLeave << self << "at" << windowPoint;

    // Send 0 mime data to indicate drag exit
    QWindowSystemInterface::handleDrag(target, nullptr,
                                       mapWindowCoordinates(m_platformWindow->window(), target, windowPoint),
                                       Qt::IgnoreAction, Qt::NoButton, Qt::NoModifier);
}

// Called on drop, send the drop to Qt and return if it was accepted
- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender
{
    if (!m_platformWindow)
        return false;

    QWindow *target = findEventTargetWindow(m_platformWindow->window());
    if (!target)
        return false;

    QPoint windowPoint = QPointF::fromCGPoint([self convertPoint:sender.draggingLocation fromView:nil]).toPoint();

    Qt::DropActions qtAllowed = qt_mac_mapNSDragOperations(sender.draggingSourceOperationMask);

    QPlatformDropQtResponse response(false, Qt::IgnoreAction);
    QCocoaDrag* nativeDrag = QCocoaIntegration::instance()->drag();
    const auto modifiers = [QNSView convertKeyModifiers:NSApp.currentEvent.modifierFlags];
    const auto buttons = currentlyPressedMouseButtons();
    const auto point = mapWindowCoordinates(m_platformWindow->window(), target, windowPoint);

    qCDebug(lcQpaMouse) << QEvent::Drop << "at" << windowPoint << "with" << buttons;

    if (nativeDrag->currentDrag()) {
        // The drag was started from within the application
        response = QWindowSystemInterface::handleDrop(target, nativeDrag->dragMimeData(),
                                                      point, qtAllowed, buttons, modifiers);
        nativeDrag->setAcceptedAction(response.acceptedAction());
    } else {
        QCocoaDropData mimeData(sender.draggingPasteboard);
        response = QWindowSystemInterface::handleDrop(target, &mimeData,
                                                      point, qtAllowed, buttons, modifiers);
    }
    return response.isAccepted();
}

- (void)draggingSession:(NSDraggingSession *)session endedAtPoint:(NSPoint)screenPoint operation:(NSDragOperation)operation
{
    Q_UNUSED(session);
    Q_UNUSED(screenPoint);
    Q_UNUSED(operation);

    if (!m_platformWindow)
        return;

    QWindow *target = findEventTargetWindow(m_platformWindow->window());
    if (!target)
        return;

    QCocoaDrag* nativeDrag = QCocoaIntegration::instance()->drag();
    Q_ASSERT(nativeDrag);
    nativeDrag->exitDragLoop();
    nativeDrag->setAcceptedAction(qt_mac_mapNSDragOperation(operation));

    // Qt starts drag-and-drop on a mouse button press event. Cococa in
    // this case won't send the matching release event, so we have to
    // synthesize it here.
    m_buttons = currentlyPressedMouseButtons();
    const auto modifiers = [QNSView convertKeyModifiers:NSApp.currentEvent.modifierFlags];

    NSPoint windowPoint = [self.window convertRectFromScreen:NSMakeRect(screenPoint.x, screenPoint.y, 1, 1)].origin;
    NSPoint nsViewPoint = [self convertPoint: windowPoint fromView: nil];

    QPoint qtWindowPoint(nsViewPoint.x, nsViewPoint.y);
    QPoint qtScreenPoint = QCocoaScreen::mapFromNative(screenPoint).toPoint();

    QWindowSystemInterface::handleMouseEvent(
        target,
        mapWindowCoordinates(m_platformWindow->window(), target, qtWindowPoint),
        qtScreenPoint,
        m_buttons,
        Qt::NoButton,
        QEvent::MouseButtonRelease,
        modifiers);

    qCDebug(lcQpaMouse) << "Drag session" << session << "ended, with" << m_buttons;
}

@end
