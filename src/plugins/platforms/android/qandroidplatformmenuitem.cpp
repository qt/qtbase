// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidplatformmenuitem.h"
#include "qandroidplatformmenu.h"

QT_BEGIN_NAMESPACE

QAndroidPlatformMenuItem::QAndroidPlatformMenuItem()
{
    m_menu = 0;
    m_isVisible = true;
    m_isSeparator = false;
    m_role = NoRole;
    m_isCheckable = false;
    m_isChecked = false;
    m_isEnabled = true;
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
    Q_UNUSED(font);
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
    Q_UNUSED(shortcut);
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
    Q_UNUSED(size);
}

QT_END_NAMESPACE
