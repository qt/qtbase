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

@implementation QT_MANGLE_NAMESPACE(QNSView) (Dragging)

-(void)registerDragTypes
{
    QMacAutoReleasePool pool;
    QStringList customTypes = qt_mac_enabledDraggedTypes();
    if (currentCustomDragTypes == 0 || *currentCustomDragTypes != customTypes) {
        if (currentCustomDragTypes == 0)
            currentCustomDragTypes = new QStringList();
        *currentCustomDragTypes = customTypes;
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
    // by creating a fake move event
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

    QCocoaDrag* nativeDrag = QCocoaIntegration::instance()->drag();
    nativeDrag->setAcceptedAction(qt_mac_mapNSDragOperation(operation));

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
