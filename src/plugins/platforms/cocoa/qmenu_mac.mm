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

#include "qmenu_mac.h"

#include <Cocoa/Cocoa.h>

#include "qmenu.h"
#include "qhash.h"
#include <qdebug.h>
#include "qapplication.h"
#include "qregexp.h"
#include "qtoolbar.h"
#include "qevent.h"
#include "qstyle.h"
#include "qwidgetaction.h"

#include <private/qmenu_p.h>
#include <private/qmenubar_p.h>
#include <private/qguiapplication_p.h>

#include "qcocoahelpers.h"
#include "qcocoaapplication.h"
#include "qcocoamenuloader.h"
#include "qcocoamenu.h"
#include "qcocoahelpers.h"
#include "qcocoaautoreleasepool.h"

QT_BEGIN_NAMESPACE

/*****************************************************************************
  QMenu debug facilities
 *****************************************************************************/

/*****************************************************************************
  QMenu globals
 *****************************************************************************/
bool qt_mac_no_menubar_merge = false;
bool qt_mac_quit_menu_item_enabled = true;
int qt_mac_menus_open_count = 0;

static OSMenuRef qt_mac_create_menu(QWidget *w);

static struct {
    QPointer<QMenuBar> qmenubar;
    bool modal;
} qt_mac_current_menubar = { 0, false };




/*****************************************************************************
  Externals
 *****************************************************************************/
extern OSViewRef qt_mac_hiview_for(const QWidget *w); //qwidget_mac.cpp
extern IconRef qt_mac_create_iconref(const QPixmap &px); //qpixmap_mac.cpp
extern QWidget * mac_keyboard_grabber; //qwidget_mac.cpp
extern bool qt_sendSpontaneousEvent(QObject*, QEvent*); //qapplication_xxx.cpp
RgnHandle qt_mac_get_rgn(); //qregion_mac.cpp
void qt_mac_dispose_rgn(RgnHandle r); //qregion_mac.cpp

/*****************************************************************************
  QMenu utility functions
 *****************************************************************************/
bool qt_mac_watchingAboutToShow(QMenu *menu)
{
    return menu; /* && menu->receivers(SIGNAL(aboutToShow()));*/
}

static int qt_mac_CountMenuItems(OSMenuRef menu)
{
    if (menu) {
        return [menu numberOfItems];
    }
    return 0;
}

void qt_mac_menu_collapseSeparators(NSMenu * theMenu, bool collapse)
{
    QCocoaAutoReleasePool pool;
    OSMenuRef menu = static_cast<OSMenuRef>(theMenu);
    if (collapse) {
        bool previousIsSeparator = true; // setting to true kills all the separators placed at the top.
        NSMenuItem *previousItem = nil;

        NSArray *itemArray = [menu itemArray];
        for (unsigned int i = 0; i < [itemArray count]; ++i) {
            NSMenuItem *item = reinterpret_cast<NSMenuItem *>([itemArray objectAtIndex:i]);
            if ([item isSeparatorItem]) {
                [item setHidden:previousIsSeparator];
            }

            if (![item isHidden]) {
                previousItem = item;
                previousIsSeparator = ([previousItem isSeparatorItem]);
            }
        }

        // We now need to check the final item since we don't want any separators at the end of the list.
        if (previousItem && previousIsSeparator)
            [previousItem setHidden:YES];
    } else {
        NSArray *itemArray = [menu itemArray];
        for (unsigned int i = 0; i < [itemArray count]; ++i) {
            NSMenuItem *item = reinterpret_cast<NSMenuItem *>([itemArray objectAtIndex:i]);
            if (QAction *action = reinterpret_cast<QAction *>([item tag]))
                [item setHidden:!action->isVisible()];
        }
    }
}

#ifndef QT_NO_TRANSLATION
static const char *application_menu_strings[] = {
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","Services"),
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","Hide %1"),
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","Hide Others"),
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","Show All"),
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","Preferences..."),
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","Quit %1"),
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","About %1")
    };

QString qt_mac_applicationmenu_string(int type)
{
    QString menuString = QString::fromLatin1(application_menu_strings[type]);
    QString translated = qApp->translate("QMenuBar", application_menu_strings[type]);
    if (translated != menuString)
        return translated;
    else
        return qApp->translate("MAC_APPLICATION_MENU",
                               application_menu_strings[type]);
}
#endif


static quint32 constructModifierMask(quint32 accel_key)
{
    quint32 ret = 0;
    const bool dontSwap = qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta);
    if ((accel_key & Qt::CTRL) == Qt::CTRL)
        ret |= (dontSwap ? NSControlKeyMask : NSCommandKeyMask);
    if ((accel_key & Qt::META) == Qt::META)
        ret |= (dontSwap ? NSCommandKeyMask : NSControlKeyMask);
    if ((accel_key & Qt::ALT) == Qt::ALT)
        ret |= NSAlternateKeyMask;
    if ((accel_key & Qt::SHIFT) == Qt::SHIFT)
        ret |= NSShiftKeyMask;
    return ret;
}

static void cancelAllMenuTracking()
{
    QCocoaAutoReleasePool pool;
    NSMenu *mainMenu = [NSApp mainMenu];
    [mainMenu cancelTracking];
    for (NSMenuItem *item in [mainMenu itemArray]) {
        if ([item submenu]) {
            [[item submenu] cancelTracking];
        }
    }
}

