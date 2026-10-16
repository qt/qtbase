// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "hitrace.h"
#include "provider.h"
#include "helpers.h"
#include "qtheaders.h"

#include <qfile.h>
#include <qfileinfo.h>
#include <qstringlist.h>
#include <qtextstream.h>

using namespace Qt::StringLiterals;

namespace {

constexpr QLatin1StringView traceLevel = "HITRACE_LEVEL_COMMERCIAL"_L1;
constexpr QLatin1StringView rangeSeparator = " + "_L1;
constexpr char flagSeparator = '+';

QString sliceName(const Provider &provider, const QString &name)
{
    return provider.name + u':' + name;
}

QString valueBudget(const Provider &provider, const QString &tracepointName)
{
    return "qtTraceValueBudget_"_L1 + provider.name + u'_' + tracepointName;
}

QString sliceConstant(const Provider &provider, const QString &tracepointName)
{
    return "qtTraceSlice_"_L1 + provider.name + u'_' + tracepointName;
}

QString renderFunction(const Provider &provider, const QString &tracepointName)
{
    return "qtTraceRender_"_L1 + provider.name + u'_' + tracepointName;
}

QString sanitized(const QString &valueExpression, const QString &budget)
{
    return "qtHiTraceArgument("_L1 + valueExpression + ", "_L1 + budget + ").constData()"_L1;
}

QString enumConverter(const QString &type)
{
    return "qtTraceEnum_"_L1 + typeToTypeName(type);
}

QString flagsConverter(const QString &type)
{
    return "qtTraceFlags_"_L1 + typeToTypeName(type);
}

struct CustomArgs
{
    QString format;
    QStringList arguments;
    QStringList unusedNames;
    int scalarWidth = 0;
    int stringCount = 0;
};

constexpr int numberWidth = 20;
constexpr int booleanWidth = 5;
constexpr int floatWidth = 14;
constexpr int pointerWidth = 18;

int renderedWidth(QLatin1StringView specifier)
{
    if (specifier == "%s"_L1)
        return booleanWidth;
    if (specifier == "%g"_L1)
        return floatWidth;
    if (specifier == "%p"_L1)
        return pointerWidth;
    return numberWidth;
}

enum class ValueKind { Scalar, Budgeted };

void appendScalar(CustomArgs &args, QStringList &pairs, const QString &name,
                  QLatin1StringView specifier, const QString &expression,
                  ValueKind kind = ValueKind::Scalar)
{
    pairs << name + u'=' + specifier;
    args.arguments << expression;
    if (kind == ValueKind::Budgeted)
        ++args.stringCount;
    else
        args.scalarWidth += renderedWidth(specifier);
}

enum class MemberKind { Integral, Fractional };

void appendMembers(CustomArgs &args, QStringList &pairs, const QString &name, MemberKind kind,
                   std::initializer_list<QLatin1StringView> members)
{
    for (QLatin1StringView member : members) {
        const QString accessor = name + u'.' + member + "()"_L1;
        const QString pairName = name + u'_' + member;
        if (kind == MemberKind::Integral) {
            appendScalar(args, pairs, pairName, "%lld"_L1,
                         "static_cast<long long>("_L1 + accessor + u')');
        } else {
            appendScalar(args, pairs, pairName, "%g"_L1, accessor);
        }
    }
}

CustomArgs customArgsFor(const Provider &provider, const Tracepoint &tracepoint)
{
    CustomArgs args;
    QStringList pairs;
    const QString budget = valueBudget(provider, tracepoint.name);

    for (int i = 0; i < tracepoint.args.size(); ++i) {
        const Tracepoint::Argument &arg = tracepoint.args[i];
        const Tracepoint::Field &field = tracepoint.fields[i];
        const QString &name = arg.name;
        const qsizetype before = pairs.size();

        if (arg.arrayLen == 0) {
            switch (field.backendType) {
            case Tracepoint::Field::Integer:
                if (arg.type.contains("unsigned"_L1)) {
                    appendScalar(args, pairs, name, "%llu"_L1,
                                 "static_cast<unsigned long long>("_L1 + name + u')');
                } else {
                    appendScalar(args, pairs, name, "%lld"_L1,
                                 "static_cast<long long>("_L1 + name + u')');
                }
                break;
            case Tracepoint::Field::IntegerHex:
                appendScalar(args, pairs, name, "%#llx"_L1,
                             "static_cast<unsigned long long>("_L1 + name + u')');
                break;
            case Tracepoint::Field::EnumeratedType:
                appendScalar(args, pairs, name, "%s"_L1,
                             sanitized(enumConverter(arg.type) + u'(' + name + ").constData()"_L1,
                                       budget),
                             ValueKind::Budgeted);
                break;
            case Tracepoint::Field::FlagType:
                appendScalar(args, pairs, name, "%s"_L1,
                             sanitized(flagsConverter(arg.type) + u'(' + name + ").constData()"_L1,
                                       budget),
                             ValueKind::Budgeted);
                break;
            case Tracepoint::Field::Boolean:
                appendScalar(args, pairs, name, "%s"_L1,
                             u'(' + name + " ? \"true\" : \"false\")"_L1);
                break;
            case Tracepoint::Field::Float:
                appendScalar(args, pairs, name, "%g"_L1, "static_cast<double>("_L1 + name + u')');
                break;
            case Tracepoint::Field::Pointer:
                appendScalar(args, pairs, name, "%p"_L1,
                             "static_cast<const void *>("_L1 + name + u')');
                break;
            case Tracepoint::Field::String:
            case Tracepoint::Field::QtString:
                appendScalar(args, pairs, name, "%s"_L1, sanitized(name, budget),
                             ValueKind::Budgeted);
                break;
            case Tracepoint::Field::QtUrl:
                appendScalar(args, pairs, name, "%s"_L1, sanitized(name + ".toString()"_L1, budget),
                             ValueKind::Budgeted);
                break;
            case Tracepoint::Field::QtRect:
                appendMembers(args, pairs, name, MemberKind::Integral,
                              { "x"_L1, "y"_L1, "width"_L1, "height"_L1 });
                break;
            case Tracepoint::Field::QtRectF:
                appendMembers(args, pairs, name, MemberKind::Fractional,
                              { "x"_L1, "y"_L1, "width"_L1, "height"_L1 });
                break;
            case Tracepoint::Field::QtSize:
                appendMembers(args, pairs, name, MemberKind::Integral, { "width"_L1, "height"_L1 });
                break;
            case Tracepoint::Field::QtSizeF:
                appendMembers(args, pairs, name, MemberKind::Fractional,
                              { "width"_L1, "height"_L1 });
                break;
            case Tracepoint::Field::QtByteArray:
                appendScalar(args, pairs, name, "%s"_L1,
                             sanitized(name + ".toHex().constData()"_L1, budget),
                             ValueKind::Budgeted);
                break;
            case Tracepoint::Field::Sequence:
            case Tracepoint::Field::Unknown:
                break;
            }
        }

        if (pairs.size() == before) {
            args.unusedNames << name;
            printf("Warning: '%s' argument '%s' has no HiTrace representation "
                   "and is left out of the trace.\n",
                   qPrintable(tracepoint.name), qPrintable(name));
        }
    }

    args.format = pairs.join(u',');
    return args;
}

void writeEnumConverter(QTextStream &stream, const TraceEnum &enumeration)
{
    stream << "inline QByteArray " << enumConverter(enumeration.name) << "(" << enumeration.name
           << " value)\n"
           << "{\n"
           << "    const long long raw = static_cast<long long>(value);\n";

    for (const TraceEnum::EnumValue &v : enumeration.values) {
        if (v.range != 0) {
            stream << "    if (raw >= " << v.value << " && raw <= " << v.range << ")\n"
                   << "        return QByteArrayLiteral(\"" << v.name << rangeSeparator
                   << "\") + QByteArray::number(raw - " << v.value << ");\n";
        }
    }

    stream << "    switch (raw) {\n";
    QList<int> handled;
    for (const TraceEnum::EnumValue &v : enumeration.values) {
        if (v.range == 0 && !handled.contains(v.value)) {
            stream << "    case " << v.value << ": return QByteArrayLiteral(\""
                   << aggregateListValues(v.value, enumeration.values) << "\");\n";
            handled.append(v.value);
        }
    }
    stream << "    default: break;\n"
           << "    }\n"
           << "    return QByteArray::number(raw);\n"
           << "}\n";
}

void writeFlagsConverter(QTextStream &stream, const TraceFlags &flag)
{
    stream << "inline QByteArray " << flagsConverter(flag.name) << "(" << flag.name << " value)\n"
           << "{\n"
           << "    const long long raw = static_cast<long long>(value);\n";

    for (const TraceFlags::FlagValue &v : flag.values) {
        if (v.value == 0) {
            stream << "    if (raw == 0)\n"
                   << "        return QByteArrayLiteral(\""
                   << aggregateListValues(v.value, flag.values) << "\");\n";
            break;
        }
    }

    stream << "    QByteArray names;\n";

    QList<int> handled;
    for (const TraceFlags::FlagValue &v : flag.values) {
        if (v.value != 0 && !handled.contains(v.value)) {
            handled.append(v.value);
            stream << "    if (raw & " << (1LL << (v.value - 1)) << ") {\n"
                   << "        if (!names.isEmpty())\n"
                   << "            names += '" << flagSeparator << "';\n"
                   << "        names += \"" << v.name << "\";\n"
                   << "    }\n";
        }
    }
    stream << "    return names.isEmpty() ? QByteArray::number(raw) : names;\n"
           << "}\n";
}

void writePrologue(QTextStream &stream, const QString &fileName, const Provider &provider)
{
    writeCommonPrologue(stream);
    const QString guard = includeGuard(fileName);

    stream << "\n"
           << "#include <hitrace/trace.h>\n"
           << "#include <private/qhitrace_p.h>\n\n";

    stream << "#if !defined(" << guard << ")\n";
    stream << qtHeaders();
    stream << "\n";
    if (!provider.prefixText.isEmpty())
        stream << provider.prefixText.join(u'\n') << "\n\n";
    stream << "#endif\n\n";

    stream << "#if !defined(" << guard << ")\n";
    stream << "#define " << guard << "\n\n";

    const QString namespaceGuard = guard + QStringLiteral("_USE_NAMESPACE");
    stream << "#if !defined(" << namespaceGuard << ")\n"
           << "#define " << namespaceGuard << "\n"
           << "QT_USE_NAMESPACE\n"
           << "#endif // " << namespaceGuard << "\n\n";

    if (provider.enumerations.isEmpty() && provider.flags.isEmpty())
        return;

    stream << "QT_BEGIN_NAMESPACE\n"
           << "namespace QtPrivate {\n";

    for (const TraceEnum &enumeration : provider.enumerations)
        writeEnumConverter(stream, enumeration);
    for (const TraceFlags &flag : provider.flags)
        writeFlagsConverter(stream, flag);

    stream << "} // namespace QtPrivate\n"
           << "QT_END_NAMESPACE\n\n";
}

void writeRenderDefinition(QTextStream &stream, const Tracepoint &tracepoint,
                           const CustomArgs &customArgs, const Provider &provider)
{
    if (customArgs.format.isEmpty())
        return;

    const QString &name = tracepoint.name;
    const QString guardName = QStringLiteral("TPDEF_%1_%2").arg(provider.name, name).toUpper();

    stream << "\n"
           << "#if defined(TRACEPOINT_DEFINE) && !defined(" << guardName << ")\n"
           << "#define " << guardName << "\n"
           << "QT_BEGIN_NAMESPACE\n"
           << "namespace QtPrivate {\n"
           << "const char *" << renderFunction(provider, name) << "("
           << formatFunctionSignature(tracepoint.args) << ")\n"
           << "{\n";
    for (const QString &unusedName : customArgs.unusedNames)
        stream << "    Q_UNUSED(" << unusedName << ");\n";
    stream << "    return qtHiTraceFormatArguments(sizeof(" << sliceConstant(provider, name)
           << ") - 1, \"" << customArgs.format << "\", " << customArgs.arguments.join(", "_L1)
           << ");\n"
           << "}\n"
           << "} // namespace QtPrivate\n"
           << "QT_END_NAMESPACE\n"
           << "#endif // " << guardName << "\n";
}

void writeEpilogue(QTextStream &stream, const QString &fileName)
{
    stream << "\n";
    stream << "#endif // " << includeGuard(fileName) << "\n";
}

void writeWrapper(QTextStream &stream, const Tracepoint &tracepoint, const CustomArgs &customArgs,
                  const Provider &provider)
{
    const QString argList = formatFunctionSignature(tracepoint.args);
    const QString paramList =
            formatParameterList(provider, tracepoint.args, tracepoint.fields, HITRACE);
    const QString &name = tracepoint.name;
    const QString guardName = QStringLiteral("TP_%1_%2").arg(provider.name, name).toUpper();
    const QString slice = sliceName(provider, name);
    const QString sliceIdentifier = sliceConstant(provider, name);
    const bool hasCustomArgs = !customArgs.format.isEmpty();

    stream << "\n"
           << "#ifndef " << guardName << "\n"
           << "#define " << guardName << "\n"
           << "QT_BEGIN_NAMESPACE\n"
           << "namespace QtPrivate {\n";

    stream << "inline constexpr char " << sliceIdentifier << "[] = \"" << slice << "\";\n";
    if (customArgs.stringCount > 0) {
        stream << "inline constexpr int " << valueBudget(provider, name)
               << " = qtHiTraceValueBudget(sizeof(" << sliceIdentifier << ") - 1, "
               << customArgs.format.size() + customArgs.scalarWidth << ", "
               << customArgs.stringCount << ");\n";
    }

    const QString render = renderFunction(provider, name);

    if (hasCustomArgs)
        stream << "const char *" << render << "(" << argList << ");\n";

    stream << "inline void do_trace_" << name << "(" << argList << ")\n"
           << "{\n";

    if (!hasCustomArgs) {
        for (const QString &unusedName : customArgs.unusedNames)
            stream << "    Q_UNUSED(" << unusedName << ");\n";
    }

    const QString startTrace =
            "OH_HiTrace_StartTraceEx("_L1 + traceLevel + ", "_L1 + sliceIdentifier + ", "_L1;

    if (hasCustomArgs) {
        stream << "    " << startTrace << "OH_HiTrace_IsTraceEnabled() ? " << render << "("
               << paramList << ") : \"\");\n";
    } else {
        stream << "    " << startTrace << "\"\");\n";
    }
    stream << "    OH_HiTrace_FinishTraceEx(" << traceLevel << ");\n";

    stream << "}\n";

    stream << "inline void trace_" << name << "(" << argList << ") { do_trace_" << name << "("
           << paramList << "); }\n";

    stream << "inline bool trace_" << name << "_enabled()\n"
           << "{\n"
           << "    return OH_HiTrace_IsTraceEnabled();\n"
           << "}\n";

    stream << "} // namespace QtPrivate\n"
           << "QT_END_NAMESPACE\n"
           << "#endif // " << guardName << "\n";
}

} // namespace

void writeHiTrace(QFile &file, const Provider &provider)
{
    QTextStream stream(&file);

    const QString fileName = QFileInfo(file.fileName()).fileName();

    QList<CustomArgs> customArgs;
    for (const Tracepoint &t : provider.tracepoints)
        customArgs << customArgsFor(provider, t);

    writePrologue(stream, fileName, provider);
    for (qsizetype i = 0; i < provider.tracepoints.size(); ++i)
        writeWrapper(stream, provider.tracepoints[i], customArgs[i], provider);
    writeEpilogue(stream, fileName);

    for (qsizetype i = 0; i < provider.tracepoints.size(); ++i)
        writeRenderDefinition(stream, provider.tracepoints[i], customArgs[i], provider);

    stream << "#include <private/qtrace_p.h>\n";
}
