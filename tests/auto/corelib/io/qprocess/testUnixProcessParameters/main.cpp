// Copyright (C) 2023 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <string_view>

#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s command [extra]\nSee source code for commands\n",
               argv[0]);
        return EXIT_FAILURE;
    }

    std::string_view cmd = argv[1];
    errno = 0;

    if (cmd.size() == 0) {
        // just checking that we did get here
        return EXIT_SUCCESS;
    }

    if (cmd == "reset-sighand") {
        // confirm it was not ignored
        struct sigaction action;
        sigaction(SIGUSR1, nullptr, &action);
        if (action.sa_handler == SIG_DFL)
            return EXIT_SUCCESS;
        fprintf(stderr, "SIGUSR1 is SIG_IGN\n");
        return EXIT_FAILURE;
    }

    if (cmd == "ignore-sigpipe") {
        // confirm it was ignored
        struct sigaction action;
        sigaction(SIGPIPE, nullptr, &action);
        if (action.sa_handler == SIG_IGN)
            return EXIT_SUCCESS;
        fprintf(stderr, "SIGPIPE is SIG_DFL\n");
        return EXIT_FAILURE;
    }

    if (cmd == "std-file-descriptors") {
        int fd = atoi(argv[2]);
        if (close(fd) < 0 && errno == EBADF)
            return EXIT_SUCCESS;
        fprintf(stderr, "%d is a valid file descriptor\n", fd);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Unknown command \"%s\"", cmd.data());
    return EXIT_FAILURE;
}
