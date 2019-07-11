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

#import "qcocoansmenu.h"
#include "qcocoamenu.h"
#include "qcocoamenuitem.h"
#include "qcocoamenubar.h"
#include "qcocoawindow.h"
#import "qnsview.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qcoreevent.h>

static NSString *qt_mac_removePrivateUnicode(NSString *string)
{
    if (const int len = string.length) {
        QVarLengthArray<unichar, 10> characters(len);
        bool changed = false;
        for (int i = 0; i < len; i++) {
            characters[i] = [string characterAtIndex:i];
            // check if they belong to key codes in private unicode range
            // currently we need to handle only the NSDeleteFunctionKey
            if (characters[i] == NSDeleteFunctionKey) {
                characters[i] = NSDeleteCharacter;
                changed = true;
            }
        }
        if (changed)
            return [NSString stringWithCharacters:characters.data() length:len];
    }
    return string;
}

@implementation QCocoaNSMenu
{
    QPointer<QCocoaMenu> _platformMenu;
}

- (instancetype)initWithPlatformMenu:(QCocoaMenu *)menu
{
    if ((self = [super initWithTitle:@"Untitled"])) {
        _platformMenu = menu;
        self.autoenablesItems = YES;
        self.delegate = [QCocoaNSMenuDelegate sharedMenuDelegate];
    }

    return self;
}

- (QCocoaMenu *)platformMenu
{
    return _platformMenu.data();
}

@end

@implementation QCocoaNSMenuItem
{
    QPointer<QCocoaMenuItem> _platformMenuItem;
}

+ (instancetype)separatorItemWithPlatformMenuItem:(QCocoaMenuItem *)menuItem
{
    // Safe because +[NSMenuItem separatorItem] invokes [[self alloc] init]
    auto *item = qt_objc_cast<QCocoaNSMenuItem *>([self separatorItem]);
    Q_ASSERT_X(item, qPrintable(__FUNCTION__),
               "Did +[NSMenuItem separatorItem] not invoke [[self alloc] init]?");
    if (item)
        item.platformMenuItem = menuItem;

    return item;
}

- (instancetype)initWithPlatformMenuItem:(QCocoaMenuItem *)menuItem
{
    if ((self = [super initWithTitle:@"" action:nil keyEquivalent:@""])) {
        _platformMenuItem = menuItem;
    }

    return self;
}

- (instancetype)init
{
    return [self initWithPlatformMenuItem:nullptr];
}

- (QCocoaMenuItem *)platformMenuItem
{
    return _platformMenuItem.data();
}

- (void)setPlatformMenuItem:(QCocoaMenuItem *)menuItem
{
    _platformMenuItem = menuItem;
}

@end

#define CHECK_MENU_CLASS(menu) Q_ASSERT_X([menu isMemberOfClass:[QCocoaNSMenu class]], \
                                          __FUNCTION__, "Menu is not a QCocoaNSMenu")

@implementation QCocoaNSMenuDelegate

