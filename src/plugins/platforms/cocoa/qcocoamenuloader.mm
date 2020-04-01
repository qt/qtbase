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

#include "qcocoamenuloader.h"

#include "qcocoahelpers.h"
#include "qcocoansmenu.h"
#include "qcocoamenubar.h"
#include "qcocoamenuitem.h"
#include "qcocoaintegration.h"

#include <QtCore/private/qcore_mac_p.h>
#include <QtCore/private/qthread_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtGui/private/qguiapplication_p.h>

@implementation QCocoaMenuLoader {
    NSMenu *theMenu;
    NSMenu *appMenu;
    NSMenuItem *quitItem;
    NSMenuItem *preferencesItem;
    NSMenuItem *aboutItem;
    NSMenuItem *aboutQtItem;
    NSMenuItem *hideItem;
    NSMenuItem *servicesItem;
    NSMenuItem *hideAllOthersItem;
    NSMenuItem *showAllItem;
}

+ (instancetype)sharedMenuLoader
{
    static QCocoaMenuLoader *shared = nil;
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
    if ((self = [super init])) {
        NSString *appName = qt_mac_applicationName().toNSString();

        // Menubar as menu. Title as set in the NIB file
        theMenu = [[NSMenu alloc] initWithTitle:@"Main Menu"];

        // Application menu. Since 10.6, the first menu
        // is always identified as the application menu.
        NSMenuItem *appItem = [[[NSMenuItem alloc] init] autorelease];
        appItem.title = appName;
        [theMenu addItem:appItem];
        appMenu = [[NSMenu alloc] initWithTitle:appName];
        appItem.submenu = appMenu;

        // About Application
        aboutItem = [[QCocoaNSMenuItem alloc] init];
        aboutItem.title = [@"About " stringByAppendingString:appName];
        // FIXME This seems useless since barely adding a QAction
        // with AboutRole role will reset the target/action
        aboutItem.target = self;
        aboutItem.action = @selector(orderFrontStandardAboutPanel:);
        // Disable until a QAction is associated
        aboutItem.enabled = NO;
        aboutItem.hidden = YES;
        [appMenu addItem:aboutItem];

        // About Qt (shameless self-promotion)
        aboutQtItem = [[QCocoaNSMenuItem alloc] init];
        aboutQtItem.title = @"About Qt";
        // Disable until a QAction is associated
        aboutQtItem.enabled = NO;
        aboutQtItem.hidden = YES;
        [appMenu addItem:aboutQtItem];

        [appMenu addItem:[NSMenuItem separatorItem]];

        // Preferences
        // We'll be adding app specific items after this. The macOS HIG state that,
        // "In general, a Preferences menu item should be the first app-specific menu item."
        // https://developer.apple.com/macos/human-interface-guidelines/menus/menu-bar-menus/
        preferencesItem = [[QCocoaNSMenuItem alloc] init];
        preferencesItem.title = @"Preferences…";
        preferencesItem.keyEquivalent = @",";
        // Disable until a QAction is associated
        preferencesItem.enabled = NO;
        preferencesItem.hidden = YES;
        [appMenu addItem:preferencesItem];

        [appMenu addItem:[NSMenuItem separatorItem]];

        // Services item and menu
        servicesItem = [[NSMenuItem alloc] init];
        servicesItem.title = @"Services";
        NSMenu *servicesMenu = [[[NSMenu alloc] initWithTitle:@"Services"] autorelease];
        servicesItem.submenu = servicesMenu;
        [NSApplication sharedApplication].servicesMenu = servicesMenu;
        [appMenu addItem:servicesItem];

        [appMenu addItem:[NSMenuItem separatorItem]];

        // Hide Application
        hideItem = [[NSMenuItem alloc] initWithTitle:[@"Hide " stringByAppendingString:appName]
                                              action:@selector(hide:)
                                       keyEquivalent:@"h"];
        hideItem.target = self;
        [appMenu addItem:hideItem];

        // Hide Others
        hideAllOthersItem = [[NSMenuItem alloc] initWithTitle:@"Hide Others"
                                                       action:@selector(hideOtherApplications:)
                                                keyEquivalent:@"h"];
        hideAllOthersItem.target = self;
        hideAllOthersItem.keyEquivalentModifierMask = NSEventModifierFlagCommand | NSEventModifierFlagOption;
        [appMenu addItem:hideAllOthersItem];

        // Show All
        showAllItem = [[NSMenuItem alloc] initWithTitle:@"Show All"
                                                 action:@selector(unhideAllApplications:)
                                          keyEquivalent:@""];
        showAllItem.target = self;
        [appMenu addItem:showAllItem];

        [appMenu addItem:[NSMenuItem separatorItem]];

        // Quit Application
        quitItem = [[QCocoaNSMenuItem alloc] init];
        quitItem.title = [@"Quit " stringByAppendingString:appName];
        quitItem.keyEquivalent = @"q";
        // This will remain true until synced with a QCocoaMenuItem.
        // This way, we will always have a functional Quit menu item
        // even if no QAction is added.
        quitItem.action = @selector(terminate:);
        [appMenu addItem:quitItem];
    }

    return self;
}

- (void)dealloc
{
    [theMenu release];
    [appMenu release];
    [aboutItem release];
    [aboutQtItem release];
    [preferencesItem release];
    [servicesItem release];
    [hideItem release];
    [hideAllOthersItem release];
    [showAllItem release];
    [quitItem release];

    [super dealloc];
}

