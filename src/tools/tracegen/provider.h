// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef PROVIDER_H
#define PROVIDER_H

#include <qlist.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtypeinfo.h>

struct Tracepoint
{
    struct Argument
    {
        QString type;
        QString name;
        int arrayLen;
    };

    struct Field
    {
        enum Type {
            Sequence,
            Boolean,
            Integer,
            IntegerHex,
            Float,
            String,
            Pointer,
            QtString,
            QtByteArray,
            QtUrl,
            QtRect,
            QtSize,
            QtRectF,
            QtSizeF,
            EnumeratedType,
            FlagType,
            Unknown
        };
        Type backendType;
        QString paramType;
        QString name;
        int arrayLen;
        int enumValueSize;
        QString seqLen;
    };

    QString name;
    QList<Argument> args;
    QList<Field> fields;
};

struct TraceEnum {
    QString name;
    struct EnumValue {
        QString name;
        int value;
        int range;
    };
    QList<EnumValue> values;
    int valueSize;
};

struct TraceFlags {
    QString name;
    struct FlagValue {
        QString name;
        int value;
    };
    QList<FlagValue> values;
};

Q_DECLARE_TYPEINFO(TraceEnum, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(TraceFlags, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(Tracepoint::Argument, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(Tracepoint::Field, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(Tracepoint, Q_RELOCATABLE_TYPE);

struct Provider
{
    QString name;
    QList<Tracepoint> tracepoints;
    QStringList prefixText;
    QList<TraceEnum> enumerations;
    QList<TraceFlags> flags;
};

Provider parseProvider(const QString &filename);

#endif // PROVIDER_H
