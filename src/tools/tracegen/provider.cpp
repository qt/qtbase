// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "provider.h"
#include "panic.h"

#include <qfile.h>
#include <qfileinfo.h>
#include <qtextstream.h>
#include <qregularexpression.h>
#include <qstring.h>
#include <qtpreprocessorsupport.h>

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

#define TYPEDATA_ENTRY(type, backendType) \
{ QT_STRINGIFY(type), backendType }

static Tracepoint::Field::Type backendType(QString rawType)
{
    static const struct TypeData {
        const char *type;
        Tracepoint::Field::Type backendType;
    } typeTable[] = {
        TYPEDATA_ENTRY(short_int, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(signed_short, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(signed_short_int, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(unsigned_short, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(unsigned_short_int, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(unsigned_int, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(signed_int, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(long_int, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(signed_long, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(signed_long_int, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(unsigned_long, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(unsigned_long_int, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(long_long, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(long_long_int, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(signed_long_long, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(signed_long_long_int, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(unsigned_long_long, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(bool, Tracepoint::Field::Boolean),
        TYPEDATA_ENTRY(int, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(signed, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(unsigned, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(long, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(qint64, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(char, Tracepoint::Field::Integer),
        TYPEDATA_ENTRY(intptr_t, Tracepoint::Field::IntegerHex),
        TYPEDATA_ENTRY(uintptr_t, Tracepoint::Field::IntegerHex),
        TYPEDATA_ENTRY(std::intptr_t, Tracepoint::Field::IntegerHex),
        TYPEDATA_ENTRY(std::uintptr_t, Tracepoint::Field::IntegerHex),
        TYPEDATA_ENTRY(float, Tracepoint::Field::Float),
        TYPEDATA_ENTRY(double, Tracepoint::Field::Float),
        TYPEDATA_ENTRY(long double, Tracepoint::Field::Float),
        TYPEDATA_ENTRY(QString, Tracepoint::Field::QtString),
        TYPEDATA_ENTRY(QByteArray, Tracepoint::Field::QtByteArray),
        TYPEDATA_ENTRY(QUrl, Tracepoint::Field::QtUrl),
        TYPEDATA_ENTRY(QRect, Tracepoint::Field::QtRect),
        TYPEDATA_ENTRY(QSize, Tracepoint::Field::QtSize),
        TYPEDATA_ENTRY(QRectF, Tracepoint::Field::QtRectF),
        TYPEDATA_ENTRY(QSizeF, Tracepoint::Field::QtSizeF)
    };

    auto backendType = [](const QString &rawType) {
        static const size_t tableSize = sizeof (typeTable) / sizeof (typeTable[0]);

        for (size_t i = 0; i < tableSize; ++i) {
            if (rawType == QLatin1StringView(typeTable[i].type))
                return typeTable[i];
        }

        TypeData unknown = { nullptr, Tracepoint::Field::Unknown };
        return unknown;
    };

    int arrayLen = arrayLength(rawType);
    if (arrayLen > 0)
        rawType = removeBraces(rawType);

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

    TypeData d = backendType(rawType);
    return d.backendType;
}

static Tracepoint parseTracepoint(const Provider &provider, const QString &name, const QStringList &args,
        const QString &fileName, const int lineNumber)
{
    Tracepoint t;
    t.name = name;

    auto findEnumeration = [](const QList<TraceEnum> &enums, const QString &name) {
        for (const auto &e : enums) {
            if (e.name == name)
                return e;
        }
        return TraceEnum();
    };
    auto findFlags = [](const QList<TraceFlags> &flags, const QString &name) {
        for (const auto &f : flags) {
            if (f.name == name)
                return f;
        }
        return TraceFlags();
    };


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

        Tracepoint::Field field;
        const TraceEnum &e = findEnumeration(provider.enumerations, type);
        const TraceFlags &f = findFlags(provider.flags, type);
        if (!e.name.isEmpty()) {
            field.backendType = Tracepoint::Field::EnumeratedType;
            field.enumValueSize = e.valueSize;
        } else if (!f.name.isEmpty()) {
            field.backendType = Tracepoint::Field::FlagType;
        } else {
            field.backendType = backendType(type);
        }
        field.paramType = removeBraces(type);
        field.name = name;
        field.arrayLen = arrayLen;
        field.seqLen = sequenceLength(type);

        t.fields << std::move(field);

        ++i;
    }

    return t;
}

static int minumumValueSize(int min, int max)
{
    if (min < 0) {
        if (min >= std::numeric_limits<char>::min() && max <= std::numeric_limits<char>::max())
            return 8;
        if (min >= std::numeric_limits<short>::min() && max <= std::numeric_limits<short>::max())
            return 16;
        return 32;
    }
    if (max <= std::numeric_limits<unsigned char>::max())
        return 8;
    if (max <= std::numeric_limits<unsigned short>::max())
        return 16;
    return 32;
}

static bool isPow2OrZero(quint32 value)
{
    return (value & (value - 1)) == 0;
}

static quint32 pow2Log2(quint32 v) {
    return 32 - qCountLeadingZeroBits(v);
}

Provider parseProvider(const QString &filename)
{
    QFile f(filename);

    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        panic("Cannot open %s: %s", qPrintable(filename), qPrintable(f.errorString()));

    QTextStream s(&f);

    static const QRegularExpression tracedef(QStringLiteral("^([A-Za-z][A-Za-z0-9_]*)\\((.*)\\)$"));
    static const QRegularExpression enumenddef(QStringLiteral("^} ?([A-Za-z][A-Za-z0-9_:]*);"));
    static const QRegularExpression enumdef(QStringLiteral("^([A-Za-z][A-Za-z0-9_]*)( *= *([xabcdef0-9]*))?"));
    static const QRegularExpression rangedef(QStringLiteral("^RANGE\\(([A-Za-z][A-Za-z0-9_]*) ?, ?([0-9]*) ?... ?([0-9]*) ?\\)"));

    Provider provider;
    provider.name = QFileInfo(filename).baseName();

    bool parsingPrefixText = false;
    bool parsingEnum = false;
    bool parsingFlags = false;
    TraceEnum currentEnum;
    TraceFlags currentFlags;
    int currentEnumValue = 0;
    int minEnumValue = std::numeric_limits<int>::max();
    int maxEnumValue = std::numeric_limits<int>::min();
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
        } else if (line == "ENUM {"_L1) {
            parsingEnum = true;
            continue;
        } else if (line == "FLAGS {"_L1) {
            parsingFlags = true;
            continue;
        } else if (line.startsWith("}"_L1) && (parsingEnum || parsingFlags)) {
            auto match = enumenddef.match(line);
            if (match.hasMatch()) {
                if (parsingEnum) {
                    currentEnum.name = match.captured(1);
                    currentEnum.valueSize = minumumValueSize(minEnumValue, maxEnumValue);
                    provider.enumerations.push_back(currentEnum);
                    currentEnum = TraceEnum();
                    parsingEnum = false;
                } else {
                    currentFlags.name = match.captured(1);
                    provider.flags.push_back(currentFlags);
                    currentFlags = TraceFlags();
                    parsingFlags = false;
                }

                minEnumValue = std::numeric_limits<int>::max();
                maxEnumValue = std::numeric_limits<int>::min();
            } else {
                panic("Syntax error while processing '%s' line %d:\n"
                      "    '%s' end of enum/flags does not match",
                      qPrintable(filename), lineNumber,
                      qPrintable(line));
            }

            continue;
        }

        if (line.isEmpty() || line.startsWith(u'#'))
            continue;

        if (parsingEnum || parsingFlags) {
            auto m = enumdef.match(line);
            if (parsingEnum && line.startsWith(QStringLiteral("RANGE"))) {
                auto m = rangedef.match(line);
                if (m.hasMatch()) {
                    TraceEnum::EnumValue value;
                    value.name = m.captured(1);
                    value.value = m.captured(2).toInt();
                    value.range = m.captured(3).toInt();
                    currentEnumValue = value.range + 1;
                    currentEnum.values.push_back(value);
                    maxEnumValue = qMax(maxEnumValue, value.range);
                    minEnumValue = qMin(minEnumValue, value.value);
                }
            } else if (m.hasMatch()) {
                if (m.hasCaptured(3)) {
                    if (parsingEnum) {
                        TraceEnum::EnumValue value;
                        value.name = m.captured(1);
                        value.value = m.captured(3).toInt();
                        value.range = 0;
                        currentEnumValue = value.value + 1;
                        currentEnum.values.push_back(value);
                        maxEnumValue = qMax(maxEnumValue, value.value);
                        minEnumValue = qMin(minEnumValue, value.value);
                    } else {
                        TraceFlags::FlagValue value;
                        value.name = m.captured(1);
                        if (m.captured(3).startsWith(QStringLiteral("0x")))
                            value.value = m.captured(3).toInt(nullptr, 16);
                        else
                            value.value = m.captured(3).toInt();
                        if (!isPow2OrZero(value.value)) {
                            printf("Warning: '%s' line %d:\n"
                                  "    '%s' flag value is not power of two.\n",
                                  qPrintable(filename), lineNumber,
                                  qPrintable(line));
                        } else {
                            value.value = pow2Log2(value.value);
                            currentFlags.values.push_back(value);
                        }
                    }
                } else {
                    maxEnumValue = qMax(maxEnumValue, currentEnumValue);
                    minEnumValue = qMin(minEnumValue, currentEnumValue);
                    if (parsingEnum) {
                        currentEnum.values.push_back({m.captured(0), currentEnumValue++, 0});
                    } else {
                        panic("Syntax error while processing '%s' line %d:\n"
                              "    '%s' flags value not set",
                              qPrintable(filename), lineNumber,
                              qPrintable(line));
                    }
                }
            } else {
                panic("Syntax error while processing '%s' line %d:\n"
                      "    '%s' enum/flags does not match",
                      qPrintable(filename), lineNumber,
                      qPrintable(line));
            }
            continue;
        }

        auto match = tracedef.match(line);
        if (match.hasMatch()) {
            const QString name = match.captured(1);
            const QString argsString = match.captured(2);
            const QStringList args = argsString.split(u',', Qt::SkipEmptyParts);

            provider.tracepoints << parseTracepoint(provider, name, args, filename, lineNumber);
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
