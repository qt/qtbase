// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIOSMENU_H
#define QIOSMENU_H

#import <UIKit/UIKit.h>

#include <QtCore/QtCore>
#include <qpa/qplatformmenu.h>

#import "quiview.h"

#include <QtCore/qpointer.h>

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
