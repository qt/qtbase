// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// This file is included from qnsview.mm, and only used to organize the code

#include "qcocoaapplicationdelegate.h"
#include "qcocoansmenu.h"
#include "qcocoamenuitem.h"
#include "qcocoamenu.h"
#include "qcocoamenubar.h"

@implementation QNSView (Menus)

// Qt does not (yet) have a mechanism for propagating generic actions,
// so we can only support actions that originate from a QCocoaNSMenuItem,
// where we can forward the action by emitting QPlatformMenuItem::activated().
// But waiting for forwardInvocation to check that the sender is a
// QCocoaNSMenuItem is too late, as AppKit has at that point chosen
// our view as the target for the action, and if we can't handle it
// the action will not propagate up the responder chain as it should.
// Instead, we hook in early in the process of determining the target
// via the supplementalTargetForAction API, and if we can support the
// action we forward it to a helper. The helper must be tied to the
// view, as the menu validation logic depends on the view's state.

- (id)supplementalTargetForAction:(SEL)action sender:(id)sender
{
    qCDebug(lcQpaMenus) << "Resolving action target for" << action << "from" << sender << "via" << self;

    if (qt_objc_cast<QCocoaNSMenuItem *>(sender)) {
        // The supplemental target must support the selector, but we
        // determine so dynamically, so check here before continuing.
        if ([self.menuHelper respondsToSelector:action])
            return self.menuHelper;
    } else {
        qCDebug(lcQpaMenus) << "Ignoring action for menu item we didn't create";
    }

    return [super supplementalTargetForAction:action sender:sender];
}

@end

@interface QNSViewMenuHelper ()
@property (assign) QNSView* view;
@end

@implementation QNSViewMenuHelper

- (instancetype)initWithView:(QNSView *)theView
{
    if ((self = [super init]))
        self.view = theView;

    return self;
}

- (BOOL)validateMenuItem:(NSMenuItem*)item
{
    qCDebug(lcQpaMenus) << "Validating" << item << "for" << self.view;

    auto *nativeItem = qt_objc_cast<QCocoaNSMenuItem *>(item);
    if (!nativeItem)
        return item.enabled; // FIXME Test with with Qt as plugin or embedded QWindow.

    auto *platformItem = nativeItem.platformMenuItem;
    if (!platformItem)
        return NO;

    // Menu-holding items are always enabled, as it's conventional in Cocoa
    if (platformItem->menu())
        return YES;

    // Check if a modal dialog is active. If so, enable only menu
    // items explicitly belonging to this window's own menu bar, or to the window.
    if (QGuiApplication::modalWindow() && QGuiApplication::modalWindow()->isActive()) {
        QCocoaMenuBar *menubar = nullptr;
        QCocoaWindow *menuWindow = nullptr;

        QObject *menuParent = platformItem->menuParent();
        while (menuParent && !(menubar = qobject_cast<QCocoaMenuBar *>(menuParent))) {
            menuWindow = qobject_cast<QCocoaWindow *>(menuParent);
            auto *menuObject = dynamic_cast<QCocoaMenuObject *>(menuParent);
            menuParent = menuObject ? menuObject->menuParent() : nullptr;
        }

        if ((!menuWindow || menuWindow->window() != QGuiApplication::modalWindow())
            && (!menubar || menubar->cocoaWindow() != self.view.platformWindow))
            return NO;
    }

    return platformItem->isEnabled();
}

- (BOOL)respondsToSelector:(SEL)selector
{
    // See QCocoaMenuItem::resolveTargetAction()

    if (selector == @selector(cut:)
     || selector == @selector(copy:)
     || selector == @selector(paste:)
     || selector == @selector(selectAll:)) {
        // Not exactly true. Both copy: and selectAll: can work on non key views.
        return NSApp.keyWindow == self.view.window
            && self.view.window.firstResponder == self.view;
    }

    if (selector == @selector(qt_itemFired:))
        return YES;

    return [super respondsToSelector:selector];
}

- (NSMethodSignature *)methodSignatureForSelector:(SEL)selector
{
    // Double check, in case something has cached that we respond
    // to the selector, but the result has changed since then.
    if (![self respondsToSelector:selector])
        return nil;

    auto *appDelegate = [QCocoaApplicationDelegate sharedDelegate];
    return [appDelegate methodSignatureForSelector:@selector(qt_itemFired:)];
}

- (void)forwardInvocation:(NSInvocation *)invocation
{
    NSObject *sender;
    [invocation getArgument:&sender atIndex:2];
    qCDebug(lcQpaMenus) << "Forwarding" << invocation.selector << "from" << sender;
    Q_ASSERT(qt_objc_cast<QCocoaNSMenuItem *>(sender));
    invocation.selector = @selector(qt_itemFired:);
    [invocation invokeWithTarget:[QCocoaApplicationDelegate sharedDelegate]];
}

@end
