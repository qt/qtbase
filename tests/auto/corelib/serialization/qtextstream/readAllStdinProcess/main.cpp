// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtCore/QTextStream>
#include <QtCore/QString>
#include <stdio.h>

int main(int, char**)
{
    fprintf(stderr, "%s\n", QTextStream(stdin).readAll().toLatin1().constData());
    return 0;
}
