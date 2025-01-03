// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDPLATFORMMENUBAR_H
#define QANDROIDPLATFORMMENUBAR_H

#include <qpa/qplatformmenu.h>
#include <qhash.h>
#include <qlist.h>
#include <qmutex.h>

QT_BEGIN_NAMESPACE

class QAndroidPlatformMenu;
class QAndroidPlatformMenuBar: public QPlatformMenuBar
{
public:
    typedef QList<QAndroidPlatformMenu *> PlatformMenusType;
public:
    QAndroidPlatformMenuBar();
    ~QAndroidPlatformMenuBar();

    void insertMenu(QPlatformMenu *menu, QPlatformMenu *before) override;
    void removeMenu(QPlatformMenu *menu) override;
    void syncMenu(QPlatformMenu *menu) override;
    void handleReparent(QWindow *newParentWindow) override;
    QPlatformMenu *menuForTag(quintptr tag) const override;
    QPlatformMenu *menuForId(int menuId) const;
    int menuId(QPlatformMenu *menu) const;

    QWindow *parentWindow() const override;
    PlatformMenusType menus() const;
    QMutex *menusListMutex();

private:
    PlatformMenusType m_menus;
    QWindow *m_parentWindow;
    QMutex m_menusListMutex;

    int m_nextMenuId = 0;
    QHash<int, QPlatformMenu *> m_menuHash;
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMMENUBAR_H
