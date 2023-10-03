/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#if !defined(QNSWINDOW_PROTOCOL_IMPLMENTATION)

#include <AppKit/AppKit.h>

#include "qnswindow.h"
#include "qcocoawindow.h"
#include "qcocoahelpers.h"
#include "qcocoaeventdispatcher.h"

#include <qpa/qwindowsysteminterface.h>
#include <qoperatingsystemversion.h>

Q_LOGGING_CATEGORY(lcQpaEvents, "qt.qpa.events");

static bool isMouseEvent(NSEvent *ev)
{
    switch ([ev type]) {
    case NSEventTypeLeftMouseDown:
    case NSEventTypeLeftMouseUp:
    case NSEventTypeRightMouseDown:
    case NSEventTypeRightMouseUp:
    case NSEventTypeMouseMoved:
    case NSEventTypeLeftMouseDragged:
    case NSEventTypeRightMouseDragged:
        return true;
    default:
        return false;
    }
}

@implementation NSWindow (FullScreenProperty)

+ (void)load
{
    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    [center addObserverForName:NSWindowDidEnterFullScreenNotification object:nil queue:nil
        usingBlock:^(NSNotification *notification) {
            objc_setAssociatedObject(notification.object, @selector(qt_fullScreen),
                @(YES), OBJC_ASSOCIATION_RETAIN);
        }
    ];
    [center addObserverForName:NSWindowDidExitFullScreenNotification object:nil queue:nil
        usingBlock:^(NSNotification *notification) {
            objc_setAssociatedObject(notification.object, @selector(qt_fullScreen),
                nil, OBJC_ASSOCIATION_RETAIN);
        }
    ];
}

- (BOOL)qt_fullScreen
{
    NSNumber *number = objc_getAssociatedObject(self, @selector(qt_fullScreen));
    return [number boolValue];
}
@end

@implementation QNSWindow
#define QNSWINDOW_PROTOCOL_IMPLMENTATION 1
#include "qnswindow.mm"
#undef QNSWINDOW_PROTOCOL_IMPLMENTATION

+ (void)applicationActivationChanged:(NSNotification*)notification
{
    const id sender = self;
    NSEnumerator<NSWindow*> *windowEnumerator = nullptr;
    NSApplication *application = [NSApplication sharedApplication];

    // Unfortunately there's no NSWindowListOrderedBackToFront,
    // so we have to manually reverse the order using an array.
    NSMutableArray<NSWindow *> *windows = [[NSMutableArray<NSWindow *> new] autorelease];
    [application enumerateWindowsWithOptions:NSWindowListOrderedFrontToBack
        usingBlock:^(NSWindow *window, BOOL *) {
            // For some reason AppKit will give us nil-windows, skip those
            if (!window)
                return;

            [windows addObject:window];
        }
    ];

    windowEnumerator = windows.reverseObjectEnumerator;

    for (NSWindow *window in windowEnumerator) {
        // We're meddling with normal and floating windows, so leave others alone
        if (!(window.level == NSNormalWindowLevel || window.level == NSFloatingWindowLevel))
            continue;

        // Windows that hide automatically will keep their NSFloatingWindowLevel,
        // and hence be on top of the window stack. We don't want to affect these
        // windows, as otherwise we might end up with key windows being ordered
        // behind these auto-hidden windows when activating the application by
        // clicking on a new tool window.
        if (window.hidesOnDeactivate)
            continue;

        if ([window conformsToProtocol:@protocol(QNSWindowProtocol)]) {
            QCocoaWindow *cocoaWindow = static_cast<QCocoaNSWindow *>(window).platformWindow;
            window.level = notification.name == NSApplicationWillResignActiveNotification ?
                NSNormalWindowLevel : cocoaWindow->windowLevel(cocoaWindow->window()->flags());
        }

        // The documentation says that "when a window enters a new level, it’s ordered
        // in front of all its peers in that level", but that doesn't seem to be the
        // case in practice. To keep the order correct after meddling with the window
        // levels, we explicitly order each window to the front. Since we are iterating
        // the windows in back-to-front order, this is okey. The call also triggers AppKit
        // to re-evaluate the level in relation to windows from other applications,
        // working around an issue where our tool windows would stay on top of other
        // application windows if activation was transferred to another application by
        // clicking on it instead of via the application switcher or Dock. Finally, we
        // do this re-ordering for all windows (except auto-hiding ones), otherwise we would
        // end up triggering a bug in AppKit where the tool windows would disappear behind
        // the application window.
        [window orderFront:sender];
    }
}

@end

