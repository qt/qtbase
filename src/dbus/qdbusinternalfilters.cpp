// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qdbusconnection_p.h"

#include "qdbus_symbols_p.h"
#include <QtCore/qcoreapplication.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qthread.h>

#include "qdbusabstractadaptor.h"
#include "qdbusabstractadaptor_p.h"
#include "qdbusconnection.h"
#include "qdbusextratypes.h"
#include "qdbusmessage.h"
#include "qdbusmetatype.h"
#include "qdbusmetatype_p.h"
#include "qdbusmessage_p.h"
#include "qdbusutil_p.h"
#include "qdbusvirtualobject.h"

#include <algorithm>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// defined in qdbusxmlgenerator.cpp
extern Q_DBUS_EXPORT QString qDBusGenerateMetaObjectXml(QString interface, const QMetaObject *mo,
                                                        const QMetaObject *base, int flags);

static const char introspectableInterfaceXml[] =
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "    <method name=\"Introspect\">\n"
    "      <arg name=\"xml_data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "  </interface>\n";

static const char propertiesInterfaceXml[] =
    "  <interface name=\"org.freedesktop.DBus.Properties\">\n"
    "    <method name=\"Get\">\n"
    "      <arg name=\"interface_name\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"property_name\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"value\" type=\"v\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"Set\">\n"
    "      <arg name=\"interface_name\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"property_name\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"value\" type=\"v\" direction=\"in\"/>\n"
    "    </method>\n"
    "    <method name=\"GetAll\">\n"
    "      <arg name=\"interface_name\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"values\" type=\"a{sv}\" direction=\"out\"/>\n"
    "      <annotation name=\"org.qtproject.QtDBus.QtTypeName.Out0\" value=\"QVariantMap\"/>\n"
    "    </method>\n"
    "    <signal name=\"PropertiesChanged\">\n"
    "      <arg name=\"interface_name\" type=\"s\" direction=\"out\"/>\n"
    "      <arg name=\"changed_properties\" type=\"a{sv}\" direction=\"out\"/>\n"
    "      <annotation name=\"org.qtproject.QtDBus.QtTypeName.Out1\" value=\"QVariantMap\"/>\n"
    "      <arg name=\"invalidated_properties\" type=\"as\" direction=\"out\"/>\n"
    "    </signal>\n"
    "  </interface>\n";

static const char peerInterfaceXml[] =
    "  <interface name=\"org.freedesktop.DBus.Peer\">\n"
    "    <method name=\"Ping\"/>\n"
    "    <method name=\"GetMachineId\">\n"
    "      <arg name=\"machine_uuid\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "  </interface>\n";

static QString generateSubObjectXml(const QObject *object)
{
    QString retval;
    for (const QObject *child : object->children()) {
        QString name = child->objectName();
        if (!name.isEmpty() && QDBusUtil::isValidPartOfObjectPath(name))
            retval += "  <node name=\""_L1 + name + "\"/>\n"_L1;
    }
    return retval;
}

// declared as extern in qdbusconnection_p.h

QString qDBusIntrospectObject(const QDBusConnectionPrivate::ObjectTreeNode &node, const QString &path)
{
    // object may be null

    QString xml_data(DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE ""_L1);
    xml_data += "<node>\n"_L1;

    if (node.obj) {
        Q_ASSERT_X(QThread::currentThread() == node.obj->thread(),
                   "QDBusConnection: internal threading error",
                   "function called for an object that is in another thread!!");

        if (node.flags & (QDBusConnection::ExportScriptableContents
                           | QDBusConnection::ExportNonScriptableContents)) {
            // create XML for the object itself
            const QMetaObject *mo = node.obj->metaObject();
            for ( ; mo != &QObject::staticMetaObject; mo = mo->superClass())
                xml_data += qDBusGenerateMetaObjectXml(node.interfaceName, mo, mo->superClass(),
                                                       node.flags);
        }

        // does this object have adaptors?
        QDBusAdaptorConnector *connector;
        if (node.flags & QDBusConnection::ExportAdaptors &&
            (connector = qDBusFindAdaptorConnector(node.obj))) {

            // trasverse every adaptor in this object
            for (const QDBusAdaptorConnector::AdaptorData &adaptorData :
                 std::as_const(connector->adaptors)) {
                // add the interface:
                QString ifaceXml =
                        QDBusAbstractAdaptorPrivate::retrieveIntrospectionXml(adaptorData.adaptor);
                if (ifaceXml.isEmpty()) {
                    // add the interface's contents:
                    ifaceXml += qDBusGenerateMetaObjectXml(
                            QString::fromLatin1(adaptorData.interface),
                            adaptorData.adaptor->metaObject(),
                            &QDBusAbstractAdaptor::staticMetaObject,
                            QDBusConnection::ExportScriptableContents
                                    | QDBusConnection::ExportNonScriptableContents);

                    QDBusAbstractAdaptorPrivate::saveIntrospectionXml(adaptorData.adaptor,
                                                                      ifaceXml);
                }

                xml_data += ifaceXml;
            }
        }

        // is it a virtual node that handles introspection itself?
        if (node.flags & QDBusConnectionPrivate::VirtualObject) {
            xml_data += node.treeNode->introspect(path);
        }

        xml_data += QLatin1StringView(propertiesInterfaceXml);
    }

    xml_data += QLatin1StringView(introspectableInterfaceXml);
    xml_data += QLatin1StringView(peerInterfaceXml);

    if (node.flags & QDBusConnection::ExportChildObjects) {
        xml_data += generateSubObjectXml(node.obj);
    } else {
        // generate from the object tree
        for (const QDBusConnectionPrivate::ObjectTreeNode &node : node.children) {
            if (node.obj || !node.children.isEmpty())
                xml_data += "  <node name=\""_L1 + node.name + "\"/>\n"_L1;
        }
    }

    xml_data += "</node>\n"_L1;
    return xml_data;
}