- (void)ensureAppMenuInMenu:(NSMenu *)menu
{
    // The application menu is the menu in the menu bar that contains the
    // 'Quit' item. When changing menu bar (e.g when switching between
    // windows with different menu bars), we never recreate this menu, but
    // instead pull it out the current menu bar and place into the new one:
    NSMenu *mainMenu = [NSApp mainMenu];
    if (mainMenu == menu)
        return; // nothing to do (menu is the current menu bar)!

#ifndef QT_NAMESPACE
    Q_ASSERT(mainMenu);
#endif
    // Grab the app menu out of the current menu.
    auto unparentAppMenu = ^bool (NSMenu *supermenu) {
        auto index = [supermenu indexOfItemWithSubmenu:appMenu];
        if (index != -1) {
            [supermenu removeItemAtIndex:index];
            return true;
        }
        return false;
    };

    if (!mainMenu || !unparentAppMenu(mainMenu))
        if (appMenu.supermenu)
            unparentAppMenu(appMenu.supermenu);

    NSMenuItem *appMenuItem = [[NSMenuItem alloc] initWithTitle:@"Apple"
                               action:nil keyEquivalent:@""];
    appMenuItem.submenu = appMenu;
    [menu insertItem:appMenuItem atIndex:0];
}

- (NSMenu *)menu
{
    return [[theMenu retain] autorelease];
}

- (NSMenu *)applicationMenu
{
    return [[appMenu retain] autorelease];
}

- (NSMenuItem *)quitMenuItem
{
    return [[quitItem retain] autorelease];
}

- (NSMenuItem *)preferencesMenuItem
{
    return [[preferencesItem retain] autorelease];
}

- (NSMenuItem *)aboutMenuItem
{
    return [[aboutItem retain] autorelease];
}

- (NSMenuItem *)aboutQtMenuItem
{
    return [[aboutQtItem retain] autorelease];
}

- (NSMenuItem *)hideMenuItem
{
    return [[hideItem retain] autorelease];
}

- (NSMenuItem *)appSpecificMenuItem:(QCocoaMenuItem *)platformItem
{
    // No reason to create the item if it already exists.
    for (NSMenuItem *item in appMenu.itemArray)
        if (qt_objc_cast<QCocoaNSMenuItem *>(item).platformMenuItem == platformItem)
            return item;

    // Create an App-Specific menu item, insert it into the menu and return
    // it as an autorelease item.
    QCocoaNSMenuItem *item;
    if (platformItem->isSeparator())
        item = [QCocoaNSMenuItem separatorItemWithPlatformMenuItem:platformItem];
    else
        item = [[[QCocoaNSMenuItem alloc] initWithPlatformMenuItem:platformItem] autorelease];

    const auto location = [self indexOfLastAppSpecificMenuItem];
    [appMenu insertItem:item atIndex:NSInteger(location) + 1];

    return item;
}

- (void)orderFrontStandardAboutPanel:(id)sender
{
    [NSApp orderFrontStandardAboutPanel:sender];
}

- (void)hideOtherApplications:(id)sender
{
    [NSApp hideOtherApplications:sender];
}

- (void)unhideAllApplications:(id)sender
{
    [NSApp unhideAllApplications:sender];
}

- (void)hide:(id)sender
{
    [NSApp hide:sender];
}

- (void)qtTranslateApplicationMenu
{
#ifndef QT_NO_TRANSLATION
    aboutItem.title = qt_mac_applicationmenu_string(AboutAppMenuItem).arg(qt_mac_applicationName()).toNSString();
    preferencesItem.title = qt_mac_applicationmenu_string(PreferencesAppMenuItem).toNSString();
    servicesItem.title = qt_mac_applicationmenu_string(ServicesAppMenuItem).toNSString();
    hideItem.title = qt_mac_applicationmenu_string(HideAppMenuItem).arg(qt_mac_applicationName()).toNSString();
    hideAllOthersItem.title = qt_mac_applicationmenu_string(HideOthersAppMenuItem).toNSString();
    showAllItem.title = qt_mac_applicationmenu_string(ShowAllAppMenuItem).toNSString();
    quitItem.title = qt_mac_applicationmenu_string(QuitAppMenuItem).arg(qt_mac_applicationName()).toNSString();
#endif
}

- (BOOL)validateMenuItem:(NSMenuItem*)menuItem
{
    if (menuItem.action == @selector(hideOtherApplications:)
        || menuItem.action == @selector(unhideAllApplications:))
        return [NSApp validateMenuItem:menuItem];

    if (menuItem.action == @selector(hide:)) {
        if (QCocoaIntegration::instance()->activePopupWindow())
            return NO;
        return [NSApp validateMenuItem:menuItem];
    }

    return menuItem.enabled;
}

- (NSArray<NSMenuItem *> *)mergeable
{
    // Don't include the quitItem here, since we want it always visible and enabled regardless
    auto items = [NSArray arrayWithObjects:preferencesItem, aboutItem,  aboutQtItem,
                  appMenu.itemArray[[self indexOfLastAppSpecificMenuItem]], nil];
    return items;
}

- (NSUInteger)indexOfLastAppSpecificMenuItem
{
    // Either the 'Preferences', which is the first app specific menu item, or something
    // else we appended later (thus the reverse order):
    const auto location = [appMenu.itemArray indexOfObjectWithOptions:NSEnumerationReverse
                           passingTest:^BOOL(NSMenuItem *item, NSUInteger, BOOL *) {
                               if (auto qtItem = qt_objc_cast<QCocoaNSMenuItem*>(item))
                                   return qtItem != quitItem;
                              return NO;
                           }];
    Q_ASSERT(location != NSNotFound);
    return location;
}


@end
