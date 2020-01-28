/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author James Turner <james.turner@kdab.com>
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

#include "qcocoamenu.h"
#include "qcocoansmenu.h"

#include "qcocoahelpers.h"

#include <QtCore/QtDebug>
#include "qcocoaapplication.h"
#include "qcocoaintegration.h"
#include "qcocoamenuloader.h"
#include "qcocoamenubar.h"
#include "qcocoawindow.h"
#include "qcocoascreen.h"

QT_BEGIN_NAMESPACE

QCocoaMenu::QCocoaMenu() :
    m_attachedItem(nil),
    m_updateTimer(0),
    m_enabled(true),
    m_parentEnabled(true),
    m_visible(true),
    m_isOpen(false)
{
    QMacAutoReleasePool pool;

    m_nativeMenu = [[QCocoaNSMenu alloc] initWithPlatformMenu:this];
}

QCocoaMenu::~QCocoaMenu()
{
    for (auto *item : qAsConst(m_menuItems)) {
        if (item->menuParent() == this)
            item->setMenuParent(nullptr);
    }

    [m_nativeMenu release];
}

void QCocoaMenu::setText(const QString &text)
{
    QMacAutoReleasePool pool;
    QString stripped = qt_mac_removeAmpersandEscapes(text);
    m_nativeMenu.title = stripped.toNSString();
}

void QCocoaMenu::setMinimumWidth(int width)
{
    m_nativeMenu.minimumWidth = width;
}

void QCocoaMenu::setFont(const QFont &font)
{
    if (font.resolve()) {
        NSFont *customMenuFont = [NSFont fontWithName:font.family().toNSString()
                                  size:font.pointSize()];
        m_nativeMenu.font = customMenuFont;
    }
}

NSMenu *QCocoaMenu::nsMenu() const
{
    return static_cast<NSMenu *>(m_nativeMenu);
}

void QCocoaMenu::insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before)
{
    QMacAutoReleasePool pool;
    QCocoaMenuItem *cocoaItem = static_cast<QCocoaMenuItem *>(menuItem);
    QCocoaMenuItem *beforeItem = static_cast<QCocoaMenuItem *>(before);

    cocoaItem->sync();
    if (beforeItem) {
        int index = m_menuItems.indexOf(beforeItem);
        // if a before item is supplied, it should be in the menu
        if (index < 0) {
            qWarning("Before menu item not found");
            return;
        }
        m_menuItems.insert(index, cocoaItem);
    } else {
        m_menuItems.append(cocoaItem);
    }

    insertNative(cocoaItem, beforeItem);

    // Empty menus on a menubar are hidden by default. If the menu gets
    // added to the menubar before it contains any item, we need to sync.
    if (isVisible() && attachedItem().hidden) {
        if (auto *mb = qobject_cast<QCocoaMenuBar *>(menuParent()))
            mb->syncMenu(this);
    }
}

void QCocoaMenu::insertNative(QCocoaMenuItem *item, QCocoaMenuItem *beforeItem)
{
    item->resolveTargetAction();
    NSMenuItem *nativeItem = item->nsItem();
    // Someone's adding new items after aboutToShow() was emitted
    if (isOpen() && nativeItem && item->menu())
        item->menu()->setAttachedItem(nativeItem);

    item->setParentEnabled(isEnabled());

    if (item->isMerged())
        return;

    // if the item we're inserting before is merged, skip along until
    // we find a non-merged real item to insert ahead of.
    while (beforeItem && beforeItem->isMerged()) {
        beforeItem = itemOrNull(m_menuItems.indexOf(beforeItem) + 1);
    }

    if (nativeItem.menu) {
        qWarning() << "Menu item" << item->text() << "already in menu" << QString::fromNSString(nativeItem.menu.title);
        return;
    }

    if (beforeItem) {
        if (beforeItem->isMerged()) {
            qWarning("No non-merged before menu item found");
            return;
        }
        const NSInteger nativeIndex = [m_nativeMenu indexOfItem:beforeItem->nsItem()];
        [m_nativeMenu insertItem:nativeItem atIndex:nativeIndex];
    } else {
        [m_nativeMenu addItem:nativeItem];
    }
    item->setMenuParent(this);
}

