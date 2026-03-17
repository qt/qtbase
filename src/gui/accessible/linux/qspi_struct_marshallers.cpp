// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qspi_struct_marshallers_p.h"

#include <atspi/atspi-constants.h>
#include <QtCore/qdebug.h>
#include <QtDBus/qdbusmetatype.h>

#include "qspiaccessiblebridge_p.h"

#if QT_CONFIG(accessibility)
QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN(QSpiIntList)
QT_IMPL_METATYPE_EXTERN(QSpiUIntList)
QT_IMPL_METATYPE_EXTERN(QSpiObjectReference)
QT_IMPL_METATYPE_EXTERN(QSpiObjectReferenceArray)
QT_IMPL_METATYPE_EXTERN(QSpiAccessibleCacheItem)
QT_IMPL_METATYPE_EXTERN(QSpiAccessibleCacheArray)
QT_IMPL_METATYPE_EXTERN(QSpiAction)
QT_IMPL_METATYPE_EXTERN(QSpiActionArray)
QT_IMPL_METATYPE_EXTERN(QSpiEventListener)
QT_IMPL_METATYPE_EXTERN(QSpiEventListenerArray)
QT_IMPL_METATYPE_EXTERN(QSpiRelationArrayEntry)
QT_IMPL_METATYPE_EXTERN(QSpiRelationArray)
QT_IMPL_METATYPE_EXTERN(QSpiTextRange)
QT_IMPL_METATYPE_EXTERN(QSpiTextRangeList)
QT_IMPL_METATYPE_EXTERN(QSpiAttributeSet)
QT_IMPL_METATYPE_EXTERN(QSpiDeviceEvent)
QT_IMPL_METATYPE_EXTERN(QSpiMatchRule)

QSpiObjectReference::QSpiObjectReference()
    : path(QDBusObjectPath(ATSPI_DBUS_PATH_NULL))
{}

/* QSpiAccessibleCacheArray */
/*---------------------------------------------------------------------------*/

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiAccessibleCacheItem &item)
{
    argument.beginStructure();
    argument << item.path;
    argument << item.application;
    argument << item.parent;
    argument << item.index_in_parent;
    argument << item.child_count;
    argument << item.supportedInterfaces;
    argument << item.name;
    argument << item.role;
    argument << item.description;
    argument << item.state;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiAccessibleCacheItem &item)
{
    argument.beginStructure();
    argument >> item.path;
    argument >> item.application;
    argument >> item.parent;
    argument >> item.index_in_parent;
    argument >> item.child_count;
    argument >> item.supportedInterfaces;
    argument >> item.name;
    argument >> item.role;
    argument >> item.description;
    argument >> item.state;
    argument.endStructure();
    return argument;
}

/* QSpiObjectReference */
/*---------------------------------------------------------------------------*/

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiObjectReference &address)
{
    argument.beginStructure();
    argument << address.service;
    argument << address.path;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiObjectReference &address)
{
    argument.beginStructure();
    argument >> address.service;
    argument >> address.path;
    argument.endStructure();
    return argument;
}

/* QSpiAction */
/*---------------------------------------------------------------------------*/

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiAction &action)
{
    argument.beginStructure();
    argument << action.name;
    argument << action.description;
    argument << action.keyBinding;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiAction &action)
{
    argument.beginStructure();
    argument >> action.name;
    argument >> action.description;
    argument >> action.keyBinding;
    argument.endStructure();
    return argument;
}


QDBusArgument &operator<<(QDBusArgument &argument, const QSpiEventListener &ev)
{
    argument.beginStructure();
    argument << ev.listenerAddress;
    argument << ev.eventName;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiEventListener &ev)
{
    argument.beginStructure();
    argument >> ev.listenerAddress;
    argument >> ev.eventName;
    argument.endStructure();
    return argument;
}

/* QSpiRelationArrayEntry */
/*---------------------------------------------------------------------------*/

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiRelationArrayEntry &entry) {
    argument.beginStructure();
    argument << entry.first << entry.second;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiRelationArrayEntry &entry) {
    argument.beginStructure();
    argument >> entry.first >> entry.second;
    argument.endStructure();
    return argument;
}

/* QSpiDeviceEvent */
/*---------------------------------------------------------------------------*/

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiDeviceEvent &event) {
    argument.beginStructure();
    argument << event.type
             << event.id
             << event.hardwareCode
             << event.modifiers
             << event.timestamp
             << event.text
             << event.isText;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiDeviceEvent &event) {
    argument.beginStructure();
    argument >> event.type
             >> event.id
             >> event.hardwareCode
             >> event.modifiers
             >> event.timestamp
             >> event.text
             >> event.isText;
    argument.endStructure();
    return argument;
}

/* QSpiMatchRule */
/*---------------------------------------------------------------------------*/

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiMatchRule &matchRule) {
    argument.beginStructure();
    argument << matchRule.states
             << matchRule.stateMatchType
             << matchRule.attributes
             << matchRule.attributeMatchType
             << matchRule.interfaces
             << matchRule.interfaceMatchType
             << matchRule.roles
             << matchRule.roleMatchType
             << matchRule.invert;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiMatchRule &matchRule) {
    argument.beginStructure();
    argument >> matchRule.states
            >> matchRule.stateMatchType
            >> matchRule.attributes
            >> matchRule.attributeMatchType
            >> matchRule.roles
            >> matchRule.roleMatchType
            >> matchRule.interfaces
            >> matchRule.interfaceMatchType
            >> matchRule.invert;
    argument.endStructure();
    return argument;
}

void qSpiInitializeStructTypes()
{
    qDBusRegisterMetaType<QSpiIntList>();
    qDBusRegisterMetaType<QSpiUIntList>();
    qDBusRegisterMetaType<QSpiAccessibleCacheItem>();
    qDBusRegisterMetaType<QSpiAccessibleCacheArray>();
    qDBusRegisterMetaType<QSpiObjectReference>();
    qDBusRegisterMetaType<QSpiObjectReferenceArray>();
    qDBusRegisterMetaType<QSpiAttributeSet>();
    qDBusRegisterMetaType<QSpiAction>();
    qDBusRegisterMetaType<QSpiActionArray>();
    qDBusRegisterMetaType<QSpiEventListener>();
    qDBusRegisterMetaType<QSpiEventListenerArray>();
    qDBusRegisterMetaType<QSpiDeviceEvent>();
    qDBusRegisterMetaType<QSpiMatchRule>();
    qDBusRegisterMetaType<QSpiRelationArrayEntry>();
    qDBusRegisterMetaType<QSpiRelationArray>();
}

QT_END_NAMESPACE
#endif // QT_CONFIG(accessibility)
