// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtCore/QTextStream>
#include <stdio.h>

int main(int, char**)
{
    QTextStream qin(stdin);
    if (!qin.atEnd()) {
        int a, b, c;
        qin >> a >> b >> c;
        fprintf(stderr, "%d %d %d\n", a, b, c);
    }
    return 0;
}
