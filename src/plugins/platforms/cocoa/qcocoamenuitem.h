/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QCOCOAMENUITEM_H
#define QCOCOAMENUITEM_H

#include <qpa/qplatformmenu.h>
#include <QtGui/QImage>

//#define QT_COCOA_ENABLE_MENU_DEBUG

Q_FORWARD_DECLARE_OBJC_CLASS(NSMenuItem);
Q_FORWARD_DECLARE_OBJC_CLASS(NSMenu);
Q_FORWARD_DECLARE_OBJC_CLASS(NSObject);
Q_FORWARD_DECLARE_OBJC_CLASS(NSView);

QT_BEGIN_NAMESPACE

class QCocoaMenu;

class QCocoaMenuObject
{
public:
    void setMenuParent(QObject *o)
    {
        parent = o;
    }

    QObject *menuParent() const
    {
        return parent;
    }

private:
    QPointer<QObject> parent;
};

class QCocoaMenuItem : public QPlatformMenuItem, public QCocoaMenuObject
{
public:
    QCocoaMenuItem();
    ~QCocoaMenuItem();

    void setTag(quintptr tag) Q_DECL_OVERRIDE
        { m_tag = tag; }
    quintptr tag() const Q_DECL_OVERRIDE
        { return m_tag; }

    void setText(const QString &text) Q_DECL_OVERRIDE;
    void setIcon(const QIcon &icon) Q_DECL_OVERRIDE;
    void setMenu(QPlatformMenu *menu) Q_DECL_OVERRIDE;
    void setVisible(bool isVisible) Q_DECL_OVERRIDE;
    void setIsSeparator(bool isSeparator) Q_DECL_OVERRIDE;
    void setFont(const QFont &font) Q_DECL_OVERRIDE;
    void setRole(MenuRole role) Q_DECL_OVERRIDE;
#ifndef QT_NO_SHORTCUT
    void setShortcut(const QKeySequence& shortcut) Q_DECL_OVERRIDE;
#endif
    void setCheckable(bool checkable) Q_DECL_OVERRIDE { Q_UNUSED(checkable) }
    void setChecked(bool isChecked) Q_DECL_OVERRIDE;
    void setEnabled(bool isEnabled) Q_DECL_OVERRIDE;
    void setIconSize(int size) Q_DECL_OVERRIDE;

    void setNativeContents(WId item) Q_DECL_OVERRIDE;

    inline QString text() const { return m_text; }
    inline NSMenuItem * nsItem() { return m_native; }
    NSMenuItem *sync();

    void syncMerged();
    void setParentEnabled(bool enabled);

    inline bool isMerged() const { return m_merged; }
    inline bool isEnabled() const { return m_enabled && m_parentEnabled; }
    inline bool isSeparator() const { return m_isSeparator; }

    QCocoaMenu *menu() const { return m_menu; }
    MenuRole effectiveRole() const;

private:
    QString mergeText();
    QKeySequence mergeAccel();

    NSMenuItem *m_native;
    NSView *m_itemView;
    QString m_text;
    QIcon m_icon;
    QPointer<QCocoaMenu> m_menu;
    QFont m_font;
    MenuRole m_role;
    MenuRole m_detectedRole;
#ifndef QT_NO_SHORTCUT
    QKeySequence m_shortcut;
#endif
    quintptr m_tag;
    int m_iconSize;
    bool m_textSynced:1;
    bool m_isVisible:1;
    bool m_enabled:1;
    bool m_parentEnabled:1;
    bool m_isSeparator:1;
    bool m_checked:1;
    bool m_merged:1;
};

QT_END_NAMESPACE

#endif
