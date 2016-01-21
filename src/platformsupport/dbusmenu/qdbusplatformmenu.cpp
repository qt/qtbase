/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
QHash<int, QDBusPlatformMenu *> menusByID;
QHash<int, QDBusPlatformMenuItem *> menuItemsByID;
QList<QDBusPlatformMenu *> QDBusPlatformMenu::m_topLevelMenus;

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
    m_subMenu = static_cast<QDBusPlatformMenu *>(menu);
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
    return menuItemsByID[id];
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
    , m_dbusID(nextDBusID++)
    , m_revision(0)
{
    menusByID.insert(m_dbusID, this);
    // Assume it's top-level until we find out otherwise
    m_topLevelMenus << this;
}

QDBusPlatformMenu::~QDBusPlatformMenu()
{
    menusByID.remove(m_dbusID);
    m_topLevelMenus.removeOne(this);
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
    // If a menu is found as a submenu under an item, we know that it's not a top-level menu.
    if (item->menu())
        m_topLevelMenus.removeOne(const_cast<QDBusPlatformMenu *>(static_cast<const QDBusPlatformMenu *>(item->menu())));
}

void QDBusPlatformMenu::removeMenuItem(QPlatformMenuItem *menuItem)
{
    m_items.removeAll(static_cast<QDBusPlatformMenuItem *>(menuItem));
    m_itemsByTag.remove(menuItem->tag());
}

void QDBusPlatformMenu::syncMenuItem(QPlatformMenuItem *menuItem)
{
    // TODO keep around copies of the QDBusMenuLayoutItems so they can be updated?
    // or eliminate them by putting dbus streaming operators in this class instead?
    // or somehow tell the dbusmenu client that something has changed, so it will ask for properties again
    emitUpdated();
    QDBusMenuItemList updated;
    QDBusMenuItemKeysList removed;
    updated << QDBusMenuItem(static_cast<QDBusPlatformMenuItem *>(menuItem));
    qCDebug(qLcMenu) << updated;
    emit propertiesUpdated(updated, removed);
}

QDBusPlatformMenu *QDBusPlatformMenu::byId(int id)
{
    return menusByID[id];
}

void QDBusPlatformMenu::emitUpdated()
{
    emit updated(++m_revision, m_dbusID);
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

QPlatformMenuItem *QDBusPlatformMenu::menuItemAt(int position) const
{
    return m_items.at(position);
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
