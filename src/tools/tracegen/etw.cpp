// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "etw.h"
#include "provider.h"
#include "helpers.h"
#include "qtheaders.h"

#include <qfile.h>
#include <qfileinfo.h>
#include <qtextstream.h>
#include <quuid.h>

using namespace Qt::StringLiterals;

static inline QString providerVar(const QString &providerName)
{
    return providerName + "_provider"_L1;
}

static void writeEtwMacro(QTextStream &stream, const Tracepoint::Field &field)
{
    const QString &name = field.name;

    if (field.arrayLen > 0) {
        for (int i = 0; i < field.arrayLen; i++) {
            stream << "TraceLoggingValue(" << name << "[" << i << "], \"" << name << "[" << i << "]\")";
            if (i + 1 < field.arrayLen)
                stream << ",\n        ";
        }
        return;
    }

    switch (field.backendType) {
    case Tracepoint::Field::QtString:
        stream << "TraceLoggingCountedWideString(reinterpret_cast<LPCWSTR>("
               << name << ".utf16()), static_cast<ULONG>(" << name << ".size()), \""
               << name << "\")";
        return;
    case Tracepoint::Field::QtByteArray:
        stream << "TraceLoggingBinary(" << name << ".constData(), "
               << name << ".size(), \"" << name << "\")";
        return;
    case Tracepoint::Field::QtUrl:
        stream << "TraceLoggingValue(" << name << ".toEncoded().constData(), \"" << name << "\")";
        return;
    case Tracepoint::Field::QtRect:
    case Tracepoint::Field::QtRectF:
        stream << "TraceLoggingValue(" << name << ".x(), \"x\"), "
               << "TraceLoggingValue(" << name << ".y(), \"y\"), "
               << "TraceLoggingValue(" << name << ".width(), \"width\"), "
               << "TraceLoggingValue(" << name << ".height(), \"height\")";
        return;
    case Tracepoint::Field::QtSize:
    case Tracepoint::Field::QtSizeF:
        stream << "TraceLoggingValue(" << name << ".width(), \"width\"), "
               << "TraceLoggingValue(" << name << ".height(), \"height\")";
        return;
    case Tracepoint::Field::Pointer:
        stream << "TraceLoggingPointer(" << name << ", \"" << name << "\")";
        return;
    case Tracepoint::Field::Unknown:
        // Write down the previously stringified data (like we do for QString).
        // The string is already created in writeWrapper().
        // Variable name is name##Str.
        stream << "TraceLoggingCountedWideString(reinterpret_cast<LPCWSTR>(" << name
               << "Str.utf16()), static_cast<ULONG>(" << name << "Str.size()), \"" << name << "\")";
        return;
    case Tracepoint::Field::EnumeratedType:
    case Tracepoint::Field::FlagType:
        stream << "TraceLoggingString(trace_convert_" << typeToTypeName(field.paramType) << "(" << name << ").toUtf8().constData(), \"" << name << "\")";
        return;
    default:
        break;
    }

    stream << "TraceLoggingValue(" << name << ", \"" << name << "\")";
}

static QString createGuid(const QUuid &uuid)
{
    QString guid;

    QTextStream stream(&guid);

    Qt::hex(stream);

    stream << "("
           << "0x" << uuid.data1    << ", "
           << "0x" << uuid.data2    << ", "
           << "0x" << uuid.data3    << ", "
           << "0x" << uuid.data4[0] << ", "
           << "0x" << uuid.data4[1] << ", "
           << "0x" << uuid.data4[2] << ", "
           << "0x" << uuid.data4[3] << ", "
           << "0x" << uuid.data4[4] << ", "
           << "0x" << uuid.data4[5] << ", "
           << "0x" << uuid.data4[6] << ", "
           << "0x" << uuid.data4[7]
           << ")";

    return guid;
}

static void writePrologue(QTextStream &stream, const QString &fileName, const Provider &provider)
{
    writeCommonPrologue(stream);
    QUuid uuid = QUuid::createUuidV5(QUuid(), provider.name.toLocal8Bit());

    const QString providerV = providerVar(provider.name);
    const QString guard = includeGuard(fileName);
    const QString guid = createGuid(uuid);
    const QString guidString = uuid.toString();

    stream << "#ifndef " << guard << "\n"
           << "#define " << guard << "\n"
           << "\n"
           << "#include <qt_windows.h>\n"
           << "#include <TraceLoggingProvider.h>\n"
           << "\n";

    /* TraceLogging API macros cannot deal with UTF8
     * source files, so we work around it like this
     */
    stream << "#undef _TlgPragmaUtf8Begin\n"
              "#undef _TlgPragmaUtf8End\n"
              "#define _TlgPragmaUtf8Begin\n"
              "#define _TlgPragmaUtf8End\n";

    stream << "\n";
    stream << qtHeaders();
    stream << "\n";

    if (!provider.prefixText.isEmpty())
        stream << provider.prefixText.join(u'\n') << "\n\n";

    stream << "#ifdef TRACEPOINT_DEFINE\n"
           << "/* " << guidString << " */\n"
           << "TRACELOGGING_DEFINE_PROVIDER(" << providerV << ", \""
           << provider.name <<"\", " << guid << ");\n\n";

    stream << "static inline void registerProvider()\n"
           << "{\n"
           << "    TraceLoggingRegister(" << providerV << ");\n"
           << "}\n\n";

    stream << "static inline void unregisterProvider()\n"
           << "{\n"
           << "    TraceLoggingUnregister(" << providerV << ");\n"
           << "}\n";

    stream << "Q_CONSTRUCTOR_FUNCTION(registerProvider)\n"
           << "Q_DESTRUCTOR_FUNCTION(unregisterProvider)\n\n";

    stream << "#else\n"
           << "TRACELOGGING_DECLARE_PROVIDER(" << providerV << ");\n"
           << "#endif // TRACEPOINT_DEFINE\n\n";
}

