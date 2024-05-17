// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QCoreApplication>
#include <QDebug>
#include <QStringList>
#include <QSystemSemaphore>

int acquire(const QNativeIpcKey &key, int count = 1)
{
    QSystemSemaphore sem(key);

    for (int i = 0; i < count; ++i) {
        if (!sem.acquire()) {
            qWarning() << "Could not acquire" << sem.key();
            return EXIT_FAILURE;
        }
    }
    qDebug("done aquiring");
    return EXIT_SUCCESS;
}

int release(const QNativeIpcKey &key)
{
    QSystemSemaphore sem(key);
    if (!sem.release()) {
        qWarning() << "Could not release" << sem.key();
        return EXIT_FAILURE;
    }
    qDebug("done releasing");
    return EXIT_SUCCESS;
}

int acquirerelease(const QNativeIpcKey &key)
{
    QSystemSemaphore sem(key);
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
    if (arguments.size() < 2) {
        fprintf(stderr,
                "Usage: %s <acquire|release|acquirerelease> <key> [other args...]\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    QString function = arguments.takeFirst();
    QNativeIpcKey key = QNativeIpcKey::fromString(arguments.takeFirst());
    if (function == QLatin1String("acquire")) {
        int count = 1;
        bool ok = true;
        if (arguments.size())
            count = arguments.takeFirst().toInt(&ok);
        if (!ok)
            count = 1;
        return acquire(key, count);
    } else if (function == QLatin1String("release")) {
        return release(key);
    } else if (function == QLatin1String("acquirerelease")) {
        return acquirerelease(key);
    } else {
        qWarning() << "Unknown function" << function;
    }
    return EXIT_SUCCESS;
}
