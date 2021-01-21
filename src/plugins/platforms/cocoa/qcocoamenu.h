/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author James Turner <james.turner@kdab.com>
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
