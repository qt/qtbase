// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtCore>
#include <QtNetwork>
#if defined(Q_OS_WIN) && defined(Q_CC_MSVC)
#  include <crtdbg.h>
#endif
#ifdef Q_OS_UNIX
#  include <unistd.h>
#endif

int main(int argc, char *argv[])
{
     // Windows: Suppress crash notification dialog.
#if defined(Q_OS_WIN) && defined(Q_CC_MSVC)
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif
    QCoreApplication app(argc, argv);

    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 49199)) {
        fprintf(stderr, "Failed to listen: %s\n", server.errorString().toLatin1().constData());
        if (server.serverError() == QTcpSocket::AddressInUseError) {
            // let's see if we can find the process that would be holding this
            // still open
#ifdef Q_OS_LINUX
            static const char *ss_args[] = {
                "ss", "-nap", "sport", "=", ":49199", nullptr
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
