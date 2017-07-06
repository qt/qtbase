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

#include "qnswindow.h"
#include "qnswindowdelegate.h"
#include "qcocoawindow.h"
#include "qcocoahelpers.h"
#include "qcocoaeventdispatcher.h"

#include <qoperatingsystemversion.h>

static bool isMouseEvent(NSEvent *ev)
{
    switch ([ev type]) {
    case NSLeftMouseDown:
    case NSLeftMouseUp:
    case NSRightMouseDown:
    case NSRightMouseUp:
    case NSMouseMoved:
    case NSLeftMouseDragged:
    case NSRightMouseDragged:
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
                [NSNumber numberWithBool:YES], OBJC_ASSOCIATION_RETAIN);
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

@implementation QNSWindowHelper

- (QCocoaWindow *)platformWindow
{
    return _platformWindow.data();
}

- (id)initWithNSWindow:(QCocoaNSWindow *)window platformWindow:(QCocoaWindow *)platformWindow
{
    if (self = [super init]) {
        _window = window;
        _platformWindow = platformWindow;

        _window.delegate = [[QNSWindowDelegate alloc] initWithQCocoaWindow:_platformWindow];

        // Prevent Cocoa from releasing the window on close. Qt
        // handles the close event asynchronously and we want to
        // make sure that NSWindow stays valid until the
        // QCocoaWindow is deleted by Qt.
        [_window setReleasedWhenClosed:NO];
    }

    return self;
}

- (void)handleWindowEvent:(NSEvent *)theEvent
{
    // We might get events for a NSWindow after the corresponding platform
    // window has been deleted, as the NSWindow can outlive the QCocoaWindow
    // e.g. if being retained by other parts of AppKit, or in an auto-release
    // pool. We guard against this in QNSView as well, as not all callbacks
    // come via events, but if they do there's no point in propagating them.
    if (!self.platformWindow)
        return;

    [self.window superSendEvent:theEvent];

    if (!self.platformWindow)
        return; // Platform window went away while processing event

    QCocoaWindow *pw = self.platformWindow;
    if (pw->frameStrutEventsEnabled() && isMouseEvent(theEvent)) {
        NSPoint loc = [theEvent locationInWindow];
        NSRect windowFrame = [self.window convertRectFromScreen:[self.window frame]];
        NSRect contentFrame = [[self.window contentView] frame];
        if (NSMouseInRect(loc, windowFrame, NO) && !NSMouseInRect(loc, contentFrame, NO))
            [qnsview_cast(pw->view()) handleFrameStrutMouseEvent:theEvent];
    }
}

- (void)detachFromPlatformWindow
{
    _platformWindow.clear();
    [self.window.delegate release];
    self.window.delegate = nil;
}

- (void)dealloc
{
    _window = nil;
    _platformWindow.clear();
    [super dealloc];
}

@end

// Deferring window creation breaks OpenGL (the GL context is
// set up before the window is shown and needs a proper window)
static const bool kNoDefer = NO;

@implementation QNSWindow

@synthesize helper = _helper;

- (id)initWithContentRect:(NSRect)contentRect
      screen:(NSScreen*)screen
      styleMask:(NSUInteger)windowStyle
      qPlatformWindow:(QCocoaWindow *)qpw
{
    if (self = [super initWithContentRect:contentRect styleMask:windowStyle
                      backing:NSBackingStoreBuffered defer:kNoDefer screen:screen]) {
        _helper = [[QNSWindowHelper alloc] initWithNSWindow:self platformWindow:qpw];
    }
    return self;
}

- (BOOL)canBecomeKeyWindow
{
    QCocoaWindow *pw = self.helper.platformWindow;
    if (!pw)
        return NO;

    if (pw->shouldRefuseKeyWindowAndFirstResponder())
        return NO;

    // The default implementation returns NO for title-bar less windows,
    // override and return yes here to make sure popup windows such as
    // the combobox popup can become the key window.
    return YES;
}

- (BOOL)canBecomeMainWindow
{
    BOOL canBecomeMain = YES; // By default, windows can become the main window

    // Windows with a transient parent (such as combobox popup windows)
    // cannot become the main window:
    QCocoaWindow *pw = self.helper.platformWindow;
    if (!pw || pw->window()->transientParent())
        canBecomeMain = NO;

    return canBecomeMain;
}

- (void)sendEvent:(NSEvent*)theEvent
{
    [self.helper handleWindowEvent:theEvent];
}

- (void)superSendEvent:(NSEvent *)theEvent
{
    [super sendEvent:theEvent];
}

- (void)closeAndRelease
{
    qCDebug(lcQpaCocoaWindow) << "closeAndRelease" << self;

    [self.helper detachFromPlatformWindow];
    [self close];
    [self release];
}

- (void)dealloc
{
    [_helper release];
    _helper = nil;
    [super dealloc];
}

@end

@implementation QNSPanel

@synthesize helper = _helper;

+ (void)applicationActivationChanged:(NSNotification*)notification
{
    const id sender = self;
    NSEnumerator<NSWindow*> *windowEnumerator = nullptr;
    NSApplication *application = [NSApplication sharedApplication];

#if QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_12)
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSSierra) {
        // Unfortunately there's no NSWindowListOrderedBackToFront,
        // so we have to manually reverse the order using an array.
        NSMutableArray *windows = [[[NSMutableArray alloc] init] autorelease];
        [application enumerateWindowsWithOptions:NSWindowListOrderedFrontToBack
            usingBlock:^(NSWindow *window, BOOL *) {
                // For some reason AppKit will give us nil-windows, skip those
                if (!window)
                    return;

                [(NSMutableArray*)windows addObject:window];
            }
        ];

        windowEnumerator = windows.reverseObjectEnumerator;
    } else
#endif
    {
        // No way to get ordered list of windows, so fall back to unordered,
        // list, which typically corresponds to window creation order.
        windowEnumerator = application.windows.objectEnumerator;
    }

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
            QCocoaWindow *cocoaWindow = static_cast<id<QNSWindowProtocol>>(window).helper.platformWindow;
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

- (id)initWithContentRect:(NSRect)contentRect
      screen:(NSScreen*)screen
      styleMask:(NSUInteger)windowStyle
      qPlatformWindow:(QCocoaWindow *)qpw
{
    if (self = [super initWithContentRect:contentRect styleMask:windowStyle
                      backing:NSBackingStoreBuffered defer:kNoDefer screen:screen]) {
        _helper = [[QNSWindowHelper alloc] initWithNSWindow:self platformWindow:qpw];

        if (qpw->alwaysShowToolWindow()) {
            static dispatch_once_t onceToken;
            dispatch_once(&onceToken, ^{
                NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
                [center addObserver:[self class] selector:@selector(applicationActivationChanged:)
                    name:NSApplicationWillResignActiveNotification object:nil];
                [center addObserver:[self class] selector:@selector(applicationActivationChanged:)
                    name:NSApplicationWillBecomeActiveNotification object:nil];
            });
        }
    }
    return self;
}

- (BOOL)canBecomeKeyWindow
{
    QCocoaWindow *pw = self.helper.platformWindow;
    if (!pw)
        return NO;

    if (pw->shouldRefuseKeyWindowAndFirstResponder())
        return NO;

    // Only tool or dialog windows should become key:
    Qt::WindowType type = pw->window()->type();
    if (type == Qt::Tool || type == Qt::Dialog)
        return YES;

    return NO;
}

- (void)sendEvent:(NSEvent*)theEvent
{
    [self.helper handleWindowEvent:theEvent];
}

- (void)superSendEvent:(NSEvent *)theEvent
{
    [super sendEvent:theEvent];
}

- (void)closeAndRelease
{
    qCDebug(lcQpaCocoaWindow) << "closeAndRelease" << self;

    [self.helper detachFromPlatformWindow];
    [self close];
    [self release];
}

- (void)dealloc
{
    [_helper release];
    _helper = nil;
    [super dealloc];
}

@end
