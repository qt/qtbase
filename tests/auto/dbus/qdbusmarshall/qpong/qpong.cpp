// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QCoreApplication>
#include <QDBusMessage>
#include <QDBusConnection>

static const char serviceName[] = "org.qtproject.autotests.qpong";
static const char objectPath[] = "/org/qtproject/qpong";
//static const char *interfaceName = serviceName;

class Pong: public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.qtproject.autotests.qpong")
public slots:

    void ping(QDBusMessage msg)
    {
        msg.setDelayedReply(true);
        if (!QDBusConnection::sessionBus().send(msg.createReply(msg.arguments())))
            exit(1);
    }

    void quit()
    {
        qApp->quit();
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QDBusConnection con = QDBusConnection::sessionBus();
    if (!con.isConnected())
        exit(1);

    if (!con.registerService(serviceName))
        exit(2);

    Pong pong;
    con.registerObject(objectPath, &pong, QDBusConnection::ExportAllSlots);

    printf("ready.\n");
    fflush(stdout);

    return app.exec();
}

#include "qpong.moc"
