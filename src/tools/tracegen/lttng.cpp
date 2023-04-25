// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "lttng.h"
#include "provider.h"
#include "helpers.h"
#include "panic.h"
#include "qtheaders.h"

#include <qfile.h>
#include <qfileinfo.h>
#include <qtextstream.h>
#include <qdebug.h>

static void writeCtfMacro(QTextStream &stream, const Provider &provider, const Tracepoint::Field &field)
{
    const QString &paramType = field.paramType;
    const QString &name = field.name;
    const QString &seqLen = field.seqLen;
    const int arrayLen = field.arrayLen;

    if (arrayLen > 0) {
        if (paramType == QStringLiteral("double") || paramType == QStringLiteral("float")) {
            const char *newline = nullptr;
            for (int i = 0; i < arrayLen; i++) {
                stream << newline;
                stream << "ctf_float(" <<paramType << ", " << name << "_" << QString::number(i) << ", "
                       << name << "[" << QString::number(i) << "]" << ")";
                newline = "\n        ";
            }

        } else {
            stream << "ctf_array(" <<paramType << ", "
                   << name << ", " << name << ", " << arrayLen << ")";
        }
        return;
    }

    switch (field.backendType) {
    case Tracepoint::Field::Sequence:
        stream << "ctf_sequence(" << paramType
            << ", " << name << ", " << name
            << ", unsigned int, "  << seqLen << ")";
        return;
    case Tracepoint::Field::Boolean:
    case Tracepoint::Field::Integer:
        stream << "ctf_integer(" << paramType << ", " << name << ", " << name << ")";
        return;
    case Tracepoint::Field::IntegerHex:
    case Tracepoint::Field::Pointer:
        stream << "ctf_integer_hex(" << paramType << ", " << name << ", " << name << ")";
        return;
    case Tracepoint::Field::Float:
        stream << "ctf_float(" << paramType << ", " << name << ", " << name << ")";
        return;
    case Tracepoint::Field::String:
        stream << "ctf_string(" << name << ", " << name << ")";
        return;
    case Tracepoint::Field::QtString:
        stream << "ctf_string(" << name << ", " << name << ".toUtf8().constData())";
        return;
    case Tracepoint::Field::QtByteArray:
        stream << "ctf_sequence(const char, " << name << ", "
               << name << ".constData(), unsigned int, " << name << ".size())";
        return;
    case Tracepoint::Field::QtUrl:
        stream << "ctf_string(" << name << ", " << name << ".toString().toUtf8().constData())";
        return;
    case Tracepoint::Field::QtRect:
        stream << "ctf_integer(int, QRect_" << name << "_x, " << name << ".x()) "
               << "ctf_integer(int, QRect_" << name << "_y, " << name << ".y()) "
               << "ctf_integer(int, QRect_" << name << "_width, " << name << ".width()) "
               << "ctf_integer(int, QRect_" << name << "_height, " << name << ".height()) ";
        return;
    case Tracepoint::Field::QtSizeF:
        stream << "ctf_float(double, QSizeF_" << name << "_width, " << name << ".width()) "
               << "ctf_float(double, QSizeF_" << name << "_height, " << name << ".height()) ";
        return;
    case Tracepoint::Field::QtRectF:
        stream << "ctf_float(double, QRectF_" << name << "_x, " << name << ".x()) "
               << "ctf_float(double, QRectF_" << name << "_y, " << name << ".y()) "
               << "ctf_float(double, QRectF_" << name << "_width, " << name << ".width()) "
               << "ctf_float(double, QRectF_" << name << "_height, " << name << ".height()) ";
        return;
    case Tracepoint::Field::QtSize:
        stream << "ctf_integer(int, QSize_" << name << "_width, " << name << ".width()) "
               << "ctf_integer(int, QSize_" << name << "_height, " << name << ".height()) ";
        return;
    case Tracepoint::Field::EnumeratedType:
        stream << "ctf_enum(" << provider.name << ", " << typeToTypeName(paramType) << ", int, " << name << ", " << name << ") ";
        return;
    case Tracepoint::Field::FlagType:
        stream << "ctf_sequence(const char , " << name << ", "
               << name << ".constData(), unsigned int, " << name << ".size())";
        return;
    case Tracepoint::Field::Unknown:
        panic("Cannot deduce CTF type for '%s %s", qPrintable(paramType), qPrintable(name));
        break;
    }
}

