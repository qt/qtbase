// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QFile>
#include <QDebug>

int main(int, char **)
{
    QFile file(":/test.txt");
    if (!file.open(QFile::ReadOnly))
        return 1;

    QString data = QString::fromUtf8(file.readAll());
    qDebug() << data;
    return 0;
}
