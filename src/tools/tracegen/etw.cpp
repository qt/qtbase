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

#include "etw.h"
#include "provider.h"
#include "helpers.h"
#include "qtheaders.h"

#include <qfile.h>
#include <qfileinfo.h>
#include <qtextstream.h>
#include <quuid.h>

static inline QString providerVar(const QString &providerName)
{
    return providerName + QLatin1String("_provider");
}

static void writeEtwMacro(QTextStream &stream, const Tracepoint::Field &field)
{
    const QString &name = field.name;

    switch (field.backendType) {
    case Tracepoint::Field::QtString:
        stream << "TraceLoggingCountedWideString(reinterpret_cast<LPCWSTR>("
               << name << ".utf16()), " << name << ".size(), \"" << name << "\")";
        return;
    case Tracepoint::Field::QtByteArray:
        stream << "TraceLoggingBinary(" << name << ".constData(), "
               << name << ".size(), \"" << name << "\")";
        return;
    case Tracepoint::Field::QtUrl:
        stream << "TraceLoggingValue(" << name << ".toEncoded().constData(), \"" << name << "\")";
        return;
    case Tracepoint::Field::QtRect:
        stream << "TraceLoggingValue(" << name << ".x(), \"x\"), "
               << "TraceLoggingValue(" << name << ".y(), \"y\"), "
               << "TraceLoggingValue(" << name << ".width(), \"width\"), "
               << "TraceLoggingValue(" << name << ".height(), \"height\")";
        return;
    case Tracepoint::Field::Pointer:
        stream << "TraceLoggingPointer(" << name << ", \"" << name << "\")";
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
    QUuid uuid = QUuid::createUuidV5(QUuid(), provider.name.toLocal8Bit());

    const QString providerV = providerVar(provider.name);
    const QString guard = includeGuard(fileName);
    const QString guid = createGuid(uuid);
    const QString guidString = uuid.toString();

    stream << "#ifndef " << guard << "\n"
           << "#define " << guard << "\n"
           << "\n"
           << "#include <windows.h>\n"
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
        stream << provider.prefixText.join(QLatin1Char('\n')) << "\n\n";

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

static void writeWrapper(QTextStream &stream, const Tracepoint &tracepoint,
                         const QString &providerName)
{
    const QString argList = formatFunctionSignature(tracepoint.args);
    const QString paramList = formatParameterList(tracepoint.args, ETW);
    const QString &name = tracepoint.name;
    const QString includeGuard = QStringLiteral("TP_%1_%2").arg(providerName).arg(name).toUpper();
    const QString provider = providerVar(providerName);

    stream << "\n";

    stream << "inline void trace_" << name << "(" << argList << ")\n"
           << "{\n"
           << "    TraceLoggingWrite(" << provider << ", \"" << name << "\"";

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
           << "    return TraceLoggingProviderEnabled(" << provider << ", 0, 0);\n"
           << "}\n";
}

static void writeTracepoints(QTextStream &stream, const Provider &provider)
{
    if (provider.tracepoints.isEmpty())
        return;

    const QString includeGuard = QStringLiteral("TP_%1_PROVIDER").arg(provider.name).toUpper();

    stream << "#if !defined(" << includeGuard << ") && !defined(TRACEPOINT_DEFINE)\n"
           << "#define " << includeGuard << "\n"
           << "namespace QtPrivate {\n";

    for (const Tracepoint &t : provider.tracepoints)
        writeWrapper(stream, t, provider.name);

    stream << "} // namespace QtPrivate\n"
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

