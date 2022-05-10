// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <qcoreapplication.h>
#include <qelapsedtimer.h>
#include <qeventloop.h>
#include <qlocalsocket.h>
#include <qlocalserver.h>
#include <qtimer.h>

const QString serverName = QStringLiteral("qlocalsocket_autotest");
const QByteArray testData("test");

bool runServer(int numberOfConnections)
{
    QLocalServer *server = new QLocalServer(qApp);
    if (!server->removeServer(serverName)) {
        fprintf(stderr, "server: cannot remove server: %s\n", qPrintable(server->errorString()));
        return false;
    }
    printf("server: listen on \"%s\"\n", qPrintable(serverName));
    if (!server->listen(serverName)) {
        fprintf(stderr, "server: listen failed: %s\n", qPrintable(server->errorString()));
        return false;
    }
    for (int i = 1; i <= numberOfConnections; ++i) {
        printf("server: wait for connection %d\n", i);
        if (!server->waitForNewConnection(30000)) {
            fprintf(stderr, "server: waitForNewConnection failed: %s\n",
                    qPrintable(server->errorString()));
            return false;
        }
        QLocalSocket *socket = server->nextPendingConnection();
        printf("server: writing \"%s\"\n", testData.data());
        socket->write(testData);
        if (!socket->waitForBytesWritten()) {
            fprintf(stderr, "server: waitForBytesWritten failed: %s\n",
                    qPrintable(socket->errorString()));
            return false;
        }
        printf("server: data written\n");
        if (socket->error() != QLocalSocket::UnknownSocketError) {
            fprintf(stderr, "server: socket error %d\n", socket->error());
            return false;
        }
    }
    return true;
}

bool runClient()
{
    QLocalSocket socket;
    printf("client: connecting to \"%s\"\n", qPrintable(serverName));
    QElapsedTimer connectTimer;
    connectTimer.start();
    forever {
        socket.connectToServer(serverName, QLocalSocket::ReadWrite);
        if (socket.waitForConnected())
            break;
        if (socket.error() == QLocalSocket::ServerNotFoundError
            || socket.error() == QLocalSocket::ConnectionRefusedError) {
            if (connectTimer.elapsed() > 5000) {
                fprintf(stderr, "client: server not found or connection refused. Giving up.\n");
                return false;
            }
            printf("client: server not found. Trying again...\n");
            QEventLoop eventLoop;
            QTimer::singleShot(500, &eventLoop, SLOT(quit()));
            eventLoop.exec();
            continue;
        }
        fprintf(stderr, "client: waitForConnected failed: %s\n",
                qPrintable(socket.errorString()));
        return false;
    }
    printf("client: connected\n");
    if (!socket.waitForReadyRead()) {
        fprintf(stderr, "client: waitForReadyRead failed: %s\n",
                qPrintable(socket.errorString()));
        return false;
    }
    printf("client: data is available for reading\n");
    const QByteArray data = socket.readLine();
    printf("client: received \"%s\"\n", data.data());
    if (data != testData) {
        fprintf(stderr, "client: received unexpected data\n");
        return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc < 2)
        return EXIT_FAILURE;
    if (strcmp(argv[1], "--server") == 0) {
        if (argc < 3) {
            fprintf(stderr, "--server needs the number of incoming connections\n");
            return EXIT_FAILURE;
        }
        bool ok;
        int n = QByteArray(argv[2]).toInt(&ok);
        if (!ok) {
            fprintf(stderr, "Cannot convert %s to a number.\n", argv[2]);
            return EXIT_FAILURE;
        }
        if (!runServer(n))
            return EXIT_FAILURE;
    } else if (strcmp(argv[1], "--client") == 0) {
        if (!runClient())
            return EXIT_FAILURE;
    } else {
        fprintf(stderr, "unknown command line option: %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
