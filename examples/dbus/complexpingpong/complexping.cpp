// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "ping-common.h"
#include "complexping.h"

#include <QCoreApplication>
#include <QDBusReply>
#include <QDBusServiceWatcher>
#include <QFile>
#include <QDebug>
#include <QProcess>

#include <stdio.h>

void Ping::start(const QString &name)
{
    if (name != SERVICE_NAME)
        return;

    // open stdin for reading
    qstdin.open(stdin, QIODevice::ReadOnly);

    // find our remote
    iface = new QDBusInterface(SERVICE_NAME, "/", "org.example.QtDBus.ComplexPong.Pong",
                               QDBusConnection::sessionBus(), this);
    if (!iface->isValid()) {
        fprintf(stderr, "%s\n",
                qPrintable(QDBusConnection::sessionBus().lastError().message()));
        QCoreApplication::instance()->quit();
    }

    connect(iface, SIGNAL(aboutToQuit()), QCoreApplication::instance(), SLOT(quit()));

    while (true) {
        printf("Ask your question: ");

        QString line = QString::fromLocal8Bit(qstdin.readLine()).trimmed();
        if (line.isEmpty()) {
            iface->call("quit");
            return;
        } else if (line == "value") {
            QVariant reply = iface->property("value");
            if (!reply.isNull())
                printf("value = %s\n", qPrintable(reply.toString()));
        } else if (line.startsWith("value=")) {
            iface->setProperty("value", line.mid(6));
        } else {
            QDBusReply<QDBusVariant> reply = iface->call("query", line);
            if (reply.isValid())
                printf("Reply was: %s\n", qPrintable(reply.value().variant().toString()));
        }

        if (iface->lastError().isValid())
            fprintf(stderr, "Call failed: %s\n", qPrintable(iface->lastError().message()));
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (!QDBusConnection::sessionBus().isConnected()) {
        fprintf(stderr, "Cannot connect to the D-Bus session bus.\n"
                "To start it, run:\n"
                "\teval `dbus-launch --auto-syntax`\n");
        return 1;
    }

    QDBusServiceWatcher serviceWatcher(SERVICE_NAME, QDBusConnection::sessionBus(),
                                       QDBusServiceWatcher::WatchForRegistration);

    Ping ping;
    QObject::connect(&serviceWatcher, &QDBusServiceWatcher::serviceRegistered,
                     &ping, &Ping::start);

    QProcess pong;
    pong.start("./complexpong");

    app.exec();
}
