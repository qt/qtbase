// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QIBUSTYPES_H
#define QIBUSTYPES_H

#include <qlist.h>
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

    void serializeTo(QDBusArgument &argument) const;
    void deserializeFrom(const QDBusArgument &argument);

    QString name;
    QHash<QString, QDBusArgument>  attachments;
};

class QIBusAttribute : private QIBusSerializable
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

    QTextCharFormat format() const;

    void serializeTo(QDBusArgument &argument) const;
    void deserializeFrom(const QDBusArgument &argument);

    Type type;
    quint32 value;
    quint32 start;
    quint32 end;
};
Q_DECLARE_TYPEINFO(QIBusAttribute, Q_RELOCATABLE_TYPE);

class QIBusAttributeList : private QIBusSerializable
{
public:
    QIBusAttributeList();

    QList<QInputMethodEvent::Attribute> imAttributes() const;

    void serializeTo(QDBusArgument &argument) const;
    void deserializeFrom(const QDBusArgument &argument);

    QList<QIBusAttribute> attributes;
};
Q_DECLARE_TYPEINFO(QIBusAttributeList, Q_RELOCATABLE_TYPE);

class QIBusText : private QIBusSerializable
{
public:
    QIBusText();

    void serializeTo(QDBusArgument &argument) const;
    void deserializeFrom(const QDBusArgument &argument);

    QString text;
    QIBusAttributeList attributes;
};
Q_DECLARE_TYPEINFO(QIBusText, Q_RELOCATABLE_TYPE);

class QIBusEngineDesc : private QIBusSerializable
{
public:
    QIBusEngineDesc();

    void serializeTo(QDBusArgument &argument) const;
    void deserializeFrom(const QDBusArgument &argument);

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
Q_DECLARE_TYPEINFO(QIBusEngineDesc, Q_RELOCATABLE_TYPE);

inline QDBusArgument &operator<<(QDBusArgument &argument, const QIBusAttribute &attribute)
{ attribute.serializeTo(argument); return argument; }
inline const QDBusArgument &operator>>(const QDBusArgument &argument, QIBusAttribute &attribute)
{ attribute.deserializeFrom(argument); return argument; }

inline QDBusArgument &operator<<(QDBusArgument &argument, const QIBusAttributeList &attributeList)
{ attributeList.serializeTo(argument); return argument; }
inline const QDBusArgument &operator>>(const QDBusArgument &argument, QIBusAttributeList &attributeList)
{ attributeList.deserializeFrom(argument); return argument; }

inline QDBusArgument &operator<<(QDBusArgument &argument, const QIBusText &text)
{ text.serializeTo(argument); return argument; }
inline const QDBusArgument &operator>>(const QDBusArgument &argument, QIBusText &text)
{ text.deserializeFrom(argument); return argument; }

inline QDBusArgument &operator<<(QDBusArgument &argument, const QIBusEngineDesc &desc)
{ desc.serializeTo(argument); return argument; }
inline const QDBusArgument &operator>>(const QDBusArgument &argument, QIBusEngineDesc &desc)
{ desc.deserializeFrom(argument); return argument; }

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QIBusAttribute)
Q_DECLARE_METATYPE(QIBusAttributeList)
Q_DECLARE_METATYPE(QIBusText)
Q_DECLARE_METATYPE(QIBusEngineDesc)

#endif