static bool actualMenuItemVisibility(const QCocoaMenuBar *mbp,
                                     const QCocoaMenuAction *action)
{
    bool visible = action->action->isVisible();
    if (visible && action->action->text() == QString(QChar(0x14)))
        return false;

    if (visible && action->action->menu() && !action->action->menu()->actions().isEmpty() &&
/* ###        !qt_mac_CountMenuItems(cocoaMenu->macMenu(mbp->apple_menu)) &&*/
        !qt_mac_watchingAboutToShow(action->action->menu())) {
        return false;
    }
    return visible;
}

static inline void syncNSMenuItemVisiblity(NSMenuItem *menuItem, bool actionVisibility)
{
    [menuItem setHidden:NO];
    [menuItem setHidden:YES];
    [menuItem setHidden:!actionVisibility];
}

static inline void syncNSMenuItemEnabled(NSMenuItem *menuItem, bool enabled)
{
    [menuItem setEnabled:NO];
    [menuItem setEnabled:YES];
    [menuItem setEnabled:enabled];
}

static inline void syncMenuBarItemsVisiblity(const QCocoaMenuBar *mac_menubar)
{
    const QList<QCocoaMenuAction *> &menubarActions = mac_menubar->actionItems;
    for (int i = 0; i < menubarActions.size(); ++i) {
        const QCocoaMenuAction *action = menubarActions.at(i);
        syncNSMenuItemVisiblity(action->menuItem, actualMenuItemVisibility(mac_menubar, action));
    }
}

static inline QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *getMenuLoader()
{
    return [NSApp QT_MANGLE_NAMESPACE(qt_qcocoamenuLoader)];
}

static NSMenuItem *createNSMenuItem(const QString &title)
{
    NSMenuItem *item = [[NSMenuItem alloc] 
                         initWithTitle:qt_mac_QStringToNSString(title)
                         action:@selector(qtDispatcherToQAction:) keyEquivalent:@""];
    [item setTarget:nil];
    return item;
}

// helper that recurses into a menu structure and en/dis-ables them
void qt_mac_set_modal_state_helper_recursive(OSMenuRef menu, OSMenuRef merge, bool on)
{
    bool modalWindowOnScreen = qApp->activeModalWidget() != 0;
    for (NSMenuItem *item in [menu itemArray]) {
        OSMenuRef submenu = [item submenu];
        if (submenu != merge) {
            if (submenu)
                qt_mac_set_modal_state_helper_recursive(submenu, merge, on);
            if (!on) {
                // The item should follow what the QAction has.
                if ([item tag]) {
                    QAction *action = reinterpret_cast<QAction *>([item tag]);
                    syncNSMenuItemEnabled(item, action->isEnabled());
                } else {
                    syncNSMenuItemEnabled(item, YES);
                }
                // We sneak in some extra code here to handle a menu problem:
                // If there is no window on screen, we cannot set 'nil' as
                // menu item target, because then cocoa will disable the item
                // (guess it assumes that there will be no first responder to
                // catch the trigger anyway?) OTOH, If we have a modal window,
                // then setting the menu loader as target will make cocoa not
                // deliver the trigger because the loader is then seen as modally
                // shaddowed). So either way there are shortcomings. Instead, we
                // decide the target as late as possible:
                [item setTarget:modalWindowOnScreen ? nil : getMenuLoader()];
            } else {
                syncNSMenuItemEnabled(item, NO);
            }
        }
    }
}

//toggling of modal state
static void qt_mac_set_modal_state(OSMenuRef menu, bool on)
{
    OSMenuRef merge = QCocoaMenu::mergeMenuHash.value(menu);
    qt_mac_set_modal_state_helper_recursive(menu, merge, on);
    // I'm ignoring the special items now, since they should get handled via a syncAction()
}

bool qt_mac_menubar_is_open()
{
    return qt_mac_menus_open_count > 0;
}

QCocoaMenuAction::~QCocoaMenuAction()
{
    [menu release];
    // Update the menu item if this action still owns it. For some items
    // (like 'Quit') ownership will be transferred between all menu bars...
    if (action && action.data() == reinterpret_cast<QAction *>([menuItem tag])) {
        QAction::MenuRole role = action->menuRole();
        // Check if the item is owned by Qt, and should be hidden to keep it from causing
        // problems. Do it for everything but the quit menu item since that should always
        // be visible.
        if (role > QAction::ApplicationSpecificRole && role < QAction::QuitRole) {
            [menuItem setHidden:YES];
        } else if (role == QAction::TextHeuristicRole
                   && menuItem != [getMenuLoader() quitMenuItem]) {
            [menuItem setHidden:YES];
        }
        [menuItem setTag:nil];
    }
    [menuItem release];
}