static void writePrologue(QTextStream &stream, const QString &fileName, const Provider &provider)
{
    writeCommonPrologue(stream);
    const QString guard = includeGuard(fileName);

    stream << "#undef TRACEPOINT_PROVIDER\n";
    stream << "#define TRACEPOINT_PROVIDER " << provider.name << "\n";
    stream << "\n";

    // include prefix text or qt headers only once
    stream << "#if !defined(" << guard << ")\n";
    stream << qtHeaders();
    stream << "\n";
    if (!provider.prefixText.isEmpty())
        stream << provider.prefixText.join(u'\n') << "\n\n";
    stream << "#endif\n\n";

    /* the first guard is the usual one, the second is required
     * by LTTNG to force the re-evaluation of TRACEPOINT_* macros
     */
    stream << "#if !defined(" << guard << ") || defined(TRACEPOINT_HEADER_MULTI_READ)\n";

    stream << "#define " << guard << "\n\n"
           << "#undef TRACEPOINT_INCLUDE\n"
           << "#define TRACEPOINT_INCLUDE \"" << fileName << "\"\n\n";

    stream << "#include <lttng/tracepoint.h>\n\n";

    const QString namespaceGuard = guard + QStringLiteral("_USE_NAMESPACE");
    stream << "#if !defined(" << namespaceGuard << ")\n"
           << "#define " << namespaceGuard << "\n"
           << "QT_USE_NAMESPACE\n"
           << "#endif // " << namespaceGuard << "\n\n";
}

static void writeEpilogue(QTextStream &stream, const QString &fileName)
{
    stream << "\n";
    stream << "#endif // " << includeGuard(fileName) << "\n"
           << "#include <lttng/tracepoint-event.h>\n"
           << "#include <private/qtrace_p.h>\n";
}

static void writeWrapper(QTextStream &stream,
        const Tracepoint &tracepoint, const Provider &provider)
{
    const QString argList = formatFunctionSignature(tracepoint.args);
    const QString paramList = formatParameterList(provider, tracepoint.args, tracepoint.fields, LTTNG);
    const QString &name = tracepoint.name;
    const QString includeGuard = QStringLiteral("TP_%1_%2").arg(provider.name).arg(name).toUpper();

    /* prevents the redefinion of the inline wrapper functions
     * once LTTNG recursively includes this header file
     */
    stream << "\n"
           << "#ifndef " << includeGuard << "\n"
           << "#define " << includeGuard << "\n"
           << "QT_BEGIN_NAMESPACE\n"
           << "namespace QtPrivate {\n";

    stream << "inline void trace_" << name << "(" << argList << ")\n"
           << "{\n"
           << "    tracepoint(" << provider.name << ", " << name << paramList << ");\n"
           << "}\n";

    stream << "inline void do_trace_" << name << "(" << argList << ")\n"
           << "{\n"
           << "    do_tracepoint(" << provider.name << ", " << name << paramList << ");\n"
           << "}\n";

    stream << "inline bool trace_" << name << "_enabled()\n"
           << "{\n"
           << "    return tracepoint_enabled(" << provider.name << ", " << name << ");\n"
           << "}\n";

    stream << "} // namespace QtPrivate\n"
           << "QT_END_NAMESPACE\n"
           << "#endif // " << includeGuard << "\n\n";
}

