/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qcoreapplication.h>
#include <QtCore/qvector.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qxmlstream.h>

class VkSpecParser
{
public:
    bool parse();

    struct TypedName {
        QString name;
        QString type;
        QString typeSuffix;
    };

    struct Command {
        TypedName cmd;
        QVector<TypedName> args;
        bool deviceLevel;
    };

    QVector<Command> commands() const { return m_commands; }

    void setFileName(const QString &fn) { m_fn = fn; }

private:
    void skip();
    void parseCommands();
    Command parseCommand();
    TypedName parseParamOrProto(const QString &tag);
    QString parseName();

    QFile m_file;
    QXmlStreamReader m_reader;
    QVector<Command> m_commands;
    QString m_fn;
};

bool VkSpecParser::parse()
{
    m_file.setFileName(m_fn);
    if (!m_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("Failed to open %s", qPrintable(m_file.fileName()));
        return false;
    }

    m_reader.setDevice(&m_file);
    while (!m_reader.atEnd()) {
        m_reader.readNext();
        if (m_reader.isStartElement()) {
            if (m_reader.name() == QStringLiteral("commands"))
                parseCommands();
        }
    }

    return true;
}

void VkSpecParser::skip()
{
    QString tag = m_reader.name().toString();
    while (!m_reader.atEnd()) {
        m_reader.readNext();
        if (m_reader.isEndElement() && m_reader.name() == tag)
            break;
    }
}

void VkSpecParser::parseCommands()
{
    m_commands.clear();

    while (!m_reader.atEnd()) {
        m_reader.readNext();
        if (m_reader.isEndElement() && m_reader.name() == QStringLiteral("commands"))
            return;
        if (m_reader.isStartElement() && m_reader.name() == "command")
            m_commands.append(parseCommand());
    }
}

VkSpecParser::Command VkSpecParser::parseCommand()
{
    Command c;

    while (!m_reader.atEnd()) {
        m_reader.readNext();
        if (m_reader.isEndElement() && m_reader.name() == QStringLiteral("command"))
            break;
        if (m_reader.isStartElement()) {
            const QString protoStr = QStringLiteral("proto");
            const QString paramStr = QStringLiteral("param");
            if (m_reader.name() == protoStr) {
                c.cmd = parseParamOrProto(protoStr);
            } else if (m_reader.name() == paramStr) {
                c.args.append(parseParamOrProto(paramStr));
            } else {
                skip();
            }
        }
    }

    c.deviceLevel = false;
    if (!c.args.isEmpty()) {
        QStringList dispatchableDeviceAndChildTypes {
            QStringLiteral("VkDevice"),
            QStringLiteral("VkQueue"),
            QStringLiteral("VkCommandBuffer")
        };
        if (dispatchableDeviceAndChildTypes.contains(c.args[0].type)
                && c.cmd.name != QStringLiteral("vkGetDeviceProcAddr"))
        {
            c.deviceLevel = true;
        }
    }

    return c;
}

VkSpecParser::TypedName VkSpecParser::parseParamOrProto(const QString &tag)
{
    TypedName t;

    while (!m_reader.atEnd()) {
        m_reader.readNext();
        if (m_reader.isEndElement() && m_reader.name() == tag)
            break;
        if (m_reader.isStartElement()) {
            if (m_reader.name() == QStringLiteral("name")) {
                t.name = parseName();
            } else if (m_reader.name() != QStringLiteral("type")) {
                skip();
            }
        } else {
            QStringRef text = m_reader.text().trimmed();
            if (!text.isEmpty()) {
                if (text.startsWith(QLatin1Char('['))) {
                    t.typeSuffix += text;
                } else {
                    if (!t.type.isEmpty())
                        t.type += QLatin1Char(' ');
                    t.type += text;
                }
            }
        }
    }

    return t;
}

QString VkSpecParser::parseName()
{
    QString name;
    while (!m_reader.atEnd()) {
        m_reader.readNext();
        if (m_reader.isEndElement() && m_reader.name() == QStringLiteral("name"))
            break;
        name += m_reader.text();
    }
    return name.trimmed();
}

