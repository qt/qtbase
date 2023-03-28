// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "ping-common.h"
#include "complexping.h"

#include <QCoreApplication>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusServiceWatcher>
#include <QDebug>
#include <QProcess>

#include <iostream>
#include <string>

void Ping::start(const QString &name)
{
    if (name != SERVICE_NAME)
        return;

    auto connection = QDBusConnection::sessionBus();
    // find our remote
    auto iface = new QDBusInterface(SERVICE_NAME, "/", "org.example.QtDBus.ComplexPong.Pong",
                                    connection, this);
    if (!iface->isValid()) {
        qWarning().noquote() << connection.lastError().message();
        QCoreApplication::instance()->quit();
    }

    connect(iface, SIGNAL(aboutToQuit()), QCoreApplication::instance(), SLOT(quit()));

    std::string s;

    while (true) {
        std::cout << qPrintable(tr("Ask your question: ")) << std::flush;

        std::getline(std::cin, s);
        auto line = QString::fromStdString(s).trimmed();

        if (line.isEmpty()) {
            iface->call("quit");
            return;
        } else if (line == "value") {
            QVariant reply = iface->property("value");
            if (!reply.isNull())
                std::cout << "value = " << qPrintable(reply.toString()) << std::endl;
        } else if (line.startsWith("value=")) {
            iface->setProperty("value", line.mid(6));
        } else {
            QDBusReply<QDBusVariant> reply = iface->call("query", line);
            if (reply.isValid()) {
                std::cout << qPrintable(tr("Reply was: %1").arg(reply.value().variant().toString()))
                          << std::endl;
            }
        }

        if (iface->lastError().isValid())
            qWarning().noquote() << tr("Call failed: %1").arg(iface->lastError().message());
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (!QDBusConnection::sessionBus().isConnected()) {
        qWarning().noquote() << QCoreApplication::translate(
                "complexping",
                "Cannot connect to the D-Bus session bus.\n"
                "To start it, run:\n"
                "\teval `dbus-launch --auto-syntax`");
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
