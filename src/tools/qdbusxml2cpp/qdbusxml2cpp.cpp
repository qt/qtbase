/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
#define PROGRAMCOPYRIGHT "Copyright (C) 2020 The Qt Company Ltd."

#define ANNOTATION_NO_WAIT      "org.freedesktop.DBus.Method.NoReply"

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
static QStringList wantedInterfaces;

static const char includeList[] =
    "#include <QtCore/QByteArray>\n"
    "#include <QtCore/QList>\n"
    "#include <QtCore/QMap>\n"
    "#include <QtCore/QString>\n"
    "#include <QtCore/QStringList>\n"
    "#include <QtCore/QVariant>\n";

static const char forwardDeclarations[] =
    "QT_BEGIN_NAMESPACE\n"
    "class QByteArray;\n"
    "template<class T> class QList;\n"
    "template<class Key, class Value> class QMap;\n"
    "class QString;\n"
    "class QStringList;\n"
    "class QVariant;\n"
    "QT_END_NAMESPACE\n";

static QDBusIntrospection::Interfaces readInput()
{
    QFile input(inputFile);
    if (inputFile.isEmpty() || inputFile == QLatin1String("-")) {
        input.open(stdin, QIODevice::ReadOnly);
    } else {
        input.open(QIODevice::ReadOnly);
    }

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

// produce a header name from the file name
static QString header(const QString &name)
{
    QStringList parts = name.split(QLatin1Char(':'));
    QString retval = parts.first();

    if (retval.isEmpty() || retval == QLatin1String("-"))
        return retval;

    if (!retval.endsWith(QLatin1String(".h")) && !retval.endsWith(QLatin1String(".cpp")) &&
        !retval.endsWith(QLatin1String(".cc")))
        retval.append(QLatin1String(".h"));

    return retval;
}

// produce a cpp name from the file name
static QString cpp(const QString &name)
{
    QStringList parts = name.split(QLatin1Char(':'));
    QString retval = parts.last();

    if (retval.isEmpty() || retval == QLatin1String("-"))
        return retval;

    if (!retval.endsWith(QLatin1String(".h")) && !retval.endsWith(QLatin1String(".cpp")) &&
        !retval.endsWith(QLatin1String(".cc")))
        retval.append(QLatin1String(".cpp"));

    return retval;
}

// produce a moc name from the file name
static QString moc(const QString &name)
{
    QString retval = header(name);
    if (retval.isEmpty())
        return retval;

    retval.truncate(retval.length() - 1); // drop the h in .h
    retval += QLatin1String("moc");
    return retval;
}

static QTextStream &writeHeader(QTextStream &ts, bool changesWillBeLost)
{
    ts << "/*" << Qt::endl
       << " * This file was generated by " PROGRAMNAME " version " PROGRAMVERSION << Qt::endl
       << " * Command line was: " << commandLine << Qt::endl
       << " *" << Qt::endl
       << " * " PROGRAMNAME " is " PROGRAMCOPYRIGHT << Qt::endl
       << " *" << Qt::endl
       << " * This is an auto-generated file." << Qt::endl;

    if (changesWillBeLost)
        ts << " * Do not edit! All changes made to it will be lost." << Qt::endl;
    else
        ts << " * This file may have been hand-edited. Look for HAND-EDIT comments" << Qt::endl
           << " * before re-generating it." << Qt::endl;

    ts << " */" << Qt::endl
       << Qt::endl;

    return ts;
}

enum ClassType { Proxy, Adaptor };
static QString classNameForInterface(const QString &interface, ClassType classType)
{
    if (!globalClassName.isEmpty())
        return globalClassName;

    const auto parts = interface.splitRef(QLatin1Char('.'));

    QString retval;
    if (classType == Proxy) {
        for (const auto &part : parts)
            retval += part[0].toUpper() + part.mid(1);
    } else {
        retval += parts.last()[0].toUpper() + parts.last().mid(1);
    }

    if (classType == Proxy)
        retval += QLatin1String("Interface");
    else
        retval += QLatin1String("Adaptor");

    return retval;
}

// ### Qt6 Remove the two isSignal ifs
// They are only here because before signal arguments where previously searched as "In" so to maintain compatibility
// we first search for "Out" and if not found we search for "In"
static QByteArray qtTypeName(const QString &signature, const QDBusIntrospection::Annotations &annotations, int paramId = -1, const char *direction = "Out", bool isSignal = false)
{
    int type = QDBusMetaType::signatureToType(signature.toLatin1());
    if (type == QMetaType::UnknownType) {
        QString annotationName = QString::fromLatin1("org.qtproject.QtDBus.QtTypeName");
        if (paramId >= 0)
            annotationName += QString::fromLatin1(".%1%2").arg(QLatin1String(direction)).arg(paramId);
        QString qttype = annotations.value(annotationName);
        if (!qttype.isEmpty())
            return std::move(qttype).toLatin1();

        QString oldAnnotationName = QString::fromLatin1("com.trolltech.QtDBus.QtTypeName");
        if (paramId >= 0)
            oldAnnotationName += QString::fromLatin1(".%1%2").arg(QLatin1String(direction)).arg(paramId);
        qttype = annotations.value(oldAnnotationName);

        if (qttype.isEmpty()) {
            if (!isSignal || qstrcmp(direction, "Out") == 0) {
                fprintf(stderr, "%s: Got unknown type `%s' processing '%s'\n",
                        PROGRAMNAME, qPrintable(signature), qPrintable(inputFile));
                fprintf(stderr, "You should add <annotation name=\"%s\" value=\"<type>\"/> to the XML description\n",
                        qPrintable(annotationName));
            }

            if (isSignal)
                return qtTypeName(signature, annotations, paramId, "In", isSignal);

            exit(1);
        }

        fprintf(stderr, "%s: Warning: deprecated annotation '%s' found while processing '%s'; "
                        "suggest updating to '%s'\n",
                PROGRAMNAME, qPrintable(oldAnnotationName), qPrintable(inputFile),
                qPrintable(annotationName));
        return std::move(qttype).toLatin1();
    }

    return QVariant::typeToName(QVariant::Type(type));
}

static QString nonConstRefArg(const QByteArray &arg)
{
    return QLatin1String(arg + " &");
}

static QString templateArg(const QByteArray &arg)
{
    if (!arg.endsWith('>'))
        return QLatin1String(arg);

    return QLatin1String(arg + ' ');
}

static QString constRefArg(const QByteArray &arg)
{
    if (!arg.startsWith('Q'))
        return QLatin1String(arg + ' ');
    else
        return QString( QLatin1String("const %1 &") ).arg( QLatin1String(arg) );
}

static QStringList makeArgNames(const QDBusIntrospection::Arguments &inputArgs,
                                const QDBusIntrospection::Arguments &outputArgs =
                                QDBusIntrospection::Arguments())
{
    QStringList retval;
    const int numInputArgs = inputArgs.count();
    const int numOutputArgs = outputArgs.count();
    retval.reserve(numInputArgs + numOutputArgs);
    for (int i = 0; i < numInputArgs; ++i) {
        const QDBusIntrospection::Argument &arg = inputArgs.at(i);
        QString name = arg.name;
        if (name.isEmpty())
            name = QString( QLatin1String("in%1") ).arg(i);
        else
            name.replace(QLatin1Char('-'), QLatin1Char('_'));
        while (retval.contains(name))
            name += QLatin1String("_");
        retval << name;
    }
    for (int i = 0; i < numOutputArgs; ++i) {
        const QDBusIntrospection::Argument &arg = outputArgs.at(i);
        QString name = arg.name;
        if (name.isEmpty())
            name = QString( QLatin1String("out%1") ).arg(i);
        else
            name.replace(QLatin1Char('-'), QLatin1Char('_'));
        while (retval.contains(name))
            name += QLatin1String("_");
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
    int argPos = 0;
    for (int i = 0; i < inputArgs.count(); ++i) {
        const QDBusIntrospection::Argument &arg = inputArgs.at(i);
        QString type = constRefArg(qtTypeName(arg.type, annotations, i, "In"));

        if (!first)
            ts << ", ";
        ts << type << argNames.at(argPos++);
        first = false;
    }

    argPos++;

    // output args
    // yes, starting from 1
    for (int i = 1; i < outputArgs.count(); ++i) {
        const QDBusIntrospection::Argument &arg = outputArgs.at(i);

        if (!first)
            ts << ", ";
        ts << nonConstRefArg(qtTypeName(arg.type, annotations, i, "Out"))
           << argNames.at(argPos++);
        first = false;
    }
}

static void writeSignalArgList(QTextStream &ts, const QStringList &argNames,
                         const QDBusIntrospection::Annotations &annotations,
                         const QDBusIntrospection::Arguments &outputArgs)
{
    bool first = true;
    int argPos = 0;
    for (int i = 0; i < outputArgs.count(); ++i) {
        const QDBusIntrospection::Argument &arg = outputArgs.at(i);
        QString type = constRefArg(qtTypeName(arg.type, annotations, i, "Out", true /* isSignal */));

        if (!first)
            ts << ", ";
        ts << type << argNames.at(argPos++);
        first = false;
    }
}

static QString propertyGetter(const QDBusIntrospection::Property &property)
{
    QString getter = property.annotations.value(QLatin1String("org.qtproject.QtDBus.PropertyGetter"));
    if (!getter.isEmpty())
        return getter;

    getter = property.annotations.value(QLatin1String("com.trolltech.QtDBus.propertyGetter"));
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
    QString setter = property.annotations.value(QLatin1String("org.qtproject.QtDBus.PropertySetter"));
    if (!setter.isEmpty())
        return setter;

    setter = property.annotations.value(QLatin1String("com.trolltech.QtDBus.propertySetter"));
    if (!setter.isEmpty()) {
        fprintf(stderr, "%s: Warning: deprecated annotation 'com.trolltech.QtDBus.propertySetter' found"
                " while processing '%s';"
                " suggest updating to 'org.qtproject.QtDBus.PropertySetter'\n",
                PROGRAMNAME, qPrintable(inputFile));
        return setter;
    }

    setter = QLatin1String("set") + property.name;
    setter[3] = setter[3].toUpper();
    return setter;
}

static QString methodName(const QDBusIntrospection::Method &method)
{
    QString name = method.annotations.value(QStringLiteral("org.qtproject.QtDBus.MethodName"));
    if (!name.isEmpty())
        return name;

    return method.name;
}

static QString stringify(const QString &data)
{
    QString retval;
    int i;
    for (i = 0; i < data.length(); ++i) {
        retval += QLatin1Char('\"');
        for ( ; i < data.length() && data[i] != QLatin1Char('\n') && data[i] != QLatin1Char('\r'); ++i)
            if (data[i] == QLatin1Char('\"'))
                retval += QLatin1String("\\\"");
            else
                retval += data[i];
        if (i+1 < data.length() && data[i] == QLatin1Char('\r') && data[i+1] == QLatin1Char('\n'))
            i++;
        retval += QLatin1String("\\n\"\n");
    }
    return retval;
}

static bool openFile(const QString &fileName, QFile &file)
{
    if (fileName.isEmpty())
        return false;

    bool isOk = false;
    if (fileName == QLatin1String("-")) {
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
    if (!headerName.isEmpty() && headerName != QLatin1String("-")) {
        includeGuard = headerName.toUpper().replace(QLatin1Char('.'), QLatin1Char('_'));
        int pos = includeGuard.lastIndexOf(QLatin1Char('/'));
        if (pos != -1)
            includeGuard = includeGuard.mid(pos + 1);
    } else {
        includeGuard = QLatin1String("QDBUSXML2CPP_PROXY");
    }
    includeGuard = QString(QLatin1String("%1"))
                   .arg(includeGuard);
    hs << "#ifndef " << includeGuard << Qt::endl
       << "#define " << includeGuard << Qt::endl
       << Qt::endl;

    // include our stuff:
    hs << "#include <QtCore/QObject>" << Qt::endl
       << includeList
       << "#include <QtDBus/QtDBus>" << Qt::endl;

    for (const QString &include : qAsConst(includes)) {
        hs << "#include \"" << include << "\"" << Qt::endl;
        if (headerName.isEmpty())
            cs << "#include \"" << include << "\"" << Qt::endl;
    }

    hs << Qt::endl;

    if (cppName != headerName) {
        if (!headerName.isEmpty() && headerName != QLatin1String("-"))
            cs << "#include \"" << headerName << "\"" << Qt::endl << Qt::endl;
    }

    for (const QDBusIntrospection::Interface *interface : interfaces) {
        QString className = classNameForInterface(interface->name, Proxy);

        // comment:
        hs << "/*" << Qt::endl
           << " * Proxy class for interface " << interface->name << Qt::endl
           << " */" << Qt::endl;
        cs << "/*" << Qt::endl
           << " * Implementation of interface class " << className << Qt::endl
           << " */" << Qt::endl
           << Qt::endl;

        // class header:
        hs << "class " << className << ": public QDBusAbstractInterface" << Qt::endl
           << "{" << Qt::endl
           << "    Q_OBJECT" << Qt::endl;

        // the interface name
        hs << "public:" << Qt::endl
           << "    static inline const char *staticInterfaceName()" << Qt::endl
           << "    { return \"" << interface->name << "\"; }" << Qt::endl
           << Qt::endl;

        // constructors/destructors:
        hs << "public:" << Qt::endl
           << "    " << className << "(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = nullptr);" << Qt::endl
           << Qt::endl
           << "    ~" << className << "();" << Qt::endl
           << Qt::endl;
        cs << className << "::" << className << "(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent)" << Qt::endl
           << "    : QDBusAbstractInterface(service, path, staticInterfaceName(), connection, parent)" << Qt::endl
           << "{" << Qt::endl
           << "}" << Qt::endl
           << Qt::endl
           << className << "::~" << className << "()" << Qt::endl
           << "{" << Qt::endl
           << "}" << Qt::endl
           << Qt::endl;

        // properties:
        for (const QDBusIntrospection::Property &property : interface->properties) {
            QByteArray type = qtTypeName(property.type, property.annotations);
            QString getter = propertyGetter(property);
            QString setter = propertySetter(property);

            hs << "    Q_PROPERTY(" << type << " " << property.name;

            // getter:
            if (property.access != QDBusIntrospection::Property::Write)
                // it's readble
                hs << " READ " << getter;

            // setter
            if (property.access != QDBusIntrospection::Property::Read)
                // it's writeable
                hs << " WRITE " << setter;

            hs << ")" << Qt::endl;

            // getter:
            if (property.access != QDBusIntrospection::Property::Write) {
                hs << "    inline " << type << " " << getter << "() const" << Qt::endl
                    << "    { return qvariant_cast< " << type << " >(property(\""
                    << property.name << "\")); }" << Qt::endl;
            }

            // setter:
            if (property.access != QDBusIntrospection::Property::Read) {
                hs << "    inline void " << setter << "(" << constRefArg(type) << "value)" << Qt::endl
                   << "    { setProperty(\"" << property.name
                   << "\", QVariant::fromValue(value)); }" << Qt::endl;
            }

            hs << Qt::endl;
        }

        // methods:
        hs << "public Q_SLOTS: // METHODS" << Qt::endl;
        for (const QDBusIntrospection::Method &method : interface->methods) {
            bool isDeprecated = method.annotations.value(QLatin1String("org.freedesktop.DBus.Deprecated")) == QLatin1String("true");
            bool isNoReply =
                method.annotations.value(QLatin1String(ANNOTATION_NO_WAIT)) == QLatin1String("true");
            if (isNoReply && !method.outputArgs.isEmpty()) {
                fprintf(stderr, "%s: warning while processing '%s': method %s in interface %s is marked 'no-reply' but has output arguments.\n",
                        PROGRAMNAME, qPrintable(inputFile), qPrintable(method.name),
                        qPrintable(interface->name));
                continue;
            }

            hs << "    inline "
               << (isDeprecated ? "Q_DECL_DEPRECATED " : "");

            if (isNoReply) {
                hs << "Q_NOREPLY void ";
            } else {
                hs << "QDBusPendingReply<";
                for (int i = 0; i < method.outputArgs.count(); ++i)
                    hs << (i > 0 ? ", " : "")
                       << templateArg(qtTypeName(method.outputArgs.at(i).type, method.annotations, i, "Out"));
                hs << "> ";
            }

            hs << methodName(method) << "(";

            QStringList argNames = makeArgNames(method.inputArgs);
            writeArgList(hs, argNames, method.annotations, method.inputArgs);

            hs << ")" << Qt::endl
               << "    {" << Qt::endl
               << "        QList<QVariant> argumentList;" << Qt::endl;

            if (!method.inputArgs.isEmpty()) {
                hs << "        argumentList";
                for (int argPos = 0; argPos < method.inputArgs.count(); ++argPos)
                    hs << " << QVariant::fromValue(" << argNames.at(argPos) << ')';
                hs << ";" << Qt::endl;
            }

            if (isNoReply)
                hs << "        callWithArgumentList(QDBus::NoBlock, "
                   <<  "QStringLiteral(\"" << method.name << "\"), argumentList);" << Qt::endl;
            else
                hs << "        return asyncCallWithArgumentList(QStringLiteral(\""
                   << method.name << "\"), argumentList);" << Qt::endl;

            // close the function:
            hs << "    }" << Qt::endl;

            if (method.outputArgs.count() > 1) {
                // generate the old-form QDBusReply methods with multiple incoming parameters
                hs << "    inline "
                   << (isDeprecated ? "Q_DECL_DEPRECATED " : "")
                   << "QDBusReply<"
                   << templateArg(qtTypeName(method.outputArgs.first().type, method.annotations, 0, "Out")) << "> ";
                hs << method.name << "(";

                QStringList argNames = makeArgNames(method.inputArgs, method.outputArgs);
                writeArgList(hs, argNames, method.annotations, method.inputArgs, method.outputArgs);

                hs << ")" << Qt::endl
                   << "    {" << Qt::endl
                   << "        QList<QVariant> argumentList;" << Qt::endl;

                int argPos = 0;
                if (!method.inputArgs.isEmpty()) {
                    hs << "        argumentList";
                    for (argPos = 0; argPos < method.inputArgs.count(); ++argPos)
                        hs << " << QVariant::fromValue(" << argNames.at(argPos) << ')';
                    hs << ";" << Qt::endl;
                }

                hs << "        QDBusMessage reply = callWithArgumentList(QDBus::Block, "
                   <<  "QStringLiteral(\"" << method.name << "\"), argumentList);" << Qt::endl;

                argPos++;
                hs << "        if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().count() == "
                   << method.outputArgs.count() << ") {" << Qt::endl;

                // yes, starting from 1
                for (int i = 1; i < method.outputArgs.count(); ++i)
                    hs << "            " << argNames.at(argPos++) << " = qdbus_cast<"
                       << templateArg(qtTypeName(method.outputArgs.at(i).type, method.annotations, i, "Out"))
                       << ">(reply.arguments().at(" << i << "));" << Qt::endl;
                hs << "        }" << Qt::endl
                   << "        return reply;" << Qt::endl
                   << "    }" << Qt::endl;
            }

            hs << Qt::endl;
        }

        hs << "Q_SIGNALS: // SIGNALS" << Qt::endl;
        for (const QDBusIntrospection::Signal &signal : interface->signals_) {
            hs << "    ";
            if (signal.annotations.value(QLatin1String("org.freedesktop.DBus.Deprecated")) ==
                QLatin1String("true"))
                hs << "Q_DECL_DEPRECATED ";

            hs << "void " << signal.name << "(";

            QStringList argNames = makeArgNames(signal.outputArgs);
            writeSignalArgList(hs, argNames, signal.annotations, signal.outputArgs);

            hs << ");" << Qt::endl; // finished for header
        }

        // close the class:
        hs << "};" << Qt::endl
           << Qt::endl;
    }

    if (!skipNamespaces) {
        QStringList last;
        QDBusIntrospection::Interfaces::ConstIterator it = interfaces.constBegin();
        do
        {
            QStringList current;
            QString name;
            if (it != interfaces.constEnd()) {
                current = it->constData()->name.split(QLatin1Char('.'));
                name = current.takeLast();
            }

            int i = 0;
            while (i < current.count() && i < last.count() && current.at(i) == last.at(i))
                ++i;

            // i parts matched
            // close last.arguments().count() - i namespaces:
            for (int j = i; j < last.count(); ++j)
                hs << QString((last.count() - j - 1 + i) * 2, QLatin1Char(' ')) << "}" << Qt::endl;

            // open current.arguments().count() - i namespaces
            for (int j = i; j < current.count(); ++j)
                hs << QString(j * 2, QLatin1Char(' ')) << "namespace " << current.at(j) << " {" << Qt::endl;

            // add this class:
            if (!name.isEmpty()) {
                hs << QString(current.count() * 2, QLatin1Char(' '))
                   << "typedef ::" << classNameForInterface(it->constData()->name, Proxy)
                   << " " << name << ";" << Qt::endl;
            }

            if (it == interfaces.constEnd())
                break;
            ++it;
            last = current;
        } while (true);
    }

    // close the include guard
    hs << "#endif" << Qt::endl;

    QString mocName = moc(filename);
    if (includeMocs && !mocName.isEmpty())
        cs << Qt::endl
           << "#include \"" << mocName << "\"" << Qt::endl;

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
    if (!headerName.isEmpty() && headerName != QLatin1String("-")) {
        includeGuard = headerName.toUpper().replace(QLatin1Char('.'), QLatin1Char('_'));
        int pos = includeGuard.lastIndexOf(QLatin1Char('/'));
        if (pos != -1)
            includeGuard = includeGuard.mid(pos + 1);
    } else {
        includeGuard = QLatin1String("QDBUSXML2CPP_ADAPTOR");
    }
    includeGuard = QString(QLatin1String("%1"))
                   .arg(includeGuard);
    hs << "#ifndef " << includeGuard << Qt::endl
       << "#define " << includeGuard << Qt::endl
       << Qt::endl;

    // include our stuff:
    hs << "#include <QtCore/QObject>" << Qt::endl;
    if (cppName == headerName)
        hs << "#include <QtCore/QMetaObject>" << Qt::endl
           << "#include <QtCore/QVariant>" << Qt::endl;
    hs << "#include <QtDBus/QtDBus>" << Qt::endl;

    for (const QString &include : qAsConst(includes)) {
        hs << "#include \"" << include << "\"" << Qt::endl;
        if (headerName.isEmpty())
            cs << "#include \"" << include << "\"" << Qt::endl;
    }

    if (cppName != headerName) {
        if (!headerName.isEmpty() && headerName != QLatin1String("-"))
            cs << "#include \"" << headerName << "\"" << Qt::endl;

        cs << "#include <QtCore/QMetaObject>" << Qt::endl
           << includeList
           << Qt::endl;
        hs << forwardDeclarations;
    } else {
        hs << includeList;
    }

    hs << Qt::endl;

    QString parent = parentClassName;
    if (parentClassName.isEmpty())
        parent = QLatin1String("QObject");

    for (const QDBusIntrospection::Interface *interface : interfaces) {
        QString className = classNameForInterface(interface->name, Adaptor);

        // comment:
        hs << "/*" << Qt::endl
           << " * Adaptor class for interface " << interface->name << Qt::endl
           << " */" << Qt::endl;
        cs << "/*" << Qt::endl
           << " * Implementation of adaptor class " << className << Qt::endl
           << " */" << Qt::endl
           << Qt::endl;

        // class header:
        hs << "class " << className << ": public QDBusAbstractAdaptor" << Qt::endl
           << "{" << Qt::endl
           << "    Q_OBJECT" << Qt::endl
           << "    Q_CLASSINFO(\"D-Bus Interface\", \"" << interface->name << "\")" << Qt::endl
           << "    Q_CLASSINFO(\"D-Bus Introspection\", \"\"" << Qt::endl
           << stringify(interface->introspection)
           << "        \"\")" << Qt::endl
           << "public:" << Qt::endl
           << "    " << className << "(" << parent << " *parent);" << Qt::endl
           << "    virtual ~" << className << "();" << Qt::endl
           << Qt::endl;

        if (!parentClassName.isEmpty())
            hs << "    inline " << parent << " *parent() const" << Qt::endl
               << "    { return static_cast<" << parent << " *>(QObject::parent()); }" << Qt::endl
               << Qt::endl;

        // constructor/destructor
        cs << className << "::" << className << "(" << parent << " *parent)" << Qt::endl
           << "    : QDBusAbstractAdaptor(parent)" << Qt::endl
           << "{" << Qt::endl
           << "    // constructor" << Qt::endl
           << "    setAutoRelaySignals(true);" << Qt::endl
           << "}" << Qt::endl
           << Qt::endl
           << className << "::~" << className << "()" << Qt::endl
           << "{" << Qt::endl
           << "    // destructor" << Qt::endl
           << "}" << Qt::endl
           << Qt::endl;

        hs << "public: // PROPERTIES" << Qt::endl;
        for (const QDBusIntrospection::Property &property : interface->properties) {
            QByteArray type = qtTypeName(property.type, property.annotations);
            QString constRefType = constRefArg(type);
            QString getter = propertyGetter(property);
            QString setter = propertySetter(property);

            hs << "    Q_PROPERTY(" << type << " " << property.name;
            if (property.access != QDBusIntrospection::Property::Write)
                hs << " READ " << getter;
            if (property.access != QDBusIntrospection::Property::Read)
                hs << " WRITE " << setter;
            hs << ")" << Qt::endl;

            // getter:
            if (property.access != QDBusIntrospection::Property::Write) {
                hs << "    " << type << " " << getter << "() const;" << Qt::endl;
                cs << type << " "
                   << className << "::" << getter << "() const" << Qt::endl
                   << "{" << Qt::endl
                   << "    // get the value of property " << property.name << Qt::endl
                   << "    return qvariant_cast< " << type <<" >(parent()->property(\"" << property.name << "\"));" << Qt::endl
                   << "}" << Qt::endl
                   << Qt::endl;
            }

            // setter
            if (property.access != QDBusIntrospection::Property::Read) {
                hs << "    void " << setter << "(" << constRefType << "value);" << Qt::endl;
                cs << "void " << className << "::" << setter << "(" << constRefType << "value)" << Qt::endl
                   << "{" << Qt::endl
                   << "    // set the value of property " << property.name << Qt::endl
                   << "    parent()->setProperty(\"" << property.name << "\", QVariant::fromValue(value";
                if (constRefType.contains(QLatin1String("QDBusVariant")))
                    cs << ".variant()";
                cs << "));" << Qt::endl
                   << "}" << Qt::endl
                   << Qt::endl;
            }

            hs << Qt::endl;
        }

        hs << "public Q_SLOTS: // METHODS" << Qt::endl;
        for (const QDBusIntrospection::Method &method : interface->methods) {
            bool isNoReply =
                method.annotations.value(QLatin1String(ANNOTATION_NO_WAIT)) == QLatin1String("true");
            if (isNoReply && !method.outputArgs.isEmpty()) {
                fprintf(stderr, "%s: warning while processing '%s': method %s in interface %s is marked 'no-reply' but has output arguments.\n",
                        PROGRAMNAME, qPrintable(inputFile), qPrintable(method.name), qPrintable(interface->name));
                continue;
            }

            hs << "    ";
            if (method.annotations.value(QLatin1String("org.freedesktop.DBus.Deprecated")) ==
                QLatin1String("true"))
                hs << "Q_DECL_DEPRECATED ";

            QByteArray returnType;
            if (isNoReply) {
                hs << "Q_NOREPLY void ";
                cs << "void ";
            } else if (method.outputArgs.isEmpty()) {
                hs << "void ";
                cs << "void ";
            } else {
                returnType = qtTypeName(method.outputArgs.first().type, method.annotations, 0, "Out");
                hs << returnType << " ";
                cs << returnType << " ";
            }

            QString name = methodName(method);
            hs << name << "(";
            cs << className << "::" << name << "(";

            QStringList argNames = makeArgNames(method.inputArgs, method.outputArgs);
            writeArgList(hs, argNames, method.annotations, method.inputArgs, method.outputArgs);
            writeArgList(cs, argNames, method.annotations, method.inputArgs, method.outputArgs);

            hs << ");" << Qt::endl; // finished for header
            cs << ")" << Qt::endl
               << "{" << Qt::endl
               << "    // handle method call " << interface->name << "." << methodName(method) << Qt::endl;

            // make the call
            bool usingInvokeMethod = false;
            if (parentClassName.isEmpty() && method.inputArgs.count() <= 10
                && method.outputArgs.count() <= 1)
                usingInvokeMethod = true;

            if (usingInvokeMethod) {
                // we are using QMetaObject::invokeMethod
                if (!returnType.isEmpty())
                    cs << "    " << returnType << " " << argNames.at(method.inputArgs.count())
                       << ";" << Qt::endl;

                static const char invoke[] = "    QMetaObject::invokeMethod(parent(), \"";
                cs << invoke << name << "\"";

                if (!method.outputArgs.isEmpty())
                    cs << ", Q_RETURN_ARG("
                       << qtTypeName(method.outputArgs.at(0).type, method.annotations,
                                     0, "Out")
                       << ", "
                       << argNames.at(method.inputArgs.count())
                       << ")";

                for (int i = 0; i < method.inputArgs.count(); ++i)
                    cs << ", Q_ARG("
                       << qtTypeName(method.inputArgs.at(i).type, method.annotations,
                                     i, "In")
                       << ", "
                       << argNames.at(i)
                       << ")";

                cs << ");" << Qt::endl;

                if (!returnType.isEmpty())
                    cs << "    return " << argNames.at(method.inputArgs.count()) << ";" << Qt::endl;
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

                int argPos = 0;
                bool first = true;
                for (int i = 0; i < method.inputArgs.count(); ++i) {
                    cs << (first ? "" : ", ") << argNames.at(argPos++);
                    first = false;
                }
                ++argPos;           // skip retval, if any
                for (int i = 1; i < method.outputArgs.count(); ++i) {
                    cs << (first ? "" : ", ") << argNames.at(argPos++);
                    first = false;
                }

                cs << ");" << Qt::endl;
            }
            cs << "}" << Qt::endl
               << Qt::endl;
        }

        hs << "Q_SIGNALS: // SIGNALS" << Qt::endl;
        for (const QDBusIntrospection::Signal &signal : interface->signals_) {
            hs << "    ";
            if (signal.annotations.value(QLatin1String("org.freedesktop.DBus.Deprecated")) ==
                QLatin1String("true"))
                hs << "Q_DECL_DEPRECATED ";

            hs << "void " << signal.name << "(";

            QStringList argNames = makeArgNames(signal.outputArgs);
            writeSignalArgList(hs, argNames, signal.annotations, signal.outputArgs);

            hs << ");" << Qt::endl; // finished for header
        }

        // close the class:
        hs << "};" << Qt::endl
           << Qt::endl;
    }

    // close the include guard
    hs << "#endif" << Qt::endl;

    QString mocName = moc(filename);
    if (includeMocs && !mocName.isEmpty())
        cs << Qt::endl
           << "#include \"" << mocName << "\"" << Qt::endl;

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
    parser.setApplicationDescription(QLatin1String(
            "Produces the C++ code to implement the interfaces defined in the input file.\n\n"
            "If the file name given to the options -a and -p does not end in .cpp or .h, the\n"
            "program will automatically append the suffixes and produce both files.\n"
            "You can also use a colon (:) to separate the header name from the source file\n"
            "name, as in '-a filename_p.h:filename.cpp'.\n\n"
            "If you pass a dash (-) as the argument to either -p or -a, the output is written\n"
            "to the standard output."));

    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("xml-or-xml-file"), QStringLiteral("XML file to use."));
    parser.addPositionalArgument(QStringLiteral("interfaces"), QStringLiteral("List of interfaces to use."),
                QStringLiteral("[interfaces ...]"));

    QCommandLineOption adapterCodeOption(QStringList() << QStringLiteral("a") << QStringLiteral("adaptor"),
                QStringLiteral("Write the adaptor code to <filename>"), QStringLiteral("filename"));
    parser.addOption(adapterCodeOption);

    QCommandLineOption classNameOption(QStringList() << QStringLiteral("c") << QStringLiteral("classname"),
                QStringLiteral("Use <classname> as the class name for the generated classes"), QStringLiteral("classname"));
    parser.addOption(classNameOption);

    QCommandLineOption addIncludeOption(QStringList() << QStringLiteral("i") << QStringLiteral("include"),
                QStringLiteral("Add #include to the output"), QStringLiteral("filename"));
    parser.addOption(addIncludeOption);

    QCommandLineOption adapterParentOption(QStringLiteral("l"),
                QStringLiteral("When generating an adaptor, use <classname> as the parent class"), QStringLiteral("classname"));
    parser.addOption(adapterParentOption);

    QCommandLineOption mocIncludeOption(QStringList() << QStringLiteral("m") << QStringLiteral("moc"),
                QStringLiteral("Generate #include \"filename.moc\" statements in the .cpp files"));
    parser.addOption(mocIncludeOption);

    QCommandLineOption noNamespaceOption(QStringList() << QStringLiteral("N") << QStringLiteral("no-namespaces"),
                QStringLiteral("Don't use namespaces"));
    parser.addOption(noNamespaceOption);

    QCommandLineOption proxyCodeOption(QStringList() << QStringLiteral("p") << QStringLiteral("proxy"),
                QStringLiteral("Write the proxy code to <filename>"), QStringLiteral("filename"));
    parser.addOption(proxyCodeOption);

    QCommandLineOption verboseOption(QStringList() << QStringLiteral("V") << QStringLiteral("verbose"),
                QStringLiteral("Be verbose."));
    parser.addOption(verboseOption);

    parser.process(app);

    adaptorFile = parser.value(adapterCodeOption);
    globalClassName = parser.value(classNameOption);
    includes = parser.values(addIncludeOption);
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
        QLoggingCategory::setFilterRules(QStringLiteral("dbus.parser.debug=true"));

    QDBusIntrospection::Interfaces interfaces = readInput();
    cleanInterfaces(interfaces);

    QStringList args = app.arguments();
    args.removeFirst();
    commandLine = QLatin1String(PROGRAMNAME " ");
    commandLine += args.join(QLatin1Char(' '));

    if (!proxyFile.isEmpty() || adaptorFile.isEmpty())
        writeProxy(proxyFile, interfaces);

    if (!adaptorFile.isEmpty())
        writeAdaptor(adaptorFile, interfaces);

    return 0;
}

