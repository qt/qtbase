// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QCoreApplication>
#include <QDebug>
#include <QStringList>
#include <QSystemSemaphore>

int acquire(int count = 1)
{
    QSystemSemaphore sem("store");

    for (int i = 0; i < count; ++i) {
        if (!sem.acquire()) {
            qWarning() << "Could not acquire" << sem.key();
            return EXIT_FAILURE;
        }
    }
    qDebug("done aquiring");
    return EXIT_SUCCESS;
}

int release()
{
    QSystemSemaphore sem("store");
    if (!sem.release()) {
        qWarning() << "Could not release" << sem.key();
        return EXIT_FAILURE;
    }
    qDebug("done releasing");
    return EXIT_SUCCESS;
}

int acquirerelease()
{
    QSystemSemaphore sem("store");
    if (!sem.acquire()) {
        qWarning() << "Could not acquire" << sem.key();
        return EXIT_FAILURE;
    }
    if (!sem.release()) {
        qWarning() << "Could not release" << sem.key();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QStringList arguments = app.arguments();
    // binary name is not used here
    arguments.takeFirst();
    if (arguments.size() < 1) {
        qWarning("Please call the helper with the function to call as argument");
        return EXIT_FAILURE;
    }
    QString function = arguments.takeFirst();
    if (function == QLatin1String("acquire")) {
        int count = 1;
        bool ok = true;
        if (arguments.size())
            count = arguments.takeFirst().toInt(&ok);
        if (!ok)
            count = 1;
        return acquire(count);
    } else if (function == QLatin1String("release")) {
        return release();
    } else if (function == QLatin1String("acquirerelease")) {
        return acquirerelease();
    } else {
        qWarning() << "Unknown function" << function;
    }
    return EXIT_SUCCESS;
}
