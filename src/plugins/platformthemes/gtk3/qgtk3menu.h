/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

    quintptr tag() const;
    void setTag(quintptr tag) override;

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

    QKeySequence shortcut() const;
    void setShortcut(const QKeySequence &shortcut) override;

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
    quintptr m_tag;
    QGtk3Menu *m_menu;
    GtkWidget *m_item;
    QString m_text;
    QKeySequence m_shortcut;
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

    quintptr tag() const override;
    void setTag(quintptr tag) override;

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
    quintptr m_tag;
    GtkWidget *m_menu;
    QPoint m_targetPos;
    QVector<QGtk3MenuItem *> m_items;
};

QT_END_NAMESPACE

#endif // QGTK3MENU_H
