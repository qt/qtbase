// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtCore>
#include <QtNetwork>

const int timeout = 60 * 1000;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QTcpServer server;
    QObject::connect(&server, &QTcpServer::newConnection, [&server](){
        qDebug() << "new connection";

        QByteArray *receiveBuffer = new QByteArray();

        QTcpSocket *socket = server.nextPendingConnection();
        QObject::connect(socket, &QIODevice::readyRead, [socket, receiveBuffer](){

            // This implements a very simple command protocol, where the server
            // processes a stream of commands delimited by ';', and then performs
            // an action in reply. The supported commands with actions are:
            //
            //  echo:<message>;  writes the received <message> back
            //  close;           closes the socket
            //

            // We might receive multiple or partial commands; read all available data
            // and then scan the buffer for complete commands.
            QByteArray newData = socket->readAll();
            *receiveBuffer += newData;

            int pos = receiveBuffer->indexOf(";");
            while (pos != -1) {
                QByteArray command = receiveBuffer->left(pos);
                receiveBuffer->remove(0, pos + 1);
                pos = receiveBuffer->indexOf(";");

                if (command.startsWith("echo")) {
                    // Echo expects echo:<message>
                    QList<QByteArray> parts = command.split(':');
                    QByteArray reply = parts.last() + ';';
                    qDebug() << "Command: echo:" << parts.last();
                    socket->write(reply);
                    socket->flush();

                } else if (command.startsWith("close")) {
                    qDebug() << "Command: close";
                    socket->write("bye!;");
                    socket->flush();
                    socket->close();
                    break;
                } else {
                    qDebug() << "Unknown command:" << command;
                }
            }
        });

        QObject::connect(socket, &QAbstractSocket::disconnected, [socket, receiveBuffer](){
            delete receiveBuffer;
            socket->deleteLater();
        });
    });

    // This is example is intended to be used together with WebSockify on
    // the server and acts as a counterpart to the client examples which
    // run in the browser. (This example does not run in the browser).

    qDebug() << "\nStarting echo server at port 1516. You should now start the"
             << "\nWebSockify forwarding server, and then connect from one of"
             << "\nthe client examples."
             << "\n  websockify 1515 localhost:1516";

    server.listen(QHostAddress::Any, 1516);

    return app.exec();
}