static void writeEpilogue(QTextStream &stream, const QString &fileName)
{
    stream << "\n#endif // " << includeGuard(fileName) << "\n"
           << "#include <private/qtrace_p.h>\n";
}

static void writeWrapper(QTextStream &stream, const Provider &provider, const Tracepoint &tracepoint,
                         const QString &providerName)
{
    const QString argList = formatFunctionSignature(tracepoint.args);
    const QString paramList = formatParameterList(provider, tracepoint.args, tracepoint.fields, ETW);
    const QString &name = tracepoint.name;
    const QString includeGuard = QStringLiteral("TP_%1_%2").arg(providerName).arg(name).toUpper();
    const QString provar = providerVar(providerName);

    stream << "\n";

    stream << "inline void trace_" << name << "(" << argList << ")\n"
           << "{\n";

    // Convert all unknown types to QString's using QDebug.
    // Note the naming convention: it's field.name##Str
    for (const Tracepoint::Field &field : tracepoint.fields) {
        if (field.backendType == Tracepoint::Field::Unknown) {
            stream << "    const QString " << field.name << "Str = QDebug::toString(" << field.name
                   << ");\n";
        }
    }
    stream << "    TraceLoggingWrite(" << provar << ", \"" << name << "\"";

    for (const Tracepoint::Field &field : tracepoint.fields) {
        stream << ",\n";
        stream << "        ";
        writeEtwMacro(stream, field);
    }

    stream << ");\n"
           << "}\n\n";

    stream << "inline void do_trace_" << name << "(" << argList << ")\n"
           << "{\n"
           << "    trace_" << name << "(" << paramList << ");\n"
           << "}\n";

    stream << "inline bool trace_" << name << "_enabled()\n"
           << "{\n"
           << "    return TraceLoggingProviderEnabled(" << provar << ", 0, 0);\n"
           << "}\n";
}

static void writeEnumConverter(QTextStream &stream, const TraceEnum &enumeration)
{
    stream << "inline QString trace_convert_" << typeToTypeName(enumeration.name) << "(" << enumeration.name << " val)\n";
    stream << "{\n";
    for (const auto &v : enumeration.values) {
        if (v.range != 0) {
            stream << "    if (val >= " << v.value << " && val <= " << v.range << ")\n"
                   << "        return QStringLiteral(\"" << v.name << " + \") + QString::number((int)val - " << v.value << ");\n";
        }
    }
    stream << "\n    QString ret;\n    switch (val) {\n";

    QList<int> handledValues;
    for (const auto &v : enumeration.values) {
        if (v.range == 0 && !handledValues.contains(v.value)) {
            stream << "    case " << v.value << ": ret = QStringLiteral(\""
                   << aggregateListValues(v.value, enumeration.values) << "\"); break;\n";
            handledValues.append(v.value);
        }
    }

    stream << "    }\n    return ret;\n}\n";
}

static void writeFlagConverter(QTextStream &stream, const TraceFlags &flag)
{
    stream << "inline QString trace_convert_" << typeToTypeName(flag.name) << "(" << flag.name << " val)\n";
    stream << "{\n    QString ret;\n";
    for (const auto &v : flag.values) {
        if (v.value == 0) {
            stream << "    if (val == 0)\n        return QStringLiteral(\""
                   << aggregateListValues(v.value, flag.values) << "\");\n";
            break;
        }
    }
    QList<int> handledValues;
    for (const auto &v : flag.values) {
        if (v.value != 0 && !handledValues.contains(v.value)) {
            stream << "    if (val & " << (1 << (v.value - 1))
                   << ") { if (ret.length()) ret += QLatin1Char(\'|\'); ret += QStringLiteral(\"" << v.name << "\"); }\n";
            handledValues.append(v.value);
        }
    }
    stream << "    return ret;\n}\n";
}

static void writeTracepoints(QTextStream &stream, const Provider &provider)
{
    if (provider.tracepoints.isEmpty())
        return;

    const QString includeGuard = QStringLiteral("TP_%1_PROVIDER").arg(provider.name).toUpper();

    stream << "#if !defined(" << includeGuard << ") && !defined(TRACEPOINT_DEFINE)\n"
           << "#define " << includeGuard << "\n"
           << "QT_BEGIN_NAMESPACE\n"
           << "namespace QtPrivate {\n";

    for (const auto &enumeration : provider.enumerations)
        writeEnumConverter(stream, enumeration);

    for (const auto &flag : provider.flags)
        writeFlagConverter(stream, flag);

    for (const Tracepoint &t : provider.tracepoints)
        writeWrapper(stream, provider, t, provider.name);

    stream << "} // namespace QtPrivate\n"
           << "QT_END_NAMESPACE\n"
           << "#endif // " << includeGuard << "\n\n";
}

void writeEtw(QFile &file, const Provider &provider)
{
    QTextStream stream(&file);

    const QString fileName = QFileInfo(file.fileName()).fileName();

    writePrologue(stream, fileName, provider);
    writeTracepoints(stream, provider);
    writeEpilogue(stream, fileName);
}

