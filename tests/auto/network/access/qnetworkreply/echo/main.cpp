// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QFile>

int main(int argc, char **)
{
    if (argc < 2) {
        printf("usage: echo\n");
        printf("echos all its input to its output.\n");
        return 1;
    }

    QFile file;
    file.open(stdin, QFile::ReadWrite);
    QByteArray data = file.readAll();
    file.close();

    file.open(stdout, QFile::WriteOnly);
    file.write(data);
    file.close();
    return 0;
}
