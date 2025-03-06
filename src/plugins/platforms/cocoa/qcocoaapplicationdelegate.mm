// Copyright (C) 2018 The Qt Company Ltd.
// Copyright (c) 2007-2008, Apple, Inc.
// SPDX-License-Identifier: BSD-3-Clause

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
    if ([reflectionDelegate respondsToSelector:_cmd])
        [reflectionDelegate applicationWillFinishLaunching:notification];

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
    if ([reflectionDelegate respondsToSelector:_cmd])
        [reflectionDelegate applicationDidFinishLaunching:aNotification];

    inLaunch = false;

    if (qEnvironmentVariableIsEmpty("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM")) {
        auto currentApplication = NSRunningApplication.currentApplication;
        if (!currentApplication.active) {
            // Move the application to front to avoid launching behind the terminal.
            // Ignoring other apps is necessary (we must ignore the terminal), but makes
            // Qt apps play slightly less nice with other apps when launching from Finder
            // (see the activateIgnoringOtherApps docs). FIXME: Try to distinguish between
            // being non-active here because another application stole activation in the
            // time it took us to launch from Finder, and being non-active because we were
            // launched from Terminal or something that doesn't activate us at all.
            auto frontmostApplication = NSWorkspace.sharedWorkspace.frontmostApplication;
            qCDebug(lcQpaApplication) << "Launched with" << frontmostApplication
                << "as frontmost application. Activating" << currentApplication << "instead.";
            [NSApplication.sharedApplication activateIgnoringOtherApps:YES];
        }

        if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSSonoma) {
            // Qt windows are typically shown in main(), at which point the application
            // is not active yet. When the application is activated, either externally
            // or via the override above, it will only bring the main and key windows
            // forward, which differs from the behavior if these windows had been shown
            // once the application was already active. To work around this, we explicitly
            // activate the current application again, bringing all windows to the front.
            // We only do this on Sonoma and up, as earlier macOS versions have a bug where
            // the app will deactivate as part of activating, even if it's active app,
            // which in turn results in losing key window status for the key window.
            // FIXME: Consider bringing our windows to the front via orderFront instead,
            // or deferring the orderFront during setVisible until the app is active.
            [currentApplication activateWithOptions:NSApplicationActivateAllWindows];
        }
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

- (BOOL)application:(NSApplication *)application continueUserActivity:(NSUserActivity *)userActivity
          restorationHandler:(void(^)(NSArray<id<NSUserActivityRestoring>> *restorableObjects))restorationHandler
{
    // Check if eg. user has installed an app delegate capable of handling this
    if ([reflectionDelegate respondsToSelector:_cmd]
        && [reflectionDelegate application:application continueUserActivity:userActivity
                         restorationHandler:restorationHandler] == YES) {
        return YES;
    }

    if (!QGuiApplication::instance())
        return NO;

    if ([userActivity.activityType isEqualToString:NSUserActivityTypeBrowsingWeb]) {
        QCocoaIntegration *cocoaIntegration = QCocoaIntegration::instance();
        Q_ASSERT(cocoaIntegration);
        return cocoaIntegration->services()->handleUrl(QUrl::fromNSURL(userActivity.webpageURL));
    }

    return NO;
}

- (void)getUrl:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent
{
    Q_UNUSED(replyEvent);

    NSString *urlString = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];
    const QString qurlString = QString::fromNSString(urlString);

    if (event.eventClass == kInternetEventClass && event.eventID == kAEGetURL) {
        // 'GURL' (Get URL) event this application should handle
        if (!QGuiApplication::instance())
            return;
        QCocoaIntegration *cocoaIntegration = QCocoaIntegration::instance();
        Q_ASSERT(cocoaIntegration);
        if (cocoaIntegration->services()->handleUrl(QUrl(qurlString)))
            return;
    }

    // The string we get from the requesting application might not necessarily meet
    // QUrl's requirement for a IDN-compliant host. So if we can't parse into a QUrl,
    // then we pass the string on to the application as the name of a file (and
    // QFileOpenEvent::file is not guaranteed to be the path to a local, open'able
    // file anyway).
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

    QGuiApplicationPrivate::modifier_buttons = QAppleKeyMapper::fromCocoaModifiers([NSEvent modifierFlags]);

    static QMetaMethod activatedSignal = QMetaMethod::fromSignal(&QCocoaMenuItem::activated);
    activatedSignal.invoke(platformItem, Qt::QueuedConnection);
}

@end
