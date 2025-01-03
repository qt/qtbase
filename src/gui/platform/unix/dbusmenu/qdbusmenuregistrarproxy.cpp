// Copyright (C) 2016 Dmitry Shachnev <mitya57@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

/*
 * This file was originally created by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp -p qdbusmenuregistrarproxy ../../3rdparty/dbus-ifaces/com.canonical.AppMenu.Registrar.xml
 *
 * However it is maintained manually.
 */

#include "qdbusmenuregistrarproxy_p.h"

QT_BEGIN_NAMESPACE

/*
 * Implementation of interface class QDBusMenuRegistrarInterface
 */

QDBusMenuRegistrarInterface::QDBusMenuRegistrarInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(service, path, staticInterfaceName(), connection, parent)
{
}

QDBusMenuRegistrarInterface::~QDBusMenuRegistrarInterface()
{
}

QT_END_NAMESPACE

#include "moc_qdbusmenuregistrarproxy_p.cpp"
