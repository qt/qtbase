// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "ping-common.h"

#include <QObject>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>

class Pong : public QObject
{
    Q_OBJECT
public slots:
    QString ping(const QString &arg);
};

QString Pong::ping(const QString &arg)
{
    QMetaObject::invokeMethod(QCoreApplication::instance(), &QCoreApplication::quit);
    return QString("ping(\"%1\") got called").arg(arg);
}

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

    if (!connection.registerService(SERVICE_NAME)) {
        qWarning("%s\n", qPrintable(connection.lastError().message()));
        exit(1);
    }

    Pong pong;
    connection.registerObject("/", &pong, QDBusConnection::ExportAllSlots);

    app.exec();
    return 0;
}

#include "pong.moc"
