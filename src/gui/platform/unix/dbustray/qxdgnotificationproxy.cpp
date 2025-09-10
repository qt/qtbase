// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qxdgnotificationproxy_p.h"

QT_BEGIN_NAMESPACE

QXdgNotificationInterface::QXdgNotificationInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(service, path, staticInterfaceName(), connection, parent)
{
}

QXdgNotificationInterface::~QXdgNotificationInterface()
{
}

QT_END_NAMESPACE

#include "moc_qxdgnotificationproxy_p.cpp"
