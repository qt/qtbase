/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qapplication.h"
#include "qvarlengtharray.h"
#import "qcocoamenu.h"
#import "qcocoamenuloader.h"
#import "qcocoaapplication.h"
#include "qcocoahelpers.h"
#include <private/qapplication_p.h>
#include <private/qaction_p.h>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QWidget)
QT_FORWARD_DECLARE_CLASS(QApplication)
QT_FORWARD_DECLARE_CLASS(QCoreApplication)
QT_FORWARD_DECLARE_CLASS(QApplicationPrivate)
QT_FORWARD_DECLARE_CLASS(QKeyEvent)
QT_FORWARD_DECLARE_CLASS(QEvent)

QT_BEGIN_NAMESPACE
extern void qt_mac_menu_collapseSeparators(NSMenu *menu, bool collapse);
void qt_mac_clear_status_text(QAction *action);
extern void qt_mac_emit_menuSignals(QMenu *menu, bool show);
extern void qt_mac_menu_emit_hovered(QMenu *menu, QAction *action);
QT_END_NAMESPACE

QT_USE_NAMESPACE

@implementation QT_MANGLE_NAMESPACE(QNativeCocoaMenu)

- (id)initWithQMenu:(QMenu*)menu
{
    self = [super init];
    if (self) {
        qmenu = menu;
        previousAction = 0;
        [self setAutoenablesItems:NO];
        [self setDelegate:self];
    }
    return self;
}

- (void)menu:(NSMenu*)menu willHighlightItem:(NSMenuItem*)item
{
    Q_UNUSED(menu);

    if (!item) {
        if (previousAction) {
            qt_mac_clear_status_text(previousAction);
            previousAction = 0;
        }
        return;
    }

    if (QAction *action = reinterpret_cast<QAction *>([item tag])) {
        QMenu *qtmenu = static_cast<QT_MANGLE_NAMESPACE(QNativeCocoaMenu) *>(menu)->qmenu;
        previousAction = action;
        action->activate(QAction::Hover);
        qt_mac_menu_emit_hovered(qtmenu, action);
        action->showStatusText(0); // 0 widget -> action's parent
    }
}

- (void)menuWillOpen:(NSMenu*)menu
{
    while (QWidget *popup
                = QApplication::activePopupWidget())
        popup->close();
    QMenu *qtmenu = static_cast<QT_MANGLE_NAMESPACE(QNativeCocoaMenu) *>(menu)->qmenu;
    qt_mac_emit_menuSignals(qtmenu, true);
    qt_mac_menu_collapseSeparators(menu, qtmenu->separatorsCollapsible());
}

- (void)menuDidClose:(NSMenu*)menu
{
    qt_mac_emit_menuSignals(((QT_MANGLE_NAMESPACE(QNativeCocoaMenu) *)menu)->qmenu, false);
    if (previousAction) {
        qt_mac_clear_status_text(previousAction);
        previousAction = 0;
    }
}

- (BOOL)hasShortcut:(NSMenu *)menu forKey:(NSString *)key forModifiers:(NSUInteger)modifier
  whichItem:(NSMenuItem**)outItem
{
    for (NSMenuItem *item in [menu itemArray]) {
        if (![item isEnabled] || [item isHidden] || [item isSeparatorItem])
            continue;
        if ([item hasSubmenu]) {
            if ([self hasShortcut:[item submenu]
                           forKey:key
                     forModifiers:modifier whichItem:outItem]) {
                if (outItem)
                    *outItem = item;
                return YES;
            }
        }
        NSString *menuKey = [item keyEquivalent];
        if (menuKey && NSOrderedSame == [menuKey compare:key]
            && (modifier == [item keyEquivalentModifierMask])) {
            if (outItem)
                *outItem = item;
            return YES;
        }
    }
    if (outItem)
        *outItem = 0;
    return NO;
}

