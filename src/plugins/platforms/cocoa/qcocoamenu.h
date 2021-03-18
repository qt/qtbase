/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author James Turner <james.turner@kdab.com>
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

#ifndef QCOCOAMENU_H
#define QCOCOAMENU_H

#include <QtCore/QList>
#include <qpa/qplatformmenu.h>
#include "qcocoamenuitem.h"
#include "qcocoansmenu.h"

QT_BEGIN_NAMESPACE

class QCocoaMenuBar;

class QCocoaMenu : public QPlatformMenu, public QCocoaMenuObject
{
public:
    QCocoaMenu();
    ~QCocoaMenu();

    void insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before) override;
    void removeMenuItem(QPlatformMenuItem *menuItem) override;
    void syncMenuItem(QPlatformMenuItem *menuItem) override;
    void setEnabled(bool enabled) override;
    bool isEnabled() const override;
    void setVisible(bool visible) override;
    void showPopup(const QWindow *parentWindow, const QRect &targetRect, const QPlatformMenuItem *item) override;
    void dismiss() override;

    void syncSeparatorsCollapsible(bool enable) override;

    void propagateEnabledState(bool enabled);

    void setIcon(const QIcon &icon) override { Q_UNUSED(icon) }

    void setText(const QString &text) override;
    void setMinimumWidth(int width) override;
    void setFont(const QFont &font) override;

    NSMenu *nsMenu() const;

    inline bool isVisible() const { return m_visible; }

    QPlatformMenuItem *menuItemAt(int position) const override;
    QPlatformMenuItem *menuItemForTag(quintptr tag) const override;

    QList<QCocoaMenuItem *> items() const;
    QList<QCocoaMenuItem *> merged() const;

    void setAttachedItem(NSMenuItem *item);
    NSMenuItem *attachedItem() const;

    bool isOpen() const;
    void setIsOpen(bool isOpen);

    bool isAboutToShow() const;
    void setIsAboutToShow(bool isAbout);

    void timerEvent(QTimerEvent *e) override;

    void syncMenuItem_helper(QPlatformMenuItem *menuItem, bool menubarUpdate);

    void setItemTargetAction(QCocoaMenuItem *item) const;

private:
    QCocoaMenuItem *itemOrNull(int index) const;
    void insertNative(QCocoaMenuItem *item, QCocoaMenuItem *beforeItem);
    void scheduleUpdate();

    QList<QCocoaMenuItem *> m_menuItems;
    QCocoaNSMenu *m_nativeMenu;
    NSMenuItem *m_attachedItem;
    int m_updateTimer;
    bool m_enabled:1;
    bool m_parentEnabled:1;
    bool m_visible:1;
    bool m_isOpen:1;
    bool m_isAboutToShow:1;
};

QT_END_NAMESPACE

#endif
