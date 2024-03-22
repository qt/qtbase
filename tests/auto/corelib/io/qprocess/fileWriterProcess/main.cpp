// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QCoreApplication>
#include <QFile>

#include <stdio.h>

int main(int argc, char **argv)
{
    QCoreApplication ca(argc, argv);
    QFile f;
    if (!f.open(stdin, QIODevice::ReadOnly))
        return 1;
    QByteArray input;
    char buf[1024];
    qint64 len;
    while ((len = f.read(buf, 1024)) > 0)
        input.append(buf, len);
    f.close();
    QFile f2("fileWriterProcess.txt");
    if (!f2.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fprintf(stderr, "Cannot open %s for writing: %s\n",
                qPrintable(f2.fileName()), qPrintable(f2.errorString()));
        return 1;
    }
    f2.write(input);
    f2.close();
    return 0;
}
