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

#ifndef QANDROIDPLATFORMMENU_H
#define QANDROIDPLATFORMMENU_H

#include <qpa/qplatformmenu.h>
#include <qvector.h>
#include <qmutex.h>

QT_BEGIN_NAMESPACE

class QAndroidPlatformMenuItem;
class QAndroidPlatformMenu: public QPlatformMenu
{
public:
    typedef QVector<QAndroidPlatformMenuItem *> PlatformMenuItemsType;

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
