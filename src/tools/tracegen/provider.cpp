// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "provider.h"
#include "panic.h"

#include <qfile.h>
#include <qfileinfo.h>
#include <qtextstream.h>
#include <qregularexpression.h>
#include <qstring.h>

using namespace Qt::StringLiterals;

#ifdef TRACEGEN_DEBUG
#include <qdebug.h>

static void dumpTracepoint(const Tracepoint &t)
{
    qDebug() << "=== BEGIN TRACEPOINT ===";
    qDebug() << "name:" << t.name;
    qDebug() << "ARGS\n";

    int j = 0;

    for (auto i = t.args.constBegin(); i != t.args.constEnd(); ++i) {
        qDebug() << "ARG[" << j << "] type:" << i->type;
        qDebug() << "ARG[" << j << "] name:" << i->name;
        qDebug() << "ARG[" << j << "] arrayLen:" << i->arrayLen;
        ++j;
    }

    qDebug() << "\nFIELDS\n";

    j = 0;

    for (auto i = t.fields.constBegin(); i != t.fields.constEnd(); ++i) {
        qDebug() << "FIELD[" << j << "] backend_type" << static_cast<int>(i->backendType);
        qDebug() << "FIELD[" << j << "] param_type" << i->paramType;
        qDebug() << "FIELD[" << j << "] name" << i->name;
        qDebug() << "FIELD[" << j << "] arrayLen" << i->arrayLen;
        qDebug() << "FIELD[" << j << "] seqLen" << i->seqLen;
        ++j;
    }

    qDebug() << "=== END TRACEPOINT ===\n";

}
#endif

static inline int arrayLength(const QString &rawType)
{
    /* matches the length of an ordinary array type
     * Ex: foo[10] yields '10'
     */
    static const QRegularExpression rx(QStringLiteral("\\[([0-9]+)\\]"));

    auto match = rx.match(rawType);
    if (!match.hasMatch())
        return 0;

    return match.captured(1).toInt();
}

static inline QString sequenceLength(const QString &rawType)
{
    /* matches the identifier pointing to length of a CTF sequence type, which is
     * a dynamic sized array.
     * Ex: in qcoreapplication_foo(const char[len], some_string, unsigned int, * len)
     * it will match the 'len' part of 'const char[len]')
     */
    static const QRegularExpression rx(QStringLiteral("\\[([A-Za-z_][A-Za-z_0-9]*)\\]"));

    auto match = rx.match(rawType);
    if (!match.hasMatch())
        return QString();

    return match.captured(1);
}

static QString decayArrayToPointer(QString type)
{
    /* decays an array to a pointer, i.e., if 'type' holds int[10],
     * this function returns 'int *'
     */
    static QRegularExpression rx(QStringLiteral("\\[([^\\]]+)\\]"));

    return type.replace(rx, QStringLiteral("*"));
}

static QString removeBraces(QString type)
{
    static const QRegularExpression rx(QStringLiteral("\\[.*\\]"));

    return type.remove(rx);
}

static Tracepoint::Field::BackendType backendType(QString rawType)
{
    static const struct {
        const char *type;
        Tracepoint::Field::BackendType backendType;
    } typeTable[] = {
        { "bool",                   Tracepoint::Field::Integer },
        { "short_int",              Tracepoint::Field::Integer },
        { "signed_short",           Tracepoint::Field::Integer },
        { "signed_short_int",       Tracepoint::Field::Integer },
        { "unsigned_short",         Tracepoint::Field::Integer },
        { "unsigned_short_int",     Tracepoint::Field::Integer },
        { "int",                    Tracepoint::Field::Integer },
        { "signed",                 Tracepoint::Field::Integer },
        { "signed_int",             Tracepoint::Field::Integer },
        { "unsigned",               Tracepoint::Field::Integer },
        { "unsigned_int",           Tracepoint::Field::Integer },
        { "long",                   Tracepoint::Field::Integer },
        { "long_int",               Tracepoint::Field::Integer },
        { "signed_long",            Tracepoint::Field::Integer },
        { "signed_long_int",        Tracepoint::Field::Integer },
        { "unsigned_long",          Tracepoint::Field::Integer },
        { "unsigned_long_int",      Tracepoint::Field::Integer },
        { "long_long",              Tracepoint::Field::Integer },
        { "long_long_int",          Tracepoint::Field::Integer },
        { "signed_long_long",       Tracepoint::Field::Integer },
        { "signed_long_long_int",   Tracepoint::Field::Integer },
        { "unsigned_long_long",     Tracepoint::Field::Integer },
        { "char",                   Tracepoint::Field::Integer },
        { "intptr_t",               Tracepoint::Field::IntegerHex },
        { "uintptr_t",              Tracepoint::Field::IntegerHex },
        { "std::intptr_t",          Tracepoint::Field::IntegerHex },
        { "std::uintptr_t",         Tracepoint::Field::IntegerHex },
        { "float",                  Tracepoint::Field::Float },
        { "double",                 Tracepoint::Field::Float },
        { "long_double",            Tracepoint::Field::Float },
        { "QString",                Tracepoint::Field::QtString },
        { "QByteArray",             Tracepoint::Field::QtByteArray },
        { "QUrl",                   Tracepoint::Field::QtUrl },
        { "QRect",                  Tracepoint::Field::QtRect }
    };

    auto backendType = [](const QString &rawType) {
        static const size_t tableSize = sizeof (typeTable) / sizeof (typeTable[0]);

        for (size_t i = 0; i < tableSize; ++i) {
            if (rawType == QLatin1StringView(typeTable[i].type))
                return typeTable[i].backendType;
        }

        return Tracepoint::Field::Unknown;
    };

    if (arrayLength(rawType) > 0)
        return Tracepoint::Field::Array;

    if (!sequenceLength(rawType).isNull())
        return Tracepoint::Field::Sequence;

    static const QRegularExpression constMatch(QStringLiteral("\\bconst\\b"));
    rawType.remove(constMatch);
    rawType.remove(u'&');

    static const QRegularExpression ptrMatch(QStringLiteral("\\s*\\*\\s*"));
    rawType.replace(ptrMatch, QStringLiteral("_ptr"));
    rawType = rawType.trimmed();
    rawType.replace(QStringLiteral(" "), QStringLiteral("_"));

    if (rawType == "char_ptr"_L1)
        return Tracepoint::Field::String;

    if (rawType.endsWith("_ptr"_L1))
        return Tracepoint::Field::Pointer;

    return backendType(rawType);
}

static Tracepoint parseTracepoint(const QString &name, const QStringList &args,
        const QString &fileName, const int lineNumber)
{
    Tracepoint t;
    t.name = name;

    if (args.isEmpty())
        return t;

    auto i = args.constBegin();
    auto end = args.constEnd();
    int argc = 0;

    static const QRegularExpression rx(QStringLiteral("^(.*)\\b([A-Za-z_][A-Za-z0-9_]*)$"));

    while (i != end) {
        auto match = rx.match(*i);

        const QString type = match.captured(1).trimmed();

        if (type.isNull()) {
            panic("Missing parameter type for argument %d of %s (%s:%d)",
                    argc, qPrintable(name), qPrintable(fileName), lineNumber);
        }

        const QString name = match.captured(2).trimmed();

        if (name.isNull()) {
            panic("Missing parameter name for argument %d of %s (%s:%d)",
                    argc, qPrintable(name), qPrintable(fileName), lineNumber);
        }

        int arrayLen = arrayLength(type);

        Tracepoint::Argument a;
        a.arrayLen = arrayLen;
        a.name = name;
        a.type = decayArrayToPointer(type);

        t.args << std::move(a);

        Tracepoint::Field f;
        f.backendType = backendType(type);
        f.paramType = removeBraces(type);
        f.name = name;
        f.arrayLen = arrayLen;
        f.seqLen = sequenceLength(type);

        t.fields << std::move(f);

        ++i;
    }

    return t;
}

Provider parseProvider(const QString &filename)
{
    QFile f(filename);

    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        panic("Cannot open %s: %s", qPrintable(filename), qPrintable(f.errorString()));

    QTextStream s(&f);

    static const QRegularExpression tracedef(QStringLiteral("^([A-Za-z][A-Za-z0-9_]*)\\((.*)\\)$"));

    Provider provider;
    provider.name = QFileInfo(filename).baseName();

    bool parsingPrefixText = false;
    for (int lineNumber = 1; !s.atEnd(); ++lineNumber) {
        QString line = s.readLine().trimmed();

        if (line == "{"_L1) {
            parsingPrefixText = true;
            continue;
        } else if (parsingPrefixText && line == "}"_L1) {
            parsingPrefixText = false;
            continue;
        } else if (parsingPrefixText) {
            provider.prefixText.append(line);
            continue;
        }

        if (line.isEmpty() || line.startsWith(u'#'))
            continue;

        auto match = tracedef.match(line);
        if (match.hasMatch()) {
            const QString name = match.captured(1);
            const QString argsString = match.captured(2);
            const QStringList args = argsString.split(u',', Qt::SkipEmptyParts);

            provider.tracepoints << parseTracepoint(name, args, filename, lineNumber);
        } else {
            panic("Syntax error while processing '%s' line %d:\n"
                  "    '%s' does not look like a tracepoint definition",
                  qPrintable(filename), lineNumber,
                  qPrintable(line));
        }
    }

    if (parsingPrefixText) {
        panic("Syntax error while processing '%s': "
              "no closing brace found for prefix text block",
              qPrintable(filename));
    }

#ifdef TRACEGEN_DEBUG
    qDebug() << provider.prefixText;
    for (auto i = provider.tracepoints.constBegin(); i != provider.tracepoints.constEnd(); ++i)
        dumpTracepoint(*i);
#endif

    return provider;
}
