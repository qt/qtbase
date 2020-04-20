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

    // Check if a modal dialog is active. Validate only menu
    // items belonging to this view's window own menu bar.
    if (QGuiApplication::modalWindow()) {
        QCocoaMenuBar *menubar = nullptr;

        QObject *menuParent = platformItem->menuParent();
        while (menuParent && !(menubar = qobject_cast<QCocoaMenuBar *>(menuParent))) {
            auto *menuObject = dynamic_cast<QCocoaMenuObject *>(menuParent);
            menuParent = menuObject->menuParent();
        }

        // we have no menubar parent for the application menu items, e.g About and Preferences
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
