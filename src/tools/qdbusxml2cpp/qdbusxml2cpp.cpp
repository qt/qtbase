// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qbytearray.h>
#include <qcommandlineparser.h>
#include <qcoreapplication.h>
#include <qdebug.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qloggingcategory.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtextstream.h>
#include <qset.h>

#include <qdbusmetatype.h>
#include <private/qdbusintrospection_p.h>

#include <stdio.h>
#include <stdlib.h>

#define PROGRAMNAME     "qdbusxml2cpp"
#define PROGRAMVERSION  "0.8"
#define PROGRAMCOPYRIGHT QT_COPYRIGHT

#define ANNOTATION_NO_WAIT      "org.freedesktop.DBus.Method.NoReply"

using namespace Qt::StringLiterals;

static QString globalClassName;
static QString parentClassName;
static QString proxyFile;
static QString adaptorFile;
static QString inputFile;
static bool skipNamespaces;
static bool verbose;
static bool includeMocs;
static QString commandLine;
static QStringList includes;
static QStringList globalIncludes;
static QStringList wantedInterfaces;

static const char includeList[] =
    "#include <QtCore/QByteArray>\n"
    "#include <QtCore/QList>\n"
    "#include <QtCore/QMap>\n"
    "#include <QtCore/QString>\n"
    "#include <QtCore/QStringList>\n"
    "#include <QtCore/QVariant>\n";

static const char forwardDeclarations[] =
    "#include <QtCore/qcontainerfwd.h>\n";

static QDBusIntrospection::Interfaces readInput()
{
    QFile input(inputFile);
    if (inputFile.isEmpty() || inputFile == "-"_L1)
        input.open(stdin, QIODevice::ReadOnly);
    else
        input.open(QIODevice::ReadOnly);

    QByteArray data = input.readAll();

    // check if the input is already XML
    data = data.trimmed();
    if (data.startsWith("<!DOCTYPE ") || data.startsWith("<?xml") ||
        data.startsWith("<node") || data.startsWith("<interface"))
        // already XML
        return QDBusIntrospection::parseInterfaces(QString::fromUtf8(data));

    fprintf(stderr, "%s: Cannot process input: '%s'. Stop.\n",
            PROGRAMNAME, qPrintable(inputFile));
    exit(1);
}

static void cleanInterfaces(QDBusIntrospection::Interfaces &interfaces)
{
    if (!wantedInterfaces.isEmpty()) {
        QDBusIntrospection::Interfaces::Iterator it = interfaces.begin();
        while (it != interfaces.end())
            if (!wantedInterfaces.contains(it.key()))
                it = interfaces.erase(it);
            else
                ++it;
    }
}

static bool isSupportedSuffix(QStringView suffix)
{
    const QLatin1StringView candidates[] = {
        "h"_L1,
        "cpp"_L1,
        "cc"_L1
    };

    for (auto candidate : candidates)
        if (suffix == candidate)
            return true;

    return false;
}

// produce a header name from the file name
static QString header(const QString &name)
{
    QStringList parts = name.split(u':');
    QString retval = parts.front();

    if (retval.isEmpty() || retval == "-"_L1)
        return retval;

    QFileInfo header{retval};
    if (!isSupportedSuffix(header.suffix()))
        retval.append(".h"_L1);

    return retval;
}

// produce a cpp name from the file name
static QString cpp(const QString &name)
{
    QStringList parts = name.split(u':');
    QString retval = parts.back();

    if (retval.isEmpty() || retval == "-"_L1)
        return retval;

    QFileInfo source{retval};
    if (!isSupportedSuffix(source.suffix()))
        retval.append(".cpp"_L1);

    return retval;
}

// produce a moc name from the file name
static QString moc(const QString &name)
{
    QString retval;
    const QStringList fileNames = name.split(u':');

    if (fileNames.size() == 1) {
        QFileInfo fi{fileNames.front()};
        if (isSupportedSuffix(fi.suffix())) {
            // Generates a file that contains the header and the implementation: include "filename.moc"
            retval += fi.completeBaseName();
            retval += ".moc"_L1;
        } else {
            // Separate source and header files are generated: include "moc_filename.cpp"
            retval += "moc_"_L1;
            retval += fi.fileName();
            retval += ".cpp"_L1;
        }
    } else {
        QString headerName = fileNames.front();
        QString sourceName = fileNames.back();

        if (sourceName.isEmpty() || sourceName == "-"_L1) {
            // If only a header is generated, don't include anything
        } else if (headerName.isEmpty() || headerName == "-"_L1) {
            // If only source file is generated: include "moc_sourcename.cpp"
            QFileInfo source{sourceName};

            retval += "moc_"_L1;
            retval += source.completeBaseName();
            retval += ".cpp"_L1;

            fprintf(stderr, "warning: no header name is provided, assuming it to be \"%s\"\n",
                    qPrintable(source.completeBaseName() + ".h"_L1));
        } else {
            // Both source and header generated: include "moc_headername.cpp"
            QFileInfo header{headerName};

            retval += "moc_"_L1;
            retval += header.completeBaseName();
            retval += ".cpp"_L1;
        }
    }

    return retval;
}

