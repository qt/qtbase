// Copyright (C) 2018 The Qt Company Ltd.
// Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author James Turner <james.turner@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <AppKit/AppKit.h>

#include "qcocoamenubar.h"
#include "qcocoawindow.h"
#include "qcocoamenuloader.h"
#include "qcocoaapplication.h" // for custom application category
#include "qcocoaapplicationdelegate.h"

#include <QtGui/QGuiApplication>
#include <QtCore/QDebug>

#include <QtCore/private/qcore_mac_p.h>
#include <QtGui/private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

static QList<QCocoaMenuBar*> static_menubars;

QCocoaMenuBar::QCocoaMenuBar()
{
    static_menubars.append(this);

    // clicks into the menu bar should close all popup windows
    static QMacNotificationObserver menuBarClickObserver(nil, NSMenuDidBeginTrackingNotification, ^{
        QGuiApplicationPrivate::instance()->closeAllPopups();
    });

    m_nativeMenu = [[NSMenu alloc] init];
#ifdef QT_COCOA_ENABLE_MENU_DEBUG
    qDebug() << "Construct QCocoaMenuBar" << this << m_nativeMenu;
#endif
}

QCocoaMenuBar::~QCocoaMenuBar()
{
#ifdef QT_COCOA_ENABLE_MENU_DEBUG
    qDebug() << "~QCocoaMenuBar" << this;
#endif
    for (auto menu : std::as_const(m_menus)) {
        if (!menu)
            continue;
        NSMenuItem *item = nativeItemForMenu(menu);
        if (menu->attachedItem() == item)
            menu->setAttachedItem(nil);
    }

    [m_nativeMenu release];
    static_menubars.removeOne(this);

    if (!m_window.isNull() && m_window->menubar() == this) {
        m_window->setMenubar(nullptr);

        // Delete the children first so they do not cause
        // the native menu items to be hidden after
        // the menu bar was updated
        qDeleteAll(children());
        updateMenuBarImmediately();
    }
}

bool QCocoaMenuBar::needsImmediateUpdate()
{
    if (!m_window.isNull()) {
        if (m_window->window()->isActive())
            return true;
    } else {
        // Only update if the focus/active window has no
        // menubar, which means it'll be using this menubar.
        // This is to avoid a modification in a parentless
        // menubar to affect a window-assigned menubar.
        QWindow *fw = QGuiApplication::focusWindow();
        if (!fw) {
            // Same if there's no focus window, BTW.
            return true;
        } else {
            QCocoaWindow *cw = static_cast<QCocoaWindow *>(fw->handle());
            if (cw && !cw->menubar())
                return true;
        }
    }

    // Either the menubar is attached to a non-active window,
    // or the application's focus window has its own menubar
    // (which is different from this one)
    return false;
}

void QCocoaMenuBar::insertMenu(QPlatformMenu *platformMenu, QPlatformMenu *before)
{
    QCocoaMenu *menu = static_cast<QCocoaMenu *>(platformMenu);
    QCocoaMenu *beforeMenu = static_cast<QCocoaMenu *>(before);
#ifdef QT_COCOA_ENABLE_MENU_DEBUG
    qDebug() << "QCocoaMenuBar" << this << "insertMenu" << menu << "before" << before;
#endif

    if (m_menus.contains(QPointer<QCocoaMenu>(menu))) {
        qWarning("This menu already belongs to the menubar, remove it first");
        return;
    }

    if (beforeMenu && !m_menus.contains(QPointer<QCocoaMenu>(beforeMenu))) {
        qWarning("The before menu does not belong to the menubar");
        return;
    }

    int insertionIndex = beforeMenu ? m_menus.indexOf(beforeMenu) : m_menus.size();
    m_menus.insert(insertionIndex, menu);

    {
        QMacAutoReleasePool pool;
        NSMenuItem *item = [[[NSMenuItem alloc] init] autorelease];
        item.tag = reinterpret_cast<NSInteger>(menu);

        if (beforeMenu) {
            // QMenuBar::toNSMenu() exposes the native menubar and
            // the user could have inserted its own items in there.
            // Same remark applies to removeMenu().
            NSMenuItem *beforeItem = nativeItemForMenu(beforeMenu);
            NSInteger nativeIndex = [m_nativeMenu indexOfItem:beforeItem];
            [m_nativeMenu insertItem:item atIndex:nativeIndex];
        } else {
            [m_nativeMenu addItem:item];
        }
    }

    syncMenu_helper(menu, false /*internaCall*/);

    if (needsImmediateUpdate())
        updateMenuBarImmediately();
}

