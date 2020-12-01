/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
    // items explicitly belonging to this window's own menu bar.
    if (QGuiApplication::modalWindow() && QGuiApplication::modalWindow()->isActive()) {
        QCocoaMenuBar *menubar = nullptr;

        QObject *menuParent = platformItem->menuParent();
        while (menuParent && !(menubar = qobject_cast<QCocoaMenuBar *>(menuParent))) {
            auto *menuObject = dynamic_cast<QCocoaMenuObject *>(menuParent);
            menuParent = menuObject->menuParent();
        }

        if (!menubar || menubar->cocoaWindow() != self.platformWindow)
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
        if (auto *nativeItem = qt_objc_cast<QCocoaNSMenuItem *>(sender)) {
            [self qt_itemFired:nativeItem];
            return;
        }
    }

    [super forwardInvocation:invocation];
}

@end
