/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

