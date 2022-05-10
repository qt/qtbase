// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdbusxmlparser_p.h"
#include "qdbusutil_p.h"

#include <QtCore/qmap.h>
#include <QtCore/qvariant.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qxmlstream.h>
#include <QtCore/qdebug.h>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(dbusParser, "dbus.parser", QtWarningMsg)

#define qDBusParserError(...) qCDebug(dbusParser, ##__VA_ARGS__)

static bool parseArg(const QXmlStreamAttributes &attributes, QDBusIntrospection::Argument &argData,
        QDBusIntrospection::Interface *ifaceData)
{
    const QString argType = attributes.value("type"_L1).toString();

    bool ok = QDBusUtil::isValidSingleSignature(argType);
    if (!ok) {
        qDBusParserError("Invalid D-BUS type signature '%s' found while parsing introspection",
                qPrintable(argType));
    }

    argData.name = attributes.value("name"_L1).toString();
    argData.type = argType;

    ifaceData->introspection += "      <arg"_L1;
    if (attributes.hasAttribute("direction"_L1)) {
        const QString direction = attributes.value("direction"_L1).toString();
        ifaceData->introspection += " direction=\""_L1 + direction + u'"';
    }
    ifaceData->introspection += " type=\""_L1 + argData.type + u'"';
    if (!argData.name.isEmpty())
        ifaceData->introspection += " name=\""_L1 + argData.name + u'"';
    ifaceData->introspection += "/>\n"_L1;

    return ok;
}

static bool parseAnnotation(const QXmlStreamReader &xml, QDBusIntrospection::Annotations &annotations,
        QDBusIntrospection::Interface *ifaceData, bool interfaceAnnotation = false)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "annotation"_L1);

    const QXmlStreamAttributes attributes = xml.attributes();
    const QString name = attributes.value("name"_L1).toString();

    if (!QDBusUtil::isValidInterfaceName(name)) {
        qDBusParserError("Invalid D-BUS annotation '%s' found while parsing introspection",
                qPrintable(name));
        return false;
    }
    const QString value = attributes.value("value"_L1).toString();
    annotations.insert(name, value);
    if (!interfaceAnnotation)
        ifaceData->introspection += "  "_L1;
    ifaceData->introspection += "    <annotation value=\""_L1 + value.toHtmlEscaped() + "\" name=\""_L1 + name + "\"/>\n"_L1;
    return true;
}

static bool parseProperty(QXmlStreamReader &xml, QDBusIntrospection::Property &propertyData,
                QDBusIntrospection::Interface *ifaceData)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "property"_L1);

    QXmlStreamAttributes attributes = xml.attributes();
    const QString propertyName = attributes.value("name"_L1).toString();
    if (!QDBusUtil::isValidMemberName(propertyName)) {
        qDBusParserError("Invalid D-BUS member name '%s' found in interface '%s' while parsing introspection",
                qPrintable(propertyName), qPrintable(ifaceData->name));
        xml.skipCurrentElement();
        return false;
    }

    // parse data
    propertyData.name = propertyName;
    propertyData.type = attributes.value("type"_L1).toString();

    if (!QDBusUtil::isValidSingleSignature(propertyData.type)) {
        // cannot be!
        qDBusParserError("Invalid D-BUS type signature '%s' found in property '%s.%s' while parsing introspection",
                qPrintable(propertyData.type), qPrintable(ifaceData->name),
                qPrintable(propertyName));
    }

    const QString access = attributes.value("access"_L1).toString();
    if (access == "read"_L1)
        propertyData.access = QDBusIntrospection::Property::Read;
    else if (access == "write"_L1)
        propertyData.access = QDBusIntrospection::Property::Write;
    else if (access == "readwrite"_L1)
        propertyData.access = QDBusIntrospection::Property::ReadWrite;
    else {
        qDBusParserError("Invalid D-BUS property access '%s' found in property '%s.%s' while parsing introspection",
                qPrintable(access), qPrintable(ifaceData->name),
                qPrintable(propertyName));
        return false;       // invalid one!
    }

    ifaceData->introspection += "    <property access=\""_L1 + access + "\" type=\""_L1 + propertyData.type + "\" name=\""_L1 + propertyName + u'"';

    if (!xml.readNextStartElement()) {
        ifaceData->introspection += "/>\n"_L1;
    } else {
        ifaceData->introspection += ">\n"_L1;

        do {
            if (xml.name() == "annotation"_L1) {
                parseAnnotation(xml, propertyData.annotations, ifaceData);
            } else if (xml.prefix().isEmpty()) {
                qDBusParserError() << "Unknown element" << xml.name() << "while checking for annotations";
            }
            xml.skipCurrentElement();
        } while (xml.readNextStartElement());

        ifaceData->introspection += "    </property>\n"_L1;
    }

    if (!xml.isEndElement() || xml.name() != "property"_L1) {
        qDBusParserError() << "Invalid property specification" << xml.tokenString() << xml.name();
        return false;
    }

    return true;
}