static NSMenuItem *qt_mac_menu_merge_action(OSMenuRef merge, QCocoaMenuAction *action)
{
    if (qt_mac_no_menubar_merge || action->action->menu() || action->action->isSeparator()
            || action->action->menuRole() == QAction::NoRole)
        return 0;

    QString t = qt_mac_removeMnemonics(action->action->text().toLower());
    int st = t.lastIndexOf(QLatin1Char('\t'));
    if (st != -1)
        t.remove(st, t.length()-st);
    t.replace(QRegExp(QString::fromLatin1("\\.*$")), QLatin1String("")); //no ellipses
    //now the fun part
    NSMenuItem *ret = 0;
    QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *loader = getMenuLoader();

    switch (action->action->menuRole()) {
    case QAction::NoRole:
        ret = 0;
        break;
    case QAction::ApplicationSpecificRole:
        ret = [loader appSpecificMenuItem];
        break;
    case QAction::AboutRole:
        ret = [loader aboutMenuItem];
        break;
    case QAction::AboutQtRole:
        ret = [loader aboutQtMenuItem];
        break;
    case QAction::QuitRole:
        ret = [loader quitMenuItem];
        break;
    case QAction::PreferencesRole:
        ret = [loader preferencesMenuItem];
        break;
    case QAction::TextHeuristicRole: {
        QString aboutString = QMenuBar::tr("About").toLower();
        if (t.startsWith(aboutString) || t.endsWith(aboutString)) {
            if (t.indexOf(QRegExp(QString::fromLatin1("qt$"), Qt::CaseInsensitive)) == -1) {
                ret = [loader aboutMenuItem];
            } else {
                ret = [loader aboutQtMenuItem];
            }
        } else if (t.startsWith(QMenuBar::tr("Config").toLower())
                   || t.startsWith(QMenuBar::tr("Preference").toLower())
                   || t.startsWith(QMenuBar::tr("Options").toLower())
                   || t.startsWith(QMenuBar::tr("Setting").toLower())
                   || t.startsWith(QMenuBar::tr("Setup").toLower())) {
            ret = [loader preferencesMenuItem];
        } else if (t.startsWith(QMenuBar::tr("Quit").toLower())
                   || t.startsWith(QMenuBar::tr("Exit").toLower())) {
            ret = [loader quitMenuItem];
        }
    }
        break;
    }

    if (QMenuMergeList *list = QCocoaMenu::mergeMenuItemsHash.value(merge)) {
        for(int i = 0; i < list->size(); ++i) {
            const QMenuMergeItem &item = list->at(i);
            if (item.menuItem == ret && item.action)
                return 0;
        }
    }

    return ret;
}

static QString qt_mac_menu_merge_text(QCocoaMenuAction *action)
{
    QString ret;
    extern QString qt_mac_applicationmenu_string(int type);
    QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *loader = getMenuLoader();
    if (action->action->menuRole() == QAction::ApplicationSpecificRole)
        ret = action->action->text();
    else if (action->menuItem == [loader aboutMenuItem]) {
        ret = qt_mac_applicationmenu_string(6).arg(qt_mac_applicationName());
    } else if (action->menuItem == [loader aboutQtMenuItem]) {
        if (action->action->text() == QString("About Qt"))
            ret = QMenuBar::tr("About Qt");
        else
            ret = action->action->text();
    } else if (action->menuItem == [loader preferencesMenuItem]) {
        ret = qt_mac_applicationmenu_string(4);
    } else if (action->menuItem == [loader quitMenuItem]) {
        ret = qt_mac_applicationmenu_string(5).arg(qt_mac_applicationName());
    }
    return ret;
}

static QKeySequence qt_mac_menu_merge_accel(QCocoaMenuAction *action)
{
    QKeySequence ret;
    QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *loader = getMenuLoader();
    if (action->action->menuRole() == QAction::ApplicationSpecificRole)
        ret = action->action->shortcut();
    else if (action->menuItem == [loader preferencesMenuItem])
        ret = QKeySequence(QKeySequence::Preferences);
    else if (action->menuItem == [loader quitMenuItem])
        ret = QKeySequence(QKeySequence::Quit);
    return ret;
}

void Q_WIDGETS_EXPORT qt_mac_set_menubar_icons(bool b)
{ QApplication::instance()->setAttribute(Qt::AA_DontShowIconsInMenus, !b); }
void Q_WIDGETS_EXPORT qt_mac_set_native_menubar(bool b)
{  QApplication::instance()->setAttribute(Qt::AA_DontUseNativeMenuBar, !b); }
void Q_WIDGETS_EXPORT qt_mac_set_menubar_merge(bool b) { qt_mac_no_menubar_merge = !b; }

/*****************************************************************************
  QMenu bindings
 *****************************************************************************/

QCocoaMenuAction::QCocoaMenuAction()
    : menuItem(0)
    , ignore_accel(0), merged(0), menu(0)
{

}

QCocoaMenu::QCocoaMenu(QMenu *a_qtMenu) : menu(0), qtMenu(a_qtMenu)
{
}

QCocoaMenu::~QCocoaMenu()
{
    QCocoaAutoReleasePool pool;
    while (actionItems.size()) {
        QCocoaMenuAction *action = static_cast<QCocoaMenuAction *>(actionItems.takeFirst());
        if (QMenuMergeList *list = mergeMenuItemsHash.value(action->menu)) {
            int i = 0;
            while (i < list->size()) {
                const QMenuMergeItem &item = list->at(i);
                if (item.action == action)
                    list->removeAt(i);
                else
                    ++i;
            }
        }
        delete action;
    }
    mergeMenuHash.remove(menu);
    mergeMenuItemsHash.remove(menu);
    [menu release];
}


void QCocoaMenu::addAction(QAction *a, QAction *before)
{
    QCocoaMenuAction *action = new QCocoaMenuAction;
    action->action = a;
    action->ignore_accel = 0;
    action->merged = 0;
    action->menu = 0;

    QCocoaMenuAction *cocoaBefore = findAction(before);
    addAction(action, cocoaBefore);
}


