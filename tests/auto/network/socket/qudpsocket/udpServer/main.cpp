/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtNetwork>

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
