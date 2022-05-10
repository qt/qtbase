// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <stdio.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#  include <windows.h>

void sleepForever()
{
    ::Sleep(INFINITE);
}
#else
#  include <unistd.h>

void sleepForever()
{
    ::pause();
}
#endif

int main()
{
    puts("ready.");
    fflush(stdout);
    fprintf(stderr, "ready.\n");
    fflush(stderr);

    // sleep forever, simulating a hung application
    sleepForever();
    return 0;
}
