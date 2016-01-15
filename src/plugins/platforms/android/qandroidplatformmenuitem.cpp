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

#include "qandroidplatformmenuitem.h"
#include "qandroidplatformmenu.h"

QT_BEGIN_NAMESPACE

QAndroidPlatformMenuItem::QAndroidPlatformMenuItem()
{
    m_tag = reinterpret_cast<quintptr>(this); // QMenu will overwrite this later, but we need a unique ID for QtQuick
    m_menu = 0;
    m_isVisible = true;
    m_isSeparator = false;
    m_role = NoRole;
    m_isCheckable = false;
    m_isChecked = false;
    m_isEnabled = true;
}

void QAndroidPlatformMenuItem::setTag(quintptr tag)
{
    m_tag = tag;
}

quintptr QAndroidPlatformMenuItem::tag() const
{
    return m_tag;
}

void QAndroidPlatformMenuItem::setText(const QString &text)
{
    m_text = text;
    if (m_menu)
        m_menu->setText(m_text);
}

QString QAndroidPlatformMenuItem::text() const
{
    return m_text;
}

void QAndroidPlatformMenuItem::setIcon(const QIcon &icon)
{
    m_icon = icon;
    if (m_menu)
        m_menu->setIcon(m_icon);
}

QIcon QAndroidPlatformMenuItem::icon() const
{
    return m_icon;
}

void QAndroidPlatformMenuItem::setMenu(QPlatformMenu *menu)
{
    m_menu = static_cast<QAndroidPlatformMenu *>(menu);
    if (!m_menu)
        return;

    m_menu->setText(m_text);
    m_menu->setIcon(m_icon);
    m_menu->setVisible(m_isVisible);
    m_menu->setEnabled(m_isEnabled);
}

QAndroidPlatformMenu *QAndroidPlatformMenuItem::menu() const
{
    return m_menu;
}

void QAndroidPlatformMenuItem::setVisible(bool isVisible)
{
    m_isVisible = isVisible;
    if (m_menu)
        m_menu->setVisible(m_isVisible);
}

bool QAndroidPlatformMenuItem::isVisible() const
{
    return m_isVisible;
}

void QAndroidPlatformMenuItem::setIsSeparator(bool isSeparator)
{
    m_isSeparator = isSeparator;
}

bool QAndroidPlatformMenuItem::isSeparator() const
{
    return m_isSeparator;
}

void QAndroidPlatformMenuItem::setFont(const QFont &font)
{
    Q_UNUSED(font)
}

void QAndroidPlatformMenuItem::setRole(QPlatformMenuItem::MenuRole role)
{
    m_role = role;
}

QPlatformMenuItem::MenuRole QAndroidPlatformMenuItem::role() const
{
    return m_role;
}

void QAndroidPlatformMenuItem::setCheckable(bool checkable)
{
    m_isCheckable = checkable;
}

bool QAndroidPlatformMenuItem::isCheckable() const
{
    return m_isCheckable;
}

void QAndroidPlatformMenuItem::setChecked(bool isChecked)
{
    m_isChecked = isChecked;
}

bool QAndroidPlatformMenuItem::isChecked() const
{
    return m_isChecked;
}

void QAndroidPlatformMenuItem::setShortcut(const QKeySequence &shortcut)
{
    Q_UNUSED(shortcut)
}

void QAndroidPlatformMenuItem::setEnabled(bool enabled)
{
    m_isEnabled = enabled;
    if (m_menu)
        m_menu->setEnabled(m_isEnabled);
}

bool QAndroidPlatformMenuItem::isEnabled() const
{
    return m_isEnabled;
}

void QAndroidPlatformMenuItem::setIconSize(int size)
{
    Q_UNUSED(size)
}

QT_END_NAMESPACE
