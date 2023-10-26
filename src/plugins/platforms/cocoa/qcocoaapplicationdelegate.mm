// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

/****************************************************************************
 **
 ** Copyright (c) 2007-2008, Apple, Inc.
 **
 ** All rights reserved.
 **
 ** Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions are met:
 **
 **   * Redistributions of source code must retain the above copyright notice,
 **     this list of conditions and the following disclaimer.
 **
 **   * Redistributions in binary form must reproduce the above copyright notice,
 **     this list of conditions and the following disclaimer in the documentation
 **     and/or other materials provided with the distribution.
 **
 **   * Neither the name of Apple, Inc. nor the names of its contributors
 **     may be used to endorse or promote products derived from this software
 **     without specific prior written permission.
 **
 ** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 ** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 ** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 ** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 ** CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 ** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 ** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 ** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 ** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 ** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 ** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **
 ****************************************************************************/

#include <AppKit/AppKit.h>

#include "qcocoaapplicationdelegate.h"
#include "qcocoaintegration.h"
#include "qcocoamenubar.h"
#include "qcocoamenu.h"
#include "qcocoamenuloader.h"
#include "qcocoamenuitem.h"
#include "qcocoansmenu.h"
#include "qcocoahelpers.h"

#if QT_CONFIG(sessionmanager)
#  include "qcocoasessionmanager.h"
#endif

#include <qevent.h>
#include <qurl.h>
#include <qdebug.h>
#include <qguiapplication.h>
#include <qpa/qwindowsysteminterface.h>
#include <qwindowdefs.h>

QT_USE_NAMESPACE

@implementation QCocoaApplicationDelegate {
    NSObject <NSApplicationDelegate> *reflectionDelegate;
    bool inLaunch;
}

+ (instancetype)sharedDelegate
{
    static QCocoaApplicationDelegate *shared = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        shared = [[self alloc] init];
        atexit_b(^{
            [shared release];
            shared = nil;
        });
    });
    return shared;
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        inLaunch = true;
    }
    return self;
}

- (void)dealloc
{
    [_dockMenu release];
    if (reflectionDelegate) {
        [[NSApplication sharedApplication] setDelegate:reflectionDelegate];
        [reflectionDelegate release];
    }
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    [super dealloc];
}

- (NSMenu *)applicationDockMenu:(NSApplication *)sender
{
    Q_UNUSED(sender);
    // Manually invoke the delegate's -menuWillOpen: method.
    // See QTBUG-39604 (and its fix) for details.
    [self.dockMenu.delegate menuWillOpen:self.dockMenu];
    return [[self.dockMenu retain] autorelease];
}

// This function will only be called when NSApp is actually running.
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    if ([reflectionDelegate respondsToSelector:_cmd])
        return [reflectionDelegate applicationShouldTerminate:sender];

    if (QGuiApplicationPrivate::instance()->threadData.loadRelaxed()->eventLoops.isEmpty()) {
        // No event loop is executing. This probably means that Qt is used as a plugin,
        // or as a part of a native Cocoa application. In any case it should be fine to
        // terminate now.
        qCDebug(lcQpaApplication) << "No running event loops, terminating now";
        return NSTerminateNow;
    }

#if QT_CONFIG(sessionmanager)
    QCocoaSessionManager *cocoaSessionManager = QCocoaSessionManager::instance();
    cocoaSessionManager->resetCancellation();
    cocoaSessionManager->appCommitData();

    if (cocoaSessionManager->wasCanceled()) {
        qCDebug(lcQpaApplication) << "Session management canceled application termination";
        return NSTerminateCancel;
    }
#endif

    if (!QWindowSystemInterface::handleApplicationTermination<QWindowSystemInterface::SynchronousDelivery>()) {
        qCDebug(lcQpaApplication) << "Application termination canceled";
        return NSTerminateCancel;
    }

    // Even if the application termination was accepted by the application we can't
    // return NSTerminateNow, as that would trigger AppKit to ultimately call exit().
    // We need to ensure that the runloop continues spinning so that we can return
    // from our own event loop back to main(), and exit from there.
    qCDebug(lcQpaApplication) << "Termination accepted, but returning to runloop for exit through main()";
    return NSTerminateCancel;
}

- (void)applicationWillFinishLaunching:(NSNotification *)notification
{
    Q_UNUSED(notification);

    /*
        From the Cocoa documentation: "A good place to install event handlers
        is in the applicationWillFinishLaunching: method of the application
        delegate. At that point, the Application Kit has installed its default
        event handlers, so if you install a handler for one of the same events,
        it will replace the Application Kit version."
    */

    /*
        If Qt is used as a plugin, we let the 3rd party application handle
        events like quit and open file events. Otherwise, if we install our own
        handlers, we easily end up breaking functionality the 3rd party
        application depends on.
     */
    NSAppleEventManager *eventManager = [NSAppleEventManager sharedAppleEventManager];
    [eventManager setEventHandler:self
                      andSelector:@selector(getUrl:withReplyEvent:)
                    forEventClass:kInternetEventClass
                       andEventID:kAEGetURL];
}

