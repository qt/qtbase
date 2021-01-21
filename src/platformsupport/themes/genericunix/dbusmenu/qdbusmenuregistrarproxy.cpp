/****************************************************************************
**
** Copyright (C) 2016 Dmitry Shachnev <mitya57@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
