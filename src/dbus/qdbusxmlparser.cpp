// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdbusxmlparser_p.h"
#include "qdbusutil_p.h"

#include <QtCore/qmap.h>
#include <QtCore/qvariant.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qdebug.h>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(dbusParser, "dbus.parser", QtWarningMsg)

#define qDBusParserError(...) qCDebug(dbusParser, ##__VA_ARGS__)

bool QDBusXmlParser::parseArg(const QXmlStreamAttributes &attributes,
                              QDBusIntrospection::Argument &argData)
{
    Q_ASSERT(m_currentInterface);

    const QString argType = attributes.value("type"_L1).toString();

    bool ok = QDBusUtil::isValidSingleSignature(argType);
    if (!ok) {
        qDBusParserError("Invalid D-BUS type signature '%s' found while parsing introspection",
                qPrintable(argType));
    }

    argData.name = attributes.value("name"_L1).toString();
    argData.type = argType;

    m_currentInterface->introspection += "      <arg"_L1;
    if (attributes.hasAttribute("direction"_L1)) {
        const QString direction = attributes.value("direction"_L1).toString();
        m_currentInterface->introspection += " direction=\""_L1 + direction + u'"';
    }
    m_currentInterface->introspection += " type=\""_L1 + argData.type + u'"';
    if (!argData.name.isEmpty())
        m_currentInterface->introspection += " name=\""_L1 + argData.name + u'"';
    m_currentInterface->introspection += "/>\n"_L1;

    return ok;
}

bool QDBusXmlParser::parseAnnotation(QDBusIntrospection::Annotations &annotations,
                                     bool interfaceAnnotation)
{
    Q_ASSERT(m_currentInterface);
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "annotation"_L1);

    const QXmlStreamAttributes attributes = m_xml.attributes();
    const QString name = attributes.value("name"_L1).toString();

    if (!QDBusUtil::isValidInterfaceName(name)) {
        qDBusParserError("Invalid D-BUS annotation '%s' found while parsing introspection",
                qPrintable(name));
        return false;
    }
    const QString value = attributes.value("value"_L1).toString();
    annotations.insert(name, value);
    if (!interfaceAnnotation)
        m_currentInterface->introspection += "  "_L1;
    m_currentInterface->introspection += "    <annotation value=\""_L1 + value.toHtmlEscaped() + "\" name=\""_L1 + name + "\"/>\n"_L1;
    return true;
}

bool QDBusXmlParser::parseProperty(QDBusIntrospection::Property &propertyData)
{
    Q_ASSERT(m_currentInterface);
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "property"_L1);

    QXmlStreamAttributes attributes = m_xml.attributes();
    const QString propertyName = attributes.value("name"_L1).toString();
    if (!QDBusUtil::isValidMemberName(propertyName)) {
        qDBusParserError("Invalid D-BUS member name '%s' found in interface '%s' while parsing introspection",
                qPrintable(propertyName), qPrintable(m_currentInterface->name));
        m_xml.skipCurrentElement();
        return false;
    }

    // parse data
    propertyData.name = propertyName;
    propertyData.type = attributes.value("type"_L1).toString();

    if (!QDBusUtil::isValidSingleSignature(propertyData.type)) {
        // cannot be!
        qDBusParserError("Invalid D-BUS type signature '%s' found in property '%s.%s' while parsing introspection",
                qPrintable(propertyData.type), qPrintable(m_currentInterface->name),
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
                qPrintable(access), qPrintable(m_currentInterface->name),
                qPrintable(propertyName));
        return false;       // invalid one!
    }

    m_currentInterface->introspection += "    <property access=\""_L1 + access + "\" type=\""_L1 + propertyData.type + "\" name=\""_L1 + propertyName + u'"';

    if (!m_xml.readNextStartElement()) {
        m_currentInterface->introspection += "/>\n"_L1;
    } else {
        m_currentInterface->introspection += ">\n"_L1;

        do {
            if (m_xml.name() == "annotation"_L1) {
                parseAnnotation(propertyData.annotations);
            } else if (m_xml.prefix().isEmpty()) {
                qDBusParserError() << "Unknown element" << m_xml.name() << "while checking for annotations";
            }
            m_xml.skipCurrentElement();
        } while (m_xml.readNextStartElement());

        m_currentInterface->introspection += "    </property>\n"_L1;
    }

    if (!m_xml.isEndElement() || m_xml.name() != "property"_L1) {
        qDBusParserError() << "Invalid property specification" << m_xml.tokenString() << m_xml.name();
        return false;
    }

    return true;
}

