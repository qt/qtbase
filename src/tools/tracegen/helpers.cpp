// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "helpers.h"
#include <qdebug.h>

using namespace Qt::StringLiterals;

QString includeGuard(const QString &filename)
{
    QString guard = filename.toUpper();

    for (int i = 0; i < guard.size(); ++i) {
        if (!guard.at(i).isLetterOrNumber())
            guard[i] = u'_';
    }

    return guard;
}

template<typename T>
static QString joinArguments(const QList<Tracepoint::Argument> &args, T joinFunction)
{
    QString ret;
    bool first = true;

    for (const Tracepoint::Argument &arg : args) {
        if (!first)
            ret += ", "_L1;

        ret += joinFunction(arg);

        first = false;
    }

    return ret;
}

QString formatFunctionSignature(const QList<Tracepoint::Argument> &args)
{
    return joinArguments(args, [](const Tracepoint::Argument &arg) {
            return QStringLiteral("%1 %2").arg(arg.type).arg(arg.name);
    });
}

QString formatParameterList(const QList<Tracepoint::Argument> &args, ParamType type)
{
    if (type == LTTNG) {
        QString ret;

        for (const Tracepoint::Argument &arg : args)
            ret += ", "_L1 + arg.name;

        return ret;
    }

    return joinArguments(args, [](const Tracepoint::Argument &arg) { return arg.name; });
}
