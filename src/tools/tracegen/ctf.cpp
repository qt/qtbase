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
    writeCommonPrologue(stream);
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


static void writeMetadataGenerators(QTextStream &stream)
{
    stream << R"CPP(
template <typename T>
inline QString integerToMetadata(const QString &name)
{
    QString ret;
    if (!std::is_signed<T>().value)
        ret += QLatin1Char('u');
    if (sizeof(T) == 8)
        ret += QStringLiteral("int64_t ");
    else if (sizeof(T) == 4)
        ret += QStringLiteral("int32_t ");
    else if (sizeof(T) == 2)
        ret += QStringLiteral("int16_t ");
    else if (sizeof(T) == 1)
        ret += QStringLiteral("int8_t ");
    ret += name + QLatin1Char(';');
    return ret;
}

template <typename T>
inline QString integerArrayToMetadata(const QString &size, const QString &name)
{
    QString ret;
    if (!std::is_signed<T>().value)
        ret += QLatin1Char('u');
    if (sizeof(T) == 8)
        ret += QStringLiteral("int64_t ");
    else if (sizeof(T) == 4)
        ret += QStringLiteral("int32_t ");
    else if (sizeof(T) == 2)
        ret += QStringLiteral("int16_t ");
    else if (sizeof(T) == 1)
        ret += QStringLiteral("int8_t ");
    ret += name + QLatin1Char('[') + size + QStringLiteral("];");
    return ret;
}

template <typename T>
inline QString floatToMetadata(const QString &name)
{
    QString ret;
    if (sizeof(T) == 8)
        ret += QStringLiteral("double ");
    else if (sizeof(T) == 4)
        ret += QStringLiteral("float ");
    ret += name + QLatin1Char(';');
    return ret;
}

template <typename T>
inline QString floatArrayToMetadata(const QString &size, const QString &name)
{
    QString ret;
    if (sizeof(T) == 8)
        ret += QStringLiteral("double ");
    else if (sizeof(T) == 4)
        ret += QStringLiteral("float ");
    ret += name + QLatin1Char('[') + size + QLatin1Char(']');
    return ret + QLatin1Char(';');
}

inline QString pointerToMetadata(const QString &name)
{
    QString ret;
    if (QT_POINTER_SIZE == 8)
        ret += QStringLiteral("intptr64_t ");
    else if (QT_POINTER_SIZE == 4)
        ret += QStringLiteral("intptr32_t ");
    ret += name + QLatin1Char(';');
    return ret;
}

)CPP";
}

