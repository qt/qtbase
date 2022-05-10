// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <stdio.h>

int main()
{
    int c;
    for (;;) {
        c = fgetc(stdin);
        if (c == '\0')
            break;
        if (c != -1) {
            fputc(c, stdout);
            fputc(c, stderr);
            fflush(stdout);
            fflush(stderr);
        }
    }

    return 0;
}
