/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QIBUSTYPES_H
#define QIBUSTYPES_H

#include <qvector.h>
#include <qevent.h>
#include <QDBusArgument>
#include <QTextCharFormat>
#include <QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qtQpaInputMethods)
Q_DECLARE_LOGGING_CATEGORY(qtQpaInputMethodsSerialize)

class QIBusSerializable
{
public:
    QIBusSerializable();
    virtual ~QIBusSerializable();

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

    QTextCharFormat format() const;

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

    QList<QInputMethodEvent::Attribute> imAttributes() const;

    QVector<QIBusAttribute> attributes;
};

class QIBusText : public QIBusSerializable
{
public:
    QIBusText();
    ~QIBusText();

    QString text;
    QIBusAttributeList attributes;
};

class QIBusEngineDesc : public QIBusSerializable
{
public:
    QIBusEngineDesc();
    ~QIBusEngineDesc();

    QString engine_name;
    QString longname;
    QString description;
    QString language;
    QString license;
    QString author;
    QString icon;
    QString layout;
    unsigned int rank;
    QString hotkeys;
    QString symbol;
    QString setup;
    QString layout_variant;
    QString layout_option;
    QString version;
    QString textdomain;
    QString iconpropkey;
};

QDBusArgument &operator<<(QDBusArgument &argument, const QIBusSerializable &object);
const QDBusArgument &operator>>(const QDBusArgument &argument, QIBusSerializable &object);

QDBusArgument &operator<<(QDBusArgument &argument, const QIBusAttribute &attribute);
const QDBusArgument &operator>>(const QDBusArgument &argument, QIBusAttribute &attribute);

QDBusArgument &operator<<(QDBusArgument &argument, const QIBusAttributeList &attributeList);
const QDBusArgument &operator>>(const QDBusArgument &arg, QIBusAttributeList &attrList);

QDBusArgument &operator<<(QDBusArgument &argument, const QIBusText &text);
const QDBusArgument &operator>>(const QDBusArgument &argument, QIBusText &text);

QDBusArgument &operator<<(QDBusArgument &argument, const QIBusEngineDesc &desc);
const QDBusArgument &operator>>(const QDBusArgument &argument, QIBusEngineDesc &desc);

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QIBusSerializable)
Q_DECLARE_METATYPE(QIBusAttribute)
Q_DECLARE_METATYPE(QIBusAttributeList)
Q_DECLARE_METATYPE(QIBusText)
Q_DECLARE_METATYPE(QIBusEngineDesc)

#endif
