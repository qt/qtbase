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


#ifndef MINIHTTPSERVER_H
#define MINIHTTPSERVER_H

#include <QtCore/QThread>
#include <QtCore/QFile>
#include <QtCore/QTimer>

class QFile;
class QSemaphore;
class QTcpServer;
class QTcpSocket;

class MiniHttpServer : public QThread
{
    Q_OBJECT
public:
    explicit MiniHttpServer(QObject *parent = 0);
    ~MiniHttpServer();

    int port() { return portnum; }

protected:
    void run();

private slots:
    void handleConnection();

private:
    QTcpServer *server;
    QObject *quitObject;
    QSemaphore *readyToGo;
    int portnum;
};

class MiniHttpServerConnection: public QObject
{
    Q_OBJECT
    QTcpSocket * const socket;
    QFile source;
    QTimer timeout;
    QByteArray buffer;
    bool connectionClose;
public:
    explicit MiniHttpServerConnection(QTcpSocket *socket);

    void sendError500();
    void sendError404();
    void handlePendingRequest();

public slots:
    void handleReadyRead();
    void handleBytesWritten();
    void handleDisconnected();
    void handleTimeout();
};

#endif // MINIHTTPSERVER_H
