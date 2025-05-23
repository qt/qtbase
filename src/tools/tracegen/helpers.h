// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef HELPERS_H
#define HELPERS_H

#include "provider.h"

#include <qlist.h>
#include <qstring.h>
#include <qtextstream.h>

enum ParamType {
    LTTNG,
    ETW,
    CTF
};

QString typeToTypeName(const QString &type);
QString includeGuard(const QString &filename);
QString formatFunctionSignature(const QList<Tracepoint::Argument> &args);
QString formatParameterList(const Provider &provider, const QList<Tracepoint::Argument> &args, const QList<Tracepoint::Field> &fields, ParamType type);

void writeCommonPrologue(QTextStream &stream);

template <typename T>
static QString aggregateListValues(int value, const QList<T> &list)
{
    QStringList values;
    for (const T &l : list) {
        if (l.value == value)
            values << l.name;
    }
    return values.join(QLatin1Char('_'));
}

#endif // HELPERS_H