void QCocoaMenuBar::removeMenu(QPlatformMenu *platformMenu)
{
    QCocoaMenu *menu = static_cast<QCocoaMenu *>(platformMenu);
    if (!m_menus.contains(menu)) {
        qWarning("Trying to remove a menu that does not belong to the menubar");
        return;
    }

    NSMenuItem *item = nativeItemForMenu(menu);
    if (menu->attachedItem() == item)
        menu->setAttachedItem(nil);
    m_menus.removeOne(menu);

    QMacAutoReleasePool pool;

    // See remark in insertMenu().
    NSInteger nativeIndex = [m_nativeMenu indexOfItem:item];
    [m_nativeMenu removeItemAtIndex:nativeIndex];
}

void QCocoaMenuBar::syncMenu(QPlatformMenu *menu)
{
    syncMenu_helper(menu, false /*internaCall*/);
}

void QCocoaMenuBar::syncMenu_helper(QPlatformMenu *menu, bool menubarUpdate)
{
    QMacAutoReleasePool pool;

    QCocoaMenu *cocoaMenu = static_cast<QCocoaMenu *>(menu);
    for (QCocoaMenuItem *item : cocoaMenu->items())
        cocoaMenu->syncMenuItem_helper(item, menubarUpdate);

    BOOL shouldHide = YES;
    if (cocoaMenu->isVisible()) {
        // If the NSMenu has no visible items, or only separators, we should hide it
        // on the menubar. This can happen after syncing the menu items since they
        // can be moved to other menus.
        for (NSMenuItem *item in cocoaMenu->nsMenu().itemArray)
            if (!item.separatorItem && !item.hidden) {
                shouldHide = NO;
                break;
            }
    }

    if (NSMenuItem *attachedItem = cocoaMenu->attachedItem()) {
        // Non-nil attached item means the item's submenu is set
        attachedItem.title = cocoaMenu->nsMenu().title;
        attachedItem.hidden = shouldHide;
    }
}

NSMenuItem *QCocoaMenuBar::nativeItemForMenu(QCocoaMenu *menu) const
{
    if (!menu)
        return nil;

    return [m_nativeMenu itemWithTag:reinterpret_cast<NSInteger>(menu)];
}

void QCocoaMenuBar::handleReparent(QWindow *newParentWindow)
{
#ifdef QT_COCOA_ENABLE_MENU_DEBUG
    qDebug() << "QCocoaMenuBar" << this << "handleReparent" << newParentWindow;
#endif

    if (!m_window.isNull())
        m_window->setMenubar(nullptr);

    if (!newParentWindow) {
        m_window.clear();
    } else {
        newParentWindow->create();
        m_window = static_cast<QCocoaWindow*>(newParentWindow->handle());
        m_window->setMenubar(this);
    }

    updateMenuBarImmediately();
}

QWindow *QCocoaMenuBar::parentWindow() const
{
    return m_window ? m_window->window() : nullptr;
}


QCocoaWindow *QCocoaMenuBar::findWindowForMenubar()
{
    if (qApp->focusWindow())
        return static_cast<QCocoaWindow*>(qApp->focusWindow()->handle());

    return nullptr;
}

QCocoaMenuBar *QCocoaMenuBar::findGlobalMenubar()
{
    for (auto *menubar : std::as_const(static_menubars)) {
        if (menubar->m_window.isNull())
            return menubar;
    }

    return nullptr;
}

