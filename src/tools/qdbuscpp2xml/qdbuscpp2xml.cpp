// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qbuffer.h>
#include <qbytearray.h>
#include <qdebug.h>
#include <qfile.h>
#include <qlist.h>
#include <qstring.h>
#include <qvarlengtharray.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <qdbusconnection.h>    // for the Export* flags
#include <private/qdbusconnection_p.h>    // for the qDBusCheckAsyncTag
#include <private/qdbusmetatype_p.h> // for QDBusMetaTypeId::init()

using namespace Qt::StringLiterals;

// copied from dbus-protocol.h:
static const char docTypeHeader[] =
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
    "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n";

#define ANNOTATION_NO_WAIT      "org.freedesktop.DBus.Method.NoReply"
#define QCLASSINFO_DBUS_INTERFACE       "D-Bus Interface"
#define QCLASSINFO_DBUS_INTROSPECTION   "D-Bus Introspection"

#include <qdbusmetatype.h>
#include <private/qdbusmetatype_p.h>
#include <private/qdbusutil_p.h>

#include "moc.h"
#include "generator.h"
#include "preprocessor.h"

#define PROGRAMNAME     "qdbuscpp2xml"
#define PROGRAMVERSION  "0.2"
#define PROGRAMCOPYRIGHT QT_COPYRIGHT

static QString outputFile;
static int flags;

static const char help[] =
    "Usage: " PROGRAMNAME " [options...] [files...]\n"
    "Parses the C++ source or header file containing a QObject-derived class and\n"
    "produces the D-Bus Introspection XML."
    "\n"
    "Options:\n"
    "  -p|-s|-m             Only parse scriptable Properties, Signals and Methods (slots)\n"
    "  -P|-S|-M             Parse all Properties, Signals and Methods (slots)\n"
    "  -a                   Output all scriptable contents (equivalent to -psm)\n"
    "  -A                   Output all contents (equivalent to -PSM)\n"
    "  -t <type>=<dbustype> Output <type> (ex: MyStruct) as <dbustype> (ex: {ss})\n"
    "  -o <filename>        Write the output to file <filename>\n"
    "  -h                   Show this information\n"
    "  -V                   Show the program version and quit.\n"
    "\n";

int qDBusParametersForMethod(const FunctionDef &mm, QList<QMetaType> &metaTypes, QString &errorMsg)
{
    QList<QByteArray> parameterTypes;
    parameterTypes.reserve(mm.arguments.size());

    for (const ArgumentDef &arg : mm.arguments)
        parameterTypes.append(arg.normalizedType);

    return qDBusParametersForMethod(parameterTypes, metaTypes, errorMsg);
}


static inline QString typeNameToXml(const char *typeName)
{
    QString plain = QLatin1StringView(typeName);
    return plain.toHtmlEscaped();
}