@implementation QNSPanel
#define QNSWINDOW_PROTOCOL_IMPLMENTATION 1
#include "qnswindow.mm"
#undef QNSWINDOW_PROTOCOL_IMPLMENTATION

- (BOOL)worksWhenModal
{
    if (!m_platformWindow)
        return NO;

    // Conceptually there are two sets of windows we need consider:
    //
    //   - windows 'lower' in the modal session stack
    //   - windows 'within' the current modal session
    //
    // The first set of windows should always be blocked by the current
    // modal session, regardless of window type. The latter set may contain
    // windows with a transient parent, which from Qt's point of view makes
    // them 'child' windows, so we treat them as operable within the current
    // modal session.

    if (!NSApp.modalWindow)
        return NO;

    // Special case popup windows (menus, completions, etc), as these usually
    // don't have a transient parent set, and we don't want to block them. The
    // assumption is that these windows are only opened intermittently, from
    // within windows that can already be interacted with in this modal session.
    Qt::WindowType type = m_platformWindow->window()->type();
    if (type == Qt::Popup)
        return YES;

    // If the current modal window (top level modal session) is not a Qt window we
    // have no way of knowing if this window is transient child of the modal window.
    if (![NSApp.modalWindow conformsToProtocol:@protocol(QNSWindowProtocol)])
        return NO;

    if (auto *modalWindow = static_cast<QCocoaNSWindow *>(NSApp.modalWindow).platformWindow) {
        if (modalWindow->window()->isAncestorOf(m_platformWindow->window(), QWindow::IncludeTransients))
            return YES;
    }

    return NO;
}
@end

#if !defined(QT_APPLE_NO_PRIVATE_APIS)
// When creating an NSWindow the worksWhenModal function is queried,
// and the resulting state is used to set the corresponding window tag,
// which the window server uses to determine whether or not the window
// should be allowed to activate via mouse clicks in the title-bar.
// Unfortunately, prior to macOS 10.15, this window tag was never
// updated after the initial assignment in [NSWindow _commonAwake],
// which meant that windows that dynamically change their worksWhenModal
// state will behave as if they were never allowed to work when modal.
// We work around this by manually updating the window tag when needed.

typedef uint32_t CGSConnectionID;
typedef uint32_t CGSWindowID;

extern "C" {
CGSConnectionID CGSMainConnectionID() __attribute__((weak_import));
OSStatus CGSSetWindowTags(const CGSConnectionID, const CGSWindowID, int *, int) __attribute__((weak_import));
OSStatus CGSClearWindowTags(const CGSConnectionID, const CGSWindowID, int *, int) __attribute__((weak_import));
}

@interface QNSPanel (WorksWhenModalWindowTagWorkaround) @end
@implementation QNSPanel (WorksWhenModalWindowTagWorkaround)
- (void)setWorksWhenModal:(BOOL)worksWhenModal
{
    [super setWorksWhenModal:worksWhenModal];

    if (QOperatingSystemVersion::current() < QOperatingSystemVersion::MacOSCatalina) {
        if (CGSMainConnectionID && CGSSetWindowTags && CGSClearWindowTags) {
            static int kWorksWhenModalWindowTag = 0x40;
            auto *function = worksWhenModal ? CGSSetWindowTags : CGSClearWindowTags;
            function(CGSMainConnectionID(), self.windowNumber, &kWorksWhenModalWindowTag, 64);
        } else {
            qWarning() << "Missing APIs for window tag handling, can not update worksWhenModal state";
        }
    }
}
@end
#endif // QT_APPLE_NO_PRIVATE_APIS

#else // QNSWINDOW_PROTOCOL_IMPLMENTATION

// The following content is mixed in to the QNSWindow and QNSPanel classes via includes

{
    // Member variables
    QPointer<QCocoaWindow> m_platformWindow;
}

- (instancetype)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)style
    backing:(NSBackingStoreType)backingStoreType defer:(BOOL)defer screen:(NSScreen *)screen
    platformWindow:(QCocoaWindow*)window
{
    // Initializing the window will end up in [NSWindow _commonAwake], which calls many
    // of the getters below. We need to set up the platform window reference first, so
    // we can properly reflect the window's state during initialization.
    m_platformWindow = window;

    return [super initWithContentRect:contentRect styleMask:style backing:backingStoreType defer:defer screen:screen];
}

- (QCocoaWindow *)platformWindow
{
    return m_platformWindow;
}

