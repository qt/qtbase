// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore>
#include <QtNetwork>

int main(int argc, char **argv) {

    QCoreApplication app(argc, argv);

    // This example connects to localhost, but note that the host can
    // be any host reachable from the client using webscokets, at any port.
    QString hostName = "localhost";
    int port = 1515;
    qDebug() << "## This example connects to a server at" << hostName << "port" << port << ","
             << "where it expects to find a WebSockify server, which forwards to the fortune server.";

    auto echo = [hostName, port]() {
        qDebug() << "Connecting to" << hostName << port;

        QTcpSocket socket;
        socket.connectToHost(hostName, port);
        bool connected = socket.waitForConnected(3000);
        if (!connected) {
            qDebug() << "connect failure";
            return;
        }

        qDebug() << "Connected";
        socket.write("echo:Hello, echo server!;");
        socket.flush();

        qDebug() << "Calling waitForReadyRead()";
        socket.waitForReadyRead(20000);
        QByteArray data = socket.readAll();
        qDebug() << "Got echo:" << data;

        socket.disconnectFromHost();
        socket.deleteLater();
        qDebug() << "Disconnected";
    };

    QThread thread;
    QObject::connect(&thread, &QThread::started, [echo](){
        echo();
    });
    thread.start();

    app.exec();
}