// called by QCocoaIntegration's destructor before resetting the application delegate to nil
- (void)removeAppleEventHandlers
{
    NSAppleEventManager *eventManager = [NSAppleEventManager sharedAppleEventManager];
    [eventManager removeEventHandlerForEventClass:kInternetEventClass andEventID:kAEGetURL];
}

- (bool)inLaunch
{
    return inLaunch;
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    Q_UNUSED(aNotification);
    inLaunch = false;

    if (qEnvironmentVariableIsEmpty("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM")) {
        auto frontmostApplication = NSWorkspace.sharedWorkspace.frontmostApplication;
        auto currentApplication = NSRunningApplication.currentApplication;
        if (frontmostApplication != currentApplication) {
            // Move the application to front to avoid launching behind the terminal.
            // Ignoring other apps is necessary (we must ignore the terminal), but makes
            // Qt apps play slightly less nice with other apps when launching from Finder
            // (see the activateIgnoringOtherApps docs). FIXME: Try to distinguish between
            // being non-active here because another application stole activation in the
            // time it took us to launch from Finder, and being non-active because we were
            // launched from Terminal or something that doesn't activate us at all.
            qCDebug(lcQpaApplication) << "Launched with" << frontmostApplication
                << "as frontmost application. Activating" << currentApplication << "instead.";
            [NSApplication.sharedApplication activateIgnoringOtherApps:YES];
        }

        // Qt windows are typically shown in main(), at which point the application
        // is not active yet. When the application is activated, either externally
        // or via the override above, it will only bring the main and key windows
        // forward, which differs from the behavior if these windows had been shown
        // once the application was already active. To work around this, we explicitly
        // activate the current application again, bringing all windows to the front.
        [currentApplication activateWithOptions:NSApplicationActivateAllWindows];
    }

    QCocoaMenuBar::insertWindowMenu();
}

- (void)application:(NSApplication *)sender openFiles:(NSArray *)filenames
{
    Q_UNUSED(filenames);
    Q_UNUSED(sender);

    for (NSString *fileName in filenames) {
        QString qtFileName = QString::fromNSString(fileName);
        if (inLaunch) {
            // We need to be careful because Cocoa will be nice enough to take
            // command line arguments and send them to us as events. Given the history
            // of Qt Applications, this will result in behavior people don't want, as
            // they might be doing the opening themselves with the command line parsing.
            if (qApp->arguments().contains(qtFileName))
                continue;
        }
        QWindowSystemInterface::handleFileOpenEvent(qtFileName);
    }

    if ([reflectionDelegate respondsToSelector:_cmd])
        [reflectionDelegate application:sender openFiles:filenames];

}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    if ([reflectionDelegate respondsToSelector:_cmd])
        return [reflectionDelegate applicationShouldTerminateAfterLastWindowClosed:sender];

    return NO; // Someday qApp->quitOnLastWindowClosed(); when QApp and NSApp work closer together.
}

- (void)applicationDidBecomeActive:(NSNotification *)notification
{
    if (QCocoaWindow::s_applicationActivationObserver) {
        [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:QCocoaWindow::s_applicationActivationObserver];
        QCocoaWindow::s_applicationActivationObserver = nil;
    }

    if ([reflectionDelegate respondsToSelector:_cmd])
        [reflectionDelegate applicationDidBecomeActive:notification];

    QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationActive);

    if (QCocoaWindow::s_windowUnderMouse) {
        QPointF windowPoint;
        QPointF screenPoint;
        QNSView *view = qnsview_cast(QCocoaWindow::s_windowUnderMouse->m_view);
        [view convertFromScreen:[NSEvent mouseLocation] toWindowPoint:&windowPoint andScreenPoint:&screenPoint];
        QWindow *windowUnderMouse = QCocoaWindow::s_windowUnderMouse->window();
        qCInfo(lcQpaMouse) << "Application activated with mouse at" << windowPoint << "; sending" << QEvent::Enter << "to" << windowUnderMouse;
        QWindowSystemInterface::handleEnterEvent(windowUnderMouse, windowPoint, screenPoint);
    }
}

