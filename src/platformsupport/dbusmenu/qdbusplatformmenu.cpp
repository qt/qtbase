/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qdbusplatformmenu_p.h"

#include <QDebug>
#include <QWindow>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcMenu, "qt.qpa.menu")

static int nextDBusID = 1;
QHash<int, QDBusPlatformMenuItem *> menuItemsByID;

QDBusPlatformMenuItem::QDBusPlatformMenuItem(quintptr tag)
    : m_tag(tag ? tag : reinterpret_cast<quintptr>(this)) // QMenu will overwrite this later
    , m_subMenu(Q_NULLPTR)
    , m_role(NoRole)
    , m_isEnabled(true)
    , m_isVisible(true)
    , m_isSeparator(false)
    , m_isCheckable(false)
    , m_isChecked(false)
    , m_dbusID(nextDBusID++)
    , m_hasExclusiveGroup(false)
{
    menuItemsByID.insert(m_dbusID, this);
}

QDBusPlatformMenuItem::~QDBusPlatformMenuItem()
{
    menuItemsByID.remove(m_dbusID);
}

void QDBusPlatformMenuItem::setTag(quintptr tag)
{
    m_tag = tag;
}

void QDBusPlatformMenuItem::setText(const QString &text)
{
    qCDebug(qLcMenu) << m_dbusID << text;
    m_text = text;
}

void QDBusPlatformMenuItem::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

/*!
    Set a submenu under this menu item.
*/
void QDBusPlatformMenuItem::setMenu(QPlatformMenu *menu)
{
    if (m_subMenu)
        static_cast<QDBusPlatformMenu *>(m_subMenu)->setContainingMenuItem(Q_NULLPTR);
    m_subMenu = menu;
    if (menu)
        static_cast<QDBusPlatformMenu *>(menu)->setContainingMenuItem(this);
}

void QDBusPlatformMenuItem::setEnabled(bool enabled)
{
    m_isEnabled = enabled;
}

void QDBusPlatformMenuItem::setVisible(bool isVisible)
{
    m_isVisible = isVisible;
}

void QDBusPlatformMenuItem::setIsSeparator(bool isSeparator)
{
    m_isSeparator = isSeparator;
}

void QDBusPlatformMenuItem::setRole(QPlatformMenuItem::MenuRole role)
{
    m_role = role;
}

void QDBusPlatformMenuItem::setCheckable(bool checkable)
{
    m_isCheckable = checkable;
}

void QDBusPlatformMenuItem::setChecked(bool isChecked)
{
    m_isChecked = isChecked;
}

void QDBusPlatformMenuItem::setHasExclusiveGroup(bool hasExclusiveGroup)
{
    m_hasExclusiveGroup = hasExclusiveGroup;
}

void QDBusPlatformMenuItem::setShortcut(const QKeySequence &shortcut)
{
    m_shortcut = shortcut;
}

void QDBusPlatformMenuItem::trigger()
{
    emit activated();
}

QDBusPlatformMenuItem *QDBusPlatformMenuItem::byId(int id)
{
    // We need to check contains because otherwise QHash would insert
    // a default-constructed nullptr value into menuItemsByID
    if (menuItemsByID.contains(id))
        return menuItemsByID[id];
    return Q_NULLPTR;
}

QList<const QDBusPlatformMenuItem *> QDBusPlatformMenuItem::byIds(const QList<int> &ids)
{
    QList<const QDBusPlatformMenuItem *> ret;
    Q_FOREACH (int id, ids) {
        if (menuItemsByID.contains(id))
            ret << menuItemsByID[id];
    }
    return ret;
}


QDBusPlatformMenu::QDBusPlatformMenu(quintptr tag)
    : m_tag(tag ? tag : reinterpret_cast<quintptr>(this))
    , m_isEnabled(true)
    , m_isVisible(true)
    , m_isSeparator(false)
    , m_revision(1)
    , m_containingMenuItem(Q_NULLPTR)
{
}

QDBusPlatformMenu::~QDBusPlatformMenu()
{
}

