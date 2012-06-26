/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author James Turner <james.turner@kdab.com>
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCOCOAMENUITEM_H
#define QCOCOAMENUITEM_H

#include <qpa/qplatformmenu.h>
#include <QtGui/QImage>

//#define QT_COCOA_ENABLE_MENU_DEBUG

@class NSMenuItem;
@class NSMenu;

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QCocoaMenu;

class QCocoaMenuItem : public QPlatformMenuItem
{
public:
    QCocoaMenuItem();
    virtual ~QCocoaMenuItem();

    inline virtual void setTag(quintptr tag)
        { m_tag = tag; }
    inline virtual quintptr tag() const
        { return m_tag; }

    void setText(const QString &text);
    void setIcon(const QIcon &icon);
    void setMenu(QPlatformMenu *menu);
    void setVisible(bool isVisible);
    void setIsSeparator(bool isSeparator);
    void setFont(const QFont &font);
    void setRole(MenuRole role);
    void setShortcut(const QKeySequence& shortcut);
    void setChecked(bool isChecked);
    void setEnabled(bool isEnabled);

    inline QString text() const { return m_text; }
    inline NSMenuItem * nsItem() { return m_native; }
    NSMenuItem *sync();

    void syncMerged();
    void syncModalState(bool modal);

    inline bool isMerged() const { return m_merged; }
    inline bool isEnabled() const { return m_enabled; }
    inline bool isSeparator() const { return m_isSeparator; }

    QCocoaMenu *menu() const { return m_menu; }
private:
    QString mergeText();
    QKeySequence mergeAccel();

    NSMenuItem *m_native;
    QString m_text;
    QIcon m_icon;
    QCocoaMenu *m_menu;
    bool m_isVisible;
    bool m_enabled;
    bool m_isSeparator;
    QFont m_font;
    MenuRole m_role;
    QKeySequence m_shortcut;
    bool m_checked;
    bool m_merged;
    quintptr m_tag;
};

QT_END_HEADER

QT_END_NAMESPACE

#endif