- (void)applicationDidResignActive:(NSNotification *)notification
{
    if ([reflectionDelegate respondsToSelector:_cmd])
        [reflectionDelegate applicationDidResignActive:notification];

    QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationInactive);

    if (QCocoaWindow::s_windowUnderMouse) {
        QWindow *windowUnderMouse = QCocoaWindow::s_windowUnderMouse->window();
        qCInfo(lcQpaMouse) << "Application deactivated; sending" << QEvent::Leave << "to" << windowUnderMouse;
        QWindowSystemInterface::handleLeaveEvent(windowUnderMouse);
    }
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag
{
    if ([reflectionDelegate respondsToSelector:_cmd])
        return [reflectionDelegate applicationShouldHandleReopen:theApplication hasVisibleWindows:flag];

    /*
       true to force delivery of the event even if the application state is already active,
       because rapp (handle reopen) events are sent each time the dock icon is clicked regardless
       of the active state of the application or number of visible windows. For example, a browser
       app that has no windows opened would need the event be to delivered even if it was already
       active in order to create a new window as per OS X conventions.
     */
    QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationActive, true /*forcePropagate*/);

    return YES;
}

- (void)setReflectionDelegate:(NSObject <NSApplicationDelegate> *)oldDelegate
{
    [oldDelegate retain];
    [reflectionDelegate release];
    reflectionDelegate = oldDelegate;
}

- (NSMethodSignature *)methodSignatureForSelector:(SEL)aSelector
{
    NSMethodSignature *result = [super methodSignatureForSelector:aSelector];
    if (!result && reflectionDelegate) {
        result = [reflectionDelegate methodSignatureForSelector:aSelector];
    }
    return result;
}

- (BOOL)respondsToSelector:(SEL)aSelector
{
    return [super respondsToSelector:aSelector] || [reflectionDelegate respondsToSelector:aSelector];
}

- (void)forwardInvocation:(NSInvocation *)invocation
{
    SEL invocationSelector = [invocation selector];
    if ([reflectionDelegate respondsToSelector:invocationSelector])
        [invocation invokeWithTarget:reflectionDelegate];
    else
        [self doesNotRecognizeSelector:invocationSelector];
}

- (void)getUrl:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent
{
    Q_UNUSED(replyEvent);
    NSString *urlString = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];
    // The string we get from the requesting application might not necessarily meet
    // QUrl's requirement for a IDN-compliant host. So if we can't parse into a QUrl,
    // then we pass the string on to the application as the name of a file (and
    // QFileOpenEvent::file is not guaranteed to be the path to a local, open'able
    // file anyway).
    const QString qurlString = QString::fromNSString(urlString);
    if (const QUrl url(qurlString); url.isValid())
        QWindowSystemInterface::handleFileOpenEvent(url);
    else
        QWindowSystemInterface::handleFileOpenEvent(qurlString);
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)application
{
    if (@available(macOS 12, *)) {
        if ([reflectionDelegate respondsToSelector:_cmd])
            return [reflectionDelegate applicationSupportsSecureRestorableState:application];
    }

    // We don't support or implement state restorations via the AppKit
    // state restoration APIs, but if we did, we would/should support
    // secure state restoration. This is the default for apps linked
    // against the macOS 14 SDK, but as we target versions below that
    // as well we need to return YES here explicitly to silence a runtime
    // warning.
    return YES;
}

@end

@implementation QCocoaApplicationDelegate (Menus)

- (BOOL)validateMenuItem:(NSMenuItem*)item
{
    qCDebug(lcQpaMenus) << "Validating" << item << "for" << self;

    auto *nativeItem = qt_objc_cast<QCocoaNSMenuItem *>(item);
    if (!nativeItem)
        return item.enabled; // FIXME Test with with Qt as plugin or embedded QWindow.

    auto *platformItem = nativeItem.platformMenuItem;
    if (!platformItem) // Try a bit harder with orphan menu items
        return item.hasSubmenu || (item.enabled && (item.action != @selector(qt_itemFired:)));

    // Menu-holding items are always enabled, as it's conventional in Cocoa
    if (platformItem->menu())
        return YES;

    return platformItem->isEnabled();
}

@end

@implementation QCocoaApplicationDelegate (MenuAPI)

- (void)qt_itemFired:(QCocoaNSMenuItem *)item
{
    qCDebug(lcQpaMenus) << "Activating" << item;

    if (item.hasSubmenu)
        return;

    auto *nativeItem = qt_objc_cast<QCocoaNSMenuItem *>(item);
    Q_ASSERT_X(nativeItem, qPrintable(__FUNCTION__), "Triggered menu item is not a QCocoaNSMenuItem.");
    auto *platformItem = nativeItem.platformMenuItem;
    // Menu-holding items also get a target to play nicely
    // with NSMenuValidation but should not trigger.
    if (!platformItem || platformItem->menu())
        return;

    QScopedScopeLevelCounter scopeLevelCounter(QGuiApplicationPrivate::instance()->threadData.loadRelaxed());
    QGuiApplicationPrivate::modifier_buttons = QAppleKeyMapper::fromCocoaModifiers([NSEvent modifierFlags]);

    static QMetaMethod activatedSignal = QMetaMethod::fromSignal(&QCocoaMenuItem::activated);
    activatedSignal.invoke(platformItem, Qt::QueuedConnection);
}

@end