static QTextStream &writeHeader(QTextStream &ts, bool changesWillBeLost)
{
    ts << "/*\n"
          " * This file was generated by " PROGRAMNAME " version " PROGRAMVERSION "\n"
          " * Command line was: " << commandLine << "\n"
          " *\n"
          " * " PROGRAMNAME " is " PROGRAMCOPYRIGHT "\n"
          " *\n"
          " * This is an auto-generated file.\n";

    if (changesWillBeLost)
        ts << " * Do not edit! All changes made to it will be lost.\n";
    else
        ts << " * This file may have been hand-edited. Look for HAND-EDIT comments\n"
              " * before re-generating it.\n";

    ts << " */\n\n";

    return ts;
}

enum ClassType { Proxy, Adaptor };
static QString classNameForInterface(const QString &interface, ClassType classType)
{
    if (!globalClassName.isEmpty())
        return globalClassName;

    const auto parts = QStringView{interface}.split(u'.');

    QString retval;
    if (classType == Proxy) {
        for (const auto &part : parts) {
            retval += part[0].toUpper();
            retval += part.mid(1);
        }
    } else {
        retval += parts.last()[0].toUpper() + parts.last().mid(1);
    }

    if (classType == Proxy)
        retval += "Interface"_L1;
    else
        retval += "Adaptor"_L1;

    return retval;
}

static QByteArray qtTypeName(const QString &where, const QString &signature,
                             const QDBusIntrospection::Annotations &annotations, qsizetype paramId = -1,
                             const char *direction = "Out")
{
    int type = QDBusMetaType::signatureToMetaType(signature.toLatin1()).id();
    if (type == QMetaType::UnknownType) {
        QString annotationName = u"org.qtproject.QtDBus.QtTypeName"_s;
        if (paramId >= 0)
            annotationName += ".%1%2"_L1.arg(QLatin1StringView(direction)).arg(paramId);
        QString qttype = annotations.value(annotationName);
        if (!qttype.isEmpty())
            return std::move(qttype).toLatin1();

        QString oldAnnotationName = u"com.trolltech.QtDBus.QtTypeName"_s;
        if (paramId >= 0)
            oldAnnotationName += ".%1%2"_L1.arg(QLatin1StringView(direction)).arg(paramId);
        qttype = annotations.value(oldAnnotationName);

        if (qttype.isEmpty()) {
            fprintf(stderr, "%s: Got unknown type `%s' processing '%s'\n",
                    PROGRAMNAME, qPrintable(signature), qPrintable(inputFile));
            fprintf(stderr,
                    "You should add <annotation name=\"%s\" value=\"<type>\"/> to the XML "
                    "description for '%s'\n",
                    qPrintable(annotationName), qPrintable(where));

            exit(1);
        }

        fprintf(stderr, "%s: Warning: deprecated annotation '%s' found while processing '%s'; "
                        "suggest updating to '%s'\n",
                PROGRAMNAME, qPrintable(oldAnnotationName), qPrintable(inputFile),
                qPrintable(annotationName));
        return std::move(qttype).toLatin1();
    }

    return QMetaType(type).name();
}

static QString nonConstRefArg(const QByteArray &arg)
{
    return QLatin1StringView(arg) + " &"_L1;
}

static QString templateArg(const QByteArray &arg)
{
    if (!arg.endsWith('>'))
        return QLatin1StringView(arg);

    return QLatin1StringView(arg) + " "_L1;
}

static QString constRefArg(const QByteArray &arg)
{
    if (!arg.startsWith('Q'))
        return QLatin1StringView(arg) + " "_L1;
    else
        return "const %1 &"_L1.arg(QLatin1StringView(arg));
}

static QStringList makeArgNames(const QDBusIntrospection::Arguments &inputArgs,
                                const QDBusIntrospection::Arguments &outputArgs =
                                QDBusIntrospection::Arguments())
{
    QStringList retval;
    const qsizetype numInputArgs = inputArgs.size();
    const qsizetype numOutputArgs = outputArgs.size();
    retval.reserve(numInputArgs + numOutputArgs);
    for (qsizetype i = 0; i < numInputArgs; ++i) {
        const QDBusIntrospection::Argument &arg = inputArgs.at(i);
        QString name = arg.name;
        if (name.isEmpty())
            name = u"in%1"_s.arg(i);
        else
            name.replace(u'-', u'_');
        while (retval.contains(name))
            name += "_"_L1;
        retval << name;
    }
    for (qsizetype i = 0; i < numOutputArgs; ++i) {
        const QDBusIntrospection::Argument &arg = outputArgs.at(i);
        QString name = arg.name;
        if (name.isEmpty())
            name = u"out%1"_s.arg(i);
        else
            name.replace(u'-', u'_');
        while (retval.contains(name))
            name += "_"_L1;
        retval << name;
    }
    return retval;
}