QString funcSig(const VkSpecParser::Command &c, const char *className = nullptr)
{
    QString s(QString::asprintf("%s %s%s%s", qPrintable(c.cmd.type),
                                (className ? className : ""), (className ? "::" : ""),
                                qPrintable(c.cmd.name)));
    if (!c.args.isEmpty()) {
        s += QLatin1Char('(');
        bool first = true;
        for (const VkSpecParser::TypedName &a : c.args) {
            if (!first)
                s += QStringLiteral(", ");
            else
                first = false;
            s += QString::asprintf("%s%s%s%s", qPrintable(a.type),
                                   (a.type.endsWith(QLatin1Char('*')) ? "" : " "),
                                   qPrintable(a.name), qPrintable(a.typeSuffix));
        }
        s += QLatin1Char(')');
    }
    return s;
}

QString funcCall(const VkSpecParser::Command &c, int idx)
{
    // template:
    //     [return] reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(d_ptr->m_funcs[0])(instance, pPhysicalDeviceCount, pPhysicalDevices);
    QString s = QString::asprintf("%sreinterpret_cast<PFN_%s>(d_ptr->m_funcs[%d])",
                                  (c.cmd.type == QStringLiteral("void") ? "" : "return "),
                                  qPrintable(c.cmd.name),
                                  idx);
    if (!c.args.isEmpty()) {
        s += QLatin1Char('(');
        bool first = true;
        for (const VkSpecParser::TypedName &a : c.args) {
            if (!first)
                s += QStringLiteral(", ");
            else
                first = false;
            s += a.name;
        }
        s += QLatin1Char(')');
    }
    return s;
}

class Preamble
{
public:
    QByteArray get(const QString &fn);

private:
    QByteArray m_str;
} preamble;

QByteArray Preamble::get(const QString &fn)
{
    if (!m_str.isEmpty())
        return m_str;

    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("Failed to open %s", qPrintable(fn));
        return m_str;
    }

    m_str = f.readAll();
    m_str.replace("FOO", "QtGui");
    m_str += "\n// This file is automatically generated by qvkgen. Do not edit.\n";

    return m_str;
}

bool genVulkanFunctionsH(const QVector<VkSpecParser::Command> &commands, const QString &licHeaderFn, const QString &outputBase)
{
    QFile f(outputBase + QStringLiteral(".h"));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning("Failed to write %s", qPrintable(f.fileName()));
        return false;
    }

    static const char *s =
"%s\n"
"#ifndef QVULKANFUNCTIONS_H\n"
"#define QVULKANFUNCTIONS_H\n"
"\n"
"#include <QtGui/qtguiglobal.h>\n"
"\n"
"#if QT_CONFIG(vulkan) || defined(Q_CLANG_QDOC)\n"
"\n"
"#ifndef VK_NO_PROTOTYPES\n"
"#define VK_NO_PROTOTYPES\n"
"#endif\n"
"#include <vulkan/vulkan.h>\n"
"\n"
"#include <QtCore/qscopedpointer.h>\n"
"\n"
"QT_BEGIN_NAMESPACE\n"
"\n"
"class QVulkanInstance;\n"
"class QVulkanFunctionsPrivate;\n"
"class QVulkanDeviceFunctionsPrivate;\n"
"\n"
"class Q_GUI_EXPORT QVulkanFunctions\n"
"{\n"
"public:\n"
"    ~QVulkanFunctions();\n"
"\n"
"%s\n"
"private:\n"
"    Q_DISABLE_COPY(QVulkanFunctions)\n"
"    QVulkanFunctions(QVulkanInstance *inst);\n"
"\n"
"    QScopedPointer<QVulkanFunctionsPrivate> d_ptr;\n"
"    friend class QVulkanInstance;\n"
"};\n"
"\n"
"class Q_GUI_EXPORT QVulkanDeviceFunctions\n"
"{\n"
"public:\n"
"    ~QVulkanDeviceFunctions();\n"
"\n"
"%s\n"
"private:\n"
"    Q_DISABLE_COPY(QVulkanDeviceFunctions)\n"
"    QVulkanDeviceFunctions(QVulkanInstance *inst, VkDevice device);\n"
"\n"
"    QScopedPointer<QVulkanDeviceFunctionsPrivate> d_ptr;\n"
"    friend class QVulkanInstance;\n"
"};\n"
"\n"
"QT_END_NAMESPACE\n"
"\n"
"#endif // QT_CONFIG(vulkan) || defined(Q_CLANG_QDOC)\n"
"\n"
"#endif // QVULKANFUNCTIONS_H\n";

    QString instCmdStr;
    QString devCmdStr;
    for (const VkSpecParser::Command &c : commands) {
        QString *dst = c.deviceLevel ? &devCmdStr : &instCmdStr;
        *dst += QStringLiteral("    ");
        *dst += funcSig(c);
        *dst += QStringLiteral(";\n");
    }

    f.write(QString::asprintf(s, preamble.get(licHeaderFn).constData(),
                              instCmdStr.toUtf8().constData(),
                              devCmdStr.toUtf8().constData()).toUtf8());

    return true;
}