void QCocoaMenuBar::updateMenuBarImmediately()
{
    QMacAutoReleasePool pool;
    QCocoaMenuBar *mb = findGlobalMenubar();
    QCocoaWindow *cw = findWindowForMenubar();

    QWindow *win = cw ? cw->window() : nullptr;
    if (win && (win->flags() & Qt::Popup) == Qt::Popup) {
        // context menus, comboboxes, etc. don't need to update the menubar,
        // but if an application has only Qt::Tool window(s) on start,
        // we still have to update the menubar.
        if ((win->flags() & Qt::WindowType_Mask) != Qt::Tool)
            return;
        NSApplication *app = [NSApplication sharedApplication];
        if (![app.delegate isKindOfClass:[QCocoaApplicationDelegate class]])
            return;
        // We apply this logic _only_ during the startup.
        QCocoaApplicationDelegate *appDelegate = app.delegate;
        if (!appDelegate.inLaunch)
            return;
    }

    if (cw && cw->menubar())
        mb = cw->menubar();

    if (!mb)
        return;

#ifdef QT_COCOA_ENABLE_MENU_DEBUG
    qDebug() << "QCocoaMenuBar" << "updateMenuBarImmediately" << cw;
#endif
    bool disableForModal = mb->shouldDisable(cw);

    for (auto menu : std::as_const(mb->m_menus)) {
        if (!menu)
            continue;
        NSMenuItem *item = mb->nativeItemForMenu(menu);
        menu->setAttachedItem(item);
        menu->setMenuParent(mb);
        // force a sync?
        mb->syncMenu_helper(menu, true /*menubarUpdate*/);
        menu->propagateEnabledState(!disableForModal);
    }

    QCocoaMenuLoader *loader = [QCocoaMenuLoader sharedMenuLoader];
    [loader ensureAppMenuInMenu:mb->nsMenu()];

    NSMutableSet *mergedItems = [[NSMutableSet setWithCapacity:mb->merged().count()] retain];
    for (auto mergedItem : mb->merged()) {
        [mergedItems addObject:mergedItem->nsItem()];
        mergedItem->syncMerged();
    }

    // hide+disable all mergeable items we're not currently using
    for (NSMenuItem *mergeable in [loader mergeable]) {
        if (![mergedItems containsObject:mergeable]) {
            mergeable.hidden = YES;
            mergeable.enabled = NO;
        }
    }

    [mergedItems release];
    [NSApp setMainMenu:mb->nsMenu()];
    insertWindowMenu();
    [loader qtTranslateApplicationMenu];

    for (auto menu : std::as_const(mb->m_menus)) {
        if (!menu)
            continue;

        const QString captionNoAmpersand = QString::fromNSString(menu->nsMenu().title).remove(u'&');
        if (captionNoAmpersand != QCoreApplication::translate("QCocoaMenu", "Edit"))
            continue;

        NSMenuItem *item = mb->nativeItemForMenu(menu);
        auto *nsMenu = item.submenu;
        if ([nsMenu indexOfItemWithTarget:NSApp andAction:@selector(startDictation:)] == -1) {
            // AppKit was not able to recognize the special role of this menu item.
            mb->insertDefaultEditItems(menu);
        }
    }
}

void QCocoaMenuBar::insertWindowMenu()
{
    // For such an item/menu we get for 'free' an additional feature -
    // a list of windows the application has created in the Dock's menu.

    NSApplication *app = NSApplication.sharedApplication;
    if (app.windowsMenu)
        return;

    NSMenu *mainMenu = app.mainMenu;
    NSMenuItem *winMenuItem = [[[NSMenuItem alloc] initWithTitle:@"QtWindowMenu"
                                                   action:nil keyEquivalent:@""] autorelease];
    // We don't want to show this menu, nobody asked us to do so:
    winMenuItem.hidden = YES;

    winMenuItem.submenu = [[[NSMenu alloc] initWithTitle:@"QtWindowMenu"] autorelease];
    [mainMenu insertItem:winMenuItem atIndex:mainMenu.itemArray.count];
    app.windowsMenu = winMenuItem.submenu;

    // Windows that have already been ordered in at this point have already been
    // evaluated by AppKit via _addToWindowsMenuIfNecessary and added to the menu,
    // but since the menu didn't exist at that point the addition was a noop.
    // Instead of trying to duplicate the logic AppKit uses for deciding if
    // a window should be part of the Window menu we toggle one of the settings
    // that definitely will affect this, which results in AppKit reevaluating the
    // situation and adding the window to the menu if necessary.
    for (NSWindow *win in app.windows) {
        win.excludedFromWindowsMenu = !win.excludedFromWindowsMenu;
        win.excludedFromWindowsMenu = !win.excludedFromWindowsMenu;
    }
}