void QDBusPlatformMenu::insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before)
{
    QDBusPlatformMenuItem *item = static_cast<QDBusPlatformMenuItem *>(menuItem);
    QDBusPlatformMenuItem *beforeItem = static_cast<QDBusPlatformMenuItem *>(before);
    int idx = m_items.indexOf(beforeItem);
    qCDebug(qLcMenu) << item->dbusID() << item->text();
    if (idx < 0)
        m_items.append(item);
    else
        m_items.insert(idx, item);
    m_itemsByTag.insert(item->tag(), item);
    if (item->menu())
        syncSubMenu(static_cast<const QDBusPlatformMenu *>(item->menu()));
    emitUpdated();
}

void QDBusPlatformMenu::removeMenuItem(QPlatformMenuItem *menuItem)
{
    QDBusPlatformMenuItem *item = static_cast<QDBusPlatformMenuItem *>(menuItem);
    m_items.removeAll(item);
    m_itemsByTag.remove(menuItem->tag());
    if (item->menu()) {
        // disconnect from the signals we connected to in syncSubMenu()
        const QDBusPlatformMenu *menu = static_cast<const QDBusPlatformMenu *>(item->menu());
        disconnect(menu, &QDBusPlatformMenu::propertiesUpdated,
                   this, &QDBusPlatformMenu::propertiesUpdated);
        disconnect(menu, &QDBusPlatformMenu::updated,
                   this, &QDBusPlatformMenu::updated);
    }
    emitUpdated();
}

void QDBusPlatformMenu::syncSubMenu(const QDBusPlatformMenu *menu)
{
    // The adaptor is only connected to the propertiesUpdated signal of the top-level
    // menu, so the submenus should transfer their signals to their parents.
    connect(menu, &QDBusPlatformMenu::propertiesUpdated,
            this, &QDBusPlatformMenu::propertiesUpdated, Qt::UniqueConnection);
    connect(menu, &QDBusPlatformMenu::updated,
            this, &QDBusPlatformMenu::updated, Qt::UniqueConnection);
}

void QDBusPlatformMenu::syncMenuItem(QPlatformMenuItem *menuItem)
{
    QDBusPlatformMenuItem *item = static_cast<QDBusPlatformMenuItem *>(menuItem);
    // if a submenu was added to this item, we need to connect to its signals
    if (item->menu())
        syncSubMenu(static_cast<const QDBusPlatformMenu *>(item->menu()));
    // TODO keep around copies of the QDBusMenuLayoutItems so they can be updated?
    // or eliminate them by putting dbus streaming operators in this class instead?
    // or somehow tell the dbusmenu client that something has changed, so it will ask for properties again
    QDBusMenuItemList updated;
    QDBusMenuItemKeysList removed;
    updated << QDBusMenuItem(item);
    qCDebug(qLcMenu) << updated;
    emit propertiesUpdated(updated, removed);
}

void QDBusPlatformMenu::emitUpdated()
{
    if (m_containingMenuItem)
        emit updated(++m_revision, m_containingMenuItem->dbusID());
    else
        emit updated(++m_revision, 0);
}

void QDBusPlatformMenu::setTag(quintptr tag)
{
    m_tag = tag;
}

void QDBusPlatformMenu::setText(const QString &text)
{
    m_text = text;
}

void QDBusPlatformMenu::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

void QDBusPlatformMenu::setEnabled(bool enabled)
{
    m_isEnabled = enabled;
}

void QDBusPlatformMenu::setVisible(bool isVisible)
{
    m_isVisible = isVisible;
}

void QDBusPlatformMenu::setContainingMenuItem(QDBusPlatformMenuItem *item)
{
    m_containingMenuItem = item;
}

QPlatformMenuItem *QDBusPlatformMenu::menuItemAt(int position) const
{
    return m_items.value(position);
}

QPlatformMenuItem *QDBusPlatformMenu::menuItemForTag(quintptr tag) const
{
    return m_itemsByTag[tag];
}

const QList<QDBusPlatformMenuItem *> QDBusPlatformMenu::items() const
{
    return m_items;
}

QPlatformMenuItem *QDBusPlatformMenu::createMenuItem() const
{
    QDBusPlatformMenuItem *ret = new QDBusPlatformMenuItem();
    return ret;
}

QPlatformMenu *QDBusPlatformMenu::createSubMenu() const
{
    return new QDBusPlatformMenu;
}

QT_END_NAMESPACE
