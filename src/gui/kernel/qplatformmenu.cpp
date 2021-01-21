/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2014 Martin Graesslin <mgraesslin@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
