/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <qcoreapplication.h>
#include <qlocalsocket.h>
#include <qlocalserver.h>
#include <qsystemsemaphore.h>

const QString serverName = QStringLiteral("qlocalsocket_autotest");
const QByteArray testData("test");
QSystemSemaphore *semaphore = 0;

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
    semaphore->release();
    for (int i = 1; i <= numberOfConnections; ++i) {
        printf("server: wait for connection %d\n", i);
        if (!server->waitForNewConnection(30000)) {
            semaphore->acquire();
            fprintf(stderr, "server: waitForNewConnection failed: %s\n",
                    qPrintable(server->errorString()));
            return false;
        }
        QLocalSocket *socket = server->nextPendingConnection();
        printf("server: writing \"%s\"\n", testData.data());
        socket->write(testData);
        if (!socket->waitForBytesWritten()) {
            semaphore->acquire();
            fprintf(stderr, "server: waitForBytesWritten failed: %s\n",
                    qPrintable(socket->errorString()));
            return false;
        }
        printf("server: data written\n");
        if (socket->error() != QLocalSocket::UnknownSocketError) {
            semaphore->acquire();
            fprintf(stderr, "server: socket error %d\n", socket->error());
            return false;
        }
    }
    semaphore->acquire();
    return true;
}

bool runClient()
{
    semaphore->acquire();   // wait until the server is up and running
    semaphore->release();

    QLocalSocket socket;
    printf("client: connecting to \"%s\"\n", qPrintable(serverName));
    socket.connectToServer(serverName, QLocalSocket::ReadWrite);
    if (!socket.waitForConnected()) {
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
    QSystemSemaphore s("tst_qlocalsocket_socketprocess");
    semaphore = &s;
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