void QCocoaMenu::addAction(QCocoaMenuAction *action, QCocoaMenuAction *before)
{
    QCocoaAutoReleasePool pool;
    if (!action)
        return;
    int before_index = actionItems.indexOf(before);
    if (before_index < 0) {
        before = 0;
        before_index = actionItems.size();
    }
    actionItems.insert(before_index, action);

    [menu retain];
    [action->menu release];
    action->menu = menu;

    /* When the action is considered a mergable action it
       will stay that way, until removed.. */
    if (!qt_mac_no_menubar_merge) {
        OSMenuRef merge = QCocoaMenu::mergeMenuHash.value(menu);
        if (merge) {
            if (NSMenuItem *cmd = qt_mac_menu_merge_action(merge, action)) {
                action->merged = 1;
                [merge retain];
                [action->menu release];
                action->menu = merge;
                [cmd retain];
                [cmd setAction:@selector(qtDispatcherToQAction:)];
                [cmd setTarget:nil];
                [action->menuItem release];
                action->menuItem = cmd;
                QMenuMergeList *list = QCocoaMenu::mergeMenuItemsHash.value(merge);
                if (!list) {
                    list = new QMenuMergeList;
                    QCocoaMenu::mergeMenuItemsHash.insert(merge, list);
                }
                list->append(QMenuMergeItem(cmd, action));
            }
        }
    }

    NSMenuItem *newItem = action->menuItem;
    if (newItem == 0) {
        newItem = createNSMenuItem(action->action->text());
        action->menuItem = newItem;
        if (before) {
            [menu insertItem:newItem atIndex:qMax(before_index, 0)];
        } else {
            [menu addItem:newItem];
        }
    } else {
        [newItem setEnabled:YES];
        // ###
        //[newItem setEnabled:!QApplicationPrivate::modalState()];

    }
    [newItem setTag:long(static_cast<QAction *>(action->action))];
    syncAction(action);
}

void QCocoaMenu::syncAction(QAction *a)
{
    syncAction(findAction(a));
}

void QCocoaMenu::removeAction(QAction *a)
{
    removeAction(findAction(a));
}

QCocoaMenuAction *QCocoaMenu::findAction(QAction *action) const
{
    for (int i = 0; i < actionItems.size(); i++) {
        QCocoaMenuAction *act = actionItems[i];
        if (action == act->action)
            return act;
    }
    return 0;
}


// return an autoreleased string given a QKeySequence (currently only looks at the first one).
NSString *keySequenceToKeyEqivalent(const QKeySequence &accel)
{
    quint32 accel_key = (accel[0] & ~(Qt::MODIFIER_MASK | Qt::UNICODE_ACCEL));
    QChar cocoa_key = qt_mac_qtKey2CocoaKey(Qt::Key(accel_key));
    if (cocoa_key.isNull())
        cocoa_key = QChar(accel_key).toLower().unicode();
    return [NSString stringWithCharacters:&cocoa_key.unicode() length:1];
}

// return the cocoa modifier mask for the QKeySequence (currently only looks at the first one).
NSUInteger keySequenceModifierMask(const QKeySequence &accel)
{
    return constructModifierMask(accel[0]);
}

void QCocoaMenu::syncAction(QCocoaMenuAction *action)
{
    if (!action)
        return;

    NSMenuItem *item = action->menuItem;
    if (!item)
        return;

    QCocoaAutoReleasePool pool;
    NSMenu *menu = [item menu];
    bool actionVisible = action->action->isVisible();
    [item setHidden:!actionVisible];
    if (!actionVisible)
        return;

    int itemIndex = [menu indexOfItem:item];
    Q_ASSERT(itemIndex != -1);
    if (action->action->isSeparator()) {
        action->menuItem = [NSMenuItem separatorItem];
        [action->menuItem retain];
        [menu insertItem: action->menuItem atIndex:itemIndex];
        [menu removeItem:item];
        [item release];
        item = action->menuItem;
        return;
    } else if ([item isSeparatorItem]) {
        // I'm no longer a separator...
        action->menuItem = createNSMenuItem(action->action->text());
        [menu insertItem:action->menuItem atIndex:itemIndex];
        [menu removeItem:item];
        [item release];
        item = action->menuItem;
    }

    //find text (and accel)
    action->ignore_accel = 0;
    QString text = action->action->text();
    QKeySequence accel = action->action->shortcut();
    {
        int st = text.lastIndexOf(QLatin1Char('\t'));
        if (st != -1) {
            action->ignore_accel = 1;
            accel = QKeySequence(text.right(text.length()-(st+1)));
            text.remove(st, text.length()-st);
        }
    }
    {
        QString cmd_text = qt_mac_menu_merge_text(action);
        if (!cmd_text.isEmpty()) {
            text = cmd_text;
            accel = qt_mac_menu_merge_accel(action);
        }
    }
    // Show multiple key sequences as part of the menu text.
    if (accel.count() > 1)
        text += QLatin1String(" (") + accel.toString(QKeySequence::NativeText) + QLatin1String(")");

#if 0
    QString finalString = qt_mac_removeMnemonics(text);
#else
    QString finalString = qt_mac_removeMnemonics(text);
#endif
    // Cocoa Font and title
    if (action->action->font().resolve()) {
        const QFont &actionFont = action->action->font();
        NSFont *customMenuFont = [NSFont fontWithName:qt_mac_QStringToNSString(actionFont.family())
                                  size:actionFont.pointSize()];
        NSArray *keys = [NSArray arrayWithObjects:NSFontAttributeName, nil];
        NSArray *objects = [NSArray arrayWithObjects:customMenuFont, nil];
        NSDictionary *attributes = [NSDictionary dictionaryWithObjects:objects forKeys:keys];
        NSAttributedString *str = [[[NSAttributedString alloc] initWithString:qt_mac_QStringToNSString(finalString)
                                 attributes:attributes] autorelease];
       [item setAttributedTitle: str];
    } else {
            [item setTitle: qt_mac_QStringToNSString(finalString)];
    }

    if (action->action->menuRole() == QAction::AboutRole || action->action->menuRole() == QAction::QuitRole)
        [item setTitle:qt_mac_QStringToNSString(text)];
    else
        [item setTitle:qt_mac_QStringToNSString(qt_mac_removeMnemonics(text))];

    // Cocoa Enabled
    [item setEnabled: action->action->isEnabled()];

    // Cocoa icon
    NSImage *nsimage = 0;
    if (!action->action->icon().isNull() && action->action->isIconVisibleInMenu()) {
        nsimage = static_cast<NSImage *>(qt_mac_create_nsimage(action->action->icon().pixmap(16, QIcon::Normal)));
    }
    [item setImage:nsimage];
    [nsimage release];

    if (action->action->menu()) { //submenu
        QCocoaMenu *cocoaMenu = static_cast<QCocoaMenu *>(action->action->menu()->platformMenu());
        NSMenu *subMenu = cocoaMenu->macMenu();
        if ([subMenu supermenu] && [subMenu supermenu] != [item menu]) {
            // The menu is already a sub-menu of another one. Cocoa will throw an exception,
            // in such cases. For the time being, a new QMenu with same set of actions is the
            // only workaround.
            action->action->setEnabled(false);
        } else {
            [item setSubmenu:subMenu];
        }
    } else { //respect some other items
        [item setSubmenu:0];
        // No key equivalent set for multiple key QKeySequence.
        if (accel.count() == 1) {
            [item setKeyEquivalent:keySequenceToKeyEqivalent(accel)];
            [item setKeyEquivalentModifierMask:keySequenceModifierMask(accel)];
        } else {
            [item setKeyEquivalent:@""];
            [item setKeyEquivalentModifierMask:NSCommandKeyMask];
        }
    }
    //mark glyph
    [item setState:action->action->isChecked() ?  NSOnState : NSOffState];
}

