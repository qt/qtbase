// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QtCore/QtCore>


#include "../serverobject.h"

static const char serviceName[] = "org.qtproject.autotests.performance";
static const char objectPath[] = "/";

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QDBusConnection con = QDBusConnection::sessionBus();
    if (!con.isConnected())
        exit(1);

    if (!con.registerService(serviceName))
        exit(2);

    ServerObject obj(objectPath, con);
    printf("ready.\n");
    fflush(stdout);
    return app.exec();
}

