/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#ifndef QGTK3MENU_H
#define QGTK3MENU_H

#include <QtGui/qpa/qplatformmenu.h>

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkMenuItem GtkMenuItem;
typedef struct _GtkCheckMenuItem GtkCheckMenuItem;

QT_BEGIN_NAMESPACE

class QGtk3Menu;

class QGtk3MenuItem: public QPlatformMenuItem
{
public:
    QGtk3MenuItem();
    ~QGtk3MenuItem();

    bool isInvalid() const;

    GtkWidget *create();
    GtkWidget *handle() const;

    QString text() const;
    void setText(const QString &text) override;

    QGtk3Menu *menu() const;
    void setMenu(QPlatformMenu *menu) override;

    bool isVisible() const;
    void setVisible(bool visible) override;

    bool isSeparator() const;
    void setIsSeparator(bool separator) override;

    bool isCheckable() const;
    void setCheckable(bool checkable) override;

    bool isChecked() const;
    void setChecked(bool checked) override;

#if QT_CONFIG(shortcut)
    QKeySequence shortcut() const;
    void setShortcut(const QKeySequence &shortcut) override;
#endif

    bool isEnabled() const;
    void setEnabled(bool enabled) override;

    bool hasExclusiveGroup() const;
    void setHasExclusiveGroup(bool exclusive) override;

    void setRole(MenuRole role) override { Q_UNUSED(role); }
    void setFont(const QFont &font) override { Q_UNUSED(font); }
    void setIcon(const QIcon &icon) override { Q_UNUSED(icon); }
    void setIconSize(int size) override { Q_UNUSED(size); }

protected:
    static void onSelect(GtkMenuItem *item, void *data);
    static void onActivate(GtkMenuItem *item, void *data);
    static void onToggle(GtkCheckMenuItem *item, void *data);

private:
    bool m_visible;
    bool m_separator;
    bool m_checkable;
    bool m_checked;
    bool m_enabled;
    bool m_exclusive;
    bool m_underline;
    bool m_invalid;
    QGtk3Menu *m_menu;
    GtkWidget *m_item;
    QString m_text;
#if QT_CONFIG(shortcut)
    QKeySequence m_shortcut;
#endif
};

class QGtk3Menu : public QPlatformMenu
{
    Q_OBJECT

public:
    QGtk3Menu();
    ~QGtk3Menu();

    GtkWidget *handle() const;

    void insertMenuItem(QPlatformMenuItem *item, QPlatformMenuItem *before) override;
    void removeMenuItem(QPlatformMenuItem *item) override;
    void syncMenuItem(QPlatformMenuItem *item) override;
    void syncSeparatorsCollapsible(bool enable) override;

    void setEnabled(bool enabled) override;
    void setVisible(bool visible) override;

    void setIcon(const QIcon &icon) override { Q_UNUSED(icon); }
    void setText(const QString &text) override { Q_UNUSED(text); }

    QPoint targetPos() const;

    void showPopup(const QWindow *parentWindow, const QRect &targetRect, const QPlatformMenuItem *item) override;
    void dismiss() override;

    QPlatformMenuItem *menuItemAt(int position) const override;
    QPlatformMenuItem *menuItemForTag(quintptr tag) const override;

    QPlatformMenuItem *createMenuItem() const override;
    QPlatformMenu *createSubMenu() const override;

protected:
    static void onShow(GtkWidget *menu, void *data);
    static void onHide(GtkWidget *menu, void *data);

private:
    GtkWidget *m_menu;
    QPoint m_targetPos;
    QVector<QGtk3MenuItem *> m_items;
};

QT_END_NAMESPACE

#endif // QGTK3MENU_H