static bool parseMethod(QXmlStreamReader &xml, QDBusIntrospection::Method &methodData,
        QDBusIntrospection::Interface *ifaceData)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "method"_L1);

    const QXmlStreamAttributes attributes = xml.attributes();
    const QString methodName = attributes.value("name"_L1).toString();
    if (!QDBusUtil::isValidMemberName(methodName)) {
        qDBusParserError("Invalid D-BUS member name '%s' found in interface '%s' while parsing introspection",
                qPrintable(methodName), qPrintable(ifaceData->name));
        return false;
    }

    methodData.name = methodName;
    ifaceData->introspection += "    <method name=\""_L1 + methodName + u'"';

    QDBusIntrospection::Arguments outArguments;
    QDBusIntrospection::Arguments inArguments;
    QDBusIntrospection::Annotations annotations;

    if (!xml.readNextStartElement()) {
        ifaceData->introspection += "/>\n"_L1;
    } else {
        ifaceData->introspection += ">\n"_L1;

        do {
            if (xml.name() == "annotation"_L1) {
                parseAnnotation(xml, annotations, ifaceData);
            } else if (xml.name() == "arg"_L1) {
                const QXmlStreamAttributes attributes = xml.attributes();
                const QString direction = attributes.value("direction"_L1).toString();
                QDBusIntrospection::Argument argument;
                if (!attributes.hasAttribute("direction"_L1) || direction == "in"_L1) {
                    parseArg(attributes, argument, ifaceData);
                    inArguments << argument;
                } else if (direction == "out"_L1) {
                    parseArg(attributes, argument, ifaceData);
                    outArguments << argument;
                }
            } else if (xml.prefix().isEmpty()) {
                qDBusParserError() << "Unknown element" << xml.name() << "while checking for method arguments";
            }
            xml.skipCurrentElement();
        } while (xml.readNextStartElement());

        ifaceData->introspection += "    </method>\n"_L1;
    }

    methodData.inputArgs = inArguments;
    methodData.outputArgs = outArguments;
    methodData.annotations = annotations;

    return true;
}


static bool parseSignal(QXmlStreamReader &xml, QDBusIntrospection::Signal &signalData,
        QDBusIntrospection::Interface *ifaceData)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "signal"_L1);

    const QXmlStreamAttributes attributes = xml.attributes();
    const QString signalName = attributes.value("name"_L1).toString();

    if (!QDBusUtil::isValidMemberName(signalName)) {
        qDBusParserError("Invalid D-BUS member name '%s' found in interface '%s' while parsing introspection",
                qPrintable(signalName), qPrintable(ifaceData->name));
        return false;
    }

    signalData.name = signalName;
    ifaceData->introspection += "    <signal name=\""_L1 + signalName + u'"';

    QDBusIntrospection::Arguments arguments;
    QDBusIntrospection::Annotations annotations;

    if (!xml.readNextStartElement()) {
        ifaceData->introspection += "/>\n"_L1;
    } else {
        ifaceData->introspection += ">\n"_L1;

        do {
            if (xml.name() == "annotation"_L1) {
                parseAnnotation(xml, annotations, ifaceData);
            } else if (xml.name() == "arg"_L1) {
                const QXmlStreamAttributes attributes = xml.attributes();
                QDBusIntrospection::Argument argument;
                if (!attributes.hasAttribute("direction"_L1) ||
                    attributes.value("direction"_L1) == "out"_L1) {
                    parseArg(attributes, argument, ifaceData);
                    arguments << argument;
                }
            } else {
                qDBusParserError() << "Unknown element" << xml.name() << "while checking for signal arguments";
            }
            xml.skipCurrentElement();
        } while (xml.readNextStartElement());

        ifaceData->introspection += "    </signal>\n"_L1;
    }

    signalData.outputArgs = arguments;
    signalData.annotations = annotations;

    return true;
}

