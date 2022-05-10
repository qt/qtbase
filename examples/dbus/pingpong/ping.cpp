// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "ping-common.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

#include <stdio.h>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (!QDBusConnection::sessionBus().isConnected()) {
        fprintf(stderr, "Cannot connect to the D-Bus session bus.\n"
                "To start it, run:\n"
                "\teval `dbus-launch --auto-syntax`\n");
        return 1;
    }

    QDBusInterface iface(SERVICE_NAME, "/", "", QDBusConnection::sessionBus());
    if (iface.isValid()) {
        QDBusReply<QString> reply = iface.call("ping", argc > 1 ? argv[1] : "");
        if (reply.isValid()) {
            printf("Reply was: %s\n", qPrintable(reply.value()));
            return 0;
        }

        fprintf(stderr, "Call failed: %s\n", qPrintable(reply.error().message()));
        return 1;
    }

    fprintf(stderr, "%s\n",
            qPrintable(QDBusConnection::sessionBus().lastError().message()));
    return 1;
}
