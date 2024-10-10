// Copyright (C) 2023 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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

    if (cmd == "reset-ids") {
        if (getuid() == geteuid() && getgid() == getegid())
            return EXIT_SUCCESS;
        fprintf(stderr, "Real: %d %d; Effective: %d %d\n",
                getuid(), getgid(), geteuid(), getegid());
        return EXIT_FAILURE;
    }

    if (cmd == "reset-sighand") {
        bool ok = true;

        // confirm our signal block mask is empty
        sigset_t set;
        sigprocmask(SIG_SETMASK, nullptr, &set);
        for (int signo = 1; signo < NSIG; ++signo) {
            if (sigismember(&set, signo)) {
                fprintf(stderr, "'%s' is blocked.\n", strsignal(signo));
                ok = false;
            }
        }

        // confirm SIGUSR1 was not ignored
        struct sigaction action;
        sigaction(SIGUSR1, nullptr, &action);
        if (action.sa_handler != SIG_DFL) {
            fprintf(stderr, "SIGUSR1 is SIG_IGN\n");
            ok = false;
        }
        return ok ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    if (cmd == "ignore-sigpipe") {
        // confirm SIGPIPE was ignored
        struct sigaction action;
        sigaction(SIGPIPE, nullptr, &action);
        if (action.sa_handler == SIG_IGN)
            return EXIT_SUCCESS;
        fprintf(stderr, "SIGPIPE is SIG_DFL\n");
        return EXIT_FAILURE;
    }

    if (cmd == "file-descriptors") {
        int fd = atoi(argv[2]);
        if (close(fd) < 0 && errno == EBADF)
            return EXIT_SUCCESS;
        fprintf(stderr, "%d is a valid file descriptor\n", fd);
        return EXIT_FAILURE;
    }

    if (cmd == "file-descriptors2") {
        int fd1 = atoi(argv[2]);    // should be open
        int fd2 = atoi(argv[3]);    // should be closed
        if (close(fd1) < 0)
            fprintf(stderr, "%d was not a valid file descriptor\n", fd1);
        if (close(fd2) == 0 || errno != EBADF)
            fprintf(stderr, "%d is a valid file descriptor\n", fd2);
        return EXIT_SUCCESS;
    }

    if (cmd == "noctty") {
        int fd = open("/dev/tty", O_RDONLY);
        if (fd == -1)
            return EXIT_SUCCESS;
        fprintf(stderr, "Could open /dev/tty\n");
        return EXIT_FAILURE;
    }

    if (cmd == "no-coredumps") {
        struct rlimit corelimit;
        if (getrlimit(RLIMIT_CORE, &corelimit) != 0) {
            // this shouldn't happen, so just assume it worked
            return EXIT_SUCCESS;
        }
        if (corelimit.rlim_cur == 0)
            return EXIT_SUCCESS;
        fprintf(stderr, "rlim_cur = %lx\n", (unsigned long)corelimit.rlim_cur);
        return EXIT_FAILURE;
    }

    if (cmd == "setsid") {
        pid_t pgid = getpgrp();
        if (pgid == getpid())
            return EXIT_SUCCESS;
        fprintf(stderr, "Process group was %d\n", pgid);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Unknown command \"%s\"", cmd.data());
    return EXIT_FAILURE;
}
