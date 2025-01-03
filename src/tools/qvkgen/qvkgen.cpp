// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qcoreapplication.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qxmlstream.h>

// generate wrappers for core functions from the following versions
static const QStringList VERSIONS = {
    QStringLiteral("VK_VERSION_1_0"), // must be the first and always present
    QStringLiteral("VK_VERSION_1_1"),
    QStringLiteral("VK_VERSION_1_2"),
    QStringLiteral("VK_VERSION_1_3")
};

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
        QList<TypedName> args;
        bool deviceLevel;
    };

    QList<Command> commands() const { return m_commands; }
    QMap<QString, QStringList> versionCommandMapping() const { return m_versionCommandMapping; }

    void setFileName(const QString &fn) { m_fn = fn; }

private:
    void skip();
    void parseFeature();
    void parseFeatureRequire(const QString &versionDefine);
    void parseCommands();
    Command parseCommand();
    TypedName parseParamOrProto(const QString &tag);
    QString parseName();

    QFile m_file;
    QXmlStreamReader m_reader;
    QList<Command> m_commands;
    QMap<QString, QStringList> m_versionCommandMapping; // "1.0" -> ["vkGetPhysicalDeviceProperties", ...]
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

    m_commands.clear();
    m_versionCommandMapping.clear();

    while (!m_reader.atEnd()) {
        m_reader.readNext();
        if (m_reader.isStartElement()) {
            if (m_reader.name() == QStringLiteral("commands"))
                parseCommands();
            else if (m_reader.name() == QStringLiteral("feature"))
                parseFeature();
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

void VkSpecParser::parseFeature()
{
    // <feature api="vulkan" name="VK_VERSION_1_0" number="1.0" comment="Vulkan core API interface definitions">
    //   <require comment="Device initialization">

    QString api;
    QString versionName;
    for (const QXmlStreamAttribute &attr : m_reader.attributes()) {
        if (attr.name() == QStringLiteral("api"))
            api = attr.value().toString().trimmed();
        else if (attr.name() == QStringLiteral("name"))
            versionName = attr.value().toString().trimmed();
    }
    const bool isVulkan = api == QStringLiteral("vulkan");

    while (!m_reader.atEnd()) {
        m_reader.readNext();
        if (m_reader.isEndElement() && m_reader.name() == QStringLiteral("feature"))
            return;
        if (m_reader.isStartElement() && m_reader.name() == QStringLiteral("require")) {
            if (isVulkan)
                parseFeatureRequire(versionName);
        }
    }
}

void VkSpecParser::parseFeatureRequire(const QString &versionDefine)
{
    // <require comment="Device initialization">
    //   <command name="vkCreateInstance"/>

    while (!m_reader.atEnd()) {
        m_reader.readNext();
        if (m_reader.isEndElement() && m_reader.name() == QStringLiteral("require"))
            return;
        if (m_reader.isStartElement() && m_reader.name() == QStringLiteral("command")) {
            for (const QXmlStreamAttribute &attr : m_reader.attributes()) {
                if (attr.name() == QStringLiteral("name"))
                    m_versionCommandMapping[versionDefine].append(attr.value().toString().trimmed());
            }
        }
    }
}

void VkSpecParser::parseCommands()
{
    // <commands comment="Vulkan command definitions">
    //   <command successcodes="VK_SUCCESS" ...>

    while (!m_reader.atEnd()) {
        m_reader.readNext();
        if (m_reader.isEndElement() && m_reader.name() == QStringLiteral("commands"))
            return;
        if (m_reader.isStartElement() && m_reader.name() == QStringLiteral("command")) {
            const Command c = parseCommand();
            if (!c.cmd.name.isEmpty()) // skip aliases
                m_commands.append(c);
        }
    }
}

VkSpecParser::Command VkSpecParser::parseCommand()
{
    Command c;

    // <command successcodes="VK_SUCCESS" ...>
    //   <proto><type>VkResult</type> <name>vkCreateInstance</name></proto>
    //   <param>const <type>VkInstanceCreateInfo</type>* <name>pCreateInfo</name></param>
    //   <param optional="true">const <type>VkAllocationCallbacks</type>* <name>pAllocator</name></param>
    //   <param><type>VkInstance</type>* <name>pInstance</name></param>

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
            auto text = m_reader.text().trimmed();
            if (!text.isEmpty()) {
                if (text.startsWith(u'[')) {
                    t.typeSuffix += text;
                } else {
                    if (!t.type.isEmpty())
                        t.type += u' ';
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
        s += u'(';
        bool first = true;
        for (const VkSpecParser::TypedName &a : c.args) {
            if (!first)
                s += QStringLiteral(", ");
            else
                first = false;
            s += QString::asprintf("%s%s%s%s", qPrintable(a.type),
                                   (a.type.endsWith(u'*') ? "" : " "),
                                   qPrintable(a.name), qPrintable(a.typeSuffix));
        }
        s += u')';
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
        s += u'(';
        bool first = true;
        for (const VkSpecParser::TypedName &a : c.args) {
            if (!first)
                s += QStringLiteral(", ");
            else
                first = false;
            s += a.name;
        }
        s += u')';
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

bool genVulkanFunctionsH(const QList<VkSpecParser::Command> &commands,
                         const QMap<QString, QStringList> &versionCommandMapping,
                         const QString &licHeaderFn,
                         const QString &outputBase)
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
"#if 0\n"
"#pragma qt_no_master_include\n"
"#endif\n"
"\n"
"#include <QtGui/qtguiglobal.h>\n"
"\n"
"#if QT_CONFIG(vulkan) || defined(Q_QDOC)\n"
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
"#endif // QT_CONFIG(vulkan) || defined(Q_QDOC)\n"
"\n"
"#endif // QVULKANFUNCTIONS_H\n";

    QString instCmdStr;
    QString devCmdStr;
    for (const QString &version : VERSIONS) {
        const QStringList &coreFunctionsInVersion = versionCommandMapping[version];
        instCmdStr += "#if " + version + "\n";
        devCmdStr += "#if " + version + "\n";
        for (const VkSpecParser::Command &c : commands) {
            if (!coreFunctionsInVersion.contains(c.cmd.name))
                continue;

            QString *dst = c.deviceLevel ? &devCmdStr : &instCmdStr;
            *dst += QStringLiteral("    ");
            *dst += funcSig(c);
            *dst += QStringLiteral(";\n");
        }
        instCmdStr += "#endif\n";
        devCmdStr += "#endif\n";
    }

    f.write(QString::asprintf(s, preamble.get(licHeaderFn).constData(),
                              instCmdStr.toUtf8().constData(),
                              devCmdStr.toUtf8().constData()).toUtf8());

    return true;
}

bool genVulkanFunctionsPH(const QList<VkSpecParser::Command> &commands,
                          const QMap<QString, QStringList> &versionCommandMapping,
                          const QString &licHeaderFn,
                          const QString &outputBase)
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

    int devLevelCount = 0;
    int instLevelCount = 0;
    for (const QString &version : VERSIONS) {
        const QStringList &coreFunctionsInVersion = versionCommandMapping[version];
        for (const VkSpecParser::Command &c : commands) {
            if (!coreFunctionsInVersion.contains(c.cmd.name))
                continue;

            if (c.deviceLevel)
                devLevelCount += 1;
            else
                instLevelCount += 1;
        }
    }

    f.write(QString::asprintf(s, preamble.get(licHeaderFn).constData(), instLevelCount, devLevelCount).toUtf8());

    return true;
}

bool genVulkanFunctionsPC(const QList<VkSpecParser::Command> &commands,
                          const QMap<QString, QStringList> &versionCommandMapping,
                          const QString &licHeaderFn,
                          const QString &outputBase)
{
    QFile f(outputBase + QStringLiteral("_p.cpp"));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning("Failed to write %s", qPrintable(f.fileName()));
        return false;
    }

    static const char s[] =
"%s\n"
"#include \"qvulkanfunctions_p.h\"\n"
"#include \"qvulkaninstance.h\"\n"
"\n"
"#include <QtCore/private/qoffsetstringarray_p.h>\n"
"\n"
"QT_BEGIN_NAMESPACE\n"
"\n%s"
"QVulkanFunctionsPrivate::QVulkanFunctionsPrivate(QVulkanInstance *inst)\n"
"{\n"
"    static constexpr auto funcNames = qOffsetStringArray(\n"
"%s\n"
"    );\n"
"    static_assert(std::extent_v<decltype(m_funcs)> == size_t(funcNames.count()));\n"
"    for (int i = 0; i < funcNames.count(); ++i) {\n"
"        m_funcs[i] = inst->getInstanceProcAddr(funcNames.at(i));\n"
"        if (i < %d && !m_funcs[i])\n"
"            qWarning(\"QVulkanFunctions: Failed to resolve %%s\", funcNames.at(i));\n"
"    }\n"
"}\n"
"\n%s"
"QVulkanDeviceFunctionsPrivate::QVulkanDeviceFunctionsPrivate(QVulkanInstance *inst, VkDevice device)\n"
"{\n"
"    QVulkanFunctions *f = inst->functions();\n"
"    Q_ASSERT(f);\n\n"
"    static constexpr auto funcNames = qOffsetStringArray(\n"
"%s\n"
"    );\n"
"    static_assert(std::extent_v<decltype(m_funcs)> == size_t(funcNames.count()));\n"
"    for (int i = 0; i < funcNames.count(); ++i) {\n"
"        m_funcs[i] = f->vkGetDeviceProcAddr(device, funcNames.at(i));\n"
"        if (i < %d && !m_funcs[i])\n"
"            qWarning(\"QVulkanDeviceFunctions: Failed to resolve %%s\", funcNames.at(i));\n"
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
    int vulkan10DevCount = 0;
    int vulkan10InstCount = 0;

    for (const QString &version : VERSIONS) {
        const QStringList &coreFunctionsInVersion = versionCommandMapping[version];
        instCmdWrapperStr += "\n#if " + version + "\n";
        devCmdWrapperStr += "\n#if " + version + "\n";
        for (const VkSpecParser::Command &c : commands) {
            if (!coreFunctionsInVersion.contains(c.cmd.name))
                continue;

            QString *dst = c.deviceLevel ? &devCmdWrapperStr : &instCmdWrapperStr;
            int *idx = c.deviceLevel ? &devIdx : &instIdx;
            *dst += funcSig(c, c.deviceLevel ? "QVulkanDeviceFunctions" : "QVulkanFunctions");
            *dst += QString(QStringLiteral("\n{\n    Q_ASSERT(d_ptr->m_funcs[%1]);\n    ")).arg(*idx);
            *dst += funcCall(c, *idx);
            *dst += QStringLiteral(";\n}\n\n");
            *idx += 1;

            dst = c.deviceLevel ? &devCmdNamesStr : &instCmdNamesStr;
            *dst += QStringLiteral("        \"");
            *dst += c.cmd.name;
            *dst += QStringLiteral("\",\n");
        }
        if (version == QStringLiteral("VK_VERSION_1_0")) {
            vulkan10InstCount = instIdx;
            vulkan10DevCount = devIdx;
        }
        instCmdWrapperStr += "#endif\n\n";
        devCmdWrapperStr += "#endif\n\n";
    }

    if (devCmdNamesStr.size() > 2)
        devCmdNamesStr.chop(2);
    if (instCmdNamesStr.size() > 2)
        instCmdNamesStr.chop(2);

    const QString str =
            QString::asprintf(s, preamble.get(licHeaderFn).constData(),
                              instCmdWrapperStr.toUtf8().constData(),
                              instCmdNamesStr.toUtf8().constData(), vulkan10InstCount,
                              devCmdWrapperStr.toUtf8().constData(),
                              devCmdNamesStr.toUtf8().constData(), vulkan10DevCount);

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

    // Now we have a list of functions (commands), including extensions, and a
    // table of Version (1.0, 1.1, 1.2) -> Core functions in that version.
    QList<VkSpecParser::Command> commands = parser.commands();
    QMap<QString, QStringList> versionCommandMapping = parser.versionCommandMapping();

    QStringList ignoredFuncs {
        QStringLiteral("vkCreateInstance"),
        QStringLiteral("vkDestroyInstance"),
        QStringLiteral("vkGetInstanceProcAddr"),
        QStringLiteral("vkEnumerateInstanceVersion")
    };
    for (int i = 0; i < commands.size(); ++i) {
        if (ignoredFuncs.contains(commands[i].cmd.name))
            commands.remove(i--);
    }

    QString licenseHeaderFileName = QString::fromUtf8(argv[2]);
    QString outputBase = QString::fromUtf8(argv[3]);
    genVulkanFunctionsH(commands, versionCommandMapping, licenseHeaderFileName, outputBase);
    genVulkanFunctionsPH(commands, versionCommandMapping, licenseHeaderFileName, outputBase);
    genVulkanFunctionsPC(commands, versionCommandMapping, licenseHeaderFileName, outputBase);

    return 0;
}
