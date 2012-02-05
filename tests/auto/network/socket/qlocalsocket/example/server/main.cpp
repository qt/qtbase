/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qlocalserver.h"
#include "qlocalsocket.h"

#include <qcoreapplication.h>
#include <qdebug.h>

class EchoServer : public QLocalServer
{
public:
    void incomingConnection(int socketDescriptor) {
        QLocalServer::incomingConnection(socketDescriptor);
        QLocalSocket *socket = nextPendingConnection();
        socket->open(QIODevice::ReadWrite);

        qDebug() << "server connection";

        do {
            const int Timeout = 5 * 1000;
            while (!socket->canReadLine()) {
                if (!socket->waitForReadyRead(Timeout)) {
                    return;
                }
            }
            char str[100];
            int n = socket->readLine(str, 100);
	    if (n < 0) {
                perror("recv");
                break;
            }
            if (n == 0)
                break;
	    qDebug() << "Read" << str;
            if ("exit" == str)
                qApp->quit();

            if (socket->write(str, 100) < 0) {
                perror("send");
                break;
            }
        } while (true);
    }
};

#define SOCK_PATH "echo_socket"

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);

    EchoServer echoServer;
    echoServer.listen(SOCK_PATH);

    return application.exec();
}

