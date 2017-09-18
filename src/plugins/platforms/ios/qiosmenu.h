/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QIOSMENU_H
#define QIOSMENU_H

#import <UIKit/UIKit.h>

#include <QtCore/QtCore>
#include <qpa/qplatformmenu.h>

#import "quiview.h"

class QIOSMenu;
@class QUIMenuController;
@class QUIPickerView;

class QIOSMenuItem : public QPlatformMenuItem
{
public:
    QIOSMenuItem();

    void setText(const QString &text) override;
    void setIcon(const QIcon &) override {}
    void setMenu(QPlatformMenu *) override;
    void setVisible(bool isVisible) override;
    void setIsSeparator(bool) override;
    void setFont(const QFont &) override {}
    void setRole(MenuRole role) override;
    void setCheckable(bool) override {}
    void setChecked(bool) override {}
#ifndef QT_NO_SHORTCUT
    void setShortcut(const QKeySequence&) override;
#endif
    void setEnabled(bool enabled) override;
    void setIconSize(int) override {}

    bool m_visible;
    QString m_text;
    MenuRole m_role;
    bool m_enabled;
    bool m_separator;
    QIOSMenu *m_menu;
    QKeySequence m_shortcut;
};

typedef QList<QIOSMenuItem *> QIOSMenuItemList;

class QIOSMenu : public QPlatformMenu
{
public:
    QIOSMenu();
    ~QIOSMenu();

    void insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before) override;
    void removeMenuItem(QPlatformMenuItem *menuItem) override;
    void syncMenuItem(QPlatformMenuItem *) override;
    void syncSeparatorsCollapsible(bool) override {}

    void setText(const QString &) override;
    void setIcon(const QIcon &) override {}
    void setEnabled(bool enabled) override;
    void setVisible(bool visible) override;
    void setMenuType(MenuType type) override;

    void showPopup(const QWindow *parentWindow, const QRect &targetRect, const QPlatformMenuItem *item) override;
    void dismiss() override;

    QPlatformMenuItem *menuItemAt(int position) const override;
    QPlatformMenuItem *menuItemForTag(quintptr tag) const override;

    void handleItemSelected(QIOSMenuItem *menuItem);

    static QIOSMenu *currentMenu() { return m_currentMenu; }
    static id menuActionTarget() { return m_currentMenu ? m_currentMenu->m_menuController : 0; }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    bool m_enabled;
    bool m_visible;
    QString m_text;
    MenuType m_menuType;
    MenuType m_effectiveMenuType;
    QPointer<QWindow> m_parentWindow;
    QRect m_targetRect;
    const QIOSMenuItem *m_targetItem;
    QUIMenuController *m_menuController;
    QUIPickerView *m_pickerView;
    QIOSMenuItemList m_menuItems;

    static QIOSMenu *m_currentMenu;

    void updateVisibility();
    void toggleShowUsingUIMenuController(bool show);
    void toggleShowUsingUIPickerView(bool show);
    QIOSMenuItemList visibleMenuItems() const;
    QIOSMenuItemList filterFirstResponderActions(const QIOSMenuItemList &menuItems);
    void repositionMenu();
};

#endif // QIOSMENU_H