void QCocoaMenu::removeAction(QCocoaMenuAction *action)
{
    if (!action)
        return;
    QCocoaAutoReleasePool pool;
    if (action->merged) {
        if (reinterpret_cast<QAction *>([action->menuItem tag]) == action->action) {
            QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *loader = getMenuLoader();
            [action->menuItem setEnabled:false];
            if (action->menuItem != [loader quitMenuItem]
                && action->menuItem != [loader preferencesMenuItem]) {
                [[action->menuItem menu] removeItem:action->menuItem];
            }
        }
    } else {
        [[action->menuItem menu] removeItem:action->menuItem];
    }
    actionItems.removeAll(action);
}

OSMenuRef QCocoaMenu::macMenu(OSMenuRef merge)
{
    if (menu)
        return menu;
    menu = qt_mac_create_menu(qtMenu);
    if (merge) {
        mergeMenuHash.insert(menu, merge);
    }
    QList<QAction*> items = qtMenu->actions();
    for(int i = 0; i < items.count(); i++)
        addAction(items[i], 0);
    syncSeparatorsCollapsible(qtMenu->separatorsCollapsible());
    return menu;
}

/*!
  \internal
*/
void
QCocoaMenu::syncSeparatorsCollapsible(bool collapse)
{
    qt_mac_menu_collapseSeparators(menu, collapse);
}

/*!
  \internal
*/
void QCocoaMenu::setMenuEnabled(bool enable)
{
    QCocoaAutoReleasePool pool;
    if (enable) {
        for (int i = 0; i < actionItems.count(); ++i) {
            QCocoaMenuAction *menuItem = static_cast<QCocoaMenuAction *>(actionItems.at(i));
            if (menuItem && menuItem->action && menuItem->action->isEnabled()) {
                [menuItem->menuItem setEnabled:true];
            }
        }
    } else {
        NSMenu *menu = menu;
        for (NSMenuItem *item in [menu itemArray]) {
            [item setEnabled:false];
        }
    }
}

/*!
    \internal

    This function will return the OSMenuRef used to create the native menu bar
    bindings.

    If Qt is built against Carbon, the OSMenuRef is a MenuRef that can be used
    with Carbon's Menu Manager API.

    If Qt is built against Cocoa, the OSMenuRef is a NSMenu pointer.

    \warning This function is not portable.

    \sa QMenuBar::macMenu()
*/
/// OSMenuRef QMenu::macMenu(OSMenuRef merge) { return d_func()->macMenu(merge); }

/*****************************************************************************
  QMenuBar bindings
 *****************************************************************************/
typedef QHash<QWidget *, QMenuBar *> MenuBarHash;
Q_GLOBAL_STATIC(MenuBarHash, menubars)
static QMenuBar *fallback = 0;

QCocoaMenuBar::QCocoaMenuBar(QMenuBar *a_qtMenuBar) : menu(0), apple_menu(0), qtMenuBar(a_qtMenuBar)
{
    macCreateMenuBar(qtMenuBar->parentWidget());
}

QCocoaMenuBar::~QCocoaMenuBar()
{
    for(QList<QCocoaMenuAction*>::Iterator it = actionItems.begin(); it != actionItems.end(); ++it)
        delete (*it);
    [apple_menu release];
    [menu release];
}
void QCocoaMenuBar::handleReparent(QWidget *newParent)
{
    if (macWidgetHasNativeMenubar(newParent)) {
        // If the new parent got a native menubar from before, keep that
        // menubar rather than replace it with this one (because a parents
        // menubar has precedence over children menubars).
        macDestroyMenuBar();
        macCreateMenuBar(newParent);
     }

}

