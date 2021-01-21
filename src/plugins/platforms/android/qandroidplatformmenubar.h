/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QANDROIDPLATFORMMENUBAR_H
#define QANDROIDPLATFORMMENUBAR_H

#include <qpa/qplatformmenu.h>
#include <qvector.h>
#include <qmutex.h>
#include <qhash.h>

QT_BEGIN_NAMESPACE

class QAndroidPlatformMenu;
class QAndroidPlatformMenuBar: public QPlatformMenuBar
{
public:
    typedef QVector<QAndroidPlatformMenu *> PlatformMenusType;
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