// implement the D-Bus interface org.freedesktop.DBus.Properties

static inline QDBusMessage interfaceNotFoundError(const QDBusMessage &msg, const QString &interface_name)
{
    return msg.createErrorReply(QDBusError::UnknownInterface,
                                "Interface %1 was not found in object %2"_L1
                                .arg(interface_name, msg.path()));
}

static inline QDBusMessage
propertyNotFoundError(const QDBusMessage &msg, const QString &interface_name, const QByteArray &property_name)
{
    return msg.createErrorReply(QDBusError::UnknownProperty,
                                "Property %1%2%3 was not found in object %4"_L1
                                .arg(interface_name,
                                     interface_name.isEmpty() ? ""_L1 : "."_L1,
                                     QLatin1StringView(property_name),
                                     msg.path()));
}

QDBusMessage qDBusPropertyGet(const QDBusConnectionPrivate::ObjectTreeNode &node,
                              const QDBusMessage &msg)
{
    Q_ASSERT(msg.arguments().size() == 2);
    Q_ASSERT_X(!node.obj || QThread::currentThread() == node.obj->thread(),
               "QDBusConnection: internal threading error",
               "function called for an object that is in another thread!!");

    QString interface_name = msg.arguments().at(0).toString();
    QByteArray property_name = msg.arguments().at(1).toString().toUtf8();

    const QDBusAdaptorConnector *connector;
    QVariant value;
    bool interfaceFound = false;
    if (node.flags & QDBusConnection::ExportAdaptors &&
        (connector = qDBusFindAdaptorConnector(node.obj))) {

        // find the class that implements interface_name or try until we've found the property
        // in case of an empty interface
        if (interface_name.isEmpty()) {
            for (const QDBusAdaptorConnector::AdaptorData &adaptorData : connector->adaptors) {
                const QMetaObject *mo = adaptorData.adaptor->metaObject();
                int pidx = mo->indexOfProperty(property_name);
                if (pidx != -1) {
                    value = mo->property(pidx).read(adaptorData.adaptor);
                    break;
                }
            }
        } else {
            QDBusAdaptorConnector::AdaptorMap::ConstIterator it;
            it = std::lower_bound(connector->adaptors.constBegin(), connector->adaptors.constEnd(),
                                  interface_name);
            if (it != connector->adaptors.constEnd() && interface_name == QLatin1StringView(it->interface)) {
                interfaceFound = true;
                value = it->adaptor->property(property_name);
            }
        }
    }

    if (!interfaceFound && !value.isValid()
        && node.flags & (QDBusConnection::ExportAllProperties |
                         QDBusConnection::ExportNonScriptableProperties)) {
        // try the object itself
        if (!interface_name.isEmpty())
            interfaceFound = qDBusInterfaceInObject(node.obj, interface_name);

        if (interfaceFound) {
            int pidx = node.obj->metaObject()->indexOfProperty(property_name);
            if (pidx != -1) {
                QMetaProperty mp = node.obj->metaObject()->property(pidx);
                if ((mp.isScriptable() && (node.flags & QDBusConnection::ExportScriptableProperties)) ||
                    (!mp.isScriptable() && (node.flags & QDBusConnection::ExportNonScriptableProperties)))
                    value = mp.read(node.obj);
            }
        }
    }

    if (!value.isValid()) {
        // the property was not found
        if (!interfaceFound)
            return interfaceNotFoundError(msg, interface_name);
        return propertyNotFoundError(msg, interface_name, property_name);
    }

    return msg.createReply(QVariant::fromValue(QDBusVariant(value)));
}

