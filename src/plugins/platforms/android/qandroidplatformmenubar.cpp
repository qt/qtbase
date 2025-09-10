// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