QList<QCocoaMenuItem*> QCocoaMenuBar::merged() const
{
    QList<QCocoaMenuItem*> r;
    for (auto menu : std::as_const(m_menus)) {
        if (!menu)
            continue;
        r.append(menu->merged());
    }

    return r;
}

bool QCocoaMenuBar::shouldDisable(QCocoaWindow *active) const
{
    if (active && (active->window()->modality() == Qt::NonModal))
        return false;

    if (m_window == active) {
        // modal window owns us, we should be enabled!
        return false;
    }

    QWindowList topWindows(qApp->topLevelWindows());
    // When there is an application modal window on screen, the entries of
    // the menubar should be disabled. The exception in Qt is that if the
    // modal window is the only window on screen, then we enable the menu bar.
    for (auto *window : std::as_const(topWindows)) {
        if (window->isVisible() && window->modality() == Qt::ApplicationModal) {
            // check for other visible windows
            for (auto *other : std::as_const(topWindows)) {
                if ((window != other) && (other->isVisible())) {
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
    }

    return true;
}

QPlatformMenu *QCocoaMenuBar::menuForTag(quintptr tag) const
{
    for (auto menu : std::as_const(m_menus))
        if (menu && menu->tag() == tag)
            return menu;

    return nullptr;
}

NSMenuItem *QCocoaMenuBar::itemForRole(QPlatformMenuItem::MenuRole role)
{
    for (auto menu : std::as_const(m_menus)) {
        if (menu) {
            for (auto *item : menu->items())
                if (item->effectiveRole() == role)
                    return item->nsItem();
        }
    }

    return nil;
}

QCocoaWindow *QCocoaMenuBar::cocoaWindow() const
{
    return m_window.data();
}

void QCocoaMenuBar::insertDefaultEditItems(QCocoaMenu *menu)
{
    if (menu->items().isEmpty())
        return;

    NSMenu *nsEditMenu = menu->nsMenu();
    if ([nsEditMenu itemAtIndex:nsEditMenu.numberOfItems - 1].action
        == @selector(orderFrontCharacterPalette:)) {
        for (auto defaultEditMenuItem : std::as_const(m_defaultEditMenuItems)) {
            if (menu->items().contains(defaultEditMenuItem))
                menu->removeMenuItem(defaultEditMenuItem);
        }
        qDeleteAll(m_defaultEditMenuItems);
        m_defaultEditMenuItems.clear();
    } else {
        if (m_defaultEditMenuItems.isEmpty()) {
            QCocoaMenuItem *separator = new QCocoaMenuItem;
            separator->setIsSeparator(true);

            QCocoaMenuItem *dictationItem = new QCocoaMenuItem;
            dictationItem->setText(QCoreApplication::translate("QCocoaMenuItem", "Start Dictation..."));
            QObject::connect(dictationItem, &QPlatformMenuItem::activated, this, []{
                [NSApplication.sharedApplication performSelector:@selector(startDictation:)];
            });

            QCocoaMenuItem *emojiItem = new QCocoaMenuItem;
            emojiItem->setText(QCoreApplication::translate("QCocoaMenuItem", "Emoji && Symbols"));
            emojiItem->setShortcut(QKeyCombination(Qt::MetaModifier|Qt::ControlModifier, Qt::Key_Space));
            QObject::connect(emojiItem, &QPlatformMenuItem::activated, this, []{
                [NSApplication.sharedApplication orderFrontCharacterPalette:nil];
            });

            m_defaultEditMenuItems << separator << dictationItem << emojiItem;
        }
        for (auto defaultEditMenuItem : std::as_const(m_defaultEditMenuItems)) {
            if (menu->items().contains(defaultEditMenuItem))
                menu->removeMenuItem(defaultEditMenuItem);
            menu->insertMenuItem(defaultEditMenuItem, nullptr);
        }
    }
}

QT_END_NAMESPACE

#include "moc_qcocoamenubar.cpp"