static void writeTracepoint(QTextStream &stream, const Provider &provider,
        const Tracepoint &tracepoint, const QString &providerName)
{
    stream  << "TRACEPOINT_EVENT(\n"
            << "    " << providerName << ",\n"
            << "    " << tracepoint.name << ",\n"
            << "    TP_ARGS(";

    const char *comma = nullptr;

    for (int i = 0; i < tracepoint.args.size(); i++) {
        const auto &arg = tracepoint.args[i];
        const auto &field = tracepoint.fields[i];
        if (field.backendType == Tracepoint::Field::FlagType)
            stream << comma << "QByteArray, " << arg.name;
        else
            stream << comma << arg.type << ", " << arg.name;
        comma = ", ";
    }

    stream << "),\n"
        << "    TP_FIELDS(";

    const char *newline = nullptr;

    for (const Tracepoint::Field &f : tracepoint.fields) {
        stream << newline;
        writeCtfMacro(stream, provider, f);
        newline = "\n        ";
    }

    stream << ")\n)\n\n";
}

static void writeEnums(QTextStream &stream, const Provider &provider)
{
    for (const auto &e : provider.enumerations) {
        stream << "TRACEPOINT_ENUM(\n"
               << "    " << provider.name << ",\n"
               << "    " << typeToTypeName(e.name) << ",\n"
               << "    TP_ENUM_VALUES(\n";
        QList<int> handledValues;
        for (const auto &v : e.values) {
            if (v.range > 0) {
                stream << "        ctf_enum_range(\"" << v.name << "\", " << v.value << ", " << v.range << ")\n";
            } else if (!handledValues.contains(v.value)) {
                stream << "        ctf_enum_value(\"" << aggregateListValues(v.value, e.values) << "\", " << v.value << ")\n";
                handledValues.append(v.value);
            }
        }
        stream << "    )\n)\n\n";
    }
}

static void writeFlags(QTextStream &stream, const Provider &provider)
{
    for (const auto &f : provider.flags) {
        stream << "TRACEPOINT_ENUM(\n"
               << "    " << provider.name << ",\n"
               << "    " << typeToTypeName(f.name) << ",\n"
               << "    TP_ENUM_VALUES(\n";
        QList<int> handledValues;
        for (const auto &v : f.values) {
            if (!handledValues.contains(v.value)) {
                stream << "        ctf_enum_value(\"" << aggregateListValues(v.value, f.values) << "\", " << v.value << ")\n";
                handledValues.append(v.value);
            }
        }
        stream << "    )\n)\n\n";
    }

    // converters
    const QString includeGuard = QStringLiteral("TP_%1_CONVERTERS").arg(provider.name).toUpper();
    stream << "\n"
           << "#ifndef " << includeGuard << "\n"
           << "#define " << includeGuard << "\n";
    stream << "QT_BEGIN_NAMESPACE\n";
    stream << "namespace QtPrivate {\n";
    for (const auto &f : provider.flags) {
        stream << "inline QByteArray trace_convert_" << typeToTypeName(f.name) << "(" << f.name << " val)\n";
        stream << "{\n";
        stream << "    QByteArray ret;\n";
        stream << "    if (val == 0) { ret.append((char)0); return ret; }\n";

        for (const auto &v : f.values) {
            if (!v.value)
                continue;
            stream << "    if (val & " << (1 << (v.value - 1)) << ") { ret.append((char)" << v.value << "); };\n";
        }
        stream << "    return ret;\n";
        stream << "}\n";

    }
    stream << "} // namespace QtPrivate\n"
           << "QT_END_NAMESPACE\n\n"
           << "#endif // " << includeGuard << "\n\n";
}

static void writeTracepoints(QTextStream &stream, const Provider &provider)
{
    for (const Tracepoint &t : provider.tracepoints) {
        writeTracepoint(stream, provider, t, provider.name);
        writeWrapper(stream, t, provider);
    }
}

void writeLttng(QFile &file, const Provider &provider)
{
    QTextStream stream(&file);

    const QString fileName = QFileInfo(file.fileName()).fileName();

    writePrologue(stream, fileName, provider);
    writeEnums(stream, provider);
    writeFlags(stream, provider);
    writeTracepoints(stream, provider);
    writeEpilogue(stream, fileName);
}

