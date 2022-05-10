// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <stdio.h>

int main()
{
    for (int i=0; i<10240; i++) {
        fprintf(stdout, "%d -this is a number\n", i);
        fflush(stderr);
    }
    return 0;
}
