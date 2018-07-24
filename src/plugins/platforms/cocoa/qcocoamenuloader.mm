/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "messages.h"
#include "qcocoahelpers.h"
#include "qcocoamenubar.h"
#include "qcocoamenuitem.h"
#include "qcocoaintegration.h"

#include <QtCore/private/qcore_mac_p.h>
#include <QtCore/private/qthread_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtGui/private/qguiapplication_p.h>

@implementation QCocoaMenuLoader

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
        aboutItem = [[NSMenuItem alloc] initWithTitle:[@"About " stringByAppendingString:appName]
                                               action:@selector(orderFrontStandardAboutPanel:)
                                        keyEquivalent:@""];
        aboutItem.target = self;
        // Disable until a QAction is associated
        aboutItem.enabled = NO;
        aboutItem.hidden = YES;
        [appMenu addItem:aboutItem];

        // About Qt (shameless self-promotion)
        aboutQtItem = [[NSMenuItem alloc] init];
        aboutQtItem.title = @"About Qt";
        // Disable until a QAction is associated
        aboutQtItem.enabled = NO;
        aboutQtItem.hidden = YES;
        [appMenu addItem:aboutQtItem];

        [appMenu addItem:[NSMenuItem separatorItem]];

        // Preferences
        preferencesItem = [[NSMenuItem alloc] initWithTitle:@"Preferences…"
                                                     action:@selector(qtDispatcherToQPAMenuItem:)
                                              keyEquivalent:@","];
        preferencesItem.target = self;
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
        hideAllOthersItem.keyEquivalentModifierMask = NSCommandKeyMask | NSAlternateKeyMask;
        [appMenu addItem:hideAllOthersItem];

        // Show All
        showAllItem = [[NSMenuItem alloc] initWithTitle:@"Show All"
                                                 action:@selector(unhideAllApplications:)
                                          keyEquivalent:@""];
        showAllItem.target = self;
        [appMenu addItem:showAllItem];

        [appMenu addItem:[NSMenuItem separatorItem]];

        // Quit Application
        quitItem = [[NSMenuItem alloc] initWithTitle:[@"Quit " stringByAppendingString:appName]
                                              action:@selector(terminate:)
                                       keyEquivalent:@"q"];
        quitItem.target = self;
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

    [lastAppSpecificItem release];

    [super dealloc];
}

- (void)ensureAppMenuInMenu:(NSMenu *)menu
{
    // The application menu is the menu in the menu bar that contains the
    // 'Quit' item. When changing menu bar (e.g when switching between
    // windows with different menu bars), we never recreate this menu, but
    // instead pull it out the current menu bar and place into the new one:
    NSMenu *mainMenu = [NSApp mainMenu];
    if ([NSApp mainMenu] == menu)
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
    [appMenuItem setSubmenu:appMenu];
    [menu insertItem:appMenuItem atIndex:0];
}

- (void)removeActionsFromAppMenu
{
    for (NSMenuItem *item in [appMenu itemArray])
        [item setTag:0];
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

- (NSMenuItem *)appSpecificMenuItem:(NSInteger)tag
{
    NSMenuItem *item = [appMenu itemWithTag:tag];

    // No reason to create the item if it already exists. See QTBUG-27202.
    if (item)
        return [[item retain] autorelease];

    // Create an App-Specific menu item, insert it into the menu and return
    // it as an autorelease item.
    item = [[NSMenuItem alloc] init];

    NSInteger location;
    if (lastAppSpecificItem == nil) {
        location = [appMenu indexOfItem:aboutQtItem];
    } else {
        location = [appMenu indexOfItem:lastAppSpecificItem];
        [lastAppSpecificItem release];
    }
    lastAppSpecificItem = item;  // Keep track of this for later (i.e., don't release it)
    [appMenu insertItem:item atIndex:location + 1];

    return [[item retain] autorelease];
}

- (BOOL) acceptsFirstResponder
{
    return YES;
}

- (void)terminate:(id)sender
{
    [NSApp terminate:sender];
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
    [servicesItem setTitle:qt_mac_applicationmenu_string(ServicesAppMenuItem).toNSString()];
    [hideItem setTitle:qt_mac_applicationmenu_string(HideAppMenuItem).arg(qt_mac_applicationName()).toNSString()];
    [hideAllOthersItem setTitle:qt_mac_applicationmenu_string(HideOthersAppMenuItem).toNSString()];
    [showAllItem setTitle:qt_mac_applicationmenu_string(ShowAllAppMenuItem).toNSString()];
    [preferencesItem setTitle:qt_mac_applicationmenu_string(PreferencesAppMenuItem).toNSString()];
    [quitItem setTitle:qt_mac_applicationmenu_string(QuitAppMenuItem).arg(qt_mac_applicationName()).toNSString()];
    [aboutItem setTitle:qt_mac_applicationmenu_string(AboutAppMenuItem).arg(qt_mac_applicationName()).toNSString()];
#endif
}

- (IBAction)qtDispatcherToQPAMenuItem:(id)sender
{
    NSMenuItem *item = static_cast<NSMenuItem *>(sender);
    if (item == quitItem) {
        // We got here because someone was once the quitItem, but it has been
        // abandoned (e.g., the menubar was deleted). In the meantime, just do
        // normal QApplication::quit().
        qApp->quit();
        return;
    }

    if ([item tag]) {
        QCocoaMenuItem *cocoaItem = reinterpret_cast<QCocoaMenuItem *>([item tag]);
        QScopedScopeLevelCounter scopeLevelCounter(QGuiApplicationPrivate::instance()->threadData);
        cocoaItem->activated();
    }
}

- (void)orderFrontCharacterPalette:(id)sender
{
    [NSApp orderFrontCharacterPalette:sender];
}

- (BOOL)validateMenuItem:(NSMenuItem*)menuItem
{
    if ([menuItem action] == @selector(hideOtherApplications:)
        || [menuItem action] == @selector(unhideAllApplications:)) {
        return [NSApp validateMenuItem:menuItem];
    } else if ([menuItem action] == @selector(hide:)) {
        if (QCocoaIntegration::instance()->activePopupWindow())
            return NO;
        return [NSApp validateMenuItem:menuItem];
    } else if ([menuItem tag]) {
        QCocoaMenuItem *cocoaItem = reinterpret_cast<QCocoaMenuItem *>([menuItem tag]);
        return cocoaItem->isEnabled();
    } else {
        return [menuItem isEnabled];
    }
}

- (NSArray*) mergeable
{
    // don't include the quitItem here, since we want it always visible and enabled regardless
    return [NSArray arrayWithObjects:preferencesItem, aboutItem, aboutQtItem, lastAppSpecificItem, nil];
}

@end