bool genVulkanFunctionsPH(const QVector<VkSpecParser::Command> &commands, const QString &licHeaderFn, const QString &outputBase)
{
    QFile f(outputBase + QStringLiteral("_p.h"));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning("Failed to write %s", qPrintable(f.fileName()));
        return false;
    }

    static const char *s =
"%s\n"
"#ifndef QVULKANFUNCTIONS_P_H\n"
"#define QVULKANFUNCTIONS_P_H\n"
"\n"
"//\n"
"//  W A R N I N G\n"
"//  -------------\n"
"//\n"
"// This file is not part of the Qt API.  It exists purely as an\n"
"// implementation detail.  This header file may change from version to\n"
"// version without notice, or even be removed.\n"
"//\n"
"// We mean it.\n"
"//\n"
"\n"
"#include \"qvulkanfunctions.h\"\n"
"\n"
"QT_BEGIN_NAMESPACE\n"
"\n"
"class QVulkanInstance;\n"
"\n"
"class QVulkanFunctionsPrivate\n"
"{\n"
"public:\n"
"    QVulkanFunctionsPrivate(QVulkanInstance *inst);\n"
"\n"
"    PFN_vkVoidFunction m_funcs[%d];\n"
"};\n"
"\n"
"class QVulkanDeviceFunctionsPrivate\n"
"{\n"
"public:\n"
"    QVulkanDeviceFunctionsPrivate(QVulkanInstance *inst, VkDevice device);\n"
"\n"
"    PFN_vkVoidFunction m_funcs[%d];\n"
"};\n"
"\n"
"QT_END_NAMESPACE\n"
"\n"
"#endif // QVULKANFUNCTIONS_P_H\n";

    const int devLevelCount = std::count_if(commands.cbegin(), commands.cend(),
                                            [](const VkSpecParser::Command &c) { return c.deviceLevel; });
    const int instLevelCount = commands.count() - devLevelCount;

    f.write(QString::asprintf(s, preamble.get(licHeaderFn).constData(), instLevelCount, devLevelCount).toUtf8());

    return true;
}

bool genVulkanFunctionsPC(const QVector<VkSpecParser::Command> &commands, const QString &licHeaderFn, const QString &outputBase)
{
    QFile f(outputBase + QStringLiteral("_p.cpp"));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning("Failed to write %s", qPrintable(f.fileName()));
        return false;
    }

    static const char *s =
