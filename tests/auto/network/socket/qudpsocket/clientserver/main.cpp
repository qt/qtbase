/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include <QtNetwork>

class ClientServer : public QUdpSocket
{
    Q_OBJECT
public:
    enum Type {
        ConnectedClient,
        UnconnectedClient,
        Server
    };

    ClientServer(Type type, const QString &host, quint16 port)
        : type(type)
    {
        switch (type) {
        case Server:
            if (bind(0, ShareAddress | ReuseAddressHint)) {
                printf("%d\n", localPort());
            } else {
                printf("XXX\n");
            }
            break;
        case ConnectedClient:
            connectToHost(host, port);
            startTimer(250);
            printf("ok\n");
            break;
        case UnconnectedClient:
            peerAddress = host;
            peerPort = port;
            if (bind(QHostAddress::Any, port + 1, ShareAddress | ReuseAddressHint)) {
                startTimer(250);
                printf("ok\n");
            } else {
                printf("XXX\n");
            }
            break;
        }
        fflush(stdout);

        connect(this, SIGNAL(readyRead()), this, SLOT(readTestData()));
    }

protected:
    void timerEvent(QTimerEvent *event)
    {
        static int n = 0;
        switch (type) {
        case ConnectedClient:
            write(QByteArray::number(n++));
            break;
        case UnconnectedClient:
            writeDatagram(QByteArray::number(n++), peerAddress, peerPort);
            break;
        default:
            break;
        }

        QUdpSocket::timerEvent(event);
    }

private slots:
    void readTestData()
    {
        printf("readData()\n");
        switch (type) {
        case ConnectedClient: {
            while (bytesAvailable() || hasPendingDatagrams()) {
                QByteArray data = readAll();
                printf("got %d\n", data.toInt());
            }
            break;
        }
        case UnconnectedClient: {
            while (hasPendingDatagrams()) {
                QByteArray data;
                data.resize(pendingDatagramSize());
                readDatagram(data.data(), data.size());
                printf("got %d\n", data.toInt());
            }
            break;
        }
        case Server: {
            while (hasPendingDatagrams()) {
                QHostAddress sender;
                quint16 senderPort;
                QByteArray data;
                data.resize(pendingDatagramSize());
                readDatagram(data.data(), data.size(), &sender, &senderPort);
                printf("got %d\n", data.toInt());
                printf("sending %d\n", data.toInt() * 2);
                writeDatagram(QByteArray::number(data.toInt() * 2), sender, senderPort);
            }
            break;
        }
        }
        fflush(stdout);
    }

private:
    Type type;
    QHostAddress peerAddress;
    quint16 peerPort;
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    ClientServer::Type type;
    if (app.arguments().size() < 4)  {
        qDebug("usage: %s [ConnectedClient <server> <port>|UnconnectedClient <server> <port>|Server]", argv[0]);
        return 1;
    }

    QString arg = app.arguments().at(1).trimmed().toLower();
    if (arg == "connectedclient") {
        type = ClientServer::ConnectedClient;
    } else if (arg == "unconnectedclient") {
        type = ClientServer::UnconnectedClient;
    } else if (arg == "server") {
        type = ClientServer::Server;
    } else {
        qDebug("usage: %s [ConnectedClient <server> <port>|UnconnectedClient <server> <port>|Server]", argv[0]);
        return 1;
    }

    ClientServer clientServer(type, app.arguments().at(2),
                              app.arguments().at(3).toInt());

    return app.exec();
}

#include "main.moc"
