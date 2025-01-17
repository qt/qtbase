// Copyright (C) 2025 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QCoreApplication>
#include <QSemaphore>
#include <QThread>

#ifdef Q_OS_UNIX
#  include <poll.h>
#  include <signal.h>
#  include <unistd.h>

int ready_pipe[2] = { -1, -1 };
static void setUpAbortControl()
{
    pipe(ready_pipe);

    struct sigaction sa = {};
#  ifdef SA_RESETHAND
    sa.sa_flags = SA_RESETHAND; // reset to SIG_DFL after first use
#  endif
    sa.sa_handler = [](int) {
        write(ready_pipe[1], "", 1);    // release main thread
        poll(nullptr, 0, -1);           // hang forever
    };
    sigaction(SIGABRT, &sa, nullptr);
}

static void wait()
{
    char c;
    read(ready_pipe[0], &c, 1);
}
#else
static void setUpAbortControl()
{
    qFatal("Don't know how to do this - implement me, if it is possible");
}
static void wait()
{
    Q_UNREACHABLE();
}
#endif

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (qEnvironmentVariableIntValue("QT_FATAL_WARNINGS") != 1) {
        fprintf(stderr, "QT_FATAL_WARNINGS=1 is not set\n");
        return EXIT_FAILURE;
    }

    setUpAbortControl();
    QThread *thr = QThread::create([] {
        qWarning("warning from aux thread");
    });
    thr->start();
    wait();
    qWarning("warning from main thread");
}
