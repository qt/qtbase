// Copyright (C) 2025 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QTimer>
#include <QtCore/QCoreApplication>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>

#include <stdio.h>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

static QString myInterface()
{
    return u"local.qdbusdelayeddeliveryreenablehelper"_s;
}

static void makeSynchronousCall(QDBusConnection &conn)
{
    QDBusMessage msg = QDBusMessage::createMethodCall("org.freedesktop.DBus", "/",
                                                      "org.freedesktop.DBus.Peer", "Ping");
    conn.call(msg);
}

static void emitSignal(QDBusConnection &conn)
{
    QDBusMessage msg = QDBusMessage::createSignal("/", myInterface(), "quit");
    conn.send(msg);
}

int main(int argc, char **argv)
{
    // Open a connection to the bus *before* QCoreApplication exists;
    // this will put the connection in delayed delivery mode
    QDBusConnection session = QDBusConnection::sessionBus();
    if (!session.isConnected()) {
        fprintf(stderr, "Session bus did not connect!");
        return 1;
    }
    makeSynchronousCall(session);

    QCoreApplication app(argc, argv);
    QTimer::singleShot(15s, qApp, [] {
        fprintf(stderr, "Did not receive signal.\n");
        qApp->exit(1);
    });

    // connect a remote, wildcard signal to qApp->quit()
    session.connect(QString(), QString(), myInterface(), "quit", &app, SLOT(quit()));

    // send ourselves the signal to quit, via D-Bus
    emitSignal(session);

    return app.exec();
}
