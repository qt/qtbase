/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
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

#ifndef QANDROIDPLATFORMMENUITEM_H
#define QANDROIDPLATFORMMENUITEM_H
#include <qpa/qplatformmenu.h>

QT_BEGIN_NAMESPACE

class QAndroidPlatformMenu;

class QAndroidPlatformMenuItem: public QPlatformMenuItem
{
public:
    QAndroidPlatformMenuItem();
    void setTag(quintptr tag) override;
    quintptr tag() const override;

    void setText(const QString &text) override;
    QString text() const;

    void setIcon(const QIcon &icon) override;
    QIcon icon() const;

    void setMenu(QPlatformMenu *menu) override;
    QAndroidPlatformMenu *menu() const;

    void setVisible(bool isVisible) override;
    bool isVisible() const;

    void setIsSeparator(bool isSeparator) override;
    bool isSeparator() const;

    void setFont(const QFont &font) override;

    void setRole(MenuRole role) override;
    MenuRole role() const;

    void setCheckable(bool checkable) override;
    bool isCheckable() const;

    void setChecked(bool isChecked) override;
    bool isChecked() const;

    void setShortcut(const QKeySequence &shortcut) override;

    void setEnabled(bool enabled) override;
    bool isEnabled() const;

    void setIconSize(int size) override;

private:
    quintptr m_tag;
    QString m_text;
    QIcon m_icon;
    QAndroidPlatformMenu *m_menu;
    bool m_isVisible;
    bool m_isSeparator;
    MenuRole m_role;
    bool m_isCheckable;
    bool m_isChecked;
    bool m_isEnabled;
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMMENUITEM_H
