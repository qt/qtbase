// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// This file is included from qnsview.mm, and only used to organize the code

#include "qcocoaapplicationdelegate.h"
#include "qcocoansmenu.h"
#include "qcocoamenuitem.h"
#include "qcocoamenu.h"
#include "qcocoamenubar.h"

static bool selectorIsCutCopyPaste(SEL selector)
{
    return (selector == @selector(cut:)
            || selector == @selector(copy:)
            || selector == @selector(paste:)
            || selector == @selector(selectAll:));
}

@interface QNSView (Menus)
- (void)qt_itemFired:(QCocoaNSMenuItem *)item;
@end

@implementation QNSView (Menus)

- (BOOL)validateMenuItem:(NSMenuItem*)item
{
    qCDebug(lcQpaMenus) << "Validating" << item << "for" << self;

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
            && (!menubar || menubar->cocoaWindow() != self.platformWindow))
            return NO;
    }

    return platformItem->isEnabled();
}

- (BOOL)respondsToSelector:(SEL)selector
{
    // Not exactly true. Both copy: and selectAll: can work on non key views.
    if (selectorIsCutCopyPaste(selector))
        return ([NSApp keyWindow] == self.window) && (self.window.firstResponder == self);

    return [super respondsToSelector:selector];
}

- (void)qt_itemFired:(QCocoaNSMenuItem *)item
{
    auto *appDelegate = [QCocoaApplicationDelegate sharedDelegate];
    [appDelegate qt_itemFired:item];
}

- (NSMethodSignature *)methodSignatureForSelector:(SEL)selector
{
    if (selectorIsCutCopyPaste(selector)) {
        NSMethodSignature *itemFiredSign = [super methodSignatureForSelector:@selector(qt_itemFired:)];
        return itemFiredSign;
    }

    return [super methodSignatureForSelector:selector];
}

- (void)forwardInvocation:(NSInvocation *)invocation
{
    if (selectorIsCutCopyPaste(invocation.selector)) {
        NSObject *sender;
        [invocation getArgument:&sender atIndex:2];
        qCDebug(lcQpaMenus) << "Forwarding" << invocation.selector << "from" << sender;

        // We claim to respond to standard edit actions such as cut/copy/paste,
        // but these might not be exclusively coming from menu items that we
        // control. For example, when embedded into a native UI (as a plugin),
        // the menu items might be part of the host application, and if we're
        // the first responder, we'll be the target of these actions. As we
        // don't have a mechanism in Qt to trigger generic actions, we have
        // to bail out if we don't have a QCocoaNSMenuItem we can activate().
        // Note that we skip the call to super as well, as that would just
        // try to invoke the current action on ourselves again.
        if (auto *nativeItem = qt_objc_cast<QCocoaNSMenuItem *>(sender))
            [self qt_itemFired:nativeItem];
        else
            qCDebug(lcQpaMenus) << "Ignoring action for menu item we didn't create";

        return;
    }

    [super forwardInvocation:invocation];
}

@end
