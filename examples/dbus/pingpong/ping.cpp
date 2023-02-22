// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "ping-common.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

#include <iostream>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    auto connection = QDBusConnection::sessionBus();

    if (!connection.isConnected()) {
        qWarning("Cannot connect to the D-Bus session bus.\n"
                 "To start it, run:\n"
                 "\teval `dbus-launch --auto-syntax`\n");
        return 1;
    }

    QDBusInterface iface(SERVICE_NAME, "/");
    if (iface.isValid()) {
        QDBusReply<QString> reply = iface.call("ping", argc > 1 ? argv[1] : "");
        if (reply.isValid()) {
            std::cout << "Reply was: " << qPrintable(reply.value()) << std::endl;
            return 0;
        }

        qWarning("Call failed: %s\n", qPrintable(reply.error().message()));
        return 1;
    }

    qWarning("%s\n", qPrintable(connection.lastError().message()));
    return 1;
}
