// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <stdio.h>

int main(int argc, char ** argv)
{
    for (int i = 0; i < argc; ++i) {
        if (i)
            printf("|");
        printf("%s", argv[i]);
    }
    return 0;
}
