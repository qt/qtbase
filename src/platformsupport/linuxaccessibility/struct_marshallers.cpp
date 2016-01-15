/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "struct_marshallers_p.h"

#include <atspi/atspi-constants.h>
#include <QtCore/qdebug.h>
#include <QtDBus/qdbusmetatype.h>

#include "bridge_p.h"

#ifndef QT_NO_ACCESSIBILITY
QT_BEGIN_NAMESPACE

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
    argument << item.children;
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
    argument >> item.children;
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

/* QSpiAppUpdate */
/*---------------------------------------------------------------------------*/

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiAppUpdate &update) {
    argument.beginStructure();
    argument << update.type << update.address;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiAppUpdate &update) {
    argument.beginStructure();
    argument >> update.type >> update.address;
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
    qDBusRegisterMetaType<QSpiAppUpdate>();
    qDBusRegisterMetaType<QSpiRelationArrayEntry>();
    qDBusRegisterMetaType<QSpiRelationArray>();
}

QT_END_NAMESPACE
#endif //QT_NO_ACCESSIBILITY
