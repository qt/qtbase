// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:critical reason:network-protocol

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>

#include <vector>

class Scanner
{
public:
    explicit Scanner() {}
    ~Scanner() { delete m_xml; }

    bool parseArguments(int argc, char **argv);
    void printUsage();
    bool process();
    void printErrors();

private:
    struct WaylandEnumEntry {
        QByteArray name;
        QByteArray value;
        QByteArray summary;
    };

    struct WaylandEnum {
        QByteArray name;

        std::vector<WaylandEnumEntry> entries;
    };

    struct WaylandArgument {
        QByteArray name;
        QByteArray type;
        QByteArray interface;
        QByteArray summary;
        bool allowNull;
    };

    struct WaylandEvent {
        bool request;
        QByteArray name;
        QByteArray type;
        std::vector<WaylandArgument> arguments;
    };

    struct WaylandInterface {
        QByteArray name;
        int version;

        std::vector<WaylandEnum> enums;
        std::vector<WaylandEvent> events;
        std::vector<WaylandEvent> requests;
    };

    bool isServerSide();
    bool parseOption(const QByteArray &str);

    QByteArray byteArrayValue(const QXmlStreamReader &xml, const char *name);
    int intValue(const QXmlStreamReader &xml, const char *name, int defaultValue = 0);
    bool boolValue(const QXmlStreamReader &xml, const char *name);
    WaylandEvent readEvent(QXmlStreamReader &xml, bool request);
    Scanner::WaylandEnum readEnum(QXmlStreamReader &xml);
    Scanner::WaylandInterface readInterface(QXmlStreamReader &xml);
    QByteArray waylandToCType(const QByteArray &waylandType, const QByteArray &interface);
    QByteArray waylandToQtType(const QByteArray &waylandType, const QByteArray &interface, bool cStyleArray);
    const Scanner::WaylandArgument *newIdArgument(const std::vector<WaylandArgument> &arguments);

    void printEvent(const WaylandEvent &e, bool omitNames = false, bool withResource = false);
    void printEventHandlerSignature(const WaylandEvent &e, const char *interfaceName, bool deepIndent = true);
    void printEnums(const std::vector<WaylandEnum> &enums);

    QByteArray stripInterfaceName(const QByteArray &name);
    bool ignoreInterface(const QByteArray &name);

    enum Option {
        ClientHeader,
        ServerHeader,
        ClientCode,
        ServerCode
    } m_option;

    QByteArray m_protocolName;
    QByteArray m_protocolFilePath;
    QByteArray m_scannerName;
    QByteArray m_headerPath;
    QByteArray m_prefix;
    QByteArray m_buildMacro;
    QList <QByteArray> m_includes;
    QXmlStreamReader *m_xml = nullptr;
};

bool Scanner::parseArguments(int argc, char **argv)
{
    QList<QByteArray> args;
    args.reserve(argc);
    for (int i = 0; i < argc; ++i)
        args << QByteArray(argv[i]);

    m_scannerName = args[0];

    if (argc <= 2 || !parseOption(args[1]))
        return false;

    m_protocolFilePath = args[2];

    if (argc > 3 && !args[3].startsWith('-')) {
        // legacy positional arguments
            m_headerPath = args[3];
        if (argc == 5)
            m_prefix = args[4];
    } else {
        // --header-path=<path> (14 characters)
        // --prefix=<prefix> (9 characters)
        // --add-include=<include> (14 characters)
        for (int pos = 3; pos < argc; pos++) {
            const QByteArray &option = args[pos];
            if (option.startsWith("--header-path=")) {
                m_headerPath = option.mid(14);
            } else if (option.startsWith("--prefix=")) {
                m_prefix = option.mid(10);
            } else if (option.startsWith("--build-macro=")) {
                m_buildMacro = option.mid(14);
            } else if (option.startsWith("--add-include=")) {
                auto include = option.mid(14);
                if (!include.isEmpty())
                    m_includes << include;
            } else {
                return false;
            }
        }
    }

    return true;
}

void Scanner::printUsage()
{
    fprintf(stderr, "Usage: %s [client-header|server-header|client-code|server-code] specfile [--header-path=<path>] [--prefix=<prefix>] [--add-include=<include>]\n", m_scannerName.constData());
}

bool Scanner::isServerSide()
{
    return m_option == ServerHeader || m_option == ServerCode;
}

bool Scanner::parseOption(const QByteArray &str)
{
    if (str == "client-header")
        m_option = ClientHeader;
    else if (str == "server-header")
        m_option = ServerHeader;
    else if (str == "client-code")
        m_option = ClientCode;
    else if (str == "server-code")
        m_option = ServerCode;
    else
        return false;

    return true;
}

QByteArray Scanner::byteArrayValue(const QXmlStreamReader &xml, const char *name)
{
    if (xml.attributes().hasAttribute(name))
        return xml.attributes().value(name).toUtf8();
    return QByteArray();
}

int Scanner::intValue(const QXmlStreamReader &xml, const char *name, int defaultValue)
{
    bool ok;
    int result = byteArrayValue(xml, name).toInt(&ok);
    return ok ? result : defaultValue;
}

bool Scanner::boolValue(const QXmlStreamReader &xml, const char *name)
{
    return byteArrayValue(xml, name) == "true";
}

Scanner::WaylandEvent Scanner::readEvent(QXmlStreamReader &xml, bool request)
{
    WaylandEvent event = {
        .request = request,
        .name = byteArrayValue(xml, "name"),
        .type = byteArrayValue(xml, "type"),
        .arguments = {},
    };
    while (xml.readNextStartElement()) {
        if (xml.name() == u"arg") {
            WaylandArgument argument = {
                .name      = byteArrayValue(xml, "name"),
                .type      = byteArrayValue(xml, "type"),
                .interface = byteArrayValue(xml, "interface"),
                .summary   = byteArrayValue(xml, "summary"),
                .allowNull = boolValue(xml, "allow-null"),
            };
            event.arguments.push_back(std::move(argument));
        }

        xml.skipCurrentElement();
    }
    return event;
}

