// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callback.h"
#include "provider.h"
#include "helpers.h"
#include "qtheaders.h"

#include <qfile.h>
#include <qfileinfo.h>
#include <qtextstream.h>

// The "callback" backend turns every tracepoint into a call to a single
// process-wide hook (qtTraceCallbackFunction), passing a pointer to a static
// descriptor of the firing point, and a vararg list of arguments. This is
// intended for in-process profilers that attribute time to whichever
// tracepoint is currently executing (entry/exit pairs form a stack), not for
// offline trace capture.

static void writePrologue(QTextStream &stream, const QString &fileName, const Provider &provider)
{
    writeCommonPrologue(stream);
    const QString guard = includeGuard(fileName);

    stream << "\n#include <private/qtracecallback_p.h>\n\n";

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
}

static void writeEpilogue(QTextStream &stream, const QString &fileName)
{
    stream << "\n";
    stream << "#endif // " << includeGuard(fileName) << "\n"
           << "#include <private/qtrace_p.h>\n";
}

static void writeWrapper(QTextStream &stream, const Tracepoint &tracepoint,
                         const Provider &provider)
{
    const QString argList = formatFunctionSignature(tracepoint.args);
    const QString paramList =
            formatParameterList(provider, tracepoint.args, tracepoint.fields, CALLBACK);
    const QString &name = tracepoint.name;
    const QString includeGuard = QStringLiteral("TP_%1_%2").arg(provider.name, name).toUpper();

    stream << "\n"
           << "#ifndef " << includeGuard << "\n"
           << "#define " << includeGuard << "\n"
           << "QT_BEGIN_NAMESPACE\n"
           << "namespace QtPrivate {\n";

    const auto writeBody = [&](const char *enabledCheck) {
        stream << "{\n";

        // Constant-initialised aggregate: no thread-safe-init guard, and a unique stable
        // address per tracepoint that the callback can key on. cookie starts at -1.
        // The callback function has to perform any thread synchronization itself.
        stream << "    static QTraceCallbackTracepoint qtp { \"" << provider.name << "\", \""
               << name << "\", -1 };\n"
               << "    " << enabledCheck << "\n"
               << "        qtTraceCallbackFunction(&qtp" << (paramList.isEmpty() ? "" : ", ")
               << paramList << ");\n"
               << "}\n";
    };

    stream << "inline void do_trace_" << name << "(" << argList << ")\n";
    writeBody("if (qtTraceCallbackFunction)");

    stream << "inline void trace_" << name << "(" << argList << ") { do_trace_" << name << "("
           << paramList << "); }\n";

    stream << "inline bool trace_" << name << "_enabled()\n"
           << "{\n"
           << "    return qtTraceCallbackFunction != nullptr;\n"
           << "}\n";

    stream << "} // namespace QtPrivate\n"
           << "QT_END_NAMESPACE\n"
           << "#endif // " << includeGuard << "\n";
}

void writeCallback(QFile &file, const Provider &provider)
{
    QTextStream stream(&file);

    const QString fileName = QFileInfo(file.fileName()).fileName();

    writePrologue(stream, fileName, provider);
    for (const Tracepoint &t : provider.tracepoints)
        writeWrapper(stream, t, provider);
    writeEpilogue(stream, fileName);
}