static void writeArgList(QTextStream &ts, const QStringList &argNames,
                         const QDBusIntrospection::Annotations &annotations,
                         const QDBusIntrospection::Arguments &inputArgs,
                         const QDBusIntrospection::Arguments &outputArgs = QDBusIntrospection::Arguments())
{
    // input args:
    bool first = true;
    qsizetype argPos = 0;
    for (qsizetype i = 0; i < inputArgs.size(); ++i) {
        const QDBusIntrospection::Argument &arg = inputArgs.at(i);
        QString type = constRefArg(qtTypeName(arg.name, arg.type, annotations, i, "In"));

        if (!first)
            ts << ", ";
        ts << type << argNames.at(argPos++);
        first = false;
    }

    argPos++;

    // output args
    // yes, starting from 1
    for (qsizetype i = 1; i < outputArgs.size(); ++i) {
        const QDBusIntrospection::Argument &arg = outputArgs.at(i);

        if (!first)
            ts << ", ";
        ts << nonConstRefArg(qtTypeName(arg.name, arg.type, annotations, i, "Out"))
           << argNames.at(argPos++);
        first = false;
    }
}

static void writeSignalArgList(QTextStream &ts, const QStringList &argNames,
                         const QDBusIntrospection::Annotations &annotations,
                         const QDBusIntrospection::Arguments &outputArgs)
{
    bool first = true;
    qsizetype argPos = 0;
    for (qsizetype i = 0; i < outputArgs.size(); ++i) {
        const QDBusIntrospection::Argument &arg = outputArgs.at(i);
        QString type = constRefArg(
                qtTypeName(arg.name, arg.type, annotations, i, "Out"));

        if (!first)
            ts << ", ";
        ts << type << argNames.at(argPos++);
        first = false;
    }
}

static QString propertyGetter(const QDBusIntrospection::Property &property)
{
    QString getter = property.annotations.value("org.qtproject.QtDBus.PropertyGetter"_L1);
    if (!getter.isEmpty())
        return getter;

    getter = property.annotations.value("com.trolltech.QtDBus.propertyGetter"_L1);
    if (!getter.isEmpty()) {
        fprintf(stderr, "%s: Warning: deprecated annotation 'com.trolltech.QtDBus.propertyGetter' found"
                " while processing '%s';"
                " suggest updating to 'org.qtproject.QtDBus.PropertyGetter'\n",
                PROGRAMNAME, qPrintable(inputFile));
        return getter;
    }

    getter =  property.name;
    getter[0] = getter[0].toLower();
    return getter;
}

static QString propertySetter(const QDBusIntrospection::Property &property)
{
    QString setter = property.annotations.value("org.qtproject.QtDBus.PropertySetter"_L1);
    if (!setter.isEmpty())
        return setter;

    setter = property.annotations.value("com.trolltech.QtDBus.propertySetter"_L1);
    if (!setter.isEmpty()) {
        fprintf(stderr, "%s: Warning: deprecated annotation 'com.trolltech.QtDBus.propertySetter' found"
                " while processing '%s';"
                " suggest updating to 'org.qtproject.QtDBus.PropertySetter'\n",
                PROGRAMNAME, qPrintable(inputFile));
        return setter;
    }

    setter = "set"_L1 + property.name;
    setter[3] = setter[3].toUpper();
    return setter;
}

static QString methodName(const QDBusIntrospection::Method &method)
{
    QString name = method.annotations.value(u"org.qtproject.QtDBus.MethodName"_s);
    if (!name.isEmpty())
        return name;

    return method.name;
}

static QString stringify(const QString &data)
{
    QString retval;
    qsizetype i;
    for (i = 0; i < data.size(); ++i) {
        retval += u'\"';
        for ( ; i < data.size() && data[i] != u'\n' && data[i] != u'\r'; ++i)
            if (data[i] == u'\"')
                retval += "\\\""_L1;
            else
                retval += data[i];
        if (i+1 < data.size() && data[i] == u'\r' && data[i+1] == u'\n')
            i++;
        retval += "\\n\"\n"_L1;
    }
    return retval;
}

static bool openFile(const QString &fileName, QFile &file)
{
    if (fileName.isEmpty())
        return false;

    bool isOk = false;
    if (fileName == "-"_L1) {
        isOk = file.open(stdout, QIODevice::WriteOnly | QIODevice::Text);
    } else {
        file.setFileName(fileName);
        isOk = file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    }

    if (!isOk)
        fprintf(stderr, "%s: Unable to open '%s': %s\n",
                PROGRAMNAME, qPrintable(fileName), qPrintable(file.errorString()));
    return isOk;
}