enum PropertyWriteResult {
    PropertyWriteSuccess = 0,
    PropertyNotFound,
    PropertyTypeMismatch,
    PropertyReadOnly,
    PropertyWriteFailed
};

static QDBusMessage propertyWriteReply(const QDBusMessage &msg, const QString &interface_name,
                                       const QByteArray &property_name, int status)
{
    switch (status) {
    case PropertyNotFound:
        return propertyNotFoundError(msg, interface_name, property_name);
    case PropertyTypeMismatch:
        return msg.createErrorReply(QDBusError::InvalidArgs,
                                    "Invalid arguments for writing to property %1%2%3"_L1
                                    .arg(interface_name,
                                         interface_name.isEmpty() ? ""_L1 : "."_L1,
                                         QLatin1StringView(property_name)));
    case PropertyReadOnly:
        return msg.createErrorReply(QDBusError::PropertyReadOnly,
                                    "Property %1%2%3 is read-only"_L1
                                    .arg(interface_name,
                                         interface_name.isEmpty() ? ""_L1 : "."_L1,
                                         QLatin1StringView(property_name)));
    case PropertyWriteFailed:
        return msg.createErrorReply(QDBusError::InternalError,
                                    QString::fromLatin1("Internal error"));

    case PropertyWriteSuccess:
        return msg.createReply();
    }
    Q_ASSERT_X(false, "", "Should not be reached");
    return QDBusMessage();
}

static int writeProperty(QObject *obj, const QByteArray &property_name, QVariant value,
                         int propFlags = QDBusConnection::ExportAllProperties)
{
    const QMetaObject *mo = obj->metaObject();
    int pidx = mo->indexOfProperty(property_name);
    if (pidx == -1) {
        // this object has no property by that name
        return PropertyNotFound;
    }

    QMetaProperty mp = mo->property(pidx);

    // check if this property is writable
    if (!mp.isWritable())
        return PropertyReadOnly;

    // check if this property is exported
    bool isScriptable = mp.isScriptable();
    if (!(propFlags & QDBusConnection::ExportScriptableProperties) && isScriptable)
        return PropertyNotFound;
    if (!(propFlags & QDBusConnection::ExportNonScriptableProperties) && !isScriptable)
        return PropertyNotFound;

    // we found our property
    // do we have the right type?
    QMetaType id = mp.metaType();
    if (!id.isValid()){
        // type not registered or invalid / void?
        qWarning("QDBusConnection: Unable to handle unregistered datatype '%s' for property '%s::%s'",
                 mp.typeName(), mo->className(), property_name.constData());
        return PropertyWriteFailed;
    }

    if (id.id() != QMetaType::QVariant && value.metaType() == QDBusMetaTypeId::argument()) {
        // we have to demarshall before writing
        QVariant other{QMetaType(id)};
        if (!QDBusMetaType::demarshall(qvariant_cast<QDBusArgument>(value), other.metaType(), other.data())) {
            qWarning("QDBusConnection: type '%s' (%d) is not registered with QtDBus. "
                     "Use qDBusRegisterMetaType to register it",
                     mp.typeName(), id.id());
            return PropertyWriteFailed;
        }

        value = std::move(other);
    }

    if (mp.metaType() == QMetaType::fromType<QDBusVariant>())
        value = QVariant::fromValue(QDBusVariant(value));

    // the property type here should match
    return mp.write(obj, std::move(value)) ? PropertyWriteSuccess : PropertyWriteFailed;
}