bool QDBusXmlParser::parseMethod(QDBusIntrospection::Method &methodData)
{
    Q_ASSERT(m_currentInterface);
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "method"_L1);

    const QXmlStreamAttributes attributes = m_xml.attributes();
    const QString methodName = attributes.value("name"_L1).toString();
    if (!QDBusUtil::isValidMemberName(methodName)) {
        qDBusParserError("Invalid D-BUS member name '%s' found in interface '%s' while parsing introspection",
                qPrintable(methodName), qPrintable(m_currentInterface->name));
        return false;
    }

    methodData.name = methodName;
    m_currentInterface->introspection += "    <method name=\""_L1 + methodName + u'"';

    QDBusIntrospection::Arguments outArguments;
    QDBusIntrospection::Arguments inArguments;
    QDBusIntrospection::Annotations annotations;

    if (!m_xml.readNextStartElement()) {
        m_currentInterface->introspection += "/>\n"_L1;
    } else {
        m_currentInterface->introspection += ">\n"_L1;

        do {
            if (m_xml.name() == "annotation"_L1) {
                parseAnnotation(annotations);
            } else if (m_xml.name() == "arg"_L1) {
                const QXmlStreamAttributes attributes = m_xml.attributes();
                const QString direction = attributes.value("direction"_L1).toString();
                QDBusIntrospection::Argument argument;
                if (!attributes.hasAttribute("direction"_L1) || direction == "in"_L1) {
                    parseArg(attributes, argument);
                    inArguments << argument;
                } else if (direction == "out"_L1) {
                    parseArg(attributes, argument);
                    outArguments << argument;
                }
            } else if (m_xml.prefix().isEmpty()) {
                qDBusParserError() << "Unknown element" << m_xml.name() << "while checking for method arguments";
            }
            m_xml.skipCurrentElement();
        } while (m_xml.readNextStartElement());

        m_currentInterface->introspection += "    </method>\n"_L1;
    }

    methodData.inputArgs = inArguments;
    methodData.outputArgs = outArguments;
    methodData.annotations = annotations;

    return true;
}

bool QDBusXmlParser::parseSignal(QDBusIntrospection::Signal &signalData)
{
    Q_ASSERT(m_currentInterface);
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "signal"_L1);

    const QXmlStreamAttributes attributes = m_xml.attributes();
    const QString signalName = attributes.value("name"_L1).toString();

    if (!QDBusUtil::isValidMemberName(signalName)) {
        qDBusParserError("Invalid D-BUS member name '%s' found in interface '%s' while parsing introspection",
                qPrintable(signalName), qPrintable(m_currentInterface->name));
        return false;
    }

    signalData.name = signalName;
    m_currentInterface->introspection += "    <signal name=\""_L1 + signalName + u'"';

    QDBusIntrospection::Arguments arguments;
    QDBusIntrospection::Annotations annotations;

    if (!m_xml.readNextStartElement()) {
        m_currentInterface->introspection += "/>\n"_L1;
    } else {
        m_currentInterface->introspection += ">\n"_L1;

        do {
            if (m_xml.name() == "annotation"_L1) {
                parseAnnotation(annotations);
            } else if (m_xml.name() == "arg"_L1) {
                const QXmlStreamAttributes attributes = m_xml.attributes();
                QDBusIntrospection::Argument argument;
                if (!attributes.hasAttribute("direction"_L1) ||
                    attributes.value("direction"_L1) == "out"_L1) {
                    parseArg(attributes, argument);
                    arguments << argument;
                }
            } else {
                qDBusParserError() << "Unknown element" << m_xml.name() << "while checking for signal arguments";
            }
            m_xml.skipCurrentElement();
        } while (m_xml.readNextStartElement());

        m_currentInterface->introspection += "    </signal>\n"_L1;
    }

    signalData.outputArgs = arguments;
    signalData.annotations = annotations;

    return true;
}