static void writeProxy(const QString &filename, const QDBusIntrospection::Interfaces &interfaces)
{
    // open the file
    QString headerName = header(filename);
    QByteArray headerData;
    QTextStream hs(&headerData);

    QString cppName = cpp(filename);
    QByteArray cppData;
    QTextStream cs(&cppData);

    // write the header:
    writeHeader(hs, true);
    if (cppName != headerName)
        writeHeader(cs, false);

    // include guards:
    QString includeGuard;
    if (!headerName.isEmpty() && headerName != "-"_L1) {
        includeGuard = headerName.toUpper().replace(u'.', u'_');
        qsizetype pos = includeGuard.lastIndexOf(u'/');
        if (pos != -1)
            includeGuard = includeGuard.mid(pos + 1);
    } else {
        includeGuard = u"QDBUSXML2CPP_PROXY"_s;
    }

    hs << "#ifndef " << includeGuard << "\n"
          "#define " << includeGuard << "\n\n";

    // include our stuff:
    hs << "#include <QtCore/QObject>\n"
       << includeList;
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    hs << "#include <QtDBus/QtDBus>\n";
#else
    hs << "#include <QtDBus/QDBusAbstractInterface>\n"
          "#include <QtDBus/QDBusPendingReply>\n";
#endif

    for (const QString &include : std::as_const(includes)) {
        hs << "#include \"" << include << "\"\n";
        if (headerName.isEmpty())
            cs << "#include \"" << include << "\"\n";
    }

    for (const QString &include : std::as_const(globalIncludes)) {
        hs << "#include <" << include << ">\n";
        if (headerName.isEmpty())
            cs << "#include <" << include << ">\n";
    }

    hs << "\n";

    if (cppName != headerName) {
        if (!headerName.isEmpty() && headerName != "-"_L1)
            cs << "#include \"" << headerName << "\"\n\n";
    }

    for (const QDBusIntrospection::Interface *interface : interfaces) {
        QString className = classNameForInterface(interface->name, Proxy);

        // comment:
        hs << "/*\n"
              " * Proxy class for interface " << interface->name << "\n"
              " */\n";
        cs << "/*\n"
              " * Implementation of interface class " << className << "\n"
              " */\n\n";

        // class header:
        hs << "class " << className << ": public QDBusAbstractInterface\n"
              "{\n"
              "    Q_OBJECT\n";

        // the interface name
        hs << "public:\n"
              "    static inline const char *staticInterfaceName()\n"
              "    { return \"" << interface->name << "\"; }\n\n";

        // constructors/destructors:
        hs << "public:\n"
              "    " << className << "(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = nullptr);\n\n"
              "    ~" << className << "();\n\n";
        cs << className << "::" << className << "(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent)\n"
              "    : QDBusAbstractInterface(service, path, staticInterfaceName(), connection, parent)\n"
              "{\n"
              "}\n\n"
           << className << "::~" << className << "()\n"
              "{\n"
              "}\n\n";

        // properties:
        for (const QDBusIntrospection::Property &property : interface->properties) {
            QByteArray type = qtTypeName(property.name, property.type, property.annotations);
            QString getter = propertyGetter(property);
            QString setter = propertySetter(property);

            hs << "    Q_PROPERTY(" << type << " " << property.name;

            // getter:
            if (property.access != QDBusIntrospection::Property::Write)
                // it's readable
                hs << " READ " << getter;

            // setter
            if (property.access != QDBusIntrospection::Property::Read)
                // it's writeable
                hs << " WRITE " << setter;

            hs << ")\n";

            // getter:
            if (property.access != QDBusIntrospection::Property::Write) {
                hs << "    inline " << type << " " << getter << "() const\n"
                      "    { return qvariant_cast< " << type << " >(property(\""
                   << property.name << "\")); }\n";
            }

            // setter:
            if (property.access != QDBusIntrospection::Property::Read) {
                hs << "    inline void " << setter << "(" << constRefArg(type) << "value)\n"
                      "    { setProperty(\"" << property.name
                   << "\", QVariant::fromValue(value)); }\n";
            }

            hs << "\n";
        }

        // methods:
        hs << "public Q_SLOTS: // METHODS\n";
        for (const QDBusIntrospection::Method &method : interface->methods) {
            bool isDeprecated = method.annotations.value("org.freedesktop.DBus.Deprecated"_L1) == "true"_L1;
            bool isNoReply =
                method.annotations.value(ANNOTATION_NO_WAIT ""_L1) == "true"_L1;
            if (isNoReply && !method.outputArgs.isEmpty()) {
                fprintf(stderr, "%s: warning while processing '%s': method %s in interface %s is marked 'no-reply' but has output arguments.\n",
                        PROGRAMNAME, qPrintable(inputFile), qPrintable(method.name),
                        qPrintable(interface->name));
                continue;
            }

            if (isDeprecated)
                hs << "    Q_DECL_DEPRECATED ";
            else
                hs << "    ";

            if (isNoReply) {
                hs << "Q_NOREPLY inline void ";
            } else {
                hs << "inline QDBusPendingReply<";
                for (qsizetype i = 0; i < method.outputArgs.size(); ++i)
                    hs << (i > 0 ? ", " : "")
                       << templateArg(qtTypeName(method.outputArgs.at(i).name, method.outputArgs.at(i).type,
                                                 method.annotations, i, "Out"));
                hs << "> ";
            }

            hs << methodName(method) << "(";

            QStringList argNames = makeArgNames(method.inputArgs);
            writeArgList(hs, argNames, method.annotations, method.inputArgs);

            hs << ")\n"
                  "    {\n"
                  "        QList<QVariant> argumentList;\n";

            if (!method.inputArgs.isEmpty()) {
                hs << "        argumentList";
                for (qsizetype argPos = 0; argPos < method.inputArgs.size(); ++argPos)
                    hs << " << QVariant::fromValue(" << argNames.at(argPos) << ')';
                hs << ";\n";
            }

            if (isNoReply)
                hs << "        callWithArgumentList(QDBus::NoBlock, "
                      "QStringLiteral(\"" << method.name << "\"), argumentList);\n";
            else
                hs << "        return asyncCallWithArgumentList(QStringLiteral(\""
                   << method.name << "\"), argumentList);\n";

            // close the function:
            hs << "    }\n";

            if (method.outputArgs.size() > 1) {
                // generate the old-form QDBusReply methods with multiple incoming parameters
                hs << (isDeprecated ? "    Q_DECL_DEPRECATED " : "    ") << "inline QDBusReply<"
                   << templateArg(qtTypeName(method.outputArgs.first().name, method.outputArgs.first().type,
                                             method.annotations, 0, "Out"))
                   << "> ";
                hs << method.name << "(";

                QStringList argNames = makeArgNames(method.inputArgs, method.outputArgs);
                writeArgList(hs, argNames, method.annotations, method.inputArgs, method.outputArgs);

                hs << ")\n"
                      "    {\n"
                      "        QList<QVariant> argumentList;\n";

                qsizetype argPos = 0;
                if (!method.inputArgs.isEmpty()) {
                    hs << "        argumentList";
                    for (argPos = 0; argPos < method.inputArgs.size(); ++argPos)
                        hs << " << QVariant::fromValue(" << argNames.at(argPos) << ')';
                    hs << ";\n";
                }

                hs << "        QDBusMessage reply = callWithArgumentList(QDBus::Block, "
                      "QStringLiteral(\"" << method.name << "\"), argumentList);\n";

                argPos++;
                hs << "        if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().size() == "
                   << method.outputArgs.size() << ") {\n";

                // yes, starting from 1
                for (qsizetype i = 1; i < method.outputArgs.size(); ++i)
                    hs << "            " << argNames.at(argPos++) << " = qdbus_cast<"
                       << templateArg(qtTypeName(method.outputArgs.at(i).name, method.outputArgs.at(i).type,
                                                 method.annotations, i, "Out"))
                       << ">(reply.arguments().at(" << i << "));\n";
                hs << "        }\n"
                      "        return reply;\n"
                      "    }\n";
            }

            hs << "\n";
        }

        hs << "Q_SIGNALS: // SIGNALS\n";
        for (const QDBusIntrospection::Signal &signal : interface->signals_) {
            hs << "    ";
            if (signal.annotations.value("org.freedesktop.DBus.Deprecated"_L1) == "true"_L1)
                hs << "Q_DECL_DEPRECATED ";

            hs << "void " << signal.name << "(";

            QStringList argNames = makeArgNames(signal.outputArgs);
            writeSignalArgList(hs, argNames, signal.annotations, signal.outputArgs);

            hs << ");\n"; // finished for header
        }

        // close the class:
        hs << "};\n\n";
    }

    if (!skipNamespaces) {
        QStringList last;
        QDBusIntrospection::Interfaces::ConstIterator it = interfaces.constBegin();
        do
        {
            QStringList current;
            QString name;
            if (it != interfaces.constEnd()) {
                current = it->constData()->name.split(u'.');
                name = current.takeLast();
            }

            qsizetype i = 0;
            while (i < current.size() && i < last.size() && current.at(i) == last.at(i))
                ++i;

            // i parts matched
            // close last.arguments().size() - i namespaces:
            for (qsizetype j = i; j < last.size(); ++j)
                hs << QString((last.size() - j - 1 + i) * 2, u' ') << "}\n";

            // open current.arguments().size() - i namespaces
            for (qsizetype j = i; j < current.size(); ++j)
                hs << QString(j * 2, u' ') << "namespace " << current.at(j) << " {\n";

            // add this class:
            if (!name.isEmpty()) {
                hs << QString(current.size() * 2, u' ')
                   << "using " << name << " = ::" << classNameForInterface(it->constData()->name, Proxy)
                   << ";\n";
            }

            if (it == interfaces.constEnd())
                break;
            ++it;
            last = current;
        } while (true);
    }

    // close the include guard
    hs << "#endif\n";

    QString mocName = moc(filename);
    if (includeMocs && !mocName.isEmpty())
        cs << "\n"
              "#include \"" << mocName << "\"\n";

    cs.flush();
    hs.flush();

    QFile file;
    const bool headerOpen = openFile(headerName, file);
    if (headerOpen)
        file.write(headerData);

    if (headerName == cppName) {
        if (headerOpen)
            file.write(cppData);
    } else {
        QFile cppFile;
        if (openFile(cppName, cppFile))
            cppFile.write(cppData);
    }
}