static QString addFunction(const FunctionDef &mm, bool isSignal = false) {

    QString xml = QString::asprintf("    <%s name=\"%s\">\n",
                                    isSignal ? "signal" : "method", mm.name.constData());

    // check the return type first
    int typeId = QMetaType::fromName(mm.normalizedType).id();
    if (typeId != QMetaType::Void) {
        if (typeId) {
            const char *typeName = QDBusMetaType::typeToSignature(QMetaType(typeId));
            if (typeName) {
                xml += QString::fromLatin1("      <arg type=\"%1\" direction=\"out\"/>\n")
                        .arg(typeNameToXml(typeName));

                    // do we need to describe this argument?
                    if (!QDBusMetaType::signatureToMetaType(typeName).isValid())
                        xml += QString::fromLatin1("      <annotation name=\"org.qtproject.QtDBus.QtTypeName.Out0\" value=\"%1\"/>\n")
                            .arg(typeNameToXml(mm.normalizedType.constData()));
            } else {
                return QString();
            }
        } else if (!mm.normalizedType.isEmpty()) {
            qWarning() << "Unregistered return type:" << mm.normalizedType.constData();
            return QString();
        }
    }
    QList<ArgumentDef> names = mm.arguments;
    QList<QMetaType> types;
    QString errorMsg;
    int inputCount = qDBusParametersForMethod(mm, types, errorMsg);
    if (inputCount == -1) {
        qWarning() << qPrintable(errorMsg);
        return QString();           // invalid form
    }
    if (isSignal && inputCount + 1 != types.size())
        return QString();           // signal with output arguments?
    if (isSignal && types.at(inputCount) == QDBusMetaTypeId::message())
        return QString();           // signal with QDBusMessage argument?

    bool isScriptable = mm.isScriptable;
    for (qsizetype j = 1; j < types.size(); ++j) {
        // input parameter for a slot or output for a signal
        if (types.at(j) == QDBusMetaTypeId::message()) {
            isScriptable = true;
            continue;
        }

        QString name;
        if (!names.at(j - 1).name.isEmpty())
            name = QString::fromLatin1("name=\"%1\" ").arg(QString::fromLatin1(names.at(j - 1).name));

        bool isOutput = isSignal || j > inputCount;

        const char *signature = QDBusMetaType::typeToSignature(QMetaType(types.at(j)));
        xml += QString::fromLatin1("      <arg %1type=\"%2\" direction=\"%3\"/>\n")
                .arg(name,
                     QLatin1StringView(signature),
                     isOutput ? "out"_L1 : "in"_L1);

        // do we need to describe this argument?
        if (!QDBusMetaType::signatureToMetaType(signature).isValid()) {
            const char *typeName = QMetaType(types.at(j)).name();
            xml += QString::fromLatin1("      <annotation name=\"org.qtproject.QtDBus.QtTypeName.%1%2\" value=\"%3\"/>\n")
                    .arg(isOutput ? "Out"_L1 : "In"_L1)
                    .arg(isOutput && !isSignal ? j - inputCount : j - 1)
                    .arg(typeNameToXml(typeName));
        }
    }

    int wantedMask;
    if (isScriptable)
        wantedMask = isSignal ? QDBusConnection::ExportScriptableSignals
                              : QDBusConnection::ExportScriptableSlots;
    else
        wantedMask = isSignal ? QDBusConnection::ExportNonScriptableSignals
                              : QDBusConnection::ExportNonScriptableSlots;
    if ((flags & wantedMask) != wantedMask)
        return QString();

    if (qDBusCheckAsyncTag(mm.tag.constData()))
        // add the no-reply annotation
        xml += "      <annotation name=\"" ANNOTATION_NO_WAIT "\" value=\"true\"/>\n"_L1;

    QString retval = xml;
    retval += QString::fromLatin1("    </%1>\n").arg(isSignal ? "signal"_L1 : "method"_L1);

    return retval;
}


static QString generateInterfaceXml(const ClassDef *mo)
{
    QString retval;

    // start with properties:
    if (flags & (QDBusConnection::ExportScriptableProperties |
                 QDBusConnection::ExportNonScriptableProperties)) {
        static const char *accessvalues[] = {nullptr, "read", "write", "readwrite"};
        for (const PropertyDef &mp : mo->propertyList) {
            if (!((!mp.scriptable.isEmpty() && (flags & QDBusConnection::ExportScriptableProperties)) ||
                  (!mp.scriptable.isEmpty() && (flags & QDBusConnection::ExportNonScriptableProperties))))
                continue;

            int access = 0;
            if (!mp.read.isEmpty())
                access |= 1;
            if (!mp.write.isEmpty())
                access |= 2;
            if (!mp.member.isEmpty())
                access |= 3;

            int typeId = QMetaType::fromName(mp.type).id();
            if (!typeId) {
                fprintf(stderr, PROGRAMNAME ": unregistered type: '%s', ignoring\n",
                        mp.type.constData());
                continue;
            }
            const char *signature = QDBusMetaType::typeToSignature(QMetaType(typeId));
            if (!signature)
                continue;

            retval += QString::fromLatin1("    <property name=\"%1\" type=\"%2\" access=\"%3\"")
                      .arg(QLatin1StringView(mp.name),
                           QLatin1StringView(signature),
                           QLatin1StringView(accessvalues[access]));

            if (!QDBusMetaType::signatureToMetaType(signature).isValid()) {
                retval += QString::fromLatin1(">\n      <annotation name=\"org.qtproject.QtDBus.QtTypeName\" value=\"%3\"/>\n    </property>\n")
                          .arg(typeNameToXml(mp.type.constData()));
            } else {
                retval += "/>\n"_L1;
            }
        }
    }

    // now add methods:

    if (flags & (QDBusConnection::ExportScriptableSignals | QDBusConnection::ExportNonScriptableSignals)) {
        for (const FunctionDef &mm : mo->signalList) {
            if (mm.wasCloned)
                continue;
            if (!mm.isScriptable && !(flags & QDBusConnection::ExportNonScriptableSignals))
                continue;

            retval += addFunction(mm, true);
        }
    }

    if (flags & (QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportNonScriptableSlots)) {
        for (const FunctionDef &slot : mo->slotList) {
            if (!slot.isScriptable && !(flags & QDBusConnection::ExportNonScriptableSlots))
                continue;
            if (slot.access == FunctionDef::Public)
              retval += addFunction(slot);
        }
        for (const FunctionDef &method : mo->methodList) {
            if (!method.isScriptable && !(flags & QDBusConnection::ExportNonScriptableSlots))
                continue;
            if (method.access == FunctionDef::Public)
              retval += addFunction(method);
        }
    }
    return retval;
}

