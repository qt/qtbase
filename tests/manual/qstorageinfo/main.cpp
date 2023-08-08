// Copyright (C) 2016 Intel Corporation
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QCoreApplication>

#include <stdio.h>

#include "printvolumes.cpp"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QList<QStorageInfo> volumes;
    QStringList args = a.arguments();
    args.takeFirst();   // skip application name

    for (const QString &path : std::as_const(args)) {
        QStorageInfo info(path);
        if (!info.isValid()) {
            // no error string...
            fprintf(stderr, "Could not get info on %s\n", qPrintable(path));
            return 1;
        }
        volumes << info;
    }

    if (volumes.isEmpty())
        volumes = QStorageInfo::mountedVolumes();

    printVolumes(volumes, printf);

    return 0;
}