- (NSString *)description
{
    NSMutableString *description = [NSMutableString stringWithString:[super description]];

#ifndef QT_NO_DEBUG_STREAM
    QString contentViewDescription;
    QDebug debug(&contentViewDescription);
    debug.nospace() << "; contentView=" << qnsview_cast(self.contentView) << ">";

    NSRange lastCharacter = [description rangeOfComposedCharacterSequenceAtIndex:description.length - 1];
    [description replaceCharactersInRange:lastCharacter withString:contentViewDescription.toNSString()];
#endif

    return description;
}

- (BOOL)canBecomeKeyWindow
{
    if (!m_platformWindow)
        return NO;

    if (m_platformWindow->shouldRefuseKeyWindowAndFirstResponder())
        return NO;

    if ([self isKindOfClass:[QNSPanel class]]) {
        // Only tool or dialog windows should become key:
        Qt::WindowType type = m_platformWindow->window()->type();
        if (type == Qt::Tool || type == Qt::Dialog)
            return YES;

        return NO;
    } else {
        // The default implementation returns NO for title-bar less windows,
        // override and return yes here to make sure popup windows such as
        // the combobox popup can become the key window.
        return YES;
    }
}

- (BOOL)canBecomeMainWindow
{
    // Windows with a transient parent (such as combobox popup windows)
    // cannot become the main window:
    if (!m_platformWindow || m_platformWindow->window()->transientParent())
        return NO;

    return [super canBecomeMainWindow];
}

- (BOOL)isOpaque
{
    return m_platformWindow ? m_platformWindow->isOpaque() : [super isOpaque];
}

- (NSColor *)backgroundColor
{
    // FIXME: Plumb to a WA_NoSystemBackground-like window flag,
    // or a QWindow::backgroundColor() property. In the meantime
    // we assume that if you have translucent content, without a
    // frame then you intend to do all background drawing yourself.
    const QWindow *window = m_platformWindow ? m_platformWindow->window() : nullptr;
    if (!self.opaque && window) {
        // Qt::Popup also requires clearColor - in qmacstyle
        // we fill background using a special path with rounded corners.
        if (window->flags().testFlag(Qt::FramelessWindowHint)
            || (window->flags() & Qt::WindowType_Mask) == Qt::Popup)
            return [NSColor clearColor];
    }

    // This still allows you to have translucent content with a frame,
    // where the system background (or color set via NSWindow) will
    // shine through.
    return [super backgroundColor];
}

- (void)sendEvent:(NSEvent*)theEvent
{
    qCDebug(lcQpaEvents) << "Sending" << theEvent << "to" << self;

    // We might get events for a NSWindow after the corresponding platform
    // window has been deleted, as the NSWindow can outlive the QCocoaWindow
    // e.g. if being retained by other parts of AppKit, or in an auto-release
    // pool. We guard against this in QNSView as well, as not all callbacks
    // come via events, but if they do there's no point in propagating them.
    if (!m_platformWindow)
        return;

    // Prevent deallocation of this NSWindow during event delivery, as we
    // have logic further below that depends on the window being alive.
    [[self retain] autorelease];

    const char *eventType = object_getClassName(theEvent);
    if (QWindowSystemInterface::handleNativeEvent(m_platformWindow->window(),
        QByteArray::fromRawData(eventType, qstrlen(eventType)), theEvent, nullptr)) {
        return;
    }

    const bool mouseEventInFrameStrut = [theEvent, self]{
        if (isMouseEvent(theEvent)) {
            const NSPoint loc = theEvent.locationInWindow;
            const NSRect windowFrame = [self convertRectFromScreen:self.frame];
            const NSRect contentFrame = self.contentView.frame;
            if (NSMouseInRect(loc, windowFrame, NO) && !NSMouseInRect(loc, contentFrame, NO))
                return true;
        }
        return false;
    }();
    // Any mouse-press in the frame of the window, including the title bar buttons, should
    // close open popups. Presses within the window's content are handled to do that in the
    // NSView::mouseDown implementation.
    if (theEvent.type == NSEventTypeLeftMouseDown && mouseEventInFrameStrut)
        [qnsview_cast(m_platformWindow->view()) closePopups:theEvent];

    [super sendEvent:theEvent];

    if (!m_platformWindow)
        return; // Platform window went away while processing event

    if (m_platformWindow->frameStrutEventsEnabled() && mouseEventInFrameStrut)
        [qnsview_cast(m_platformWindow->view()) handleFrameStrutMouseEvent:theEvent];
}

- (void)closeAndRelease
{
    qCDebug(lcQpaWindow) << "Closing and releasing" << self;
    [self close];
    [self release];
}

- (void)dealloc
{
    qCDebug(lcQpaWindow) << "Deallocating" << self;
    self.delegate = nil;

    [super dealloc];
}

#endif
