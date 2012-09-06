/****************************************************************************
**
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author James Turner <james.turner@kdab.com>
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcocoamenu.h"

#include "qcocoahelpers.h"
#include "qcocoaautoreleasepool.h"

@interface QT_MANGLE_NAMESPACE(QCocoaMenuDelegate) : NSObject <NSMenuDelegate> {
    QCocoaMenu *m_menu;
}

- (id) initWithMenu:(QCocoaMenu*) m;

@end

@implementation QT_MANGLE_NAMESPACE(QCocoaMenuDelegate)

- (id) initWithMenu:(QCocoaMenu*) m
{
    if ((self = [super init]))
        m_menu = m;

    return self;
}

- (void) menuWillOpen:(NSMenu*)m
{
    Q_UNUSED(m);
    emit m_menu->aboutToShow();
}

- (void) menuDidClose:(NSMenu*)m
{
    Q_UNUSED(m);
    // wrong, but it's the best we can do
    emit m_menu->aboutToHide();
}

- (void) itemFired:(NSMenuItem*) item
{
    QCocoaMenuItem *cocoaItem = reinterpret_cast<QCocoaMenuItem *>([item tag]);
    cocoaItem->activated();
}

- (BOOL)validateMenuItem:(NSMenuItem*)menuItem
{
    if (![menuItem tag])
        return YES;


    QCocoaMenuItem* cocoaItem = reinterpret_cast<QCocoaMenuItem *>([menuItem tag]);
    return cocoaItem->isEnabled();
}

@end

QT_BEGIN_NAMESPACE

QCocoaMenu::QCocoaMenu() :
    m_enabled(true),
    m_tag(0)
{
    m_delegate = [[QT_MANGLE_NAMESPACE(QCocoaMenuDelegate) alloc] initWithMenu:this];
    m_nativeItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    m_nativeMenu = [[NSMenu alloc] initWithTitle:@"Untitled"];
    [m_nativeMenu setAutoenablesItems:YES];
    m_nativeMenu.delegate = (QT_MANGLE_NAMESPACE(QCocoaMenuDelegate) *) m_delegate;
    [m_nativeItem setSubmenu:m_nativeMenu];
}

QCocoaMenu::~QCocoaMenu()
{
    QCocoaAutoReleasePool pool;
    [m_nativeItem setSubmenu:nil];
    [m_nativeMenu release];
    [m_delegate release];
    [m_nativeItem release];
}

void QCocoaMenu::setText(const QString &text)
{
    QCocoaAutoReleasePool pool;
    QString stripped = qt_mac_removeAmpersandEscapes(text);
    [m_nativeMenu setTitle:QCFString::toNSString(stripped)];
    [m_nativeItem setTitle:QCFString::toNSString(stripped)];
}

void QCocoaMenu::insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before)
{
    QCocoaAutoReleasePool pool;
    QCocoaMenuItem *cocoaItem = static_cast<QCocoaMenuItem *>(menuItem);
    QCocoaMenuItem *beforeItem = static_cast<QCocoaMenuItem *>(before);

    cocoaItem->sync();
    if (beforeItem) {
        int index = m_menuItems.indexOf(beforeItem);
        // if a before item is supplied, it should be in the menu
        Q_ASSERT(index >= 0);
        m_menuItems.insert(index, cocoaItem);
    } else {
        m_menuItems.append(cocoaItem);
    }

    insertNative(cocoaItem, beforeItem);
}

void QCocoaMenu::insertNative(QCocoaMenuItem *item, QCocoaMenuItem *beforeItem)
{
    [item->nsItem() setTarget:m_delegate];
    if (!item->menu())
        [item->nsItem() setAction:@selector(itemFired:)];

    if (item->isMerged())
        return;

    Q_ASSERT([item->nsItem() menu] == NULL);
    // if the item we're inserting before is merged, skip along until
    // we find a non-merged real item to insert ahead of.
    while (beforeItem && beforeItem->isMerged()) {
        beforeItem = itemOrNull(m_menuItems.indexOf(beforeItem) + 1);
    }

    if (beforeItem) {
        Q_ASSERT(!beforeItem->isMerged());
        NSUInteger nativeIndex = [m_nativeMenu indexOfItem:beforeItem->nsItem()];
        [m_nativeMenu insertItem: item->nsItem() atIndex: nativeIndex];
    } else {
        [m_nativeMenu addItem: item->nsItem()];
    }
}

void QCocoaMenu::removeMenuItem(QPlatformMenuItem *menuItem)
{
    QCocoaAutoReleasePool pool;
    QCocoaMenuItem *cocoaItem = static_cast<QCocoaMenuItem *>(menuItem);
    Q_ASSERT(m_menuItems.contains(cocoaItem));
    m_menuItems.removeOne(cocoaItem);
    if (!cocoaItem->isMerged()) {
        Q_ASSERT(m_nativeMenu == [cocoaItem->nsItem() menu]);
        [m_nativeMenu removeItem: cocoaItem->nsItem()];
    }
}

QCocoaMenuItem *QCocoaMenu::itemOrNull(int index) const
{
    if ((index < 0) || (index >= m_menuItems.size()))
        return 0;

    return m_menuItems.at(index);
}

void QCocoaMenu::syncMenuItem(QPlatformMenuItem *menuItem)
{
    QCocoaAutoReleasePool pool;
    QCocoaMenuItem *cocoaItem = static_cast<QCocoaMenuItem *>(menuItem);
    Q_ASSERT(m_menuItems.contains(cocoaItem));

    bool wasMerged = cocoaItem->isMerged();
    NSMenuItem *oldItem = [m_nativeMenu itemWithTag:(NSInteger) cocoaItem];

    if (cocoaItem->sync() != oldItem) {
    // native item was changed for some reason
        if (!wasMerged) {
            [m_nativeMenu removeItem:oldItem];
        }

        QCocoaMenuItem* beforeItem = itemOrNull(m_menuItems.indexOf(cocoaItem) + 1);
        insertNative(cocoaItem, beforeItem);
    }
}

void QCocoaMenu::syncSeparatorsCollapsible(bool enable)
{
    QCocoaAutoReleasePool pool;
    if (enable) {
        bool previousIsSeparator = true; // setting to true kills all the separators placed at the top.
        NSMenuItem *previousItem = nil;

        NSArray *itemArray = [m_nativeMenu itemArray];
        for (unsigned int i = 0; i < [itemArray count]; ++i) {
            NSMenuItem *item = reinterpret_cast<NSMenuItem *>([itemArray objectAtIndex:i]);
            if ([item isSeparatorItem])
                [item setHidden:previousIsSeparator];

            if (![item isHidden]) {
                previousItem = item;
                previousIsSeparator = ([previousItem isSeparatorItem]);
            }
        }

        // We now need to check the final item since we don't want any separators at the end of the list.
        if (previousItem && previousIsSeparator)
            [previousItem setHidden:YES];
    } else {
        foreach (QCocoaMenuItem *item, m_menuItems) {
            if (!item->isSeparator())
                continue;

            // sync the visiblity directly
            item->sync();
        }
    }
}

void QCocoaMenu::setParentItem(QCocoaMenuItem *item)
{
    Q_UNUSED(item);
}

void QCocoaMenu::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

QPlatformMenuItem *QCocoaMenu::menuItemAt(int position) const
{
    return m_menuItems.at(position);
}

QPlatformMenuItem *QCocoaMenu::menuItemForTag(quintptr tag) const
{
    foreach (QCocoaMenuItem *item, m_menuItems) {
        if (item->tag() ==  tag)
            return item;
    }

    return 0;
}

QList<QCocoaMenuItem *> QCocoaMenu::merged() const
{
    QList<QCocoaMenuItem *> result;
    foreach (QCocoaMenuItem *item, m_menuItems) {
        if (item->menu()) { // recurse into submenus
            result.append(item->menu()->merged());
            continue;
        }

        if (item->isMerged())
            result.append(item);
    }

    return result;
}

void QCocoaMenu::syncModalState(bool modal)
{
    if (!m_enabled)
        modal = true;

    [m_nativeItem setEnabled:!modal];

    foreach (QCocoaMenuItem *item, m_menuItems) {
        if (item->menu()) { // recurse into submenus
            item->menu()->syncModalState(modal);
            continue;
        }

        item->syncModalState(modal);
    }
}

QT_END_NAMESPACE
