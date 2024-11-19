// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtNetwork/qudpsocket.h>
#include <QtNetwork/qhostaddress.h>

#include <QtCore/qcoreapplication.h>

class Server : public QObject
{
    Q_OBJECT
public:

    Server() { connect(&serverSocket, &QIODevice::readyRead, this, &Server::sendEcho); }

    bool bind(quint16 port)
    {
        const bool result = serverSocket.bind(QHostAddress::Any, port,
                                              QUdpSocket::ReuseAddressHint
                                              | QUdpSocket::ShareAddress);
        if (result) {
            printf("OK\n");
        } else {
            printf("FAILED: %s\n", qPrintable(serverSocket.errorString()));
        }
        fflush(stdout);
        return result;
    }

private slots:
    void sendEcho()
    {
        QHostAddress senderAddress;
        quint16 senderPort;

        char data[1024];
        qint64 bytes = serverSocket.readDatagram(data, sizeof(data), &senderAddress, &senderPort);
        if (bytes == 1 && data[0] == '\0')
            QCoreApplication::instance()->quit();

        for (int i = 0; i < bytes; ++i)
            data[i] += 1;
        serverSocket.writeDatagram(data, bytes, senderAddress, senderPort);
    }

private:
    QUdpSocket serverSocket;
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStringList arguments = QCoreApplication::arguments();
    arguments.pop_front();
    quint16 port = 0;
    if (!arguments.isEmpty())
        port = arguments.constFirst().toUShort();
    if (!port) {
        printf("Specify port number\n");
        return -1;
    }

    Server server;
    if (!server.bind(port))
        return -2;

    return app.exec();
}

#include "main.moc"
