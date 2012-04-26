/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QtCore/qglobal.h>
#include <QtCore/qpointer.h>
#include <QtWidgets/qaction.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QMenuPrivate;
class Q_WIDGETS_EXPORT QPlatformMenuAction
{
public:
    virtual ~QPlatformMenuAction();
    QPointer<QAction> action;
};

class Q_WIDGETS_EXPORT QPlatformMenu {
public:
    QPlatformMenu();
    virtual ~QPlatformMenu();

    virtual bool merged(const QAction *action) const = 0;

    virtual void addAction(QAction *action, QAction *before) = 0;
    virtual void removeAction(QAction *action) = 0;
    virtual void syncAction(QAction *action) = 0;

    virtual void setMenuEnabled(bool enable);
    virtual void syncSeparatorsCollapsible(bool enable);
};

class Q_WIDGETS_EXPORT QPlatformMenuBar {
public:
    QPlatformMenuBar();
    virtual ~QPlatformMenuBar();

    virtual void addAction(QAction *action, QAction *before = 0) = 0;
    virtual void syncAction(QAction *action) = 0;
    virtual void removeAction(QAction *action) = 0;

    virtual void handleReparent(QWidget *newParent);
};

QT_END_NAMESPACE

QT_END_HEADER

#endif