static void writeTracepoint(QTextStream &stream,
                            const Tracepoint &tracepoint, const QString &providerName)
{
    stream  << "TRACEPOINT_EVENT(\n"
            << "    " << providerName << ",\n"
            << "    " << tracepoint.name << ",\n";

    const auto checkUnknownArgs = [](const Tracepoint &tracepoint) {
        for (auto &field : tracepoint.fields) {
            if (field.backendType == Tracepoint::Field::Unknown)
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
        return typeToTypeName(ret);
    };
    QString eventSize;
    bool variableSize = false;
    const bool emptyMetadata = checkUnknownArgs(tracepoint) || tracepoint.args.size() == 0;
    if (!emptyMetadata) {
        for (int i = 0; i < tracepoint.args.size(); i++) {
            auto &arg = tracepoint.args[i];
            auto &field = tracepoint.fields[i];
            if (i > 0) {
                stream << " + QStringLiteral(\"\\n\\\n        \") + ";
                eventSize += QStringLiteral(" + ");
            }
            const bool array = field.arrayLen > 0;
            switch (field.backendType) {
            case Tracepoint::Field::Boolean: {
                stream << "QStringLiteral(\"Boolean " << arg.name << ";\")";
                eventSize += QStringLiteral("sizeof(bool)");
            } break;
            case Tracepoint::Field::Integer: {
                if (array) {
                    stream << "integerArrayToMetadata<" << formatType(arg.type)
                           << ">(QStringLiteral(\"" << field.arrayLen << "\"), QStringLiteral(\""
                           << arg.name << "\"))";
                } else {
                    stream << "integerToMetadata<" << formatType(arg.type) << ">(QStringLiteral(\""
                           << arg.name << "\"))";
                }
                eventSize += QStringLiteral("sizeof(") + formatType(arg.type) + QStringLiteral(")");
                if (array)
                    eventSize += QStringLiteral(" * ") + QString::number(field.arrayLen);
            } break;
            case Tracepoint::Field::Pointer:
            case Tracepoint::Field::IntegerHex: {
                stream << "pointerToMetadata(QStringLiteral(\"" << formatType(arg.type) << "_"
                       << arg.name << "\"))";
                eventSize += QStringLiteral("QT_POINTER_SIZE");
            } break;
            case Tracepoint::Field::Float: {
                if (array) {
                    stream << "floatArrayToMetadata<" << formatType(arg.type)
                           << ">(QStringLiteral(\"" << field.arrayLen << "\"), QStringLiteral(\""
                           << arg.name << "\"))";
                } else {
                    stream << "floatToMetadata<" << formatType(arg.type) << ">(QStringLiteral(\""
                           << arg.name << "\"))";
                }
                eventSize += QStringLiteral("sizeof(") + formatType(arg.type) + QStringLiteral(")");
                if (array)
                    eventSize += QStringLiteral(" * ") + QString::number(field.arrayLen);
            } break;
            case Tracepoint::Field::QtUrl:
            case Tracepoint::Field::QtString:
            case Tracepoint::Field::String: {
                stream << "QStringLiteral(\"string " << arg.name << ";\")";
                eventSize += QStringLiteral("1");
                variableSize = true;
            } break;
            case Tracepoint::Field::QtRect: {
                stream << "QStringLiteral(\"int32_t QRect_" << arg.name << "_x;\\n\\\n        \")";
                stream << " + QStringLiteral(\"int32_t QRect_" << arg.name << "_y;\\n\\\n        \")";
                stream << " + QStringLiteral(\"int32_t QRect_" << arg.name << "_width;\\n\\\n        \")";
                stream << " + QStringLiteral(\"int32_t QRect_" << arg.name << "_height;\\n\\\n        \")";
                eventSize += QStringLiteral("16");
            } break;
            case Tracepoint::Field::QtSize: {
                stream << "QStringLiteral(\"int32_t QSize_" << arg.name << "_width;\\n\\\n        \")";
                stream << " + QStringLiteral(\"int32_t QSize_" << arg.name << "_height;\\n\\\n        \")";
                eventSize += QStringLiteral("8");
            } break;
            case Tracepoint::Field::QtRectF: {
                stream << "QStringLiteral(\"float QRectF_" << arg.name << "_x;\\n\\\n        \")";
                stream << " + QStringLiteral(\"float QRectF_" << arg.name << "_y;\\n\\\n        \")";
                stream << " + QStringLiteral(\"float QRectF_" << arg.name << "_width;\\n\\\n        \")";
                stream << " + QStringLiteral(\"float QRectF_" << arg.name << "_height;\\n\\\n        \")";
                eventSize += QStringLiteral("16");
            } break;
            case Tracepoint::Field::QtSizeF: {
                stream << "QStringLiteral(\"float QSizeF_" << arg.name << "_width;\\n\\\n        \")";
                stream << " + QStringLiteral(\"float QSizeF_" << arg.name << "_height;\\n\\\n        \")";
                eventSize += QStringLiteral("8");
            } break;
            case Tracepoint::Field::Unknown:
                break;
            case Tracepoint::Field::EnumeratedType: {
                stream << "QStringLiteral(\"" << typeToTypeName(arg.type) << " " << arg.name << ";\")";
                eventSize += QString::number(field.enumValueSize / 8);
                variableSize = true;
            } break;
            case Tracepoint::Field::FlagType: {
                stream << "QStringLiteral(\"uint8_t " << arg.name << "_length;\\n\\\n        ";
                stream << typeToTypeName(arg.type) << " " << arg.name << "[" << arg.name << "_length];\")";
                eventSize += QStringLiteral("2");
                variableSize = true;
            } break;
            case Tracepoint::Field::QtByteArray:
            case Tracepoint::Field::Sequence:
                panic("Unhandled type '%s %s", qPrintable(arg.type), qPrintable(arg.name));
                break;
            }
        }
    }

    if (emptyMetadata)
        stream << "{},\n";
    else
        stream << ",\n";
    if (eventSize.length())
        stream << eventSize << ", \n";
    else
        stream << "0, \n";
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
        stream << "QStringLiteral(\"typealias enum : integer { size = " << e.valueSize << "; } {\\n\\\n";

        const auto values = e.values;
        QList<int> handledValues;

        for (const auto &v : values) {
            if (handledValues.contains(v.value))
                continue;
            if (v.range) {
                stream << v.name << " = " << v.value << " ... "  << v.range << ", \\n\\\n";
            } else {
                const QString names = aggregateListValues(v.value, values);
                stream << names << " = " << v.value << ", \\n\\\n";
                handledValues.append(v.value);
            }
        }
        stream << "} := " << name << ";\\n\\n\"));\n\n";
    }
    stream << "\n";
}

static void writeFlags(QTextStream &stream, const Provider &provider)
{
    for (const auto &e : provider.flags) {
        QString name = e.name;
        name.replace(QStringLiteral("::"), QStringLiteral("_"));
        stream << "TRACEPOINT_METADATA(" << provider.name << ", " << name << ", \n";
        stream << "QStringLiteral(\"typealias enum : integer { size = 8; } {\\n\\\n";

        const auto values = e.values;
        QList<int> handledValues;

        for (const auto &v : values) {
            if (handledValues.contains(v.value))
                continue;
            const QString names = aggregateListValues(v.value, values);
            stream << names << " = " << v.value << ", \\n\\\n";
            handledValues.append(v.value);
        }
        stream << "} := " << name << ";\\n\\n\"));\n\n";
    }
    stream << "\n";
}

void writeCtf(QFile &file, const Provider &provider)
{
    QTextStream stream(&file);

    const QString fileName = QFileInfo(file.fileName()).fileName();

    writePrologue(stream, fileName, provider);
    writeMetadataGenerators(stream);
    writeEnums(stream, provider);
    writeFlags(stream, provider);
    writeTracepoints(stream, provider);
    writeEpilogue(stream, fileName);
}
