// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtNetwork/qtcpserver.h>
#include <QtNetwork/qtcpsocket.h>
#include <QtNetwork/qhostaddress.h>

#include <QtCore/qcoreapplication.h>

#ifdef Q_OS_UNIX
#  include <unistd.h>
#endif

int main(int argc, char *argv[])
{
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
            static const char *const ss_args[] = {
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