void QCocoaMenuBar::addAction(QAction *action, QAction *beforeAction)
{
    if (action->isSeparator() || !menu)
        return;
    QCocoaMenuAction *cocoaAction = new QCocoaMenuAction;
    cocoaAction->action = action;
    cocoaAction->ignore_accel = 1;
    QCocoaMenuAction *cocoaBeforeAction = findAction(beforeAction);
    addAction(cocoaAction, cocoaBeforeAction);
}

void QCocoaMenuBar::addAction(QCocoaMenuAction *action, QCocoaMenuAction *before)
{
    if (!action || !menu)
        return;

    int before_index = actionItems.indexOf(before);
    if (before_index < 0) {
        before = 0;
        before_index = actionItems.size();
    }
    actionItems.insert(before_index, action);

    MenuItemIndex index = actionItems.size()-1;

    action->menu = menu;
    QCocoaAutoReleasePool pool;
    [action->menu retain];
    NSMenuItem *newItem = createNSMenuItem(action->action->text());
    action->menuItem = newItem;

    if (before) {
        [menu insertItem:newItem atIndex:qMax(1, before_index + 1)];
        index = before_index;
    } else {
        [menu addItem:newItem];
    }
    [newItem setTag:long(static_cast<QAction *>(action->action))];
    syncAction(action);
}


void QCocoaMenuBar::syncAction(QCocoaMenuAction *action)
{
    if (!action || !menu)
        return;

    QCocoaAutoReleasePool pool;
    NSMenuItem *item = action->menuItem;

    OSMenuRef submenu = 0;
    bool release_submenu = false;
    if (action->action->menu()) {
        QCocoaMenu *cocoaMenu = static_cast<QCocoaMenu *>(action->action->menu()->platformMenu());
        if (!cocoaMenu) {

        }

        if ((submenu = cocoaMenu->macMenu(apple_menu))) {
            if ([submenu supermenu] && [submenu supermenu] != [item menu])
                return;
            else
                [item setSubmenu:submenu];
        }
    }

    if (submenu) {
        bool visible = actualMenuItemVisibility(this, action);
        [item setSubmenu: submenu];
        [submenu setTitle:qt_mac_QStringToNSString(qt_mac_removeMnemonics(action->action->text()))];
        syncNSMenuItemVisiblity(item, visible);
        if (release_submenu) { //no pointers to it
            [submenu release];
        }
    } else {
        qWarning("QMenu: No OSMenuRef created for popup menu");
    }
}


void QCocoaMenuBar::removeAction(QCocoaMenuAction *action)
{
    if (!action || !menu)
        return;
    QCocoaAutoReleasePool pool;
    [action->menu removeItem:action->menuItem];
    actionItems.removeAll(action);
}

void QCocoaMenuBar::syncAction(QAction *a)
{
    syncAction(findAction(a));
}

void QCocoaMenuBar::removeAction(QAction *a)
{
    removeAction(findAction(a));
}

QCocoaMenuAction *QCocoaMenuBar::findAction(QAction *action) const
{
    for (int i = 0; i < actionItems.size(); i++) {
        QCocoaMenuAction *act = actionItems[i];
        if (action == act->action)
            return act;
    }
    return 0;
}

bool QCocoaMenuBar::macWidgetHasNativeMenubar(QWidget *widget)
{
    // This function is different from q->isNativeMenuBar(), as
    // it returns true only if a native menu bar is actually
    // _created_.
    if (!widget)
        return false;
    return menubars()->contains(widget->window());
}

void QCocoaMenuBar::macCreateMenuBar(QWidget *parent)
{
    static int dontUseNativeMenuBar = -1;
    // We call the isNativeMenuBar function here
    // because that will make sure that local overrides
    // are dealt with correctly. q->isNativeMenuBar() will, if not
    // overridden, depend on the attribute Qt::AA_DontUseNativeMenuBar:
    bool qt_mac_no_native_menubar = !qtMenuBar->isNativeMenuBar();
    if (qt_mac_no_native_menubar == false && dontUseNativeMenuBar < 0) {
        // The menubar is set to be native. Let's check (one time only
        // for all menubars) if this is OK with the rest of the environment.
        // As a result, Qt::AA_DontUseNativeMenuBar is set. NB: the application
        // might still choose to not respect, or change, this flag.
        bool isPlugin = QApplication::testAttribute(Qt::AA_MacPluginApplication);
        bool environmentSaysNo = !qgetenv("QT_MAC_NO_NATIVE_MENUBAR").isEmpty();
        dontUseNativeMenuBar = isPlugin || environmentSaysNo;
        QApplication::instance()->setAttribute(Qt::AA_DontUseNativeMenuBar, dontUseNativeMenuBar);
        qt_mac_no_native_menubar = !qtMenuBar->isNativeMenuBar();
    }
    if (qt_mac_no_native_menubar == false) {
        // INVARIANT: Use native menubar.
        macUpdateMenuBar();
        if (!parent && !fallback) {
            fallback = qtMenuBar;
        } else if (parent && parent->isWindow()) {
            menubars()->insert(qtMenuBar->window(), qtMenuBar);
        }
    }
}

void QCocoaMenuBar::macDestroyMenuBar()
{
    QCocoaAutoReleasePool pool;
    if (fallback == qtMenuBar)
        fallback = 0;
    QWidget *tlw = qtMenuBar->window();
    menubars()->remove(tlw);

    if (!qt_mac_current_menubar.qmenubar || qt_mac_current_menubar.qmenubar == qtMenuBar) {
        QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *loader = getMenuLoader();
        [loader removeActionsFromAppMenu];
        QCocoaMenuBar::macUpdateMenuBar();
    }
}

