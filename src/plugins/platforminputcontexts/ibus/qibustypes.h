/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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
#ifndef QIBUSTYPES_H
#define QIBUSTYPES_H

#include <qvector.h>
#include <qevent.h>

QT_BEGIN_NAMESPACE

class QDBusArgument;

class QIBusSerializable
{
public:
    QIBusSerializable();
    virtual ~QIBusSerializable();

    virtual void fromDBusArgument(const QDBusArgument &arg);

    QString name;
    QHash<QString, QDBusArgument>  attachments;
};

class QIBusAttribute : public QIBusSerializable
{
public:
    enum Type {
        Invalid = 0,
        Underline = 1,
        Foreground = 2,
        Background = 3,
    };

    enum Underline {
        UnderlineNone = 0,
        UnderlineSingle  = 1,
        UnderlineDouble  = 2,
        UnderlineLow = 3,
        UnderlineError = 4,
    };

    QIBusAttribute();
    ~QIBusAttribute();

    void fromDBusArgument(const QDBusArgument &arg);
    QTextFormat format() const;

    Type type;
    quint32 value;
    quint32 start;
    quint32 end;
};

class QIBusAttributeList : public QIBusSerializable
{
public:
    QIBusAttributeList();
    ~QIBusAttributeList();

    void fromDBusArgument(const QDBusArgument &arg);

    QList<QInputMethodEvent::Attribute> imAttributes() const;

    QVector<QIBusAttribute> attributes;
};

class QIBusText : public QIBusSerializable
{
public:
    QIBusText();
    ~QIBusText();

    void fromDBusArgument(const QDBusArgument &arg);

    QString text;
    QIBusAttributeList attributes;
};

QT_END_NAMESPACE

#endif
