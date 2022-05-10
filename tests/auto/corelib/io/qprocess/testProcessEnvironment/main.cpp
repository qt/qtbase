// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc == 1)
        return 1;

    char *env = getenv(argv[1]);
    if (env) {
        printf("%s", env);
        return 0;
    }
    return 1;
}