bool QCocoaMenu::isOpen() const
{
    return m_isOpen;
}

void QCocoaMenu::setIsOpen(bool isOpen)
{
    m_isOpen = isOpen;
}

bool QCocoaMenu::isAboutToShow() const
{
    return m_isAboutToShow;
}

void QCocoaMenu::setIsAboutToShow(bool isAbout)
{
    m_isAboutToShow = isAbout;
}

void QCocoaMenu::removeMenuItem(QPlatformMenuItem *menuItem)
{
    QMacAutoReleasePool pool;
    QCocoaMenuItem *cocoaItem = static_cast<QCocoaMenuItem *>(menuItem);
    if (!m_menuItems.contains(cocoaItem)) {
        qWarning("Menu does not contain the item to be removed");
        return;
    }

    if (cocoaItem->menuParent() == this)
        cocoaItem->setMenuParent(nullptr);

    // Ignore any parent enabled state
    cocoaItem->setParentEnabled(true);

    m_menuItems.removeOne(cocoaItem);
    if (!cocoaItem->isMerged()) {
        if (m_nativeMenu != cocoaItem->nsItem().menu) {
            qWarning("Item to remove does not belong to this menu");
            return;
        }
        [m_nativeMenu removeItem:cocoaItem->nsItem()];
    }
}

QCocoaMenuItem *QCocoaMenu::itemOrNull(int index) const
{
    if ((index < 0) || (index >= m_menuItems.size()))
        return nullptr;

    return m_menuItems.at(index);
}

void QCocoaMenu::scheduleUpdate()
{
    if (!m_updateTimer)
        m_updateTimer = startTimer(0);
}

void QCocoaMenu::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == m_updateTimer) {
        killTimer(m_updateTimer);
        m_updateTimer = 0;
        [m_nativeMenu update];
    }
}

void QCocoaMenu::syncMenuItem(QPlatformMenuItem *menuItem)
{
    syncMenuItem_helper(menuItem, false /*menubarUpdate*/);
}

void QCocoaMenu::syncMenuItem_helper(QPlatformMenuItem *menuItem, bool menubarUpdate)
{
    QMacAutoReleasePool pool;
    QCocoaMenuItem *cocoaItem = static_cast<QCocoaMenuItem *>(menuItem);
    if (!m_menuItems.contains(cocoaItem)) {
        qWarning("Item does not belong to this menu");
        return;
    }

    const bool wasMerged = cocoaItem->isMerged();
    NSMenuItem *oldItem = cocoaItem->nsItem();
    NSMenuItem *syncedItem = cocoaItem->sync();

    if (syncedItem != oldItem) {
        // native item was changed for some reason
        if (oldItem) {
            if (wasMerged) {
                oldItem.enabled = NO;
                oldItem.hidden = YES;
                oldItem.keyEquivalent = @"";
                oldItem.keyEquivalentModifierMask = NSEventModifierFlagCommand;

            } else {
                [m_nativeMenu removeItem:oldItem];
            }
        }

        QCocoaMenuItem* beforeItem = itemOrNull(m_menuItems.indexOf(cocoaItem) + 1);
        insertNative(cocoaItem, beforeItem);
    } else {
        // Schedule NSMenuValidation to kick in. This is needed e.g.
        // when an item's enabled state changes after menuWillOpen:
        scheduleUpdate();
    }

    // This may be a good moment to attach this item's eventual submenu to the
    // synced item, but only on the condition we're all currently hooked to the
    // menunbar. A good indicator of this being the right moment is knowing that
    // we got called from QCocoaMenuBar::updateMenuBarImmediately().
    if (menubarUpdate)
        if (QCocoaMenu *submenu = cocoaItem->menu())
            submenu->setAttachedItem(syncedItem);
}

