// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QFile>

int main(int argc, char **)
{
    if (argc < 2) {
        printf("usage: echo\n");
        printf("echos all its input to its output.\n");
        return 1;
    }

    QFile file;
    if (!file.open(stdin, QFile::ReadWrite))
        return 1;
    QByteArray data = file.readAll();
    file.close();

    if (!file.open(stdout, QFile::WriteOnly))
        return 1;
    file.write(data);
    file.close();
    return 0;
}
