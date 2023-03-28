// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "ping-common.h"
#include "complexpong.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QDebug>

QString Pong::value() const
{
    return m_value;
}

void Pong::setValue(const QString &newValue)
{
    m_value = newValue;
}

void Pong::quit()
{
    QMetaObject::invokeMethod(QCoreApplication::instance(), &QCoreApplication::quit,
                              Qt::QueuedConnection);
}

QDBusVariant Pong::query(const QString &query)
{
    QString q = query.toLower();
    if (q == "hello")
        return QDBusVariant("World");
    if (q == "ping")
        return QDBusVariant("Pong");
    if (q.indexOf("the answer to life, the universe and everything") != -1)
        return QDBusVariant(42);
    if (q.indexOf("unladen swallow") != -1) {
        if (q.indexOf("european") != -1)
            return QDBusVariant(11.0);
        return QDBusVariant(QByteArray("african or european?"));
    }

    return QDBusVariant("Sorry, I don't know the answer");
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QObject obj;
    Pong *pong = new Pong(&obj);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, pong, &Pong::aboutToQuit);
    pong->setProperty("value", "initial value");

    auto connection = QDBusConnection::sessionBus();
    connection.registerObject("/", &obj);

    if (!connection.registerService(SERVICE_NAME)) {
        qWarning().noquote() << connection.lastError().message();
        return 1;
    }

    app.exec();
    return 0;
}