QString qDBusInterfaceFromClassDef(const ClassDef *mo)
{
    QString interface;

    for (const ClassInfoDef &cid : mo->classInfoList) {
        if (cid.name == QCLASSINFO_DBUS_INTERFACE)
            return QString::fromUtf8(cid.value);
    }
    interface = QLatin1StringView(mo->classname);
    interface.replace("::"_L1, "."_L1);

    if (interface.startsWith("QDBus"_L1)) {
        interface.prepend("org.qtproject.QtDBus."_L1);
    } else if (interface.startsWith(u'Q') &&
                interface.size() >= 2 && interface.at(1).isUpper()) {
        // assume it's Qt
        interface.prepend("local.org.qtproject.Qt."_L1);
    } else {
        interface.prepend("local."_L1);
    }

    return interface;
}


QString qDBusGenerateClassDefXml(const ClassDef *cdef)
{
    for (const ClassInfoDef &cid : cdef->classInfoList) {
        if (cid.name == QCLASSINFO_DBUS_INTROSPECTION)
            return QString::fromUtf8(cid.value);
    }

    // generate the interface name from the meta object
    QString interface = qDBusInterfaceFromClassDef(cdef);

    QString xml = generateInterfaceXml(cdef);

    if (xml.isEmpty())
        return QString();       // don't add an empty interface
    return QString::fromLatin1("  <interface name=\"%1\">\n%2  </interface>\n")
        .arg(interface, xml);
}

static void showHelp()
{
    printf("%s", help);
    exit(0);
}

static void showVersion()
{
    printf("%s version %s\n", PROGRAMNAME, PROGRAMVERSION);
    printf("D-Bus QObject-to-XML converter\n");
    exit(0);
}

class CustomType {
public:
    CustomType(const QByteArray &typeName)
        : typeName(typeName)
    {
        metaTypeImpl.name = typeName.constData();
    }
    QMetaType metaType() const { return QMetaType(&metaTypeImpl); }

private:
    // not copiable and not movable because of QBasicAtomicInt
    QtPrivate::QMetaTypeInterface metaTypeImpl =
    {  /*.revision=*/ 0,
       /*.alignment=*/ 0,
       /*.size=*/ 0,
       /*.flags=*/ 0,
       /*.typeId=*/ 0,
       /*.metaObjectFn=*/ 0,
       /*.name=*/ nullptr, // set by the constructor
       /*.defaultCtr=*/ nullptr,
       /*.copyCtr=*/ nullptr,
       /*.moveCtr=*/ nullptr,
       /*.dtor=*/ nullptr,
       /*.equals=*/ nullptr,
       /*.lessThan=*/ nullptr,
       /*.debugStream=*/ nullptr,
       /*.dataStreamOut=*/ nullptr,
       /*.dataStreamIn=*/ nullptr,
       /*.legacyRegisterOp=*/ nullptr
    };
    QByteArray typeName;
};
// Unlike std::vector, std::deque works with non-copiable non-movable types
static std::deque<CustomType> s_customTypes;