Scanner::WaylandEnum Scanner::readEnum(QXmlStreamReader &xml)
{
    WaylandEnum result = {
        .name = byteArrayValue(xml, "name"),
        .entries = {},
    };

    while (xml.readNextStartElement()) {
        if (xml.name() == u"entry") {
            WaylandEnumEntry entry = {
                .name    = byteArrayValue(xml, "name"),
                .value   = byteArrayValue(xml, "value"),
                .summary = byteArrayValue(xml, "summary"),
            };
            result.entries.push_back(std::move(entry));
        }

        xml.skipCurrentElement();
    }

    return result;
}

Scanner::WaylandInterface Scanner::readInterface(QXmlStreamReader &xml)
{
    WaylandInterface interface = {
        .name = byteArrayValue(xml, "name"),
        .version = intValue(xml, "version", 1),
        .enums = {},
        .events = {},
        .requests = {},
    };

    while (xml.readNextStartElement()) {
        if (xml.name() == u"event")
            interface.events.push_back(readEvent(xml, false));
        else if (xml.name() == u"request")
            interface.requests.push_back(readEvent(xml, true));
        else if (xml.name() == u"enum")
            interface.enums.push_back(readEnum(xml));
        else
            xml.skipCurrentElement();
    }

    return interface;
}

QByteArray Scanner::waylandToCType(const QByteArray &waylandType, const QByteArray &interface)
{
    if (waylandType == "string")
        return "const char *";
    else if (waylandType == "int")
        return "int32_t";
    else if (waylandType == "uint")
        return "uint32_t";
    else if (waylandType == "fixed")
        return "wl_fixed_t";
    else if (waylandType == "fd")
        return "int32_t";
    else if (waylandType == "array")
        return "wl_array *";
    else if (waylandType == "object" || waylandType == "new_id") {
        if (isServerSide())
            return "struct ::wl_resource *";
        if (interface.isEmpty())
            return "struct ::wl_object *";
        return "struct ::" + interface + " *";
    }
    return waylandType;
}

QByteArray Scanner::waylandToQtType(const QByteArray &waylandType, const QByteArray &interface, bool cStyleArray)
{
    if (waylandType == "string")
        return "const QString &";
    else if (waylandType == "array")
        return cStyleArray ? "wl_array *" : "const QByteArray &";
    else
        return waylandToCType(waylandType, interface);
}

const Scanner::WaylandArgument *Scanner::newIdArgument(const std::vector<WaylandArgument> &arguments)
{
    for (const WaylandArgument &a : arguments) {
        if (a.type == "new_id")
            return &a;
    }
    return nullptr;
}

void Scanner::printEvent(const WaylandEvent &e, bool omitNames, bool withResource)
{
    printf("%s(", e.name.constData());
    bool needsComma = false;
    if (isServerSide()) {
        if (e.request) {
            printf("Resource *%s", omitNames ? "" : "resource");
            needsComma = true;
        } else if (withResource) {
            printf("struct ::wl_resource *%s", omitNames ? "" : "resource");
            needsComma = true;
        }
    }
    for (const WaylandArgument &a : e.arguments) {
        bool isNewId = a.type == "new_id";
        if (isNewId && !isServerSide() && (a.interface.isEmpty() != e.request))
            continue;
        if (needsComma)
            printf(", ");
        needsComma = true;
        if (isNewId) {
            if (isServerSide()) {
                if (e.request) {
                    printf("uint32_t");
                    if (!omitNames)
                        printf(" %s", a.name.constData());
                    continue;
                }
            } else {
                if (e.request) {
                    printf("const struct ::wl_interface *%s, uint32_t%s", omitNames ? "" : "interface", omitNames ? "" : " version");
                    continue;
                }
            }
        }

        QByteArray qtType = waylandToQtType(a.type, a.interface, e.request == isServerSide());
        printf("%s%s%s", qtType.constData(), qtType.endsWith("&") || qtType.endsWith("*") ? "" : " ", omitNames ? "" : a.name.constData());
    }
    printf(")");
}

void Scanner::printEventHandlerSignature(const WaylandEvent &e, const char *interfaceName, bool deepIndent)
{
    const char *indent = deepIndent ? "    " : "";
    printf("handle_%s(\n", e.name.constData());
    if (isServerSide()) {
        printf("        %s::wl_client *client,\n", indent);
        printf("        %sstruct wl_resource *resource", indent);
    } else {
        printf("        %svoid *data,\n", indent);
        printf("        %sstruct ::%s *object", indent, interfaceName);
    }
    for (const WaylandArgument &a : e.arguments) {
        printf(",\n");
        bool isNewId = a.type == "new_id";
        if (isServerSide() && isNewId) {
            printf("        %suint32_t %s", indent, a.name.constData());
        } else {
            QByteArray cType = waylandToCType(a.type, a.interface);
            printf("        %s%s%s%s", indent, cType.constData(), cType.endsWith("*") ? "" : " ", a.name.constData());
        }
    }
    printf(")");
}

void Scanner::printEnums(const std::vector<WaylandEnum> &enums)
{
    for (const WaylandEnum &e : enums) {
        printf("\n");
        printf("        enum %s {\n", e.name.constData());
        for (const WaylandEnumEntry &entry : e.entries) {
            printf("            %s_%s = %s,", e.name.constData(), entry.name.constData(), entry.value.constData());
            if (!entry.summary.isNull())
                printf(" // %s", entry.summary.constData());
            printf("\n");
        }
        printf("        };\n");
    }
}

QByteArray Scanner::stripInterfaceName(const QByteArray &name)
{
    if (!m_prefix.isEmpty() && name.startsWith(m_prefix))
        return name.mid(m_prefix.size());
    if (name.startsWith("qt_") || name.startsWith("wl_"))
        return name.mid(3);

    return name;
}

bool Scanner::ignoreInterface(const QByteArray &name)
{
    return name == "wl_display"
           || (isServerSide() && name == "wl_registry");
}