OSMenuRef QCocoaMenuBar::macMenu()
{
    if (!qtMenuBar->isNativeMenuBar()) {
        return 0;
    } else if (!menu) {
        menu = qt_mac_create_menu(qtMenuBar);
        ProcessSerialNumber mine, front;
        if (GetCurrentProcess(&mine) == noErr && GetFrontProcess(&front) == noErr) {
            if (!qt_mac_no_menubar_merge && !apple_menu) {
                apple_menu = qt_mac_create_menu(qtMenuBar);
                [apple_menu setTitle:qt_mac_QStringToNSString(QString(QChar(0x14)))];
                NSMenuItem *apple_menuItem = [[NSMenuItem alloc] init];
                [apple_menuItem setSubmenu:menu];
                [apple_menu addItem:apple_menuItem];
                [apple_menuItem release];
            }
            if (apple_menu) {
                QCocoaMenu::mergeMenuHash.insert(menu, apple_menu);
            }
            QList<QAction*> items = qtMenuBar->actions();
            for(int i = 0; i < items.count(); i++)
                addAction(items[i], 0);
        }
    }
    return menu;
}

/*!
    \internal

    This function will return the OSMenuRef used to create the native menu bar
    bindings. This OSMenuRef is then set as the root menu for the Menu
    Manager.

    \warning This function is not portable.

    \sa QMenu::macMenu()
*/
//OSMenuRef QMenuBar::macMenu() { return d_func()->macMenu(); }

/* !
    \internal
    Ancestor function that crosses windows (QWidget::isAncestorOf
    only considers widgets within the same window).
*/
static bool qt_mac_is_ancestor(QWidget* possibleAncestor, QWidget *child)
{
    if (!possibleAncestor)
        return false;

    QWidget * current = child->parentWidget();
    while (current != 0) {
        if (current == possibleAncestor)
            return true;
        current = current->parentWidget();
    }
    return false;
}

/* !
    \internal
    Returns true if the entries of menuBar should be disabled,
    based on the modality type of modalWidget.
*/
static bool qt_mac_should_disable_menu(QMenuBar *menuBar)
{
    QWidget *modalWidget = qApp->activeModalWidget();
    if (!modalWidget)
        return false;

    if (menuBar && menuBar == menubars()->value(modalWidget))
        // The menu bar is owned by the modal widget.
        // In that case we should enable it:
        return false;

    // When there is an application modal window on screen, the entries of
    // the menubar should be disabled. The exception in Qt is that if the
    // modal window is the only window on screen, then we enable the menu bar.
    QWidget *w = modalWidget;
    QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
    while (w) {
        if (w->isVisible() && w->windowModality() == Qt::ApplicationModal) {
            for (int i=0; i<topLevelWidgets.size(); ++i) {
                QWidget *top = topLevelWidgets.at(i);
                if (w != top && top->isVisible()) {
                    // INVARIANT: we found another visible window
                    // on screen other than our modalWidget. We therefore
                    // disable the menu bar to follow normal modality logic:
                    return true;
                }
            }
            // INVARIANT: We have only one window on screen that happends
            // to be application modal. We choose to enable the menu bar
            // in that case to e.g. enable the quit menu item.
            return false;
        }
        w = w->parentWidget();
    }

    // INVARIANT: modalWidget is window modal. Disable menu entries
    // if the menu bar belongs to an ancestor of modalWidget. If menuBar
    // is nil, we understand it as the default menu bar set by the nib:
    return menuBar ? qt_mac_is_ancestor(menuBar->parentWidget(), modalWidget) : false;
}

static QWidget *findWindowThatShouldDisplayMenubar()
{
    QWidget *w = qApp->activeWindow();

    if (!w) {
        // We have no active window on screen. Try to
        // find a window from the list of top levels:
        QWidgetList tlws = QApplication::topLevelWidgets();
        for(int i = 0; i < tlws.size(); ++i) {
            QWidget *tlw = tlws.at(i);
            if ((tlw->isVisible() && tlw->windowType() != Qt::Tool &&
                tlw->windowType() != Qt::Popup)) {
                w = tlw;
                break;
            }
        }
    }

    return w;
}

static QMenuBar *findMenubarForWindow(QWidget *w)
{
    QMenuBar *mb = 0;
    if (w) {
        mb = menubars()->value(w);

#if 0
// ###
//#ifndef QT_NO_MAINWINDOW
        QDockWidget *dw = qobject_cast<QDockWidget *>(w);
        if (!mb && dw) {
            QMainWindow *mw = qobject_cast<QMainWindow *>(dw->parentWidget());
            if (mw && (mb = menubars()->value(mw)))
                w = mw;
        }
#endif
        while(w && !mb)
            mb = menubars()->value((w = w->parentWidget()));
    }

    if (!mb) {
        // We could not find a menu bar for the window. Lets
        // check if we have a global (parentless) menu bar instead:
        mb = fallback;
    }

    return mb;
}

void qt_mac_clear_menubar()
{
    if (QApplication::testAttribute(Qt::AA_MacPluginApplication))
        return;

    QCocoaAutoReleasePool pool;
    QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *loader = getMenuLoader();
    NSMenu *menu = [loader menu];
    [loader ensureAppMenuInMenu:menu];
    [NSApp setMainMenu:menu];
    const bool modal = qt_mac_should_disable_menu(0);
    if (qt_mac_current_menubar.qmenubar || modal != qt_mac_current_menubar.modal)
        qt_mac_set_modal_state(menu, modal);
    qt_mac_current_menubar.qmenubar = 0;
    qt_mac_current_menubar.modal = modal;
}

