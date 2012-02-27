/****************************************************************************
**
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author James Turner <james.turner@kdab.com>
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qplatformmenu.h"

QT_BEGIN_NAMESPACE

void QPlatformMenuItem::setText(const QString &text)
{

}

void QPlatformMenuItem::setIcon(const QImage &icon)
{

}

void QPlatformMenuItem::setMenu(QPlatformMenu *menu)
{

}

void QPlatformMenuItem::setVisible(bool isVisible)
{

}

void QPlatformMenuItem::setIsSeparator(bool isSeparator)
{

}

void QPlatformMenuItem::setFont(const QFont &font)
{

}

void QPlatformMenuItem::setRole(QPlatformMenuItem::MenuRole role)
{

}

void QPlatformMenuItem::setChecked(bool isChecked)
{

}

void QPlatformMenuItem::setShortcut(const QKeySequence& shortcut)
{

}

void QPlatformMenuItem::setEnabled(bool enabled)
{

}

void QPlatformMenuItem::setTag(quintptr tag)
{
}

quintptr QPlatformMenuItem::tag() const
{
    return 0;
}

void QPlatformMenu::insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem* before)
{

}

void QPlatformMenu::removeMenuItem(QPlatformMenuItem *menuItem)
{

}

void QPlatformMenu::syncMenuItem(QPlatformMenuItem *menuItem)
{

}

void QPlatformMenu::syncSeparatorsCollapsible(bool enable)
{

}

QPlatformMenuItem* QPlatformMenu::menuItemAt(int position) const
{
    return 0;
}

QPlatformMenuItem* QPlatformMenu::menuItemForTag(quintptr tag) const
{
    return 0;
}

void QPlatformMenuBar::insertMenu(QPlatformMenu *menuItem, QPlatformMenu* before)
{

}

void QPlatformMenuBar::removeMenu(QPlatformMenu *menuItem)
{

}

void QPlatformMenuBar::syncMenu(QPlatformMenuItem *menuItem)
{

}

void QPlatformMenuBar::handleReparent(QWindow *newParentWindow)
{

}

QPlatformMenu *QPlatformMenuBar::menuForTag(quintptr tag) const
{
    Q_UNUSED(tag);
    return 0;
}

QT_END_NAMESPACE