NSString *qt_mac_removePrivateUnicode(NSString* string)
{
    int len = [string length];
    if (len) {
        QVarLengthArray <unichar, 10> characters(len);
        bool changed = false;
        for (int i = 0; i<len; i++) {
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

- (BOOL)menuHasKeyEquivalent:(NSMenu *)menu forEvent:(NSEvent *)event target:(id *)target action:(SEL *)action
{
    // Check if the menu actually has a keysequence defined for this key event.
    // If it does, then we will first send the key sequence to the QWidget that has focus
    // since (in Qt's eyes) it needs to a chance at the key event first. If the widget
    // accepts the key event, we then return YES, but set the target and action to be nil,
    // which means that the action should not be triggered, and instead dispatch the event ourselves.
    // In every other case we return NO, which means that Cocoa can do as it pleases
    // (i.e., fire the menu action).
    NSMenuItem *whichItem;
    // Change the private unicode keys to the ones used in setting the "Key Equivalents"
    NSString *characters = qt_mac_removePrivateUnicode([event characters]);
    if ([self hasShortcut:menu
            forKey:characters
            // Interested only in Shift, Cmd, Ctrl & Alt Keys, so ignoring masks like, Caps lock, Num Lock ...
            forModifiers:([event modifierFlags] & (NSShiftKeyMask | NSControlKeyMask | NSCommandKeyMask | NSAlternateKeyMask))
            whichItem:&whichItem]) {
        QWidget *widget = 0;
        QAction *qaction = 0;
        if (whichItem && [whichItem tag]) {
            qaction = reinterpret_cast<QAction *>([whichItem tag]);
        }
        if (qApp->activePopupWidget())
            widget = (qApp->activePopupWidget()->focusWidget() ?
                      qApp->activePopupWidget()->focusWidget() : qApp->activePopupWidget());
        else if (QApplicationPrivate::focus_widget)
            widget = QApplicationPrivate::focus_widget;
        // If we could not find any receivers, pass it to the active window
        if (!widget)
            widget = qApp->activeWindow();
        if (qaction && widget) {
            int key = qaction->shortcut();
            QKeyEvent accel_ev(QEvent::ShortcutOverride, (key & (~Qt::KeyboardModifierMask)),
                               Qt::KeyboardModifiers(key & Qt::KeyboardModifierMask));
            accel_ev.ignore();

// ###           qt_sendSpontaneousEvent(widget, &accel_ev);

            if (accel_ev.isAccepted()) {
                qWarning("Unimplemented: qt_dispatchKeyEvent");
#if 0
                qt_dispatchKeyEvent(event, widget);
#endif
                *target = nil;
                *action = nil;
                return YES;
            }
        }
    }
    return NO;
}

- (NSInteger)indexOfItemWithTarget:(id)anObject andAction:(SEL)actionSelector
{
     NSInteger index = [super indexOfItemWithTarget:anObject andAction:actionSelector];
     static SEL selForOFCP = NSSelectorFromString(@"orderFrontCharacterPalette:");
     if (index == -1 && selForOFCP == actionSelector) {
         // Check if the 'orderFrontCharacterPalette' SEL exists for QNativeCocoaMenuLoader object
         QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *loader = [NSApp QT_MANGLE_NAMESPACE(qt_qcocoamenuLoader)];
         return [super indexOfItemWithTarget:loader andAction:actionSelector];
     }
     return index;
}

@end

QT_BEGIN_NAMESPACE
extern int qt_mac_menus_open_count; // qmenu_mac.mm

void qt_mac_emit_menuSignals(QMenu *menu, bool show)
{
    if (!menu)
        return;
    int delta;
    if (show) {
        emit menu->aboutToShow();
        delta = 1;
    } else {
        emit menu->aboutToHide();
        delta = -1;
    }
    qt_mac_menus_open_count += delta;
}

void qt_mac_clear_status_text(QAction *action)
{
    action->d_func()->showStatusText(0, QString());
}

void qt_mac_menu_emit_hovered(QMenu *menu, QAction *action)
{
    emit menu->hovered(action);
}


QT_END_NAMESPACE