static void writeAdaptor(const QString &filename, const QDBusIntrospection::Interfaces &interfaces)
{
    // open the file
    QString headerName = header(filename);
    QByteArray headerData;
    QTextStream hs(&headerData);

    QString cppName = cpp(filename);
    QByteArray cppData;
    QTextStream cs(&cppData);

    // write the headers
    writeHeader(hs, false);
    if (cppName != headerName)
        writeHeader(cs, true);

    // include guards:
    QString includeGuard;
    if (!headerName.isEmpty() && headerName != "-"_L1) {
        includeGuard = headerName.toUpper().replace(u'.', u'_');
        qsizetype pos = includeGuard.lastIndexOf(u'/');
        if (pos != -1)
            includeGuard = includeGuard.mid(pos + 1);
    } else {
        includeGuard = u"QDBUSXML2CPP_ADAPTOR"_s;
    }

    hs << "#ifndef " << includeGuard << "\n"
          "#define " << includeGuard << "\n\n";

    // include our stuff:
    hs << "#include <QtCore/QObject>\n";
    if (cppName == headerName)
        hs << "#include <QtCore/QMetaObject>\n"
              "#include <QtCore/QVariant>\n";
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    hs << "#include <QtDBus/QtDBus>\n";
#else
    hs << "#include <QtDBus/QDBusAbstractAdaptor>\n"
          "#include <QtDBus/QDBusObjectPath>\n";
#endif

    for (const QString &include : std::as_const(includes)) {
        hs << "#include \"" << include << "\"\n";
        if (headerName.isEmpty())
            cs << "#include \"" << include << "\"\n";
    }

    for (const QString &include : std::as_const(globalIncludes)) {
        hs << "#include <" << include << ">\n";
        if (headerName.isEmpty())
            cs << "#include <" << include << ">\n";
    }

    if (cppName != headerName) {
        if (!headerName.isEmpty() && headerName != "-"_L1)
            cs << "#include \"" << headerName << "\"\n";

        cs << "#include <QtCore/QMetaObject>\n"
           << includeList
           << "\n";
        hs << forwardDeclarations;
    } else {
        hs << includeList;
    }

    hs << "\n";

    QString parent = parentClassName;
    if (parentClassName.isEmpty())
        parent = u"QObject"_s;

    for (const QDBusIntrospection::Interface *interface : interfaces) {
        QString className = classNameForInterface(interface->name, Adaptor);

        // comment:
        hs << "/*\n"
              " * Adaptor class for interface " << interface->name << "\n"
              " */\n";
        cs << "/*\n"
              " * Implementation of adaptor class " << className << "\n"
              " */\n\n";

        // class header:
        hs << "class " << className << ": public QDBusAbstractAdaptor\n"
              "{\n"
              "    Q_OBJECT\n"
              "    Q_CLASSINFO(\"D-Bus Interface\", \"" << interface->name << "\")\n"
              "    Q_CLASSINFO(\"D-Bus Introspection\", \"\"\n"
           << stringify(interface->introspection)
           << "        \"\")\n"
              "public:\n"
              "    " << className << "(" << parent << " *parent);\n"
              "    virtual ~" << className << "();\n\n";

        if (!parentClassName.isEmpty())
            hs << "    inline " << parent << " *parent() const\n"
                  "    { return static_cast<" << parent << " *>(QObject::parent()); }\n\n";

        // constructor/destructor
        cs << className << "::" << className << "(" << parent << " *parent)\n"
              "    : QDBusAbstractAdaptor(parent)\n"
              "{\n"
              "    // constructor\n"
              "    setAutoRelaySignals(true);\n"
              "}\n\n"
           << className << "::~" << className << "()\n"
              "{\n"
              "    // destructor\n"
              "}\n\n";

        hs << "public: // PROPERTIES\n";
        for (const QDBusIntrospection::Property &property : interface->properties) {
            QByteArray type = qtTypeName(property.name, property.type, property.annotations);
            QString constRefType = constRefArg(type);
            QString getter = propertyGetter(property);
            QString setter = propertySetter(property);

            hs << "    Q_PROPERTY(" << type << " " << property.name;
            if (property.access != QDBusIntrospection::Property::Write)
                hs << " READ " << getter;
            if (property.access != QDBusIntrospection::Property::Read)
                hs << " WRITE " << setter;
            hs << ")\n";

            // getter:
            if (property.access != QDBusIntrospection::Property::Write) {
                hs << "    " << type << " " << getter << "() const;\n";
                cs << type << " "
                   << className << "::" << getter << "() const\n"
                      "{\n"
                      "    // get the value of property " << property.name << "\n"
                      "    return qvariant_cast< " << type <<" >(parent()->property(\"" << property.name << "\"));\n"
                      "}\n\n";
            }

            // setter
            if (property.access != QDBusIntrospection::Property::Read) {
                hs << "    void " << setter << "(" << constRefType << "value);\n";
                cs << "void " << className << "::" << setter << "(" << constRefType << "value)\n"
                      "{\n"
                      "    // set the value of property " << property.name << "\n"
                      "    parent()->setProperty(\"" << property.name << "\", QVariant::fromValue(value";
                if (constRefType.contains("QDBusVariant"_L1))
                    cs << ".variant()";
                cs << "));\n"
                      "}\n\n";
            }

            hs << "\n";
        }

        hs << "public Q_SLOTS: // METHODS\n";
        for (const QDBusIntrospection::Method &method : interface->methods) {
            bool isNoReply =
                method.annotations.value(ANNOTATION_NO_WAIT ""_L1) == "true"_L1;
            if (isNoReply && !method.outputArgs.isEmpty()) {
                fprintf(stderr, "%s: warning while processing '%s': method %s in interface %s is marked 'no-reply' but has output arguments.\n",
                        PROGRAMNAME, qPrintable(inputFile), qPrintable(method.name), qPrintable(interface->name));
                continue;
            }

            hs << "    ";
            QByteArray returnType;
            if (isNoReply) {
                hs << "Q_NOREPLY void ";
                cs << "void ";
            } else if (method.outputArgs.isEmpty()) {
                hs << "void ";
                cs << "void ";
            } else {
                returnType = qtTypeName(method.outputArgs.first().name, method.outputArgs.first().type,
                                        method.annotations, 0, "Out");
                hs << returnType << " ";
                cs << returnType << " ";
            }

            QString name = methodName(method);
            hs << name << "(";
            cs << className << "::" << name << "(";

            QStringList argNames = makeArgNames(method.inputArgs, method.outputArgs);
            writeArgList(hs, argNames, method.annotations, method.inputArgs, method.outputArgs);
            writeArgList(cs, argNames, method.annotations, method.inputArgs, method.outputArgs);

            hs << ");\n"; // finished for header
            cs << ")\n"
                  "{\n"
                  "    // handle method call " << interface->name << "." << methodName(method) << "\n";

            // make the call
            bool usingInvokeMethod = false;
            if (parentClassName.isEmpty() && method.inputArgs.size() <= 10
                && method.outputArgs.size() <= 1)
                usingInvokeMethod = true;

            if (usingInvokeMethod) {
                // we are using QMetaObject::invokeMethod
                if (!returnType.isEmpty())
                    cs << "    " << returnType << " " << argNames.at(method.inputArgs.size())
                       << ";\n";

                static const char invoke[] = "    QMetaObject::invokeMethod(parent(), \"";
                cs << invoke << name << "\"";

                if (!method.outputArgs.isEmpty())
                    cs << ", Q_RETURN_ARG("
                       << qtTypeName(method.outputArgs.at(0).name, method.outputArgs.at(0).type, method.annotations,
                                     0, "Out")
                       << ", " << argNames.at(method.inputArgs.size()) << ")";

                for (qsizetype i = 0; i < method.inputArgs.size(); ++i)
                    cs << ", Q_ARG("
                       << qtTypeName(method.inputArgs.at(i).name, method.inputArgs.at(i).type, method.annotations,
                                     i, "In")
                       << ", " << argNames.at(i) << ")";

                cs << ");\n";

                if (!returnType.isEmpty())
                    cs << "    return " << argNames.at(method.inputArgs.size()) << ";\n";
            } else {
                if (parentClassName.isEmpty())
                    cs << "    //";
                else
                    cs << "    ";

                if (!method.outputArgs.isEmpty())
                    cs << "return ";

                if (parentClassName.isEmpty())
                    cs << "static_cast<YourObjectType *>(parent())->";
                else
                    cs << "parent()->";
                cs << name << "(";

                qsizetype argPos = 0;
                bool first = true;
                for (qsizetype i = 0; i < method.inputArgs.size(); ++i) {
                    cs << (first ? "" : ", ") << argNames.at(argPos++);
                    first = false;
                }
                ++argPos;           // skip retval, if any
                for (qsizetype i = 1; i < method.outputArgs.size(); ++i) {
                    cs << (first ? "" : ", ") << argNames.at(argPos++);
                    first = false;
                }

                cs << ");\n";
            }
            cs << "}\n\n";
        }

        hs << "Q_SIGNALS: // SIGNALS\n";
        for (const QDBusIntrospection::Signal &signal : interface->signals_) {
            hs << "    void " << signal.name << "(";

            QStringList argNames = makeArgNames(signal.outputArgs);
            writeSignalArgList(hs, argNames, signal.annotations, signal.outputArgs);

            hs << ");\n"; // finished for header
        }

        // close the class:
        hs << "};\n\n";
    }

    // close the include guard
    hs << "#endif\n";

    QString mocName = moc(filename);
    if (includeMocs && !mocName.isEmpty())
        cs << "\n"
              "#include \"" << mocName << "\"\n";

    cs.flush();
    hs.flush();

    QFile file;
    const bool headerOpen = openFile(headerName, file);
    if (headerOpen)
        file.write(headerData);

    if (headerName == cppName) {
        if (headerOpen)
            file.write(cppData);
    } else {
        QFile cppFile;
        if (openFile(cppName, cppFile))
            cppFile.write(cppData);
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral(PROGRAMNAME));
    QCoreApplication::setApplicationVersion(QStringLiteral(PROGRAMVERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(
            "Produces the C++ code to implement the interfaces defined in the input file.\n\n"
            "If the file name given to the options -a and -p does not end in .cpp or .h, the\n"
            "program will automatically append the suffixes and produce both files.\n"
            "You can also use a colon (:) to separate the header name from the source file\n"
            "name, as in '-a filename_p.h:filename.cpp'.\n\n"
            "If you pass a dash (-) as the argument to either -p or -a, the output is written\n"
            "to the standard output."_L1);

    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(u"xml-or-xml-file"_s, u"XML file to use."_s);
    parser.addPositionalArgument(u"interfaces"_s, u"List of interfaces to use."_s,
                u"[interfaces ...]"_s);

    QCommandLineOption adapterCodeOption(QStringList{u"a"_s, u"adaptor"_s},
                u"Write the adaptor code to <filename>"_s, u"filename"_s);
    parser.addOption(adapterCodeOption);

    QCommandLineOption classNameOption(QStringList{u"c"_s, u"classname"_s},
                u"Use <classname> as the class name for the generated classes. "
                u"This option can only be used when processing a single interface."_s,
                u"classname"_s);
    parser.addOption(classNameOption);

    QCommandLineOption addIncludeOption(QStringList{u"i"_s, u"include"_s},
                u"Add #include \"filename\" to the output"_s, u"filename"_s);
    parser.addOption(addIncludeOption);

    QCommandLineOption addGlobalIncludeOption(QStringList{u"I"_s, u"global-include"_s},
                u"Add #include <filename> to the output"_s, u"filename"_s);
    parser.addOption(addGlobalIncludeOption);

    QCommandLineOption adapterParentOption(u"l"_s,
                u"When generating an adaptor, use <classname> as the parent class"_s, u"classname"_s);
    parser.addOption(adapterParentOption);

    QCommandLineOption mocIncludeOption(QStringList{u"m"_s, u"moc"_s},
                u"Generate #include \"filename.moc\" statements in the .cpp files"_s);
    parser.addOption(mocIncludeOption);

    QCommandLineOption noNamespaceOption(QStringList{u"N"_s, u"no-namespaces"_s},
                u"Don't use namespaces"_s);
    parser.addOption(noNamespaceOption);

    QCommandLineOption proxyCodeOption(QStringList{u"p"_s, u"proxy"_s},
                u"Write the proxy code to <filename>"_s, u"filename"_s);
    parser.addOption(proxyCodeOption);

    QCommandLineOption verboseOption(QStringList{u"V"_s, u"verbose"_s},
                u"Be verbose."_s);
    parser.addOption(verboseOption);

    parser.process(app);

    adaptorFile = parser.value(adapterCodeOption);
    globalClassName = parser.value(classNameOption);
    includes = parser.values(addIncludeOption);
    globalIncludes = parser.values(addGlobalIncludeOption);
    parentClassName = parser.value(adapterParentOption);
    includeMocs = parser.isSet(mocIncludeOption);
    skipNamespaces = parser.isSet(noNamespaceOption);
    proxyFile = parser.value(proxyCodeOption);
    verbose = parser.isSet(verboseOption);

    wantedInterfaces = parser.positionalArguments();
    if (!wantedInterfaces.isEmpty()) {
        inputFile = wantedInterfaces.takeFirst();

        QFileInfo inputInfo(inputFile);
        if (!inputInfo.exists() || !inputInfo.isFile() || !inputInfo.isReadable()) {
            qCritical("Error: Input %s is not a file or cannot be accessed\n", qPrintable(inputFile));
            return 1;
        }
    }

    if (verbose)
        QLoggingCategory::setFilterRules(u"dbus.parser.debug=true"_s);

    QDBusIntrospection::Interfaces interfaces = readInput();
    cleanInterfaces(interfaces);

    if (!globalClassName.isEmpty() && interfaces.count() != 1) {
        qCritical("Option -c/--classname can only be used with a single interface.\n");
        return 1;
    }

    QStringList args = app.arguments();
    args.removeFirst();
    commandLine = PROGRAMNAME " "_L1 + args.join(u' ');

    if (!proxyFile.isEmpty() || adaptorFile.isEmpty())
        writeProxy(proxyFile, interfaces);

    if (!adaptorFile.isEmpty())
        writeAdaptor(adaptorFile, interfaces);

    return 0;
}

