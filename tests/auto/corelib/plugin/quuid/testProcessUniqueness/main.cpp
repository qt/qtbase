// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <stdio.h>
#include <QUuid>

// This is a testcase for QTBUG-11213
int main(int argc, char **argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    // Now print a few uuids.
    printf("%s", qPrintable(QUuid::createUuid().toString()));
    printf("%s", qPrintable(QUuid::createUuid().toString()));
    printf("%s", qPrintable(QUuid::createUuid().toString()));

    // Done
    return 0;
}

