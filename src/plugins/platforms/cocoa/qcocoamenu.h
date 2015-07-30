/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author James Turner <james.turner@kdab.com>
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

#ifndef QCOCOAMENU_H
#define QCOCOAMENU_H

#include <QtCore/QList>
#include <qpa/qplatformmenu.h>
#include "qcocoamenuitem.h"

QT_BEGIN_NAMESPACE

class QCocoaMenuBar;

class QCocoaMenu : public QPlatformMenu
{
public:
    QCocoaMenu();
    ~QCocoaMenu();

    void setTag(quintptr tag) Q_DECL_OVERRIDE
    { m_tag = tag; }
    quintptr tag() const Q_DECL_OVERRIDE
    { return m_tag; }

    void insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before) Q_DECL_OVERRIDE;
    void removeMenuItem(QPlatformMenuItem *menuItem) Q_DECL_OVERRIDE;
    void syncMenuItem(QPlatformMenuItem *menuItem) Q_DECL_OVERRIDE;
    void setEnabled(bool enabled) Q_DECL_OVERRIDE;
    bool isEnabled() const Q_DECL_OVERRIDE;
    void setVisible(bool visible) Q_DECL_OVERRIDE;
    void showPopup(const QWindow *parentWindow, const QRect &targetRect, const QPlatformMenuItem *item) Q_DECL_OVERRIDE;
    void dismiss() Q_DECL_OVERRIDE;

    void syncSeparatorsCollapsible(bool enable) Q_DECL_OVERRIDE;

    void syncModalState(bool modal);

    void setIcon(const QIcon &icon) Q_DECL_OVERRIDE { Q_UNUSED(icon) }

    void setText(const QString &text) Q_DECL_OVERRIDE;
    void setMinimumWidth(int width) Q_DECL_OVERRIDE;
    void setFont(const QFont &font) Q_DECL_OVERRIDE;

    inline NSMenu *nsMenu() const
        { return m_nativeMenu; }
    inline NSMenuItem *nsMenuItem() const
        { return m_nativeItem; }

    inline bool isVisible() const { return m_visible; }

    QPlatformMenuItem *menuItemAt(int position) const Q_DECL_OVERRIDE;
    QPlatformMenuItem *menuItemForTag(quintptr tag) const Q_DECL_OVERRIDE;

    QList<QCocoaMenuItem *> items() const;
    QList<QCocoaMenuItem *> merged() const;
    void setMenuBar(QCocoaMenuBar *menuBar);
    QCocoaMenuBar *menuBar() const;

    void setContainingMenuItem(QCocoaMenuItem *menuItem);
    QCocoaMenuItem *containingMenuItem() const;

private:
    QCocoaMenuItem *itemOrNull(int index) const;
    void insertNative(QCocoaMenuItem *item, QCocoaMenuItem *beforeItem);

    QList<QCocoaMenuItem *> m_menuItems;
    NSMenu *m_nativeMenu;
    NSMenuItem *m_nativeItem;
    NSObject *m_delegate;
    bool m_enabled;
    bool m_visible;
    quintptr m_tag;
    QCocoaMenuBar *m_menuBar;
    QCocoaMenuItem *m_containingMenuItem;
};

QT_END_NAMESPACE

#endif
