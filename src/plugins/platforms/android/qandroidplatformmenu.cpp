/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
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

#include "qandroidplatformmenu.h"
#include "qandroidplatformmenuitem.h"
#include "androidjnimenu.h"
#include <QtCore/private/qjni_p.h>

QT_BEGIN_NAMESPACE

QAndroidPlatformMenu::QAndroidPlatformMenu()
{
    m_tag = reinterpret_cast<quintptr>(this); // QMenu will overwrite this later, but we need a unique ID for QtQuick
    m_enabled = true;
    m_isVisible = true;
}

QAndroidPlatformMenu::~QAndroidPlatformMenu()
{
    QtAndroidMenu::androidPlatformMenuDestroyed(this);
}

void QAndroidPlatformMenu::insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before)
{
    QMutexLocker lock(&m_menuItemsMutex);
    m_menuItems.insert(std::find(m_menuItems.begin(),
                                 m_menuItems.end(),
                                 static_cast<QAndroidPlatformMenuItem *>(before)),
                       static_cast<QAndroidPlatformMenuItem *>(menuItem));
}

void QAndroidPlatformMenu::removeMenuItem(QPlatformMenuItem *menuItem)
{
    QMutexLocker lock(&m_menuItemsMutex);
    PlatformMenuItemsType::iterator it = std::find(m_menuItems.begin(),
                                                   m_menuItems.end(),
                                                   static_cast<QAndroidPlatformMenuItem *>(menuItem));
    if (it != m_menuItems.end())
        m_menuItems.erase(it);
}

void QAndroidPlatformMenu::syncMenuItem(QPlatformMenuItem *menuItem)
{
    PlatformMenuItemsType::iterator it;
    for (it = m_menuItems.begin(); it != m_menuItems.end(); ++it) {
        if ((*it)->tag() == menuItem->tag())
            break;
    }

    if (it != m_menuItems.end())
        QtAndroidMenu::syncMenu(this);
}

void QAndroidPlatformMenu::syncSeparatorsCollapsible(bool enable)
{
    Q_UNUSED(enable)
}

void QAndroidPlatformMenu::setTag(quintptr tag)
{
    m_tag = tag;
}

quintptr QAndroidPlatformMenu::tag() const
{
    return m_tag;
}

void QAndroidPlatformMenu::setText(const QString &text)
{
    m_text = text;
}

QString QAndroidPlatformMenu::text() const
{
    return m_text;
}

void QAndroidPlatformMenu::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

QIcon QAndroidPlatformMenu::icon() const
{
    return m_icon;
}

void QAndroidPlatformMenu::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool QAndroidPlatformMenu::isEnabled() const
{
    return m_enabled;
}

void QAndroidPlatformMenu::setVisible(bool visible)
{
    m_isVisible = visible;
}

bool QAndroidPlatformMenu::isVisible() const
{
    return m_isVisible;
}

void QAndroidPlatformMenu::showPopup(const QWindow *parentWindow, const QRect &targetRect, const QPlatformMenuItem *item)
{
    Q_UNUSED(parentWindow);
    Q_UNUSED(item);
    setVisible(true);
    QtAndroidMenu::showContextMenu(this, targetRect, QJNIEnvironmentPrivate());
}

QPlatformMenuItem *QAndroidPlatformMenu::menuItemAt(int position) const
{
    if (position < m_menuItems.size())
        return m_menuItems[position];
    return 0;
}

QPlatformMenuItem *QAndroidPlatformMenu::menuItemForTag(quintptr tag) const
{
    for (QPlatformMenuItem *menuItem : m_menuItems) {
        if (menuItem->tag() == tag)
            return menuItem;
    }
    return 0;
}

QAndroidPlatformMenu::PlatformMenuItemsType QAndroidPlatformMenu::menuItems() const
{
    return m_menuItems;
}

QMutex *QAndroidPlatformMenu::menuItemsMutex()
{
    return &m_menuItemsMutex;
}

QT_END_NAMESPACE
