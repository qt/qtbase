/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2014 Martin Graesslin <mgraesslin@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
