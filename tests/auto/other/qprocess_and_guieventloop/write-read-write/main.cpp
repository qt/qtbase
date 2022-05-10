// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <stdio.h>

int main(int, char **argv)
{
    const char *msg = argv[1];
    char buf[2];

    puts(msg);
    fflush(stdout);

    // wait for a newline
    const char *result = fgets(buf, sizeof buf, stdin);
    if (result != buf)
        return -1;

    puts(msg);
    fflush(stdout);
    return 0;
}
