/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QACCESSIBLEOBJECT_H
#define QACCESSIBLEOBJECT_H

#include <QtGui/qaccessible.h>
#include <QtGui/qaccessible2.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_ACCESSIBILITY

class QAccessibleObjectPrivate;
class QObject;

class Q_GUI_EXPORT QAccessibleObject : public QAccessibleInterface
{
public:
    explicit QAccessibleObject(QObject *object);

    bool isValid() const;
    QObject *object() const;

    // properties
    QRect rect() const;
    void setText(QAccessible::Text t, const QString &text);
    QAccessibleInterface *childAt(int x, int y) const;

protected:
    virtual ~QAccessibleObject();

private:
    QAccessibleObjectPrivate *d;
    Q_DISABLE_COPY(QAccessibleObject)
};

class Q_GUI_EXPORT QAccessibleApplication : public QAccessibleObject
{
public:
    QAccessibleApplication();

    QWindow *window() const;
    // relations
    int childCount() const;
    int indexOfChild(const QAccessibleInterface*) const;
    QAccessible::Relation relationTo(const QAccessibleInterface *other) const;

    // navigation
    QAccessibleInterface *parent() const;
    QAccessibleInterface *child(int index) const;
    int navigate(QAccessible::RelationFlag, int, QAccessibleInterface **) const;

    // properties and state
    QString text(QAccessible::Text t) const;
    QAccessible::Role role() const;
    QAccessible::State state() const;
};

#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE

QT_END_HEADER

#endif // QACCESSIBLEOBJECT_H