void QCocoaMenu::syncSeparatorsCollapsible(bool enable)
{
    QMacAutoReleasePool pool;
    if (enable) {
        bool previousIsSeparator = true; // setting to true kills all the separators placed at the top.
        NSMenuItem *previousItem = nil;

        for (NSMenuItem *item in m_nativeMenu.itemArray) {
            if (item.separatorItem) {
                if (auto *cocoaItem = qt_objc_cast<QCocoaNSMenuItem *>(item).platformMenuItem)
                    cocoaItem->setVisible(!previousIsSeparator);
                item.hidden = previousIsSeparator;
            }

            if (!item.hidden) {
                previousItem = item;
                previousIsSeparator = previousItem.separatorItem;
            }
        }

        // We now need to check the final item since we don't want any separators at the end of the list.
        if (previousItem && previousIsSeparator) {
            if (auto *cocoaItem = qt_objc_cast<QCocoaNSMenuItem *>(previousItem).platformMenuItem)
                cocoaItem->setVisible(false);
            previousItem.hidden = YES;
        }
    } else {
        for (auto *item : qAsConst(m_menuItems)) {
            if (!item->isSeparator())
                continue;

            // sync the visiblity directly
            item->sync();
        }
    }
}

void QCocoaMenu::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    const bool wasParentEnabled = m_parentEnabled;
    propagateEnabledState(m_enabled);
    m_parentEnabled = wasParentEnabled; // Reset to the parent value
}

bool QCocoaMenu::isEnabled() const
{
    return m_enabled && m_parentEnabled;
}

void QCocoaMenu::setVisible(bool visible)
{
    m_visible = visible;
}

void QCocoaMenu::showPopup(const QWindow *parentWindow, const QRect &targetRect, const QPlatformMenuItem *item)
{
    QMacAutoReleasePool pool;

    QPoint pos =  QPoint(targetRect.left(), targetRect.top() + targetRect.height());
    QCocoaWindow *cocoaWindow = parentWindow ? static_cast<QCocoaWindow *>(parentWindow->handle()) : nullptr;
    NSView *view = cocoaWindow ? cocoaWindow->view() : nil;
    NSMenuItem *nsItem = item ? ((QCocoaMenuItem *)item)->nsItem() : nil;

    QScreen *screen = nullptr;
    if (parentWindow)
        screen = parentWindow->screen();
    if (!screen && !QGuiApplication::screens().isEmpty())
        screen = QGuiApplication::screens().at(0);
    Q_ASSERT(screen);

    // Ideally, we would call -popUpMenuPositioningItem:atLocation:inView:.
    // However, this showed not to work with modal windows where the menu items
    // would appear disabled. So, we resort to a more artisanal solution. Note
    // that this implies several things.
    if (nsItem) {
        // If we want to position the menu popup so that a specific item lies under
        // the mouse cursor, we resort to NSPopUpButtonCell to do that. This is the
        // typical use-case for a choice list, or non-editable combobox. We can't
        // re-use the popUpContextMenu:withEvent:forView: logic below since it won't
        // respect the menu's minimum width.
        NSPopUpButtonCell *popupCell = [[[NSPopUpButtonCell alloc] initTextCell:@"" pullsDown:NO]
                                                                   autorelease];
        popupCell.altersStateOfSelectedItem = NO;
        popupCell.transparent = YES;
        popupCell.menu = m_nativeMenu;
        [popupCell selectItem:nsItem];

        QCocoaScreen *cocoaScreen = static_cast<QCocoaScreen *>(screen->handle());
        int availableHeight = cocoaScreen->availableGeometry().height();
        const QPoint &globalPos = cocoaWindow->mapToGlobal(pos);
        int menuHeight = m_nativeMenu.size.height;
        if (globalPos.y() + menuHeight > availableHeight) {
            // Maybe we need to fix the vertical popup position but we don't know the
            // exact popup height at the moment (and Cocoa is just guessing) nor its
            // position. So, instead of translating by the popup's full height, we need
            // to estimate where the menu will show up and translate by the remaining height.
            float idx = ([m_nativeMenu indexOfItem:nsItem] + 1.0f) / m_nativeMenu.numberOfItems;
            float heightBelowPos = (1.0 - idx) * menuHeight;
            if (globalPos.y() + heightBelowPos > availableHeight)
                pos.setY(pos.y() - globalPos.y() + availableHeight - heightBelowPos);
        }

        NSRect cellFrame = NSMakeRect(pos.x(), pos.y(), m_nativeMenu.minimumWidth, 10);
        [popupCell performClickWithFrame:cellFrame inView:view];
    } else {
        // Else, we need to transform 'pos' to window or screen coordinates.
        NSPoint nsPos = NSMakePoint(pos.x() - 1, pos.y());
        if (view) {
            // convert coordinates from view to the view's window
            nsPos = [view convertPoint:nsPos toView:nil];
        } else {
            nsPos.y = screen->availableVirtualSize().height() - nsPos.y;
        }

        if (view) {
            // Finally, we need to synthesize an event.
            NSEvent *menuEvent = [NSEvent mouseEventWithType:NSEventTypeRightMouseDown
                                          location:nsPos
                                          modifierFlags:0
                                          timestamp:0
                                          windowNumber:view ? view.window.windowNumber : 0
                                          context:nil
                                          eventNumber:0
                                          clickCount:1
                                          pressure:1.0];
            [NSMenu popUpContextMenu:m_nativeMenu withEvent:menuEvent forView:view];
        } else {
            [m_nativeMenu popUpMenuPositioningItem:nsItem atLocation:nsPos inView:nil];
        }
    }

    // The calls above block, and also swallow any mouse release event,
    // so we need to clear any mouse button that triggered the menu popup.
    if (!cocoaWindow->isForeignWindow())
        [qnsview_cast(view) resetMouseButtons];
}

