// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2014 Martin Graesslin <mgraesslin@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformmenu.h"

#include <qpa/qplatformtheme.h>
#include <private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

QPlatformMenuItem::QPlatformMenuItem()
{
    m_tag = reinterpret_cast<quintptr>(this);
}

void QPlatformMenuItem::setTag(quintptr tag)
{
    m_tag = tag;
}

quintptr QPlatformMenuItem::tag() const
{
    return m_tag;
}

QPlatformMenu::QPlatformMenu()
{
    m_tag = reinterpret_cast<quintptr>(this);
}

void QPlatformMenu::setTag(quintptr tag)
{
    m_tag = tag;
}

quintptr QPlatformMenu::tag() const
{
    return m_tag;

}

QPlatformMenuItem *QPlatformMenu::createMenuItem() const
{
    return QGuiApplicationPrivate::platformTheme()->createPlatformMenuItem();
}

QPlatformMenu *QPlatformMenu::createSubMenu() const
{
    return QGuiApplicationPrivate::platformTheme()->createPlatformMenu();
}

QPlatformMenu *QPlatformMenuBar::createMenu() const
{
    return QGuiApplicationPrivate::platformTheme()->createPlatformMenu();
}

QT_END_NAMESPACE

#include "moc_qplatformmenu.cpp"
