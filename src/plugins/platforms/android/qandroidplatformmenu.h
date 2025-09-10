// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDPLATFORMMENU_H
#define QANDROIDPLATFORMMENU_H

#include <qhash.h>
#include <qpa/qplatformmenu.h>
#include <qlist.h>
#include <qmutex.h>

QT_BEGIN_NAMESPACE

class QAndroidPlatformMenuItem;
class QAndroidPlatformMenu: public QPlatformMenu
{
public:
    typedef QList<QAndroidPlatformMenuItem *> PlatformMenuItemsType;

public:
    QAndroidPlatformMenu();
    ~QAndroidPlatformMenu();

    void insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before) override;
    void removeMenuItem(QPlatformMenuItem *menuItem) override;
    void syncMenuItem(QPlatformMenuItem *menuItem) override;
    void syncSeparatorsCollapsible(bool enable) override;

    void setText(const QString &text) override;
    QString text() const;
    void setIcon(const QIcon &icon) override;
    QIcon icon() const;
    void setEnabled(bool enabled) override;
    bool isEnabled() const override;
    void setVisible(bool visible) override;
    bool isVisible() const;
    void showPopup(const QWindow *parentWindow, const QRect &targetRect, const QPlatformMenuItem *item) override;

    QPlatformMenuItem *menuItemAt(int position) const override;
    QPlatformMenuItem *menuItemForTag(quintptr tag) const override;
    QPlatformMenuItem *menuItemForId(int menuId) const;
    int menuId(QPlatformMenuItem *menuItem) const;

    PlatformMenuItemsType menuItems() const;
    QMutex *menuItemsMutex();

private:
    PlatformMenuItemsType m_menuItems;
    QString m_text;
    QIcon m_icon;
    bool m_enabled;
    bool m_isVisible;
    QMutex m_menuItemsMutex;

    int m_nextMenuId = 0;
    QHash<int, QPlatformMenuItem *> m_menuHash;
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMMENU_H