static void readInterface(QXmlStreamReader &xml, QDBusIntrospection::Object *objData,
        QDBusIntrospection::Interfaces *interfaces)
{
    const QString ifaceName = xml.attributes().value("name"_L1).toString();
    if (!QDBusUtil::isValidInterfaceName(ifaceName)) {
        qDBusParserError("Invalid D-BUS interface name '%s' found while parsing introspection",
                qPrintable(ifaceName));
        return;
    }

    objData->interfaces.append(ifaceName);

    QDBusIntrospection::Interface *ifaceData = new QDBusIntrospection::Interface;
    ifaceData->name = ifaceName;
    ifaceData->introspection += "  <interface name=\""_L1 + ifaceName + "\">\n"_L1;

    while (xml.readNextStartElement()) {
        if (xml.name() == "method"_L1) {
            QDBusIntrospection::Method methodData;
            if (parseMethod(xml, methodData, ifaceData))
                ifaceData->methods.insert(methodData.name, methodData);
        } else if (xml.name() == "signal"_L1) {
            QDBusIntrospection::Signal signalData;
            if (parseSignal(xml, signalData, ifaceData))
                ifaceData->signals_.insert(signalData.name, signalData);
        } else if (xml.name() == "property"_L1) {
            QDBusIntrospection::Property propertyData;
            if (parseProperty(xml, propertyData, ifaceData))
                ifaceData->properties.insert(propertyData.name, propertyData);
        } else if (xml.name() == "annotation"_L1) {
            parseAnnotation(xml, ifaceData->annotations, ifaceData, true);
            xml.skipCurrentElement(); // skip over annotation object
        } else {
            if (xml.prefix().isEmpty()) {
                qDBusParserError() << "Unknown element while parsing interface" << xml.name();
            }
            xml.skipCurrentElement();
        }
    }

    ifaceData->introspection += "  </interface>"_L1;

    interfaces->insert(ifaceName, QSharedDataPointer<QDBusIntrospection::Interface>(ifaceData));

    if (!xml.isEndElement() || xml.name() != "interface"_L1) {
        qDBusParserError() << "Invalid Interface specification";
    }
}

static void readNode(const QXmlStreamReader &xml, QDBusIntrospection::Object *objData, int nodeLevel)
{
    const QString objName = xml.attributes().value("name"_L1).toString();
    const QString fullName = objData->path.endsWith(u'/')
                                ? (objData->path + objName)
                                : QString(objData->path + u'/' + objName);
    if (!QDBusUtil::isValidObjectPath(fullName)) {
        qDBusParserError("Invalid D-BUS object path '%s' found while parsing introspection",
                 qPrintable(fullName));
        return;
    }

    if (nodeLevel > 0)
        objData->childObjects.append(objName);
}

QDBusXmlParser::QDBusXmlParser(const QString& service, const QString& path,
                               const QString& xmlData)
    : m_service(service), m_path(path), m_object(new QDBusIntrospection::Object)
{
//    qDBusParserError() << "parsing" << xmlData;

    m_object->service = m_service;
    m_object->path = m_path;

    QXmlStreamReader xml(xmlData);

    int nodeLevel = -1;

    while (!xml.atEnd()) {
        xml.readNext();

        switch (xml.tokenType()) {
        case QXmlStreamReader::StartElement:
            if (xml.name() == "node"_L1) {
                readNode(xml, m_object, ++nodeLevel);
            } else if (xml.name() == "interface"_L1) {
                readInterface(xml, m_object, &m_interfaces);
            } else {
                if (xml.prefix().isEmpty()) {
                    qDBusParserError() << "skipping unknown element" << xml.name();
                }
                xml.skipCurrentElement();
            }
            break;
        case QXmlStreamReader::EndElement:
            if (xml.name() == "node"_L1) {
                --nodeLevel;
            } else {
                qDBusParserError() << "Invalid Node declaration" << xml.name();
            }
            break;
        case QXmlStreamReader::StartDocument:
        case QXmlStreamReader::EndDocument:
        case QXmlStreamReader::DTD:
            // not interested
            break;
        case QXmlStreamReader::Comment:
            // ignore comments and processing instructions
            break;
        case QXmlStreamReader::Characters:
            // ignore whitespace
            if (xml.isWhitespace())
                break;
            Q_FALLTHROUGH();
        default:
            qDBusParserError() << "unknown token" << xml.name() << xml.tokenString();
            break;
        }
    }

    if (xml.hasError()) {
        qDBusParserError() << "xml error" << xml.errorString() << "doc" << xmlData;
    }
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