void QCocoaMenu::dismiss()
{
    [m_nativeMenu cancelTracking];
}

QPlatformMenuItem *QCocoaMenu::menuItemAt(int position) const
{
    if (0 <= position && position < m_menuItems.count())
        return m_menuItems.at(position);

    return nullptr;
}

QPlatformMenuItem *QCocoaMenu::menuItemForTag(quintptr tag) const
{
    for (auto *item : qAsConst(m_menuItems)) {
        if (item->tag() ==  tag)
            return item;
    }

    return nullptr;
}

QList<QCocoaMenuItem *> QCocoaMenu::items() const
{
    return m_menuItems;
}

QList<QCocoaMenuItem *> QCocoaMenu::merged() const
{
    QList<QCocoaMenuItem *> result;
    for (auto *item : qAsConst(m_menuItems)) {
        if (item->menu()) { // recurse into submenus
            result.append(item->menu()->merged());
            continue;
        }

        if (item->isMerged())
            result.append(item);
    }

    return result;
}

void QCocoaMenu::propagateEnabledState(bool enabled)
{
    QMacAutoReleasePool pool; // FIXME Is this still needed for Creator? See 6a0bb4206a2928b83648

    m_parentEnabled = enabled;
    if (!m_enabled && enabled) // Some ancestor was enabled, but this menu is not
        return;

    for (auto *item : qAsConst(m_menuItems)) {
        if (QCocoaMenu *menu = item->menu())
            menu->propagateEnabledState(enabled);
        else
            item->setParentEnabled(enabled);
    }
}

void QCocoaMenu::setAttachedItem(NSMenuItem *item)
{
    if (item == m_attachedItem)
        return;

    if (m_attachedItem)
        m_attachedItem.submenu = nil;

    m_attachedItem = item;

    if (m_attachedItem)
        m_attachedItem.submenu = m_nativeMenu;

}

NSMenuItem *QCocoaMenu::attachedItem() const
{
    return m_attachedItem;
}

QT_END_NAMESPACE
