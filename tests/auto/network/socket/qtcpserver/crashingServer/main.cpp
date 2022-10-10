// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtCore>
#include <QtNetwork>
#if defined(Q_OS_WIN) && defined(Q_CC_MSVC)
#  include <crtdbg.h>
#endif
#ifdef Q_OS_UNIX
#  include <sys/resource.h>
#  include <unistd.h>
#endif

int main(int argc, char *argv[])
{
#if defined(Q_OS_WIN) && defined(Q_CC_MSVC)
    // Windows: Suppress crash notification dialog.
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#elif defined(RLIMIT_CORE)
    // Unix: set our core dump limit to zero to request no dialogs.
    if (struct rlimit rlim; getrlimit(RLIMIT_CORE, &rlim) == 0) {
        rlim.rlim_cur = 0;
        setrlimit(RLIMIT_CORE, &rlim);
    }
#endif

    QCoreApplication app(argc, argv);
    if (argc < 1) {
        fprintf(stderr, "Need a port number\n");
        return 1;
    }

    int port = QByteArrayView(argv[1]).toInt();
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, port)) {
        fprintf(stderr, "Failed to listen: %s\n", server.errorString().toLatin1().constData());
        if (server.serverError() == QTcpSocket::AddressInUseError) {
            // let's see if we can find the process that would be holding this
            // still open
#ifdef Q_OS_LINUX
            static const char *ss_args[] = {
                "ss", "-nap", "sport", "=", argv[1], nullptr
            };
            dup2(STDERR_FILENO, STDOUT_FILENO);
            execvp(ss_args[0], const_cast<char **>(ss_args));
#endif
        }
        return 1;
    }

    printf("Listening\n");
    fflush(stdout);

    server.waitForNewConnection(5000);
    qFatal("Crash");
    return 0;
}
