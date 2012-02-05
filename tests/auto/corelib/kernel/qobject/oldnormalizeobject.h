/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtTest module of the Qt Toolkit.
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

#ifndef OLDNORMALIZEOBJECT_H
#define OLDNORMALIZEOBJECT_H

#include <QObject>

struct Struct;
class Class;
template <typename T> class Template;

// An object with old moc output that incorrectly normalizes 'T<C> const &' in the function
// signatures
class OldNormalizeObject : public QObject
{
    /* tmake ignore Q_OBJECT */
    Q_OBJECT

signals:
    void typeRefSignal(Template<Class &> &ref);
    void constTypeRefSignal(const Template<const Class &> &ref);
    void typeConstRefSignal(Template<Class const &> const &ref);

public slots:
    void typeRefSlot(Template<Class &> &) {}
    void constTypeRefSlot(const Template<const Class &> &) {}
    void typeConstRefSlot(Template<Class const &> const &) {}
};

#endif // OLDNORMALIZEOBJECT_H
