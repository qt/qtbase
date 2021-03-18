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

#include "qandroidplatformmenubar.h"
#include "qandroidplatformmenu.h"
#include "androidjnimenu.h"

QT_BEGIN_NAMESPACE

QAndroidPlatformMenuBar::QAndroidPlatformMenuBar()
{
    m_parentWindow = 0;
    QtAndroidMenu::addMenuBar(this);
}

QAndroidPlatformMenuBar::~QAndroidPlatformMenuBar()
{
    QtAndroidMenu::removeMenuBar(this);
}

void QAndroidPlatformMenuBar::insertMenu(QPlatformMenu *menu, QPlatformMenu *before)
{
    QMutexLocker lock(&m_menusListMutex);
    m_menus.insert(std::find(m_menus.begin(),
                             m_menus.end(),
                             static_cast<QAndroidPlatformMenu *>(before)),
                   static_cast<QAndroidPlatformMenu *>(menu));
    m_menuHash.insert(m_nextMenuId++, menu);
}

void QAndroidPlatformMenuBar::removeMenu(QPlatformMenu *menu)
{
    QMutexLocker lock(&m_menusListMutex);
    m_menus.erase(std::find(m_menus.begin(),
                            m_menus.end(),
                            static_cast<QAndroidPlatformMenu *>(menu)));

    int maxId = -1;
    QHash<int, QPlatformMenu *>::iterator it = m_menuHash.begin();
    while (it != m_menuHash.end()) {
        if (it.value() == menu) {
            it = m_menuHash.erase(it);
        } else {
            maxId = qMax(maxId, it.key());
            ++it;
        }
    }

    m_nextMenuId = maxId + 1;
}

int QAndroidPlatformMenuBar::menuId(QPlatformMenu *menu) const
{
    QHash<int, QPlatformMenu *>::const_iterator it;
    for (it = m_menuHash.constBegin(); it != m_menuHash.constEnd(); ++it) {
        if (it.value() == menu)
            return it.key();
    }

    return -1;
}

void QAndroidPlatformMenuBar::syncMenu(QPlatformMenu *menu)
{
    QtAndroidMenu::syncMenu(static_cast<QAndroidPlatformMenu *>(menu));
}

void QAndroidPlatformMenuBar::handleReparent(QWindow *newParentWindow)
{
    if (m_parentWindow == newParentWindow)
        return;
    m_parentWindow = newParentWindow;
    QtAndroidMenu::setMenuBar(this, newParentWindow);
}

QPlatformMenu *QAndroidPlatformMenuBar::menuForTag(quintptr tag) const
{
    for (QAndroidPlatformMenu *menu : m_menus) {
        if (menu->tag() == tag)
            return menu;
    }

    return nullptr;
}

QPlatformMenu *QAndroidPlatformMenuBar::menuForId(int menuId) const
{
    return m_menuHash.value(menuId);
}

QWindow *QAndroidPlatformMenuBar::parentWindow() const
{
    return m_parentWindow;
}

QAndroidPlatformMenuBar::PlatformMenusType QAndroidPlatformMenuBar::menus() const
{
    return m_menus;
}

QMutex *QAndroidPlatformMenuBar::menusListMutex()
{
    return &m_menusListMutex;
}

QT_END_NAMESPACE
