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

    PlatformMenuItemsType menuItems() const;
    QMutex *menuItemsMutex();

private:
    PlatformMenuItemsType m_menuItems;
    QString m_text;
    QIcon m_icon;
    bool m_enabled;
    bool m_isVisible;
    QMutex m_menuItemsMutex;
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMMENU_H
