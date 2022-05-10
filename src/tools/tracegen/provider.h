// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
        enum BackendType {
            Array,
            Sequence,
            Integer,
            IntegerHex,
            Float,
            String,
            Pointer,
            QtString,
            QtByteArray,
            QtUrl,
            QtRect,
            Unknown
        };

        BackendType backendType;
        QString paramType;
        QString name;
        int arrayLen;
        QString seqLen;
    };

    QString name;
    QList<Argument> args;
    QList<Field> fields;
};

struct Provider
{
    QString name;
    QList<Tracepoint> tracepoints;
    QStringList prefixText;
};

Provider parseProvider(const QString &filename);

Q_DECLARE_TYPEINFO(Tracepoint::Argument, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(Tracepoint::Field, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(Tracepoint, Q_RELOCATABLE_TYPE);

#endif // PROVIDER_H
