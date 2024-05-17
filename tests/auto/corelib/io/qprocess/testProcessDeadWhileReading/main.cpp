// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <stdio.h>

int main()
{
    for (int i=0; i<10240; i++)
        fprintf(stdout, "%d dead while reading\n", i);
    fflush(stdout);

    return 0;
}
