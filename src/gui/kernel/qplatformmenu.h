/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QPLATFORMMENU_H
#define QPLATFORMMENU_H
//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtCore/qglobal.h>
#include <QtCore/qpointer.h>
#include <QtGui/QFont>
#include <QtGui/QKeySequence>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QPlatformMenu;
class Q_GUI_EXPORT QPlatformMenuItem : public QObject
{
Q_OBJECT
public:
    // copied from, and must stay in sync with, QAction menu roles.
    enum MenuRole { NoRole = 0, TextHeuristicRole, ApplicationSpecificRole, AboutQtRole,
                    AboutRole, PreferencesRole, QuitRole };

    virtual void setTag(quintptr tag);
    virtual quintptr tag() const;

    virtual void setText(const QString &text);
    virtual void setIcon(const QImage &icon);
    virtual void setMenu(QPlatformMenu *menu);
    virtual void setVisible(bool isVisible);
    virtual void setIsSeparator(bool isSeparator);
    virtual void setFont(const QFont &font);
    virtual void setRole(MenuRole role);
    virtual void setChecked(bool isChecked);
    virtual void setShortcut(const QKeySequence& shortcut);
    virtual void setEnabled(bool enabled);
Q_SIGNALS:
    void activated();
    void hovered();
};

class Q_GUI_EXPORT QPlatformMenu : public QPlatformMenuItem // Some (but not all) of the PlatformMenuItem API applies to QPlatformMenu as well.
{
Q_OBJECT
public:
    virtual void insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before);
    virtual void removeMenuItem(QPlatformMenuItem *menuItem);
    virtual void syncMenuItem(QPlatformMenuItem *menuItem);
    virtual void syncSeparatorsCollapsible(bool enable);

    virtual QPlatformMenuItem *menuItemAt(int position) const;
    virtual QPlatformMenuItem *menuItemForTag(quintptr tag) const;
Q_SIGNALS:
    void aboutToShow();
    void aboutToHide();
};

class Q_GUI_EXPORT QPlatformMenuBar : public QPlatformMenu
{
Q_OBJECT
public:
    virtual void insertMenu(QPlatformMenu *menu, QPlatformMenu *before);
    virtual void removeMenu(QPlatformMenu *menu);
    virtual void syncMenu(QPlatformMenuItem *menuItem);
    virtual void handleReparent(QWindow *newParentWindow);

    virtual QPlatformMenu *menuForTag(quintptr tag) const;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif

