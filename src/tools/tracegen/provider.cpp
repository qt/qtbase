/****************************************************************************
**
** Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "provider.h"
#include "panic.h"

#include <qfile.h>
#include <qfileinfo.h>
#include <qtextstream.h>
#include <qregexp.h>
#include <qstring.h>

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
    static const QRegExp rx(QStringLiteral(".*\\[([0-9]+)\\].*"));

    if (!rx.exactMatch(rawType.trimmed()))
        return 0;

    return rx.cap(1).toInt();
}

static inline QString sequenceLength(const QString &rawType)
{
    /* matches the identifier pointing to length of a CTF sequence type, which is
     * a dynamic sized array.
     * Ex: in qcoreapplication_foo(const char[len], some_string, unsigned int, * len)
     * it will match the 'len' part of 'const char[len]')
     */
    static const QRegExp rx(QStringLiteral(".*\\[([A-Za-z_][A-Za-z_0-9]*)\\].*"));

    if (!rx.exactMatch(rawType.trimmed()))
        return QString();

    return rx.cap(1);
}

static QString decayArrayToPointer(QString type)
{
    /* decays an array to a pointer, i.e., if 'type' holds int[10],
     * this function returns 'int *'
     */
    static QRegExp rx(QStringLiteral("\\[(.+)\\]"));

    rx.setMinimal(true);
    return type.replace(rx, QStringLiteral("*"));
}

static QString removeBraces(QString type)
{
    static const QRegExp rx(QStringLiteral("\\[.*\\]"));

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
            if (rawType == QLatin1String(typeTable[i].type))
                return typeTable[i].backendType;
        }

        return Tracepoint::Field::Unknown;
    };

    if (arrayLength(rawType) > 0)
        return Tracepoint::Field::Array;

    if (!sequenceLength(rawType).isNull())
        return Tracepoint::Field::Sequence;

    static const QRegExp constMatch(QStringLiteral("\\bconst\\b"));
    rawType.remove(constMatch);
    rawType.remove(QLatin1Char('&'));

    static const QRegExp ptrMatch(QStringLiteral("\\s*\\*\\s*"));
    rawType.replace(ptrMatch, QStringLiteral("_ptr"));
    rawType = rawType.trimmed();
    rawType.replace(QStringLiteral(" "), QStringLiteral("_"));

    if (rawType == QLatin1String("char_ptr"))
        return Tracepoint::Field::String;

    if (rawType.endsWith(QLatin1String("_ptr")))
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

    static const QRegExp rx(QStringLiteral("(.*)\\b([A-Za-z_][A-Za-z0-9_]*)$"));

    while (i != end) {
        rx.exactMatch(*i);

        const QString type = rx.cap(1).trimmed();

        if (type.isNull()) {
            panic("Missing parameter type for argument %d of %s (%s:%d)",
                    argc, qPrintable(name), qPrintable(fileName), lineNumber);
        }

        const QString name = rx.cap(2).trimmed();

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

    static const QRegExp tracedef(QStringLiteral("([A-Za-z][A-Za-z0-9_]*)\\((.*)\\)"));

    Provider provider;
    provider.name = QFileInfo(filename).baseName();

    bool parsingPrefixText = false;
    for (int lineNumber = 1; !s.atEnd(); ++lineNumber) {
        QString line = s.readLine().trimmed();

        if (line == QLatin1String("{")) {
            parsingPrefixText = true;
            continue;
        } else if (parsingPrefixText && line == QLatin1String("}")) {
            parsingPrefixText = false;
            continue;
        } else if (parsingPrefixText) {
            provider.prefixText.append(line);
            continue;
        }

        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;

        if (tracedef.exactMatch(line)) {
            const QString name = tracedef.cap(1);
            const QString argsString = tracedef.cap(2);
            const QStringList args = argsString.split(QLatin1Char(','), Qt::SkipEmptyParts);

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
