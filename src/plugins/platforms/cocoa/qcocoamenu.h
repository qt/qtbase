/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author James Turner <james.turner@kdab.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCOCOAMENU_H
#define QCOCOAMENU_H

#include <QtCore/QList>
#include <qpa/qplatformmenu.h>
#include "qcocoamenuitem.h"

@class NSMenuItem;
@class NSMenu;
@class NSObject;

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QCocoaMenu : public QPlatformMenu
{
public:
    QCocoaMenu();
    ~QCocoaMenu();

    inline virtual void setTag(quintptr tag)
        { m_tag = tag; }
    inline virtual quintptr tag() const
        { return m_tag; }

    void insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before);
    void removeMenuItem(QPlatformMenuItem *menuItem);
    void syncMenuItem(QPlatformMenuItem *menuItem);
    void setEnabled(bool enabled);
    void setVisible(bool visible);
    void showPopup(const QWindow *parentWindow, QPoint pos, const QPlatformMenuItem *item);

    void syncSeparatorsCollapsible(bool enable);

    void syncModalState(bool modal);

    void setText(const QString &text);
    void setMinimumWidth(int width);
    void setFont(const QFont &font);

    void setParentItem(QCocoaMenuItem* item);

    inline NSMenu *nsMenu() const
        { return m_nativeMenu; }
    inline NSMenuItem *nsMenuItem() const
        { return m_nativeItem; }

    virtual QPlatformMenuItem *menuItemAt(int position) const;
    virtual QPlatformMenuItem *menuItemForTag(quintptr tag) const;

    QList<QCocoaMenuItem *> merged() const;
private:
    QCocoaMenuItem *itemOrNull(int index) const;
    void insertNative(QCocoaMenuItem *item, QCocoaMenuItem *beforeItem);

    QList<QCocoaMenuItem *> m_menuItems;
    NSMenu *m_nativeMenu;
    NSMenuItem *m_nativeItem;
    NSObject *m_delegate;
    bool m_enabled;
    quintptr m_tag;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif
