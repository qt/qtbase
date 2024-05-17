// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <stdio.h>

int main()
{
    int c;
    while ((c = fgetc(stdin)) != -1) {
        if (c == '\0')
            break;
        fputc(c, stdout);
        fflush(stdout);
    }

    return 0;
}
