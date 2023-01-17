// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "ctf.h"
#include "provider.h"
#include "helpers.h"
#include "panic.h"
#include "qtheaders.h"

#include <qfile.h>
#include <qfileinfo.h>
#include <qtextstream.h>
#include <qdebug.h>


static void writePrologue(QTextStream &stream, const QString &fileName, const Provider &provider)
{
    const QString guard = includeGuard(fileName);

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

    stream << "#include <private/qctf_p.h>\n\n";

    const QString namespaceGuard = guard + QStringLiteral("_USE_NAMESPACE");
    stream << "#if !defined(" << namespaceGuard << ")\n"
           << "#define " << namespaceGuard << "\n"
           << "QT_USE_NAMESPACE\n"
           << "#endif // " << namespaceGuard << "\n\n";

    stream << "TRACEPOINT_PROVIDER(" << provider.name << ");\n\n";
}

static void writeEpilogue(QTextStream &stream, const QString &fileName)
{
    stream << "\n";
    stream << "#endif // " << includeGuard(fileName) << "\n"
           << "#include <private/qtrace_p.h>\n";
}

static void writeWrapper(QTextStream &stream,
        const Tracepoint &tracepoint, const Provider &provider)
{
    const QString argList = formatFunctionSignature(tracepoint.args);
    const QString paramList = formatParameterList(provider, tracepoint.args, tracepoint.fields, CTF);
    const QString &name = tracepoint.name;
    const QString includeGuard = QStringLiteral("TP_%1_%2").arg(provider.name).arg(name).toUpper();

    /* prevents the redefinion of the inline wrapper functions
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

static void writeTracepoint(QTextStream &stream,
                            const Tracepoint &tracepoint, const QString &providerName)
{
    stream  << "TRACEPOINT_EVENT(\n"
            << "    " << providerName << ",\n"
            << "    " << tracepoint.name << ",\n";
    stream << "\"";

    const auto checkUnknownArgs = [](const Tracepoint &tracepoint) {
        for (auto &field : tracepoint.fields) {
            if (field.backendType.backendType == Tracepoint::Field::Unknown)
                return true;
        }
        return false;
    };

    const auto formatType = [](const QString &type) {
        QString ret = type;
        if (type.endsWith(QLatin1Char('*')) || type.endsWith(QLatin1Char('&')))
            ret = type.left(type.length() - 1).simplified();
        if (ret.startsWith(QStringLiteral("const")))
            ret = ret.right(ret.length() - 6).simplified();
        return typeToName(ret);
    };
    int eventSize = 0;
    bool variableSize = false;
    if (!checkUnknownArgs(tracepoint)) {
        for (int i = 0; i < tracepoint.args.size(); i++) {
            auto &arg = tracepoint.args[i];
            auto &field = tracepoint.fields[i];
            if (i > 0) {
                stream << " \\n\\\n";
                stream << "        ";
            }
            const bool array = field.arrayLen > 0;
            switch (field.backendType.backendType) {
            case Tracepoint::Field::Boolean: {
                stream << "Boolean " << arg.name << ";";
                eventSize += 8;
            } break;
            case Tracepoint::Field::Integer: {
                if (!field.backendType.isSigned)
                    stream << "u";
                stream << "int" << field.backendType.bits << "_t ";
                if (array)
                    stream << arg.name << "[" << field.arrayLen << "];";
                else
                    stream << arg.name << ";";
                eventSize += field.backendType.bits * qMax(1, field.arrayLen);
            } break;
            case Tracepoint::Field::Pointer: {
                if (QT_POINTER_SIZE == 8)
                    stream << "intptr64_t " << formatType(arg.type) << "_" << arg.name << ";";
                else
                    stream << "intptr32_t " << formatType(arg.type) << "_" << arg.name << ";";
                eventSize += QT_POINTER_SIZE * 8;
            } break;
            case Tracepoint::Field::IntegerHex: {
                if (field.backendType.bits == 64)
                    stream << "intptr64_t " << formatType(arg.name) << ";";
                else
                    stream << "intptr32_t " << formatType(arg.name) << ";";
                eventSize += field.backendType.bits;
            } break;
            case Tracepoint::Field::Float: {
                if (field.backendType.bits == 32)
                    stream << "float " << arg.name;
                else
                    stream << "double " << arg.name;
                if (array) {
                    stream << "[" << field.arrayLen << "];";
                } else {
                    stream << ";";
                }
                eventSize += field.backendType.bits * qMax(1, field.arrayLen);
            } break;
            case Tracepoint::Field::String: {
                stream << "string " << arg.name << ";";
                eventSize += 8;
                variableSize = true;
            } break;
            case Tracepoint::Field::QtString: {
                stream << "string " << arg.name << ";";
                eventSize += 8;
                variableSize = true;
            } break;
            case Tracepoint::Field::QtByteArray:
                break;
            case Tracepoint::Field::QtUrl: {
                stream << "string " << arg.name << ";";
                eventSize += 8;
                variableSize = true;
            } break;
            case Tracepoint::Field::QtRect: {
                stream << "int32_t QRect_" << arg.name << "_x;\\n\\\n        ";
                stream << "int32_t QRect_" << arg.name << "_y;\\n\\\n        ";
                stream << "int32_t QRect_" << arg.name << "_width;\\n\\\n        ";
                stream << "int32_t QRect_" << arg.name << "_height;";
                eventSize += 32 * 4;
            } break;
            case Tracepoint::Field::QtSize: {
                stream << "int32_t QSize_" << arg.name << "_width;\\n\\\n        ";
                stream << "int32_t QSize_" << arg.name << "_height;";
                eventSize += 32 * 2;
            } break;
            case Tracepoint::Field::Unknown:
                break;
            case Tracepoint::Field::EnumeratedType: {
                QString type = arg.type;
                type.replace(QStringLiteral("::"), QStringLiteral("_"));
                stream << type << " " << arg.name << ";";
                eventSize += field.backendType.bits;
                variableSize = true;
            } break;
            case Tracepoint::Field::FlagType: {
                QString type = arg.type;
                type.replace(QStringLiteral("::"), QStringLiteral("_"));
                stream << "uint8_t " << arg.name << "_length;\\n\\\n        ";
                stream << type << " " << arg.name << "[" << arg.name << "_length];";
                eventSize += 16;
            } break;
            case Tracepoint::Field::Sequence:
                panic("Unhandled sequence '%s %s", qPrintable(arg.type), qPrintable(arg.name));
                break;
            }
        }
    }

    stream << "\",\n";
    stream << eventSize / 8 << ", \n";
    stream << (variableSize ? "true" : "false") << "\n";
    stream << ")\n\n";
}

static void writeTracepoints(QTextStream &stream, const Provider &provider)
{
    for (const Tracepoint &t : provider.tracepoints) {
        writeTracepoint(stream, t, provider.name);
        writeWrapper(stream, t, provider);
    }
}

static void writeEnums(QTextStream &stream, const Provider &provider)
{
    for (const auto &e : provider.enumerations) {
        QString name = e.name;
        name.replace(QStringLiteral("::"), QStringLiteral("_"));
        stream << "TRACEPOINT_METADATA(" << provider.name << ", " << name << ", \n";
        stream << "\"typealias enum : integer { size = " << e.valueSize << "; } {\\n\\\n";
        for (const auto &v : e.values) {
            if (v.range)
                stream << v.name << " = " << v.value << " ... "  << v.range << ", \\n\\\n";
            else
                stream << v.name << " = " << v.value << ", \\n\\\n";
        }
        stream << "} := " << name << ";\\n\\n\");\n\n";
    }
    stream << "\n";
}

static void writeFlags(QTextStream &stream, const Provider &provider)
{
    for (const auto &e : provider.flags) {
        QString name = e.name;
        name.replace(QStringLiteral("::"), QStringLiteral("_"));
        stream << "TRACEPOINT_METADATA(" << provider.name << ", " << name << ", \n";
        stream << "\"typealias enum : integer { size = 8; } {\\n\\\n";
        for (const auto &v : e.values) {
            stream << v.name << " = " << v.value << ", \\n\\\n";
        }
        stream << "} := " << name << ";\\n\\n\");\n\n";
    }
    stream << "\n";
}

void writeCtf(QFile &file, const Provider &provider)
{
    QTextStream stream(&file);

    const QString fileName = QFileInfo(file.fileName()).fileName();

    writePrologue(stream, fileName, provider);
    writeEnums(stream, provider);
    writeFlags(stream, provider);
    writeTracepoints(stream, provider);
    writeEpilogue(stream, fileName);
}