/*!
  \internal

  This function will update the current menu bar and set it as the
  active menu bar in the Menu Manager.

  \warning This function is not portable.
*/
void QCocoaMenuBar::macUpdateMenuBar()
{
    [getMenuLoader() performSelectorOnMainThread: @selector(qtUpdateMenubar) withObject: nil waitUntilDone: NO];
}

bool QCocoaMenuBar::macUpdateMenuBarImmediatly()
{
    bool ret = false;
    cancelAllMenuTracking();
    QWidget *w = findWindowThatShouldDisplayMenubar();
    QMenuBar *mb = findMenubarForWindow(w);

    // ###  extern bool qt_mac_app_fullscreen; //qapplication_mac.mm
    bool qt_mac_app_fullscreen = false;
    // We need to see if we are in full screen mode, if so we need to
    // switch the full screen mode to be able to show or hide the menubar.
    if(w && mb) {
        // This case means we are creating a menubar, check if full screen
        if(w->isFullScreen()) {
            // Ok, switch to showing the menubar when hovering over it.
            SetSystemUIMode(kUIModeAllHidden, kUIOptionAutoShowMenuBar);
            qt_mac_app_fullscreen = true;
        }
    } else if(w) {
        // Removing a menubar
        if(w->isFullScreen()) {
            // Ok, switch to not showing the menubar when hovering on it
            SetSystemUIMode(kUIModeAllHidden, 0);
            qt_mac_app_fullscreen = true;
        }
    }

    if (mb && mb->isNativeMenuBar()) {

        // ###
        bool modal = false;
        //bool modal = QGuiApplicationPrivate::modalState();
        QCocoaAutoReleasePool pool;
        if (OSMenuRef menu = reinterpret_cast<QCocoaMenuBar *>(mb->platformMenuBar())->macMenu()) {
            QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *loader = getMenuLoader();
            [loader ensureAppMenuInMenu:menu];
            [NSApp setMainMenu:menu];
            syncMenuBarItemsVisiblity(reinterpret_cast<QCocoaMenuBar *>(mb->platformMenuBar()));

            if (OSMenuRef tmpMerge = QCocoaMenu::mergeMenuHash.value(menu)) {
                if (QMenuMergeList *mergeList
                        = QCocoaMenu::mergeMenuItemsHash.value(tmpMerge)) {
                    const int mergeListSize = mergeList->size();

                    for (int i = 0; i < mergeListSize; ++i) {
                        const QMenuMergeItem &mergeItem = mergeList->at(i);
                        // Ideally we would call QCocoaMenu::syncAction, but that requires finding
                        // the original QMen and likely doing more work than we need.
                        // For example, enabled is handled below.
                        [mergeItem.menuItem setTag:reinterpret_cast<long>(
                                                    static_cast<QAction *>(mergeItem.action->action))];
                        [mergeItem.menuItem setHidden:!(mergeItem.action->action->isVisible())];
                    }
                }
            }
            // Check if menu is modally shaddowed and should  be disabled:
            modal = qt_mac_should_disable_menu(mb);
            if (mb != qt_mac_current_menubar.qmenubar || modal != qt_mac_current_menubar.modal)
                qt_mac_set_modal_state(menu, modal);
        }
        qt_mac_current_menubar.qmenubar = mb;
        qt_mac_current_menubar.modal = modal;
        ret = true;
    } else if (qt_mac_current_menubar.qmenubar && qt_mac_current_menubar.qmenubar->isNativeMenuBar()) {
        // INVARIANT: The currently active menu bar (if any) is not native. But we do have a
        // native menu bar from before. So we need to decide whether or not is should be enabled:
        const bool modal = qt_mac_should_disable_menu(qt_mac_current_menubar.qmenubar);
        if (modal != qt_mac_current_menubar.modal) {
            ret = true;
            if (OSMenuRef menu = reinterpret_cast<QCocoaMenuBar *>(qt_mac_current_menubar.qmenubar->platformMenuBar())->macMenu()) {
                QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *loader = getMenuLoader();
                [loader ensureAppMenuInMenu:menu];
                [NSApp setMainMenu:menu];
                syncMenuBarItemsVisiblity(reinterpret_cast<QCocoaMenuBar *>(qt_mac_current_menubar.qmenubar->platformMenuBar()));
                qt_mac_set_modal_state(menu, modal);
            }
            qt_mac_current_menubar.modal = modal;
        }
    }

    if (!ret) {
        qt_mac_clear_menubar();
    }
    return ret;
}

QHash<OSMenuRef, OSMenuRef> QCocoaMenu::mergeMenuHash;
QHash<OSMenuRef, QMenuMergeList*> QCocoaMenu::mergeMenuItemsHash;

bool QCocoaMenu::merged(const QAction *action) const
{
    if (OSMenuRef merge = mergeMenuHash.value(menu)) {
        if (QMenuMergeList *list = mergeMenuItemsHash.value(merge)) {
            for(int i = 0; i < list->size(); ++i) {
                const QMenuMergeItem &item = list->at(i);
                if (item.action->action == action)
                    return true;
            }
        }
    }
    return false;
}

//creation of the OSMenuRef
static OSMenuRef qt_mac_create_menu(QWidget *w)
{
    OSMenuRef ret;
    if (QMenu *qmenu = qobject_cast<QMenu *>(w)){
        ret = [[QT_MANGLE_NAMESPACE(QNativeCocoaMenu) alloc] initWithQMenu:qmenu];
    } else {
        ret = [[NSMenu alloc] init];
    }
    return ret;
}

QT_END_NAMESPACE
