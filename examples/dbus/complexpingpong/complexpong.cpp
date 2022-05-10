// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "ping-common.h"
#include "complexpong.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QTimer>

#include <stdio.h>
#include <stdlib.h>

// the property
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
    QTimer::singleShot(0, QCoreApplication::instance(), &QCoreApplication::quit);
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
    QDBusConnection::sessionBus().registerObject("/", &obj);

    if (!QDBusConnection::sessionBus().registerService(SERVICE_NAME)) {
        fprintf(stderr, "%s\n",
                qPrintable(QDBusConnection::sessionBus().lastError().message()));
        exit(1);
    }

    app.exec();
    return 0;
}