"%s\n"
"#include \"qvulkanfunctions_p.h\"\n"
"#include \"qvulkaninstance.h\"\n"
"\n"
"QT_BEGIN_NAMESPACE\n"
"\n%s"
"QVulkanFunctionsPrivate::QVulkanFunctionsPrivate(QVulkanInstance *inst)\n"
"{\n"
"    static const char *funcNames[] = {\n"
"%s\n"
"    };\n"
"    for (int i = 0; i < %d; ++i) {\n"
"        m_funcs[i] = inst->getInstanceProcAddr(funcNames[i]);\n"
"        if (!m_funcs[i])\n"
"            qWarning(\"QVulkanFunctions: Failed to resolve %%s\", funcNames[i]);\n"
"    }\n"
"}\n"
"\n%s"
"QVulkanDeviceFunctionsPrivate::QVulkanDeviceFunctionsPrivate(QVulkanInstance *inst, VkDevice device)\n"
"{\n"
"    QVulkanFunctions *f = inst->functions();\n"
"    Q_ASSERT(f);\n\n"
"    static const char *funcNames[] = {\n"
"%s\n"
"    };\n"
"    for (int i = 0; i < %d; ++i) {\n"
"        m_funcs[i] = f->vkGetDeviceProcAddr(device, funcNames[i]);\n"
"        if (!m_funcs[i])\n"
"            qWarning(\"QVulkanDeviceFunctions: Failed to resolve %%s\", funcNames[i]);\n"
"    }\n"
"}\n"
"\n"
"QT_END_NAMESPACE\n";

    QString devCmdWrapperStr;
    QString instCmdWrapperStr;
    int devIdx = 0;
    int instIdx = 0;
    QString devCmdNamesStr;
    QString instCmdNamesStr;

    for (int i = 0; i < commands.count(); ++i) {
        QString *dst = commands[i].deviceLevel ? &devCmdWrapperStr : &instCmdWrapperStr;
        int *idx = commands[i].deviceLevel ? &devIdx : &instIdx;
        *dst += funcSig(commands[i], commands[i].deviceLevel ? "QVulkanDeviceFunctions" : "QVulkanFunctions");
        *dst += QString(QStringLiteral("\n{\n    Q_ASSERT(d_ptr->m_funcs[%1]);\n    ")).arg(*idx);
        *dst += funcCall(commands[i], *idx);
        *dst += QStringLiteral(";\n}\n\n");
        ++*idx;

        dst = commands[i].deviceLevel ? &devCmdNamesStr : &instCmdNamesStr;
        *dst += QStringLiteral("        \"");
        *dst += commands[i].cmd.name;
        *dst += QStringLiteral("\",\n");
    }

    if (devCmdNamesStr.count() > 2)
        devCmdNamesStr = devCmdNamesStr.left(devCmdNamesStr.count() - 2);
    if (instCmdNamesStr.count() > 2)
        instCmdNamesStr = instCmdNamesStr.left(instCmdNamesStr.count() - 2);

    const QString str =
            QString::asprintf(s, preamble.get(licHeaderFn).constData(),
                              instCmdWrapperStr.toUtf8().constData(),
                              instCmdNamesStr.toUtf8().constData(), instIdx,
                              devCmdWrapperStr.toUtf8().constData(),
                              devCmdNamesStr.toUtf8().constData(), commands.count() - instIdx);

    f.write(str.toUtf8());

    return true;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    VkSpecParser parser;

    if (argc < 4) {
        qWarning("Usage: qvkgen input_vk_xml input_license_header output_base\n"
                 "  For example: qvkgen vulkan/vk.xml vulkan/qvulkanfunctions.header vulkan/qvulkanfunctions");
        return 1;
    }

    parser.setFileName(QString::fromUtf8(argv[1]));

    if (!parser.parse())
        return 1;

    QVector<VkSpecParser::Command> commands = parser.commands();
    QStringList ignoredFuncs {
        QStringLiteral("vkCreateInstance"),
        QStringLiteral("vkDestroyInstance"),
        QStringLiteral("vkGetInstanceProcAddr")
    };

    // Filter out extensions and unwanted functions.
    // The check for the former is rather simplistic for now: skip if the last letter is uppercase...
    for (int i = 0; i < commands.count(); ++i) {
        QString name = commands[i].cmd.name;
        QChar c = name[name.count() - 1];
        if (c.isUpper() || ignoredFuncs.contains(name))
            commands.remove(i--);
    }

    QString licenseHeaderFileName = QString::fromUtf8(argv[2]);
    QString outputBase = QString::fromUtf8(argv[3]);
    genVulkanFunctionsH(commands, licenseHeaderFileName, outputBase);
    genVulkanFunctionsPH(commands, licenseHeaderFileName, outputBase);
    genVulkanFunctionsPC(commands, licenseHeaderFileName, outputBase);

    return 0;
}