QDBusMessage qDBusPropertySet(const QDBusConnectionPrivate::ObjectTreeNode &node,
                              const QDBusMessage &msg)
{
    Q_ASSERT(msg.arguments().size() == 3);
    Q_ASSERT_X(!node.obj || QThread::currentThread() == node.obj->thread(),
               "QDBusConnection: internal threading error",
               "function called for an object that is in another thread!!");

    QString interface_name = msg.arguments().at(0).toString();
    QByteArray property_name = msg.arguments().at(1).toString().toUtf8();
    QVariant value = qvariant_cast<QDBusVariant>(msg.arguments().at(2)).variant();

    QDBusAdaptorConnector *connector;
    if (node.flags & QDBusConnection::ExportAdaptors &&
        (connector = qDBusFindAdaptorConnector(node.obj))) {

        // find the class that implements interface_name or try until we've found the property
        // in case of an empty interface
        if (interface_name.isEmpty()) {
            for (const QDBusAdaptorConnector::AdaptorData &adaptorData :
                 std::as_const(connector->adaptors)) {
                int status = writeProperty(adaptorData.adaptor, property_name, value);
                if (status == PropertyNotFound)
                    continue;
                return propertyWriteReply(msg, interface_name, property_name, status);
            }
        } else {
            QDBusAdaptorConnector::AdaptorMap::ConstIterator it;
            it = std::lower_bound(connector->adaptors.constBegin(), connector->adaptors.constEnd(),
                                  interface_name);
            if (it != connector->adaptors.cend() && interface_name == QLatin1StringView(it->interface)) {
                return propertyWriteReply(msg, interface_name, property_name,
                                          writeProperty(it->adaptor, property_name, value));
            }
        }
    }

    if (node.flags & (QDBusConnection::ExportScriptableProperties |
                      QDBusConnection::ExportNonScriptableProperties)) {
        // try the object itself
        bool interfaceFound = true;
        if (!interface_name.isEmpty())
            interfaceFound = qDBusInterfaceInObject(node.obj, interface_name);

        if (interfaceFound) {
            return propertyWriteReply(msg, interface_name, property_name,
                                      writeProperty(node.obj, property_name, value, node.flags));
        }
    }

    // the property was not found
    if (!interface_name.isEmpty())
        return interfaceNotFoundError(msg, interface_name);
    return propertyWriteReply(msg, interface_name, property_name, PropertyNotFound);
}

// unite two QVariantMaps, but don't generate duplicate keys
static QVariantMap &operator+=(QVariantMap &lhs, const QVariantMap &rhs)
{
    for (const auto &[key, value] : rhs.asKeyValueRange())
        lhs.insert(key, value);
    return lhs;
}

static QVariantMap readAllProperties(QObject *object, int flags)
{
    QVariantMap result;
    const QMetaObject *mo = object->metaObject();

    // QObject has properties, so don't start from 0
    for (int i = QObject::staticMetaObject.propertyCount(); i < mo->propertyCount(); ++i) {
        QMetaProperty mp = mo->property(i);

        // is it readable?
        if (!mp.isReadable())
            continue;

        // is it a registered property?
        QMetaType type = mp.metaType();
        if (!type.isValid())
            continue;
        const char *signature = QDBusMetaType::typeToSignature(type);
        if (!signature)
            continue;

        // is this property visible from the outside?
        if ((mp.isScriptable() && flags & QDBusConnection::ExportScriptableProperties) ||
            (!mp.isScriptable() && flags & QDBusConnection::ExportNonScriptableProperties)) {
            // yes, it's visible
            QVariant value = mp.read(object);
            if (value.isValid())
                result.insert(QString::fromLatin1(mp.name()), value);
        }
    }

    return result;
}

QDBusMessage qDBusPropertyGetAll(const QDBusConnectionPrivate::ObjectTreeNode &node,
                                 const QDBusMessage &msg)
{
    Q_ASSERT(msg.arguments().size() == 1);
    Q_ASSERT_X(!node.obj || QThread::currentThread() == node.obj->thread(),
               "QDBusConnection: internal threading error",
               "function called for an object that is in another thread!!");

    QString interface_name = msg.arguments().at(0).toString();

    bool interfaceFound = false;
    QVariantMap result;

    QDBusAdaptorConnector *connector;
    if (node.flags & QDBusConnection::ExportAdaptors &&
        (connector = qDBusFindAdaptorConnector(node.obj))) {

        if (interface_name.isEmpty()) {
            // iterate over all interfaces
            for (const QDBusAdaptorConnector::AdaptorData &adaptorData :
                 std::as_const(connector->adaptors)) {
                result += readAllProperties(adaptorData.adaptor,
                                            QDBusConnection::ExportAllProperties);
            }
        } else {
            // find the class that implements interface_name
            QDBusAdaptorConnector::AdaptorMap::ConstIterator it;
            it = std::lower_bound(connector->adaptors.constBegin(), connector->adaptors.constEnd(),
                                  interface_name);
            if (it != connector->adaptors.constEnd() && interface_name == QLatin1StringView(it->interface)) {
                interfaceFound = true;
                result = readAllProperties(it->adaptor, QDBusConnection::ExportAllProperties);
            }
        }
    }

    if (node.flags & QDBusConnection::ExportAllProperties &&
        (!interfaceFound || interface_name.isEmpty())) {
        // try the object itself
        result += readAllProperties(node.obj, node.flags);
        interfaceFound = true;
    }

    if (!interfaceFound && !interface_name.isEmpty()) {
        // the interface was not found
        return interfaceNotFoundError(msg, interface_name);
    }

    return msg.createReply(QVariant::fromValue(result));
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