bool Scanner::process()
{
    QFile file(m_protocolFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        fprintf(stderr, "Unable to open file %s\n", m_protocolFilePath.constData());
        return false;
    }

    m_xml = new QXmlStreamReader(&file);
    if (!m_xml->readNextStartElement())
        return false;

    if (m_xml->name() != u"protocol") {
        m_xml->raiseError(QStringLiteral("The file is not a wayland protocol file."));
        return false;
    }

    m_protocolName = byteArrayValue(*m_xml, "name");

    if (m_protocolName.isEmpty()) {
        m_xml->raiseError(QStringLiteral("Missing protocol name."));
        return false;
    }

    //We should convert - to _ so that the preprocessor wont generate code which will lead to unexpected behavior
    //However, the wayland-scanner doesn't do so we will do the same for now
    //QByteArray preProcessorProtocolName = QByteArray(m_protocolName).replace('-', '_').toUpper();
    QByteArray preProcessorProtocolName = QByteArray(m_protocolName).toUpper();

    const QByteArray fileBaseName = QFileInfo(file).completeBaseName().toLocal8Bit();

    std::vector<WaylandInterface> interfaces;

    while (m_xml->readNextStartElement()) {
        if (m_xml->name() == u"interface")
            interfaces.push_back(readInterface(*m_xml));
        else
            m_xml->skipCurrentElement();
    }

    if (m_xml->hasError())
        return false;

    printf("// This file was generated by qtwaylandscanner\n");
    printf("// source file is %s\n\n", qPrintable(QFileInfo(file).fileName()));

    for (auto b : std::as_const(m_includes))
        printf("#include %s\n", b.constData());

    auto printExportMacro = [this](const char *prefix, const QByteArray &preProcessorProtocolName) {
        QByteArray exportMacro = prefix + preProcessorProtocolName + "_EXPORT";
        printf("#if !defined(%s)\n", exportMacro.constData());
        printf("#  if defined(QT_SHARED) && !defined(QT_STATIC)\n");
        if (m_buildMacro.isEmpty()) {
            printf("#    define %s Q_DECL_EXPORT\n", exportMacro.constData());
        } else {
            printf("#    if defined(%s)\n", m_buildMacro.constData());
            printf("#      define %s Q_DECL_EXPORT\n", exportMacro.constData());
            printf("#    else\n");
            printf("#      define %s Q_DECL_IMPORT\n", exportMacro.constData());
            printf("#    endif\n");
        }
        printf("#  else\n");
        printf("#    define %s\n", exportMacro.constData());
        printf("#  endif\n");
        printf("#endif\n");
        return exportMacro;
    };

    if (m_option == ServerHeader) {
        QByteArray inclusionGuard = QByteArray("QT_WAYLAND_SERVER_") + preProcessorProtocolName.constData();
        printf("#ifndef %s\n", inclusionGuard.constData());
        printf("#define %s\n", inclusionGuard.constData());
        printf("\n");
        printf("#include \"wayland-server-core.h\"\n");
        if (m_headerPath.isEmpty())
            printf("#include \"wayland-%s-server-protocol.h\"\n", fileBaseName.constData());
        else
            printf("#include <%s/wayland-%s-server-protocol.h>\n", m_headerPath.constData(), fileBaseName.constData());
        printf("#include <QByteArray>\n");
        printf("#include <QMultiMap>\n");
        printf("#include <QString>\n");

        printf("\n");
        printf("#ifndef WAYLAND_VERSION_CHECK\n");
        printf("#define WAYLAND_VERSION_CHECK(major, minor, micro) \\\n");
        printf("    ((WAYLAND_VERSION_MAJOR > (major)) || \\\n");
        printf("    (WAYLAND_VERSION_MAJOR == (major) && WAYLAND_VERSION_MINOR > (minor)) || \\\n");
        printf("    (WAYLAND_VERSION_MAJOR == (major) && WAYLAND_VERSION_MINOR == (minor) && WAYLAND_VERSION_MICRO >= (micro)))\n");
        printf("#endif\n");

        printf("\n");
        printf("QT_BEGIN_NAMESPACE\n");
        printf("QT_WARNING_PUSH\n");
        printf("QT_WARNING_DISABLE_GCC(\"-Wmissing-field-initializers\")\n");
        printf("QT_WARNING_DISABLE_CLANG(\"-Wmissing-field-initializers\")\n");
        QByteArray serverExport;
        if (m_headerPath.size())
            serverExport = printExportMacro("Q_WAYLAND_SERVER_", preProcessorProtocolName);
        printf("\n");
        printf("namespace QtWaylandServer {\n");

        bool needsNewLine = false;
        for (const WaylandInterface &interface : interfaces) {

            if (ignoreInterface(interface.name))
                continue;

            if (needsNewLine)
                printf("\n");
            needsNewLine = true;

            const char *interfaceName = interface.name.constData();

            QByteArray stripped = stripInterfaceName(interface.name);
            const char *interfaceNameStripped = stripped.constData();

            printf("    class %s %s\n    {\n", serverExport.constData(), interfaceName);
            printf("    public:\n");
            printf("        %s(struct ::wl_client *client, uint32_t id, int version);\n", interfaceName);
            printf("        %s(struct ::wl_display *display, int version);\n", interfaceName);
            printf("        %s(struct ::wl_resource *resource);\n", interfaceName);
            printf("        %s();\n", interfaceName);
            printf("\n");
            printf("        virtual ~%s();\n", interfaceName);
            printf("\n");
            printf("        class Resource\n");
            printf("        {\n");
            printf("        public:\n");
            printf("            Resource() : %s_object(nullptr), handle(nullptr) {}\n", interfaceNameStripped);
            printf("            virtual ~Resource() {}\n");
            printf("\n");
            printf("            %s *%s_object;\n", interfaceName, interfaceNameStripped);
            printf("            %s *object() { return %s_object; } \n", interfaceName, interfaceNameStripped);
            printf("            struct ::wl_resource *handle;\n");
            printf("\n");
            printf("            struct ::wl_client *client() const { return wl_resource_get_client(handle); }\n");
            printf("            int version() const { return wl_resource_get_version(handle); }\n");
            printf("\n");
            printf("            static Resource *fromResource(struct ::wl_resource *resource);\n");
            printf("        };\n");
            printf("\n");
            printf("        void init(struct ::wl_client *client, uint32_t id, int version);\n");
            printf("        void init(struct ::wl_display *display, int version);\n");
            printf("        void init(struct ::wl_resource *resource);\n");
            printf("\n");
            printf("        Resource *add(struct ::wl_client *client, int version);\n");
            printf("        Resource *add(struct ::wl_client *client, uint32_t id, int version);\n");
            printf("        Resource *add(struct wl_list *resource_list, struct ::wl_client *client, uint32_t id, int version);\n");
            printf("\n");
            printf("        Resource *resource() { return m_resource; }\n");
            printf("        const Resource *resource() const { return m_resource; }\n");
            printf("\n");
            printf("        QMultiMap<struct ::wl_client*, Resource*> resourceMap() { return m_resource_map; }\n");
            printf("        const QMultiMap<struct ::wl_client*, Resource*> resourceMap() const { return m_resource_map; }\n");
            printf("\n");
            printf("        bool isGlobal() const { return m_global != nullptr; }\n");
            printf("        bool isResource() const { return m_resource != nullptr; }\n");
            printf("\n");
            printf("        static const struct ::wl_interface *interface();\n");
            printf("        static QByteArray interfaceName() { return interface()->name; }\n");
            printf("        static int interfaceVersion() { return interface()->version; }\n");
            printf("\n");

            printEnums(interface.enums);

            bool hasEvents = !interface.events.empty();

            if (hasEvents) {
                printf("\n");
                for (const WaylandEvent &e : interface.events) {
                    printf("        void send_");
                    printEvent(e);
                    printf(";\n");
                    printf("        void send_");
                    printEvent(e, false, true);
                    printf(";\n");
                }
            }

            printf("\n");
            printf("    protected:\n");
            printf("        virtual Resource *%s_allocate();\n", interfaceNameStripped);
            printf("\n");
            printf("        virtual void %s_bind_resource(Resource *resource);\n", interfaceNameStripped);
            printf("        virtual void %s_destroy_resource(Resource *resource);\n", interfaceNameStripped);

            bool hasRequests = !interface.requests.empty();

            if (hasRequests) {
                printf("\n");
                for (const WaylandEvent &e : interface.requests) {
                    printf("        virtual void %s_", interfaceNameStripped);
                    printEvent(e);
                    printf(";\n");
                }
            }

            printf("\n");
            printf("    private:\n");
            printf("        static void bind_func(struct ::wl_client *client, void *data, uint32_t version, uint32_t id);\n");
            printf("        static void destroy_func(struct ::wl_resource *client_resource);\n");
            printf("        static void display_destroy_func(struct ::wl_listener *listener, void *data);\n");
            printf("\n");
            printf("        Resource *bind(struct ::wl_client *client, uint32_t id, int version);\n");
            printf("        Resource *bind(struct ::wl_resource *handle);\n");

            if (hasRequests) {
                printf("\n");
                printf("        static const struct ::%s_interface m_%s_interface;\n", interfaceName, interfaceName);

                printf("\n");
                for (const WaylandEvent &e : interface.requests) {
                    printf("        static void ");

                    printEventHandlerSignature(e, interfaceName);
                    printf(";\n");
                }
            }

            printf("\n");
            printf("        QMultiMap<struct ::wl_client*, Resource*> m_resource_map;\n");
            printf("        Resource *m_resource;\n");
            printf("        struct ::wl_global *m_global;\n");
            printf("        struct DisplayDestroyedListener : ::wl_listener {\n");
            printf("            %s *parent;\n", interfaceName);
            printf("        };\n");
            printf("        DisplayDestroyedListener m_displayDestroyedListener;\n");
            printf("    };\n");
        }

        printf("}\n");
        printf("\n");
        printf("QT_WARNING_POP\n");
        printf("QT_END_NAMESPACE\n");
        printf("\n");
        printf("#endif\n");
    }

    if (m_option == ServerCode) {
        if (m_headerPath.isEmpty())
            printf("#include \"qwayland-server-%s.h\"\n", fileBaseName.constData());
        else
            printf("#include <%s/qwayland-server-%s.h>\n", m_headerPath.constData(), fileBaseName.constData());
        printf("\n");
        printf("QT_BEGIN_NAMESPACE\n");
        printf("QT_WARNING_PUSH\n");
        printf("QT_WARNING_DISABLE_GCC(\"-Wmissing-field-initializers\")\n");
        printf("QT_WARNING_DISABLE_CLANG(\"-Wmissing-field-initializers\")\n");
        printf("\n");
        printf("namespace QtWaylandServer {\n");

        bool needsNewLine = false;
        for (const WaylandInterface &interface : interfaces) {

            if (ignoreInterface(interface.name))
                continue;

            if (needsNewLine)
                printf("\n");

            needsNewLine = true;

            const char *interfaceName = interface.name.constData();

            QByteArray stripped = stripInterfaceName(interface.name);
            const char *interfaceNameStripped = stripped.constData();

            printf("    %s::%s(struct ::wl_client *client, uint32_t id, int version)\n", interfaceName, interfaceName);
            printf("        : m_resource_map()\n");
            printf("        , m_resource(nullptr)\n");
            printf("        , m_global(nullptr)\n");
            printf("    {\n");
            printf("        init(client, id, version);\n");
            printf("    }\n");
            printf("\n");

            printf("    %s::%s(struct ::wl_display *display, int version)\n", interfaceName, interfaceName);
            printf("        : m_resource_map()\n");
            printf("        , m_resource(nullptr)\n");
            printf("        , m_global(nullptr)\n");
            printf("    {\n");
            printf("        init(display, version);\n");
            printf("    }\n");
            printf("\n");

            printf("    %s::%s(struct ::wl_resource *resource)\n", interfaceName, interfaceName);
            printf("        : m_resource_map()\n");
            printf("        , m_resource(nullptr)\n");
            printf("        , m_global(nullptr)\n");
            printf("    {\n");
            printf("        init(resource);\n");
            printf("    }\n");
            printf("\n");

            printf("    %s::%s()\n", interfaceName, interfaceName);
            printf("        : m_resource_map()\n");
            printf("        , m_resource(nullptr)\n");
            printf("        , m_global(nullptr)\n");
            printf("    {\n");
            printf("    }\n");
            printf("\n");

            printf("    %s::~%s()\n", interfaceName, interfaceName);
            printf("    {\n");
            printf("        for (auto resource : std::as_const(m_resource_map))\n");
            printf("            resource->%s_object = nullptr;\n", interfaceNameStripped);
            printf("\n");
            printf("        if (m_resource)\n");
            printf("            m_resource->%s_object = nullptr;\n", interfaceNameStripped);
            printf("\n");
            printf("        if (m_global) {\n");
            printf("            wl_global_destroy(m_global);\n");
            printf("            wl_list_remove(&m_displayDestroyedListener.link);\n");
            printf("        }\n");
            printf("    }\n");
            printf("\n");

            printf("    void %s::init(struct ::wl_client *client, uint32_t id, int version)\n", interfaceName);
            printf("    {\n");
            printf("        m_resource = bind(client, id, version);\n");
            printf("    }\n");
            printf("\n");

            printf("    void %s::init(struct ::wl_resource *resource)\n", interfaceName);
            printf("    {\n");
            printf("        m_resource = bind(resource);\n");
            printf("    }\n");
            printf("\n");

            printf("    %s::Resource *%s::add(struct ::wl_client *client, int version)\n", interfaceName, interfaceName);
            printf("    {\n");
            printf("        Resource *resource = bind(client, 0, version);\n");
            printf("        m_resource_map.insert(client, resource);\n");
            printf("        return resource;\n");
            printf("    }\n");
            printf("\n");

            printf("    %s::Resource *%s::add(struct ::wl_client *client, uint32_t id, int version)\n", interfaceName, interfaceName);
            printf("    {\n");
            printf("        Resource *resource = bind(client, id, version);\n");
            printf("        m_resource_map.insert(client, resource);\n");
            printf("        return resource;\n");
            printf("    }\n");
            printf("\n");

            printf("    void %s::init(struct ::wl_display *display, int version)\n", interfaceName);
            printf("    {\n");
            printf("        m_global = wl_global_create(display, &::%s_interface, version, this, bind_func);\n", interfaceName);
            printf("        m_displayDestroyedListener.notify = %s::display_destroy_func;\n", interfaceName);
            printf("        m_displayDestroyedListener.parent = this;\n");
            printf("        wl_display_add_destroy_listener(display, &m_displayDestroyedListener);\n");
            printf("    }\n");
            printf("\n");

            printf("    const struct wl_interface *%s::interface()\n", interfaceName);
            printf("    {\n");
            printf("        return &::%s_interface;\n", interfaceName);
            printf("    }\n");
            printf("\n");

            printf("    %s::Resource *%s::%s_allocate()\n", interfaceName, interfaceName, interfaceNameStripped);
            printf("    {\n");
            printf("        return new Resource;\n");
            printf("    }\n");
            printf("\n");

            printf("    void %s::%s_bind_resource(Resource *)\n", interfaceName, interfaceNameStripped);
            printf("    {\n");
            printf("    }\n");
            printf("\n");

            printf("    void %s::%s_destroy_resource(Resource *)\n", interfaceName, interfaceNameStripped);
            printf("    {\n");
            printf("    }\n");
            printf("\n");

            printf("    void %s::bind_func(struct ::wl_client *client, void *data, uint32_t version, uint32_t id)\n", interfaceName);
            printf("    {\n");
            printf("        %s *that = static_cast<%s *>(data);\n", interfaceName, interfaceName);
            printf("        that->add(client, id, version);\n");
            printf("    }\n");
            printf("\n");

            printf("    void %s::display_destroy_func(struct ::wl_listener *listener, void *data)\n", interfaceName);
            printf("    {\n");
            printf("        Q_UNUSED(data);\n");
            printf("        %s *that = static_cast<%s::DisplayDestroyedListener *>(listener)->parent;\n", interfaceName, interfaceName);
            printf("        that->m_global = nullptr;\n");
            printf("    }\n");
            printf("\n");

            printf("    void %s::destroy_func(struct ::wl_resource *client_resource)\n", interfaceName);
            printf("    {\n");
            printf("        Resource *resource = Resource::fromResource(client_resource);\n");
            printf("        Q_ASSERT(resource);\n");
            printf("        %s *that = resource->%s_object;\n", interfaceName, interfaceNameStripped);
            printf("        if (Q_LIKELY(that)) {\n");
            printf("            that->m_resource_map.remove(resource->client(), resource);\n");
            printf("            that->%s_destroy_resource(resource);\n", interfaceNameStripped);
            printf("\n");
            printf("            that = resource->%s_object;\n", interfaceNameStripped);
            printf("            if (that && that->m_resource == resource)\n");
            printf("                that->m_resource = nullptr;\n");
            printf("        }\n");
            printf("        delete resource;\n");
            printf("    }\n");
            printf("\n");

            bool hasRequests = !interface.requests.empty();

            QByteArray interfaceMember = hasRequests ? "&m_" + interface.name + "_interface" : QByteArray("nullptr");

            //We should consider changing bind so that it doesn't special case id == 0
            //and use function overloading instead. Jan do you have a lot of code dependent on this behavior?
            printf("    %s::Resource *%s::bind(struct ::wl_client *client, uint32_t id, int version)\n", interfaceName, interfaceName);
            printf("    {\n");
            printf("        Q_ASSERT_X(!wl_client_get_object(client, id), \"QWaylandObject bind\", QStringLiteral(\"binding to object %%1 more than once\").arg(id).toLocal8Bit().constData());\n");
            printf("        struct ::wl_resource *handle = wl_resource_create(client, &::%s_interface, version, id);\n", interfaceName);
            printf("        return bind(handle);\n");
            printf("    }\n");
            printf("\n");

            printf("    %s::Resource *%s::bind(struct ::wl_resource *handle)\n", interfaceName, interfaceName);
            printf("    {\n");
            printf("        Resource *resource = %s_allocate();\n", interfaceNameStripped);
            printf("        resource->%s_object = this;\n", interfaceNameStripped);
            printf("\n");
            printf("        wl_resource_set_implementation(handle, %s, resource, destroy_func);", interfaceMember.constData());
            printf("\n");
            printf("        resource->handle = handle;\n");
            printf("        %s_bind_resource(resource);\n", interfaceNameStripped);
            printf("        return resource;\n");
            printf("    }\n");

            printf("    %s::Resource *%s::Resource::fromResource(struct ::wl_resource *resource)\n", interfaceName, interfaceName);
            printf("    {\n");
            printf("        if (Q_UNLIKELY(!resource))\n");
            printf("            return nullptr;\n");
            printf("        if (wl_resource_instance_of(resource, &::%s_interface, %s))\n",  interfaceName, interfaceMember.constData());
            printf("            return static_cast<Resource *>(wl_resource_get_user_data(resource));\n");
            printf("        return nullptr;\n");
            printf("    }\n");

            if (hasRequests) {
                printf("\n");
                printf("    const struct ::%s_interface %s::m_%s_interface = {", interfaceName, interfaceName, interfaceName);
                bool needsComma = false;
                for (const WaylandEvent &e : interface.requests) {
                    if (needsComma)
                        printf(",");
                    needsComma = true;
                    printf("\n");
                    printf("        %s::handle_%s", interfaceName, e.name.constData());
                }
                printf("\n");
                printf("    };\n");

                for (const WaylandEvent &e : interface.requests) {
                    printf("\n");
                    printf("    void %s::%s_", interfaceName, interfaceNameStripped);
                    printEvent(e, true);
                    printf("\n");
                    printf("    {\n");
                    printf("    }\n");
                }
                printf("\n");

                for (const WaylandEvent &e : interface.requests) {
                    printf("\n");
                    printf("    void %s::", interfaceName);

                    printEventHandlerSignature(e, interfaceName, false);

                    printf("\n");
                    printf("    {\n");
                    printf("        Q_UNUSED(client);\n");
                    printf("        Resource *r = Resource::fromResource(resource);\n");
                    printf("        if (Q_UNLIKELY(!r->%s_object)) {\n", interfaceNameStripped);
                    if (e.type == "destructor")
                        printf("            wl_resource_destroy(resource);\n");
                    printf("            return;\n");
                    printf("        }\n");
                    printf("        static_cast<%s *>(r->%s_object)->%s_%s(\n", interfaceName, interfaceNameStripped, interfaceNameStripped, e.name.constData());
                    printf("            r");
                    for (const WaylandArgument &a : e.arguments) {
                        printf(",\n");
                        QByteArray cType = waylandToCType(a.type, a.interface);
                        QByteArray qtType = waylandToQtType(a.type, a.interface, e.request);
                        const char *argumentName = a.name.constData();
                        if (cType == qtType)
                            printf("            %s", argumentName);
                        else if (a.type == "string")
                            printf("            QString::fromUtf8(%s)", argumentName);
                    }
                    printf(");\n");
                    printf("    }\n");
                }
            }

            for (const WaylandEvent &e : interface.events) {
                printf("\n");
                printf("    void %s::send_", interfaceName);
                printEvent(e);
                printf("\n");
                printf("    {\n");
                printf("        Q_ASSERT_X(m_resource, \"%s::%s\", \"Uninitialised resource\");\n", interfaceName, e.name.constData());
                printf("        if (Q_UNLIKELY(!m_resource)) {\n");
                printf("            qWarning(\"could not call %s::%s as it's not initialised\");\n", interfaceName, e.name.constData());
                printf("            return;\n");
                printf("        }\n");
                printf("        send_%s(\n", e.name.constData());
                printf("            m_resource->handle");
                for (const WaylandArgument &a : e.arguments) {
                    printf(",\n");
                    printf("            %s", a.name.constData());
                }
                printf(");\n");
                printf("    }\n");
                printf("\n");

                printf("    void %s::send_", interfaceName);
                printEvent(e, false, true);
                printf("\n");
                printf("    {\n");

                for (const WaylandArgument &a : e.arguments) {
                    if (a.type != "array")
                        continue;
                    QByteArray array = a.name + "_data";
                    const char *arrayName = array.constData();
                    const char *variableName = a.name.constData();
                    printf("        struct wl_array %s;\n", arrayName);
                    printf("        %s.size = %s.size();\n", arrayName, variableName);
                    printf("        %s.data = static_cast<void *>(const_cast<char *>(%s.constData()));\n", arrayName, variableName);
                    printf("        %s.alloc = 0;\n", arrayName);
                    printf("\n");
                }

                printf("        %s_send_%s(\n", interfaceName, e.name.constData());
                printf("            resource");

                for (const WaylandArgument &a : e.arguments) {
                    printf(",\n");
                    QByteArray cType = waylandToCType(a.type, a.interface);
                    QByteArray qtType = waylandToQtType(a.type, a.interface, e.request);
                    if (a.type == "string") {
                        printf("            ");
                        if (a.allowNull)
                            printf("%s.isNull() ? nullptr : ", a.name.constData());
                        printf("%s.toUtf8().constData()", a.name.constData());
                    } else if (a.type == "array")
                        printf("            &%s_data", a.name.constData());
                    else if (cType == qtType)
                        printf("            %s", a.name.constData());
                }

                printf(");\n");
                printf("    }\n");
                printf("\n");
            }
        }
        printf("}\n");
        printf("\n");
        printf("QT_WARNING_POP\n");
        printf("QT_END_NAMESPACE\n");
    }

    if (m_option == ClientHeader) {
        QByteArray inclusionGuard = QByteArray("QT_WAYLAND_") + preProcessorProtocolName.constData();
        printf("#ifndef %s\n", inclusionGuard.constData());
        printf("#define %s\n", inclusionGuard.constData());
        printf("\n");
        if (m_headerPath.isEmpty())
            printf("#include \"wayland-%s-client-protocol.h\"\n", fileBaseName.constData());
        else
            printf("#include <%s/wayland-%s-client-protocol.h>\n", m_headerPath.constData(), fileBaseName.constData());
        printf("#include <QByteArray>\n");
        printf("#include <QString>\n");
        printf("\n");
        printf("struct wl_registry;\n");
        printf("\n");
        printf("QT_BEGIN_NAMESPACE\n");
        printf("QT_WARNING_PUSH\n");
        printf("QT_WARNING_DISABLE_GCC(\"-Wmissing-field-initializers\")\n");
        printf("QT_WARNING_DISABLE_CLANG(\"-Wmissing-field-initializers\")\n");

        QByteArray clientExport;
        if (m_headerPath.size())
            clientExport = printExportMacro("Q_WAYLAND_CLIENT_", preProcessorProtocolName);

        printf("\n");
        printf("namespace QtWayland {\n");

        bool needsNewLine = false;
        for (const WaylandInterface &interface : interfaces) {

            if (ignoreInterface(interface.name))
                continue;

            if (needsNewLine)
                printf("\n");
            needsNewLine = true;

            const char *interfaceName = interface.name.constData();

            QByteArray stripped = stripInterfaceName(interface.name);
            const char *interfaceNameStripped = stripped.constData();

            printf("    class %s %s\n    {\n", clientExport.constData(), interfaceName);
            printf("    public:\n");
            printf("        %s(struct ::wl_registry *registry, uint32_t id, int version);\n", interfaceName);
            printf("        %s(struct ::%s *object);\n", interfaceName, interfaceName);
            printf("        %s();\n", interfaceName);
            printf("\n");
            printf("        virtual ~%s();\n", interfaceName);
            printf("\n");
            printf("        void init(struct ::wl_registry *registry, uint32_t id, int version);\n");
            printf("        void init(struct ::%s *object);\n", interfaceName);
            printf("\n");
            printf("        struct ::%s *object() { return m_%s; }\n", interfaceName, interfaceName);
            printf("        const struct ::%s *object() const { return m_%s; }\n", interfaceName, interfaceName);
            printf("        static %s *fromObject(struct ::%s *object);\n", interfaceName, interfaceName);
            printf("\n");
            printf("        bool isInitialized() const;\n");
            printf("\n");
            printf("        uint32_t version() const;");
            printf("\n");
            printf("        static const struct ::wl_interface *interface();\n");

            printEnums(interface.enums);

            if (!interface.requests.empty()) {
                printf("\n");
                for (const WaylandEvent &e : interface.requests) {
                    const WaylandArgument *new_id = newIdArgument(e.arguments);
                    QByteArray new_id_str = "void ";
                    if (new_id) {
                        if (new_id->interface.isEmpty())
                            new_id_str = "void *";
                        else
                            new_id_str = "struct ::" + new_id->interface + " *";
                    }
                    printf("        %s", new_id_str.constData());
                    printEvent(e);
                    printf(";\n");
                }
            }

            bool hasEvents = !interface.events.empty();

            if (hasEvents) {
                printf("\n");
                printf("    protected:\n");
                for (const WaylandEvent &e : interface.events) {
                    printf("        virtual void %s_", interfaceNameStripped);
                    printEvent(e);
                    printf(";\n");
                }
            }

            printf("\n");
            printf("    private:\n");
            if (hasEvents) {
                printf("        void init_listener();\n");
                printf("        static const struct %s_listener m_%s_listener;\n", interfaceName, interfaceName);
                for (const WaylandEvent &e : interface.events) {
                    printf("        static void ");

                    printEventHandlerSignature(e, interfaceName);
                    printf(";\n");
                }
            }
            printf("        struct ::%s *m_%s;\n", interfaceName, interfaceName);
            printf("    };\n");
        }
        printf("}\n");
        printf("\n");
        printf("QT_WARNING_POP\n");
        printf("QT_END_NAMESPACE\n");
        printf("\n");
        printf("#endif\n");
    }

    if (m_option == ClientCode) {
        if (m_headerPath.isEmpty())
            printf("#include \"qwayland-%s.h\"\n",  fileBaseName.constData());
        else
            printf("#include <%s/qwayland-%s.h>\n", m_headerPath.constData(),  fileBaseName.constData());
        printf("\n");
        printf("QT_BEGIN_NAMESPACE\n");
        printf("QT_WARNING_PUSH\n");
        printf("QT_WARNING_DISABLE_GCC(\"-Wmissing-field-initializers\")\n");
        printf("QT_WARNING_DISABLE_CLANG(\"-Wmissing-field-initializers\")\n");
        printf("\n");
        printf("namespace QtWayland {\n");
        printf("\n");

        // wl_registry_bind is part of the protocol, so we can't use that... instead we use core
        // libwayland API to do the same thing a wayland-scanner generated wl_registry_bind would.
        printf("static inline void *wlRegistryBind(struct ::wl_registry *registry, uint32_t name, const struct ::wl_interface *interface, uint32_t version)\n");
        printf("{\n");
        printf("    const uint32_t bindOpCode = 0;\n");
        printf("    return (void *) wl_proxy_marshal_constructor_versioned((struct wl_proxy *) registry,\n");
        printf("    bindOpCode, interface, version, name, interface->name, version, nullptr);\n");
        printf("}\n");
        printf("\n");

        bool needsNewLine = false;
        for (const WaylandInterface &interface : interfaces) {

            if (ignoreInterface(interface.name))
                continue;

            if (needsNewLine)
                printf("\n");
            needsNewLine = true;

            const char *interfaceName = interface.name.constData();

            QByteArray stripped = stripInterfaceName(interface.name);
            const char *interfaceNameStripped = stripped.constData();

            bool hasEvents = !interface.events.empty();

            printf("    %s::%s(struct ::wl_registry *registry, uint32_t id, int version)\n", interfaceName, interfaceName);
            printf("    {\n");
            printf("        init(registry, id, version);\n");
            printf("    }\n");
            printf("\n");

            printf("    %s::%s(struct ::%s *obj)\n", interfaceName, interfaceName, interfaceName);
            printf("        : m_%s(obj)\n", interfaceName);
            printf("    {\n");
            if (hasEvents)
                printf("        init_listener();\n");
            printf("    }\n");
            printf("\n");

            printf("    %s::%s()\n", interfaceName, interfaceName);
            printf("        : m_%s(nullptr)\n", interfaceName);
            printf("    {\n");
            printf("    }\n");
            printf("\n");

            printf("    %s::~%s()\n", interfaceName, interfaceName);
            printf("    {\n");
            printf("    }\n");
            printf("\n");

            printf("    void %s::init(struct ::wl_registry *registry, uint32_t id, int version)\n", interfaceName);
            printf("    {\n");
            printf("        m_%s = static_cast<struct ::%s *>(wlRegistryBind(registry, id, &%s_interface, version));\n", interfaceName, interfaceName, interfaceName);
            if (hasEvents)
                printf("        init_listener();\n");
            printf("    }\n");
            printf("\n");

            printf("    void %s::init(struct ::%s *obj)\n", interfaceName, interfaceName);
            printf("    {\n");
            printf("        m_%s = obj;\n", interfaceName);
            if (hasEvents)
                printf("        init_listener();\n");
            printf("    }\n");
            printf("\n");

            printf("    %s *%s::fromObject(struct ::%s *object)\n", interfaceName, interfaceName, interfaceName);
            printf("    {\n");
            if (hasEvents) {
                printf("        if (wl_proxy_get_listener((struct ::wl_proxy *)object) != (void *)&m_%s_listener)\n", interfaceName);
                printf("            return nullptr;\n");
            }
            printf("        return static_cast<%s *>(%s_get_user_data(object));\n", interfaceName, interfaceName);
            printf("    }\n");
            printf("\n");

            printf("    bool %s::isInitialized() const\n", interfaceName);
            printf("    {\n");
            printf("        return m_%s != nullptr;\n", interfaceName);
            printf("    }\n");
            printf("\n");

            printf("    uint32_t %s::version() const\n", interfaceName);
            printf("    {\n");
            printf("        return wl_proxy_get_version(reinterpret_cast<wl_proxy*>(m_%s));\n", interfaceName);
            printf("    }\n");
            printf("\n");

            printf("    const struct wl_interface *%s::interface()\n", interfaceName);
            printf("    {\n");
            printf("        return &::%s_interface;\n", interfaceName);
            printf("    }\n");

            for (const WaylandEvent &e : interface.requests) {
                printf("\n");
                const WaylandArgument *new_id = newIdArgument(e.arguments);
                QByteArray new_id_str = "void ";
                if (new_id) {
                    if (new_id->interface.isEmpty())
                        new_id_str = "void *";
                    else
                        new_id_str = "struct ::" + new_id->interface + " *";
                }
                printf("    %s%s::", new_id_str.constData(), interfaceName);
                printEvent(e);
                printf("\n");
                printf("    {\n");
                for (const WaylandArgument &a : e.arguments) {
                    if (a.type != "array")
                        continue;
                    QByteArray array = a.name + "_data";
                    const char *arrayName = array.constData();
                    const char *variableName = a.name.constData();
                    printf("        struct wl_array %s;\n", arrayName);
                    printf("        %s.size = %s.size();\n", arrayName, variableName);
                    printf("        %s.data = static_cast<void *>(const_cast<char *>(%s.constData()));\n", arrayName, variableName);
                    printf("        %s.alloc = 0;\n", arrayName);
                    printf("\n");
                }
                int actualArgumentCount = new_id ? int(e.arguments.size()) - 1 : int(e.arguments.size());
                if (new_id)
                    printf("        %s %s = ::%s_%s(\n", new_id_str.constData(), new_id->name.constData(), interfaceName, e.name.constData());
                else
                    printf("        ::%s_%s(\n", interfaceName, e.name.constData());
                printf("            m_%s%s", interfaceName, actualArgumentCount > 0 ? "," : "");
                bool needsComma = false;
                for (const WaylandArgument &a : e.arguments) {
                    bool isNewId = a.type == "new_id";
                    if (isNewId && !a.interface.isEmpty())
                        continue;
                    if (needsComma)
                        printf(",");
                    needsComma = true;
                    printf("\n");
                    if (isNewId) {
                        printf("            interface,\n");
                        printf("            version");
                    } else {
                        QByteArray cType = waylandToCType(a.type, a.interface);
                        QByteArray qtType = waylandToQtType(a.type, a.interface, e.request);
                        if (a.type == "string") {
                            printf("            ");
                            if (a.allowNull)
                                printf("%s.isNull() ? nullptr : ", a.name.constData());
                            printf("%s.toUtf8().constData()", a.name.constData());
                        } else if (a.type == "array")
                            printf("            &%s_data", a.name.constData());
                        else if (cType == qtType)
                            printf("            %s", a.name.constData());
                    }
                }
                printf(");\n");
                if (e.type == "destructor")
                    printf("        m_%s = nullptr;\n", interfaceName);
                if (new_id)
                    printf("        return %s;\n", new_id->name.constData());
                printf("    }\n");
            }

            if (hasEvents) {
                printf("\n");
                for (const WaylandEvent &e : interface.events) {
                    printf("    void %s::%s_", interfaceName, interfaceNameStripped);
                    printEvent(e, true);
                    printf("\n");
                    printf("    {\n");
                    printf("    }\n");
                    printf("\n");
                    printf("    void %s::", interfaceName);
                    printEventHandlerSignature(e, interfaceName, false);
                    printf("\n");
                    printf("    {\n");
                    printf("        Q_UNUSED(object);\n");
                    printf("        static_cast<%s *>(data)->%s_%s(", interfaceName, interfaceNameStripped, e.name.constData());
                    bool needsComma = false;
                    for (const WaylandArgument &a : e.arguments) {
                        if (needsComma)
                            printf(",");
                        needsComma = true;
                        printf("\n");
                        const char *argumentName = a.name.constData();
                        if (a.type == "string")
                            printf("            QString::fromUtf8(%s)", argumentName);
                        else
                            printf("            %s", argumentName);
                    }
                    printf(");\n");

                    printf("    }\n");
                    printf("\n");
                }
                printf("    const struct %s_listener %s::m_%s_listener = {\n", interfaceName, interfaceName, interfaceName);
                for (const WaylandEvent &e : interface.events) {
                    printf("        %s::handle_%s,\n", interfaceName, e.name.constData());
                }
                printf("    };\n");
                printf("\n");

                printf("    void %s::init_listener()\n", interfaceName);
                printf("    {\n");
                printf("        %s_add_listener(m_%s, &m_%s_listener, this);\n", interfaceName, interfaceName, interfaceName);
                printf("    }\n");
            }
        }
        printf("}\n");
        printf("\n");
        printf("QT_WARNING_POP\n");
        printf("QT_END_NAMESPACE\n");
    }

    return true;
}

void Scanner::printErrors()
{
    if (m_xml->hasError())
        fprintf(stderr, "XML error: %s\nLine %lld, column %lld\n", m_xml->errorString().toLocal8Bit().constData(), m_xml->lineNumber(), m_xml->columnNumber());
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    Scanner scanner;

    if (!scanner.parseArguments(argc, argv)) {
        scanner.printUsage();
        return EXIT_FAILURE;
    }

    if (!scanner.process()) {
        scanner.printErrors();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