static void parseCmdLine(QStringList &arguments)
{
    flags = 0;
    for (qsizetype i = 0; i < arguments.size(); ++i) {
        const QString arg = arguments.at(i);

        if (arg == "--help"_L1)
            showHelp();

        if (!arg.startsWith(u'-'))
            continue;

        char c = arg.size() == 2 ? arg.at(1).toLatin1() : char(0);
        switch (c) {
        case 'P':
            flags |= QDBusConnection::ExportNonScriptableProperties;
            Q_FALLTHROUGH();
        case 'p':
            flags |= QDBusConnection::ExportScriptableProperties;
            break;

        case 'S':
            flags |= QDBusConnection::ExportNonScriptableSignals;
            Q_FALLTHROUGH();
        case 's':
            flags |= QDBusConnection::ExportScriptableSignals;
            break;

        case 'M':
            flags |= QDBusConnection::ExportNonScriptableSlots;
            Q_FALLTHROUGH();
        case 'm':
            flags |= QDBusConnection::ExportScriptableSlots;
            break;

        case 'A':
            flags |= QDBusConnection::ExportNonScriptableContents;
            Q_FALLTHROUGH();
        case 'a':
            flags |= QDBusConnection::ExportScriptableContents;
            break;

        case 't':
            if (arguments.size() < i + 2) {
                printf("-t expects a type=dbustype argument\n");
                exit(1);
            } else {
                const QByteArray arg = arguments.takeAt(i + 1).toUtf8();
                // lastIndexOf because the C++ type could contain '=' while the DBus type can't
                const qsizetype separator = arg.lastIndexOf('=');
                if (separator == -1) {
                    printf("-t expects a type=dbustype argument, but no '=' was found\n");
                    exit(1);
                }
                const QByteArray type = arg.left(separator);
                const QByteArray dbustype = arg.mid(separator+1);

                s_customTypes.emplace_back(type);
                QMetaType metaType = s_customTypes.back().metaType();
                QDBusMetaType::registerCustomType(metaType, dbustype);
            }
            break;

        case 'o':
            if (arguments.size() < i + 2 || arguments.at(i + 1).startsWith(u'-')) {
                printf("-o expects a filename\n");
                exit(1);
            }
            outputFile = arguments.takeAt(i + 1);
            break;

        case 'h':
        case '?':
            showHelp();
            break;

        case 'V':
            showVersion();
            break;

        default:
            printf("unknown option: \"%s\"\n", qPrintable(arg));
            exit(1);
        }
    }

    if (flags == 0)
        flags = QDBusConnection::ExportScriptableContents
                | QDBusConnection::ExportNonScriptableContents;
}

int main(int argc, char **argv)
{
    QStringList args;
    args.reserve(argc - 1);
    for (int n = 1; n < argc; ++n)
        args.append(QString::fromLocal8Bit(argv[n]));
    parseCmdLine(args);

    QDBusMetaTypeId::init();

    QList<ClassDef> classes;

    if (args.isEmpty())
        args << u"-"_s;
    for (const auto &arg: std::as_const(args)) {
        if (arg.startsWith(u'-') && arg.size() > 1)
            continue;

        QFile f;
        bool fileIsOpen;
        QString fileName;
        if (arg == u'-') {
            fileName = "stdin"_L1;
            fileIsOpen = f.open(stdin, QIODevice::ReadOnly | QIODevice::Text);
        } else {
            fileName = arg;
            f.setFileName(arg);
            fileIsOpen = f.open(QIODevice::ReadOnly | QIODevice::Text);
        }
        if (!fileIsOpen) {
            fprintf(stderr, PROGRAMNAME ": could not open '%s': %s\n",
                    qPrintable(fileName), qPrintable(f.errorString()));
            return 1;
        }

        Preprocessor pp;
        Moc moc;
        pp.macros["Q_MOC_RUN"];
        pp.macros["__cplusplus"];

        const QByteArray filename = arg.toLocal8Bit();

        moc.filename = filename;
        moc.currentFilenames.push(filename);

        moc.symbols = pp.preprocessed(moc.filename, &f);
        moc.parse();

        if (moc.classList.isEmpty())
            return 0;
        classes = moc.classList;

        f.close();
    }

    QFile output;
    if (outputFile.isEmpty()) {
        if (!output.open(stdout, QIODevice::WriteOnly)) {
            fprintf(stderr, PROGRAMNAME ": could not open standard output: %s\n",
                    qPrintable(output.errorString()));
            return 1;
        }
    } else {
        output.setFileName(outputFile);
        if (!output.open(QIODevice::WriteOnly)) {
            fprintf(stderr, PROGRAMNAME ": could not open output file '%s': %s\n",
                    qPrintable(outputFile), qPrintable(output.errorString()));
            return 1;
        }
    }

    output.write(docTypeHeader);
    output.write("<node>\n");
    for (const ClassDef &cdef : std::as_const(classes)) {
        QString xml = qDBusGenerateClassDefXml(&cdef);
        output.write(std::move(xml).toLocal8Bit());
    }
    output.write("</node>\n");

    return 0;
}

