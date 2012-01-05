/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
// Use if you need

class DummyHttpServer : public QTcpServer
{
    Q_OBJECT
public:
    DummyHttpServer() : phase(Header)
        { listen(); }

protected:
    enum {
        Header,
        Data1,
        Data2,
        Close
    } phase;
    void incomingConnection(int socketDescriptor)
    {
        QSslSocket *socket = new QSslSocket(this);
        socket->setSocketDescriptor(socketDescriptor, QAbstractSocket::ConnectedState);
        socket->ignoreSslErrors();
        socket->startServerEncryption();
        connect(socket, SIGNAL(readyRead()), SLOT(handleReadyRead()));
    }

public slots:
    void handleReadyRead()
    {
        QTcpSocket *socket = static_cast<QTcpSocket *>(sender());
        socket->readAll();
        if (phase != Header)
            return;

        phase = Data1;
        static const char header[] =
            "HTTP/1.0 200 OK\r\n"
            "Date: Fri, 07 Sep 2007 12:33:18 GMT\r\n"
            "Server: Apache\r\n"
            "Expires:\r\n"
            "Cache-Control:\r\n"
            "Pragma:\r\n"
            "Last-Modified: Thu, 06 Sep 2007 08:52:06 +0000\r\n"
            "Etag: a700f59a6ccb1ad39af68d998aa36fb1\r\n"
            "Vary: Accept-Encoding\r\n"
            "Content-Length: 6560\r\n"
            "Connection: close\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "\r\n";


        socket->write(header, sizeof header - 1);
        connect(socket, SIGNAL(bytesWritten(qint64)), SLOT(handleBytesWritten()), Qt::QueuedConnection);
    }

    void handleBytesWritten()
    {
        QTcpSocket *socket = static_cast<QTcpSocket *>(sender());
        if (socket->bytesToWrite() != 0)
            return;

        if (phase == Data1) {
            QByteArray data(4096, 'a');
            socket->write(data);
            phase = Data2;
        } else if (phase == Data2) {
            QByteArray data(2464, 'a');
            socket->write(data);
            phase = Close;
        } else {
            //socket->disconnectFromHost();
            //socket->deleteLater();
        }
    }
};
