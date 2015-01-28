/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


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
        if (socket.error() == QLocalSocket::ServerNotFoundError) {
            if (connectTimer.elapsed() > 5000) {
                fprintf(stderr, "client: server not found. Giving up.\n");
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
