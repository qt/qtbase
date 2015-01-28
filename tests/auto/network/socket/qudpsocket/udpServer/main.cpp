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
#include <QtNetwork>

class Server : public QObject
{
    Q_OBJECT
public:
    Server(int port)
    {
        connect(&serverSocket, SIGNAL(readyRead()),
                this, SLOT(sendEcho()));
        if (serverSocket.bind(QHostAddress::Any, port,
                              QUdpSocket::ReuseAddressHint
                              | QUdpSocket::ShareAddress)) {
            printf("OK\n");
        } else {
            printf("FAILED\n");
        }
        fflush(stdout);
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

    Server server(app.arguments().at(1).toInt());

    return app.exec();
}

#include "main.moc"