void QDBusXmlParser::readInterface()
{
    Q_ASSERT(!m_currentInterface);

    const QString ifaceName = m_xml.attributes().value("name"_L1).toString();
    if (!QDBusUtil::isValidInterfaceName(ifaceName)) {
        qDBusParserError("Invalid D-BUS interface name '%s' found while parsing introspection",
                qPrintable(ifaceName));
        return;
    }

    m_object->interfaces.append(ifaceName);

    m_currentInterface = std::make_unique<QDBusIntrospection::Interface>();
    m_currentInterface->name = ifaceName;
    m_currentInterface->introspection += "  <interface name=\""_L1 + ifaceName + "\">\n"_L1;

    while (m_xml.readNextStartElement()) {
        if (m_xml.name() == "method"_L1) {
            QDBusIntrospection::Method methodData;
            if (parseMethod(methodData))
                m_currentInterface->methods.insert(methodData.name, methodData);
        } else if (m_xml.name() == "signal"_L1) {
            QDBusIntrospection::Signal signalData;
            if (parseSignal(signalData))
                m_currentInterface->signals_.insert(signalData.name, signalData);
        } else if (m_xml.name() == "property"_L1) {
            QDBusIntrospection::Property propertyData;
            if (parseProperty(propertyData))
                m_currentInterface->properties.insert(propertyData.name, propertyData);
        } else if (m_xml.name() == "annotation"_L1) {
            parseAnnotation(m_currentInterface->annotations, true);
            m_xml.skipCurrentElement(); // skip over annotation object
        } else {
            if (m_xml.prefix().isEmpty()) {
                qDBusParserError() << "Unknown element while parsing interface" << m_xml.name();
            }
            m_xml.skipCurrentElement();
        }
    }

    m_currentInterface->introspection += "  </interface>"_L1;

    m_interfaces.insert(
            ifaceName,
            QSharedDataPointer<QDBusIntrospection::Interface>(m_currentInterface.release()));

    if (!m_xml.isEndElement() || m_xml.name() != "interface"_L1) {
        qDBusParserError() << "Invalid Interface specification";
    }
}

void QDBusXmlParser::readNode(int nodeLevel)
{
    const QString objName = m_xml.attributes().value("name"_L1).toString();
    const QString fullName = m_object->path.endsWith(u'/')
                                ? (m_object->path + objName)
                                : QString(m_object->path + u'/' + objName);
    if (!QDBusUtil::isValidObjectPath(fullName)) {
        qDBusParserError("Invalid D-BUS object path '%s' found while parsing introspection",
                 qPrintable(fullName));
        return;
    }

    if (nodeLevel > 0)
        m_object->childObjects.append(objName);
}

QDBusXmlParser::QDBusXmlParser(const QString &service, const QString &path, const QString &xmlData)
    : m_service(service), m_path(path), m_object(new QDBusIntrospection::Object), m_xml(xmlData)
{
    m_object->service = m_service;
    m_object->path = m_path;

    int nodeLevel = -1;

    while (!m_xml.atEnd()) {
        m_xml.readNext();

        switch (m_xml.tokenType()) {
        case QXmlStreamReader::StartElement:
            if (m_xml.name() == "node"_L1) {
                readNode(++nodeLevel);
            } else if (m_xml.name() == "interface"_L1) {
                readInterface();
            } else {
                if (m_xml.prefix().isEmpty()) {
                    qDBusParserError() << "skipping unknown element" << m_xml.name();
                }
                m_xml.skipCurrentElement();
            }
            break;
        case QXmlStreamReader::EndElement:
            if (m_xml.name() == "node"_L1) {
                --nodeLevel;
            } else {
                qDBusParserError() << "Invalid Node declaration" << m_xml.name();
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
            if (m_xml.isWhitespace())
                break;
            Q_FALLTHROUGH();
        default:
            qDBusParserError() << "unknown token" << m_xml.name() << m_xml.tokenString();
            break;
        }
    }

    if (m_xml.hasError()) {
        qDBusParserError() << "xml error" << m_xml.errorString() << "doc" << xmlData;
    }
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