+ (instancetype)sharedMenuDelegate
{
    static QCocoaNSMenuDelegate *shared = nil;
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

- (NSInteger)numberOfItemsInMenu:(NSMenu *)menu
{
    CHECK_MENU_CLASS(menu);
    return menu.numberOfItems;
}

- (BOOL)menu:(NSMenu *)menu updateItem:(NSMenuItem *)item atIndex:(NSInteger)index shouldCancel:(BOOL)shouldCancel
{
    Q_UNUSED(index);
    CHECK_MENU_CLASS(menu);

    if (shouldCancel)
        return NO;

    const auto &platformMenu = static_cast<QCocoaNSMenu *>(menu).platformMenu;
    if (!platformMenu)
        return YES;

    if (auto *platformItem = qt_objc_cast<QCocoaNSMenuItem *>(item).platformMenuItem) {
        if (platformMenu->items().contains(platformItem)) {
            if (auto *itemSubmenu = platformItem->menu())
                itemSubmenu->setAttachedItem(item);
        }
    }

    return YES;
}

- (void)menu:(NSMenu *)menu willHighlightItem:(NSMenuItem *)item
{
    CHECK_MENU_CLASS(menu);
    if (auto *platformItem = qt_objc_cast<QCocoaNSMenuItem *>(item).platformMenuItem)
        emit platformItem->hovered();
}

- (void)menuWillOpen:(NSMenu *)menu
{
    CHECK_MENU_CLASS(menu);
    auto *platformMenu = static_cast<QCocoaNSMenu *>(menu).platformMenu;
    if (!platformMenu)
        return;

    platformMenu->setIsOpen(true);
    platformMenu->setIsAboutToShow(true);
    emit platformMenu->aboutToShow();
    platformMenu->setIsAboutToShow(false);
}

- (void)menuDidClose:(NSMenu *)menu
{
    CHECK_MENU_CLASS(menu);
    auto *platformMenu = static_cast<QCocoaNSMenu *>(menu).platformMenu;
    if (!platformMenu)
        return;

    platformMenu->setIsOpen(false);
    // wrong, but it's the best we can do
    emit platformMenu->aboutToHide();
}

- (BOOL)menuHasKeyEquivalent:(NSMenu *)menu forEvent:(NSEvent *)event target:(id *)target action:(SEL *)action
{
    /*
       Check if the menu actually has a keysequence defined for this key event.
       If it does, then we will first send the key sequence to the QWidget that has focus
       since (in Qt's eyes) it needs to a chance at the key event first (QEvent::ShortcutOverride).
       If the widget accepts the key event, we then return YES, but set the target and action to be nil,
       which means that the action should not be triggered, and instead dispatch the event ourselves.
       In every other case we return NO, which means that Cocoa can do as it pleases
       (i.e., fire the menu action).
    */

    CHECK_MENU_CLASS(menu);

    // Interested only in Shift, Cmd, Ctrl & Alt Keys, so ignoring masks like, Caps lock, Num Lock ...
    static const NSUInteger mask = NSEventModifierFlagShift | NSEventModifierFlagControl
                                 | NSEventModifierFlagCommand | NSEventModifierFlagOption;

    // Change the private unicode keys to the ones used in setting the "Key Equivalents"
    NSString *characters = qt_mac_removePrivateUnicode(event.charactersIgnoringModifiers);
    const auto modifiers = event.modifierFlags & mask;
    NSMenuItem *keyEquivalentItem = [self findItemInMenu:menu
                                                  forKey:characters
                                               modifiers:modifiers];
    if (!keyEquivalentItem) {
        // Maybe the modified character is what we're looking for after all
        characters = qt_mac_removePrivateUnicode(event.characters);
        keyEquivalentItem = [self findItemInMenu:menu
                                          forKey:characters
                                       modifiers:modifiers];
    }

    if (keyEquivalentItem) {
        QObject *object = qApp->focusObject();
        if (object) {
            QChar ch;
            int keyCode;
            ulong nativeModifiers = event.modifierFlags;
            Qt::KeyboardModifiers modifiers = [QNSView convertKeyModifiers:nativeModifiers];
            NSString *charactersIgnoringModifiers = event.charactersIgnoringModifiers;
            NSString *characters = event.characters;

            if (charactersIgnoringModifiers.length > 0) { // convert the first character into a key code
                if ((modifiers & Qt::ControlModifier) && characters.length > 0) {
                    ch = QChar([characters characterAtIndex:0]);
                } else {
                    ch = QChar([charactersIgnoringModifiers characterAtIndex:0]);
                }
                keyCode = qt_mac_cocoaKey2QtKey(ch);
            } else {
                // might be a dead key
                ch = QChar::ReplacementCharacter;
                keyCode = Qt::Key_unknown;
            }

            QKeyEvent accel_ev(QEvent::ShortcutOverride, (keyCode & (~Qt::KeyboardModifierMask)),
                               Qt::KeyboardModifiers(modifiers & Qt::KeyboardModifierMask));
            accel_ev.ignore();
            QCoreApplication::sendEvent(object, &accel_ev);
            if (accel_ev.isAccepted()) {
                [[NSApp keyWindow] sendEvent:event];
                *target = nil;
                *action = nil;
                return YES;
            }
        }
    }

    return NO;
}

- (NSMenuItem *)findItemInMenu:(NSMenu *)menu
                        forKey:(NSString *)key
                     modifiers:(NSUInteger)modifiers
{
    // Find an item in 'menu' that has the same key equivalent as specified by
    // 'key' and 'modifiers'. We ignore disabled, hidden and separator items.
    // In a similar fashion, we don't need to recurse into submenus because their
    // delegate will have [menuHasKeyEquivalent:...] invoked at some point.

    for (NSMenuItem *item in menu.itemArray) {
        if (!item.enabled || item.hidden || item.separatorItem)
            continue;

        if (item.hasSubmenu)
            continue;

        NSString *menuKey = item.keyEquivalent;
        if (menuKey && NSOrderedSame == [menuKey compare:key]
            && modifiers == item.keyEquivalentModifierMask)
                return item;
    }

    return nil;
}

@end

#undef CHECK_MENU_CLASS
