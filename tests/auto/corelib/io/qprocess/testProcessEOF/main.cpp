// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <stdio.h>
#include <string.h>

int main()
{
    char buf[32];
    memset(buf, 0, sizeof(buf));

    char *cptr = buf;
    int c;
    while (cptr != buf + 31 && (c = fgetc(stdin)) != EOF)
        *cptr++ = (char) c;

    printf("%s", buf);
    return 0;
}
