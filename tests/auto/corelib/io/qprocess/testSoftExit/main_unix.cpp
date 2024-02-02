// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

int main()
{
    struct sigaction noaction;
    memset(&noaction, 0, sizeof(noaction));
    noaction.sa_handler = SIG_IGN;
    ::sigaction(SIGTERM, &noaction, 0);

    printf("Ready\n");
    fflush(stdout);

    for (int i = 0; i < 5; ++i)
        sleep(1);
    return 0;
}
